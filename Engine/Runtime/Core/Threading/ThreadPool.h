#ifndef ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H
#define ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H

#include "Core/Platform/PlatformTypes.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace AE::Threading
{

class ThreadPool
{
public:
	static ThreadPool& Get();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	~ThreadPool();

	SizeT WorkerCount() const;
	void SetWorkerCount(SizeT workerCount);
	void ParallelFor(SizeT itemCount, SizeT minItemsPerJob, const std::function<void(SizeT begin, SizeT end)>& function);

private:
	ThreadPool();

	void Start(SizeT workerCount);
	void Stop();
	void WorkerLoop();

	mutable std::mutex mutex;
	std::condition_variable workAvailable;
	std::condition_variable workFinished;
	std::vector<std::thread> workers;
	std::function<void(SizeT begin, SizeT end)> currentTask;
	std::atomic<SizeT> nextItem{0};
	SizeT taskCount = 0;
	SizeT chunkSize = 1;
	SizeT activeWorkers = 0;
	SizeT workGeneration = 0;
	bool hasWork = false;
	bool stopping = false;
};

} // namespace AE::Threading

#endif
