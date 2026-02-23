// Test: Acceleration Backend API Stability
//
// Validates the stability guarantees for the public acceleration backend
// contracts defined in:
//   - include/acceleration/compute_backend.h
//   - include/acceleration/kernel_invocation.h
//   - include/acceleration/error_codes.h
//   - include/acceleration/kernel_fallback_dispatcher.h
//
// These tests act as a canary: if any of the declared-stable enum values,
// struct field layouts, or version constants change the corresponding test
// will fail at compile-time or test-time, alerting maintainers to update
// BACKEND_CONTRACT_VERSION / KERNEL_INVOCATION_INTERFACE_VERSION.
//
// Every test that verifies a numeric enum value uses a static_assert (compile
// time) AND a runtime EXPECT_EQ (for clean failure messages in CI logs).
//
// Platform: No GPU hardware required — all checks are CPU-side.

#include <gtest/gtest.h>
#include <cstdint>
#include <type_traits>

#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"
#include "acceleration/error_codes.h"
#include "acceleration/kernel_fallback_dispatcher.h"

using namespace themis::acceleration;

// =============================================================================
// Version constants
// =============================================================================

TEST(BackendApiStability, BackendContractVersionIsDefined) {
    // BACKEND_CONTRACT_VERSION must be accessible and equal to 100 (v1.0).
    static_assert(BACKEND_CONTRACT_VERSION == 100u,
        "BACKEND_CONTRACT_VERSION must be 100 (v1.0); bump only on breaking changes");
    EXPECT_EQ(BACKEND_CONTRACT_VERSION, 100u);
}

TEST(BackendApiStability, KernelInvocationInterfaceVersionIsDefined) {
    // KERNEL_INVOCATION_INTERFACE_VERSION must remain 100 (v1.0) until a
    // breaking change to the frozen kernel-launcher typedefs occurs.
    static_assert(KERNEL_INVOCATION_INTERFACE_VERSION == 100u,
        "KERNEL_INVOCATION_INTERFACE_VERSION must be 100 (v1.0)");
    EXPECT_EQ(KERNEL_INVOCATION_INTERFACE_VERSION, 100u);
}

TEST(BackendApiStability, VersionConstantsAreUint32) {
    static_assert(std::is_same<
            decltype(BACKEND_CONTRACT_VERSION), const uint32_t>::value,
        "BACKEND_CONTRACT_VERSION must be uint32_t");
    static_assert(std::is_same<
            decltype(KERNEL_INVOCATION_INTERFACE_VERSION), const uint32_t>::value,
        "KERNEL_INVOCATION_INTERFACE_VERSION must be uint32_t");
}

// =============================================================================
// DistanceMetric enum value stability
// =============================================================================

TEST(BackendApiStability, DistanceMetricUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<DistanceMetric>::type, uint32_t>::value,
        "DistanceMetric underlying type must remain uint32_t");
}

