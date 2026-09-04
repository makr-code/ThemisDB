/**
 * @file phase2_phase3_hardening.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 90/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @brief Phase 2/3 Geo Module Hardening Implementation.
 *
 * This module consolidates the hardening work for:
 *
 * **Phase 2 (Core Implementation):**
 *   - Backend dispatch determinism: ensure GPU/CPU fallback is silent, result-consistent, and bounded (≤100µs)
 *   - Geometry internals: strengthen coordinate validation, ring closure, minimum vertices
 *   - Spatial index contracts: verify insert durability, bounding-box consistency, concurrent safety
 *
 * **Phase 3 (Error Handling & Edge Cases):**
 *   - Fail-closed behavior: all invalid geometry → explicit GeoErrorCode, no silent degradation
 *   - Unified diagnostics: consolidated error messages across join/clustering/raster/temporal paths
 *   - Diagnostic helpers: actionable errors for backend fallback and capability drift incidents
 *
 * Roadmap reference: src/geo/ROADMAP.md §"Phase 2 & Phase 3"
 * FUTURE_ENHANCEMENTS reference: src/geo/FUTURE_ENHANCEMENTS.md §"Implementation Notes"
 */

#include <chrono>
#include <atomic>
#include <thread>
#include <memory>
#include <sstream>
#include <cstring>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Backend Dispatch Hardening (Phase 2)
// ============================================================================

// BackendDispatchTimeoutGuard is fully defined in include/geo/phase2_phase3_hardening.h.
// No duplicate definition in this translation unit.

// ============================================================================
// § 2  Backend Dispatch Result Consistency (Phase 2)
// ============================================================================

/**
 * @brief Verify CPU and GPU results are numerically consistent.
 *
 * Backend dispatch invariant per geo_api_contract.h §2(b):
 *   "Fallback does NOT produce different results; numeric accuracy is
 *    equivalent within kBackendAccuracyTolerance."
 *
 * This helper compares distance results from CPU and GPU with relative tolerance.
 * Used in diagnostics and circuit-breaker decision logic to ensure silent
 * fallback doesn't silently diverge results.
 *
 * @param cpu_result  CPU-computed distance
 * @param gpu_result  GPU-computed distance
 * @param tolerance   Relative tolerance (default 1e-9 per contract)
 * @return true if results are within tolerance; false if divergence detected
 */
[[nodiscard]] inline bool isDistanceResultConsistent(
    double cpu_result,
    double gpu_result,
    double tolerance = 1e-9) noexcept {
    // Handle edge cases: both zero, both infinite, both NaN
    if (cpu_result == gpu_result) {
        return true;
    }

    // If either is zero, use absolute tolerance
    if (cpu_result == 0.0 || gpu_result == 0.0) {
        return std::abs(cpu_result - gpu_result) < tolerance;
    }

    // Use relative tolerance: |a - b| / max(|a|, |b|) < tolerance
    const double abs_diff = std::abs(cpu_result - gpu_result);
    const double max_val = std::max(std::abs(cpu_result), std::abs(gpu_result));
    return (abs_diff / max_val) < tolerance;
}

/**
 * @brief Verify containment (point-in-polygon) results are consistent.
 *
 * Backend dispatch invariant per geo_api_contract.h §2(b):
 *   "Fallback does NOT produce different results"
 *
 * For binary containment results (inside/outside), consistency means
 * the two backends return the same boolean value for each test point.
 *
 * @param cpu_mask GPU-computed containment bitset
 * @param gpu_mask GPU-computed containment bitset
 * @param count    Number of test points
 * @return true if all entries match; false if any divergence detected
 */
[[nodiscard]] inline bool isContainmentResultConsistent(
    const std::vector<uint8_t>& cpu_mask,
    const std::vector<uint8_t>& gpu_mask,
    std::size_t count) noexcept {
    if (cpu_mask.size() != count || gpu_mask.size() != count) {
        return false;
    }
    return std::equal(cpu_mask.begin(), cpu_mask.begin() + count,
                      gpu_mask.begin());
}

// ============================================================================
// § 3  Geometry Validation Hardening (Phase 2)
// ============================================================================

/**
 * @brief Validate a coordinate is within WGS84 bounds.
 *
 * Geometry validation contract per geo_api_contract.h §1(b):
 *   "WGS84 BOUNDS: Longitude must be in [-180, +180]; latitude in [-90, +90].
 *    Out-of-bounds coordinates → COORDINATE_OUT_OF_BOUNDS."
 *
 * @param lon Longitude in decimal degrees
 * @param lat Latitude in decimal degrees
 * @return true if both lon and lat are finite and within bounds; false otherwise
 */
