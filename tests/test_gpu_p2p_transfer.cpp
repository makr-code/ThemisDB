/*
 * Unit tests for GPUP2PTransferManager
 * ======================================
 * All tests run on CI without GPU hardware.  The CPU simulation path is
 * exercised end-to-end: feature-gate enforcement, peer access lifecycle,
 * transfer routing (CPU fallback), error conditions, stats tracking, and
 * concurrent safety.
 *
 * Tests that require hardware peer access verify the return codes without
 * expecting a hardware P2P success, since the CI machine has no CUDA/HIP
 * device.  The transfer() tests inject synthetic DeviceInfo objects via the
 * devices parameter so the full transfer pipeline can be validated.
 */

#include <gtest/gtest.h>
#include "themis/gpu/p2p_transfer.h"
#include "themis/gpu/feature_flags.h"
#include "themis/gpu/device_discovery.h"

#include <atomic>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace themis::gpu;
using Status = GPUP2PTransferManager::Status;

// ---------------------------------------------------------------------------
// Helpers: synthetic device list for testing without real GPU hardware
// ---------------------------------------------------------------------------

static DeviceInfo makeFakeCUDA(int index, int compute_major = 8) {
    DeviceInfo d;
    d.index            = index;
    d.device_index     = index;
    d.name             = "Fake CUDA Device";
    d.backend          = "CUDA";
    d.compute_major    = compute_major;
    d.compute_minor    = 0;
    d.total_vram_bytes = 16ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = d.total_vram_bytes;
    d.is_healthy       = true;
    return d;
}

static DeviceInfo makeFakeCPUFallback(int index = 0) {
    DeviceInfo d;
    d.index        = index;
    d.device_index = index;
    d.name         = "CPU_FALLBACK";
    d.backend      = "CPU_FALLBACK";
    d.is_healthy   = true;
    return d;
}

static std::vector<DeviceInfo> twoGPUs() {
    return { makeFakeCUDA(0), makeFakeCUDA(1) };
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class P2PTransferTest : public ::testing::Test {
protected:
    GPUP2PTransferManager mgr;

    void SetUp() override {
        mgr.reset();
        GPUFeatureFlags::GetInstance().enable(
            GPUFeatureFlags::Feature::PEER_TO_PEER);
    }

    void TearDown() override {
        mgr.reset();
        GPUFeatureFlags::GetInstance().resetToDefaults();
    }
};

// ===========================================================================
// p2pStatusName
// ===========================================================================

TEST(P2PStatusNameTest, AllStatusValuesHaveNames) {
    EXPECT_STREQ(p2pStatusName(Status::OK), "OK");
    EXPECT_STREQ(p2pStatusName(Status::FEATURE_DISABLED), "FEATURE_DISABLED");
    EXPECT_STREQ(p2pStatusName(Status::PEER_ACCESS_NOT_SUPPORTED),
                 "PEER_ACCESS_NOT_SUPPORTED");
    EXPECT_STREQ(p2pStatusName(Status::PEER_ACCESS_ALREADY_ENABLED),
                 "PEER_ACCESS_ALREADY_ENABLED");
    EXPECT_STREQ(p2pStatusName(Status::PEER_ACCESS_NOT_ENABLED),
                 "PEER_ACCESS_NOT_ENABLED");
    EXPECT_STREQ(p2pStatusName(Status::INVALID_DEVICE), "INVALID_DEVICE");
    EXPECT_STREQ(p2pStatusName(Status::TRANSFER_FAILED), "TRANSFER_FAILED");
    EXPECT_STREQ(p2pStatusName(Status::OUT_OF_MEMORY), "OUT_OF_MEMORY");
}

// ===========================================================================
// Feature flag gate
// ===========================================================================

TEST_F(P2PTransferTest, FeatureDisabled_EnablePeerAccessReturnsFeatureDisabled) {
    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::PEER_TO_PEER);
    EXPECT_EQ(mgr.enablePeerAccess(0, 1, twoGPUs()),
              Status::FEATURE_DISABLED);
}