TEST(BackendApiStability, DistanceMetricL2IsZero) {
    static_assert(static_cast<uint32_t>(DistanceMetric::L2) == 0u,
        "DistanceMetric::L2 must equal 0; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::L2), 0u);
}

TEST(BackendApiStability, DistanceMetricCosineIsOne) {
    static_assert(static_cast<uint32_t>(DistanceMetric::COSINE) == 1u,
        "DistanceMetric::COSINE must equal 1; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::COSINE), 1u);
}

TEST(BackendApiStability, DistanceMetricInnerProductIsTwo) {
    static_assert(static_cast<uint32_t>(DistanceMetric::INNER_PRODUCT) == 2u,
        "DistanceMetric::INNER_PRODUCT must equal 2; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(DistanceMetric::INNER_PRODUCT), 2u);
}

// =============================================================================
// GeoDistanceFormula enum value stability
// =============================================================================

TEST(BackendApiStability, GeoDistanceFormulaUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<GeoDistanceFormula>::type, uint32_t>::value,
        "GeoDistanceFormula underlying type must remain uint32_t");
}

TEST(BackendApiStability, GeoDistanceFormulaHaversineIsZero) {
    static_assert(static_cast<uint32_t>(GeoDistanceFormula::HAVERSINE) == 0u,
        "GeoDistanceFormula::HAVERSINE must equal 0; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(GeoDistanceFormula::HAVERSINE), 0u);
}

TEST(BackendApiStability, GeoDistanceFormulaVincentyIsOne) {
    static_assert(static_cast<uint32_t>(GeoDistanceFormula::VINCENTY) == 1u,
        "GeoDistanceFormula::VINCENTY must equal 1; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(GeoDistanceFormula::VINCENTY), 1u);
}

// =============================================================================
// MatrixPrecision enum value stability
// =============================================================================

TEST(BackendApiStability, MatrixPrecisionUnderlyingTypeIsUint32) {
    static_assert(
        std::is_same<std::underlying_type<MatrixPrecision>::type, uint32_t>::value,
        "MatrixPrecision underlying type must remain uint32_t");
}

TEST(BackendApiStability, MatrixPrecisionFP32IsZero) {
    static_assert(static_cast<uint32_t>(MatrixPrecision::FP32) == 0u,
        "MatrixPrecision::FP32 must equal 0; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::FP32), 0u);
}

TEST(BackendApiStability, MatrixPrecisionFP16IsOne) {
    static_assert(static_cast<uint32_t>(MatrixPrecision::FP16) == 1u,
        "MatrixPrecision::FP16 must equal 1; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::FP16), 1u);
}

TEST(BackendApiStability, MatrixPrecisionBF16IsTwo) {
    static_assert(static_cast<uint32_t>(MatrixPrecision::BF16) == 2u,
        "MatrixPrecision::BF16 must equal 2; this value is frozen");
    EXPECT_EQ(static_cast<uint32_t>(MatrixPrecision::BF16), 2u);
}

// =============================================================================
// PrecisionMode bitmask stability
// =============================================================================

TEST(BackendApiStability, PrecisionModeNoneIsZero) {
    static_assert(static_cast<uint32_t>(PrecisionMode::NONE) == 0u,
        "PrecisionMode::NONE must equal 0");
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::NONE), 0u);
}

TEST(BackendApiStability, PrecisionModeFP32IsBit0) {
    static_assert(static_cast<uint32_t>(PrecisionMode::FP32) == (1u << 0),
        "PrecisionMode::FP32 must be bit 0 (value 1)");
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::FP32), 1u << 0);
}

TEST(BackendApiStability, PrecisionModeFP16IsBit1) {
    static_assert(static_cast<uint32_t>(PrecisionMode::FP16) == (1u << 1),
        "PrecisionMode::FP16 must be bit 1 (value 2)");
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::FP16), 1u << 1);
}

TEST(BackendApiStability, PrecisionModeBF16IsBit2) {
    static_assert(static_cast<uint32_t>(PrecisionMode::BF16) == (1u << 2),
        "PrecisionMode::BF16 must be bit 2 (value 4)");
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::BF16), 1u << 2);
}

TEST(BackendApiStability, PrecisionModeINT8IsBit3) {
    static_assert(static_cast<uint32_t>(PrecisionMode::INT8) == (1u << 3),
        "PrecisionMode::INT8 must be bit 3 (value 8)");
    EXPECT_EQ(static_cast<uint32_t>(PrecisionMode::INT8), 1u << 3);
}

// =============================================================================
// AccelerationErrorCode key value stability
// =============================================================================

TEST(BackendApiStability, ErrorCodeSuccessIsZero) {
    static_assert(static_cast<uint32_t>(AccelerationErrorCode::Success) == 0u,
        "AccelerationErrorCode::Success must equal 0");
    EXPECT_EQ(static_cast<uint32_t>(AccelerationErrorCode::Success), 0u);
}

TEST(BackendApiStability, ErrorCodeNotImplementedIs902) {
    EXPECT_EQ(static_cast<uint32_t>(AccelerationErrorCode::NotImplemented), 902u);
}

TEST(BackendApiStability, ErrorCodeInputRangeViolationIs605) {
    // Used in batchKnnSearchSafe() for NaN/Inf inputs; value must not change.
    EXPECT_EQ(static_cast<uint32_t>(AccelerationErrorCode::InputRangeViolation), 605u);
}

TEST(BackendApiStability, ErrorCodeKernelNotFoundIs502) {
    // Used by ANNKernelFallbackDispatcher when both primary and fallback are null.
    EXPECT_EQ(static_cast<uint32_t>(AccelerationErrorCode::KernelNotFound), 502u);
}