[[nodiscard]] inline bool isValidWgs84CoordinatePair(
    double lon,
    double lat) noexcept {
    return std::isfinite(lon) && std::isfinite(lat) &&
           lon >= -180.0 && lon <= 180.0 &&
           lat >= -90.0 && lat <= 90.0;
}

/**
 * @brief Validate a polygon ring is closed (first == last coordinate).
 *
 * Geometry validation contract per geo_api_contract.h §1(c):
 *   "RING CLOSURE: Polygon exterior and interior rings MUST be closed
 *    (first coordinate == last coordinate). Open rings → GEOMETRY_INVALID."
 *
 * @param lons Ring longitude coordinates
 * @param lats Ring latitude coordinates
 * @return true if ring is closed and has ≥4 vertices (including closing point);
 *         false if open, too small, or sizes mismatch
 */
[[nodiscard]] inline bool isRingClosed(
    const std::vector<double>& lons,
    const std::vector<double>& lats) noexcept {
    // Minimum 4 vertices: 3 unique + 1 closing point
    if (lons.size() < 4 || lats.size() < 4 || lons.size() != lats.size()) {
        return false;
    }

    const std::size_t n = lons.size();
    return lons[0] == lons[static_cast<int>(n - 1)] && lats[0] == lats[static_cast<int>(n - 1)];
}

/**
 * @brief Check if a LineString has the minimum required vertices.
 *
 * Geometry validation contract per geo_api_contract.h §1(d):
 *   "MINIMUM VERTICES: A valid LineString requires ≥ 2 points"
 *
 * @param count Number of coordinates in the LineString
 * @return true if count >= 2; false otherwise
 */
[[nodiscard]] inline bool lineStringHasMinimumVertices(
    std::size_t count) noexcept {
    return count >= 2;
}

/**
 * @brief Check if a Polygon exterior ring has the minimum required vertices.
 *
 * Geometry validation contract per geo_api_contract.h §1(d):
 *   "Polygon exterior ring requires ≥ 4 points (including the closing point)"
 *
 * @param count Number of coordinates in the ring (including closing point)
 * @return true if count >= 4; false otherwise
 */
[[nodiscard]] inline bool polygonExteriorRingHasMinimumVertices(
    std::size_t count) noexcept {
    return count >= 4;
}

// ============================================================================
// § 4  Spatial Index Contract Verification (Phase 2)
// ============================================================================

/**
 * @brief Verify bounding-box consistency invariant for spatial index queries.
 *
 * Spatial index contract per geo_api_contract.h §3(b):
 *   "BOUNDING-BOX CONSISTENCY: A query with bounding box B MUST return all
 *    entries whose MBR intersects B. It MUST NOT return entries whose MBR
 *    does not intersect B."
 *
 * This helper checks that a query result set respects the query envelope.
 * Used in index tests and diagnostics to detect index corruption.
 *
 * @param query_bbox Query bounding box [minX, minY, maxX, maxY]
 * @param result_mbrs Minimum bounding rectangles of results [each has 4 values]
 * @return true if all results have MBRs intersecting the query box;
 *         false if any result is outside the query box
 */
[[nodiscard]] inline bool isSpatialIndexQueryConsistent(
    const std::array<double, 4>& query_bbox,
    const std::vector<std::array<double, 4>>& result_mbrs) noexcept {
    const double q_minx = query_bbox[0];
    const double q_miny = query_bbox[1];
    const double q_maxx = query_bbox[2];
    const double q_maxy = query_bbox[3];

    for (const auto& mbr : result_mbrs) {
        const double r_minx = mbr[0];
        const double r_miny = mbr[1];
        const double r_maxx = mbr[2];
        const double r_maxy = mbr[3];

        // Check if MBR intersects query box: no intersection if
        // r_maxx < q_minx || r_minx > q_maxx || r_maxy < q_miny || r_miny > q_maxy
        if (r_maxx < q_minx || r_minx > q_maxx ||
            r_maxy < q_miny || r_miny > q_maxy) {
            return false;  // MBR does not intersect query box
        }
    }

    return true;  // All results respect the query envelope
}

// ============================================================================
// § 5  Unified Error Diagnostics (Phase 3)
// ============================================================================

/**
 * @brief Structured diagnostic context for backend fallback incidents.
 *
 * Unified diagnostics per geo_api_contract.h and Phase 3 requirements:
 *   "unify diagnostics across join/clustering/raster/temporal and fallback paths"
 *
 * Contains machine-readable context for actionable error reporting:
 *   - geometry type and operation
 *   - backend (GPU model, CUDA version, HIP version)
 *   - failure reason (timeout, device OOM, driver error, etc.)
 *   - recovery action taken (CPU fallback, retry, circuit-breaker)
 */