TEST_F(P2PTransferTest, FeatureDisabled_DisablePeerAccessReturnsFeatureDisabled) {
    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::PEER_TO_PEER);
    EXPECT_EQ(mgr.disablePeerAccess(0, 1), Status::FEATURE_DISABLED);
}

TEST_F(P2PTransferTest, FeatureDisabled_TransferReturnsError) {
    GPUFeatureFlags::GetInstance().disable(
        GPUFeatureFlags::Feature::PEER_TO_PEER);

    char src[8] = "hello";
    char dst[8] = {};
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = src;
    req.dst_ptr    = dst;
    req.size_bytes = sizeof(src);

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_FALSE(res.ok);
    EXPECT_FALSE(res.error_message.empty());

    const auto stats = mgr.getStats();
    EXPECT_EQ(stats.failed_transfers, 1u);
    EXPECT_EQ(stats.total_transfers, 0u);
}

// ===========================================================================
// canAccessPeer
// ===========================================================================

TEST_F(P2PTransferTest, CanAccessPeer_SameDevice_ReturnsFalse) {
    EXPECT_FALSE(mgr.canAccessPeer(0, 0, twoGPUs()));
}

TEST_F(P2PTransferTest, CanAccessPeer_InvalidIndex_ReturnsFalse) {
    EXPECT_FALSE(mgr.canAccessPeer(-1, 0, twoGPUs()));
    EXPECT_FALSE(mgr.canAccessPeer(0, 99, twoGPUs()));
}

TEST_F(P2PTransferTest, CanAccessPeer_CPUFallbackDevices_ReturnsFalse) {
    std::vector<DeviceInfo> devs = { makeFakeCPUFallback(0), makeFakeCPUFallback(1) };
    // CPU fallback devices never have hardware P2P support.
    EXPECT_FALSE(mgr.canAccessPeer(0, 1, devs));
}

// ===========================================================================
// enablePeerAccess / disablePeerAccess
// ===========================================================================

TEST_F(P2PTransferTest, EnablePeerAccess_InvalidDevice_ReturnsInvalidDevice) {
    EXPECT_EQ(mgr.enablePeerAccess(0, 99, twoGPUs()), Status::INVALID_DEVICE);
    EXPECT_EQ(mgr.enablePeerAccess(-1, 0, twoGPUs()), Status::INVALID_DEVICE);
}

TEST_F(P2PTransferTest, EnablePeerAccess_NoCUDA_ReturnsNotSupported) {
    // Without real CUDA/HIP hardware peer access is not supported.
    // The CPU simulation path always returns PEER_ACCESS_NOT_SUPPORTED.
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    EXPECT_EQ(mgr.enablePeerAccess(0, 1, twoGPUs()),
              Status::PEER_ACCESS_NOT_SUPPORTED);
#else
    // On real hardware the result depends on the driver.
    auto s = mgr.enablePeerAccess(0, 1, twoGPUs());
    EXPECT_TRUE(s == Status::OK || s == Status::PEER_ACCESS_NOT_SUPPORTED ||
                s == Status::INVALID_DEVICE);
#endif
}

TEST_F(P2PTransferTest, DisablePeerAccess_NotEnabled_ReturnsNotEnabled) {
    EXPECT_EQ(mgr.disablePeerAccess(0, 1), Status::PEER_ACCESS_NOT_ENABLED);
}

TEST_F(P2PTransferTest, DisablePeerAccess_AfterEnableFails_StillNotEnabled) {
    // On CPU path enablePeerAccess returns NOT_SUPPORTED (no hardware).
    // A subsequent disablePeerAccess must report NOT_ENABLED (pair was never
    // stored), not crash, and not corrupt stats.
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    auto en = mgr.enablePeerAccess(0, 1, twoGPUs());
    EXPECT_EQ(en, Status::PEER_ACCESS_NOT_SUPPORTED);
    EXPECT_EQ(mgr.disablePeerAccess(0, 1), Status::PEER_ACCESS_NOT_ENABLED);
    EXPECT_EQ(mgr.getStats().peer_access_enabled_count, 0u);
    EXPECT_EQ(mgr.getStats().peer_access_disabled_count, 0u);
#endif
}