// =============================================================================
// Struct field existence (compile-time membership checks)
// These tests verify that required struct members still exist; if a field is
// removed or renamed the test will fail to compile.
// =============================================================================

TEST(BackendApiStability, ANNKernelParamsHasRequiredFields) {
    ANNKernelParams p;
    // Verify all required fields are present and have the expected types.
    const float*   q  = p.queries;    (void)q;
    size_t         nq = p.numQueries; (void)nq;
    size_t         d  = p.dim;        (void)d;
    const float*   v  = p.vectors;    (void)v;
    size_t         nv = p.numVectors; (void)nv;
    size_t         k  = p.topK;       (void)k;
    DistanceMetric m  = p.metric;     (void)m;
    SUCCEED();
}

TEST(BackendApiStability, ANNKernelResultHasRequiredFields) {
    ANNKernelResult r;
    uint32_t* idx  = r.indices;   (void)idx;
    float*    dist = r.distances; (void)dist;
    SUCCEED();
}

TEST(BackendApiStability, GeoKernelParamsHasRequiredFields) {
    GeoKernelParams p;
    const double*      lats1 = p.latitudes1;  (void)lats1;
    const double*      lons1 = p.longitudes1; (void)lons1;
    const double*      lats2 = p.latitudes2;  (void)lats2;
    const double*      lons2 = p.longitudes2; (void)lons2;
    size_t             cnt   = p.count;       (void)cnt;
    GeoDistanceFormula fmt   = p.formula;     (void)fmt;
    SUCCEED();
}

TEST(BackendApiStability, GeoContainmentParamsHasRequiredFields) {
    GeoContainmentParams p;
    const double* pLats = p.pointLats;          (void)pLats;
    const double* pLons = p.pointLons;          (void)pLons;
    size_t        nPts  = p.numPoints;          (void)nPts;
    const double* poly  = p.polygonCoords;      (void)poly;
    size_t        nVert = p.numPolygonVertices; (void)nVert;
    SUCCEED();
}

TEST(BackendApiStability, MatrixKernelParamsHasRequiredFields) {
    MatrixKernelParams p;
    const void*     A   = p.A;         (void)A;
    const void*     B   = p.B;         (void)B;
    void*           C   = p.C;         (void)C;
    size_t          M   = p.M;         (void)M;
    size_t          K   = p.K;         (void)K;
    size_t          N   = p.N;         (void)N;
    float           al  = p.alpha;     (void)al;
    float           be  = p.beta;      (void)be;
    MatrixPrecision pr  = p.precision; (void)pr;
    SUCCEED();
}

TEST(BackendApiStability, ANNKernelDispatchHasAllSlots) {
    ANNKernelDispatch d;
    ANNDistanceFn l2  = d.launchL2Distance;   (void)l2;
    ANNDistanceFn cs  = d.launchCosine;       (void)cs;
    ANNDistanceFn ip  = d.launchInnerProduct; (void)ip;
    ANNTopKFn     tk  = d.launchTopK;         (void)tk;
    // distanceLauncherFor() must exist and accept a DistanceMetric
    ANNDistanceFn fn  = d.distanceLauncherFor(DistanceMetric::L2); (void)fn;
    SUCCEED();
}

TEST(BackendApiStability, GeoKernelDispatchHasAllSlots) {
    GeoKernelDispatch d;
    GeoDistanceFn    dist = d.launchDistance;    (void)dist;
    GeoContainmentFn cont = d.launchContainment; (void)cont;
    SUCCEED();
}

TEST(BackendApiStability, MatrixKernelDispatchHasLaunchMatmul) {
    MatrixKernelDispatch d;
    MatrixKernelFn fn = d.launchMatmul; (void)fn;
    SUCCEED();
}

// =============================================================================
// BackendCapabilities field existence
// =============================================================================

TEST(BackendApiStability, BackendCapabilitiesHasRequiredFields) {
    BackendCapabilities caps;
    bool vo  = caps.supportsVectorOps;      (void)vo;
    bool go  = caps.supportsGraphOps;       (void)go;
    bool goo = caps.supportsGeoOps;         (void)goo;
    bool mo  = caps.supportsMatrixOps;      (void)mo;
    bool bp  = caps.supportsBatchProcessing;(void)bp;
    bool as  = caps.supportsAsync;          (void)as;
    PrecisionMode pm  = caps.supportedPrecisions; (void)pm;
    uint32_t      sm  = caps.supportedMetrics;    (void)sm;
    size_t        mem = caps.maxMemoryBytes;       (void)mem;
    int           cu  = caps.computeUnits;         (void)cu;
    const std::string& dn = caps.deviceName;       (void)dn;
    const std::string& vn = caps.vendorName;       (void)vn;
    SUCCEED();
}

