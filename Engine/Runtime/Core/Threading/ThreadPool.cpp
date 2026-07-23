#include "Core/Threading/ThreadPool.h"

#include <algorithm>

namespace AE::Threading
{

ThreadPool& ThreadPool::Get()
{
    static ThreadPool pool;
    return pool;
}

ThreadPool::ThreadPool()
{
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    const std::size_t workerCount = hardwareThreads > 2u ? hardwareThreads - 1u : 1u;
    Start(workerCount);
}

ThreadPool::~ThreadPool()
{
    Stop();
}

std::size_t ThreadPool::WorkerCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return workers.size();
}

void ThreadPool::SetWorkerCount(std::size_t workerCount)
{
    workerCount = std::max<std::size_t>(1u, workerCount);

    {
        std::unique_lock<std::mutex> lock(mutex);
        workFinished.wait(lock, [this]()
        {
            return !hasWork;
        });
        if (workers.size() == workerCount)
        {
            return;
        }
    }

    Stop();
    Start(workerCount);
}

void ThreadPool::ParallelFor(
    std::size_t itemCount,
    std::size_t minItemsPerJob,
    const std::function<void(std::size_t begin, std::size_t end)>& function)
{
    if (itemCount == 0u)
    {
        return;
    }

    const std::size_t workerCount = WorkerCount();
    minItemsPerJob = std::max<std::size_t>(1u, minItemsPerJob);
    const std::size_t jobCount = std::min(workerCount, (itemCount + minItemsPerJob - 1u) / minItemsPerJob);

    if (jobCount < 2u)
    {
        function(0u, itemCount);
        return;
    }

    {
        std::unique_lock<std::mutex> lock(mutex);
        workFinished.wait(lock, [this]()
        {
            return !hasWork;
        });

        currentTask = function;
        taskCount = itemCount;
        chunkSize = (itemCount + jobCount - 1u) / jobCount;
        nextItem.store(0u, std::memory_order_relaxed);
        activeWorkers = workers.size();
        hasWork = true;
        ++workGeneration;
    }

    workAvailable.notify_all();

    std::unique_lock<std::mutex> lock(mutex);
    workFinished.wait(lock, [this]()
    {
        return !hasWork;
    });
    currentTask = nullptr;
}

void ThreadPool::Start(std::size_t workerCount)
{
    std::lock_guard<std::mutex> lock(mutex);
    stopping = false;
    hasWork = false;
    activeWorkers = 0u;
    workGeneration = 0u;
    workers.reserve(workerCount);
    for (std::size_t index = 0; index < workerCount; ++index)
    {
        workers.emplace_back(&ThreadPool::WorkerLoop, this);
    }
}

void ThreadPool::Stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stopping = true;
        ++workGeneration;
    }

    workAvailable.notify_all();

    for (std::thread& worker : workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        workers.clear();
        currentTask = nullptr;
        nextItem.store(0u, std::memory_order_relaxed);
        taskCount = 0u;
        chunkSize = 1u;
        activeWorkers = 0u;
        workGeneration = 0u;
        hasWork = false;
        stopping = false;
    }
}

void ThreadPool::WorkerLoop()
{
    std::size_t observedGeneration = 0u;

    while (true)
    {
        std::function<void(std::size_t begin, std::size_t end)> task;
        std::size_t count = 0u;
        std::size_t chunk = 1u;

        {
            std::unique_lock<std::mutex> lock(mutex);
            workAvailable.wait(lock, [this, &observedGeneration]()
            {
                return stopping || workGeneration != observedGeneration;
            });

            if (stopping)
            {
                return;
            }

            observedGeneration = workGeneration;
            task = currentTask;
            count = taskCount;
            chunk = chunkSize;
        }

        while (true)
        {
            const std::size_t begin = nextItem.fetch_add(chunk, std::memory_order_relaxed);
            if (begin >= count)
            {
                break;
            }

            task(begin, std::min(begin + chunk, count));
        }

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (activeWorkers > 0u)
            {
                --activeWorkers;
            }
            if (activeWorkers == 0u)
            {
                hasWork = false;
                workFinished.notify_one();
            }
        }
    }
}

}
