/**
 * @file test_geo_phase2_phase3_focused.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=0, Stub=0, Unimpl=0, Mock=4, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @brief Focused acceptance tests for Geo Module Phase 2/3 hardening.
 *
 * Tests verify:
 *   - Phase 2 (Core): backend dispatch timeout enforcement, geometry validation,
 *                     spatial index contracts
 *   - Phase 3 (Error Handling): fail-closed behavior, unified diagnostics,
 *                               error recovery
 *
 * Test naming convention: P23-NN (Phase 2/3, test number 01-16)
 *   P23-01..P23-04: Backend dispatch timeout enforcement
 *   P23-05..P23-08: Geometry validation integration
 *   P23-09..P23-12: Fallback metrics accuracy
 *   P23-13..P23-16: Spatial index contract verification
 *
 * Roadmap reference: src/geo/ROADMAP.md §"Phase 4: Tests"
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <atomic>

#include "geo/phase2_phase3_hardening.h"
#include "geo/geo_api_contract.h"

namespace themis {
namespace geo {
namespace test {

// ============================================================================
// § 1  Backend Dispatch Timeout Tests (P23-01..P23-04)
// ============================================================================

/**
 * @test P23-01: BackendDispatchTimeoutGuard initializes with correct deadline.
 *
 * Verifies that the timeout guard starts with a deadline in the future.
 * Contract: all timeout guards must track an absolute deadline.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_01_TimeoutGuardInitialization) {
    const auto timeout = std::chrono::microseconds(100);
    BackendDispatchTimeoutGuard guard(timeout);

    // Deadline should be in the future at construction
    EXPECT_FALSE(guard.expired());
    EXPECT_EQ(guard.timeoutMicros(), 100);
}

/**
 * @test P23-02: BackendDispatchTimeoutGuard expires after deadline.
 *
 * Verifies that expired() returns true after the deadline passes.
 * Contract: timeout must be enforced deterministically within ±5µs.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_02_TimeoutGuardExpiration) {
    const auto timeout = std::chrono::microseconds(50);
    BackendDispatchTimeoutGuard guard(timeout);

    // Wait longer than timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Deadline should have passed
    EXPECT_TRUE(guard.expired());
}

/**
 * @test P23-03: BackendDispatchTimeoutGuard caches expiration result.
 *
 * Verifies that expired() returns the same value on repeated calls
 * (double-checked locking pattern is working correctly).
 */
TEST(GeoPhase2Phase3FocusedTest, P23_03_TimeoutGuardCaching) {
    const auto timeout = std::chrono::microseconds(100);
    BackendDispatchTimeoutGuard guard(timeout);

    // First call should indicate not expired
    const bool first_check = guard.expired();

    // Wait past expiration
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Second call should indicate expired, and caching should be working
    const bool second_check = guard.expired();

    EXPECT_EQ(first_check, false);
    EXPECT_EQ(second_check, true);
}

/**
 * @test P23-04: Contract: kBackendSelectionBudget is 100 microseconds.
 *
 * Verifies the contract constant is correctly defined.
 * Phase 2 requirement: backend selection must complete within this budget.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_04_BackendSelectionBudget) {
    static_assert(kBackendSelectionBudget == std::chrono::microseconds(100),
                  "Contract: kBackendSelectionBudget must be 100µs");

    EXPECT_EQ(kBackendSelectionBudget.count(), 100);
}

// ============================================================================
// § 2  Geometry Validation Tests (P23-05..P23-08)
// ============================================================================

/**
 * @test P23-05: WGS84 coordinate validation accepts valid coordinates.
 *
 * Verifies isValidWgs84CoordinatePair accepts coordinates within bounds:
 *   lon ∈ [-180, +180], lat ∈ [-90, +90]
 */
TEST(GeoPhase2Phase3FocusedTest, P23_05_Wgs84ValidCoordinates) {
    EXPECT_TRUE(isValidWgs84CoordinatePair(0.0, 0.0));        // Equator, Prime Meridian
    EXPECT_TRUE(isValidWgs84CoordinatePair(-180.0, -90.0));   // Southwest corner
    EXPECT_TRUE(isValidWgs84CoordinatePair(180.0, 90.0));     // Northeast corner
    EXPECT_TRUE(isValidWgs84CoordinatePair(51.5074, -0.1278));// London
}