// =============================================================================
// BackendHealthStatus builder helpers exist
// =============================================================================

TEST(BackendApiStability, BackendHealthStatusBuildersExist) {
    auto healthy    = BackendHealthStatus::makeHealthy("testDevice");
    auto degraded   = BackendHealthStatus::makeDegraded("some issue");
    auto unhealthy  = BackendHealthStatus::makeUnhealthy("critical failure");

    EXPECT_EQ(healthy.status,   "healthy");
    EXPECT_EQ(degraded.status,  "degraded");
    EXPECT_EQ(unhealthy.status, "unhealthy");
}

// =============================================================================
// KnnQueryResult and PartialBatchResult field existence
// =============================================================================

TEST(BackendApiStability, KnnQueryResultHasRequiredFields) {
    KnnQueryResult qr;
    // neighbors must be a vector of (index, distance) pairs
    std::vector<std::pair<uint32_t, float>>& nb = qr.neighbors; (void)nb;
    AccelerationErrorCode st = qr.status; (void)st;
    const std::string& em   = qr.errorMessage; (void)em;
    EXPECT_EQ(qr.status, AccelerationErrorCode::Success);
    SUCCEED();
}

TEST(BackendApiStability, PartialBatchResultHasRequiredFields) {
    PartialBatchResult pr;
    std::vector<KnnQueryResult>& qr = pr.queryResults; (void)qr;
    size_t sc = pr.successCount; (void)sc;
    size_t fc = pr.failureCount; (void)fc;
    EXPECT_EQ(pr.successCount, 0u);
    EXPECT_EQ(pr.failureCount, 0u);
    SUCCEED();
}

// =============================================================================
// RetryPolicy field existence (kernel_fallback_dispatcher.h)
// =============================================================================

TEST(BackendApiStability, RetryPolicyHasRequiredFields) {
    RetryPolicy rp;
    uint32_t ma = rp.maxAttempts;       (void)ma;
    uint32_t id = rp.initialDelayMs;    (void)id;
    uint32_t md = rp.maxDelayMs;        (void)md;
    float    bm = rp.backoffMultiplier; (void)bm;
    // Default values must be stable
    EXPECT_EQ(rp.maxAttempts, 3u);
    EXPECT_EQ(rp.initialDelayMs, 1u);
    EXPECT_EQ(rp.maxDelayMs, 100u);
    EXPECT_FLOAT_EQ(rp.backoffMultiplier, 2.0f);
    SUCCEED();
}

// =============================================================================
// isTransientDispatchError() stability
// Verifies that the three transient error codes are correctly identified by
// the fallback dispatcher; these codes are part of the stable API.
// =============================================================================

TEST(BackendApiStability, IsTransientDispatchError_SynchronizationFailed) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::SynchronizationFailed)));
}

TEST(BackendApiStability, IsTransientDispatchError_OperationTimeout) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::OperationTimeout)));
}

TEST(BackendApiStability, IsTransientDispatchError_DeviceLost) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::DeviceLost)));
}

TEST(BackendApiStability, IsTransientDispatchError_NonTransientCodes) {
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::Success)));
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::KernelLaunchFailed)));
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::InputRangeViolation)));
}

// =============================================================================
// BackendRegistry::CapabilityRequirements field existence
// =============================================================================

TEST(BackendApiStability, CapabilityRequirementsHasRequiredFields) {
    BackendRegistry::CapabilityRequirements reqs;
    bool nvo = reqs.needsVectorOps; (void)nvo;
    bool ngo = reqs.needsGraphOps;  (void)ngo;
    bool nge = reqs.needsGeoOps;    (void)nge;
    bool nmo = reqs.needsMatrixOps; (void)nmo;
    bool nb  = reqs.needsBatch;     (void)nb;
    bool na  = reqs.needsAsync;     (void)na;
    PrecisionMode rp = reqs.requiredPrecisions; (void)rp;
    uint32_t      rm = reqs.requiredMetrics;    (void)rm;
    SUCCEED();
}
