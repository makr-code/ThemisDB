// Test: Acceleration Regression Tests — Invalid Input & Runtime Fallbacks
//
// Regression suite that locks down:
//  1. Specific error codes (not just "not success") on invalid input for all
//     three CPU backend types (vector, graph, geo).
//  2. Error context fields (backendName, message) populated on validation failure.
//  3. getLastError() cleared after a subsequent valid operation.
//  4. Runtime fallback: BackendRegistry selects CPU when GPU requirements cannot
//     be satisfied (no GPU hardware in CI).
//  5. batchKnnSearchSafe batch-level validation (null pointers, zero counts).
//  6. CPU dispatch table integration as live fallback for ANNKernelFallbackDispatcher.
//
// All tests run on any platform — no GPU is required.

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/batch_validator.h"
#include "acceleration/kernel_fallback_dispatcher.h"

#include <cmath>
#include <limits>
#include <vector>

using namespace themis::acceleration;

// =============================================================================
// 1. Invalid input → specific error code (CPUVectorBackend)
// =============================================================================

class CPUVectorInvalidInputTest : public ::testing::Test {
protected:
    CPUVectorBackend backend;
    void SetUp() override { ASSERT_TRUE(backend.initialize()); }
    void TearDown() override { backend.shutdown(); }
};

TEST_F(CPUVectorInvalidInputTest, ComputeDistances_NullQuery_ErrorCode_InvalidInputShape) {
    const float v[] = {1.f, 0.f};
    backend.computeDistances(nullptr, 1, 2, v, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, ComputeDistances_NullVectors_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f, 0.f};
    backend.computeDistances(q, 1, 2, nullptr, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, ComputeDistances_ZeroDim_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f};
    const float v[] = {1.f};
    backend.computeDistances(q, 1, 0, v, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, ComputeDistances_ZeroNumQueries_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    backend.computeDistances(q, 0, 2, v, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, BatchKnnSearch_NullQuery_ErrorCode_InvalidInputShape) {
    const float v[] = {1.f, 0.f};
    backend.batchKnnSearch(nullptr, 1, 2, v, 1, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, BatchKnnSearch_NullVectors_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f, 0.f};
    backend.batchKnnSearch(q, 1, 2, nullptr, 1, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, BatchKnnSearch_ZeroK_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    backend.batchKnnSearch(q, 1, 2, v, 1, 0, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUVectorInvalidInputTest, BatchKnnSearch_ZeroDim_ErrorCode_InvalidInputShape) {
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    backend.batchKnnSearch(q, 1, 0, v, 1, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

// After an invalid call, a subsequent valid call must clear the error.
TEST_F(CPUVectorInvalidInputTest, ErrorClearedAfterSuccessfulCall) {
    // First: trigger an error
    backend.batchKnnSearch(nullptr, 1, 2, nullptr, 1, 1, true);
    ASSERT_NE(backend.getLastError().code, AccelerationErrorCode::Success);

    // Then: valid call
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f, 0.f, 1.f};
    auto results = backend.batchKnnSearch(q, 1, 2, v, 2, 1, true);
    EXPECT_FALSE(results.empty());
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::Success);
}

// k > numVectors must succeed with clamped result size (no error).
TEST_F(CPUVectorInvalidInputTest, BatchKnnSearch_KLargerThanVectors_NoError) {
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f,  0.f, 1.f};
    auto results = backend.batchKnnSearch(q, 1, 2, v, 2, 100, true);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_LE(results[0].size(), 2u);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::Success);
}

// =============================================================================
// 2. Invalid input → error context (backendName and message populated)
// =============================================================================

TEST_F(CPUVectorInvalidInputTest, InvalidInput_ErrorContext_BackendNameSet) {
    const float v[] = {1.f, 0.f};
    backend.computeDistances(nullptr, 1, 2, v, 1, true);
    const auto& ctx = backend.getLastError();
    EXPECT_FALSE(ctx.backendName.empty());
}

TEST_F(CPUVectorInvalidInputTest, InvalidInput_ErrorContext_MessageSet) {
    const float v[] = {1.f, 0.f};
    backend.computeDistances(nullptr, 1, 2, v, 1, true);
    const auto& ctx = backend.getLastError();
    EXPECT_FALSE(ctx.message.empty());
}

// =============================================================================
// 3. Invalid input → specific error code (CPUGeoBackend)
// =============================================================================

class CPUGeoInvalidInputTest : public ::testing::Test {
protected:
    CPUGeoBackend backend;
    void SetUp() override { ASSERT_TRUE(backend.initialize()); }
    void TearDown() override { backend.shutdown(); }
};

TEST_F(CPUGeoInvalidInputTest, BatchDistances_NullLats_ErrorCode_InvalidInputShape) {
    const double lons1[] = {0.0}, lats2[] = {10.0}, lons2[] = {10.0};
    backend.batchDistances(nullptr, lons1, lats2, lons2, 1, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGeoInvalidInputTest, BatchDistances_ZeroCount_ErrorCode_InvalidInputShape) {
    const double lats1[] = {0.0}, lons1[] = {0.0}, lats2[] = {10.0}, lons2[] = {10.0};
    backend.batchDistances(lats1, lons1, lats2, lons2, 0, true);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGeoInvalidInputTest, BatchPointInPolygon_NullPoints_ErrorCode_InvalidInputShape) {
    const double poly[] = {0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0};
    const double lons[] = {0.5};
    backend.batchPointInPolygon(nullptr, lons, 1, poly, 4);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGeoInvalidInputTest, BatchPointInPolygon_TooFewVertices_ErrorCode_InvalidInputShape) {
    const double lats[] = {0.5}, lons[] = {0.5};
    const double poly[] = {0.0,0.0, 1.0,0.0};  // only 2 vertices — invalid polygon
    backend.batchPointInPolygon(lats, lons, 1, poly, 2);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGeoInvalidInputTest, ErrorClearedAfterSuccessfulCall) {
    // Trigger an error
    backend.batchDistances(nullptr, nullptr, nullptr, nullptr, 1, true);
    ASSERT_NE(backend.getLastError().code, AccelerationErrorCode::Success);

    // Valid haversine call — Paris to London ≈ 340 km
    const double lat1[] = {48.8566}, lon1[] = {2.3522};
    const double lat2[] = {51.5074}, lon2[] = {-0.1278};
    auto dists = backend.batchDistances(lat1, lon1, lat2, lon2, 1, true);
    EXPECT_FALSE(dists.empty());
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::Success);
}

// =============================================================================
// 4. Invalid input → specific error code (CPUGraphBackend)
// =============================================================================

class CPUGraphInvalidInputTest : public ::testing::Test {
protected:
    CPUGraphBackend backend;
    void SetUp() override { ASSERT_TRUE(backend.initialize()); }
    void TearDown() override { backend.shutdown(); }
};

TEST_F(CPUGraphInvalidInputTest, BatchBFS_NullAdjacency_ErrorCode_InvalidInputShape) {
    const uint32_t starts[] = {0u};
    backend.batchBFS(nullptr, 4, starts, 1, 2);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGraphInvalidInputTest, BatchBFS_ZeroNumStarts_ErrorCode_InvalidInputShape) {
    const uint32_t adj[] = {0u};
    const uint32_t starts[] = {0u};
    backend.batchBFS(adj, 4, starts, 0, 2);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGraphInvalidInputTest, BatchShortestPath_NullAdjacency_ErrorCode_InvalidInputShape) {
    const float weights[] = {1.f};
    const uint32_t s[] = {0u}, e[] = {1u};
    backend.batchShortestPath(nullptr, weights, 4, s, e, 1);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGraphInvalidInputTest, BatchShortestPath_ZeroNumPairs_ErrorCode_InvalidInputShape) {
    const uint32_t adj[] = {0u};
    const float weights[] = {1.f};
    const uint32_t s[] = {0u}, e[] = {1u};
    backend.batchShortestPath(adj, weights, 4, s, e, 0);
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::InvalidInputShape);
}

TEST_F(CPUGraphInvalidInputTest, ErrorClearedAfterSuccessfulCall) {
    // Trigger an error
    backend.batchBFS(nullptr, 4, nullptr, 1, 2);
    ASSERT_NE(backend.getLastError().code, AccelerationErrorCode::Success);

    // Valid BFS on a 3-node chain: 0→1→2
    const uint32_t adj[] = {
        1, 0xFFFFFFFF,         // vertex 0 → [1]
        2, 0xFFFFFFFF,         // vertex 1 → [2]
        0xFFFFFFFF             // vertex 2 → []
    };
    const uint32_t starts[] = {0u};
    auto result = backend.batchBFS(adj, 3, starts, 1, 5);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(backend.getLastError().code, AccelerationErrorCode::Success);
}

// =============================================================================
// 5. batchKnnSearchSafe — batch-level validation regressions
// =============================================================================

class BatchKnnSearchSafeTest : public ::testing::Test {
protected:
    CPUVectorBackend backend;
    void SetUp() override { ASSERT_TRUE(backend.initialize()); }
    void TearDown() override { backend.shutdown(); }
};

// Zero numQueries: must return empty batch (0 entries, 0 successes, 0 failures).
TEST_F(BatchKnnSearchSafeTest, ZeroNumQueries_ReturnsEmptyBatch) {
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearchSafe(q, 0, 2, v, 1, 1, true);
    EXPECT_EQ(result.queryResults.size(), 0u);
    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failureCount, 0u);
}

