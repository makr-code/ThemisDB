// Test: Kernel Fallback / Retry Dispatcher
//
// Validates the ANNKernelFallbackDispatcher and GeoKernelFallbackDispatcher
// classes defined in include/acceleration/kernel_fallback_dispatcher.h.
//
// All tests run on any platform — no GPU is required.

#include <gtest/gtest.h>
#include "acceleration/kernel_fallback_dispatcher.h"
#include "acceleration/kernel_invocation.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/error_codes.h"

#include <cstdint>
#include <vector>
#include <atomic>

using namespace themis::acceleration;

// =============================================================================
// Test helpers — minimal stub kernels
//
// Each stub is a plain free function with the exact signature required by the
// corresponding function-pointer typedef so it can be stored in the dispatch
// tables without any adaptor glue.
// =============================================================================

// Shared call counters (reset in SetUp)
static std::atomic<int> g_primaryCallCount{0};
static std::atomic<int> g_fallbackCallCount{0};

// Return codes for the primary stub — tests can change this
static int g_primaryReturnCode = 0;
// How many transient failures before the primary succeeds (0 = always fail)
static int g_transientFailsRemaining = 0;

// --- ANN distance stub (primary): writes 0.5 into every output cell ---
static int stubPrimaryL2(const float*, const float*, float* d,
                         int nq, int nv, int, void*) {
    if (g_transientFailsRemaining > 0) {
        --g_transientFailsRemaining;
        ++g_primaryCallCount;
        return static_cast<int>(AccelerationErrorCode::DeviceLost);
    }
    int rc = g_primaryReturnCode;
    if (rc == 0) {
        for (int i = 0; i < nq * nv; ++i) d[i] = 0.5f;
    }
    ++g_primaryCallCount;
    return rc;
}

// --- ANN distance stub (fallback): writes 9.9 into every output cell ---
static int stubFallbackL2(const float*, const float*, float* d,
                          int nq, int nv, int, void*) {
    for (int i = 0; i < nq * nv; ++i) d[i] = 9.9f;
    ++g_fallbackCallCount;
    return 0;
}

// --- ANN top-k stub (primary): marks as selected by writing distance 1.f ---
static int stubPrimaryTopK(const float*, uint32_t* idx, float* dists,
                            int nq, int topK, int, void*) {
    for (int i = 0; i < nq * topK; ++i) { idx[i] = 0; dists[i] = 1.f; }
    ++g_primaryCallCount;
    return g_primaryReturnCode;
}

// --- ANN top-k stub (fallback): writes index 99 ---
static int stubFallbackTopK(const float*, uint32_t* idx, float* dists,
                             int nq, int topK, int, void*) {
    for (int i = 0; i < nq * topK; ++i) { idx[i] = 99; dists[i] = 99.f; }
    ++g_fallbackCallCount;
    return 0;
}

// --- Geo distance stub (primary) ---
static int stubPrimaryGeoDist(const double*, const double*,
                               const double*, const double*,
                               float* out, int count,
                               GeoDistanceFormula, void*) {
    for (int i = 0; i < count; ++i) out[i] = 1.f;
    ++g_primaryCallCount;
    return g_primaryReturnCode;
}

// --- Geo distance stub (fallback) ---
static int stubFallbackGeoDist(const double*, const double*,
                                const double*, const double*,
                                float* out, int count,
                                GeoDistanceFormula, void*) {
    for (int i = 0; i < count; ++i) out[i] = 999.f;
    ++g_fallbackCallCount;
    return 0;
}

// --- Geo containment stub (primary) ---
static int stubPrimaryGeoContain(const double*, const double*, int numPts,
                                  const double*, int,
                                  uint8_t* res, void*) {
    for (int i = 0; i < numPts; ++i) res[i] = 1;
    ++g_primaryCallCount;
    return g_primaryReturnCode;
}

// --- Geo containment stub (fallback) ---
static int stubFallbackGeoContain(const double*, const double*, int numPts,
                                   const double*, int,
                                   uint8_t* res, void*) {
    for (int i = 0; i < numPts; ++i) res[i] = 0;
    ++g_fallbackCallCount;
    return 0;
}

