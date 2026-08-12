/*
 * Unit tests for the Vulkan compute backend (vulkan_backend.h / vulkan_backend.cpp).
 *
 * All tests are designed to run on CI without Vulkan hardware.  When Vulkan is
 * not available the backend transparently falls back to CPU execution and the
 * tests verify that the fallback path behaves correctly.  On a machine with a
 * real Vulkan-capable GPU the same tests exercise the actual Vulkan code paths.
 */

#include <gtest/gtest.h>
#include "themis/gpu/vulkan_backend.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/stream_manager.h"
#include "themis/gpu/launcher.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

class VulkanComputeBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any streams left by previous tests.
        for (const auto& n : VulkanComputeBackend::GetInstance().streamNames()) {
            VulkanComputeBackend::GetInstance().destroyStream(n);
        }
        VulkanComputeBackend::GetInstance().resetStats();
    }

    void TearDown() override {
        for (const auto& n : VulkanComputeBackend::GetInstance().streamNames()) {
            VulkanComputeBackend::GetInstance().destroyStream(n);
        }
        VulkanComputeBackend::GetInstance().resetStats();
    }

    VulkanComputeBackend& backend() {
        return VulkanComputeBackend::GetInstance();
    }
};

// ---------------------------------------------------------------------------
// Device query
// ---------------------------------------------------------------------------

TEST_F(VulkanComputeBackendTest, DeviceCount_IsNonNegative) {
    EXPECT_GE(backend().deviceCount(), 0);
}

TEST_F(VulkanComputeBackendTest, IsAvailable_MatchesDeviceCount) {
    const bool avail = backend().isAvailable();
    const int  count = backend().deviceCount();
    EXPECT_EQ(avail, count > 0);
}

TEST_F(VulkanComputeBackendTest, VendorName_IsNonEmpty) {
    // vendorName() always returns a non-empty string (may be "Unknown" on CI).
    const std::string name = backend().vendorName();
    EXPECT_FALSE(name.empty());
}

// ---------------------------------------------------------------------------
// Launcher backend
// ---------------------------------------------------------------------------

TEST_F(VulkanComputeBackendTest, CreateBackendFn_ReturnsCallable) {
    auto fn = backend().createBackendFn(0);
    ASSERT_TRUE(fn != nullptr);
}

TEST_F(VulkanComputeBackendTest, CreateBackendFn_WorkItemWithNoArgs_Succeeds) {
    auto fn = backend().createBackendFn(0);
    GPULauncher::WorkItem item;
    item.kernel_id = "vulkan_test_kernel";
    EXPECT_TRUE(fn(item));
}

TEST_F(VulkanComputeBackendTest, CreateBackendFn_WorkItemWithArgs_Succeeds) {
    auto fn = backend().createBackendFn(0);
    GPULauncher::WorkItem item;
    item.kernel_id = "vulkan_kernel_with_args";
    item.args      = {0x01, 0x02, 0x03};
    EXPECT_TRUE(fn(item));
}

TEST_F(VulkanComputeBackendTest, CreateBackendFn_CanBePassedToGPULauncher) {
    auto fn = backend().createBackendFn(0);
    GPULauncher launcher(std::move(fn));

    GPULauncher::WorkItem item;
    item.kernel_id = "vulkan_launcher_test";
    auto result = launcher.submit(std::move(item)).get();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.kernel_id, "vulkan_launcher_test");
}

// ---------------------------------------------------------------------------
// Stats updated by backend function
// ---------------------------------------------------------------------------

TEST_F(VulkanComputeBackendTest, BackendFn_DispatchUpdatesStats) {
    auto fn = backend().createBackendFn(0);
    GPULauncher::WorkItem item;
    item.kernel_id = "vulkan_stats_kernel";
    fn(item);

    const auto s = backend().getStats();
    // Either dispatched or cpu_fallbacks should be incremented.
    EXPECT_EQ(s.dispatched + s.cpu_fallbacks, 1u);
}

// ---------------------------------------------------------------------------
// Stream lifecycle
// ---------------------------------------------------------------------------

TEST_F(VulkanComputeBackendTest, CreateStream_NewName_Succeeds) {
    auto r = backend().createStream("vk_stream_a");
    EXPECT_TRUE(r.ok) << r.error_message;
    EXPECT_TRUE(backend().hasStream("vk_stream_a"));
}

TEST_F(VulkanComputeBackendTest, CreateStream_EmptyName_Fails) {
    auto r = backend().createStream("");
    EXPECT_FALSE(r.ok);
}

TEST_F(VulkanComputeBackendTest, CreateStream_DuplicateName_Fails) {
    ASSERT_TRUE(backend().createStream("dup").ok);
    auto r = backend().createStream("dup");
    EXPECT_FALSE(r.ok);
}

TEST_F(VulkanComputeBackendTest, DestroyStream_ExistingStream_Succeeds) {
    ASSERT_TRUE(backend().createStream("to_destroy").ok);
    auto r = backend().destroyStream("to_destroy");
    EXPECT_TRUE(r.ok) << r.error_message;
    EXPECT_FALSE(backend().hasStream("to_destroy"));
}