// Null query pointer with numQueries == 0: loop body never executes, no crash.
TEST_F(BatchKnnSearchSafeTest, NullQueriesPointer_ZeroNumQueries_ReturnsEmptyBatch) {
    const float v[] = {1.f, 0.f};
    auto result = backend.batchKnnSearchSafe(nullptr, 0, 2, v, 1, 1, true);
    EXPECT_EQ(result.queryResults.size(), 0u);
    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failureCount, 0u);
}

// NaN in query → InputRangeViolation (regression for partial-failure path).
TEST_F(BatchKnnSearchSafeTest, NaNInQuery_SetsInputRangeViolation) {
    const float v[] = {1.f, 0.f};
    const float kNaN = std::numeric_limits<float>::quiet_NaN();
    float q[] = {kNaN, 0.f};
    auto result = backend.batchKnnSearchSafe(q, 1, 2, v, 1, 1, true);
    ASSERT_EQ(result.queryResults.size(), 1u);
    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::InputRangeViolation);
    EXPECT_EQ(result.successCount, 0u);
    EXPECT_EQ(result.failureCount, 1u);
}

// Inf in query → InputRangeViolation.
TEST_F(BatchKnnSearchSafeTest, InfInQuery_SetsInputRangeViolation) {
    const float v[] = {1.f, 0.f};
    const float kInf = std::numeric_limits<float>::infinity();
    float q[] = {kInf, 0.f};
    auto result = backend.batchKnnSearchSafe(q, 1, 2, v, 1, 1, true);
    ASSERT_EQ(result.queryResults.size(), 1u);
    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::InputRangeViolation);
}

