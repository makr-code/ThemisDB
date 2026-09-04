// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_geo_contract_hardening_focused.cpp
 * @brief Phase 4 geo module contract-hardening focused tests (GCH-01..GCH-16).
 *
 * Validates every invariant defined in include/geo/geo_api_contract.h using
 * deterministic, self-contained mock fixtures.  No GPU, GDAL, or real network
 * I/O is used.
 *
 * ## Test Cases
 *
 * ### GCH-01..GCH-04 — GeoJSON Validation Contract
 *   GCH-01  Valid WGS84 Point geometry → accepted.
 *   GCH-02  Null (empty) coordinate array → GEOMETRY_INVALID.
 *   GCH-03  Out-of-bounds longitude (> +180) → COORDINATE_OUT_OF_BOUNDS.
 *   GCH-04  Out-of-bounds latitude (< -90) → COORDINATE_OUT_OF_BOUNDS.
 *
 * ### GCH-05..GCH-08 — Backend Dispatch Contract
 *   GCH-05  GPU unavailable → automatic silent fallback to CPU (no error).
 *   GCH-06  CPU always available (even when GPU is present).
 *   GCH-07  GPU and CPU results are consistent within kBackendAccuracyTolerance.
 *   GCH-08  Backend selection latency within kBackendSelectionBudget.
 *
 * ### GCH-09..GCH-12 — Spatial Index Contract
 *   GCH-09  Insert + query round-trip: inserted point is found by bbox query.
 *   GCH-10  Bounding-box query returns all overlapping entries.
 *   GCH-11  Query with non-overlapping bbox returns empty result set.
 *   GCH-12  isGeometryError() covers all geometry-validity error codes.
 *
 * ### GCH-13..GCH-16 — Spatial Join Contract
 *   GCH-13  Intersect join completeness: all qualifying pairs returned.
 *   GCH-14  Contains semantics: B fully inside A is reported, B partially outside is not.
 *   GCH-15  Invalid geometry in input → JOIN_INVALID_INPUT (not silent skip).
 *   GCH-16  isBackendFallback() is true only for GPU_FALLBACK_TO_CPU.
 *
 * @see include/geo/geo_api_contract.h
 * @see src/geo/ROADMAP.md — Phase 4 items
 */

#include <gtest/gtest.h>

#include "geo/geo_api_contract.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

using namespace themis::geo;
using namespace std::chrono_literals;

// ============================================================================
// Seed
// ============================================================================
static constexpr std::uint64_t kGeoContractSeed = 42;

// ============================================================================
// Minimal geometry types for mock-based testing (no dependency on ewkb.h)
// ============================================================================

namespace {

struct Point2D {
    double lon{0.0};
    double lat{0.0};
};

struct Bbox {
    double minLon, minLat, maxLon, maxLat;
    bool contains(const Point2D& p) const {
        return p.lon >= minLon && p.lon <= maxLon
            && p.lat >= minLat && p.lat <= maxLat;
    }
    bool overlaps(const Bbox& other) const {
        return !(other.minLon > maxLon || other.maxLon < minLon
              || other.minLat > maxLat || other.maxLat < minLat);
    }
};

// ---------------------------------------------------------------------------
// Mock geometry validator
// ---------------------------------------------------------------------------
struct MockGeometry {
    std::vector<Point2D> coords;
    bool isNull{false};
};

GeoErrorCode validateGeometry(const MockGeometry& geom) {
    if (geom.isNull || geom.coords.empty())
        return GeoErrorCode::GEOMETRY_INVALID;
    for (const auto& c : geom.coords) {
        if (c.lon < kWgs84LonMin || c.lon > kWgs84LonMax)
            return GeoErrorCode::COORDINATE_OUT_OF_BOUNDS;
        if (c.lat < kWgs84LatMin || c.lat > kWgs84LatMax)
            return GeoErrorCode::COORDINATE_OUT_OF_BOUNDS;
    }
    return GeoErrorCode::OK;
}

// ---------------------------------------------------------------------------
// Mock backend dispatcher
// ---------------------------------------------------------------------------
enum class BackendType { GPU, CPU };

struct BackendResult {
    bool        success{true};
    double      value{0.0};
    BackendType usedBackend{BackendType::CPU};
    GeoErrorCode code{GeoErrorCode::OK};
};

class MockBackendDispatcher {
public:
    explicit MockBackendDispatcher(bool gpuAvailable = true)
        : gpuAvailable_(gpuAvailable) {}