struct BackendFallbackDiagnostic {
    // Operation context
    std::string operation_name;  // e.g., "ST_Intersects", "ST_Contains", "ST_DWithin"
    std::string geometry_type_a; // e.g., "Polygon", "Point", "LineString"
    std::string geometry_type_b; // e.g., "Polygon", "Point", "LineString"
    std::size_t batch_size;      // Number of geometry pairs

    // Backend context
    std::string backend_name;    // e.g., "CUDA", "HIP", "CPU_FALLBACK"
    int         backend_version; // GPU compute capability or driver version
    std::string device_name;     // GPU model (e.g., "RTX 3090", "MI250X")

    // Failure context
    std::string failure_reason;  // e.g., "timeout_exceeded", "device_out_of_memory"
    int         error_code;      // OS/driver-specific error code
    std::int64_t elapsed_micros; // Time spent in GPU operation before timeout

    // Recovery context
    bool        fallback_taken;  // true if CPU fallback was invoked
    bool        result_verified; // true if CPU result was consistent with GPU attempt

    /**
     * @brief Format diagnostic as human-readable error message.
     * @return Formatted string suitable for logging or user error reporting
     */
    [[nodiscard]] std::string formatMessage() const noexcept {
        std::ostringstream oss = {};
        oss << "Geo backend fallback: operation=" << operation_name
            << " geom_a=" << geometry_type_a << " geom_b=" << geometry_type_b
            << " batch_size=" << batch_size
            << " backend=" << backend_name << " device=" << device_name
            << " failure=" << failure_reason << " error_code=" << error_code
            << " elapsed_us=" << elapsed_micros
            << " fallback=" << (fallback_taken ? "yes" : "no")
            << " verified=" << (result_verified ? "yes" : "no");
        return oss.str();
    }
};

/**
 * @brief Unified error message builder for geometry validation failures.
 *
 * Phase 3 requirement: "consolidate error messages across join/clustering/raster/temporal paths"
 *
 * Generates consistent, actionable error messages for all geometry validation
 * failures, with context about the invalid geometry and the validation rule
 * that was violated.
 */
class GeometryValidationErrorBuilder {
public:
    explicit GeometryValidationErrorBuilder() noexcept = default;

    /**
     * @brief Build error message for non-finite coordinates.
     */
    [[nodiscard]] static std::string nonFiniteCoordinate(
        double value,
        const std::string& coord_name,
        std::size_t coord_index) noexcept {
        std::ostringstream oss = {};
        oss << "Geometry validation failed: " << coord_name
            << "[" << coord_index << "] is not finite (value=" << value << "); "
            << "expected a finite real number in [-180, 180] for longitude "
            << "or [-90, 90] for latitude";
        return oss.str();
    }

    /**
     * @brief Build error message for out-of-bounds coordinate.
     */
    [[nodiscard]] static std::string coordinateOutOfBounds(
        double value,
        const std::string& coord_name,
        double min_bound,
        double max_bound) noexcept {
        std::ostringstream oss = {};
        oss << "Geometry validation failed: " << coord_name
            << " is out of bounds (value=" << value << "); "
            << "expected range [" << min_bound << ", " << max_bound << "]";
        return oss.str();
    }

    /**
     * @brief Build error message for unclosed polygon ring.
     */
    [[nodiscard]] static std::string unclosedRing(
        std::size_t ring_index,
        std::size_t vertex_count) noexcept {
        std::ostringstream oss = {};
        oss << "Geometry validation failed: polygon ring " << ring_index
            << " is not closed (first and last coordinates differ); "
            << "ring has " << vertex_count << " vertices; "
            << "RFC 7946 requires rings to be closed (first == last)";
        return oss.str();
    }

    /**
     * @brief Build error message for insufficient vertices.
     */
    [[nodiscard]] static std::string insufficientVertices(
        const std::string& geometry_type,
        std::size_t actual_count,
        std::size_t required_min) noexcept {
        std::ostringstream oss = {};
        oss << "Geometry validation failed: " << geometry_type
            << " has insufficient vertices (actual=" << actual_count
            << ", required_minimum=" << required_min << "); "
            << "see RFC 7946 for minimum requirements per geometry type";
        return oss.str();
    }

    /**
     * @brief Build error message for geometry too large.
     */
    [[nodiscard]] static std::string geometryTooLarge(
        std::size_t coordinate_count,
        std::size_t max_allowed) noexcept {
        std::ostringstream oss = {};
        oss << "Geometry validation failed: geometry is too large "
            << "(coordinate_count=" << coordinate_count
            << ", max_allowed=" << max_allowed << "); "
            << "split into smaller geometries or increase system limits";
        return oss.str();
    }
};

} // namespace geo
} // namespace themis