// =============================================================================
// Test fixture
// =============================================================================

class KernelFallbackDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_primaryCallCount        = 0;
        g_fallbackCallCount       = 0;
        g_primaryReturnCode       = 0;
        g_transientFailsRemaining = 0;
    }

    // Build a primary ANN dispatch with all 4 slots populated.
    static ANNKernelDispatch makePrimaryANN() {
        ANNKernelDispatch d;
        d.launchL2Distance   = stubPrimaryL2;
        d.launchCosine       = stubPrimaryL2;   // reuse stub
        d.launchInnerProduct = stubPrimaryL2;
        d.launchTopK         = stubPrimaryTopK;
        return d;
    }

    // Build a fallback ANN dispatch with all 4 slots populated.
    static ANNKernelDispatch makeFallbackANN() {
        ANNKernelDispatch d;
        d.launchL2Distance   = stubFallbackL2;
        d.launchCosine       = stubFallbackL2;
        d.launchInnerProduct = stubFallbackL2;
        d.launchTopK         = stubFallbackTopK;
        return d;
    }

    // Build a primary Geo dispatch with both slots populated.
    static GeoKernelDispatch makePrimaryGeo() {
        GeoKernelDispatch d;
        d.launchDistance    = stubPrimaryGeoDist;
        d.launchContainment = stubPrimaryGeoContain;
        return d;
    }

    // Build a fallback Geo dispatch.
    static GeoKernelDispatch makeFallbackGeo() {
        GeoKernelDispatch d;
        d.launchDistance    = stubFallbackGeoDist;
        d.launchContainment = stubFallbackGeoContain;
        return d;
    }

    // Zero-delay policy so tests run fast.
    static RetryPolicy fastPolicy(uint32_t maxAttempts = 3) {
        RetryPolicy p;
        p.maxAttempts       = maxAttempts;
        p.initialDelayMs    = 0;
        p.maxDelayMs        = 0;
        p.backoffMultiplier = 2.0f;
        return p;
    }
};

// =============================================================================
// isTransientDispatchError
// =============================================================================

TEST(KernelFallbackDispatcher, IsTransient_DeviceLost) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::DeviceLost)));
}

TEST(KernelFallbackDispatcher, IsTransient_OperationTimeout) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::OperationTimeout)));
}

TEST(KernelFallbackDispatcher, IsTransient_SynchronizationFailed) {
    EXPECT_TRUE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::SynchronizationFailed)));
}

TEST(KernelFallbackDispatcher, IsTransient_ZeroIsNotTransient) {
    EXPECT_FALSE(isTransientDispatchError(0));
}

TEST(KernelFallbackDispatcher, IsTransient_PermanentErrorsAreNotTransient) {
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::KernelLaunchFailed)));
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::KernelNotFound)));
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::OutOfDeviceMemory)));
    EXPECT_FALSE(isTransientDispatchError(
        static_cast<int>(AccelerationErrorCode::FeatureNotSupported)));
}

// =============================================================================
// RetryPolicy defaults
// =============================================================================

TEST(KernelFallbackDispatcher, RetryPolicyDefaults) {
    RetryPolicy p;
    EXPECT_EQ(p.maxAttempts,       3u);
    EXPECT_EQ(p.initialDelayMs,    1u);
    EXPECT_EQ(p.maxDelayMs,        100u);
    EXPECT_NEAR(p.backoffMultiplier, 2.0f, 1e-6f);
}

// =============================================================================
// ANNKernelFallbackDispatcher — null-slot fallback (no retry needed)
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_NullPrimarySlot_UsesFallback) {
    ANNKernelDispatch primary;   // all slots nullptr by default
    ANNKernelDispatch fallback = makeFallbackANN();

    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    float distances[2] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 2, 4, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount, 0);
    EXPECT_EQ(g_fallbackCallCount, 1);
    // Fallback stub writes 9.9
    EXPECT_NEAR(distances[0], 9.9f, 1e-4f);
    EXPECT_NEAR(distances[1], 9.9f, 1e-4f);
}

