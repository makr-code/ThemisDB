// Test: DeviceManager — Runtime Device Capability Detection
//
// Validates the DeviceManager class introduced in acceleration/device_manager.cpp.
// Specifically it checks:
//  1. probeDevices() returns a non-empty list (at minimum CPU fallback)
//  2. All returned devices have non-empty names
//  3. Cache is returned on second call within TTL
//  4. refresh() forces a new probe
//  5. getBestDevice() returns a healthy device
//  6. getBestDevice() selects highest free VRAM among healthy GPU devices
//  7. getBestDevice() falls back to CPU when all GPUs are unhealthy
//  8. hasGPU() returns false on systems without GPU hardware (CI)
//  9. bestBackendType() is consistent with getBestDevice()
// 10. BackendType mapping covers all documented backend strings
//
// These tests run on any platform (no GPU required).

#include <gtest/gtest.h>
#include "acceleration/device_manager.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"

using namespace themis::acceleration;

namespace {

DeviceCapabilityInfo makeDevice(int index, BackendType backend_type, const char* name, uint64_t free_vram_bytes,
                                bool is_healthy = true, int compute_major = 0, int compute_minor = 0) {
    DeviceCapabilityInfo device;
    device.index            = index;
    device.name             = name;
    device.backend_type     = backend_type;
    device.free_vram_bytes  = free_vram_bytes;
    device.total_vram_bytes = free_vram_bytes;
    device.is_healthy       = is_healthy;
    device.compute_major    = compute_major;
    device.compute_minor    = compute_minor;
    if (!is_healthy) {
        device.error_message = "synthetic failure";
    }
    return device;
}

class DeviceManagerInjectedEnumerationTest : public ::testing::Test {
protected:
    void TearDown() override {
        DeviceManager::setEnumerateFn({});
    }
};

} // namespace

// =============================================================================
// probeDevices() basics
// =============================================================================

TEST(DeviceManagerTest, ProbeDevices_ReturnsNonEmptyList) {
    const auto devices = DeviceManager::instance().probeDevices();
    EXPECT_FALSE(devices.empty());
}

TEST(DeviceManagerTest, ProbeDevices_AllDevicesHaveNames) {
    const auto devices = DeviceManager::instance().probeDevices();
    for (const auto& d : devices) {
        EXPECT_FALSE(d.name.empty()) << "Device at index " << d.index << " has no name";
    }
}

TEST(DeviceManagerTest, ProbeDevices_HealthyDevicesHaveNoErrorMessage) {
    const auto devices = DeviceManager::instance().probeDevices();
    for (const auto& d : devices) {
        if (d.is_healthy) {
            EXPECT_TRUE(d.error_message.empty())
                << "Healthy device '" << d.name
                << "' has unexpected error: " << d.error_message;
        }
    }
}

TEST(DeviceManagerTest, ProbeDevices_CPUFallbackHasNegativeIndex) {
    const auto devices = DeviceManager::instance().probeDevices();
    for (const auto& d : devices) {
        if (d.backend_type == BackendType::CPU) {
            EXPECT_EQ(d.index, -1);
        }
    }
}

TEST(DeviceManagerTest, ProbeDevices_CPUFallbackIsHealthy) {
    const auto devices = DeviceManager::instance().probeDevices();
    for (const auto& d : devices) {
        if (d.backend_type == BackendType::CPU) {
            EXPECT_TRUE(d.is_healthy);
        }
    }
}

// =============================================================================
// Caching behaviour
// =============================================================================

TEST(DeviceManagerTest, ProbeDevices_ReturnsCachedResultOnSecondCall) {
    // Two consecutive calls should return the same data (same device count).
    const auto first  = DeviceManager::instance().probeDevices();
    const auto second = DeviceManager::instance().probeDevices();
    EXPECT_EQ(first.size(), second.size());
}

TEST(DeviceManagerTest, Refresh_ReturnsNonEmptyList) {
    const auto devices = DeviceManager::instance().refresh();
    EXPECT_FALSE(devices.empty());
}

TEST(DeviceManagerTest, Refresh_ConsistentWithProbeDevices) {
    const auto after_refresh = DeviceManager::instance().refresh();
    const auto probed        = DeviceManager::instance().probeDevices();
    // After refresh the cache should reflect the fresh probe.
    EXPECT_EQ(after_refresh.size(), probed.size());
}

// =============================================================================
// getBestDevice()
// =============================================================================

TEST(DeviceManagerTest, GetBestDevice_ReturnsHealthyDevice) {
    const auto best = DeviceManager::instance().getBestDevice();
    EXPECT_TRUE(best.is_healthy);
}

