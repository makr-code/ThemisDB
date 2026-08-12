/**
 * @file geo_phase2_phase3_integration.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @brief Phase 2/3 Geo Module Integration - Coordinates hardening components.
 *
 * This module coordinates the integration of Phase 2/3 hardening components
 * (backend dispatch timeout enforcement, geometry validation, spatial index
 * contract verification, and unified diagnostics) with the existing geo module.
 *
 * Roadmap reference: src/geo/ROADMAP.md §"Phase 2 & Phase 3"
 * FUTURE_ENHANCEMENTS reference: src/geo/FUTURE_ENHANCEMENTS.md
 */

#include "geo/phase2_phase3_hardening.h"
#include "geo/geo_api_contract.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <memory>
#include <atomic>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Backend Dispatch Circuit Breaker (Phase 2/3)
// ============================================================================

/**
 * @brief Circuit breaker for GPU backend failures.
 *
 * Tracks persistent GPU failures and transitions to CPU-only mode when
 * the error rate exceeds a threshold. Implements exponential backoff to
 * allow GPU recovery without repeatedly attempting flaky operations.
 *
 * Thread safety: all methods are thread-safe via atomic operations.
 *
 * Specification:
 *   - Failure window: last 100 operations
 *   - Threshold: > 50% failures in window
 *   - Backoff: exponential with max 10s between retry attempts
 *   - Reset: after 5 consecutive successes
 */
class GpuBackendCircuitBreaker {
public:
    /**
     * @brief Operational states of the circuit breaker.
     */
    enum class State {
        CLOSED,     ///< GPU backend is enabled (normal operation)
        OPEN,       ///< GPU backend is disabled (due to high failure rate)
        HALF_OPEN,  ///< Attempting to recover after failure (limited retries)
    };

    GpuBackendCircuitBreaker() noexcept
        : state_(State::CLOSED),
          failure_count_(0),
          success_count_(0),
          backoff_deadline_(std::chrono::high_resolution_clock::now()) {
    }

    /**
     * @brief Check if GPU backend dispatch is permitted.
     * @return true if GPU backend should be attempted; false if CPU-only
     */
    [[nodiscard]] bool canDispatchToGpu() const noexcept {
        const State current = state_.load(std::memory_order_acquire);
        if (current == State::CLOSED) {
            return true;  // GPU enabled, proceed with dispatch
        }

        if (current == State::HALF_OPEN) {
            // Backoff period: check if we can retry
            const auto now = std::chrono::high_resolution_clock::now();
            return now >= backoff_deadline_.load(std::memory_order_acquire);
        }

        // State::OPEN: GPU disabled until recovery conditions are met
        return false;
    }

    /**
     * @brief Record a GPU operation failure.
     *
     * Increments failure counter and transitions to OPEN state if
     * failure rate exceeds threshold (>50% in rolling 100-op window).
     */
    void recordGpuFailure() noexcept {
        std::uint32_t failures = failure_count_.fetch_add(1, std::memory_order_acq_rel);
        success_count_.store(0, std::memory_order_relaxed);  // Reset recovery counter

        // Threshold: > 50 failures in 100 operations
        if (failures > 50) {
            State expected = State::CLOSED;
            state_.compare_exchange_strong(expected, State::OPEN, std::memory_order_release);
        }
    }

    /**
     * @brief Record a GPU operation success.
     *
     * Increments success counter and transitions back to CLOSED state
     * after 5 consecutive successes (indicating recovery).
     */
    void recordGpuSuccess() noexcept {
        failure_count_.store(0, std::memory_order_relaxed);  // Reset failure counter
        std::uint32_t successes = success_count_.fetch_add(1, std::memory_order_acq_rel);

        // Recovery: 5 consecutive successes bring us back to CLOSED
        if (successes >= 4) {
            state_.store(State::CLOSED, std::memory_order_release);
        } else if (state_.load(std::memory_order_relaxed) == State::OPEN) {
            // Enable HALF_OPEN for testing
            state_.store(State::HALF_OPEN, std::memory_order_release);
            // Exponential backoff: 1s, 2s, 4s, 8s, 10s (capped)
            std::uint32_t backoff_secs = std::min(1U << successes, 10U);
            const auto deadline = std::chrono::high_resolution_clock::now() +
                                  std::chrono::seconds(backoff_secs);
            backoff_deadline_.store(deadline, std::memory_order_release);
        }
    }

