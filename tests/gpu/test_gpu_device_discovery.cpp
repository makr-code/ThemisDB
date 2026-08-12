#include <gtest/gtest.h>
#include "themis/gpu/device_discovery.h"

using namespace themis::gpu;

// ---------------------------------------------------------------------------
// Enumerate
// ---------------------------------------------------------------------------

TEST(DeviceDiscoveryTest, Enumerate_ReturnsNonEmptyList) {
    // On CI without GPU hardware, expect the CPU-fallback sentinel.
    const auto devices = DeviceDiscovery::Enumerate();
    EXPECT_FALSE(devices.empty());
}

TEST(DeviceDiscoveryTest, Enumerate_AllDevicesHaveNames) {
    const auto devices = DeviceDiscovery::Enumerate();
    for (const auto& d : devices) {
        EXPECT_FALSE(d.name.empty()) << "Device at index " << d.index << " has no name";
    }
}

TEST(DeviceDiscoveryTest, Enumerate_AllDevicesHaveBackend) {
    const auto devices = DeviceDiscovery::Enumerate();
    for (const auto& d : devices) {
        EXPECT_FALSE(d.backend.empty()) << "Device at index " << d.index << " has no backend";
    }
}

TEST(DeviceDiscoveryTest, Enumerate_CPUFallbackHasNegativeIndex) {
    const auto devices = DeviceDiscovery::Enumerate();
    // If the list contains a CPU_FALLBACK entry its index must be -1.
    for (const auto& d : devices) {
        if (d.backend == "CPU_FALLBACK") {
            EXPECT_EQ(d.index, -1);
        }
    }
}

TEST(DeviceDiscoveryTest, Enumerate_CPUFallbackIsHealthy) {
    const auto devices = DeviceDiscovery::Enumerate();
    for (const auto& d : devices) {
        if (d.backend == "CPU_FALLBACK") {
            EXPECT_TRUE(d.is_healthy);
        }
    }
}

TEST(DeviceDiscoveryTest, Enumerate_HealthyDevicesHaveNoErrorMessage) {
    const auto devices = DeviceDiscovery::Enumerate();
    for (const auto& d : devices) {
        if (d.is_healthy) {
            EXPECT_TRUE(d.error_message.empty())
                << "Healthy device at index " << d.index
                << " has unexpected error: " << d.error_message;
        }
    }
}

// ---------------------------------------------------------------------------
// GetBestDevice
// ---------------------------------------------------------------------------

TEST(DeviceDiscoveryTest, GetBestDevice_ReturnsADevice) {
    const auto best = DeviceDiscovery::GetBestDevice();
    EXPECT_FALSE(best.name.empty());
    EXPECT_TRUE(best.is_healthy);
}

TEST(DeviceDiscoveryTest, GetBestDevice_FromDeviceList_ReturnsHighestFreeVRAM) {
    std::vector<DeviceInfo> devices;

    DeviceInfo d0;
    d0.index            = 0;
    d0.name             = "GPU_A";
    d0.backend          = "CUDA";
    d0.total_vram_bytes = 8ULL  * 1024 * 1024 * 1024;
    d0.free_vram_bytes  = 4ULL  * 1024 * 1024 * 1024;
    d0.is_healthy       = true;
    devices.push_back(d0);

    DeviceInfo d1;
    d1.index            = 1;
    d1.name             = "GPU_B";
    d1.backend          = "CUDA";
    d1.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;
    d1.free_vram_bytes  = 20ULL * 1024 * 1024 * 1024;
    d1.is_healthy       = true;
    devices.push_back(d1);

    const auto best = DeviceDiscovery::GetBestDevice(devices);
    EXPECT_EQ(best.name, "GPU_B");
    EXPECT_EQ(best.free_vram_bytes, 20ULL * 1024 * 1024 * 1024);
}

TEST(DeviceDiscoveryTest, GetBestDevice_SkipsUnhealthyDevices) {
    std::vector<DeviceInfo> devices;

    DeviceInfo bad;
    bad.index            = 0;
    bad.name             = "Bad_GPU";
    bad.backend          = "CUDA";
    bad.free_vram_bytes  = 100ULL * 1024 * 1024 * 1024;  // huge but unhealthy
    bad.is_healthy       = false;
    bad.error_message    = "device lost";
    devices.push_back(bad);

    DeviceInfo good;
    good.index           = 1;
    good.name            = "Good_GPU";
    good.backend         = "CUDA";
    good.free_vram_bytes = 8ULL  * 1024 * 1024 * 1024;
    good.is_healthy      = true;
    devices.push_back(good);

    const auto best = DeviceDiscovery::GetBestDevice(devices);
    EXPECT_EQ(best.name, "Good_GPU");
}

