#ifndef ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H
#define ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H

#include "Core/Platform/PlatformTypes.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>


namespace AE::Threading
{
/**
 * @brief Fixed-size thread pool used for parallel range processing.
 *
 * The thread pool owns a persistent set of worker threads that are created
 * once during initialization and reused for the entire lifetime of the pool.
 * Workers remain blocked on a condition variable while idle and are awakened
 * whenever a new parallel job is submitted.
 *
 * Unlike a general-purpose task scheduler, this implementation is specialized
 * for executing a single ParallelFor operation at a time. A submitted task
 * represents a contiguous range of items which is dynamically divided into
 * smaller chunks and distributed across worker threads.
 *
 * Work distribution is performed using an atomic counter instead of assigning
 * fixed ranges to each thread. Whenever a worker finishes processing its
 * current chunk, it atomically acquires the next available range until all
 * items have been processed. This approach provides automatic load balancing
 * when different chunks require different execution times.
 *
 * Thread synchronization is implemented using condition variables:
 *
 * - WorkAvailable wakes workers when a new ParallelFor begins.
 * - WorkFinished allows the calling thread to wait until all workers
 *   complete the current task.
 *
 * Every submitted task increments an internal generation counter. Worker
 * threads compare this generation value with the last processed one to detect
 * newly submitted work without relying solely on boolean state flags.
 *
 * This class is implemented as a singleton because thread creation is an
 * expensive operation and worker threads are intended to be reused across
 * the application's lifetime.
 *
 * Thread safety:
 * - Public functions are thread-safe.
 * - Only one ParallelFor operation may execute at any given time.
 * - Worker threads never execute user code while holding the internal mutex.
 *
 * 	@warning
 *	Function must be thread-safe.
 *
 * The callback must not:
 * - modify shared state without synchronization;
 * - resize containers being processed;
 * - call ParallelFor recursively;
 *
 * If the callback throws on a worker thread, the first exception is captured
 * and rethrown on the calling thread after all worker chunks have completed.
 */
class FThreadPool
{
public:
	/**
	 * @brief Returns the global thread pool instance.
	 *
	 * The pool is lazily created on first use and remains alive until
	 * application shutdown.
	 *
	 * @return Global thread pool instance.
	 */
	static FThreadPool& Get();

	FThreadPool(const FThreadPool&) = delete;
	FThreadPool& operator=(const FThreadPool&) = delete;

	/**
	 * @brief Stops all worker threads and releases internal resources.
	 */
	~FThreadPool();

	/**
	 * @brief Returns the current number of worker threads.
	 *
	 * @return Number of active worker threads.
	 */
	SizeT WorkerCount() const;

	/**
	 * @brief Recreates the worker pool with a new number of threads.
	 *
	 * If a ParallelFor operation is currently executing, this function blocks
	 * until all workers finish processing the current task before recreating
	 * the pool.
	 *
	 * A minimum of one worker thread is always maintained.
	 *
	 * @param WorkerCount Desired number of worker threads.
	 */
	void SetWorkerCount(SizeT WorkerCount);

	/**
	 * @brief Executes a function in parallel over a contiguous range of items.
	 *
	 * The range [0, ItemCount) is automatically divided into multiple chunks.
	 * Worker threads dynamically acquire chunks until every item has been
	 * processed.
	 *
	 * If the workload is too small to benefit from parallel execution, the
	 * function is executed synchronously on the calling thread.
	 *
	 * This function blocks until every worker has completed its assigned work.
	 *
	 * @param ItemCount Total number of items to process.
	 * @param MinItemsPerJob Minimum number of items assigned to a single chunk.
	 * @param Function Callback invoked for every chunk. Receives the half-open
	 *        interval [Begin, End).
	 */
	void ParallelFor(SizeT ItemCount, SizeT MinItemsPerJob, const std::function<void(SizeT begin, SizeT end)>& Function);

private:
	/**
	 * @brief Creates the worker threads and starts the processing loop.
	 *
	 * Each worker immediately enters WorkerLoop() and waits for work.
	 *
	 * @param WorkerCount Number of worker threads to create.
	 */
	FThreadPool();

	/**
	 * @brief Starts the worker threads.
	 *
	 * @param WorkerCount Number of worker threads to create.
	 */
	void Start(SizeT WorkerCount);

	/**
	 * @brief Stops all worker threads and waits for them to exit.
	 */
	void Stop();

	/**
	 * @brief Main execution loop for every worker thread.
	 *
	 * Workers sleep while no work is available. When a new ParallelFor
	 * operation begins, each worker repeatedly acquires the next available
	 * chunk using an atomic counter and executes the user callback until
	 * all work has been consumed.
	 */
	void WorkerLoop();

	/** Protects all shared state except atomic work distribution. */
	mutable std::mutex Mutex;

	/** Signals that a new ParallelFor operation has been submitted. */
	std::condition_variable WorkAvailable;

	/** Signals completion of the current parallel task. */
	std::condition_variable WorkFinished;

	/** Collection of persistent worker threads. */
	std::vector<std::thread> Workers;

	/** Callback executed by every worker during the current ParallelFor. */
	std::function<void(SizeT Begin, SizeT End)> CurrentTask;

	/** First exception thrown by the current task, rethrown by ParallelFor. */
	std::exception_ptr TaskException = nullptr;

	/**
	 * @brief Atomic index of the next unprocessed chunk.
	 *
	 * Workers obtain work by atomically incrementing this value,
	 * providing dynamic load balancing without additional locking.
	 */
	std::atomic<SizeT> NextItem{0};

	/** Total number of items in the current task. */
	SizeT TaskCount = 0;

	/** Number of items assigned to each chunk. */
	SizeT ChunkSize = 1;

	/** Number of workers still processing the current task. */
	SizeT ActiveWorkers = 0;

	/**
	 * @brief Monotonically increasing task generation.
	 *
	 * Incremented whenever a new ParallelFor operation begins. Workers use
	 * this value to detect newly submitted work.
	 */
	SizeT WorkGeneration = 0;

	/** Indicates that a ParallelFor operation is currently executing. */
	bool HasWork = false;

	/** Requests all worker threads to terminate. */
	bool Stopping = false;
};
} // namespace AE::Threading

#endif
