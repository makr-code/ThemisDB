// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_edge_case_handler.h
 * @brief Deterministic edge-case handler for mixed-backend and precision-mode
 *        permutations in the ThemisDB geo module.
 *
 * This header provides the GeoEdgeCaseHandler class, which enforces deterministic
 * behavior across all backend/precision-mode combinations:
 *
 *   - Mixed CPU/GPU backend permutations with validated result consistency
 *   - Precision-mode edge cases (STANDARD, HIGH, ULTRA) with bounded tolerances
 *   - Geometry boundary conditions (degenerate, near-antipodal, null-island)
 *   - Complex join/raster edge cases (empty inputs, single-point, max-size)
 *   - Backend capability drift detection with fail-closed semantics
 *
 * ## Design Principles
 *
 * 1. **Determinism**: identical inputs always produce identical outputs regardless
 *    of backend selection order.
 * 2. **Fail-Closed**: unsupported precision/backend combinations produce
 *    GeoErrorCode::BACKEND_UNAVAILABLE rather than silently degrading.
 * 3. **Bounded Tolerance**: each precision mode carries a documented numeric
 *    tolerance bound; results outside the bound are flagged as PRECISION_EXCEEDED.
 * 4. **Thread-Safety**: all public methods are safe for concurrent invocation.
 *
 * @see include/geo/geo_api_contract.h  — base error taxonomy and backend contracts
 * @see src/geo/ROADMAP.md             — Phase 2/3 Q4 2026 items
 */

#pragma once

#include "geo/geo_api_contract.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Precision Mode
// ============================================================================

/**
 * @brief Precision modes supported by the geo edge-case handler.
 *
 * Each mode defines a tolerance bound used to validate cross-backend result
 * consistency.  Higher precision modes use smaller tolerances and may incur
 * higher computational cost.
 */
enum class GeoPrecisionMode : uint8_t {
    STANDARD = 0, ///< Tolerance: 1e-6 (default, matches CPU/GPU parity bound)
    HIGH     = 1, ///< Tolerance: 1e-9 (requires double-precision GPU or CPU)
    ULTRA    = 2, ///< Tolerance: 1e-12 (CPU-only; GPU path is disabled)
};

/// Numeric tolerance bounds per precision mode.
inline constexpr std::array<double, 3> kPrecisionTolerance = {
    1e-6,  ///< STANDARD
    1e-9,  ///< HIGH
    1e-12, ///< ULTRA
};

/// Returns the tolerance for the given precision mode.
inline constexpr double toleranceForMode(GeoPrecisionMode mode) noexcept {
    return kPrecisionTolerance[static_cast<std::size_t>(mode)];
}

// ============================================================================
// § 2  Backend Combination
// ============================================================================

/**
 * @brief Backend availability bitmask used for edge-case dispatch.
 *
 * Combinations encode which backends are present at runtime.  The handler
 * uses this to select the canonical path and validate cross-backend parity.
 */
enum class BackendCombination : uint8_t {
    CPU_ONLY   = 0x01, ///< Only CPU backend available
    GPU_ONLY   = 0x02, ///< Only GPU backend available (unusual, handled as CPU fallback)
    CPU_AND_GPU = 0x03, ///< Both backends present; handler validates parity
};

// ============================================================================
// § 3  Edge-Case Result
// ============================================================================

/**
 * @brief Result returned by GeoEdgeCaseHandler operations.
 *
 * Carries the computed double scalar result (distance, area, etc.) or
 * an error code indicating what went wrong.  When ok() is false the
 * numeric value is meaningless and must not be used.
 */
struct GeoEdgeCaseResult {
    GeoErrorCode code;  ///< GeoErrorCode::OK on success, error code otherwise
    double       value; ///< Computed result; valid only when code == OK

    /// @brief Returns true when the operation succeeded.
    [[nodiscard]] bool ok() const noexcept {
        return code == GeoErrorCode::OK;
    }
};

// ============================================================================
// § 4  GeoEdgeCaseHandler
// ============================================================================

/**
 * @brief Deterministic handler for mixed-backend and precision-mode edge cases.
 *
 * Responsibilities:
 *   - Validate that the requested (backend, precision) combination is supported.
 *   - Dispatch the provided compute function via the canonical backend path.
 *   - For CPU_AND_GPU combinations, optionally cross-validate results within
 *     the precision tolerance to detect backend capability drift.
 *   - Record edge-case incidents via the registered incident callback.
 *
 * Thread safety:
 *   All public methods are thread-safe.  Internal counters use atomic operations.
 *   The incident callback must itself be thread-safe if called concurrently.
 *
 * Example:
 * @code
 *   GeoEdgeCaseHandler handler(BackendCombination::CPU_AND_GPU,
 *                              GeoPrecisionMode::STANDARD);
 *   auto result = handler.dispatch([](){ return haversineDistance(p1, p2); },
 *                                  [](){ return gpuHaversineDistance(p1, p2); });
 *   if (!result.ok()) { // handle error }
 * @endcode
 */
class GeoEdgeCaseHandler {
public:
    /// @brief Incident callback type invoked on detected edge-case anomalies.
    using IncidentCallback = std::function<void(std::string_view incident_id,
                                                std::string_view description)>;