TEST_F(VulkanComputeBackendTest, DestroyStream_NonexistentStream_Fails) {
    auto r = backend().destroyStream("ghost_stream");
    EXPECT_FALSE(r.ok);
}

TEST_F(VulkanComputeBackendTest, SynchronizeStream_ExistingStream_Succeeds) {
    ASSERT_TRUE(backend().createStream("sync_stream").ok);
    auto r = backend().synchronizeStream("sync_stream");
    EXPECT_TRUE(r.ok) << r.error_message;
}

TEST_F(VulkanComputeBackendTest, SynchronizeStream_NonexistentStream_Fails) {
    auto r = backend().synchronizeStream("no_such_stream");
    EXPECT_FALSE(r.ok);
}

TEST_F(VulkanComputeBackendTest, GetStream_ExistingStream_ReturnsCorrectName) {
    ASSERT_TRUE(backend().createStream("get_me").ok);
    auto h = backend().getStream("get_me");
    EXPECT_EQ(h.name, "get_me");
}

TEST_F(VulkanComputeBackendTest, GetStream_NonexistentStream_ReturnsInvalidHandle) {
    auto h = backend().getStream("no_such");
    EXPECT_FALSE(h.is_valid());
}

TEST_F(VulkanComputeBackendTest, StreamNames_ReflectsLifecycle) {
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

TEST_F(VulkanComputeBackendTest, Stats_StreamsCreatedAndDestroyed_Counted) {
    backend().createStream("st_a");
    backend().createStream("st_b");
    backend().destroyStream("st_a");
    const auto s = backend().getStats();
    EXPECT_EQ(s.streams_created,   2u);
    EXPECT_EQ(s.streams_destroyed, 1u);
}

TEST_F(VulkanComputeBackendTest, Stats_ResetClearsCounters) {
    backend().createStream("stats_s1");
    backend().destroyStream("stats_s1");
    backend().resetStats();
    const auto s = backend().getStats();
    EXPECT_EQ(s.streams_created,   0u);
    EXPECT_EQ(s.streams_destroyed, 0u);
    EXPECT_EQ(s.dispatched,        0u);
    EXPECT_EQ(s.dispatch_errors,   0u);
    EXPECT_EQ(s.cpu_fallbacks,     0u);
}

// ---------------------------------------------------------------------------
// Integration: Vulkan backend with GPUStreamManager
// ---------------------------------------------------------------------------

TEST(VulkanStreamManagerIntegrationTest,
     StreamManager_WithVulkanBackend_WorkItemSucceeds) {
    auto& sm = GPUStreamManager::GetInstance();
    auto backend_fn = VulkanComputeBackend::GetInstance().createBackendFn(0);
    sm.createStream({"vk_integrated"}, std::move(backend_fn));

    GPULauncher::WorkItem item;
    item.kernel_id = "vk_integrated_kernel";
    auto result = sm.submit("vk_integrated", std::move(item)).get();
    EXPECT_TRUE(result.success);

    sm.destroyStream("vk_integrated");
}

// ---------------------------------------------------------------------------
// Feature flag integration
// ---------------------------------------------------------------------------

TEST(VulkanFeatureFlagTest, VulkanBackend_FeatureEnabled_ByDefault) {
    using themis::gpu::GPUFeatureFlags;
    // VULKAN_BACKEND is available in all editions (Community and above).
    EXPECT_TRUE(GPUFeatureFlags::GetInstance().isEnabled(
        GPUFeatureFlags::Feature::VULKAN_BACKEND));
}

TEST(VulkanFeatureFlagTest, VulkanBackend_FeatureName_IsCorrect) {
    using themis::gpu::GPUFeatureFlags;
    EXPECT_STREQ(GPUFeatureFlags::featureName(
        GPUFeatureFlags::Feature::VULKAN_BACKEND), "VULKAN_BACKEND");
}

TEST(VulkanFeatureFlagTest, VulkanBackend_CanBeDisabledAndReEnabled) {
    using themis::gpu::GPUFeatureFlags;
    auto& flags = GPUFeatureFlags::GetInstance();

    flags.disable(GPUFeatureFlags::Feature::VULKAN_BACKEND);
    EXPECT_FALSE(flags.isEnabled(GPUFeatureFlags::Feature::VULKAN_BACKEND));

    flags.enable(GPUFeatureFlags::Feature::VULKAN_BACKEND);
    EXPECT_TRUE(flags.isEnabled(GPUFeatureFlags::Feature::VULKAN_BACKEND));

    flags.resetToDefaults();
}

TEST(VulkanFeatureFlagTest, GetAll_ContainsVulkanBackend) {
    using themis::gpu::GPUFeatureFlags;
    const auto all = GPUFeatureFlags::GetInstance().getAll();
    bool found = false;
    for (const auto& s : all) {
        if (s.feature == GPUFeatureFlags::Feature::VULKAN_BACKEND) {
            found = true;
            EXPECT_EQ(s.name, "VULKAN_BACKEND");
            break;
        }
    }
    EXPECT_TRUE(found) << "VULKAN_BACKEND not found in getAll()";
}
