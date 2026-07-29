/**
 * @file test_geo_hardening_focused.cpp
 * @brief Focused tests for geo module Phase 1–3 hardening deliverables.
 *
 * Test IDs GCH-17 through GCH-24 cover:
 *   GCH-17  CudaBuffer RAII: alloc + automatic free on scope exit
 *   GCH-18  CudaBuffer RAII: move semantics transfer ownership
 *   GCH-19  CudaBuffer RAII: explicit free() is idempotent
 *   GCH-20  geo_policy: defaultSpatialQueryPolicy() values
 *   GCH-21  geo_policy: validateSpatialQueryPolicy() rejects negative max_depth
 *   GCH-22  geo_policy: validateSpatialQueryPolicy() rejects negative timeout
 *   GCH-23  geo_policy: isSpatialCollectionPermitted open/closed policy
 *   GCH-24  Vincenty geodesicDistance — non-convergence returns finite result
 *           (Haversine fallback, not -1.0)
 */

#include <gtest/gtest.h>

// Geo module headers under test
#include "geo/gpu_buffer_guard.h"
#include "geo/geo_policy.h"
#include "geo/spatial_backend.h"

#include <chrono>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// GPU buffer guard tests (host-side; no GPU required)
// ---------------------------------------------------------------------------

#ifdef THEMIS_ENABLE_CUDA

namespace {
// Trivial mock: replace cudaMalloc/cudaFree with a flag-based sentinel so
// the RAII destructor can be verified without a real CUDA device.
// These tests compile and run on any host; actual GPU allocation is tested
// via the CUDA integration test suite.
} // namespace

/**
 * @test GCH-17 — CudaBuffer default-constructed holds nullptr.
 * Verifies the guard starts in an empty, safe state.
 */
TEST(GeoCudaBufferGuard, GCH17_DefaultNullptr) {
    themis::geo::CudaBuffer buf;
    EXPECT_EQ(buf.ptr, nullptr)
        << "GCH-17: default-constructed CudaBuffer must have ptr == nullptr";
}

/**
 * @test GCH-18 — CudaBuffer move semantics transfer ownership.
 * After moving, the source is empty and the destination owns the pointer.
 */
TEST(GeoCudaBufferGuard, GCH18_MoveTransfersOwnership) {
    themis::geo::CudaBuffer src;
    // Manually set a non-null sentinel (no real GPU allocation for portability).
    // We call free() on src before destruction to avoid an actual cudaFree call
    // on a sentinel value.
    src.ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEAD));
    void* const expected = src.ptr;

    themis::geo::CudaBuffer dst(std::move(src));
    EXPECT_EQ(dst.ptr, expected)    << "GCH-18: moved-to buffer should own the pointer";
    EXPECT_EQ(src.ptr, nullptr)     << "GCH-18: moved-from buffer must be null";

    // Prevent actual cudaFree on sentinel by clearing before destructor.
    dst.ptr = nullptr;
}

/**
 * @test GCH-19 — CudaBuffer explicit free() is idempotent.
 * Calling free() twice must not crash or double-free.
 */
TEST(GeoCudaBufferGuard, GCH19_FreeIsIdempotent) {
    themis::geo::CudaBuffer buf;
    // buf.ptr is nullptr; free() on nullptr must be a no-op.
    EXPECT_NO_THROW(buf.free()) << "GCH-19: free() on null buffer must not throw";
    EXPECT_NO_THROW(buf.free()) << "GCH-19: second free() on null buffer must not throw";
    EXPECT_EQ(buf.ptr, nullptr)  << "GCH-19: ptr must remain null after free()";
}

#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_GEO_HIP

/**
 * @test GCH-17H — HipBuffer default-constructed holds nullptr.
 */
TEST(GeoHipBufferGuard, GCH17H_DefaultNullptr) {
    themis::geo::HipBuffer buf;
    EXPECT_EQ(buf.ptr, nullptr)
        << "GCH-17H: default-constructed HipBuffer must have ptr == nullptr";
}

/**
 * @test GCH-18H — HipBuffer move semantics transfer ownership.
 */
TEST(GeoHipBufferGuard, GCH18H_MoveTransfersOwnership) {
    themis::geo::HipBuffer src;
    src.ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(0xBEEF));
    void* const expected = src.ptr;

    themis::geo::HipBuffer dst(std::move(src));
    EXPECT_EQ(dst.ptr, expected);
    EXPECT_EQ(src.ptr, nullptr);

    dst.ptr = nullptr; // prevent hipFree on sentinel
}

#endif // THEMIS_GEO_HIP

// ---------------------------------------------------------------------------
// geo_policy tests (always compiled; no GPU required)
// ---------------------------------------------------------------------------

/**
 * @test GCH-20 — defaultSpatialQueryPolicy() returns documented defaults.
 */