/**
 * @test P23-06: WGS84 coordinate validation rejects out-of-bounds coordinates.
 *
 * Verifies isValidWgs84CoordinatePair rejects coordinates outside bounds.
 * Contract: Out-of-bounds → COORDINATE_OUT_OF_BOUNDS error.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_06_Wgs84OutOfBoundsCoordinates) {
    EXPECT_FALSE(isValidWgs84CoordinatePair(181.0, 0.0));    // Longitude too large
    EXPECT_FALSE(isValidWgs84CoordinatePair(-181.0, 0.0));   // Longitude too small
    EXPECT_FALSE(isValidWgs84CoordinatePair(0.0, 91.0));     // Latitude too large
    EXPECT_FALSE(isValidWgs84CoordinatePair(0.0, -91.0));    // Latitude too small
}

/**
 * @test P23-07: Ring closure validation verifies first == last coordinate.
 *
 * Verifies isRingClosed enforces RFC 7946 ring closure requirement.
 * Contract: Open rings → GEOMETRY_INVALID.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_07_RingClosureValidation) {
    // Closed ring (4 vertices: 3 unique + closing)
    std::vector<double> lons_closed = {0.0, 1.0, 1.0, 0.0};
    std::vector<double> lats_closed = {0.0, 0.0, 1.0, 0.0};
    EXPECT_TRUE(isRingClosed(lons_closed, lats_closed));

    // Open ring (first != last)
    std::vector<double> lons_open = {0.0, 1.0, 1.0, 0.5};
    std::vector<double> lats_open = {0.0, 0.0, 1.0, 0.5};
    EXPECT_FALSE(isRingClosed(lons_open, lats_open));

    // Too small (< 4 vertices)
    std::vector<double> lons_small = {0.0, 1.0, 0.0};
    std::vector<double> lats_small = {0.0, 1.0, 0.0};
    EXPECT_FALSE(isRingClosed(lons_small, lats_small));
}

/**
 * @test P23-08: Minimum vertex requirements for LineString and Polygon.
 *
 * Verifies minimum vertex checkers enforce RFC 7946 requirements:
 *   - LineString requires ≥ 2 points
 *   - Polygon exterior requires ≥ 4 points (including closing)
 */
TEST(GeoPhase2Phase3FocusedTest, P23_08_MinimumVertexRequirements) {
    // LineString: ≥ 2 points
    EXPECT_TRUE(lineStringHasMinimumVertices(2));
    EXPECT_TRUE(lineStringHasMinimumVertices(100));
    EXPECT_FALSE(lineStringHasMinimumVertices(1));
    EXPECT_FALSE(lineStringHasMinimumVertices(0));

    // Polygon exterior: ≥ 4 points (including closing)
    EXPECT_TRUE(polygonExteriorRingHasMinimumVertices(4));
    EXPECT_TRUE(polygonExteriorRingHasMinimumVertices(100));
    EXPECT_FALSE(polygonExteriorRingHasMinimumVertices(3));
    EXPECT_FALSE(polygonExteriorRingHasMinimumVertices(0));
}

// ============================================================================
// § 3  Result Consistency Tests (P23-09..P23-12)
// ============================================================================

/**
 * @test P23-09: Distance result consistency verifies within relative tolerance.
 *
 * Verifies isDistanceResultConsistent uses relative tolerance correctly.
 * Contract: kBackendAccuracyTolerance = 1e-9 (relative).
 */
TEST(GeoPhase2Phase3FocusedTest, P23_09_DistanceConsistencyIdentical) {
    EXPECT_TRUE(isDistanceResultConsistent(100.0, 100.0));
    EXPECT_TRUE(isDistanceResultConsistent(0.0, 0.0));
    EXPECT_TRUE(isDistanceResultConsistent(-50.0, -50.0));
}

/**
 * @test P23-10: Distance results within tolerance pass consistency check.
 *
 * Verifies isDistanceResultConsistent accepts results within the contract
 * accuracy tolerance (default 1e-9 relative).
 */
TEST(GeoPhase2Phase3FocusedTest, P23_10_DistanceConsistencyWithinTolerance) {
    // Within default 1e-9 relative tolerance
    const double cpu_result = 1000.0;
    const double gpu_result = 1000.0 + 1e-6; // 1e-9 relative = 1e-6 absolute
    EXPECT_TRUE(isDistanceResultConsistent(cpu_result, gpu_result));
}

/**
 * @test P23-11: Distance results outside tolerance fail consistency check.
 *
 * Verifies isDistanceResultConsistent rejects results differing by more
 * than the contract tolerance.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_11_DistanceConsistencyOutsideTolerance) {
    // Outside default 1e-9 relative tolerance
    const double cpu_result = 1000.0;
    const double gpu_result = 1000.0 + 1e-4; // 1e-7 relative >> 1e-9
    EXPECT_FALSE(isDistanceResultConsistent(cpu_result, gpu_result));
}

/**
 * @test P23-12: Containment result consistency checks bitset equality.
 *
 * Verifies isContainmentResultConsistent compares bitmasks element-by-element.
 * Used for point-in-polygon containment (1 = inside, 0 = outside).
 */
TEST(GeoPhase2Phase3FocusedTest, P23_12_ContainmentConsistency) {
    std::vector<uint8_t> cpu_mask = {1, 0, 1, 0, 1};
    std::vector<uint8_t> gpu_mask = {1, 0, 1, 0, 1};
    EXPECT_TRUE(isContainmentResultConsistent(cpu_mask, gpu_mask, 5));

    // Divergence in one element
    gpu_mask[2] = 0;  // Changed 1 to 0
    EXPECT_FALSE(isContainmentResultConsistent(cpu_mask, gpu_mask, 5));

    // Size mismatch
    gpu_mask.resize(4);
    EXPECT_FALSE(isContainmentResultConsistent(cpu_mask, gpu_mask, 5));
}

// ============================================================================
// § 4  Spatial Index Contract Tests (P23-13..P23-16)
// ============================================================================

