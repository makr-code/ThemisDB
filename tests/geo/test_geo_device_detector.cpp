#include <gtest/gtest.h>
#include "geo/device_detector.h"
#include "themis/gpu/device_discovery.h"

using namespace themis::geo;
using namespace themis::gpu;

// ---------------------------------------------------------------------------
// GeoDeviceDetector::Assess
// ---------------------------------------------------------------------------

TEST(GeoDeviceDetectorTest, Assess_CPUFallback_NotSuitable) {
    DeviceInfo d;
    d.index            = -1;
    d.device_index     = -1;
    d.name             = "CPU Fallback";
    d.backend          = "CPU_FALLBACK";
    d.is_healthy       = true;
    d.total_vram_bytes = 0;
    d.free_vram_bytes  = 0;
    d.compute_major    = 0;
    d.compute_minor    = 0;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_FALSE(cap.suitable_for_geo);
    EXPECT_FALSE(cap.meets_compute_requirement);
    EXPECT_FALSE(cap.meets_vram_requirement);
    EXPECT_FALSE(cap.reason.empty());
}

TEST(GeoDeviceDetectorTest, Assess_UnhealthyDevice_NotSuitable) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Dead GPU";
    d.backend          = "CUDA";
    d.is_healthy       = false;
    d.error_message    = "device lost";
    d.compute_major    = 8;
    d.compute_minor    = 0;
    d.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 20ULL * 1024 * 1024 * 1024;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_FALSE(cap.suitable_for_geo);
    EXPECT_FALSE(cap.reason.empty());
}

TEST(GeoDeviceDetectorTest, Assess_BelowComputeCapability_NotSuitable) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Old GPU";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = 3;  // Below kGeoMinComputeMajor (5)
    d.compute_minor    = 5;
    d.total_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 3ULL * 1024 * 1024 * 1024;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_FALSE(cap.suitable_for_geo);
    EXPECT_FALSE(cap.meets_compute_requirement);
    EXPECT_TRUE(cap.meets_vram_requirement);
    EXPECT_FALSE(cap.reason.empty());
}

TEST(GeoDeviceDetectorTest, Assess_InsufficientVRAM_NotSuitable) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Low VRAM GPU";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = 7;
    d.compute_minor    = 5;
    d.total_vram_bytes = 64ULL * 1024 * 1024;  // 64 MiB total
    d.free_vram_bytes  = 32ULL * 1024 * 1024;  // 32 MiB free (below 128 MiB threshold)

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_FALSE(cap.suitable_for_geo);
    EXPECT_TRUE(cap.meets_compute_requirement);
    EXPECT_FALSE(cap.meets_vram_requirement);
    EXPECT_FALSE(cap.reason.empty());
}

TEST(GeoDeviceDetectorTest, Assess_SuitableDevice_AllChecksPass) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "A100";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = 8;
    d.compute_minor    = 0;
    d.total_vram_bytes = 40ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 38ULL * 1024 * 1024 * 1024;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_TRUE(cap.suitable_for_geo);
    EXPECT_TRUE(cap.meets_compute_requirement);
    EXPECT_TRUE(cap.meets_vram_requirement);
    EXPECT_TRUE(cap.reason.empty());
}

TEST(GeoDeviceDetectorTest, Assess_ExactMinimumComputeCapability_IsSuitable) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Maxwell GPU";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = kGeoMinComputeMajor;
    d.compute_minor    = kGeoMinComputeMinor;
    d.total_vram_bytes = 4ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 2ULL * 1024 * 1024 * 1024;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_TRUE(cap.meets_compute_requirement);
    EXPECT_TRUE(cap.meets_vram_requirement);
    EXPECT_TRUE(cap.suitable_for_geo);
}

TEST(GeoDeviceDetectorTest, Assess_ExactMinimumVRAM_IsSuitable) {
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Tight VRAM GPU";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = 6;
    d.compute_minor    = 0;
    d.total_vram_bytes = kGeoMinVramBytes;
    d.free_vram_bytes  = kGeoMinVramBytes;

    const auto cap = GeoDeviceDetector::Assess(d);
    EXPECT_TRUE(cap.meets_vram_requirement);
    EXPECT_TRUE(cap.suitable_for_geo);
}

// ---------------------------------------------------------------------------
// GeoDeviceDetector::Detect
// ---------------------------------------------------------------------------

TEST(GeoDeviceDetectorTest, Detect_ReturnsNonEmptyList) {
    const auto caps = GeoDeviceDetector::Detect();
    EXPECT_FALSE(caps.empty());
}

TEST(GeoDeviceDetectorTest, Detect_AllEntriesHaveDeviceName) {
    const auto caps = GeoDeviceDetector::Detect();
    for (const auto& cap : caps) {
        EXPECT_FALSE(cap.device.name.empty())
            << "Device at index " << cap.device.index << " has no name";
    }
}

TEST(GeoDeviceDetectorTest, Detect_CPUFallbackIsNotSuitable) {
    const auto caps = GeoDeviceDetector::Detect();
    for (const auto& cap : caps) {
        if (cap.device.backend == "CPU_FALLBACK") {
            EXPECT_FALSE(cap.suitable_for_geo);
        }
    }
}

// ---------------------------------------------------------------------------
// GeoDeviceDetector::BestDevice
// ---------------------------------------------------------------------------

TEST(GeoDeviceDetectorTest, BestDevice_NoArg_ReturnsADevice) {
    const auto best = GeoDeviceDetector::BestDevice();
    EXPECT_FALSE(best.device.name.empty());
}

