/*
 * test_acceleration_coverage.cpp
 *
 * Extends unit test coverage for the acceleration module to satisfy the
 * "> 80% coverage" production-readiness gate (ROADMAP.md Issue #1398).
 *
 * Covered in this file (not already covered by test_acceleration.cpp or
 * test_acceleration_dispatch.cpp):
 *
 *  1. error_codes.h  — errorCodeToString(), isSuccess(), category predicates
 *  2. error_context.h — ErrorContext constructors, format(), getCategory(),
 *                       isSuccess(), and ErrorContextHelpers factory functions
 *  3. compute_backend.h — BackendHealthStatus builder helpers,
 *                         IComputeBackend::getHealthStatus() default impl,
 *                         PrecisionMode bitmask operators and hasPrecision()
 *  4. BackendRegistry — getFallbackOrder(), getAvailableBackends(), satisfies(),
 *                       selectBackendFor(), selectVectorBackendFor(),
 *                       selectGraphBackendFor(), selectGeoBackendFor()
 *  5. CPUVectorBackend::computeDistances() with cosine metric
 *  6. CPUGraphBackend::batchBFS() and batchShortestPath() basic behaviour
 *  7. ANNKernelDispatch::distanceLauncherFor() for all DistanceMetric values
 */

#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <limits>

#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/kernel_invocation.h"
#include "acceleration/graphics_backends.h"

using namespace themis::acceleration;

// ============================================================================
// 1. error_codes.h — errorCodeToString() and category predicates
// ============================================================================

TEST(ErrorCodesTest, ErrorCodeToString_Success) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::Success), "Success");
}

TEST(ErrorCodesTest, ErrorCodeToString_InitializationCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::NoDevicesFound),    "NoDevicesFound");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::DriverNotInstalled),"DriverNotInstalled");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::DeviceNotSupported),"DeviceNotSupported");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::ContextCreationFailed),"ContextCreationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::QueueCreationFailed),  "QueueCreationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::RuntimeVersionIncompatible),"RuntimeVersionIncompatible");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::PlatformNotAvailable),"PlatformNotAvailable");
}

TEST(ErrorCodesTest, ErrorCodeToString_ResourceCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::OutOfDeviceMemory), "OutOfDeviceMemory");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::OutOfHostMemory),   "OutOfHostMemory");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::AllocationFailed),  "AllocationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::MemoryCopyFailed),  "MemoryCopyFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::BufferCreationFailed),"BufferCreationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidMemoryAccess),"InvalidMemoryAccess");
}

TEST(ErrorCodesTest, ErrorCodeToString_RuntimeCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelLaunchFailed),   "KernelLaunchFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelExecutionFailed),"KernelExecutionFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::SynchronizationFailed),"SynchronizationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::OperationTimeout),     "OperationTimeout");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::DeviceLost),           "DeviceLost");
}

TEST(ErrorCodesTest, ErrorCodeToString_KernelCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelCompilationFailed),"KernelCompilationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelNotFound),         "KernelNotFound");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidKernelArguments), "InvalidKernelArguments");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::ProgramLinkingFailed),   "ProgramLinkingFailed");
}

TEST(ErrorCodesTest, ErrorCodeToString_ValidationCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InputValidationFailed),"InputValidationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidInputShape),    "InvalidInputShape");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidInputDtype),    "InvalidInputDtype");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::BatchSizeExceeded),    "BatchSizeExceeded");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InputRangeViolation),  "InputRangeViolation");
}

TEST(ErrorCodesTest, ErrorCodeToString_ConfigurationCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidConfiguration),      "InvalidConfiguration");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::FeatureNotSupported),       "FeatureNotSupported");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidParameter),          "InvalidParameter");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::BackendNotInitialized),     "BackendNotInitialized");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::BackendAlreadyInitialized), "BackendAlreadyInitialized");
}

TEST(ErrorCodesTest, ErrorCodeToString_GenericCodes) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::UnknownError),   "UnknownError");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InternalError),  "InternalError");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::NotImplemented), "NotImplemented");
}

