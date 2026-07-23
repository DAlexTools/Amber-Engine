#ifndef ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H
#define ENGINE_RUNTIME_CORE_THREADING_THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
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

    std::size_t WorkerCount() const;
    void SetWorkerCount(std::size_t workerCount);
    void ParallelFor(
        std::size_t itemCount,
        std::size_t minItemsPerJob,
        const std::function<void(std::size_t begin, std::size_t end)>& function);

private:
    ThreadPool();

    void Start(std::size_t workerCount);
    void Stop();
    void WorkerLoop();

    mutable std::mutex mutex;
    std::condition_variable workAvailable;
    std::condition_variable workFinished;
    std::vector<std::thread> workers;
    std::function<void(std::size_t begin, std::size_t end)> currentTask;
    std::atomic<std::size_t> nextItem{0};
    std::size_t taskCount = 0;
    std::size_t chunkSize = 1;
    std::size_t activeWorkers = 0;
    std::size_t workGeneration = 0;
    bool hasWork = false;
    bool stopping = false;
};

}

#endif