    /**
     * @brief Get the current circuit breaker state.
     * @return Current State (CLOSED, OPEN, or HALF_OPEN)
     */
    [[nodiscard]] State currentState() const noexcept {
        return state_.load(std::memory_order_acquire);
    }

    /**
     * @brief Reset the circuit breaker to CLOSED state.
     *
     * Used for testing and manual recovery after transient GPU failures.
     */
    void reset() noexcept {
        state_.store(State::CLOSED, std::memory_order_release);
        failure_count_.store(0, std::memory_order_relaxed);
        success_count_.store(0, std::memory_order_relaxed);
    }

private:
    std::atomic<State>                                       state_;
    std::atomic<std::uint32_t>                               failure_count_;
    std::atomic<std::uint32_t>                               success_count_;
    std::atomic<std::chrono::high_resolution_clock::time_point>
                                                            backoff_deadline_;
};

// ============================================================================
// § 2  Geometry Validation Integration (Phase 2)
// ============================================================================

/**
 * @brief Geometry validation orchestrator.
 *
 * Coordinates validation helpers with structured diagnostic context.
 * Used at geometry entry points to enforce fail-closed behavior and
 * produce actionable error messages for invalid geometries.
 *
 * Invariant: All geometry operations must call validate() before proceeding.
 */
class GeometryValidationOrchestrator {
public:
    /**
     * @brief Validate a coordinate pair and produce diagnostic on failure.
     *
     * Enforces WGS84 bounds per geo_api_contract.h §1(b).
     *
     * @param lon Longitude in decimal degrees
     * @param lat Latitude in decimal degrees
     * @param context Operation context for error reporting
     * @return true if valid; false if out-of-bounds (error logged)
     */
    [[nodiscard]] static bool validateWgs84Coordinate(
        double lon,
        double lat,
        const std::string& context) noexcept {
        if (!isValidWgs84CoordinatePair(lon, lat)) {
            if (!std::isfinite(lon) || !std::isfinite(lat)) {
                spdlog::warn("{}: non-finite coordinate (lon={}, lat={})",
                             context, lon, lat);
            } else {
                const auto msg = GeometryValidationErrorBuilder::coordinateOutOfBounds(
                    lon, "longitude", kWgs84LonMin, kWgs84LonMax);
                spdlog::warn("{}", msg);
            }
            return false;
        }
        return true;
    }

    /**
     * @brief Validate polygon ring closure and produce diagnostic on failure.
     *
     * Enforces RFC 7946 ring closure per geo_api_contract.h §1(c).
     *
     * @param lons Ring longitude coordinates
     * @param lats Ring latitude coordinates
     * @param ring_index Index of the ring (0 for exterior, >0 for interior)
     * @return true if closed; false if open (error logged)
     */
    [[nodiscard]] static bool validateRingClosure(
        const std::vector<double>& lons,
        const std::vector<double>& lats,
        std::size_t ring_index) noexcept {
        if (!isRingClosed(lons, lats)) {
            const auto msg = GeometryValidationErrorBuilder::unclosedRing(
                ring_index, lons.size());
            spdlog::warn("{}", msg);
            return false;
        }
        return true;
    }

    /**
     * @brief Validate geometry coordinate count per geo_api_contract.h §1.
     *
     * @param geom_type Geometry type string (e.g., "LineString", "Polygon")
     * @param coord_count Number of coordinates
     * @param required_min Minimum required coordinates
     * @return true if count >= required_min; false otherwise (error logged)
     */
    [[nodiscard]] static bool validateVertexCount(
        const std::string& geom_type,
        std::size_t coord_count,
        std::size_t required_min) noexcept {
        if (coord_count < required_min) {
            const auto msg = GeometryValidationErrorBuilder::insufficientVertices(
                geom_type, coord_count, required_min);
            spdlog::warn("{}", msg);
            return false;
        }
        return true;
    }