TEST(GeoDeviceDetectorTest, BestDevice_NoSuitableDevice_ReturnsCPUFallback) {
    // Build a list with only an unsuitable (too-old) GPU.
    DeviceInfo d;
    d.index            = 0;
    d.name             = "Ancient GPU";
    d.backend          = "CUDA";
    d.is_healthy       = true;
    d.compute_major    = 2;
    d.compute_minor    = 0;
    d.total_vram_bytes = 1ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 1ULL * 1024 * 1024 * 1024;

    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess(d));

    const auto best = GeoDeviceDetector::BestDevice(caps);
    EXPECT_EQ(best.device.backend, "CPU_FALLBACK");
    EXPECT_FALSE(best.suitable_for_geo);
}

TEST(GeoDeviceDetectorTest, BestDevice_PicksHighestFreeVRAM) {
    DeviceInfo d1, d2;
    d1.index = 0; d1.name = "GPU_A"; d1.backend = "CUDA"; d1.is_healthy = true;
    d1.compute_major = 8; d1.compute_minor = 0;
    d1.total_vram_bytes = 16ULL * 1024 * 1024 * 1024;
    d1.free_vram_bytes  =  4ULL * 1024 * 1024 * 1024;

    d2.index = 1; d2.name = "GPU_B"; d2.backend = "CUDA"; d2.is_healthy = true;
    d2.compute_major = 8; d2.compute_minor = 0;
    d2.total_vram_bytes = 40ULL * 1024 * 1024 * 1024;
    d2.free_vram_bytes  = 36ULL * 1024 * 1024 * 1024;

    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess(d1));
    caps.push_back(GeoDeviceDetector::Assess(d2));

    const auto best = GeoDeviceDetector::BestDevice(caps);
    EXPECT_EQ(best.device.name, "GPU_B");
}

// ---------------------------------------------------------------------------
// GeoDeviceDetector::HasSuitableDevice
// ---------------------------------------------------------------------------

TEST(GeoDeviceDetectorTest, HasSuitableDevice_CPUFallbackOnly_ReturnsFalse) {
    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess([]() {
        DeviceInfo d;
        d.backend = "CPU_FALLBACK";
        d.name    = "CPU Fallback";
        d.is_healthy = true;
        return d;
    }()));

    EXPECT_FALSE(GeoDeviceDetector::HasSuitableDevice(caps));
}

TEST(GeoDeviceDetectorTest, HasSuitableDevice_WithSuitableGPU_ReturnsTrue) {
    DeviceInfo d;
    d.index = 0; d.name = "RTX 4090"; d.backend = "CUDA"; d.is_healthy = true;
    d.compute_major = 8; d.compute_minor = 9;
    d.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 22ULL * 1024 * 1024 * 1024;

    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess(d));

    EXPECT_TRUE(GeoDeviceDetector::HasSuitableDevice(caps));
}

// ---------------------------------------------------------------------------
// GeoDeviceDetector::ReportJson
// ---------------------------------------------------------------------------

TEST(GeoDeviceDetectorTest, ReportJson_NoArg_ReturnsNonEmptyString) {
    const auto json = GeoDeviceDetector::ReportJson();
    EXPECT_FALSE(json.empty());
}

TEST(GeoDeviceDetectorTest, ReportJson_ContainsRequiredTopLevelFields) {
    const auto json = GeoDeviceDetector::ReportJson();
    EXPECT_NE(json.find("has_suitable_device"), std::string::npos);
    EXPECT_NE(json.find("devices"), std::string::npos);
}

TEST(GeoDeviceDetectorTest, ReportJson_EachDeviceHasRequiredFields) {
    DeviceInfo d;
    d.index = 0; d.name = "Test GPU"; d.backend = "CUDA"; d.is_healthy = true;
    d.compute_major = 7; d.compute_minor = 5;
    d.total_vram_bytes = 8ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 6ULL * 1024 * 1024 * 1024;

    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess(d));

    const auto json = GeoDeviceDetector::ReportJson(caps);
    EXPECT_NE(json.find("\"index\""),                std::string::npos);
    EXPECT_NE(json.find("\"name\""),                 std::string::npos);
    EXPECT_NE(json.find("\"backend\""),              std::string::npos);
    EXPECT_NE(json.find("\"total_vram_mb\""),        std::string::npos);
    EXPECT_NE(json.find("\"free_vram_mb\""),         std::string::npos);
    EXPECT_NE(json.find("\"compute_capability\""),   std::string::npos);
    EXPECT_NE(json.find("\"is_healthy\""),           std::string::npos);
    EXPECT_NE(json.find("\"suitable_for_geo\""),     std::string::npos);
    EXPECT_NE(json.find("\"reason\""),               std::string::npos);
}

TEST(GeoDeviceDetectorTest, ReportJson_SuitableDevice_HasSuitableDeviceTrue) {
    DeviceInfo d;
    d.index = 0; d.name = "A10G"; d.backend = "CUDA"; d.is_healthy = true;
    d.compute_major = 8; d.compute_minor = 6;
    d.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;
    d.free_vram_bytes  = 20ULL * 1024 * 1024 * 1024;

    std::vector<GeoDeviceCapability> caps;
    caps.push_back(GeoDeviceDetector::Assess(d));

    const auto json = GeoDeviceDetector::ReportJson(caps);
    EXPECT_NE(json.find("\"has_suitable_device\":true"), std::string::npos);
}

TEST(GeoDeviceDetectorTest, ReportJson_CPUFallbackOnly_HasSuitableDeviceFalse) {
    const auto caps = []() {
        std::vector<GeoDeviceCapability> v;
        DeviceInfo d;
        d.backend = "CPU_FALLBACK";
        d.name    = "CPU Fallback";
        d.is_healthy = true;
        v.push_back(GeoDeviceDetector::Assess(d));
        return v;
    }();

    const auto json = GeoDeviceDetector::ReportJson(caps);
    EXPECT_NE(json.find("\"has_suitable_device\":false"), std::string::npos);
}