TEST(DeviceDiscoveryTest, GetBestDevice_AllUnhealthy_ReturnsCPUFallback) {
    std::vector<DeviceInfo> devices;

    DeviceInfo bad;
    bad.index       = 0;
    bad.name        = "Dead_GPU";
    bad.backend     = "CUDA";
    bad.is_healthy  = false;
    bad.error_message = "device lost";
    devices.push_back(bad);

    const auto best = DeviceDiscovery::GetBestDevice(devices);
    EXPECT_EQ(best.backend, "CPU_FALLBACK");
}

TEST(DeviceDiscoveryTest, GetBestDevice_EmptyList_ReturnsCPUFallback) {
    std::vector<DeviceInfo> empty;
    const auto best = DeviceDiscovery::GetBestDevice(empty);
    EXPECT_EQ(best.backend, "CPU_FALLBACK");
    EXPECT_TRUE(best.is_healthy);
}

// ---------------------------------------------------------------------------
// GetHealthyDevices
// ---------------------------------------------------------------------------

TEST(DeviceDiscoveryTest, GetHealthyDevices_FiltersUnhealthy) {
    std::vector<DeviceInfo> devices;

    DeviceInfo d_ok;
    d_ok.name      = "OK";
    d_ok.is_healthy = true;
    devices.push_back(d_ok);

    DeviceInfo d_bad;
    d_bad.name      = "BAD";
    d_bad.is_healthy = false;
    d_bad.error_message = "lost";
    devices.push_back(d_bad);

    const auto healthy = DeviceDiscovery::GetHealthyDevices(devices);
    ASSERT_EQ(healthy.size(), 1u);
    EXPECT_EQ(healthy[0].name, "OK");
}

TEST(DeviceDiscoveryTest, GetHealthyDevices_AllHealthy_ReturnsSameCount) {
    std::vector<DeviceInfo> devices;
    for (int i = 0; i < 3; ++i) {
        DeviceInfo d;
        d.index      = i;
        d.name       = "dev_" + std::to_string(i);
        d.is_healthy = true;
        devices.push_back(d);
    }
    EXPECT_EQ(DeviceDiscovery::GetHealthyDevices(devices).size(), 3u);
}

// ---------------------------------------------------------------------------
// HasGPU
// ---------------------------------------------------------------------------

TEST(DeviceDiscoveryTest, HasGPU_CPUFallbackOnly_ReturnsFalse) {
    std::vector<DeviceInfo> devices;
    DeviceInfo cpu;
    cpu.backend     = "CPU_FALLBACK";
    cpu.is_healthy  = true;
    devices.push_back(cpu);

    EXPECT_FALSE(DeviceDiscovery::HasGPU(devices));
}

TEST(DeviceDiscoveryTest, HasGPU_RealGPUPresent_ReturnsTrue) {
    std::vector<DeviceInfo> devices;
    DeviceInfo d;
    d.backend    = "CUDA";
    d.is_healthy = true;
    devices.push_back(d);

    EXPECT_TRUE(DeviceDiscovery::HasGPU(devices));
}

TEST(DeviceDiscoveryTest, HasGPU_UnhealthyGPUOnly_ReturnsFalse) {
    std::vector<DeviceInfo> devices;
    DeviceInfo d;
    d.backend       = "CUDA";
    d.is_healthy    = false;
    d.error_message = "device lost";
    devices.push_back(d);

    EXPECT_FALSE(DeviceDiscovery::HasGPU(devices));
}

// ---------------------------------------------------------------------------
// VRAM / edition limits consistency
// ---------------------------------------------------------------------------

TEST(DeviceDiscoveryTest, CPUFallbackVRAM_MatchesEditionLimit) {
    // The CPU fallback sentinel's total_vram_bytes must equal the edition's
    // VRAM limit so the memory manager and fallback logic stay consistent.
    const auto devices = DeviceDiscovery::Enumerate();
    for (const auto& d : devices) {
        if (d.backend == "CPU_FALLBACK") {
            const uint64_t edition_limit =
                static_cast<uint64_t>(themis::edition::GPU_MAX_VRAM_GB)
                * 1024ULL * 1024ULL * 1024ULL;
            EXPECT_EQ(d.total_vram_bytes, edition_limit);
        }
    }
}