TEST_F(P2PTransferTest, IsPeerAccessEnabled_InitiallyFalse) {
    EXPECT_FALSE(mgr.isPeerAccessEnabled(0, 1));
    EXPECT_FALSE(mgr.isPeerAccessEnabled(1, 0));
}

// ===========================================================================
// transfer — CPU simulation path (no CUDA/HIP hardware required)
// ===========================================================================

TEST_F(P2PTransferTest, Transfer_ZeroBytes_ReturnsOkImmediately) {
    char buf = 0;
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = &buf;
    req.dst_ptr    = &buf;
    req.size_bytes = 0;

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.bytes_transferred, 0u);
}

TEST_F(P2PTransferTest, Transfer_NullSrcPtr_ReturnsError) {
    char dst[4] = {};
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = nullptr;
    req.dst_ptr    = dst;
    req.size_bytes = 4;

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(mgr.getStats().failed_transfers, 1u);
}

TEST_F(P2PTransferTest, Transfer_NullDstPtr_ReturnsError) {
    char src[4] = "abc";
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = src;
    req.dst_ptr    = nullptr;
    req.size_bytes = 4;

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(mgr.getStats().failed_transfers, 1u);
}

TEST_F(P2PTransferTest, Transfer_InvalidDevice_ReturnsError) {
    char src[4] = {};
    char dst[4] = {};
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 99;
    req.src_ptr    = src;
    req.dst_ptr    = dst;
    req.size_bytes = 4;

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_FALSE(res.ok);
    EXPECT_EQ(mgr.getStats().failed_transfers, 1u);
}

TEST_F(P2PTransferTest, Transfer_CPUFallback_CopiesData) {
    // On the CPU simulation path (no CUDA/HIP hardware) transfer() performs a
    // memcpy and increments cpu_fallback_transfers.
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    const char src[] = "ThemisDB P2P";
    char dst[sizeof(src)] = {};

    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = src;
    req.dst_ptr    = dst;
    req.size_bytes = sizeof(src);

    auto res = mgr.transfer(req, twoGPUs());
    EXPECT_TRUE(res.ok);
    EXPECT_EQ(res.bytes_transferred, sizeof(src));
    EXPECT_STREQ(dst, "ThemisDB P2P");

    const auto stats = mgr.getStats();
    EXPECT_EQ(stats.total_transfers, 1u);
    EXPECT_EQ(stats.bytes_transferred, sizeof(src));
    EXPECT_EQ(stats.cpu_fallback_transfers, 1u);
    EXPECT_EQ(stats.failed_transfers, 0u);
#else
    GTEST_SKIP() << "capability:stub_path_active=false;reason=hardware_path_active";
#endif
}

TEST_F(P2PTransferTest, Transfer_CPUFallback_MultipleTransfers_StatsAccumulate) {
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    uint64_t src = 0xDEADBEEF;
    uint64_t dst = 0;

    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = &src;
    req.dst_ptr    = &dst;
    req.size_bytes = sizeof(src);

    for (int i = 0; i < 5; ++i) {
        auto res = mgr.transfer(req, twoGPUs());
        EXPECT_TRUE(res.ok);
    }

    const auto stats = mgr.getStats();
    EXPECT_EQ(stats.total_transfers, 5u);
    EXPECT_EQ(stats.bytes_transferred, 5u * sizeof(src));
    EXPECT_EQ(stats.cpu_fallback_transfers, 5u);
#else
    GTEST_SKIP() << "capability:stub_path_active=false;reason=hardware_path_active";
#endif
}

// ===========================================================================
// Statistics / reset
// ===========================================================================