// Valid query → Success status and non-empty neighbors.
TEST_F(BatchKnnSearchSafeTest, ValidQuery_ReturnsSuccessWithNeighbors) {
    const float v[] = {1.f, 0.f,  0.f, 1.f};
    const float q[] = {0.5f, 0.5f};
    auto result = backend.batchKnnSearchSafe(q, 1, 2, v, 2, 1, true);
    ASSERT_EQ(result.queryResults.size(), 1u);
    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::Success);
    EXPECT_FALSE(result.queryResults[0].neighbors.empty());
    EXPECT_EQ(result.successCount, 1u);
    EXPECT_EQ(result.failureCount, 0u);
}

// Mixed: first query NaN, second valid → 1 failure, 1 success.
TEST_F(BatchKnnSearchSafeTest, MixedNaNAndValidQuery_PartialFailure) {
    const float v[] = {1.f, 0.f};
    const float kNaN = std::numeric_limits<float>::quiet_NaN();
    float q[] = {kNaN, 0.f,   // query 0: invalid
                 0.5f, 0.5f}; // query 1: valid
    auto result = backend.batchKnnSearchSafe(q, 2, 2, v, 1, 1, true);
    ASSERT_EQ(result.queryResults.size(), 2u);
    EXPECT_EQ(result.queryResults[0].status, AccelerationErrorCode::InputRangeViolation);
    EXPECT_EQ(result.queryResults[1].status, AccelerationErrorCode::Success);
    EXPECT_EQ(result.successCount, 1u);
    EXPECT_EQ(result.failureCount, 1u);
}

// =============================================================================
// 6. Runtime fallback: BackendRegistry CPU selection when GPU unavailable
// =============================================================================

class RuntimeFallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure CPU backends are registered.
        auto& reg = BackendRegistry::instance();
        reg.registerBackend(std::make_unique<CPUVectorBackend>());
        reg.registerBackend(std::make_unique<CPUGraphBackend>());
        reg.registerBackend(std::make_unique<CPUGeoBackend>());
        reg.initializeRuntime();
    }
    void TearDown() override {
        BackendRegistry::instance().shutdownAll();
    }
};

// Without GPU hardware the registry must return a non-null CPU vector backend.
TEST_F(RuntimeFallbackTest, VectorBackend_IsNonNull_WhenNoGPU) {
    auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
    EXPECT_NE(vb, nullptr);
}