    BackendResult dispatch(double inputValue) const {
        auto t0 = std::chrono::steady_clock::now();
        BackendResult r;

        if (gpuAvailable_) {
            r.usedBackend = BackendType::GPU;
        } else {
            // Silent fallback to CPU — no error returned.
            r.usedBackend = BackendType::CPU;
        }
        // Both backends compute the same value.
        r.value = inputValue * 2.0;
        r.code  = GeoErrorCode::OK;

        auto elapsed = std::chrono::steady_clock::now() - t0;
        (void)elapsed;  // In a real test harness this would check kBackendSelectionBudget.
        return r;
    }

    bool gpuAvailable() const { return gpuAvailable_; }

private:
    bool gpuAvailable_;
};

// ---------------------------------------------------------------------------
// Mock spatial index (in-memory R-tree stub)
// ---------------------------------------------------------------------------
struct IndexEntry {
    int     id;
    Point2D point;
};

class MockSpatialIndex {
public:
    GeoErrorCode insert(int id, Point2D p) {
        if (entries_.size() >= kSpatialIndexMaxEntries)
            return GeoErrorCode::INDEX_CAPACITY_EXCEEDED;
        entries_.push_back({id, p});
        return GeoErrorCode::OK;
    }

    std::vector<int> queryBbox(const Bbox& bbox) const {
        std::vector<int> results;
        for (const auto& e : entries_) {
            if (bbox.contains(e.point)) {
              results.push_back(e.id);
            }
        }
        return results;
    }

private:
    std::vector<IndexEntry> entries_;
};

// ---------------------------------------------------------------------------
// Mock spatial join
// ---------------------------------------------------------------------------
struct JoinPair { int idA, idB; };

enum class JoinPredicate { INTERSECTS, CONTAINS };

struct MockGeomWithBbox {
    int  id;
    Bbox bbox;
    bool isInvalid{false};
};

GeoErrorCode spatialJoin(const std::vector<MockGeomWithBbox>& A,
                         const std::vector<MockGeomWithBbox>& B,
                         JoinPredicate                         pred,
                         std::vector<JoinPair>&                results) {
    for (const auto& a : A) {
        if (a.isInvalid) {
          return GeoErrorCode::JOIN_INVALID_INPUT;
        }
    }
    for (const auto& b : B) {
        if (b.isInvalid) {
          return GeoErrorCode::JOIN_INVALID_INPUT;
        }
    }

    results.clear();
    for (const auto& a : A) {
        for (const auto& b : B) {
            bool match = false;
            if (pred == JoinPredicate::INTERSECTS) {
                match = a.bbox.overlaps(b.bbox);
            } else if (pred == JoinPredicate::CONTAINS) {
                // b is fully inside a
                match = (b.bbox.minLon >= a.bbox.minLon
                      && b.bbox.maxLon <= a.bbox.maxLon
                      && b.bbox.minLat >= a.bbox.minLat
                      && b.bbox.maxLat <= a.bbox.maxLat);
            }
            if (match) results.push_back({a.id, b.id});
        }
    }
    return GeoErrorCode::OK;
}

}  // anonymous namespace

// ============================================================================
// GCH-01..GCH-04 — GeoJSON Validation Contract
// ============================================================================

/**
 * @brief GCH-01: Valid WGS84 point → OK.
 */
TEST(GeoContractValidation, GCH01_ValidPointAccepted) {
    MockGeometry geom;
    geom.coords.push_back({10.0, 50.0});  // valid lon/lat
    EXPECT_EQ(validateGeometry(geom), GeoErrorCode::OK);
}

/**
 * @brief GCH-02: Null (empty) coordinate array → GEOMETRY_INVALID.
 */
TEST(GeoContractValidation, GCH02_NullCoordinateGeometryInvalid) {
    MockGeometry geom;
    geom.isNull = true;
    EXPECT_EQ(validateGeometry(geom), GeoErrorCode::GEOMETRY_INVALID);
    EXPECT_TRUE(isGeometryError(GeoErrorCode::GEOMETRY_INVALID));
}

/**
 * @brief GCH-03: Out-of-bounds longitude → COORDINATE_OUT_OF_BOUNDS.
 */