TEST_F(KernelFallbackDispatcherTest, ANN_NullBothSlots_ReturnsKernelNotFound) {
    ANNKernelDispatch empty;  // all nullptr
    ANNKernelFallbackDispatcher disp(empty, empty, fastPolicy());

    float d[1] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, d, 1, 1, 1, nullptr);

    EXPECT_EQ(rc, static_cast<int>(AccelerationErrorCode::KernelNotFound));
    EXPECT_EQ(g_primaryCallCount,  0);
    EXPECT_EQ(g_fallbackCallCount, 0);
}

// =============================================================================
// ANNKernelFallbackDispatcher — success on primary (no fallback)
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_PrimarySucceeds_NoFallback) {
    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    float distances[2] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 2, 4, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  1);
    EXPECT_EQ(g_fallbackCallCount, 0);
    // Primary stub writes 0.5
    EXPECT_NEAR(distances[0], 0.5f, 1e-4f);
}

// =============================================================================
// ANNKernelFallbackDispatcher — transient error → retry → success
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_TransientError_Retries_ThenSucceeds) {
    // Primary fails once (DeviceLost) then succeeds.
    g_transientFailsRemaining = 1;

    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy(3));

    float distances[1] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 1, 1, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  2);  // 1 fail + 1 success
    EXPECT_EQ(g_fallbackCallCount, 0);  // fallback not reached
    EXPECT_NEAR(distances[0], 0.5f, 1e-4f);  // primary stub value
}

TEST_F(KernelFallbackDispatcherTest, ANN_AllTransientRetries_Exhausted_FallsBack) {
    // Primary always returns DeviceLost (set g_primaryReturnCode, not transient counter).
    g_primaryReturnCode = static_cast<int>(AccelerationErrorCode::DeviceLost);

    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy(3));

    float distances[2] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 2, 4, nullptr);

    EXPECT_EQ(rc, 0);                          // fallback succeeded
    EXPECT_EQ(g_primaryCallCount,  3);         // 3 attempts
    EXPECT_EQ(g_fallbackCallCount, 1);         // fell back once
    EXPECT_NEAR(distances[0], 9.9f, 1e-4f);   // fallback stub value
}

// =============================================================================
// ANNKernelFallbackDispatcher — permanent error → immediate fallback
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_PermanentError_NoRetry_FallsBack) {
    // Primary returns a permanent error (KernelLaunchFailed).
    g_primaryReturnCode = static_cast<int>(AccelerationErrorCode::KernelLaunchFailed);

    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy(3));

    float distances[1] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 1, 1, nullptr);

    EXPECT_EQ(rc, 0);                         // fallback succeeded
    EXPECT_EQ(g_primaryCallCount,  1);        // only 1 attempt (no retry)
    EXPECT_EQ(g_fallbackCallCount, 1);
    EXPECT_NEAR(distances[0], 9.9f, 1e-4f);  // fallback stub value
}

// =============================================================================
// ANNKernelFallbackDispatcher — resolvedDispatch()
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_ResolvedDispatch_NullSlotReplaced) {
    ANNKernelDispatch primary;               // all nullptr
    primary.launchL2Distance = stubPrimaryL2; // only L2 is set

    ANNKernelDispatch fallback = makeFallbackANN();

    ANNKernelFallbackDispatcher disp(primary, fallback);
    ANNKernelDispatch resolved = disp.resolvedDispatch();

    EXPECT_EQ(resolved.launchL2Distance,   stubPrimaryL2);   // primary slot kept
    EXPECT_EQ(resolved.launchCosine,       stubFallbackL2);  // null → fallback
    EXPECT_EQ(resolved.launchInnerProduct, stubFallbackL2);  // null → fallback
    EXPECT_EQ(resolved.launchTopK,         stubFallbackTopK);// null → fallback
}

TEST_F(KernelFallbackDispatcherTest, ANN_ResolvedDispatch_AllPrimaryKept) {
    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();

    ANNKernelFallbackDispatcher disp(primary, fallback);
    ANNKernelDispatch resolved = disp.resolvedDispatch();

    EXPECT_EQ(resolved.launchL2Distance,   primary.launchL2Distance);
    EXPECT_EQ(resolved.launchCosine,       primary.launchCosine);
    EXPECT_EQ(resolved.launchInnerProduct, primary.launchInnerProduct);
    EXPECT_EQ(resolved.launchTopK,         primary.launchTopK);
}

