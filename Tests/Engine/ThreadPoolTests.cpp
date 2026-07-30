#include "Core/BuildConfig.h"

#if C_UNIT_TEST

#include <gtest/gtest.h>

#include "Core/Threading/ThreadPool.h"

#include <atomic>
#include <functional>
#include <stdexcept>

namespace
{
struct FThreadPoolWorkerCountGuard
{
	FThreadPoolWorkerCountGuard(AE::Threading::FThreadPool& InThreadPool)
		: ThreadPool(InThreadPool)
		, OriginalWorkerCount(InThreadPool.WorkerCount())
	{
	}

	~FThreadPoolWorkerCountGuard()
	{
		ThreadPool.SetWorkerCount(OriginalWorkerCount);
	}

	AE::Threading::FThreadPool& ThreadPool;
	SizeT OriginalWorkerCount = 0u;
};
} // namespace

TEST(FThreadPoolTests, RethrowsWorkerExceptionOnCallingThreadAndClearsState)
{
	AE::Threading::FThreadPool& ThreadPool = AE::Threading::FThreadPool::Get();
	FThreadPoolWorkerCountGuard WorkerCountGuard(ThreadPool);
	ThreadPool.SetWorkerCount(2u);

	EXPECT_THROW(
		ThreadPool.ParallelFor(8u, 1u, [](SizeT Begin, SizeT End)
		{
			if (Begin == 0u)
			{
				throw std::runtime_error("thread pool task failed");
			}
			(void)End;
		}),
		std::runtime_error);

	std::atomic<SizeT> ProcessedItemCount{0u};
	EXPECT_NO_THROW(ThreadPool.ParallelFor(8u, 1u, [&ProcessedItemCount](SizeT Begin, SizeT End)
	{
		ProcessedItemCount.fetch_add(End - Begin, std::memory_order_relaxed);
	}));
	EXPECT_EQ(ProcessedItemCount.load(std::memory_order_relaxed), 8u);
}

TEST(FThreadPoolTests, RejectsEmptyCallbackForNonEmptyWork)
{
	AE::Threading::FThreadPool& ThreadPool = AE::Threading::FThreadPool::Get();

	EXPECT_THROW(
		ThreadPool.ParallelFor(1u, 1u, std::function<void(SizeT Begin, SizeT End)>{}),
		std::invalid_argument);

	EXPECT_NO_THROW(
		ThreadPool.ParallelFor(0u, 1u, std::function<void(SizeT Begin, SizeT End)>{}));
}

#endif