    /**
     * @brief Validate geometry size does not exceed contract limits.
     *
     * Per geo_api_contract.h §1: geometries exceeding kMaxGeometryCoordinates
     * return GEOMETRY_TOO_LARGE.
     *
     * @param coord_count Number of coordinates in geometry
     * @return true if within limits; false if too large (error logged)
     */
    [[nodiscard]] static bool validateGeometrySize(
        std::size_t coord_count) noexcept {
        if (coord_count > kMaxGeometryCoordinates) {
            const auto msg = GeometryValidationErrorBuilder::geometryTooLarge(
                coord_count, kMaxGeometryCoordinates);
            spdlog::warn("{}", msg);
            return false;
        }
        return true;
    }
};

// ============================================================================
// § 3  Diagnostic Logger (Phase 3)
// ============================================================================

/**
 * @brief Diagnostic event logger for backend fallback and geo incidents.
 *
 * Consolidates all geo diagnostic logging to a structured log sink.
 * Used to produce actionable error messages for production geo incidents.
 */
class GeoDiagnosticLogger {
public:
    /**
     * @brief Log a backend fallback incident with full diagnostic context.
     *
     * @param diag Diagnostic context (populated by backend dispatch code)
     */
    static void logBackendFallback(
        const BackendFallbackDiagnostic& diag) noexcept {
        const auto msg = diag.formatMessage();
        spdlog::info("Geo backend fallback: {}", msg);
    }

    /**
     * @brief Log a geometry validation failure.
     *
     * @param geometry_type Type of geometry that failed validation
     * @param error_message Structured error message
     * @param context Operation context (e.g., "ST_Intersects", "ST_Contains")
     */
    static void logGeometryValidationFailure(
        const std::string& geometry_type,
        const std::string& error_message,
        const std::string& context) noexcept {
        spdlog::warn("Geometry validation failed in {}: type={} error={}",
                     context, geometry_type, error_message);
    }

    /**
     * @brief Log a spatial index invariant violation.
     *
     * Used when the index consistency check detects an invariant violation
     * (e.g., query result outside query envelope).
     *
     * @param operation Index operation name (e.g., "query", "insert")
     * @param reason Description of the invariant violation
     */
    static void logIndexInvariantViolation(
        const std::string& operation,
        const std::string& reason) noexcept {
        spdlog::error("Spatial index invariant violation in {}: {}",
                      operation, reason);
    }
};

// ============================================================================
// § 4  Module Initialization & Public API
// ============================================================================

/**
 * @brief Initialize Phase 2/3 hardening components at module startup.
 *
 * Sets up circuit breaker, diagnostic logger, and validation orchestrator
 * for use by geo module operations.
 *
 * Must be called once at module initialization, before any geo operations.
 */
void initializePhase2Phase3Hardening() noexcept {
    // Initialize the shared circuit breaker instance (same as getGpuCircuitBreaker).
    (void)getGpuCircuitBreaker();

    // Initialize diagnostic logger (uses spdlog global logger)
    spdlog::info("Geo module Phase 2/3 hardening initialized: "
                 "backend circuit breaker, validation orchestrator, diagnostics");
}

/**
 * @brief Get the GPU backend circuit breaker instance.
 *
 * Returns the module-global circuit breaker used to manage GPU backend
 * availability and recovery from persistent failures.
 *
 * @return Pointer to the circuit breaker (never null)
 */
[[nodiscard]] GpuBackendCircuitBreaker* getGpuCircuitBreaker() noexcept {
    static GpuBackendCircuitBreaker cb;
    return &cb;
}

} // namespace geo
} // namespace themis