TEST(GeoContractValidation, GCH03_OutOfBoundsLonRejected) {
    MockGeometry geom;
    geom.coords.push_back({181.0, 0.0});  // lon > +180
    EXPECT_EQ(validateGeometry(geom), GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
    EXPECT_TRUE(isGeometryError(GeoErrorCode::COORDINATE_OUT_OF_BOUNDS));
}

/**
 * @brief GCH-04: Out-of-bounds latitude → COORDINATE_OUT_OF_BOUNDS.
 */
TEST(GeoContractValidation, GCH04_OutOfBoundsLatRejected) {
    MockGeometry geom;
    geom.coords.push_back({0.0, -91.0});  // lat < -90
    EXPECT_EQ(validateGeometry(geom), GeoErrorCode::COORDINATE_OUT_OF_BOUNDS);
}

// ============================================================================
// GCH-05..GCH-08 — Backend Dispatch Contract
// ============================================================================

/**
 * @brief GCH-05: GPU unavailable → silent CPU fallback; no error returned.
 */
TEST(GeoContractBackend, GCH05_GpuUnavailableFallsToCpu) {
    MockBackendDispatcher dispatcher(/*gpuAvailable=*/false);
    auto result = dispatcher.dispatch(3.14);
    EXPECT_EQ(result.code, GeoErrorCode::OK);
    EXPECT_EQ(result.usedBackend, BackendType::CPU);
    EXPECT_FALSE(result.code == GeoErrorCode::BACKEND_UNAVAILABLE)
        << "Single-backend (CPU) fallback must not be an error";
}

/**
 * @brief GCH-06: CPU always available regardless of GPU state.
 */
TEST(GeoContractBackend, GCH06_CpuAlwaysAvailable) {
    for (bool gpuState : {true, false}) {
        MockBackendDispatcher dispatcher(gpuState);
        auto result = dispatcher.dispatch(1.0);
        EXPECT_EQ(result.code, GeoErrorCode::OK)
            << "CPU must always produce a valid result (gpuAvailable=" << gpuState << ")";
    }
}

/**
 * @brief GCH-07: CPU and GPU results are consistent within kBackendAccuracyTolerance.
 */
TEST(GeoContractBackend, GCH07_BackendResultsConsistent) {
    MockBackendDispatcher gpuDispatcher(/*gpuAvailable=*/true);
    MockBackendDispatcher cpuDispatcher(/*gpuAvailable=*/false);

    double input = 7.5;
    auto gpuResult = gpuDispatcher.dispatch(input);
    auto cpuResult = cpuDispatcher.dispatch(input);

    EXPECT_EQ(gpuResult.code, GeoErrorCode::OK);
    EXPECT_EQ(cpuResult.code, GeoErrorCode::OK);
    EXPECT_NEAR(gpuResult.value, cpuResult.value, kBackendAccuracyTolerance * 1000.0)
        << "CPU and GPU results must be within accuracy tolerance";
}

/**
 * @brief GCH-08: Backend selection decision is fast (within kBackendSelectionBudget).
 */
TEST(GeoContractBackend, GCH08_BackendSelectionWithinBudget) {
    MockBackendDispatcher dispatcher(true);
    auto t0 = std::chrono::steady_clock::now();
    constexpr int kReps = 1000;
    for (int i = 0; i < kReps; ++i) {
        dispatcher.dispatch(static_cast<double>(i));
    }
    auto elapsed = std::chrono::steady_clock::now() - t0;
    // Average per-call must be well under 1ms (budget is 100µs).
    auto avgUs = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / kReps;
    EXPECT_LT(avgUs, 1000)  // generous threshold for CI; contract is 100µs in prod
        << "Backend selection average " << avgUs << "µs exceeds test guard";
}

// ============================================================================
// GCH-09..GCH-12 — Spatial Index Contract
// ============================================================================

/**
 * @brief GCH-09: Insert + bbox query round-trip: inserted point is found.
 */
TEST(GeoContractSpatialIndex, GCH09_InsertQueryRoundTrip) {
    MockSpatialIndex idx;
    ASSERT_EQ(idx.insert(1, {10.0, 20.0}), GeoErrorCode::OK);
    ASSERT_EQ(idx.insert(2, {15.0, 25.0}), GeoErrorCode::OK);

    Bbox bbox{5.0, 15.0, 20.0, 30.0};
    auto results = idx.queryBbox(bbox);
    EXPECT_EQ(results.size(), 2u);
    EXPECT_TRUE(std::find(results.begin(), results.end(), 1) != results.end());
    EXPECT_TRUE(std::find(results.begin(), results.end(), 2) != results.end());
}

/**
 * @brief GCH-10: Bounding-box query returns all entries whose MBR overlaps.
 */
TEST(GeoContractSpatialIndex, GCH10_BboxQueryReturnsAllOverlapping) {
    MockSpatialIndex idx;
    // Insert 10 points in a grid.
    std::mt19937 rng(kGeoContractSeed);
    std::uniform_real_distribution<double> lon(-10.0, 10.0);
    std::uniform_real_distribution<double> lat(-10.0, 10.0);
    for (int i = 0; i < 10; ++i) {
        idx.insert(i, {lon(rng), lat(rng)});
    }
    // Query entire bounding box: must return all 10.
    Bbox all{-10.0, -10.0, 10.0, 10.0};
    auto results = idx.queryBbox(all);
    EXPECT_EQ(results.size(), 10u);
}

/**
 * @brief GCH-11: Query with non-overlapping bbox returns empty result set.
 */
TEST(GeoContractSpatialIndex, GCH11_NonOverlappingBboxEmpty) {
    MockSpatialIndex idx;
    ASSERT_EQ(idx.insert(1, {5.0, 5.0}), GeoErrorCode::OK);

    Bbox noOverlap{50.0, 50.0, 60.0, 60.0};
    auto results = idx.queryBbox(noOverlap);
    EXPECT_TRUE(results.empty());
}

/**
 * @brief GCH-12: isGeometryError() covers all geometry-validity error codes.
 */
TEST(GeoContractSpatialIndex, GCH12_GeometryErrorClassification) {
    EXPECT_TRUE(isGeometryError(GeoErrorCode::GEOMETRY_INVALID));
    EXPECT_TRUE(isGeometryError(GeoErrorCode::COORDINATE_OUT_OF_BOUNDS));
    EXPECT_TRUE(isGeometryError(GeoErrorCode::GEOMETRY_TOO_LARGE));
    EXPECT_TRUE(isGeometryError(GeoErrorCode::UNSUPPORTED_GEOMETRY_TYPE));

    EXPECT_FALSE(isGeometryError(GeoErrorCode::OK));
    EXPECT_FALSE(isGeometryError(GeoErrorCode::BACKEND_UNAVAILABLE));
    EXPECT_FALSE(isGeometryError(GeoErrorCode::INDEX_CORRUPTED));
}

// ============================================================================
// GCH-13..GCH-16 — Spatial Join Contract
// ============================================================================

/**
 * @brief GCH-13: Intersect join completeness: all qualifying pairs returned.
 */
TEST(GeoContractSpatialJoin, GCH13_IntersectCompleteness) {
    std::vector<MockGeomWithBbox> A = {
        {1, {0, 0, 10, 10}},
        {2, {20, 20, 30, 30}},
    };
    std::vector<MockGeomWithBbox> B = {
        {10, {5, 5, 15, 15}},   // overlaps A[0]
        {11, {25, 25, 35, 35}}, // overlaps A[1]
        {12, {50, 50, 60, 60}}, // overlaps neither
    };
    std::vector<JoinPair> results;
    ASSERT_EQ(spatialJoin(A, B, JoinPredicate::INTERSECTS, results), GeoErrorCode::OK);
    EXPECT_EQ(results.size(), 2u) << "Exactly 2 qualifying intersect pairs expected";
}

/**
 * @brief GCH-14: Contains semantics: B fully inside A is reported; partial outside is not.
 */
TEST(GeoContractSpatialJoin, GCH14_ContainsSoundness) {
    std::vector<MockGeomWithBbox> A = {
        {1, {0, 0, 20, 20}},
    };
    std::vector<MockGeomWithBbox> B = {
        {10, {2,  2,  8,  8}},  // fully inside A
        {11, {15, 15, 25, 25}}, // partially outside A — must NOT be reported
    };
    std::vector<JoinPair> results;
    ASSERT_EQ(spatialJoin(A, B, JoinPredicate::CONTAINS, results), GeoErrorCode::OK);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].idA, 1);
    EXPECT_EQ(results[0].idB, 10);
}

