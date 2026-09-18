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

enum class GeoPrecisionMode : uint8_t {
    STANDARD = 0, ///< Tolerance: 1e-6 (default, matches CPU/GPU parity bound)
    HIGH     = 1, ///< Tolerance: 1e-9 (requires double-precision GPU or CPU)
    ULTRA    = 2, ///< Tolerance: 1e-12 (CPU-only; GPU path is disabled)
};

inline constexpr std::array<double, 3> kPrecisionTolerance = {
    1e-6,  ///< STANDARD
    1e-9,  ///< HIGH
    1e-12, ///< ULTRA
};

inline constexpr double toleranceForMode(GeoPrecisionMode mode) noexcept {
    return kPrecisionTolerance[static_cast<std::size_t>(mode)];
}

// ============================================================================
// § 2  Backend Combination
// ============================================================================

enum class BackendCombination : uint8_t {
    CPU_ONLY   = 0x01, ///< Only CPU backend available
    GPU_ONLY   = 0x02, ///< Only GPU backend available (unusual, handled as CPU fallback)
    CPU_AND_GPU = 0x03, ///< Both backends present; handler validates parity
};

// ============================================================================
// § 3  Edge-Case Result
// ============================================================================

struct GeoEdgeCaseResult {
    GeoErrorCode code;  ///< GeoErrorCode::OK on success, error code otherwise
    double       value; ///< Computed result; valid only when code == OK

    [[nodiscard]] bool ok() const noexcept {
        return code == GeoErrorCode::OK;
    }
};

// ============================================================================
// § 4  GeoEdgeCaseHandler
// ============================================================================

class GeoEdgeCaseHandler {
public:
    using IncidentCallback = std::function<void(std::string_view incident_id,
                                                std::string_view description)>;

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

    [[nodiscard]] GeoEdgeCaseResult dispatch(
        std::function<double()>                cpu_fn,
        std::function<std::optional<double>()> gpu_fn = nullptr) noexcept;

    // -------------------------------------------------------------------------
    // § 4.2  Geometry Boundary Validation
    // -------------------------------------------------------------------------

    [[nodiscard]] GeoErrorCode validateCoordinateBoundary(
        double lon, double lat) const noexcept;

    [[nodiscard]] GeoErrorCode validateRing(
        const std::vector<std::pair<double, double>>& ring) const noexcept;

    // -------------------------------------------------------------------------
    // § 4.3  Capability Drift Detection
    // -------------------------------------------------------------------------

    [[nodiscard]] bool hasCapabilityDrift() const noexcept;

    /**
     * @brief Reset Drift Counter.
     * @note Exception safety: noexcept.
     */
    void resetDriftCounter() noexcept;

    // -------------------------------------------------------------------------
    // § 4.4  Counters (diagnostic)
    // -------------------------------------------------------------------------

    [[nodiscard]] uint64_t cpuDispatchCount() const noexcept;

    [[nodiscard]] uint64_t gpuDispatchCount() const noexcept;

    [[nodiscard]] uint64_t fallbackCount() const noexcept;

    [[nodiscard]] uint64_t driftEventCount() const noexcept;

private:
    BackendCombination      combo_;
    GeoPrecisionMode        precision_;
    IncidentCallback        on_incident_;
    std::atomic<uint64_t>   cpu_dispatch_count_{0};
    std::atomic<uint64_t>   gpu_dispatch_count_{0};
    std::atomic<uint64_t>   fallback_count_{0};
    std::atomic<uint64_t>   drift_event_count_{0};

    static constexpr uint64_t kDriftThreshold = 5;

    /**
     * @brief Emit Incident.
     * @param[in] id Input parameter.
     * @param[in] desc Input parameter.
     * @note Exception safety: noexcept.
     */
    void emitIncident(std::string_view id, std::string_view desc) const noexcept;
};

} // namespace geo
} // namespace themis