TEST(ErrorCodesTest, IsSuccess) {
    EXPECT_TRUE(isSuccess(AccelerationErrorCode::Success));
    EXPECT_FALSE(isSuccess(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isSuccess(AccelerationErrorCode::UnknownError));
}

TEST(ErrorCodesTest, CategoryPredicates_InitializationErrors) {
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::DriverNotInstalled));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::PlatformNotAvailable));

    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::KernelLaunchFailed));
}

TEST(ErrorCodesTest, CategoryPredicates_ResourceErrors) {
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::AllocationFailed));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::InvalidMemoryAccess));

    EXPECT_FALSE(isResourceError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::KernelLaunchFailed));
}

TEST(ErrorCodesTest, CategoryPredicates_RuntimeErrors) {
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::KernelLaunchFailed));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::SynchronizationFailed));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::DeviceLost));

    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::InvalidConfiguration));
}

TEST(ErrorCodesTest, CategoryPredicates_ConfigurationErrors) {
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::InvalidConfiguration));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::BackendNotInitialized));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::FeatureNotSupported));

    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::KernelLaunchFailed));
}

TEST(ErrorCodesTest, CategoryPredicates_KernelErrors) {
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::KernelCompilationFailed));
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::KernelNotFound));
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::ProgramLinkingFailed));

    EXPECT_FALSE(isKernelError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isKernelError(AccelerationErrorCode::KernelLaunchFailed));   // runtime, not kernel
}

TEST(ErrorCodesTest, CategoryPredicates_ValidationErrors) {
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InputValidationFailed));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InvalidInputShape));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InputRangeViolation));

    EXPECT_FALSE(isValidationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isValidationError(AccelerationErrorCode::KernelCompilationFailed));
}

// ============================================================================
// 2. error_context.h — ErrorContext and ErrorContextHelpers
// ============================================================================

TEST(ErrorContextTest, DefaultConstructor_IsUnknownError) {
    ErrorContext ctx;
    EXPECT_EQ(ctx.code, AccelerationErrorCode::UnknownError);
    EXPECT_FALSE(ctx.isSuccess());
}

TEST(ErrorContextTest, BasicConstructor_FieldsRoundtrip) {
    ErrorContext ctx(AccelerationErrorCode::Success, "CPU", "All good");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::Success);
    EXPECT_EQ(ctx.backendName, "CPU");
    EXPECT_EQ(ctx.message, "All good");
    EXPECT_TRUE(ctx.isSuccess());
}

TEST(ErrorContextTest, ConstructorWithHint_FieldsRoundtrip) {
    ErrorContext ctx(AccelerationErrorCode::NoDevicesFound, "CUDA",
                     "No GPU found", "Install GPU driver");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::NoDevicesFound);
    EXPECT_EQ(ctx.troubleshootingHint, "Install GPU driver");
    EXPECT_FALSE(ctx.isSuccess());
}

TEST(ErrorContextTest, FullConstructor_SystemInfoPresent) {
    ErrorContext ctx(AccelerationErrorCode::OutOfDeviceMemory, "CUDA",
                     "OOM", "Reduce batch size", "Driver 535.0");
    ASSERT_TRUE(ctx.systemInfo.has_value());
    EXPECT_EQ(*ctx.systemInfo, "Driver 535.0");
}

TEST(ErrorContextTest, Format_ContainsBackendNameAndCode) {
    ErrorContext ctx(AccelerationErrorCode::NoDevicesFound, "CUDA", "No GPU");
    std::string s = ctx.format();
    EXPECT_NE(s.find("CUDA"), std::string::npos);
    EXPECT_NE(s.find("NoDevicesFound"), std::string::npos);
}

TEST(ErrorContextTest, Format_ContainsMessageAndHint) {
    ErrorContext ctx(AccelerationErrorCode::OutOfDeviceMemory, "HIP",
                     "Out of memory", "Reduce data size");
    std::string s = ctx.format();
    EXPECT_NE(s.find("Out of memory"), std::string::npos);
    EXPECT_NE(s.find("Reduce data size"), std::string::npos);
}