TEST_F(P2PTransferTest, Stats_InitiallyZero) {
    const auto s = mgr.getStats();
    EXPECT_EQ(s.total_transfers, 0u);
    EXPECT_EQ(s.bytes_transferred, 0u);
    EXPECT_EQ(s.nvlink_transfers, 0u);
    EXPECT_EQ(s.pcie_transfers, 0u);
    EXPECT_EQ(s.cpu_fallback_transfers, 0u);
    EXPECT_EQ(s.failed_transfers, 0u);
    EXPECT_EQ(s.peer_access_enabled_count, 0u);
    EXPECT_EQ(s.peer_access_disabled_count, 0u);
}

TEST_F(P2PTransferTest, Reset_ClearsStats) {
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    char src[4] = "abc";
    char dst[4] = {};
    GPUP2PTransferManager::TransferRequest req;
    req.src_device = 0;
    req.dst_device = 1;
    req.src_ptr    = src;
    req.dst_ptr    = dst;
    req.size_bytes = sizeof(src);
    mgr.transfer(req, twoGPUs());

    EXPECT_GT(mgr.getStats().total_transfers, 0u);
    mgr.reset();
    EXPECT_EQ(mgr.getStats().total_transfers, 0u);
    EXPECT_EQ(mgr.getStats().bytes_transferred, 0u);
#else
    mgr.reset();
    EXPECT_EQ(mgr.getStats().total_transfers, 0u);
#endif
}

TEST_F(P2PTransferTest, Reset_ClearsPeerAccessState) {
    // After reset, isPeerAccessEnabled should always return false.
    mgr.reset();
    EXPECT_FALSE(mgr.isPeerAccessEnabled(0, 1));
}

// ===========================================================================
// Feature flag round-trip
// ===========================================================================

TEST_F(P2PTransferTest, FeatureFlag_EnableDisableRoundtrip) {
    auto& flags = GPUFeatureFlags::GetInstance();
    flags.enable(GPUFeatureFlags::Feature::PEER_TO_PEER);
    EXPECT_TRUE(flags.isEnabled(GPUFeatureFlags::Feature::PEER_TO_PEER));
    flags.disable(GPUFeatureFlags::Feature::PEER_TO_PEER);
    EXPECT_FALSE(flags.isEnabled(GPUFeatureFlags::Feature::PEER_TO_PEER));
    flags.resetToDefaults();
}

TEST(P2PFeatureFlagTest, PeerToPeer_InGetAll) {
    auto all = GPUFeatureFlags::GetInstance().getAll();
    bool found = false;
    for (const auto& fs : all) {
        if (fs.name == "PEER_TO_PEER") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "PEER_TO_PEER not found in GPUFeatureFlags::getAll()";
}

// ===========================================================================
// Singleton
// ===========================================================================

TEST_F(P2PTransferTest, GetInstance_ReturnsSameObject) {
    EXPECT_EQ(&GPUP2PTransferManager::GetInstance(),
              &GPUP2PTransferManager::GetInstance());
}

// ===========================================================================
// Concurrent safety
// ===========================================================================

TEST_F(P2PTransferTest, ConcurrentTransfers_CPUFallback_NoRaces) {
#if !defined(THEMIS_ENABLE_CUDA) && !defined(THEMIS_ENABLE_HIP)
    static constexpr int kThreads = 8;
    static constexpr int kIter    = 50;

    std::atomic<int> successes{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            GPUP2PTransferManager local_mgr;
            GPUFeatureFlags::GetInstance().enable(
                GPUFeatureFlags::Feature::PEER_TO_PEER);

            uint64_t src = static_cast<uint64_t>(t);
            uint64_t dst = 0;

            GPUP2PTransferManager::TransferRequest req;
            req.src_device = 0;
            req.dst_device = 1;
            req.src_ptr    = &src;
            req.dst_ptr    = &dst;
            req.size_bytes = sizeof(src);

            for (int i = 0; i < kIter; ++i) {
                auto res = local_mgr.transfer(req, twoGPUs());
                if (res.ok) {
                  ++successes;
                }
            }
        });
    }

    for (auto& th : threads) {
      th.join();
    }
    EXPECT_EQ(successes.load(), kThreads * kIter);
#else
    GTEST_SKIP() << "capability:stub_path_active=false;reason=hardware_path_active";
#endif
}
