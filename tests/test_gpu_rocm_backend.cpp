/*
 * Unit tests for the ROCm/HIP backend (rocm_backend.h / rocm_backend.cpp).
 *
 * All tests are designed to run on CI without AMD GPU hardware.  When HIP is
 * not available the backend transparently falls back to CPU execution and the
 * tests verify that the fallback path behaves correctly.  On a machine with a
 * real HIP-capable GPU the same tests exercise the actual HIP code paths.
 */

#include <gtest/gtest.h>
#include "themis/gpu/rocm_backend.h"
#include "themis/gpu/stream_manager.h"
#include "themis/gpu/launcher.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Create a fresh ROCmBackend-like wrapper for each test by using the singleton
// and resetting stats between tests.  Streams must be destroyed explicitly.
class ROCmBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any streams left by previous tests (e.g. if a test crashed).
        for (const auto& n : ROCmBackend::GetInstance().streamNames()) {
            ROCmBackend::GetInstance().destroyStream(n);
        }
        ROCmBackend::GetInstance().resetStats();
    }

    void TearDown() override {
        // Best-effort clean-up.
        for (const auto& n : ROCmBackend::GetInstance().streamNames()) {
            ROCmBackend::GetInstance().destroyStream(n);
        }
        ROCmBackend::GetInstance().resetStats();
    }

    ROCmBackend& backend() { return ROCmBackend::GetInstance(); }
};

// ---------------------------------------------------------------------------
// Device query
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, DeviceCount_IsNonNegative) {
    EXPECT_GE(backend().deviceCount(), 0);
}

TEST_F(ROCmBackendTest, IsAvailable_MatchesDeviceCount) {
    const bool avail = backend().isAvailable();
    const int  count = backend().deviceCount();
    EXPECT_EQ(avail, count > 0);
}

// ---------------------------------------------------------------------------
// Launcher backend
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, CreateBackendFn_ReturnsCallable) {
    auto fn = backend().createBackendFn(0);
    ASSERT_TRUE(fn != nullptr);
}

TEST_F(ROCmBackendTest, CreateBackendFn_WorkItemWithNoArgs_Succeeds) {
    auto fn = backend().createBackendFn(0);
    GPULauncher::WorkItem item;
    item.kernel_id = "hip_test_kernel";
    EXPECT_TRUE(fn(item));
}

TEST_F(ROCmBackendTest, CreateBackendFn_WorkItemWithArgs_Succeeds) {
    auto fn = backend().createBackendFn(0);
    GPULauncher::WorkItem item;
    item.kernel_id = "hip_kernel_with_args";
    item.args      = {0x01, 0x02, 0x03};
    EXPECT_TRUE(fn(item));
}

TEST_F(ROCmBackendTest, CreateBackendFn_CanBePassedToGPULauncher) {
    auto fn = backend().createBackendFn(0);
    GPULauncher launcher(std::move(fn));

    GPULauncher::WorkItem item;
    item.kernel_id = "hip_launcher_test";
    auto result = launcher.submit(std::move(item)).get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.kernel_id, "hip_launcher_test");
}

// ---------------------------------------------------------------------------
// Stream lifecycle
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, CreateStream_NewName_Succeeds) {
    auto r = backend().createStream("hip_stream_a");
    EXPECT_TRUE(r.ok) << r.error_message;
    EXPECT_TRUE(backend().hasStream("hip_stream_a"));
}

TEST_F(ROCmBackendTest, CreateStream_EmptyName_Fails) {
    auto r = backend().createStream("");
    EXPECT_FALSE(r.ok);
}

TEST_F(ROCmBackendTest, CreateStream_DuplicateName_Fails) {
    ASSERT_TRUE(backend().createStream("dup").ok);
    auto r = backend().createStream("dup");
    EXPECT_FALSE(r.ok);
}

TEST_F(ROCmBackendTest, DestroyStream_ExistingStream_Succeeds) {
    ASSERT_TRUE(backend().createStream("to_destroy").ok);
    auto r = backend().destroyStream("to_destroy");
    EXPECT_TRUE(r.ok) << r.error_message;
    EXPECT_FALSE(backend().hasStream("to_destroy"));
}

TEST_F(ROCmBackendTest, DestroyStream_NonexistentStream_Fails) {
    auto r = backend().destroyStream("ghost_stream");
    EXPECT_FALSE(r.ok);
}

TEST_F(ROCmBackendTest, SynchronizeStream_ExistingStream_Succeeds) {
    ASSERT_TRUE(backend().createStream("sync_stream").ok);
    auto r = backend().synchronizeStream("sync_stream");
    EXPECT_TRUE(r.ok) << r.error_message;
}