// The selected vector backend must be CPU type in a no-GPU environment.
TEST_F(RuntimeFallbackTest, VectorBackend_IsCPU_WhenNoGPU) {
    auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
    ASSERT_NE(vb, nullptr);
    EXPECT_EQ(vb->type(), BackendType::CPU);
}

// The selected graph backend must be CPU type in a no-GPU environment.
TEST_F(RuntimeFallbackTest, GraphBackend_IsCPU_WhenNoGPU) {
    auto* gb = BackendRegistry::instance().getSelectedGraphBackend();
    ASSERT_NE(gb, nullptr);
    EXPECT_EQ(gb->type(), BackendType::CPU);
}

// The selected geo backend must be CPU type in a no-GPU environment.
TEST_F(RuntimeFallbackTest, GeoBackend_IsCPU_WhenNoGPU) {
    auto* geo = BackendRegistry::instance().getSelectedGeoBackend();
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->type(), BackendType::CPU);
}

// Impossible requirements (async vector ops that CPU cannot satisfy) → nullptr.
TEST_F(RuntimeFallbackTest, ImpossibleRequirements_SelectedVectorIsNull) {
    BackendRegistry::CapabilityRequirements impossible;
    impossible.needsVectorOps = true;
    impossible.needsAsync     = true; // CPU backends do not support async

    BackendRegistry::instance().initializeRuntime(
        impossible,
        BackendRegistry::defaultGraphRequirements(),
        BackendRegistry::defaultGeoRequirements());

    EXPECT_EQ(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
    // Graph and Geo should still resolve to CPU.
    EXPECT_NE(BackendRegistry::instance().getSelectedGraphBackend(), nullptr);
    EXPECT_NE(BackendRegistry::instance().getSelectedGeoBackend(),   nullptr);
}

// Re-initialisation with default requirements must restore the CPU vector backend.
TEST_F(RuntimeFallbackTest, ReInitWithDefaultRequirements_RestoresCPUVector) {
    // First: make selection fail
    BackendRegistry::CapabilityRequirements impossible;
    impossible.needsVectorOps = true;
    impossible.needsAsync     = true;
    BackendRegistry::instance().initializeRuntime(
        impossible,
        BackendRegistry::defaultGraphRequirements(),
        BackendRegistry::defaultGeoRequirements());
    ASSERT_EQ(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);

    // Second: restore with default reqs
    BackendRegistry::instance().initializeRuntime();
    EXPECT_NE(BackendRegistry::instance().getSelectedVectorBackend(), nullptr);
    EXPECT_EQ(BackendRegistry::instance().getSelectedVectorBackend()->type(),
              BackendType::CPU);
}

// getBestVectorBackend() always returns the CPU backend in a no-GPU environment.
TEST_F(RuntimeFallbackTest, GetBestVectorBackend_ReturnsCPU_WhenNoGPU) {
    auto* best = BackendRegistry::instance().getBestVectorBackend();
    ASSERT_NE(best, nullptr);
    EXPECT_EQ(best->type(), BackendType::CPU);
    EXPECT_TRUE(best->isAvailable());
}

// =============================================================================
// 7. Dispatcher fallback: CPU dispatch table used as live fallback
// =============================================================================

TEST(DispatcherRuntimeFallback, ANN_CPUFallback_L2_CorrectResults) {
    // Primary dispatch is empty (all null slots) → all calls route to CPU.
    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    ANNKernelDispatch cpuTable = cpu.populateANNDispatch();

    RetryPolicy noDelay;
    noDelay.maxAttempts    = 1;
    noDelay.initialDelayMs = 0;
    noDelay.maxDelayMs     = 0;

    ANNKernelDispatch emptyPrimary;
    ANNKernelFallbackDispatcher disp(emptyPrimary, cpuTable, noDelay);

    // Query [1,0] vs [1,0] and [0,1] → expected squared-L2: 0 and 2.
    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f,  0.f, 1.f};
    float dists[2]  = {};

    int rc = disp.launchL2Distance(q, v, dists, 1, 2, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dists[0], 0.f, 1e-5f);
    EXPECT_NEAR(dists[1], 2.f, 1e-5f);

    cpu.shutdown();
}

TEST(DispatcherRuntimeFallback, ANN_CPUFallback_Cosine_SelfDistanceIsZero) {
    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    ANNKernelDispatch cpuTable = cpu.populateANNDispatch();

    RetryPolicy noDelay;
    noDelay.maxAttempts    = 1;
    noDelay.initialDelayMs = 0;
    noDelay.maxDelayMs     = 0;

    ANNKernelDispatch emptyPrimary;
    ANNKernelFallbackDispatcher disp(emptyPrimary, cpuTable, noDelay);

    const float q[] = {1.f, 0.f};
    const float v[] = {1.f, 0.f};
    float dist = 0.f;

    int rc = disp.launchCosine(q, v, &dist, 1, 1, 2, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 0.f, 1e-5f);

    cpu.shutdown();
}