TEST_F(KernelFallbackDispatcherTest, ANN_ResolvedDispatch_AllNullWhenBothEmpty) {
    ANNKernelDispatch empty;
    ANNKernelFallbackDispatcher disp(empty, empty);
    ANNKernelDispatch resolved = disp.resolvedDispatch();

    EXPECT_EQ(resolved.launchL2Distance,   nullptr);
    EXPECT_EQ(resolved.launchCosine,       nullptr);
    EXPECT_EQ(resolved.launchInnerProduct, nullptr);
    EXPECT_EQ(resolved.launchTopK,         nullptr);
}

// =============================================================================
// ANNKernelFallbackDispatcher — TopK slot
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_TopK_PrimarySucceeds) {
    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    uint32_t idx[2] = {};
    float    dists[2] = {};
    int rc = disp.launchTopK(nullptr, idx, dists, 1, 4, 2, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  1);
    EXPECT_EQ(g_fallbackCallCount, 0);
}

TEST_F(KernelFallbackDispatcherTest, ANN_TopK_NullPrimary_UsesFallback) {
    ANNKernelDispatch primary;  // all nullptr
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    uint32_t idx[1] = {0};
    float    dists[1] = {0.f};
    int rc = disp.launchTopK(nullptr, idx, dists, 1, 4, 1, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_fallbackCallCount, 1);
    EXPECT_EQ(idx[0], 99u);  // fallback stub value
}

// =============================================================================
// ANNKernelFallbackDispatcher — maxAttempts = 1 (no retry)
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, ANN_MaxAttempts1_NoRetry_FallsBackImmediately) {
    g_primaryReturnCode = static_cast<int>(AccelerationErrorCode::DeviceLost);

    ANNKernelDispatch primary  = makePrimaryANN();
    ANNKernelDispatch fallback = makeFallbackANN();
    ANNKernelFallbackDispatcher disp(primary, fallback, fastPolicy(1));

    float distances[1] = {};
    int rc = disp.launchL2Distance(nullptr, nullptr, distances, 1, 1, 1, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  1);  // only 1 attempt
    EXPECT_EQ(g_fallbackCallCount, 1);
}

// =============================================================================
// GeoKernelFallbackDispatcher — null-slot fallback
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, Geo_NullPrimaryDistance_UsesFallback) {
    GeoKernelDispatch primary;   // all nullptr
    GeoKernelDispatch fallback = makeFallbackGeo();
    GeoKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    float out[1] = {};
    int rc = disp.launchDistance(nullptr, nullptr, nullptr, nullptr, out, 1,
                                 GeoDistanceFormula::HAVERSINE, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  0);
    EXPECT_EQ(g_fallbackCallCount, 1);
    EXPECT_NEAR(out[0], 999.f, 1e-3f);
}

TEST_F(KernelFallbackDispatcherTest, Geo_NullPrimaryContainment_UsesFallback) {
    GeoKernelDispatch primary;
    GeoKernelDispatch fallback = makeFallbackGeo();
    GeoKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    uint8_t results[2] = {1, 1};
    int rc = disp.launchContainment(nullptr, nullptr, 2, nullptr, 4,
                                    results, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_fallbackCallCount, 1);
    EXPECT_EQ(results[0], 0u);  // fallback stub writes 0
    EXPECT_EQ(results[1], 0u);
}

TEST_F(KernelFallbackDispatcherTest, Geo_PrimaryDistanceSucceeds_NoFallback) {
    GeoKernelDispatch primary  = makePrimaryGeo();
    GeoKernelDispatch fallback = makeFallbackGeo();
    GeoKernelFallbackDispatcher disp(primary, fallback, fastPolicy());

    float out[1] = {};
    int rc = disp.launchDistance(nullptr, nullptr, nullptr, nullptr, out, 1,
                                 GeoDistanceFormula::HAVERSINE, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  1);
    EXPECT_EQ(g_fallbackCallCount, 0);
    EXPECT_NEAR(out[0], 1.f, 1e-4f);  // primary stub value
}