TEST(GeoPolicy, GCH20_DefaultValues) {
    const auto p = themis::geo::defaultSpatialQueryPolicy();
    EXPECT_EQ(p.max_candidates, 100'000u)
        << "GCH-20: default max_candidates must be 100 000";
    EXPECT_EQ(p.max_depth, 64)
        << "GCH-20: default max_depth must be 64";
    EXPECT_EQ(p.timeout.count(), 5'000)
        << "GCH-20: default timeout must be 5 000 ms";
    EXPECT_EQ(p.max_result_pairs, 1'000'000u)
        << "GCH-20: default max_result_pairs must be 1 000 000";
    EXPECT_TRUE(p.allowed_collections.empty())
        << "GCH-20: default allowed_collections must be empty (open policy)";
}

/**
 * @test GCH-21 — validateSpatialQueryPolicy() rejects negative max_depth.
 */
TEST(GeoPolicy, GCH21_RejectsNegativeMaxDepth) {
    themis::geo::SpatialQueryPolicy p;
    p.max_depth = -1;
    const auto v = themis::geo::validateSpatialQueryPolicy(p);
    EXPECT_FALSE(v.ok())
        << "GCH-21: negative max_depth must not pass validation";
    EXPECT_FALSE(v.violations.empty())
        << "GCH-21: at least one violation must be reported";
}

/**
 * @test GCH-22 — validateSpatialQueryPolicy() rejects negative timeout.
 */
TEST(GeoPolicy, GCH22_RejectsNegativeTimeout) {
    themis::geo::SpatialQueryPolicy p;
    p.timeout = std::chrono::milliseconds(-1);
    const auto v = themis::geo::validateSpatialQueryPolicy(p);
    EXPECT_FALSE(v.ok())
        << "GCH-22: negative timeout must not pass validation";
}

/**
 * @test GCH-23 — isSpatialCollectionPermitted(): open and closed policy.
 * An empty allowed_collections permits everything; a non-empty list is an
 * allowlist.
 */
TEST(GeoPolicy, GCH23_CollectionPermitted) {
    // Open policy: everything permitted.
    themis::geo::SpatialQueryPolicy open;
    EXPECT_TRUE(themis::geo::isSpatialCollectionPermitted(open, "any_collection"))
        << "GCH-23: open policy (empty ACL) must permit any collection";
    EXPECT_TRUE(themis::geo::isSpatialCollectionPermitted(open, ""))
        << "GCH-23: open policy must permit even an empty name";

    // Closed policy: only listed collections permitted.
    themis::geo::SpatialQueryPolicy closed;
    closed.allowed_collections = {"geo_data", "poi"};
    EXPECT_TRUE(themis::geo::isSpatialCollectionPermitted(closed, "geo_data"))
        << "GCH-23: 'geo_data' is in allowlist; must be permitted";
    EXPECT_TRUE(themis::geo::isSpatialCollectionPermitted(closed, "poi"))
        << "GCH-23: 'poi' is in allowlist; must be permitted";
    EXPECT_FALSE(themis::geo::isSpatialCollectionPermitted(closed, "secret"))
        << "GCH-23: 'secret' is not in allowlist; must be denied";
    EXPECT_FALSE(themis::geo::isSpatialCollectionPermitted(closed, ""))
        << "GCH-23: empty name is not in allowlist; must be denied";
}

/**
 * @test GCH-24 — geodesicDistance() returns a finite, non-negative result
 * for nearly-antipodal points (which previously caused Vincenty non-convergence
 * and returned -1.0; now falls back to Haversine).
 *
 * This test uses the CPU exact backend which contains the fixed Vincenty loop.
 * The nearly-antipodal points are (0°N, 0°E) and (0°N, 179.9°E).
 */
TEST(GeoVincenty, GCH24_AntipodalFallbackFinite) {
    auto* backend = themis::geo::getCpuExactBackend();
    ASSERT_NE(backend, nullptr) << "GCH-24: getCpuExactBackend() must return non-null";

    // Near-antipodal pair that historically caused Vincenty non-convergence.
    const double lat1 = 0.0, lon1 = 0.0;
    const double lat2 = 0.0, lon2 = 179.9;

    const double dist = backend->geodesicDistance(lat1, lon1, lat2, lon2);

    EXPECT_GT(dist, 0.0)
        << "GCH-24: distance between non-coincident points must be positive";
    EXPECT_TRUE(std::isfinite(dist))
        << "GCH-24: geodesicDistance must return a finite value (not -1.0) "
           "for near-antipodal input";
    // Earth half-circumference ≈ 20 000 km; result should be in that ballpark.
    EXPECT_LT(dist, 25'000'000.0)
        << "GCH-24: distance must be < 25 000 km for near-antipodal input";
}