TEST(ErrorContextTest, Format_ContainsSystemInfoWhenPresent) {
    ErrorContext ctx(AccelerationErrorCode::DriverNotInstalled, "Vulkan",
                     "Driver missing", "Install Vulkan", "Host: Linux 6.1");
    std::string s = ctx.format();
    EXPECT_NE(s.find("Host: Linux 6.1"), std::string::npos);
}

TEST(ErrorContextTest, GetCategory_Success) {
    ErrorContext ctx(AccelerationErrorCode::Success, "CPU", "");
    EXPECT_EQ(ctx.getCategory(), "Success");
}

TEST(ErrorContextTest, GetCategory_InitializationError) {
    ErrorContext ctx(AccelerationErrorCode::NoDevicesFound, "CUDA", "");
    EXPECT_EQ(ctx.getCategory(), "Initialization");
}

TEST(ErrorContextTest, GetCategory_ResourceError) {
    ErrorContext ctx(AccelerationErrorCode::OutOfDeviceMemory, "CUDA", "");
    EXPECT_EQ(ctx.getCategory(), "Resource");
}

TEST(ErrorContextTest, GetCategory_RuntimeError) {
    ErrorContext ctx(AccelerationErrorCode::KernelLaunchFailed, "CUDA", "");
    EXPECT_EQ(ctx.getCategory(), "Runtime");
}

TEST(ErrorContextTest, GetCategory_ConfigurationError) {
    ErrorContext ctx(AccelerationErrorCode::InvalidConfiguration, "CPU", "");
    EXPECT_EQ(ctx.getCategory(), "Configuration");
}

TEST(ErrorContextTest, GetCategory_KernelError) {
    ErrorContext ctx(AccelerationErrorCode::KernelCompilationFailed, "OpenCL", "");
    EXPECT_EQ(ctx.getCategory(), "Kernel");
}

TEST(ErrorContextTest, GetCategory_ValidationError) {
    ErrorContext ctx(AccelerationErrorCode::InputRangeViolation, "CPU", "");
    EXPECT_EQ(ctx.getCategory(), "Validation");
}

TEST(ErrorContextTest, GetCategory_UnknownError) {
    ErrorContext ctx(AccelerationErrorCode::UnknownError, "CPU", "");
    EXPECT_EQ(ctx.getCategory(), "Unknown");
}

// ErrorContextHelpers

TEST(ErrorContextHelpersTest, CreateNoDevicesError) {
    auto ctx = ErrorContextHelpers::createNoDevicesError("CUDA");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::NoDevicesFound);
    EXPECT_EQ(ctx.backendName, "CUDA");
    EXPECT_FALSE(ctx.message.empty());
    EXPECT_FALSE(ctx.troubleshootingHint.empty());
}

TEST(ErrorContextHelpersTest, CreateDriverError) {
    auto ctx = ErrorContextHelpers::createDriverError("HIP");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::DriverNotInstalled);
    EXPECT_EQ(ctx.backendName, "HIP");
}

TEST(ErrorContextHelpersTest, CreateContextError) {
    auto ctx = ErrorContextHelpers::createContextError("Vulkan", "device lost");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::ContextCreationFailed);
    EXPECT_NE(ctx.message.find("device lost"), std::string::npos);
}

TEST(ErrorContextHelpersTest, CreateQueueError) {
    auto ctx = ErrorContextHelpers::createQueueError("OpenCL", "no resources");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::QueueCreationFailed);
    EXPECT_NE(ctx.message.find("no resources"), std::string::npos);
}

TEST(ErrorContextHelpersTest, CreateMemoryError_MessageContainsMB) {
    auto ctx = ErrorContextHelpers::createMemoryError("CUDA", 256 * 1024 * 1024ULL);
    EXPECT_EQ(ctx.code, AccelerationErrorCode::OutOfDeviceMemory);
    EXPECT_NE(ctx.message.find("256"), std::string::npos);
}