// =============================================================================
// GeoKernelFallbackDispatcher — transient retry
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, Geo_TransientDistance_RetriesThenFallsBack) {
    g_primaryReturnCode = static_cast<int>(AccelerationErrorCode::OperationTimeout);

    GeoKernelDispatch primary  = makePrimaryGeo();
    GeoKernelDispatch fallback = makeFallbackGeo();
    GeoKernelFallbackDispatcher disp(primary, fallback, fastPolicy(2));

    float out[1] = {};
    int rc = disp.launchDistance(nullptr, nullptr, nullptr, nullptr, out, 1,
                                 GeoDistanceFormula::HAVERSINE, nullptr);

    EXPECT_EQ(rc, 0);
    EXPECT_EQ(g_primaryCallCount,  2);  // 2 attempts before fallback
    EXPECT_EQ(g_fallbackCallCount, 1);
    EXPECT_NEAR(out[0], 999.f, 1e-3f);  // fallback stub value
}

// =============================================================================
// GeoKernelFallbackDispatcher — resolvedDispatch()
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, Geo_ResolvedDispatch_NullSlotReplaced) {
    GeoKernelDispatch primary;
    primary.launchDistance = stubPrimaryGeoDist;  // only distance set

    GeoKernelDispatch fallback = makeFallbackGeo();

    GeoKernelFallbackDispatcher disp(primary, fallback);
    GeoKernelDispatch resolved = disp.resolvedDispatch();

    EXPECT_EQ(resolved.launchDistance,    stubPrimaryGeoDist);      // primary kept
    EXPECT_EQ(resolved.launchContainment, stubFallbackGeoContain);  // null → fallback
}

TEST_F(KernelFallbackDispatcherTest, Geo_ResolvedDispatch_AllNullWhenBothEmpty) {
    GeoKernelDispatch empty;
    GeoKernelFallbackDispatcher disp(empty, empty);
    GeoKernelDispatch resolved = disp.resolvedDispatch();

    EXPECT_EQ(resolved.launchDistance,    nullptr);
    EXPECT_EQ(resolved.launchContainment, nullptr);
}

// =============================================================================
// Integration: CPU backend dispatch tables work as fallback
// =============================================================================

TEST_F(KernelFallbackDispatcherTest, Integration_CPUFallback_ANN) {
    CPUVectorBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    ANNKernelDispatch cpuTable = cpu.populateANNDispatch();

    // Use an empty primary (all null) so all calls route to the CPU.
    ANNKernelDispatch emptyPrimary;
    ANNKernelFallbackDispatcher disp(emptyPrimary, cpuTable, fastPolicy());

    // Query [1,0,0] vs vectors [1,0,0] and [0,1,0]; expected L2² = 0, 2
    const float queries[] = {1.f, 0.f, 0.f};
    const float vectors[] = {1.f, 0.f, 0.f,   0.f, 1.f, 0.f};
    float distances[2]    = {};

    int rc = disp.launchL2Distance(queries, vectors, distances,
                                   /*nq=*/1, /*nv=*/2, /*dim=*/3, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(distances[0], 0.f, 1e-5f);
    EXPECT_NEAR(distances[1], 2.f, 1e-5f);

    cpu.shutdown();
}

TEST_F(KernelFallbackDispatcherTest, Integration_CPUFallback_Geo) {
    CPUGeoBackend cpu;
    ASSERT_TRUE(cpu.initialize());
    GeoKernelDispatch cpuTable = cpu.populateGeoDispatch();

    GeoKernelDispatch emptyPrimary;
    GeoKernelFallbackDispatcher disp(emptyPrimary, cpuTable, fastPolicy());

    // Paris (48.8566, 2.3522) to London (51.5074, -0.1278) ≈ 340 km
    const double lats1[] = {48.8566};
    const double lons1[] = {2.3522};
    const double lats2[] = {51.5074};
    const double lons2[] = {-0.1278};
    float dist = 0.f;

    int rc = disp.launchDistance(lats1, lons1, lats2, lons2, &dist, 1,
                                 GeoDistanceFormula::HAVERSINE, nullptr);
    EXPECT_EQ(rc, 0);
    EXPECT_NEAR(dist, 340.f, 10.f);

    cpu.shutdown();
}
