// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_geo_q4_hardening_focused.cpp
 * @brief Geo module Q4 2026 Phase 2/3 hardening focused test suite (GEH-01..GEH-20).
 *
 * Verifies deterministic behavior across mixed backend/precision-mode permutations,
 * geometry boundary validation, complex join/raster edge cases, and operator
 * diagnostics.
 *
 * ## Test families
 *
 * ### GEH-01..04 — Precision-mode dispatch determinism
 *   GEH-01  STANDARD precision: CPU-only dispatches correctly
 *   GEH-02  HIGH precision: CPU-only dispatches within tolerance
 *   GEH-03  ULTRA precision: forces CPU path even when GPU is provided
 *   GEH-04  CPU_AND_GPU parity within STANDARD tolerance
 *
 * ### GEH-05..08 — Backend fallback behavior
 *   GEH-05  GPU unavailable → silent CPU fallback, no error returned
 *   GEH-06  GPU exception → fallback count incremented
 *   GEH-07  CPU_AND_GPU parity mismatch → drift counter incremented
 *   GEH-08  Persistent drift → hasCapabilityDrift() returns true
 *
 * ### GEH-09..12 — Coordinate boundary validation
 *   GEH-09  Valid WGS84 coordinates → OK
 *   GEH-10  Longitude out of bounds → COORDINATE_OUT_OF_BOUNDS
 *   GEH-11  Latitude out of bounds → COORDINATE_OUT_OF_BOUNDS
 *   GEH-12  NaN coordinate → GEOMETRY_INVALID
 *
 * ### GEH-13..16 — Ring validation edge cases
 *   GEH-13  Valid closed ring (4+ points) → OK
 *   GEH-14  Open ring (first ≠ last) → GEOMETRY_INVALID
 *   GEH-15  Ring with fewer than 4 points → GEOMETRY_INVALID
 *   GEH-16  Ring with invalid coordinate → COORDINATE_OUT_OF_BOUNDS
 *
 * ### GEH-17..20 — Operator diagnostics
 *   GEH-17  recordIncident stores incident with correct severity
 *   GEH-18  incidentsBySeverity filters correctly
 *   GEH-19  hasCriticalIncidents returns true only for CRITICAL incidents
 *   GEH-20  formatSummary produces non-empty output with incident details
 *
 * @see include/geo/geo_edge_case_handler.h
 * @see include/geo/geo_operator_diagnostics.h
 * @see src/geo/ROADMAP.md — Phase 2/3 Q4 2026 items
 */

#include <gtest/gtest.h>

#include "geo/geo_edge_case_handler.h"
#include "geo/geo_operator_diagnostics.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace themis {
namespace geo {
namespace test {

// ============================================================================
// § 1  Precision-Mode Dispatch Determinism (GEH-01..04)
// ============================================================================

/**
 * @test GEH-01: STANDARD precision CPU-only dispatch returns correct result.
 */
TEST(GeoQ4Hardening, GEH01_StandardPrecisionCpuOnly) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_ONLY,
                               GeoPrecisionMode::STANDARD);
    auto result = handler.dispatch([]() { return 42.0; });
    ASSERT_TRUE(result.ok()) << "Expected OK result from CPU-only dispatch";
    EXPECT_DOUBLE_EQ(result.value, 42.0);
    EXPECT_EQ(handler.cpuDispatchCount(), 1u);
}

/**
 * @test GEH-02: HIGH precision CPU-only dispatch returns value within tolerance.
 */
TEST(GeoQ4Hardening, GEH02_HighPrecisionCpuOnly) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_ONLY,
                               GeoPrecisionMode::HIGH);
    const double expected = 3.14159265358979;
    auto result = handler.dispatch([expected]() { return expected; });
    ASSERT_TRUE(result.ok());
    EXPECT_NEAR(result.value, expected, toleranceForMode(GeoPrecisionMode::HIGH));
}