TEST(ErrorContextHelpersTest, CreateKernelCompilationError) {
    auto ctx = ErrorContextHelpers::createKernelCompilationError("CUDA", "ann_l2", "syntax error");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::KernelCompilationFailed);
    EXPECT_NE(ctx.message.find("ann_l2"), std::string::npos);
    EXPECT_NE(ctx.message.find("syntax error"), std::string::npos);
}

TEST(ErrorContextHelpersTest, CreateKernelLaunchError) {
    auto ctx = ErrorContextHelpers::createKernelLaunchError("CUDA", "topK", "invalid grid");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::KernelLaunchFailed);
    EXPECT_NE(ctx.message.find("topK"), std::string::npos);
}

TEST(ErrorContextHelpersTest, CreateValidationError) {
    auto ctx = ErrorContextHelpers::createValidationError(
        "CPU", AccelerationErrorCode::InputRangeViolation, "NaN in query vector");
    EXPECT_EQ(ctx.code, AccelerationErrorCode::InputRangeViolation);
    EXPECT_NE(ctx.message.find("NaN in query vector"), std::string::npos);
}

// ============================================================================
// 3. compute_backend.h — PrecisionMode, BackendHealthStatus, getHealthStatus()
// ============================================================================

TEST(PrecisionModeTest, BitwiseOrAndHasPrecision) {
    PrecisionMode modes = PrecisionMode::FP32 | PrecisionMode::FP16;
    EXPECT_TRUE(hasPrecision(modes, PrecisionMode::FP32));
    EXPECT_TRUE(hasPrecision(modes, PrecisionMode::FP16));
    EXPECT_FALSE(hasPrecision(modes, PrecisionMode::BF16));
    EXPECT_FALSE(hasPrecision(modes, PrecisionMode::INT8));
}

TEST(PrecisionModeTest, NoneHasNoPrecision) {
    EXPECT_FALSE(hasPrecision(PrecisionMode::NONE, PrecisionMode::FP32));
    EXPECT_FALSE(hasPrecision(PrecisionMode::NONE, PrecisionMode::FP16));
}

TEST(PrecisionModeTest, AllCombination) {
    PrecisionMode all = PrecisionMode::FP32 | PrecisionMode::FP16
                      | PrecisionMode::BF16 | PrecisionMode::INT8;
    EXPECT_TRUE(hasPrecision(all, PrecisionMode::FP32));
    EXPECT_TRUE(hasPrecision(all, PrecisionMode::FP16));
    EXPECT_TRUE(hasPrecision(all, PrecisionMode::BF16));
    EXPECT_TRUE(hasPrecision(all, PrecisionMode::INT8));
}

TEST(BackendHealthStatusTest, MakeHealthy) {
    auto h = BackendHealthStatus::makeHealthy("RTX 4090");
    EXPECT_EQ(h.status, "healthy");
    EXPECT_TRUE(h.healthy);
    EXPECT_TRUE(h.ready);
    EXPECT_TRUE(h.alive);
    EXPECT_EQ(h.deviceName, "RTX 4090");
    EXPECT_TRUE(h.issues.empty());
}

TEST(BackendHealthStatusTest, MakeDegraded) {
    auto h = BackendHealthStatus::makeDegraded("ECC errors detected");
    EXPECT_EQ(h.status, "degraded");
    EXPECT_FALSE(h.healthy);
    EXPECT_FALSE(h.ready);
    EXPECT_TRUE(h.alive);
    ASSERT_EQ(h.issues.size(), 1u);
    EXPECT_EQ(h.issues[0], "ECC errors detected");
}

TEST(BackendHealthStatusTest, MakeUnhealthy) {
    auto h = BackendHealthStatus::makeUnhealthy("Device lost");
    EXPECT_EQ(h.status, "unhealthy");
    EXPECT_FALSE(h.healthy);
    EXPECT_FALSE(h.ready);
    EXPECT_FALSE(h.alive);
    ASSERT_EQ(h.issues.size(), 1u);
}

