// Test: Backend Selection and Capability Negotiation Matrix
//
// Comprehensive unit tests for the capability-driven backend selection
// algorithm and the full capability negotiation matrix.  Specifically tests:
//
//  1. Full fallback priority order (MULTI_GPU → CUDA → HIP → … → CPU)
//  2. selectMatrixBackendFor() and getBestMatrixBackend() for CPU matrix backend
//  3. CPUMatrixBackend capability declarations
//  4. Capability negotiation matrix — all combinations of boolean op flags
//  5. Precision requirement matrix — FP32/FP16/BF16/INT8 individual and combined
//  6. Distance metric requirement matrix — L2/COSINE/INNER_PRODUCT combinations
//  7. needsBatch and needsAsync requirement flags
//  8. getAvailableBackends() correctness
//  9. BackendHealthStatus construction helpers (makeHealthy/makeDegraded/makeUnhealthy)
// 10. Multi-requirement selection (vector+batch, geo+FP32, graph+batch)
//
// These tests run on any platform (no GPU required).

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"

#include <algorithm>
#include <cstdint>

using namespace themis::acceleration;

namespace {
void ensureCpuMatrixBackend() {
    auto& registry = BackendRegistry::instance();
    if (registry.getBestMatrixBackend() == nullptr) {
        registry.registerBackend(std::make_unique<CPUMatrixBackend>());
    }
}
} // namespace

// =============================================================================
// Fallback priority order
// =============================================================================

TEST(BackendSelectionMatrix, FallbackOrderStartsWithMultiGPU) {
    const auto& order = BackendRegistry::getFallbackOrder();
    ASSERT_FALSE(order.empty());
    EXPECT_EQ(order.front(), BackendType::MULTI_GPU)
        << "MULTI_GPU must be the highest-priority entry in the fallback order";
}

TEST(BackendSelectionMatrix, FallbackOrderCUDABeforeZLUDA) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const auto it_cuda  = std::find(order.begin(), order.end(), BackendType::CUDA);
    const auto it_zluda = std::find(order.begin(), order.end(), BackendType::ZLUDA);
    ASSERT_NE(it_cuda,  order.end()) << "CUDA must appear in fallback order";
    ASSERT_NE(it_zluda, order.end()) << "ZLUDA must appear in fallback order";
    EXPECT_LT(it_cuda, it_zluda) << "CUDA must precede ZLUDA in fallback order";
}

TEST(BackendSelectionMatrix, FallbackOrderVulkanBeforeCPU) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const auto it_vk  = std::find(order.begin(), order.end(), BackendType::VULKAN);
    const auto it_cpu = std::find(order.begin(), order.end(), BackendType::CPU);
    ASSERT_NE(it_vk,  order.end()) << "VULKAN must appear in fallback order";
    ASSERT_NE(it_cpu, order.end()) << "CPU must appear in fallback order";
    EXPECT_LT(it_vk, it_cpu) << "VULKAN must precede CPU in fallback order";
}

TEST(BackendSelectionMatrix, FallbackOrderDirectXBeforeROCm) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const auto it_dx   = std::find(order.begin(), order.end(), BackendType::DIRECTX);
    const auto it_rocm = std::find(order.begin(), order.end(), BackendType::ROCM);
    ASSERT_NE(it_dx,   order.end()) << "DIRECTX must appear in fallback order";
    ASSERT_NE(it_rocm, order.end()) << "ROCM must appear in fallback order";
    EXPECT_LT(it_dx, it_rocm) << "DIRECTX must precede ROCM in fallback order";
}

TEST(BackendSelectionMatrix, FallbackOrderContainsAllExpectedTypes) {
    const auto& order = BackendRegistry::getFallbackOrder();
    const std::vector<BackendType> expected = {
        BackendType::MULTI_GPU,
        BackendType::CUDA,
        BackendType::HIP,
        BackendType::ZLUDA,
        BackendType::VULKAN,
        BackendType::DIRECTX,
        BackendType::ROCM,
        BackendType::ONEAPI,
        BackendType::METAL,
        BackendType::OPENCL,
        BackendType::OPENGL,
        BackendType::WEBGPU,
        BackendType::CPU,
    };
    for (const auto bt : expected) {
        const auto it = std::find(order.begin(), order.end(), bt);
        EXPECT_NE(it, order.end())
            << "BackendType " << static_cast<int>(bt) << " must appear in fallback order";
    }
}