/**
 * @test GEH-03: ULTRA precision forces CPU path regardless of GPU availability.
 *
 * Even though a GPU function is provided, ULTRA mode must never use the GPU path.
 */
TEST(GeoQ4Hardening, GEH03_UltraPrecisionForcesCpu) {
    bool gpu_called = false;
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::ULTRA);
    auto result = handler.dispatch(
        []() { return 1.0; },
        [&]() -> std::optional<double> {
            gpu_called = true;
            return 1.0;
        });
    ASSERT_TRUE(result.ok());
    EXPECT_FALSE(gpu_called) << "ULTRA precision must not invoke GPU path";
    EXPECT_EQ(result.value, 1.0);
}

/**
 * @test GEH-04: CPU_AND_GPU parity within STANDARD tolerance, no drift recorded.
 */
TEST(GeoQ4Hardening, GEH04_CpuGpuParityWithinTolerance) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::STANDARD);
    // CPU and GPU return results within tolerance.
    auto result = handler.dispatch(
        []() { return 100.0; },
        []() -> std::optional<double> { return 100.0 + 1e-8; }); // within 1e-6
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(handler.driftEventCount(), 0u);
    EXPECT_FALSE(handler.hasCapabilityDrift());
}

// ============================================================================
// § 2  Backend Fallback Behavior (GEH-05..08)
// ============================================================================

/**
 * @test GEH-05: GPU returning nullopt triggers silent CPU fallback.
 */
TEST(GeoQ4Hardening, GEH05_GpuUnavailableFallbackToCpu) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::STANDARD);
    auto result = handler.dispatch(
        []() { return 7.0; },
        []() -> std::optional<double> { return std::nullopt; });
    ASSERT_TRUE(result.ok());
    EXPECT_DOUBLE_EQ(result.value, 7.0);
    EXPECT_EQ(handler.fallbackCount(), 1u);
}

/**
 * @test GEH-06: GPU function throwing an exception increments fallback count.
 */
TEST(GeoQ4Hardening, GEH06_GpuExceptionIncrementssFallbackCount) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::STANDARD);
    auto result = handler.dispatch(
        []() { return 99.0; },
        []() -> std::optional<double> {
            throw std::runtime_error("GPU error");
        });
    ASSERT_TRUE(result.ok());
    EXPECT_DOUBLE_EQ(result.value, 99.0);
    EXPECT_GE(handler.fallbackCount(), 1u);
}

/**
 * @test GEH-07: CPU/GPU parity mismatch increments drift counter and returns CPU result.
 */
TEST(GeoQ4Hardening, GEH07_ParityMismatchIncrementsDriftCounter) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::STANDARD);
    // CPU=1.0, GPU=2.0 → difference >> 1e-6 relative to 1.0
    auto result = handler.dispatch(
        []() { return 1.0; },
        []() -> std::optional<double> { return 2.0; });
    ASSERT_TRUE(result.ok());
    EXPECT_DOUBLE_EQ(result.value, 1.0) << "CPU result is authoritative on parity mismatch";
    EXPECT_EQ(handler.driftEventCount(), 1u);
}

/**
 * @test GEH-08: After kDriftThreshold mismatches, hasCapabilityDrift() returns true.
 */
TEST(GeoQ4Hardening, GEH08_PersistentDriftFlagSetAfterThreshold) {
    GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
                               GeoPrecisionMode::STANDARD);
    // Trigger 5 drift events (kDriftThreshold).
    for (int i = 0; i < 5; ++i) {
        handler.dispatch(
            []() { return 1.0; },
            []() -> std::optional<double> { return 1000.0; });
    }
    EXPECT_TRUE(handler.hasCapabilityDrift());
    // After reset, drift should clear.
    handler.resetDriftCounter();
    EXPECT_FALSE(handler.hasCapabilityDrift());
}

// ============================================================================
// § 3  Coordinate Boundary Validation (GEH-09..12)
// ============================================================================

/**
 * @test GEH-09: Valid WGS84 coordinates return OK.
 */