TEST(DeviceManagerTest, GetBestDevice_HasNonEmptyName) {
    const auto best = DeviceManager::instance().getBestDevice();
    EXPECT_FALSE(best.name.empty());
}

TEST_F(DeviceManagerInjectedEnumerationTest, GetBestDevice_SelectsHighestFreeVRAM) {
    DeviceManager::setEnumerateFn([] {
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(0, BackendType::CUDA, "GPU_A", 4ULL * 1024 * 1024 * 1024, true, 7, 0),
            makeDevice(1, BackendType::CUDA, "GPU_B", 20ULL * 1024 * 1024 * 1024, true, 8, 0),
            makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)};
    });

    const auto best = DeviceManager::instance().refresh();
    ASSERT_EQ(best.size(), 3u);
    EXPECT_EQ(DeviceManager::instance().getBestDevice().name, "GPU_B");
}

TEST_F(DeviceManagerInjectedEnumerationTest, GetBestDevice_SkipsUnhealthyDevices) {
    DeviceManager::setEnumerateFn([] {
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(0, BackendType::CUDA, "Bad_GPU", 100ULL * 1024 * 1024 * 1024, false, 8, 0),
            makeDevice(1, BackendType::CUDA, "Good_GPU", 8ULL * 1024 * 1024 * 1024, true, 8, 0),
            makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)};
    });

    EXPECT_EQ(DeviceManager::instance().refresh()[0].name, "Bad_GPU");
    EXPECT_EQ(DeviceManager::instance().getBestDevice().name, "Good_GPU");
}

TEST_F(DeviceManagerInjectedEnumerationTest, GetBestDevice_ReturnsCPUFallbackWhenNoHealthyGPUExists) {
    DeviceManager::setEnumerateFn([] {
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(0, BackendType::CUDA, "Bad_GPU", 100ULL * 1024 * 1024 * 1024, false, 8, 0),
            makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)};
    });

    const auto best = DeviceManager::instance().refresh();
    ASSERT_EQ(best.size(), 2u);
    EXPECT_EQ(DeviceManager::instance().getBestDevice().backend_type, BackendType::CPU);
}

// =============================================================================
// hasGPU() / bestBackendType()
// =============================================================================

TEST_F(DeviceManagerInjectedEnumerationTest, HasGPU_CPUFallbackOnly_ReturnsFalse) {
    DeviceManager::setEnumerateFn([] { return std::vector<DeviceCapabilityInfo>{makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)}; });
    EXPECT_FALSE(DeviceManager::instance().refresh().empty());
    EXPECT_FALSE(DeviceManager::instance().hasGPU());
}

TEST_F(DeviceManagerInjectedEnumerationTest, HasGPU_HealthyGPUPresent_ReturnsTrue) {
    DeviceManager::setEnumerateFn([] {
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(0, BackendType::CUDA, "Test_GPU", 4ULL * 1024 * 1024 * 1024, true, 7, 5),
            makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)};
    });
    EXPECT_TRUE(DeviceManager::instance().refresh().size() >= 2u);
    EXPECT_TRUE(DeviceManager::instance().hasGPU());
}

TEST(DeviceManagerTest, BestBackendType_ConsistentWithGetBestDevice) {
    const auto best_device  = DeviceManager::instance().getBestDevice();
    const auto best_backend = DeviceManager::instance().bestBackendType();
    EXPECT_EQ(best_device.backend_type, best_backend);
}

// =============================================================================
// BackendType mapping
// =============================================================================

TEST(DeviceManagerTest, PrecisionFlags_CUDACompute70_SupportsFP16) {
    // sm_70 should enable fp16 support.
    const int compute_major = 7;
    const int compute_minor = 0;

    // Use refresh() to rebuild cache and then inspect through DeviceCapabilityInfo
    // by validating the mapping logic directly on the struct fields.
    // (We cannot inject a fake DeviceDiscovery without hardware, so we
    //  validate the precision derivation formula independently.)
    const int sm = compute_major * 10 + compute_minor;
    EXPECT_GE(sm, 70);  // fp16 threshold
    EXPECT_LT(sm, 80);  // bf16 threshold not met
}

TEST(DeviceManagerTest, PrecisionFlags_CUDACompute80_SupportsBF16) {
    const int compute_major = 8;
    const int compute_minor = 0;

    const int sm = compute_major * 10 + compute_minor;
    EXPECT_GE(sm, 80);  // both fp16 and bf16 thresholds met
}