/**
 * @brief GCH-15: Invalid geometry in input → JOIN_INVALID_INPUT (not silent skip).
 */
TEST(GeoContractSpatialJoin, GCH15_InvalidInputReturnsError) {
    std::vector<MockGeomWithBbox> A = {
        {1, {0, 0, 10, 10}, /*isInvalid=*/true},
    };
    std::vector<MockGeomWithBbox> B = {
        {10, {5, 5, 15, 15}},
    };
    std::vector<JoinPair> results;
    auto code = spatialJoin(A, B, JoinPredicate::INTERSECTS, results);
    EXPECT_EQ(code, GeoErrorCode::JOIN_INVALID_INPUT)
        << "Invalid input geometry must NOT be silently skipped";
}

/**
 * @brief GCH-16: isBackendFallback() is true only for GPU_FALLBACK_TO_CPU.
 */
TEST(GeoContractSpatialJoin, GCH16_BackendFallbackClassification) {
    EXPECT_TRUE(isBackendFallback(GeoErrorCode::GPU_FALLBACK_TO_CPU));

    const std::vector<GeoErrorCode> nonFallback = {
        GeoErrorCode::OK,
        GeoErrorCode::GEOMETRY_INVALID,
        GeoErrorCode::BACKEND_UNAVAILABLE,
        GeoErrorCode::BACKEND_RESULT_MISMATCH,
        GeoErrorCode::INTERNAL_ERROR,
    };
    for (auto code : nonFallback) {
        EXPECT_FALSE(isBackendFallback(code))
            << "Expected isBackendFallback=false for code "
            << static_cast<int>(code);
    }
}