TEST(BackendHealthStatusTest, CPUVectorBackend_GetHealthStatus_WhenInitialized) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto h = backend.getHealthStatus();
    EXPECT_EQ(h.status, "healthy");
    EXPECT_TRUE(h.healthy);
}

TEST(BackendHealthStatusTest, CPUGeoBackend_GetHealthStatus_WhenInitialized) {
    CPUGeoBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto h = backend.getHealthStatus();
    EXPECT_TRUE(h.healthy);
}

// ============================================================================
// 4. BackendRegistry — getFallbackOrder, getAvailableBackends, satisfies,
//    selectBackendFor / selectVectorBackendFor / selectGraphBackendFor /
//    selectGeoBackendFor
// ============================================================================

class RegistryTest : public ::testing::Test {
protected:
    // Note: No TearDown() - shutdownAll() is called only once at process exit.
    // Multiple TearDown() calls pollute the singleton for later tests.
};

TEST_F(RegistryTest, GetFallbackOrder_CPUIsLast) {
    const auto& order = BackendRegistry::getFallbackOrder();
    ASSERT_FALSE(order.empty());
    EXPECT_EQ(order.back(), BackendType::CPU);
}

TEST_F(RegistryTest, GetFallbackOrder_CUDAIsFirst) {
    const auto& order = BackendRegistry::getFallbackOrder();
    ASSERT_FALSE(order.empty());

    auto available = BackendRegistry::instance().getAvailableBackends();
    const bool hasCUDA = std::find(available.begin(), available.end(), BackendType::CUDA) != available.end();

    if (hasCUDA) {
        EXPECT_EQ(order.front(), BackendType::CUDA);
    } else {
        EXPECT_NE(order.front(), BackendType::CUDA);
    }
}

TEST_F(RegistryTest, GetAvailableBackends_ContainsCPU) {
    auto types = BackendRegistry::instance().getAvailableBackends();
    bool hasCPU = std::find(types.begin(), types.end(), BackendType::CPU) != types.end();

    const auto& fallback = BackendRegistry::getFallbackOrder();
    bool cpuInFallback = std::find(fallback.begin(), fallback.end(), BackendType::CPU) != fallback.end();

    EXPECT_TRUE(hasCPU || cpuInFallback);
}

TEST_F(RegistryTest, Satisfies_EmptyRequirements_AlwaysTrue) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs; // all defaults = false / NONE / 0
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_VectorOpsRequired_CPUVectorMeets) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_GraphOpsRequired_CPUVectorFails) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_GeoOpsRequired_CPUGeoMeets) {
    CPUGeoBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_PrecisionRequired_FP32_CPUMeets) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions = PrecisionMode::FP32;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_PrecisionRequired_FP16_CPUFails) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredPrecisions = PrecisionMode::FP16;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_MetricRequired_L2_CPUVectorMeets) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2);
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_BatchRequired_CPUVectorMeets) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsBatch = true;
    EXPECT_TRUE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, Satisfies_AsyncRequired_CPUFails) {
    CPUVectorBackend backend;
    auto caps = backend.getCapabilities();
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsAsync = true;
    EXPECT_FALSE(BackendRegistry::satisfies(caps, reqs));
}

TEST_F(RegistryTest, SelectVectorBackendFor_VectorOpsReq_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;
    auto* backend = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->getCapabilities().supportsVectorOps);
}

TEST_F(RegistryTest, SelectGraphBackendFor_GraphOpsReq_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGraphOps = true;
    auto* backend = BackendRegistry::instance().selectGraphBackendFor(reqs);
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->getCapabilities().supportsGraphOps);
}

