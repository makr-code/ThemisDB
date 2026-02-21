// Test: Backend Capability Contract
//
// Validates the formal backend capability contract introduced in the
// acceleration module (Phase 1 Design/API).  Specifically it checks:
//  1. PrecisionMode enum values and bitwise helpers
//  2. metricBit() helper produces non-overlapping bitmask positions
//  3. BackendCapabilities struct carries the new fields with sane defaults
//  4. CPU backends populate the new contract fields correctly
//  5. BackendRegistry::getFallbackOrder() returns a non-empty chain
//     with CPU as the last entry
//
// These tests run on any platform (no GPU required).

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"

#include <algorithm>
#include <cstdint>
#include <type_traits>

using namespace themis::acceleration;

// =============================================================================
// PrecisionMode enum
// =============================================================================

TEST(BackendCapabilityContract, PrecisionModeUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<PrecisionMode>::type, uint32_t>::value,
        "PrecisionMode must use uint32_t as its underlying type"
    );
}

TEST(BackendCapabilityContract, PrecisionModeFP32IsSetInBit0) {
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::FP32), 1u);
}

TEST(BackendCapabilityContract, PrecisionModeFlagsAreNonZeroAndDistinct) {
    EXPECT_NE(static_cast<uint32_t>(PrecisionMode::FP32), 0u);
    EXPECT_NE(static_cast<uint32_t>(PrecisionMode::FP16), 0u);
    EXPECT_NE(static_cast<uint32_t>(PrecisionMode::BF16), 0u);
    EXPECT_NE(static_cast<uint32_t>(PrecisionMode::INT8), 0u);

    // All flags must be distinct powers-of-two
    const uint32_t fp32 = static_cast<uint32_t>(PrecisionMode::FP32);
    const uint32_t fp16 = static_cast<uint32_t>(PrecisionMode::FP16);
    const uint32_t bf16 = static_cast<uint32_t>(PrecisionMode::BF16);
    const uint32_t i8   = static_cast<uint32_t>(PrecisionMode::INT8);

    EXPECT_EQ(fp32 & fp16, 0u);
    EXPECT_EQ(fp32 & bf16, 0u);
    EXPECT_EQ(fp32 & i8,   0u);
    EXPECT_EQ(fp16 & bf16, 0u);
    EXPECT_EQ(fp16 & i8,   0u);
    EXPECT_EQ(bf16 & i8,   0u);
}

TEST(BackendCapabilityContract, PrecisionModeOrOperatorCombinesFlags) {
    const PrecisionMode combined = PrecisionMode::FP32 | PrecisionMode::FP16;
    EXPECT_TRUE(hasPrecision(combined, PrecisionMode::FP32));
    EXPECT_TRUE(hasPrecision(combined, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(combined, PrecisionMode::BF16));
    EXPECT_FALSE(hasPrecision(combined, PrecisionMode::INT8));
}

TEST(BackendCapabilityContract, PrecisionModeNoneIsZero) {
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::NONE), 0u);
    EXPECT_FALSE(hasPrecision(PrecisionMode::NONE, PrecisionMode::FP32));
}

// =============================================================================
// metricBit() helper
// =============================================================================

TEST(BackendCapabilityContract, MetricBitReturnsNonZero) {
    EXPECT_NE(metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(BackendCapabilityContract, MetricBitFlagsAreNonOverlapping) {
    const uint32_t l2 = metricBit(DistanceMetric::L2);
    const uint32_t cs = metricBit(DistanceMetric::COSINE);
    const uint32_t ip = metricBit(DistanceMetric::INNER_PRODUCT);

    EXPECT_EQ(l2 & cs, 0u);
    EXPECT_EQ(l2 & ip, 0u);
    EXPECT_EQ(cs & ip, 0u);
}

// =============================================================================
// BackendCapabilities default values
// =============================================================================

TEST(BackendCapabilityContract, BackendCapabilitiesDefaultsHavePrecisionNone) {
    BackendCapabilities caps;
    EXPECT_EQ(caps.supportedPrecisions, PrecisionMode::NONE);
}

TEST(BackendCapabilityContract, BackendCapabilitiesDefaultsHaveZeroMetrics) {
    BackendCapabilities caps;
    EXPECT_EQ(caps.supportedMetrics, 0u);
}

// =============================================================================
// CPU backend capability declarations
// =============================================================================

TEST(BackendCapabilityContract, CPUVectorBackend_CapabilitiesDeclaresFP32) {
    CPUVectorBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(BackendCapabilityContract, CPUVectorBackend_CapabilitiesDoesNotDeclareGPUPrecisions) {
    CPUVectorBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::BF16));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::INT8));
}

