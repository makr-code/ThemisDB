/**
 * @file phase2_phase3_hardening.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 90/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: phase2_phase3_hardening.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Score: 90/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * Status: Production Ready
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <memory>

namespace themis {
namespace geo {

// ============================================================================
// Phase 2/3 Hardening Components
// ============================================================================

/**
 * @brief Timeout guard for GPU backend operations.
 *
 * Ensures all GPU dispatch operations complete within a bounded time envelope.
 * If a GPU operation exceeds the timeout, the dispatcher silently falls back
 * to the CPU path and records the incident for diagnostic purposes.
 *
 * Contract per geo_api_contract.h §2:
 *   - Backend selection latency must not exceed kBackendSelectionBudget (100µs)
 *   - GPU fallback must be silent (no error logged to user)
 *   - Result consistency must be preserved (CPU and GPU produce equivalent results)
 *
 * Thread safety: const methods are thread-safe; state is immutable after construction.
 */
class BackendDispatchTimeoutGuard {
public:
    /**
     * @brief Construct a timeout guard with a deadline.
     * @param timeout_micros Maximum microseconds to wait before falling back.
     */
    explicit BackendDispatchTimeoutGuard(
        std::chrono::microseconds timeout_micros) noexcept
        : deadline_(std::chrono::high_resolution_clock::now() + timeout_micros),
          timeout_us_(timeout_micros.count()),
          expired_cached_(false),
          checked_(false) {}

    /**
     * @brief Check if the timeout has been exceeded.
     * @return true if the deadline has passed; cached after first call.
     */
    [[nodiscard]] bool expired() const noexcept {
        if (checked_.load(std::memory_order_acquire)) {
            return expired_cached_.load(std::memory_order_relaxed);
        }
        const auto now = std::chrono::high_resolution_clock::now();
        const bool is_expired = (now >= deadline_);
        if (!checked_.exchange(true, std::memory_order_acq_rel)) {
            expired_cached_.store(is_expired, std::memory_order_relaxed);
        }
        return is_expired;
    }

    /**
     * @brief Get the timeout budget in microseconds.
     * @return timeout value passed at construction.
     */
    [[nodiscard]] std::int64_t timeoutMicros() const noexcept {
        return timeout_us_;
    }

private:
    std::chrono::high_resolution_clock::time_point deadline_;
    std::int64_t                                    timeout_us_;
    mutable std::atomic<bool>                       expired_cached_{false};
    mutable std::atomic<bool>                       checked_{false};
};

// ============================================================================
// Backend Dispatch Result Consistency Helpers (Phase 2)
// ============================================================================

/**
 * @brief Verify CPU and GPU results are numerically consistent.
 *
 * Backend dispatch invariant per geo_api_contract.h §2(b):
 *   "Fallback does NOT produce different results; numeric accuracy is
 *    equivalent within kBackendAccuracyTolerance."
 *
 * @param cpu_result  CPU-computed distance
 * @param gpu_result  GPU-computed distance
 * @param tolerance   Relative tolerance (default 1e-9 per contract)
 * @return true if results are within tolerance; false if divergence detected
 */
[[nodiscard]] inline bool isDistanceResultConsistent(
    double cpu_result,
    double gpu_result,
    double tolerance = 1e-9) noexcept;

/**
 * @brief Verify containment (point-in-polygon) results are consistent.
 *
 * Backend dispatch invariant per geo_api_contract.h §2(b):
 *   "Fallback does NOT produce different results"
 *
 * @param cpu_mask CPU-computed containment bitset
 * @param gpu_mask GPU-computed containment bitset
 * @param count    Number of test points
 * @return true if all entries match; false if any divergence detected
 */
[[nodiscard]] inline bool isContainmentResultConsistent(
    const std::vector<uint8_t>& cpu_mask,
    const std::vector<uint8_t>& gpu_mask,
    std::size_t count) noexcept;

// ============================================================================
// Geometry Validation Helpers (Phase 2)
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
    double lat) noexcept;

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
    const std::vector<double>& lats) noexcept;

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
    std::size_t count) noexcept;

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
    std::size_t count) noexcept;

// ============================================================================
// Spatial Index Contract Verification (Phase 2)
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
 * @param result_mbrs Minimum bounding rectangles of results
 * @return true if all results have MBRs intersecting the query box;
 *         false if any result is outside the query box
 */
[[nodiscard]] inline bool isSpatialIndexQueryConsistent(
    const std::array<double, 4>& query_bbox,
    const std::vector<std::array<double, 4>>& result_mbrs) noexcept;

// ============================================================================
// Unified Error Diagnostics (Phase 3)
// ============================================================================

/**
 * @brief Structured diagnostic context for backend fallback incidents.
 *
 * Unified diagnostics per Phase 3 requirements:
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
    std::string operation_name;  ///< e.g., "ST_Intersects", "ST_Contains"
    std::string geometry_type_a; ///< e.g., "Polygon", "Point", "LineString"
    std::string geometry_type_b; ///< e.g., "Polygon", "Point", "LineString"
    std::size_t batch_size;      ///< Number of geometry pairs

    // Backend context
    std::string backend_name;    ///< e.g., "CUDA", "HIP", "CPU_FALLBACK"
    int         backend_version; ///< GPU compute capability or driver version
    std::string device_name;     ///< GPU model (e.g., "RTX 3090", "MI250X")

    // Failure context
    std::string failure_reason;  ///< e.g., "timeout_exceeded", "device_out_of_memory"
    int         error_code;      ///< OS/driver-specific error code
    std::int64_t elapsed_micros; ///< Time spent in GPU operation before timeout

    // Recovery context
    bool fallback_taken;         ///< true if CPU fallback was invoked
    bool result_verified;        ///< true if result was consistent with GPU attempt

    /**
     * @brief Format diagnostic as human-readable error message.
     * @return Formatted string suitable for logging or user error reporting
     */
    [[nodiscard]] std::string formatMessage() const noexcept;
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
    /**
     * @brief Build error message for non-finite coordinates.
     */
    [[nodiscard]] static std::string nonFiniteCoordinate(
        double value,
        const std::string& coord_name,
        std::size_t coord_index) noexcept;

    /**
     * @brief Build error message for out-of-bounds coordinate.
     */
    [[nodiscard]] static std::string coordinateOutOfBounds(
        double value,
        const std::string& coord_name,
        double min_bound,
        double max_bound) noexcept;

    /**
     * @brief Build error message for unclosed polygon ring.
     */
    [[nodiscard]] static std::string unclosedRing(
        std::size_t ring_index,
        std::size_t vertex_count) noexcept;

    /**
     * @brief Build error message for insufficient vertices.
     */
    [[nodiscard]] static std::string insufficientVertices(
        const std::string& geometry_type,
        std::size_t actual_count,
        std::size_t required_min) noexcept;

    /**
     * @brief Build error message for geometry too large.
     */
    [[nodiscard]] static std::string geometryTooLarge(
        std::size_t coordinate_count,
        std::size_t max_allowed) noexcept;
};

} // namespace geo
} // namespace themis