TEST(GeoQ4Hardening, GEH09_ValidWgs84CoordinatesOk) {
    GeoEdgeCaseHandler handler;
    EXPECT_EQ(handler.validateCoordinateBoundary(0.0, 0.0), GeoErrorCode::OK);
    EXPECT_EQ(handler.validateCoordinateBoundary(-180.0, -90.0), GeoErrorCode::OK);
    EXPECT_EQ(handler.validateCoordinateBoundary(180.0, 90.0), GeoErrorCode::OK);
    EXPECT_EQ(handler.validateCoordinateBoundary(13.404954, 52.520008), GeoErrorCode::OK);
}

/**
 * @test GEH-10: Longitude out of WGS84 bounds returns COORDINATE_OUT_OF_BOUNDS.
 */
TEST(GeoQ4Hardening, GEH10_LongitudeOutOfBounds) {
    GeoEdgeCaseHandler handler;
    EXPECT_EQ(handler.validateCoordinateBoundary(181.0, 0.0),
              GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
    EXPECT_EQ(handler.validateCoordinateBoundary(-181.0, 0.0),
              GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
}

/**
 * @test GEH-11: Latitude out of WGS84 bounds returns COORDINATE_OUT_OF_BOUNDS.
 */
TEST(GeoQ4Hardening, GEH11_LatitudeOutOfBounds) {
    GeoEdgeCaseHandler handler;
    EXPECT_EQ(handler.validateCoordinateBoundary(0.0, 91.0),
              GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
    EXPECT_EQ(handler.validateCoordinateBoundary(0.0, -91.0),
              GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
}

/**
 * @test GEH-12: NaN coordinate returns GEOMETRY_INVALID.
 */
TEST(GeoQ4Hardening, GEH12_NanCoordinateInvalid) {
    GeoEdgeCaseHandler handler;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    EXPECT_EQ(handler.validateCoordinateBoundary(nan, 0.0), GeoErrorCode::GEOMETRY_INVALID);
    EXPECT_EQ(handler.validateCoordinateBoundary(0.0, nan), GeoErrorCode::GEOMETRY_INVALID);
    EXPECT_EQ(handler.validateCoordinateBoundary(inf, 0.0), GeoErrorCode::GEOMETRY_INVALID);
    EXPECT_EQ(handler.validateCoordinateBoundary(0.0, -inf), GeoErrorCode::GEOMETRY_INVALID);
}

// ============================================================================
// § 4  Ring Validation Edge Cases (GEH-13..16)
// ============================================================================

/**
 * @test GEH-13: Valid closed polygon ring returns OK.
 */
TEST(GeoQ4Hardening, GEH13_ValidClosedRingOk) {
    GeoEdgeCaseHandler handler;
    // Square: 4 points (3 unique + closing).
    std::vector<std::pair<double,double>> ring = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.0, 0.0}
    };
    EXPECT_EQ(handler.validateRing(ring), GeoErrorCode::OK);
}

/**
 * @test GEH-14: Open ring (first ≠ last) returns GEOMETRY_INVALID.
 */
TEST(GeoQ4Hardening, GEH14_OpenRingInvalid) {
    GeoEdgeCaseHandler handler;
    std::vector<std::pair<double,double>> ring = {
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, {0.5, 0.5} // not closed
    };
    EXPECT_EQ(handler.validateRing(ring), GeoErrorCode::GEOMETRY_INVALID);
}

/**
 * @test GEH-15: Ring with fewer than 4 points returns GEOMETRY_INVALID.
 */
TEST(GeoQ4Hardening, GEH15_ShortRingInvalid) {
    GeoEdgeCaseHandler handler;
    std::vector<std::pair<double,double>> ring = {
        {0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0} // only 3 points
    };
    EXPECT_EQ(handler.validateRing(ring), GeoErrorCode::GEOMETRY_INVALID);
}

/**
 * @test GEH-16: Ring with out-of-bounds coordinate returns COORDINATE_OUT_OF_BOUNDS.
 */
TEST(GeoQ4Hardening, GEH16_RingWithInvalidCoordinateReturnsError) {
    GeoEdgeCaseHandler handler;
    std::vector<std::pair<double,double>> ring = {
        {0.0, 0.0}, {200.0, 0.0}, {1.0, 1.0}, {0.0, 0.0} // lon=200 invalid
    };
    EXPECT_EQ(handler.validateRing(ring), GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
}

// ============================================================================
// § 5  Operator Diagnostics (GEH-17..20)
// ============================================================================

/**
 * @test GEH-17: recordIncident stores incident with correct fields.
 */
TEST(GeoQ4Hardening, GEH17_RecordIncidentStoresCorrectFields) {
    GeoOperatorDiagnostics diag;
    diag.recordIncident("GEO-TEST-001", GeoIncidentSeverity::WARNING,
                        "test description", "test remediation",
                        GeoErrorCode::BACKEND_UNAVAILABLE);
    EXPECT_EQ(diag.totalIncidentCount(), 1u);
    auto recent = diag.recentIncidents(5);
    ASSERT_EQ(recent.size(), 1u);
    EXPECT_EQ(recent[0].incident_id, "GEO-TEST-001");
    EXPECT_EQ(recent[0].severity, GeoIncidentSeverity::WARNING);
    EXPECT_EQ(recent[0].description, "test description");
    ASSERT_TRUE(recent[0].error_code.has_value());
    EXPECT_EQ(recent[0].error_code.value(), GeoErrorCode::BACKEND_UNAVAILABLE);
}

/**
 * @test GEH-18: incidentsBySeverity filters to >= min_severity.
 */
TEST(GeoQ4Hardening, GEH18_IncidentsBySeverityFiltersCorrectly) {
    GeoOperatorDiagnostics diag;
    diag.recordIncident("I1", GeoIncidentSeverity::INFO,    "info",    "r");
    diag.recordIncident("W1", GeoIncidentSeverity::WARNING, "warning", "r");
    diag.recordIncident("E1", GeoIncidentSeverity::ERROR,   "error",   "r");

    auto warnings_and_above = diag.incidentsBySeverity(GeoIncidentSeverity::WARNING);
    EXPECT_EQ(warnings_and_above.size(), 2u);
    for (const auto& inc : warnings_and_above) {
        EXPECT_GE(static_cast<uint8_t>(inc.severity),
                  static_cast<uint8_t>(GeoIncidentSeverity::WARNING));
    }
}

/**
 * @test GEH-19: hasCriticalIncidents returns true only when CRITICAL incidents exist.
 */
TEST(GeoQ4Hardening, GEH19_HasCriticalIncidentsCorrect) {
    GeoOperatorDiagnostics diag;
    EXPECT_FALSE(diag.hasCriticalIncidents());
    diag.recordIncident("C1", GeoIncidentSeverity::CRITICAL, "critical", "escalate");
    EXPECT_TRUE(diag.hasCriticalIncidents());
    diag.clearIncidents();
    EXPECT_FALSE(diag.hasCriticalIncidents());
}

/**
 * @test GEH-20: formatSummary produces non-empty output with incident details.
 */
TEST(GeoQ4Hardening, GEH20_FormatSummaryContainsIncidentDetails) {
    GeoOperatorDiagnostics diag;
    diag.recordIncident("GEO-ECH-DRIFT", GeoIncidentSeverity::WARNING,
                        "Backend drift detected", "Recalibrate GPU");
    auto summary = diag.formatSummary(5);
    EXPECT_FALSE(summary.empty());
    EXPECT_NE(summary.find("GEO-ECH-DRIFT"), std::string::npos)
        << "Summary must include the incident ID";
    EXPECT_NE(summary.find("WARNING"), std::string::npos)
        << "Summary must include severity";
}

} // namespace test
} // namespace geo
} // namespace themis