TEST_F(RegistryTest, SelectGeoBackendFor_GeoOpsReq_ReturnsCPU) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsGeoOps = true;
    auto* backend = BackendRegistry::instance().selectGeoBackendFor(reqs);
    ASSERT_NE(backend, nullptr);
    EXPECT_TRUE(backend->getCapabilities().supportsGeoOps);
}

TEST_F(RegistryTest, SelectBackendFor_ImpossibleReqs_ReturnsNull) {
    // Async is not supported by any available backend in a CPU-only build
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsAsync = true;
    reqs.requiredPrecisions = PrecisionMode::FP16; // CPU also lacks FP16
    auto* backend = BackendRegistry::instance().selectBackendFor(reqs);
    EXPECT_EQ(backend, nullptr);
}

TEST_F(RegistryTest, SelectVectorBackendFor_AllMetricsReq_CPUMeets) {
    BackendRegistry::CapabilityRequirements reqs;
    reqs.needsVectorOps  = true;
    reqs.requiredMetrics = metricBit(DistanceMetric::L2)
                         | metricBit(DistanceMetric::COSINE)
                         | metricBit(DistanceMetric::INNER_PRODUCT);
    auto* backend = BackendRegistry::instance().selectVectorBackendFor(reqs);
    ASSERT_NE(backend, nullptr);
}

// ============================================================================
// 5. CPUVectorBackend::computeDistances() with cosine metric
// ============================================================================

class CpuVectorCosineTest : public ::testing::Test {
protected:
    CPUVectorBackend backend_;

    void SetUp() override {
        ASSERT_TRUE(backend_.initialize());
    }
};

TEST_F(CpuVectorCosineTest, CosineDistance_SameDirection_IsZero) {
    // Query and single identical vector → cosine distance = 0
    const std::vector<float> v = {1.0f, 0.0f};
    auto dists = backend_.computeDistances(v.data(), 1, 2, v.data(), 1, /*useL2=*/false);
    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 0.0f, 1e-6f);
}

TEST_F(CpuVectorCosineTest, CosineDistance_OrthogonalVectors_IsOne) {
    const std::vector<float> q  = {1.0f, 0.0f};
    const std::vector<float> db = {0.0f, 1.0f};
    auto dists = backend_.computeDistances(q.data(), 1, 2, db.data(), 1, /*useL2=*/false);
    ASSERT_EQ(dists.size(), 1u);
    EXPECT_NEAR(dists[0], 1.0f, 1e-6f);
}

TEST_F(CpuVectorCosineTest, CosineDistance_ZeroVector_IsOne) {
    // Zero DB vector — cosine undefined, should return max distance (1.0)
    const std::vector<float> q  = {1.0f, 1.0f};
    const std::vector<float> db = {0.0f, 0.0f};
    auto dists = backend_.computeDistances(q.data(), 1, 2, db.data(), 1, /*useL2=*/false);
    ASSERT_EQ(dists.size(), 1u);
    EXPECT_FLOAT_EQ(dists[0], 1.0f);
}

TEST_F(CpuVectorCosineTest, CosineDistance_BatchMultipleVectors) {
    // Two DB vectors: one aligned with query, one orthogonal
    const std::vector<float> q  = {1.0f, 0.0f};
    const std::vector<float> db = {1.0f, 0.0f,   // dist ≈ 0
                                   0.0f, 1.0f};  // dist ≈ 1
    auto dists = backend_.computeDistances(q.data(), 1, 2, db.data(), 2, /*useL2=*/false);
    ASSERT_EQ(dists.size(), 2u);
    EXPECT_NEAR(dists[0], 0.0f, 1e-6f);
    EXPECT_NEAR(dists[1], 1.0f, 1e-6f);
}