// =============================================================================
// CPUMatrixBackend capability declarations
// =============================================================================

TEST(BackendSelectionMatrix, CPUMatrixBackend_IsAvailable) {
    CPUMatrixBackend backend;
    EXPECT_TRUE(backend.isAvailable());
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_TypeIsCPU) {
    CPUMatrixBackend backend;
    EXPECT_EQ(backend.type(), BackendType::CPU);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_NameIsCPU) {
    CPUMatrixBackend backend;
    EXPECT_STREQ(backend.name(), "CPU");
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_SupportsMatrixOps) {
    CPUMatrixBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsMatrixOps);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_DoesNotSupportVectorGraphGeoOps) {
    CPUMatrixBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_FALSE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsGraphOps);
    EXPECT_FALSE(caps.supportsGeoOps);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_SupportsBatchProcessing) {
    CPUMatrixBackend backend;
    EXPECT_TRUE(backend.getCapabilities().supportsBatchProcessing);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_DoesNotSupportAsync) {
    CPUMatrixBackend backend;
    EXPECT_FALSE(backend.getCapabilities().supportsAsync);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_SupportsFP32) {
    CPUMatrixBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_TRUE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP32));
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_DoesNotSupportGPUPrecisions) {
    CPUMatrixBackend backend;
    const auto caps = backend.getCapabilities();
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::BF16));
    EXPECT_FALSE(hasPrecision(caps.supportedPrecisions, PrecisionMode::INT8));
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_InitializeSucceeds) {
    CPUMatrixBackend backend;
    EXPECT_TRUE(backend.initialize());
}

// =============================================================================
// getBestMatrixBackend() and selectMatrixBackendFor()
// =============================================================================

TEST(BackendSelectionMatrix, GetBestMatrixBackend_ReturnsCPU_WhenNoGPU) {
    ensureCpuMatrixBackend();
    auto* mb = BackendRegistry::instance().getBestMatrixBackend();
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(mb->type(), BackendType::CPU);
}

TEST(BackendSelectionMatrix, SelectMatrixBackendFor_FP32_ReturnsCPU) {
    ensureCpuMatrixBackend();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps      = true;
    reqs.requiredPrecisions  = PrecisionMode::FP32;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(mb->type(), BackendType::CPU);
}

TEST(BackendSelectionMatrix, SelectMatrixBackendFor_FP16_ReturnsNull_WhenNoGPU) {
    // CPU matrix backend declares only FP32; requesting FP16 must yield nullptr.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps     = true;
    reqs.requiredPrecisions = PrecisionMode::FP16;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    EXPECT_EQ(mb, nullptr);
}

TEST(BackendSelectionMatrix, SelectMatrixBackendFor_Async_ReturnsNull_WhenNoGPU) {
    // CPU matrix backend does not support async.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps = true;
    reqs.needsAsync     = true;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    EXPECT_EQ(mb, nullptr);
}

TEST(BackendSelectionMatrix, SelectMatrixBackendFor_BatchProcessing_ReturnsCPU) {
    ensureCpuMatrixBackend();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps = true;
    reqs.needsBatch     = true;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(mb->type(), BackendType::CPU);
}

// =============================================================================
// Capability negotiation matrix — boolean op flag combinations
// =============================================================================