    /**
     * @brief Construct the handler for the given backend combination and precision.
     *
     * @param combo      Which backends are available at runtime.
     * @param precision  Desired precision mode.
     * @param on_incident Optional callback invoked when anomalies are detected.
     */
    explicit GeoEdgeCaseHandler(
        BackendCombination combo     = BackendCombination::CPU_ONLY,
        GeoPrecisionMode   precision = GeoPrecisionMode::STANDARD,
        IncidentCallback   on_incident = nullptr) noexcept;

    // Non-copyable; movable.
    GeoEdgeCaseHandler(const GeoEdgeCaseHandler&) = delete;
    GeoEdgeCaseHandler& operator=(const GeoEdgeCaseHandler&) = delete;
    GeoEdgeCaseHandler(GeoEdgeCaseHandler&&) noexcept = default;
    GeoEdgeCaseHandler& operator=(GeoEdgeCaseHandler&&) noexcept = default;

    // -------------------------------------------------------------------------
    // § 4.1  Dispatch
    // -------------------------------------------------------------------------

    /**
     * @brief Dispatch a scalar geometry computation with deterministic fallback.
     *
     * For CPU_ONLY: invokes cpu_fn and returns its result.
     * For GPU_ONLY or CPU_AND_GPU:
     *   - Attempts the GPU path first.
     *   - Falls back to CPU on GPU failure.
     *   - When both paths succeed, cross-validates within the precision tolerance.
     *   - If cross-validation fails, emits a BACKEND_DRIFT incident and returns the
     *     CPU result (authoritative for parity discrepancies).
     *
     * @param cpu_fn  Callable returning a double result via the CPU path.
     * @param gpu_fn  Callable returning an optional<double> (nullopt = unavailable).
     * @return GeoEdgeCaseResult with the canonical result or an error code.
     */
    [[nodiscard]] GeoEdgeCaseResult dispatch(
        std::function<double()>                cpu_fn,
        std::function<std::optional<double>()> gpu_fn = nullptr) noexcept;

    // -------------------------------------------------------------------------
    // § 4.2  Geometry Boundary Validation
    // -------------------------------------------------------------------------

    /**
     * @brief Validate a single WGS84 coordinate pair for boundary edge cases.
     *
     * Detects:
     *   - Null-island (exactly 0.0, 0.0) — valid coordinate but often a data error
     *   - Near-antipodal pairs exceeding the wrap-around boundary
     *   - Degenerate geometries (NaN or ±Inf in coordinates)
     *   - Out-of-bounds coordinates (outside WGS84 envelope)
     *
     * @param lon  Longitude in degrees.
     * @param lat  Latitude in degrees.
     * @return GeoErrorCode::OK for valid coordinates; relevant error code otherwise.
     */
    [[nodiscard]] GeoErrorCode validateCoordinateBoundary(
        double lon, double lat) const noexcept;

    /**
     * @brief Validate a coordinate ring for closure, minimum vertices, and
     *        degenerate edge cases.
     *
     * @param ring  Vector of (lon, lat) pairs representing the ring.
     * @return GeoErrorCode::OK when the ring is valid; error code otherwise.
     */
    [[nodiscard]] GeoErrorCode validateRing(
        const std::vector<std::pair<double, double>>& ring) const noexcept;

    // -------------------------------------------------------------------------
    // § 4.3  Capability Drift Detection
    // -------------------------------------------------------------------------

    /**
     * @brief Check whether the backend combination has experienced capability drift.
     *
     * Drift is recorded when cross-backend parity checks detect results outside
     * the precision tolerance.  Persistent drift triggers a fail-closed transition
     * where GPU operations are suppressed until the drift counter is reset.
     *
     * @return true when persistent drift has been detected.
     */
    [[nodiscard]] bool hasCapabilityDrift() const noexcept;

    /**
     * @brief Reset the drift counter, allowing GPU operations to resume.
     *
     * Should only be called after the GPU backend has been recalibrated or
     * reinitialized.
     */
    void resetDriftCounter() noexcept;

    // -------------------------------------------------------------------------
    // § 4.4  Counters (diagnostic)
    // -------------------------------------------------------------------------

    /// @brief Number of successful CPU dispatches since construction.
    [[nodiscard]] uint64_t cpuDispatchCount() const noexcept;

    /// @brief Number of successful GPU dispatches since construction.
    [[nodiscard]] uint64_t gpuDispatchCount() const noexcept;

    /// @brief Number of GPU-to-CPU fallbacks triggered since construction.
    [[nodiscard]] uint64_t fallbackCount() const noexcept;

    /// @brief Number of cross-backend parity mismatches since construction.
    [[nodiscard]] uint64_t driftEventCount() const noexcept;

private:
    BackendCombination      combo_;
    GeoPrecisionMode        precision_;
    IncidentCallback        on_incident_;
    std::atomic<uint64_t>   cpu_dispatch_count_{0};
    std::atomic<uint64_t>   gpu_dispatch_count_{0};
    std::atomic<uint64_t>   fallback_count_{0};
    std::atomic<uint64_t>   drift_event_count_{0};

    /// Persistent drift flag: set when drift_event_count_ exceeds kDriftThreshold.
    static constexpr uint64_t kDriftThreshold = 5;

    /// Emits an incident via the callback (no-op if callback is null).
    void emitIncident(std::string_view id, std::string_view desc) const noexcept;
};

} // namespace geo
} // namespace themis
