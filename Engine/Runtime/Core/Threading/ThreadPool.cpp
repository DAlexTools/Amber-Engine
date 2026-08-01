#include "Core/Threading/ThreadPool.h"

#include <algorithm>
#include <stdexcept>

namespace AE::Threading
{

FThreadPool& FThreadPool::Get()
{
	static FThreadPool Pool;
	return Pool;
}

FThreadPool::FThreadPool()
{
	// Keep one hardware thread available for the main application thread.
	const unsigned int HardwareThreads = std::thread::hardware_concurrency();
	const SizeT WorkerCount = HardwareThreads > 2u ? HardwareThreads - 1u : 1u;
	Start(WorkerCount);
}

FThreadPool::~FThreadPool()
{
	Stop();
}

SizeT FThreadPool::WorkerCount() const
{
	std::lock_guard<std::mutex> Lock(Mutex);
	return Workers.size();
}

void FThreadPool::SetWorkerCount(SizeT WorkerCount)
{
	WorkerCount = std::max<SizeT>(1u, WorkerCount);

	{
		std::unique_lock<std::mutex> Lock(Mutex);

		// Wait until the current task completes before recreating workers.
		WorkFinished.wait(Lock, [this]()
						  { return !HasWork; });

		if (Workers.size() == WorkerCount)
		{
			// Avoid unnecessary thread recreation when the count is unchanged.
			return;
		}
	}
	// Recreate worker threads with the new count.
	Stop();
	Start(WorkerCount);
}

void FThreadPool::ParallelFor(SizeT ItemCount, SizeT MinItemsPerJob, const std::function<void(SizeT begin, SizeT end)>& Function)
{
	// Executes a range-based task by splitting the workload into dynamically
	// scheduled chunks processed by worker threads. The function blocks until
	// all chunks have been completed.

	//-------------------------------------------------------------------------
	// Validate input.
	//-------------------------------------------------------------------------
	if (ItemCount == 0u)
	{
		return;
	}

	if (!Function)
	{
		throw std::invalid_argument("FThreadPool::ParallelFor requires a valid callback.");
	}

	//-------------------------------------------------------------------------
	// Calculate work distribution.
	//
	// Determine how many chunks should be created based on the available
	// worker threads and the minimum chunk size.
	//-------------------------------------------------------------------------
	const SizeT LocalWorkerCount = WorkerCount();
	MinItemsPerJob = std::max<SizeT>(1u, MinItemsPerJob);
	const SizeT JobCount = std::min(LocalWorkerCount, (ItemCount + MinItemsPerJob - 1u) / MinItemsPerJob);

	//-------------------------------------------------------------------------
	// Execute small workloads directly.
	//
	// For small tasks, thread synchronization overhead can be higher than
	// the actual processing time, so execute them on the calling thread.
	//-------------------------------------------------------------------------
	if (JobCount < 2u)
	{
		Function(0u, ItemCount);
		return;
	}

	//-------------------------------------------------------------------------
	// Submit new task.
	//
	// Wait until the previous ParallelFor operation has finished, then
	// initialize shared task data consumed by worker threads.
	//-------------------------------------------------------------------------
	{
		std::unique_lock<std::mutex> Lock(Mutex);
		WorkFinished.wait(Lock, [this]()
						  { return !HasWork; });

		CurrentTask = Function;
		TaskException = nullptr;
		TaskCount = ItemCount;
		ChunkSize = (ItemCount + JobCount - 1u) / JobCount;
		NextItem.store(0u, std::memory_order_relaxed);

		// Track the number of workers participating in this task.
		// The last worker reaching zero signals completion.
		ActiveWorkers = Workers.size();
		HasWork = true;

		// Publish a new task generation.
		// Workers compare this value with their local generation counter
		// to detect newly submitted work.
		++WorkGeneration;
	}

	//-------------------------------------------------------------------------
	// Wake worker threads.
	//-------------------------------------------------------------------------
	WorkAvailable.notify_all();

	//-------------------------------------------------------------------------
	// Wait for completion.
	//
	// The calling thread blocks until the last worker finishes processing
	// all chunks.
	//-------------------------------------------------------------------------
	std::exception_ptr Exception;
	{
		std::unique_lock<std::mutex> Lock(Mutex);
		WorkFinished.wait(Lock, [this]()
						  { return !HasWork; });

		//-------------------------------------------------------------------------
		// Cleanup task state.
		//-------------------------------------------------------------------------
		Exception = TaskException;
		TaskException = nullptr;
		CurrentTask = nullptr;
	}

	if (Exception)
	{
		std::rethrow_exception(Exception);
	}
}

void FThreadPool::Start(SizeT WorkerCount)
{
	std::lock_guard<std::mutex> Lock(Mutex);

	// Reserve storage to prevent vector reallocations while creating threads.
	Stopping = false;
	HasWork = false;
	ActiveWorkers = 0u;
	WorkGeneration = 0u;
	Workers.reserve(WorkerCount);

	// Create persistent worker threads.
	for (SizeT Index = 0; Index < WorkerCount; ++Index)
	{
		Workers.emplace_back(&FThreadPool::WorkerLoop, this);
	}
}

void FThreadPool::Stop()
{
	//-------------------------------------------------------------------------
	// Request shutdown.
	//
	// Change the state while holding the mutex so sleeping workers can
	// safely observe the stop request.
	//-------------------------------------------------------------------------
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Stopping = true;
		// Increment generation to wake sleeping workers.
		++WorkGeneration;
	}

