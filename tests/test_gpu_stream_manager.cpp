/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gpu_stream_manager.cpp                        ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "themis/gpu/stream_manager.h"
#include <atomic>
#include <chrono>
#include <thread>

using namespace themis::gpu;

// Helper to build a simple work item.
static GPULauncher::WorkItem makeItem(const std::string& kernel = "k1",
                                       const std::string& tag    = "test") {
    GPULauncher::WorkItem w;
    w.kernel_id = kernel;
    w.tag       = tag;
    return w;
}

// Each test uses a fresh local manager (not the singleton) to avoid
// cross-test interference.
static GPUStreamManager makeManager() { return GPUStreamManager{}; }

// ---------------------------------------------------------------------------
// Stream lifecycle
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, CreateStream_SucceedsForNewName) {
    GPUStreamManager sm;
    EXPECT_TRUE(sm.createStream({"stream_a"}));
    EXPECT_TRUE(sm.hasStream("stream_a"));
}

TEST(GPUStreamManagerTest, CreateStream_FailsForDuplicateName) {
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createStream({"dup"}));
    EXPECT_FALSE(sm.createStream({"dup"}));
}

TEST(GPUStreamManagerTest, CreateStream_FailsForEmptyName) {
    GPUStreamManager sm;
    EXPECT_FALSE(sm.createStream({""}));
}

TEST(GPUStreamManagerTest, DestroyStream_SucceedsForExistingStream) {
    GPUStreamManager sm;
    sm.createStream({"s1"});
    EXPECT_TRUE(sm.destroyStream("s1"));
    EXPECT_FALSE(sm.hasStream("s1"));
}

TEST(GPUStreamManagerTest, DestroyStream_FailsForUnknownStream) {
    GPUStreamManager sm;
    EXPECT_FALSE(sm.destroyStream("no_such_stream"));
}

TEST(GPUStreamManagerTest, StreamCount_ReflectsLifecycle) {
    GPUStreamManager sm;
    EXPECT_EQ(sm.streamCount(), 0u);
    sm.createStream({"a"});
    sm.createStream({"b"});
    EXPECT_EQ(sm.streamCount(), 2u);
    sm.destroyStream("a");
    EXPECT_EQ(sm.streamCount(), 1u);
}

TEST(GPUStreamManagerTest, StreamNames_ReturnsAllNames) {
    GPUStreamManager sm;
    sm.createStream({"x"});
    sm.createStream({"y"});
    auto names = sm.streamNames();
    ASSERT_EQ(names.size(), 2u);
    // order is unspecified (hash map); just check membership
    bool has_x = (names[0] == "x" || names[1] == "x");
    bool has_y = (names[0] == "y" || names[1] == "y");
    EXPECT_TRUE(has_x);
    EXPECT_TRUE(has_y);
}

// ---------------------------------------------------------------------------
// Work submission — success path
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, Submit_ToExistingStream_Succeeds) {
    GPUStreamManager sm;
    sm.createStream({"main"}, [](const GPULauncher::WorkItem&) { return true; });
    auto fut = sm.submit("main", makeItem("k1"));
    const auto res = fut.get();
    EXPECT_TRUE(res.success);
    EXPECT_EQ(res.kernel_id, "k1");
}

TEST(GPUStreamManagerTest, Submit_ToUnknownStream_ReturnsFail) {
    GPUStreamManager sm;
    auto fut = sm.submit("no_stream", makeItem("k1"));
    const auto res = fut.get();
    EXPECT_FALSE(res.success);
    EXPECT_FALSE(res.error_message.empty());
}

TEST(GPUStreamManagerTest, Submit_WithFailingBackend_CountsFailure) {
    GPUStreamManager sm;
    sm.createStream({"failing"}, [](const GPULauncher::WorkItem&) { return false; });
    sm.submit("failing", makeItem("kfail")).get();
    const auto st = sm.getStreamStats("failing");
    EXPECT_EQ(st.submitted, 1u);
    EXPECT_EQ(st.failed,    1u);
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, GetStreamStats_UnknownStream_ReturnsZeroStats) {
    GPUStreamManager sm;
    const auto st = sm.getStreamStats("ghost");
    EXPECT_EQ(st.submitted, 0u);
    EXPECT_EQ(st.succeeded, 0u);
}