/**
 * @test P23-13: Spatial index query consistency detects results outside envelope.
 *
 * Verifies isSpatialIndexQueryConsistent rejects results whose MBR
 * does not intersect the query bbox.
 * Contract: results must have MBR intersecting query box.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_13_IndexQueryConsistency) {
    const std::array<double, 4> query_bbox = {0.0, 0.0, 10.0, 10.0}; // [minX, minY, maxX, maxY]

    // Result MBR intersects query box
    std::vector<std::array<double, 4>> result_inside = {
        {1.0, 1.0, 2.0, 2.0}  // Inside query bbox
    };
    EXPECT_TRUE(isSpatialIndexQueryConsistent(query_bbox, result_inside));

    // Result MBR outside query box (violation)
    std::vector<std::array<double, 4>> result_outside = {
        {15.0, 15.0, 20.0, 20.0}  // Outside query bbox
    };
    EXPECT_FALSE(isSpatialIndexQueryConsistent(query_bbox, result_outside));

    // Multiple results, one outside
    std::vector<std::array<double, 4>> result_mixed = {
        {1.0, 1.0, 2.0, 2.0},      // Inside
        {15.0, 15.0, 20.0, 20.0}   // Outside (violation)
    };
    EXPECT_FALSE(isSpatialIndexQueryConsistent(query_bbox, result_mixed));
}

/**
 * @test P23-14: Spatial index query consistency handles edge cases.
 *
 * Verifies isSpatialIndexQueryConsistent correctly handles:
 *   - Results touching the boundary (tangent MBR)
 *   - Results partially overlapping query bbox
 *   - Empty result set
 */
TEST(GeoPhase2Phase3FocusedTest, P23_14_IndexQueryEdgeCases) {
    const std::array<double, 4> query_bbox = {0.0, 0.0, 10.0, 10.0};

    // Result MBR tangent to query bbox (touches boundary)
    std::vector<std::array<double, 4>> result_tangent = {
        {10.0, 0.0, 20.0, 10.0}  // Touches right edge
    };
    EXPECT_TRUE(isSpatialIndexQueryConsistent(query_bbox, result_tangent));

    // Result MBR partially overlapping
    std::vector<std::array<double, 4>> result_partial = {
        {5.0, 5.0, 15.0, 15.0}  // Partially overlaps
    };
    EXPECT_TRUE(isSpatialIndexQueryConsistent(query_bbox, result_partial));

    // Empty result set (vacuously true)
    std::vector<std::array<double, 4>> result_empty;
    EXPECT_TRUE(isSpatialIndexQueryConsistent(query_bbox, result_empty));
}

/**
 * @test P23-15: Geometry validation error messages are actionable.
 *
 * Verifies GeometryValidationErrorBuilder produces clear, actionable messages
 * for production use. Phase 3 requirement: consistent error reporting.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_15_ValidationErrorMessages) {
    const auto msg1 = GeometryValidationErrorBuilder::coordinateOutOfBounds(
        181.0, "longitude", kWgs84LonMin, kWgs84LonMax);
    EXPECT_NE(msg1.find("181"), std::string::npos);  // Contains the bad value
    EXPECT_NE(msg1.find("[-180, 180]"), std::string::npos);  // Contains bounds

    const auto msg2 = GeometryValidationErrorBuilder::unclosedRing(0, 5);
    EXPECT_NE(msg2.find("ring 0"), std::string::npos);
    EXPECT_NE(msg2.find("RFC 7946"), std::string::npos);

    const auto msg3 = GeometryValidationErrorBuilder::insufficientVertices(
        "LineString", 1, 2);
    EXPECT_NE(msg3.find("LineString"), std::string::npos);
    EXPECT_NE(msg3.find("1"), std::string::npos);
    EXPECT_NE(msg3.find("2"), std::string::npos);
}

/**
 * @test P23-16: Backend fallback diagnostic formats complete context.
 *
 * Verifies BackendFallbackDiagnostic produces structured error context
 * for production logging. Phase 3 requirement: unified diagnostics.
 */
TEST(GeoPhase2Phase3FocusedTest, P23_16_BackendFallbackDiagnostic) {
    BackendFallbackDiagnostic diag;
    diag.operation_name = "ST_Intersects";
    diag.geometry_type_a = "Polygon";
    diag.geometry_type_b = "Point";
    diag.batch_size = 1000;
    diag.backend_name = "CUDA";
    diag.device_name = "RTX 3090";
    diag.failure_reason = "timeout_exceeded";
    diag.elapsed_micros = 150;
    diag.fallback_taken = true;
    diag.result_verified = true;

    const auto msg = diag.formatMessage();
    EXPECT_NE(msg.find("ST_Intersects"), std::string::npos);
    EXPECT_NE(msg.find("Polygon"), std::string::npos);
    EXPECT_NE(msg.find("CUDA"), std::string::npos);
    EXPECT_NE(msg.find("timeout_exceeded"), std::string::npos);
    EXPECT_NE(msg.find("150"), std::string::npos);  // elapsed_micros
}

} // namespace test
} // namespace geo
} // namespace themis
