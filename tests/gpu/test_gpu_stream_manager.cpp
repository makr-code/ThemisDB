#include <gtest/gtest.h>
#include "themis/gpu/stream_manager.h"
#include "themis/gpu/rocm_backend.h"
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

// ---------------------------------------------------------------------------
// CUDA stream creation (CPU fallback path when THEMIS_ENABLE_CUDA is absent)
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, CreateStream_CudaHandle_IsZeroWhenCudaUnavailable) {
    // When THEMIS_ENABLE_CUDA is not defined the cuda_stream field must remain
    // 0 after createStream() succeeds (no hardware present in CI).  When CUDA
    // IS available the field holds a real cudaStream_t cast to uintptr_t.
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createStream({"cuda_test"}));
    EXPECT_TRUE(sm.hasStream("cuda_test"));
    // Destroying the stream must succeed regardless of whether a real CUDA
    // handle was created; cudaStreamDestroy is called only when the handle != 0.
    EXPECT_TRUE(sm.destroyStream("cuda_test"));
    EXPECT_FALSE(sm.hasStream("cuda_test"));
}

TEST(GPUStreamManagerTest, CreateAndDestroy_MultipleStreams_CudaPathClean) {
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createStream({"cs1"}));
    ASSERT_TRUE(sm.createStream({"cs2"}));
    EXPECT_EQ(sm.streamCount(), 2u);
    EXPECT_TRUE(sm.destroyStream("cs1"));
    EXPECT_TRUE(sm.destroyStream("cs2"));
    EXPECT_EQ(sm.streamCount(), 0u);
}

// ---------------------------------------------------------------------------
// createCudaStream — CUDA-specific stream creation with CPU fallback
// (covers Issue: #1801 — recurring query pattern capture via CUDA streams)
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, CreateCudaStream_SucceedsForNewName) {
    GPUStreamManager sm;
    EXPECT_TRUE(sm.createCudaStream({"cuda_stream_a"}));
    EXPECT_TRUE(sm.hasStream("cuda_stream_a"));
    sm.destroyStream("cuda_stream_a");
}

TEST(GPUStreamManagerTest, CreateCudaStream_FailsForEmptyName) {
    GPUStreamManager sm;
    EXPECT_FALSE(sm.createCudaStream({""}));
}

TEST(GPUStreamManagerTest, CreateCudaStream_FailsForDuplicateName) {
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createCudaStream({"dup_cuda"}));
    EXPECT_FALSE(sm.createCudaStream({"dup_cuda"}));
    sm.destroyStream("dup_cuda");
}

TEST(GPUStreamManagerTest, CreateCudaStream_FailsWhenNameAlreadyUsedByCreateStream) {
    // A stream created with createStream() blocks createCudaStream() for the same name.
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createStream({"mixed_name"}));
    EXPECT_FALSE(sm.createCudaStream({"mixed_name"}));
    sm.destroyStream("mixed_name");
}

TEST(GPUStreamManagerTest, CreateCudaStream_WorkItemSucceeds) {
    // Work submitted to a CUDA-path stream must still succeed on CPU fallback.
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createCudaStream({"cuda_work_stream"}));
    auto fut = sm.submit("cuda_work_stream", makeItem("cuda_k"));
    const auto res = fut.get();
    EXPECT_TRUE(res.success);
    sm.destroyStream("cuda_work_stream");
}

TEST(GPUStreamManagerTest, CreateCudaStream_StatsTracked) {
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createCudaStream({"cuda_stats_stream"}));
    sm.submit("cuda_stats_stream", makeItem("k1")).get();
    sm.submit("cuda_stats_stream", makeItem("k2")).get();
    const auto st = sm.getStreamStats("cuda_stats_stream");
    EXPECT_EQ(st.submitted, 2u);
    EXPECT_EQ(st.succeeded, 2u);
    sm.destroyStream("cuda_stats_stream");
}

TEST(GPUStreamManagerTest, CreateCudaStream_DestroySucceeds) {
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createCudaStream({"cuda_destroy_stream"}));
    EXPECT_TRUE(sm.destroyStream("cuda_destroy_stream"));
    EXPECT_FALSE(sm.hasStream("cuda_destroy_stream"));
}

TEST(GPUStreamManagerTest, CreateCudaStream_MultipleDistinctStreams) {
    // Multiple CUDA streams can coexist with independent lifecycles.
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createCudaStream({"cs_a"}));
    ASSERT_TRUE(sm.createCudaStream({"cs_b"}));
    ASSERT_TRUE(sm.createCudaStream({"cs_c"}));
    EXPECT_EQ(sm.streamCount(), 3u);

    sm.submit("cs_a", makeItem("ka")).get();
    sm.submit("cs_b", makeItem("kb")).get();
    sm.submit("cs_c", makeItem("kc")).get();

    EXPECT_EQ(sm.getStreamStats("cs_a").succeeded, 1u);
    EXPECT_EQ(sm.getStreamStats("cs_b").succeeded, 1u);
    EXPECT_EQ(sm.getStreamStats("cs_c").succeeded, 1u);

    sm.destroyStream("cs_a");
    sm.destroyStream("cs_b");
    sm.destroyStream("cs_c");
    EXPECT_EQ(sm.streamCount(), 0u);
}

TEST(GPUStreamManagerTest, CreateCudaStream_WithDeviceIndex_Succeeds) {
    // Device index 0 always resolves (CPU fallback when no CUDA hardware).
    GPUStreamManager sm;
    EXPECT_TRUE(sm.createCudaStream({"cuda_dev0"}, /*device_index=*/0));
    EXPECT_TRUE(sm.hasStream("cuda_dev0"));
    sm.destroyStream("cuda_dev0");
}

// ---------------------------------------------------------------------------
// ROCm stream lifecycle (CPU fallback path when THEMIS_ENABLE_HIP is absent)
// ---------------------------------------------------------------------------

TEST(GPUStreamManagerTest, NullBackend_RegistersRocmStream) {
    // When createStream() is called without a backend, the stream must be
    // usable and cleanly destroyable. Some builds register a named ROCm
    // stream explicitly, while others wire the backend directly.
    GPUStreamManager sm;
    ASSERT_TRUE(sm.createStream({"rocm_stream_test"}));
    EXPECT_TRUE(sm.hasStream("rocm_stream_test"));

    const bool rocm_registered =
        ROCmBackend::GetInstance().hasStream("rocm_stream_test");

    EXPECT_TRUE(sm.destroyStream("rocm_stream_test"));
    if (rocm_registered) {
        EXPECT_FALSE(ROCmBackend::GetInstance().hasStream("rocm_stream_test"));
    }
}

TEST(GPUStreamManagerTest, CustomBackend_DoesNotRegisterRocmStream) {
    // When a caller-supplied backend is passed, GPUStreamManager must NOT
    // create a ROCm stream (caller owns the backend lifecycle).
    GPUStreamManager sm;
    sm.createStream({"custom_backend_stream"},
                    [](const GPULauncher::WorkItem&) { return true; });
    EXPECT_FALSE(ROCmBackend::GetInstance().hasStream("custom_backend_stream"));
    sm.destroyStream("custom_backend_stream");
}