	//-------------------------------------------------------------------------
	// Wake all workers.
	//-------------------------------------------------------------------------
	WorkAvailable.notify_all();

	//-------------------------------------------------------------------------
	// Wait for worker termination.
	//-------------------------------------------------------------------------
	for (std::thread& Worker : Workers)
	{
		if (Worker.joinable())
		{
			Worker.join();
		}
	}

	//-------------------------------------------------------------------------
	// Reset internal state.
	//-------------------------------------------------------------------------
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Workers.clear();
		CurrentTask = nullptr;
		TaskException = nullptr;
		NextItem.store(0u, std::memory_order_relaxed);
		TaskCount = 0u;
		ChunkSize = 1u;
		ActiveWorkers = 0u;
		WorkGeneration = 0u;
		HasWork = false;
		Stopping = false;
	}
}

void FThreadPool::WorkerLoop()
{
	SizeT ObservedGeneration = 0u;

	while (true)
	{
		std::function<void(SizeT begin, SizeT end)> Task;
		SizeT Count = 0u;
		SizeT Chunk = 1u;

		//-------------------------------------------------------------------------
		// Wait for new work.
		//
		// Worker threads sleep here while the pool is idle. They wake up when
		// a new task generation is published or when shutdown is requested.
		//-------------------------------------------------------------------------
		{
			std::unique_lock<std::mutex> Lock(Mutex);
			WorkAvailable.wait(Lock, [this, &ObservedGeneration]()
							   { return Stopping || WorkGeneration != ObservedGeneration; });

			if (Stopping)
			{
				return;
			}

			ObservedGeneration = WorkGeneration;

			// Copy task state locally.
			// Worker threads must release the mutex before executing user code
			// to avoid blocking other workers.
			Task = CurrentTask;
			Count = TaskCount;
			Chunk = ChunkSize;
		}

		//-------------------------------------------------------------------------
		// Process assigned chunks.
		//
		// Workers dynamically acquire chunks using an atomic counter. This
		// avoids fixed thread ownership and provides automatic load balancing.
		//-------------------------------------------------------------------------
		while (true)
		{
			// Atomically reserve the next chunk of work.
			// Multiple workers can request chunks simultaneously without locking.
			const SizeT Begin = NextItem.fetch_add(Chunk, std::memory_order_relaxed);
			if (Begin >= Count)
			{
				break;
			}
			const SizeT End = std::min(Begin + Chunk, Count);

			try
			{
				Task(Begin, End);
			}
			catch (...)
			{
				// Store the first exception thrown by a worker thread.
				// The exception will be rethrown on the calling thread after completion.
				std::lock_guard<std::mutex> Lock(Mutex);

				if (!TaskException)
				{
					TaskException = std::current_exception();
				}
			}
		}

		//-------------------------------------------------------------------------
		// Notify completion.
		//
		// The last worker finishing the task wakes the waiting thread.
		//-------------------------------------------------------------------------
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			if (ActiveWorkers > 0u)
			{
				--ActiveWorkers;
			}
			if (ActiveWorkers == 0u)
			{
				HasWork = false;
				WorkFinished.notify_one();
			}
		}
	}
}

} // namespace AE::Threading