TEST_F(ROCmBackendTest, SynchronizeStream_NonexistentStream_Fails) {
    auto r = backend().synchronizeStream("no_such_stream");
    EXPECT_FALSE(r.ok);
}

TEST_F(ROCmBackendTest, GetStream_ExistingStream_ReturnsValidHandle) {
    ASSERT_TRUE(backend().createStream("get_me").ok);
    auto h = backend().getStream("get_me");
    EXPECT_EQ(h.name, "get_me");
}

TEST_F(ROCmBackendTest, GetStream_NonexistentStream_ReturnsInvalidHandle) {
    auto h = backend().getStream("no_such");
    EXPECT_FALSE(h.is_valid());
}

TEST_F(ROCmBackendTest, StreamNames_ReflectsLifecycle) {
    ASSERT_TRUE(backend().createStream("s1").ok);
    ASSERT_TRUE(backend().createStream("s2").ok);
    auto names = backend().streamNames();
    EXPECT_EQ(names.size(), 2u);
    backend().destroyStream("s1");
    names = backend().streamNames();
    EXPECT_EQ(names.size(), 1u);
}

// ---------------------------------------------------------------------------
// Statistics — stream lifecycle
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, Stats_StreamsCreatedAndDestroyed_Counted) {
    backend().createStream("st_a");
    backend().createStream("st_b");
    backend().destroyStream("st_a");
    const auto s = backend().getStats();
    EXPECT_EQ(s.streams_created,   2u);
    EXPECT_EQ(s.streams_destroyed, 1u);
}

// ---------------------------------------------------------------------------
// Device memory
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, Allocate_ZeroBytes_ReturnsInvalidRecord) {
    auto rec = backend().allocate(0, "zero");
    EXPECT_FALSE(rec.is_valid());
}

TEST_F(ROCmBackendTest, Deallocate_InvalidRecord_Succeeds) {
    ROCmBackend::AllocationRecord empty;
    auto r = backend().deallocate(empty);
    EXPECT_TRUE(r.ok);
}

TEST_F(ROCmBackendTest, ZeroMemory_ZeroPtr_Succeeds) {
    // Calling with device_ptr == 0 should be a no-op.
    auto r = backend().zeroMemory(0, 1024);
    EXPECT_TRUE(r.ok);
}

TEST_F(ROCmBackendTest, ZeroMemory_ZeroSize_Succeeds) {
    auto r = backend().zeroMemory(0x1000, 0);
    EXPECT_TRUE(r.ok);
}

// ---------------------------------------------------------------------------
// Integration: ROCm backend with GPUStreamManager
// ---------------------------------------------------------------------------

TEST(ROCmStreamManagerIntegrationTest,
     StreamManager_WithROCmBackend_WorkItemSucceeds) {
    auto& sm = GPUStreamManager::GetInstance();
    // Wire the ROCm backend into the stream manager.
    auto backend_fn = ROCmBackend::GetInstance().createBackendFn(0);
    sm.createStream({"hip_integrated"}, std::move(backend_fn));

    GPULauncher::WorkItem item;
    item.kernel_id = "hip_integrated_kernel";
    auto result = sm.submit("hip_integrated", std::move(item)).get();
    EXPECT_TRUE(result.success);

    sm.destroyStream("hip_integrated");
}

TEST(ROCmStreamManagerIntegrationTest,
     StreamManager_NullBackend_UsesROCmFallback) {
    // When no backend is provided, GPUStreamManager now uses the ROCm backend
    // (which falls back to CPU when HIP is unavailable).
    auto& sm = GPUStreamManager::GetInstance();
    sm.createStream({"rocm_default"});

    GPULauncher::WorkItem item;
    item.kernel_id = "rocm_default_kernel";
    auto result = sm.submit("rocm_default", std::move(item)).get();
    EXPECT_TRUE(result.success);

    sm.destroyStream("rocm_default");
}

// ---------------------------------------------------------------------------
// Statistics — memory (no-HIP path)
// ---------------------------------------------------------------------------

TEST_F(ROCmBackendTest, Stats_ResetClearsCounters) {
    backend().createStream("stats_s1");
    backend().destroyStream("stats_s1");
    backend().resetStats();
    const auto s = backend().getStats();
    EXPECT_EQ(s.streams_created,   0u);
    EXPECT_EQ(s.streams_destroyed, 0u);
    EXPECT_EQ(s.alloc_count,       0u);
    EXPECT_EQ(s.dealloc_count,     0u);
    EXPECT_EQ(s.bytes_allocated,   0u);
}