TEST_F(CpuVectorCosineTest, KnnSearch_WithCosine_ReturnsSortedResults) {
    // query=(1,0): cosine to (1,0)=0, to (1,1)≈0.29, to (0,1)=1
    const std::vector<float> vectors = {
        1.0f, 0.0f,  // index 0
        0.0f, 1.0f,  // index 1
        1.0f, 1.0f,  // index 2
    };
    const std::vector<float> query = {1.0f, 0.0f};

    auto results = backend_.batchKnnSearch(
        query.data(), 1, 2,
        vectors.data(), 3,
        3, /*useL2=*/false);

    ASSERT_EQ(results.size(), 1u);
    ASSERT_EQ(results[0].size(), 3u);
    // Sorted ascending by cosine distance
    for (size_t i = 1; i < results[0].size(); ++i) {
        EXPECT_LE(results[0][i-1].second, results[0][i].second);
    }
    // Closest must be index 0 (cosine dist = 0)
    EXPECT_EQ(results[0][0].first, 0u);
    EXPECT_NEAR(results[0][0].second, 0.0f, 1e-6f);
}

// ============================================================================
// 6. CPUGraphBackend — batchBFS and batchShortestPath basic behaviour
// ============================================================================

TEST(CpuGraphBackendTest, BFS_ResultCountMatchesNumStarts) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());

    const uint32_t adj[] = {0}; // Placeholder — not used in current impl
    const uint32_t starts[] = {0, 1, 2};

    auto results = backend.batchBFS(adj, 5, starts, 3, /*maxDepth=*/2);
    EXPECT_EQ(results.size(), 3u);

    backend.shutdown();
}

TEST(CpuGraphBackendTest, BFS_StartVertexPresentInResult) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());

    const uint32_t adj[] = {0};
    const uint32_t starts[] = {3};

    auto results = backend.batchBFS(adj, 10, starts, 1, /*maxDepth=*/1);
    ASSERT_EQ(results.size(), 1u);
    // Start vertex must always appear in its own BFS result
    ASSERT_FALSE(results[0].empty());
    EXPECT_EQ(results[0][0], 3u);

    backend.shutdown();
}

TEST(CpuGraphBackendTest, ShortestPath_ResultCountMatchesNumPairs) {
    CPUGraphBackend backend;
    ASSERT_TRUE(backend.initialize());

    const uint32_t adj[] = {0};
    const float weights[] = {1.0f};
    const uint32_t starts[] = {0, 1};
    const uint32_t ends[]   = {2, 3};

    auto results = backend.batchShortestPath(adj, weights, 5, starts, ends, 2);
    EXPECT_EQ(results.size(), 2u);

    backend.shutdown();
}

TEST(CpuGraphBackendTest, GraphBackend_Capabilities) {
    CPUGraphBackend backend;
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsGraphOps);
    EXPECT_FALSE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsGeoOps);
    EXPECT_FALSE(caps.deviceName.empty());
}

// ============================================================================
// 7. ANNKernelDispatch::distanceLauncherFor() for all DistanceMetric values
// ============================================================================

TEST(ANNKernelDispatchTest, DistanceLauncherFor_AllMetrics_NotNull) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto dispatch = backend.populateANNDispatch();

    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::L2),            nullptr);
    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::COSINE),        nullptr);
    EXPECT_NE(dispatch.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);
}

TEST(ANNKernelDispatchTest, DistanceLauncherFor_L2_IsLaunchL2Distance) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto dispatch = backend.populateANNDispatch();

    EXPECT_EQ(dispatch.distanceLauncherFor(DistanceMetric::L2),
              dispatch.launchL2Distance);
}

TEST(ANNKernelDispatchTest, DistanceLauncherFor_Cosine_IsLaunchCosine) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto dispatch = backend.populateANNDispatch();

    EXPECT_EQ(dispatch.distanceLauncherFor(DistanceMetric::COSINE),
              dispatch.launchCosine);
}

TEST(ANNKernelDispatchTest, DistanceLauncherFor_InnerProduct_IsLaunchInnerProduct) {
    CPUVectorBackend backend;
    ASSERT_TRUE(backend.initialize());
    auto dispatch = backend.populateANNDispatch();

    EXPECT_EQ(dispatch.distanceLauncherFor(DistanceMetric::INNER_PRODUCT),
              dispatch.launchInnerProduct);
}