TEST(BackendCapabilityContract, CPUVectorBackend_CapabilitiesDeclaresAllANNMetrics) {
    CPUVectorBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::L2),            0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::COSINE),        0u);
    EXPECT_NE(caps.supportedMetrics & metricBit(DistanceMetric::INNER_PRODUCT), 0u);
}

TEST(BackendCapabilityContract, CPUVectorBackend_CapabilitiesSupportsVectorOps) {
    CPUVectorBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsGraphOps);
    EXPECT_FALSE(caps.supportsGeoOps);
}

TEST(BackendCapabilityContract, CPUGraphBackend_CapabilitiesDeclaresFP32) {
    CPUGraphBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(BackendCapabilityContract, CPUGraphBackend_CapabilitiesSupportsGraphOps) {
    CPUGraphBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_FALSE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsGraphOps);
    EXPECT_FALSE(caps.supportsGeoOps);
}

TEST(BackendCapabilityContract, CPUGeoBackend_CapabilitiesDeclaresFP32) {
    CPUGeoBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(BackendCapabilityContract, CPUGeoBackend_CapabilitiesSupportsGeoOps) {
    CPUGeoBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_FALSE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsGraphOps);
    EXPECT_TRUE(caps.supportsGeoOps);
}

// =============================================================================
// BackendRegistry fallback order contract
// =============================================================================

TEST(BackendCapabilityContract, FallbackOrderIsNonEmpty) {
    const auto& order = BackendRegistry::getFallbackOrder();
    EXPECT_FALSE(order.empty());
}

TEST(BackendCapabilityContract, FallbackOrderLastEntryIsCPU) {
    const auto& order = BackendRegistry::getFallbackOrder();
    ASSERT_FALSE(order.empty());
    EXPECT_EQ(order.back(), BackendType::CPU);
}

TEST(BackendCapabilityContract, FallbackOrderCUDABeforeHIPBeforeCPU) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const auto it_cuda = std::find(order.begin(), order.end(), BackendType::CUDA);
    const auto it_hip  = std::find(order.begin(), order.end(), BackendType::HIP);
    const auto it_cpu  = std::find(order.begin(), order.end(), BackendType::CPU);

    ASSERT_NE(it_cuda, order.end()) << "CUDA must appear in fallback order";
    ASSERT_NE(it_hip,  order.end()) << "HIP must appear in fallback order";
    ASSERT_NE(it_cpu,  order.end()) << "CPU must appear in fallback order";

    EXPECT_LT(it_cuda, it_hip)  << "CUDA must precede HIP in fallback order";
    EXPECT_LT(it_hip,  it_cpu)  << "HIP must precede CPU in fallback order";
}

TEST(BackendCapabilityContract, FallbackOrderContainsVulkan) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const auto it = std::find(order.begin(), order.end(), BackendType::VULKAN);
    EXPECT_NE(it, order.end()) << "Vulkan must appear in fallback order";
}

TEST(BackendCapabilityContract, FallbackOrderContainsNoDuplicates) {
    const auto& order = BackendRegistry::getFallbackOrder();
    for (size_t i = 0; i < order.size(); ++i) {
        for (size_t j = i + 1; j < order.size(); ++j) {
            EXPECT_NE(order[i], order[j])
                << "Duplicate BackendType at positions " << i << " and " << j;
        }
    }
}

// =============================================================================
// Registry integration: CPU backends are the final fallback
// =============================================================================

TEST(BackendCapabilityContract, RegistryBestVectorBackendReturnsCPU_WhenNoGPU) {
    // BackendRegistry singleton always registers CPU backends; on CI without
    // GPU hardware the best vector backend must be the CPU fallback.
    auto* vb = BackendRegistry::instance().getBestVectorBackend();
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

TEST(BackendCapabilityContract, RegistryBestGraphBackendReturnsCPU_WhenNoGPU) {
    auto* gb = BackendRegistry::instance().getBestGraphBackend();
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

TEST(BackendCapabilityContract, RegistryBestGeoBackendReturnsCPU_WhenNoGPU) {
    auto* geo = BackendRegistry::instance().getBestGeoBackend();
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}