TEST(GPUStreamManagerTest, GetStreamStats_TracksTotals) {
    GPUStreamManager sm;
    sm.createStream({"tracker"}, [](const GPULauncher::WorkItem&) { return true; });
    sm.submit("tracker", makeItem("k1")).get();
    sm.submit("tracker", makeItem("k2")).get();
    const auto st = sm.getStreamStats("tracker");
    EXPECT_EQ(st.submitted, 2u);
    EXPECT_EQ(st.succeeded, 2u);
    EXPECT_EQ(st.failed,    0u);
}

TEST(GPUStreamManagerTest, GetAllStreamStats_ReturnsOneEntryPerStream) {
    GPUStreamManager sm;
    sm.createStream({"s1"});
    sm.createStream({"s2"});
    const auto all = sm.getAllStreamStats();
    EXPECT_EQ(all.size(), 2u);
}

// ---------------------------------------------------------------------------
// CPU fallback budget
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, BudgetExceeded_CountedWhenBackendExceedsThreshold) {
    GPUStreamManager sm;
    GPUStreamManager::StreamConfig cfg;
    cfg.name           = "slow_stream";
    cfg.cpu_budget_ms  = 1;  // 1 ms budget
    // Backend sleeps for 50 ms — well above the 1 ms budget even under CI load.
    sm.createStream(cfg, [](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return true;
    });
    sm.submit("slow_stream", makeItem("slow_k")).get();
    const auto st = sm.getStreamStats("slow_stream");
    EXPECT_GE(st.budget_exceeded, 1u);
}

TEST(GPUStreamManagerTest, BudgetNotExceeded_WhenFastEnough) {
    GPUStreamManager sm;
    GPUStreamManager::StreamConfig cfg;
    cfg.name          = "fast_stream";
    cfg.cpu_budget_ms = 5000;  // 5 s — essentially never exceeded by unit test
    sm.createStream(cfg, [](const GPULauncher::WorkItem&) { return true; });
    sm.submit("fast_stream", makeItem("fast_k")).get();
    const auto st = sm.getStreamStats("fast_stream");
    EXPECT_EQ(st.budget_exceeded, 0u);
}

TEST(GPUStreamManagerTest, ZeroBudget_NeverCountsBudgetExceeded) {
    GPUStreamManager sm;
    GPUStreamManager::StreamConfig cfg;
    cfg.name          = "no_budget";
    cfg.cpu_budget_ms = 0;  // disabled
    sm.createStream(cfg, [](const GPULauncher::WorkItem&) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return true;
    });
    sm.submit("no_budget", makeItem("k")).get();
    const auto st = sm.getStreamStats("no_budget");
    EXPECT_EQ(st.budget_exceeded, 0u);
}

// ---------------------------------------------------------------------------
// Concurrent submissions to the same stream
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, ConcurrentSubmits_SubmittedCountIsAccurate) {
    GPUStreamManager sm;
    sm.createStream({"parallel"},
                    [](const GPULauncher::WorkItem&) { return true; });

    constexpr int kThreads = 4;
    constexpr int kItemsPerThread = 5;
    std::vector<std::future<GPULauncher::WorkResult>> futs;
    futs.reserve(kThreads * kItemsPerThread);

    for (int t = 0; t < kThreads; ++t) {
        for (int i = 0; i < kItemsPerThread; ++i) {
            futs.push_back(sm.submit("parallel", makeItem("k_par")));
        }
    }
    for (auto& f : futs) { f.get(); }

    const auto st = sm.getStreamStats("parallel");
    EXPECT_EQ(st.submitted, static_cast<size_t>(kThreads * kItemsPerThread));
    EXPECT_EQ(st.succeeded, static_cast<size_t>(kThreads * kItemsPerThread));
}

// ---------------------------------------------------------------------------
// No-op (nullptr) backend
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, NullptrBackend_ItemsSucceedViaCPU) {
    GPUStreamManager sm;
    sm.createStream({"noop"}, nullptr);
    auto res = sm.submit("noop", makeItem("noop_k")).get();
    EXPECT_TRUE(res.success);
}
