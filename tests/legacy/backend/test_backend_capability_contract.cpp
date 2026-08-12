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

namespace {
bool isCpuOrVulkan(BackendType type) {
    return type == BackendType::CPU || type == BackendType::VULKAN;
}

void ensureCpuFallbackBackends() {
    auto& registry = BackendRegistry::instance();
    if (registry.getBestVectorBackend() == nullptr) {
        registry.registerBackend(std::make_unique<CPUVectorBackend>());
    }
    if (registry.getBestGraphBackend() == nullptr) {
        registry.registerBackend(std::make_unique<CPUGraphBackend>());
    }
    if (registry.getBestGeoBackend() == nullptr) {
        registry.registerBackend(std::make_unique<CPUGeoBackend>());
    }
}
} // namespace

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
    ensureCpuFallbackBackends();
    // BackendRegistry singleton always registers CPU backends; on CI without
    // GPU hardware the best vector backend must be the CPU fallback.
    auto* vb = BackendRegistry::instance().getBestVectorBackend();
    ASSERT_NE(vb, nullptr);
    EXPECT_TRUE(isCpuOrVulkan(vb->type()));
}

TEST(BackendCapabilityContract, RegistryBestGraphBackendReturnsCPU_WhenNoGPU) {
    ensureCpuFallbackBackends();
    auto* gb = BackendRegistry::instance().getBestGraphBackend();
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

TEST(BackendCapabilityContract, RegistryBestGeoBackendReturnsCPU_WhenNoGPU) {
    ensureCpuFallbackBackends();
    auto* geo = BackendRegistry::instance().getBestGeoBackend();
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}

// =============================================================================
// satisfies() static helper
// =============================================================================

TEST(BackendCapabilityContract, Satisfies_EmptyRequirementsAlwaysPass) {
    // An empty requirements object imposes no constraints; every backend satisfies it.
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportedPrecisions = PrecisionMode::FP32;
    caps.supportedMetrics = metricBit(DistanceMetric::L2);

    BackendRegistry::CapabilityRequirements reqs; // all false / NONE / 0
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_VectorOpsRequired_Pass) {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportedPrecisions = PrecisionMode::FP32;
    caps.supportedMetrics = metricBit(DistanceMetric::L2);

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_VectorOpsRequired_Fail) {
    BackendCapabilities caps;
    caps.supportsVectorOps = false; // backend doesn't have it

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_PrecisionRequired_Pass) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions = PrecisionMode::FP32; // only FP32 required — OK
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_PrecisionRequired_Fail) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32; // no FP16

    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_MetricRequired_Pass) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);

    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2)
                         | metricBit(DistanceMetric::COSINE);
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST(BackendCapabilityContract, Satisfies_MetricRequired_Fail) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2); // only L2

    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2)
                         | metricBit(DistanceMetric::COSINE); // needs cosine too
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// =============================================================================
// selectBackendFor() — capability-driven selection
// =============================================================================

TEST(BackendCapabilityContract, SelectBackendFor_VectorOps_ReturnsCPU_WhenNoGPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* b = BackendRegistry::instance().selectBackendFor(reqs);
    ASSERT_NE(b, nullptr);
    EXPECT_TRUE(isCpuOrVulkan(b->type()));
}

TEST(BackendCapabilityContract, SelectVectorBackendFor_FP32_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2);

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_TRUE(isCpuOrVulkan(vb->type()));
}

TEST(BackendCapabilityContract, SelectGeoBackendFor_FP32_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* geo = BackendRegistry::instance().selectGeoBackendFor(reqs);
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}

TEST(BackendCapabilityContract, SelectGraphBackendFor_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;

    auto* gb = BackendRegistry::instance().selectGraphBackendFor(reqs);
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

TEST(BackendCapabilityContract, SelectBackendFor_ImpossibleRequirements_ReturnsNull) {
    // No backend supports async on CPU; asking for both vector + async
    // should return nullptr when only CPU backends are present.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsAsync     = true; // CPU backends don't expose async

    auto* b = BackendRegistry::instance().selectBackendFor(reqs);
    if (b == nullptr) {
        SUCCEED();
    } else {
        EXPECT_TRUE(b->getCapabilities().supportsAsync);
    }
}

TEST(BackendCapabilityContract, SelectVectorBackendFor_FP16_ReturnsNull_WhenNoGPU) {
    // CPU backends only declare FP32; requesting FP16 must return nullptr.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP16;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    EXPECT_EQ(vb, nullptr);
}

TEST(BackendCapabilityContract, SelectBackendFor_AllMetrics_ReturnsCPU) {
    // CPU vector backend declares all three ANN metrics.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps  = true;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2)
                         | metricBit(DistanceMetric::COSINE)
                         | metricBit(DistanceMetric::INNER_PRODUCT);

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_TRUE(isCpuOrVulkan(vb->type()));
}


#ifdef THEMIS_ENABLE_VULKAN
// =============================================================================
// Vulkan registration — requires THEMIS_ENABLE_VULKAN
// =============================================================================

// When compiled with Vulkan support, BackendRegistry must register a
// VulkanVectorBackend if a Vulkan ICD is present at runtime.
// On CI without a GPU the isAvailable() guard silently skips registration,
// so we only assert when the backend actually reports availability.
TEST(BackendCapabilityContract, RegistryContainsVulkanWhenAvailable) {
    auto& registry = BackendRegistry::instance();
    auto* vk = registry.getBackend(BackendType::VULKAN);
    if (vk == nullptr) {
        GTEST_SKIP() << "capability:vulkan_icd_available=false;reason=no_vulkan_icd_for_registration_check";
    }
    EXPECT_EQ(vk->type(), BackendType::VULKAN);
    auto caps = vk->getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps) << "Vulkan backend must declare vector ops";
    // vendorName may be empty on some ICD/driver combinations; do not enforce it as contract.
    SUCCEED();
}

// When Vulkan is available, getBestVectorBackend() must return Vulkan before
// falling back to CPU (CUDA/HIP/ZLUDA are higher priority but absent on
// non-NVIDIA CI runners, so Vulkan should win over CPU).
TEST(BackendCapabilityContract, RegistryBestVectorBackendPrefersVulkanOverCPU) {
    auto& registry = BackendRegistry::instance();
    auto* vk = registry.getBackend(BackendType::VULKAN);
    if (vk == nullptr) {
        GTEST_SKIP() << "capability:vulkan_icd_available=false;reason=no_vulkan_icd_available_on_system";
    }
    auto* best = registry.getBestVectorBackend();
    ASSERT_NE(best, nullptr);
    EXPECT_NE(best->type(), BackendType::CPU)
        << "When Vulkan is available, CPU must not be the best vector backend";
}
#endif // THEMIS_ENABLE_VULKAN
