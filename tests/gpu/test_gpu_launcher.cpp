#include <gtest/gtest.h>
#include "themis/gpu/launcher.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::gpu;

// Convenience work-item builder.
static GPULauncher::WorkItem makeItem(const std::string& kernel_id,
                                       const std::string& tag = "test") {
    GPULauncher::WorkItem w;
    w.kernel_id = kernel_id;
    w.tag       = tag;
    return w;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, ConstructWithNullBackend_ThrowsInvalidArgument) {
    EXPECT_THROW(GPULauncher(nullptr), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// submit — success
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, Submit_BackendSucceeds_ResultIsSuccess) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    auto fut = launcher.submit(makeItem("k1"));
    const auto result = fut.get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.kernel_id, "k1");
}

TEST(GPULauncherTest, Submit_BackendFails_ResultIsFailure) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return false; });
    auto fut = launcher.submit(makeItem("k_fail"));
    const auto result = fut.get();
    EXPECT_FALSE(result.success);
}

TEST(GPULauncherTest, Submit_BackendThrows_ResultIsFailure) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) -> bool {
        throw std::runtime_error("kernel crash");
    });
    auto fut = launcher.submit(makeItem("k_throw"));
    const auto result = fut.get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(GPULauncherTest, Submit_ElapsedTime_IsNonNegative) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    auto fut = launcher.submit(makeItem("k_time"));
    const auto result = fut.get();
    EXPECT_GE(result.elapsed.count(), 0);
}

// ---------------------------------------------------------------------------
// submitBatch
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, SubmitBatch_AllSucceed) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    std::vector<GPULauncher::WorkItem> items{
        makeItem("k1"), makeItem("k2"), makeItem("k3")};
    auto fut = launcher.submitBatch(std::move(items));
    const auto results = fut.get();
    ASSERT_EQ(results.size(), 3u);
    for (const auto& r : results) {
        EXPECT_TRUE(r.success);
    }
}

TEST(GPULauncherTest, SubmitBatch_MixedResults) {
    int call = 0;
    GPULauncher launcher([&call](const GPULauncher::WorkItem&) {
        return (++call % 2) == 1;  // odd calls succeed
    });
    std::vector<GPULauncher::WorkItem> items{
        makeItem("a"), makeItem("b"), makeItem("c"), makeItem("d")};
    auto fut = launcher.submitBatch(std::move(items));
    const auto results = fut.get();
    ASSERT_EQ(results.size(), 4u);
    size_t ok = 0, fail = 0;
    for (const auto& r : results) {
        if (r.success) ++ok; else ++fail;
    }
    EXPECT_EQ(ok, 2u);
    EXPECT_EQ(fail, 2u);
}

TEST(GPULauncherTest, SubmitBatch_Empty_ReturnsEmptyResults) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    auto fut = launcher.submitBatch({});
    const auto results = fut.get();
    EXPECT_TRUE(results.empty());
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, Stats_SuccessAndFailureCounted) {
    int call = 0;
    GPULauncher launcher([&call](const GPULauncher::WorkItem&) {
        return (++call % 2) == 0;
    });
    for (int i = 0; i < 6; ++i) {
        launcher.submit(makeItem("k")).get();
    }
    const auto s = launcher.getStats();
    EXPECT_EQ(s.submitted, 6u);
    EXPECT_EQ(s.succeeded, 3u);
    EXPECT_EQ(s.failed, 3u);
}

TEST(GPULauncherTest, Stats_BatchesCountedSeparately) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    launcher.submitBatch({makeItem("a"), makeItem("b")}).get();
    launcher.submitBatch({makeItem("c")}).get();
    EXPECT_EQ(launcher.getStats().batches_submitted, 2u);
}

// ---------------------------------------------------------------------------
// Work item fields preserved
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, WorkItem_KernelIdPreservedInResult) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    GPULauncher::WorkItem w = makeItem("unique_kernel_id");
    auto result = launcher.submit(std::move(w)).get();
    EXPECT_EQ(result.kernel_id, "unique_kernel_id");
}

// ---------------------------------------------------------------------------
// Timeout enforcement
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, Timeout_ExceededBySlowBackend_CountsAsTimedOut) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return true;
    });
    GPULauncher::WorkItem w = makeItem("slow_kernel");
    w.timeout_ms = 10;  // 10 ms — backend sleeps 200 ms
    const auto result = launcher.submit(std::move(w)).get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_GE(launcher.getStats().timed_out, 1u);
    EXPECT_GE(launcher.getStats().failed, 1u);
}

TEST(GPULauncherTest, Timeout_NotExceededByFastBackend_Succeeds) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) { return true; });
    GPULauncher::WorkItem w = makeItem("fast_kernel");
    w.timeout_ms = 5000;  // 5 s — backend returns immediately
    const auto result = launcher.submit(std::move(w)).get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(launcher.getStats().timed_out, 0u);
}

TEST(GPULauncherTest, Timeout_ZeroMeansNoTimeout) {
    GPULauncher launcher([](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return true;
    });
    GPULauncher::WorkItem w = makeItem("no_timeout_kernel");
    w.timeout_ms = 0;  // no timeout
    const auto result = launcher.submit(std::move(w)).get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(launcher.getStats().timed_out, 0u);
}

// ---------------------------------------------------------------------------
// Concurrent submits
// ---------------------------------------------------------------------------

TEST(GPULauncherTest, Concurrent_Submits_NoDataRace) {
    std::atomic<int> backend_calls{0};
    GPULauncher launcher([&backend_calls](const GPULauncher::WorkItem&) {
        backend_calls.fetch_add(1);
        return true;
    });

    constexpr int THREADS = 8, OPS_PER_THREAD = 10;
    std::vector<std::thread> threads;
    threads.reserve(THREADS);
    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                launcher.submit(makeItem("k")).get();
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(backend_calls.load(), THREADS * OPS_PER_THREAD);
    EXPECT_EQ(launcher.getStats().submitted,
              static_cast<size_t>(THREADS * OPS_PER_THREAD));
}