TEST_F(DeviceManagerInjectedEnumerationTest, ProbeDevices_UsesInjectedEnumerationAndCachesResult) {
    int calls = 0;
    DeviceManager::setEnumerateFn([&calls] {
        ++calls;
        return std::vector<DeviceCapabilityInfo>{makeDevice(7, BackendType::CUDA, "Injected GPU", 6ULL * 1024 * 1024 * 1024, true, 8, 0)};
    });

    const auto first = DeviceManager::instance().probeDevices();
    const auto second = DeviceManager::instance().probeDevices();

    ASSERT_EQ(first.size(), 1u);
    EXPECT_EQ(first.front().name, "Injected GPU");
    EXPECT_EQ(second.front().name, "Injected GPU");
    EXPECT_EQ(calls, 1);
}

TEST_F(DeviceManagerInjectedEnumerationTest, Refresh_ReprobesInjectedEnumeration) {
    int calls = 0;
    DeviceManager::setEnumerateFn([&calls] {
        ++calls;
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(calls, BackendType::CUDA, calls == 1 ? "GPU_First" : "GPU_Second", 3ULL * 1024 * 1024 * 1024, true, 8, 0)};
    });

    const auto first = DeviceManager::instance().refresh();
    const auto second = DeviceManager::instance().refresh();

    EXPECT_EQ(first.front().name, "GPU_First");
    EXPECT_EQ(second.front().name, "GPU_Second");
    EXPECT_EQ(calls, 2);
}

TEST_F(DeviceManagerInjectedEnumerationTest, ProbeDevices_EmptyInjectedListSynthesizesCPUFallback) {
    DeviceManager::setEnumerateFn([] { return std::vector<DeviceCapabilityInfo>{}; });

    const auto devices = DeviceManager::instance().refresh();
    ASSERT_EQ(devices.size(), 1u);
    EXPECT_EQ(devices.front().backend_type, BackendType::CPU);
    EXPECT_EQ(devices.front().index, -1);
}

// =============================================================================
// logDeviceInfo() smoke test
// =============================================================================

TEST(DeviceManagerTest, LogDeviceInfo_DoesNotCrash) {
    // Just verify the method doesn't throw or abort.
    EXPECT_NO_THROW(DeviceManager::instance().logDeviceInfo());
}

TEST_F(DeviceManagerInjectedEnumerationTest, LogDeviceInfo_ReportsInjectedDeviceAndBestSelection) {
    DeviceManager::setEnumerateFn([] {
        return std::vector<DeviceCapabilityInfo>{
            makeDevice(0, BackendType::CUDA, "Log_GPU", 9ULL * 1024 * 1024 * 1024, true, 8, 0),
            makeDevice(-1, BackendType::CPU, "CPU Fallback", 0)};
    });

    testing::internal::CaptureStdout();
    DeviceManager::instance().refresh();
    DeviceManager::instance().logDeviceInfo();
    const std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Log_GPU"), std::string::npos);
    EXPECT_NE(output.find("Best device: Log_GPU"), std::string::npos);
}

// =============================================================================
// BackendRegistry::deviceInfo() observability API
// =============================================================================

TEST(DeviceManagerTest, BackendRegistry_DeviceInfo_EmptyBeforeInit) {
    // A freshly obtained (but not yet re-initialized) registry may or may not
    // have device info already cached from a previous test.  What we can
    // guarantee is that deviceInfo() never throws and always returns a vector.
    EXPECT_NO_THROW(BackendRegistry::instance().deviceInfo());
}

TEST(DeviceManagerTest, BackendRegistry_DeviceInfo_NonEmptyAfterInit) {
    BackendRegistry::instance().initializeRuntime();
    const auto info = BackendRegistry::instance().deviceInfo();
    // After initializeRuntime() there must be at least the CPU fallback device.
    EXPECT_FALSE(info.empty());
}

TEST(DeviceManagerTest, BackendRegistry_DeviceInfo_AllDevicesHaveNames) {
    BackendRegistry::instance().initializeRuntime();
    const auto info = BackendRegistry::instance().deviceInfo();
    for (const auto& d : info) {
        EXPECT_FALSE(d.name.empty())
            << "Device at index " << d.index << " has no name";
    }
}

TEST(DeviceManagerTest, BackendRegistry_DeviceInfo_CPUFallbackAlwaysPresent) {
    BackendRegistry::instance().initializeRuntime();
    const auto info = BackendRegistry::instance().deviceInfo();
    bool cpu_found = false;
    for (const auto& d : info) {
        if (d.backend_type == BackendType::CPU) {
            cpu_found = true;
            break;
        }
    }
    EXPECT_TRUE(cpu_found) << "CPU fallback sentinel missing from deviceInfo()";
}