TEST(ANNKernelDispatchTest, EmptyDispatch_DistanceLauncherFor_ReturnsNull) {
    ANNKernelDispatch empty; // all null by default
    EXPECT_EQ(empty.distanceLauncherFor(DistanceMetric::L2),            nullptr);
    EXPECT_EQ(empty.distanceLauncherFor(DistanceMetric::COSINE),        nullptr);
    EXPECT_EQ(empty.distanceLauncherFor(DistanceMetric::INNER_PRODUCT), nullptr);
}

// ============================================================================
// 8. DirectXVectorBackend — stub behaviour on non-Windows / non-DIRECTX builds
// ============================================================================

// These tests cover the non-Windows code path where DirectX is unavailable.
// On Windows with THEMIS_ENABLE_DIRECTX, the tests still compile but the
// assertions are skipped under the THEMIS_ENABLE_DIRECTX guard so as not to
// require real GPU hardware in CI.

TEST(DirectXBackendTest, NameAndType) {
    DirectXVectorBackend backend;
    EXPECT_STREQ(backend.name(), "DirectX");
    EXPECT_EQ(backend.type(), BackendType::DIRECTX);
}

TEST(DirectXBackendTest, StubNotAvailableOnNonWindows) {
#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)
    DirectXVectorBackend backend;
    EXPECT_FALSE(backend.isAvailable());
#else
    GTEST_SKIP() << "Skipping on Windows with DirectX; hardware required";
#endif
}

TEST(DirectXBackendTest, StubInitializeFalseOnNonWindows) {
#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)
    DirectXVectorBackend backend;
    EXPECT_FALSE(backend.initialize());
#else
    GTEST_SKIP() << "Skipping on Windows with DirectX; hardware required";
#endif
}

TEST(DirectXBackendTest, StubComputeDistancesEmptyOnNonWindows) {
#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)
    DirectXVectorBackend backend;
    std::vector<float> queries = { 1.0f, 0.0f };
    std::vector<float> vectors = { 0.0f, 1.0f, 1.0f, 1.0f };
    auto result = backend.computeDistances(
        queries.data(), 1, 2,
        vectors.data(), 2,
        true);
    EXPECT_TRUE(result.empty());
#else
    GTEST_SKIP() << "Skipping on Windows with DirectX; hardware required";
#endif
}

TEST(DirectXBackendTest, StubBatchKnnSearchEmptyOnNonWindows) {
#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)
    DirectXVectorBackend backend;
    std::vector<float> queries = { 1.0f, 0.0f };
    std::vector<float> vectors = { 0.0f, 1.0f, 1.0f, 1.0f };
    auto result = backend.batchKnnSearch(
        queries.data(), 1, 2,
        vectors.data(), 2,
        1, true);
    EXPECT_TRUE(result.empty());
#else
    GTEST_SKIP() << "Skipping on Windows with DirectX; hardware required";
#endif
}

TEST(DirectXBackendTest, ShutdownIdempotentOnNonWindows) {
    // Should not crash when called multiple times before/after initialize
    DirectXVectorBackend backend;
    backend.shutdown();
    backend.shutdown();
}

TEST(DirectXBackendTest, GetCapabilitiesReturnsBackendType) {
#if !defined(_WIN32) || !defined(THEMIS_ENABLE_DIRECTX)
    DirectXVectorBackend backend;
    // On non-Windows, getCapabilities returns a default-constructed struct.
    auto caps = backend.getCapabilities();
    // Stub returns all-false capabilities
    EXPECT_FALSE(caps.supportsVectorOps);
    EXPECT_FALSE(caps.supportsBatchProcessing);
#else
    DirectXVectorBackend backend;
    auto caps = backend.getCapabilities();
    EXPECT_TRUE(caps.supportsVectorOps);
    EXPECT_TRUE(caps.supportsBatchProcessing);
    EXPECT_FALSE(caps.deviceName.empty());
#endif
}