TEST(DispatcherRuntimeFallback, Geo_CPUFallback_Haversine_ParisToLondon) {
    CPUGeoBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    GeoKernelDispatch cpuTable = cpu.populateGeoDispatch();

    RetryPolicy noDelay;
    noDelay.maxAttempts    = 1;
    noDelay.initialDelayMs = 0;
    noDelay.maxDelayMs     = 0;

    GeoKernelDispatch emptyPrimary;
    GeoKernelFallbackDispatcher disp(emptyPrimary, cpuTable, noDelay);

    const double lat1[] = {48.8566}, lon1[] = {2.3522};
    const double lat2[] = {51.5074}, lon2[] = {-0.1278};
    float dist = 0.f;

    int rc = disp.launchDistance(lat1, lon1, lat2, lon2, &dist, 1,
                                 GeoDistanceFormula::HAVERSINE, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 340.f, 15.f);  // Paris→London ≈ 340 km

    cpu.shutdown();
}

// =============================================================================
// 8. BatchValidator unit regression tests
// =============================================================================

TEST(BatchValidatorRegression, ValidateVectorBatch_NullQuery_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    const float v[] = {1.f, 0.f};
    bool ok = BatchValidator::validateVectorBatch("TestBackend",
                                                  nullptr, 1, 2,
                                                  v, 1, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}

TEST(BatchValidatorRegression, ValidateVectorBatch_ZeroDim_ReturnsFalse) {
    bool called = false;
    auto sink = [&](ErrorContext) { called = true; };

    const float q[] = {1.f}, v[] = {1.f};
    bool ok = BatchValidator::validateVectorBatch("TestBackend",
                                                  q, 1, 0,
                                                  v, 1, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
}

TEST(BatchValidatorRegression, ValidateVectorBatch_ValidInput_ReturnsTrue) {
    bool called = false;
    auto sink = [&](ErrorContext) { called = true; };

    const float q[] = {1.f, 0.f}, v[] = {1.f, 0.f};
    bool ok = BatchValidator::validateVectorBatch("TestBackend",
                                                  q, 1, 2,
                                                  v, 1, sink);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(called);
}

TEST(BatchValidatorRegression, ValidateK_ZeroK_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    bool ok = BatchValidator::validateK("TestBackend", 0, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}

TEST(BatchValidatorRegression, ValidateK_PositiveK_ReturnsTrue) {
    bool called = false;
    auto sink = [&](ErrorContext) { called = true; };

    bool ok = BatchValidator::validateK("TestBackend", 5, sink);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(called);
}

TEST(BatchValidatorRegression, ValidateGeoBatch_NullPointer_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    const double lons1[] = {0.0}, lats2[] = {0.0}, lons2[] = {0.0};
    bool ok = BatchValidator::validateGeoBatch("TestBackend",
                                               nullptr, lons1, lats2, lons2, 1, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}

TEST(BatchValidatorRegression, ValidatePointInPolygon_TooFewVertices_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    const double lats[] = {0.5}, lons[] = {0.5};
    const double poly[] = {0.0, 0.0, 1.0, 1.0};  // 2 vertices only
    bool ok = BatchValidator::validatePointInPolygonBatch("TestBackend",
                                                          lats, lons, 1,
                                                          poly, 2, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}

TEST(BatchValidatorRegression, ValidateGraphBFS_NullAdjacency_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    const uint32_t starts[] = {0u};
    bool ok = BatchValidator::validateGraphBFSBatch("TestBackend",
                                                    nullptr, 4, starts, 1, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}

TEST(BatchValidatorRegression, ValidateShortestPath_NullWeights_ReturnsFalse) {
    bool called = false;
    AccelerationErrorCode code = AccelerationErrorCode::Success;
    auto sink = [&](ErrorContext ctx) {
        called = true;
        code   = ctx.code;
    };

    const uint32_t adj[] = {0u}, s[] = {0u}, e[] = {1u};
    bool ok = BatchValidator::validateShortestPathBatch("TestBackend",
                                                        adj, nullptr, 4,
                                                        s, e, 1, sink);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(called);
    EXPECT_EQ(code, AccelerationErrorCode::InvalidInputShape);
}