// vector + batch → CPU (CPUVectorBackend supports both)
TEST(BackendSelectionMatrix, Satisfies_VectorAndBatch_Pass) {
    BackendCapabilities caps;
    caps.supportsVectorOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;
    caps.supportedMetrics        = metricBit(DistanceMetric::L2);

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsBatch     = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// vector but no batch declared → fails when batch required
TEST(BackendSelectionMatrix, Satisfies_VectorNoBatch_Fail) {
    BackendCapabilities caps;
    caps.supportsVectorOps       = true;
    caps.supportsBatchProcessing = false;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsBatch     = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// graph + batch → CPU
TEST(BackendSelectionMatrix, Satisfies_GraphAndBatch_Pass) {
    BackendCapabilities caps;
    caps.supportsGraphOps        = true;
    caps.supportsBatchProcessing = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;
    reqs.needsBatch    = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// geo + batch
TEST(BackendSelectionMatrix, Satisfies_GeoAndBatch_Pass) {
    BackendCapabilities caps;
    caps.supportsGeoOps          = true;
    caps.supportsBatchProcessing = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps = true;
    reqs.needsBatch  = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// matrix + batch
TEST(BackendSelectionMatrix, Satisfies_MatrixAndBatch_Pass) {
    BackendCapabilities caps;
    caps.supportsMatrixOps       = true;
    caps.supportsBatchProcessing = true;
    caps.supportedPrecisions     = PrecisionMode::FP32;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps = true;
    reqs.needsBatch     = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// All four ops required but backend only has vector → fail
TEST(BackendSelectionMatrix, Satisfies_AllOpsRequired_OnlyVectorPresent_Fail) {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportsGraphOps  = false;
    caps.supportsGeoOps    = false;
    caps.supportsMatrixOps = false;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsGraphOps  = true;
    reqs.needsGeoOps    = true;
    reqs.needsMatrixOps = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// needsAsync when backend doesn't support async → fail
TEST(BackendSelectionMatrix, Satisfies_AsyncRequired_CPUFails) {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportsAsync     = false;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsAsync     = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// needsAsync when backend supports async → pass
TEST(BackendSelectionMatrix, Satisfies_AsyncRequired_AsyncBackendPasses) {
    BackendCapabilities caps;
    caps.supportsVectorOps = true;
    caps.supportsAsync     = true;
    caps.supportedPrecisions = PrecisionMode::FP32;

    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsAsync     = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// =============================================================================
// Precision requirement matrix
// =============================================================================

// FP32 only required, FP32 available → pass
TEST(BackendSelectionMatrix, Satisfies_FP32Required_FP32Only_Pass) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::FP32;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// FP16 required, only FP32 available → fail
TEST(BackendSelectionMatrix, Satisfies_FP16Required_FP32Only_Fail) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::FP16;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// BF16 required, only FP32 available → fail
TEST(BackendSelectionMatrix, Satisfies_BF16Required_FP32Only_Fail) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::BF16;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// INT8 required, only FP32 available → fail
TEST(BackendSelectionMatrix, Satisfies_INT8Required_FP32Only_Fail) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::INT8;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// FP32|FP16 required, all four available → pass
TEST(BackendSelectionMatrix, Satisfies_FP32AndFP16Required_AllAvailable_Pass) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16
                             | PrecisionMode::BF16 | PrecisionMode::INT8;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::FP32 | PrecisionMode::FP16;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// FP32|BF16 required, only FP32|FP16 available → fail
TEST(BackendSelectionMatrix, Satisfies_FP32AndBF16Required_OnlyFP32FP16Available_Fail) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::FP32 | PrecisionMode::FP16;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::FP32 | PrecisionMode::BF16;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// NONE required → always pass regardless of available precisions
TEST(BackendSelectionMatrix, Satisfies_NoPrecisionRequired_AlwaysPass) {
    BackendCapabilities caps;
    caps.supportedPrecisions = PrecisionMode::NONE;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions  = PrecisionMode::NONE;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// =============================================================================
// Distance metric requirement matrix
// =============================================================================

// L2 only required, L2 available → pass
TEST(BackendSelectionMatrix, Satisfies_L2Required_L2Available_Pass) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2);
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = metricBit(DistanceMetric::L2);
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// COSINE required, only L2 available → fail
TEST(BackendSelectionMatrix, Satisfies_CosineRequired_L2OnlyAvailable_Fail) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2);
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = metricBit(DistanceMetric::COSINE);
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// INNER_PRODUCT required, only L2|COSINE available → fail
TEST(BackendSelectionMatrix, Satisfies_IPRequired_L2CosineOnly_Fail) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2) | metricBit(DistanceMetric::COSINE);
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = metricBit(DistanceMetric::INNER_PRODUCT);
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

// All three metrics required and all available → pass
TEST(BackendSelectionMatrix, Satisfies_AllMetricsRequired_AllAvailable_Pass) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// Zero metric requirement → always pass
TEST(BackendSelectionMatrix, Satisfies_ZeroMetricsRequired_AlwaysPass) {
    BackendCapabilities caps;
    caps.supportedMetrics = 0;
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = 0;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// Subset match: only L2 required when all three available → pass
TEST(BackendSelectionMatrix, Satisfies_L2Required_AllMetricsAvailable_Pass) {
    BackendCapabilities caps;
    caps.supportedMetrics = metricBit(DistanceMetric::L2)
                          | metricBit(DistanceMetric::COSINE)
                          | metricBit(DistanceMetric::INNER_PRODUCT);
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics  = metricBit(DistanceMetric::L2);
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

// =============================================================================
// Multi-requirement selection via registry
// =============================================================================

// vector + batch → CPU (CPUVectorBackend supports batch)
TEST(BackendSelectionMatrix, SelectVectorBackendFor_VectorAndBatch_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsBatch     = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

// geo + FP32 → CPU
TEST(BackendSelectionMatrix, SelectGeoBackendFor_GeoAndFP32_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps        = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* geo = BackendRegistry::instance().selectGeoBackendFor(reqs);
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}

// graph + batch → CPU
TEST(BackendSelectionMatrix, SelectGraphBackendFor_GraphAndBatch_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;
    reqs.needsBatch    = true;

    auto* gb = BackendRegistry::instance().selectGraphBackendFor(reqs);
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

// matrix + batch + FP32 → CPU
TEST(BackendSelectionMatrix, SelectMatrixBackendFor_MatrixBatchFP32_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps     = true;
    reqs.needsBatch         = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(mb->type(), BackendType::CPU);
}

// vector + async → nullptr (CPU doesn't support async)
TEST(BackendSelectionMatrix, SelectVectorBackendFor_VectorAndAsync_ReturnsNull) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.needsAsync     = true;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    EXPECT_EQ(vb, nullptr);
}

// vector + BF16 → nullptr (CPU only has FP32)
TEST(BackendSelectionMatrix, SelectVectorBackendFor_VectorAndBF16_ReturnsNull_WhenNoGPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps     = true;
    reqs.requiredPrecisions = PrecisionMode::BF16;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    EXPECT_EQ(vb, nullptr);
}

// vector + INT8 → nullptr
TEST(BackendSelectionMatrix, SelectVectorBackendFor_VectorAndINT8_ReturnsNull_WhenNoGPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps     = true;
    reqs.requiredPrecisions = PrecisionMode::INT8;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    EXPECT_EQ(vb, nullptr);
}

// vector + L2|COSINE → CPU (CPU vector supports all ANN metrics)
TEST(BackendSelectionMatrix, SelectVectorBackendFor_L2AndCosine_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps  = true;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2)
                         | metricBit(DistanceMetric::COSINE);

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

// vector + COSINE only → CPU
TEST(BackendSelectionMatrix, SelectVectorBackendFor_CosineOnly_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps  = true;
    reqs.requiredMetrics = metricBit(DistanceMetric::COSINE);

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

// vector + INNER_PRODUCT only → CPU
TEST(BackendSelectionMatrix, SelectVectorBackendFor_InnerProductOnly_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps  = true;
    reqs.requiredMetrics = metricBit(DistanceMetric::INNER_PRODUCT);

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

// =============================================================================
// getAvailableBackends()
// =============================================================================

TEST(BackendSelectionMatrix, GetAvailableBackends_ContainsCPU) {
    const auto types = BackendRegistry::instance().getAvailableBackends();
    const auto it    = std::find(types.begin(), types.end(), BackendType::CPU);
    EXPECT_NE(it, types.end()) << "CPU must appear in getAvailableBackends()";
}

TEST(BackendSelectionMatrix, GetAvailableBackends_NonEmpty) {
    const auto types = BackendRegistry::instance().getAvailableBackends();
    EXPECT_FALSE(types.empty());
}

TEST(BackendSelectionMatrix, GetAvailableBackends_CountMatchesRegisteredBackends) {
    // Every type returned by getAvailableBackends() must resolve via getBackend().
    const auto types = BackendRegistry::instance().getAvailableBackends();
    for (const auto bt : types) {
        EXPECT_NE(BackendRegistry::instance().getBackend(bt), nullptr)
            << "getBackend() must return non-null for type "
            << static_cast<int>(bt) << " listed in getAvailableBackends()";
    }
}

// =============================================================================
// BackendHealthStatus construction helpers
// =============================================================================

TEST(BackendSelectionMatrix, HealthStatus_MakeHealthy_Fields) {
    const auto h = BackendHealthStatus::makeHealthy("TestDevice");
    EXPECT_EQ(h.status,  "healthy");
    EXPECT_TRUE(h.healthy);
    EXPECT_TRUE(h.ready);
    EXPECT_TRUE(h.alive);
    EXPECT_EQ(h.deviceName, "TestDevice");
    EXPECT_FALSE(h.message.empty());
    EXPECT_TRUE(h.issues.empty());
}

TEST(BackendSelectionMatrix, HealthStatus_MakeDegraded_Fields) {
    const auto d = BackendHealthStatus::makeDegraded("memory pressure");
    EXPECT_EQ(d.status,  "degraded");
    EXPECT_FALSE(d.healthy);
    EXPECT_FALSE(d.ready);
    EXPECT_TRUE(d.alive);
    EXPECT_FALSE(d.message.empty());
    ASSERT_EQ(d.issues.size(), 1u);
    EXPECT_EQ(d.issues[0], "memory pressure");
}

TEST(BackendSelectionMatrix, HealthStatus_MakeUnhealthy_Fields) {
    const auto u = BackendHealthStatus::makeUnhealthy("driver crash");
    EXPECT_EQ(u.status,  "unhealthy");
    EXPECT_FALSE(u.healthy);
    EXPECT_FALSE(u.ready);
    EXPECT_FALSE(u.alive);
    EXPECT_FALSE(u.message.empty());
    ASSERT_EQ(u.issues.size(), 1u);
    EXPECT_EQ(u.issues[0], "driver crash");
}

TEST(BackendSelectionMatrix, HealthStatus_MakeHealthy_NoDevice_DeviceNameEmpty) {
    const auto h = BackendHealthStatus::makeHealthy();
    EXPECT_TRUE(h.healthy);
    EXPECT_TRUE(h.deviceName.empty());
}

// CPU backends report healthy status when available
TEST(BackendSelectionMatrix, CPUVectorBackend_HealthStatus_IsHealthy) {
    CPUVectorBackend backend;
    backend.initialize();
    const auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.healthy);
    EXPECT_TRUE(health.ready);
    EXPECT_TRUE(health.alive);
}

TEST(BackendSelectionMatrix, CPUGraphBackend_HealthStatus_IsHealthy) {
    CPUGraphBackend backend;
    backend.initialize();
    const auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.healthy);
}

TEST(BackendSelectionMatrix, CPUGeoBackend_HealthStatus_IsHealthy) {
    CPUGeoBackend backend;
    backend.initialize();
    const auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.healthy);
}

TEST(BackendSelectionMatrix, CPUMatrixBackend_HealthStatus_IsHealthy) {
    CPUMatrixBackend backend;
    backend.initialize();
    const auto health = backend.getHealthStatus();
    EXPECT_EQ(health.status, "healthy");
    EXPECT_TRUE(health.healthy);
}

// =============================================================================
// selectBackendFor() — generic (IComputeBackend) selector
// =============================================================================

TEST(BackendSelectionMatrix, SelectBackendFor_MatrixOps_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* b = BackendRegistry::instance().selectBackendFor(reqs);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->type(), BackendType::CPU);
}

TEST(BackendSelectionMatrix, SelectBackendFor_EmptyRequirements_ReturnsSomething) {
    // Empty requirements must not return nullptr (at least CPU is always available).
    BackendRegistry::CapabilityRequirements reqs;
    auto* b = BackendRegistry::instance().selectBackendFor(reqs);
    EXPECT_NE(b, nullptr);
}

TEST(BackendSelectionMatrix, SelectBackendFor_ImpossiblePrecisionAndAsync_ReturnsNull) {
    // No backend is both async and supports FP16 on CPU-only systems.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps     = true;
    reqs.needsAsync         = true;
    reqs.requiredPrecisions = PrecisionMode::FP16;

    auto* b = BackendRegistry::instance().selectBackendFor(reqs);
    EXPECT_EQ(b, nullptr);
}

// =============================================================================
// BackendRegistry O(1) type index (Issue #236 / v1.9.0)
//
// Validates that the BackendType→RegisteredBackend index correctly aggregates
// multiple backends that share the same BackendType (e.g., CPUVectorBackend,
// CPUGraphBackend, CPUGeoBackend, CPUMatrixBackend all have type CPU), so that
// typed selectors return the right specialised backend without dynamic_cast.
// =============================================================================

TEST(BackendSelectionMatrix, TypeIndex_GetBackend_ReturnsCPU) {
    // getBackend(CPU) must return a non-null IComputeBackend after the type
    // index was populated at construction time.
    auto* b = BackendRegistry::instance().getBackend(BackendType::CPU);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(b->type(), BackendType::CPU);
}

TEST(BackendSelectionMatrix, TypeIndex_SelectVectorBackendFor_ReturnsCPUVector) {
    // selectVectorBackendFor now uses the type index.  On a CPU-only system,
    // the result must be the CPUVectorBackend (implements IVectorBackend).
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* vb = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
    EXPECT_TRUE(vb->getCapabilities().supportsVectorOps);
}

TEST(BackendSelectionMatrix, TypeIndex_SelectGraphBackendFor_ReturnsCPUGraph) {
    // CPUGraphBackend and CPUVectorBackend share BackendType::CPU.
    // The type index must store graphPtr → CPUGraphBackend separately from
    // vectorPtr → CPUVectorBackend so typed selection returns the right one.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;

    auto* gb = BackendRegistry::instance().selectGraphBackendFor(reqs);
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
    EXPECT_TRUE(gb->getCapabilities().supportsGraphOps);
}

TEST(BackendSelectionMatrix, TypeIndex_SelectGeoBackendFor_ReturnsCPUGeo) {
    // CPUGeoBackend shares BackendType::CPU with vector and graph backends.
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* geo = BackendRegistry::instance().selectGeoBackendFor(reqs);
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
    EXPECT_TRUE(geo->getCapabilities().supportsGeoOps);
}

TEST(BackendSelectionMatrix, TypeIndex_SelectMatrixBackendFor_ReturnsCPUMatrix) {
    ensureCpuMatrixBackend();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsMatrixOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;

    auto* mb = BackendRegistry::instance().selectMatrixBackendFor(reqs);
    ASSERT_NE(mb, nullptr);
    EXPECT_EQ(mb->type(), BackendType::CPU);
    EXPECT_TRUE(mb->getCapabilities().supportsMatrixOps);
}

TEST(BackendSelectionMatrix, TypeIndex_TypedPointersAreDistinct) {
    // Verify that each typed selector returns a *different* interface pointer
    // even though all four CPU backends share BackendType::CPU.  This proves
    // that the type index stores separate typed pointers per interface.
    BackendRegistry::CapabilityRequirements vectorReqs; vectorReqs.needsVectorOps = true;
    BackendRegistry::CapabilityRequirements graphReqs;  graphReqs.needsGraphOps   = true;
    BackendRegistry::CapabilityRequirements geoReqs;    geoReqs.needsGeoOps       = true;

    auto* vb  = BackendRegistry::instance().selectVectorBackendFor(vectorReqs);
    auto* gb  = BackendRegistry::instance().selectGraphBackendFor(graphReqs);
    auto* geo = BackendRegistry::instance().selectGeoBackendFor(geoReqs);

    ASSERT_NE(vb, nullptr);
    ASSERT_NE(gb, nullptr);
    ASSERT_NE(geo, nullptr);

    // All are CPU type but different interface instances.
    EXPECT_EQ(vb->type(),  BackendType::CPU);
    EXPECT_EQ(gb->type(),  BackendType::CPU);
    EXPECT_EQ(geo->type(), BackendType::CPU);

    // Each implements only its declared interface (capabilities should differ).
    EXPECT_TRUE (vb->getCapabilities().supportsVectorOps);
    EXPECT_FALSE(vb->getCapabilities().supportsGraphOps);
    EXPECT_TRUE (gb->getCapabilities().supportsGraphOps);
    EXPECT_FALSE(gb->getCapabilities().supportsVectorOps);
    EXPECT_TRUE (geo->getCapabilities().supportsGeoOps);
    EXPECT_FALSE(geo->getCapabilities().supportsVectorOps);
}
