// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file geo_operator_diagnostics.h
 * @brief Operator-facing diagnostics for the ThemisDB geo module.
 *
 * Provides structured incident reporting for:
 *   - Backend fallback events (GPU → CPU transitions)
 *   - Capability drift detection (cross-backend parity failures)
 *   - Geometry validation failures with remediation hints
 *   - Spatial join and raster edge-case anomalies
 *
 * ## Design
 *
 * GeoOperatorDiagnostics aggregates incidents emitted by GeoEdgeCaseHandler
 * and other geo sub-systems.  Operators can query the diagnostics registry for
 * recent incidents and remediation guidance.
 *
 * Each incident has:
 *   - A unique incident ID (e.g. "GEO-ECH-FALLBACK")
 *   - A severity level (INFO, WARNING, ERROR, CRITICAL)
 *   - A human-readable description
 *   - A remediation hint
 *   - A timestamp (nanoseconds since epoch)
 *
 * ## Thread Safety
 *
 * All public methods are thread-safe via a shared mutex.
 *
 * @see include/geo/geo_edge_case_handler.h
 * @see src/geo/ROADMAP.md — Phase 2/3 Q4 2026
 */

#pragma once

#include "geo/geo_api_contract.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace geo {

// ============================================================================
// § 1  Severity
// ============================================================================

/// @brief Incident severity levels.
enum class GeoIncidentSeverity : uint8_t {
    INFO     = 0, ///< Informational event, no operator action required
    WARNING  = 1, ///< Degraded operation, operator should investigate
    ERROR    = 2, ///< Operation failed with automatic recovery
    CRITICAL = 3, ///< Unrecoverable failure, operator intervention required
};

/// @brief Returns a string representation of the severity level.
inline constexpr std::string_view severityName(GeoIncidentSeverity s) noexcept {
    switch (s) {
        case GeoIncidentSeverity::INFO:     return "INFO";
        case GeoIncidentSeverity::WARNING:  return "WARNING";
        case GeoIncidentSeverity::ERROR:    return "ERROR";
        case GeoIncidentSeverity::CRITICAL: return "CRITICAL";
        default:                            return "UNKNOWN";
    }
}

// ============================================================================
// § 2  GeoIncident
// ============================================================================

/**
 * @brief A single geo module incident record.
 */
struct GeoIncident {
    std::string          incident_id;     ///< Unique incident identifier (e.g. "GEO-ECH-FALLBACK")
    GeoIncidentSeverity  severity;        ///< Severity level
    std::string          description;     ///< Human-readable description
    std::string          remediation;     ///< Operator remediation hint
    int64_t              timestamp_ns;    ///< Unix timestamp in nanoseconds
    std::optional<GeoErrorCode> error_code; ///< Associated error code (if any)
};

// ============================================================================
// § 3  GeoOperatorDiagnostics
// ============================================================================

/**
 * @brief Operator-facing diagnostics registry for the geo module.
 *
 * Collects and stores geo incidents from edge-case handlers, backend dispatchers,
 * and spatial join/raster paths.  Provides query and export APIs for operators.
 *
 * Usage example:
 * @code
 *   GeoOperatorDiagnostics diag;
 *   diag.recordIncident("GEO-ECH-FALLBACK", GeoIncidentSeverity::WARNING,
 *                       "GPU → CPU fallback triggered",
 *                       "Check GPU health via device_detector");
 *   auto recent = diag.recentIncidents(10);
 * @endcode
 */
class GeoOperatorDiagnostics {
public:
    /**
     * @brief Maximum number of incidents retained in the ring buffer.
     *
     * Older incidents are evicted when the buffer is full.  A buffer size
     * of 256 retains roughly the last 256 distinct events.
     */
    static constexpr std::size_t kMaxIncidents = 256;

    GeoOperatorDiagnostics() = default;

    // Non-copyable; movable.
    GeoOperatorDiagnostics(const GeoOperatorDiagnostics&) = delete;
    GeoOperatorDiagnostics& operator=(const GeoOperatorDiagnostics&) = delete;
    GeoOperatorDiagnostics(GeoOperatorDiagnostics&&) noexcept = default;
    GeoOperatorDiagnostics& operator=(GeoOperatorDiagnostics&&) noexcept = default;

    // -------------------------------------------------------------------------
    // § 3.1  Recording
    // -------------------------------------------------------------------------

    /**
     * @brief Record a new incident.
     *
     * @param incident_id   Unique ID for this incident class.
     * @param severity      Incident severity.
     * @param description   Human-readable description.
     * @param remediation   Operator action hint.
     * @param error_code    Optional associated GeoErrorCode.
     */
    void recordIncident(
        std::string_view    incident_id,
        GeoIncidentSeverity severity,
        std::string_view    description,
        std::string_view    remediation,
        std::optional<GeoErrorCode> error_code = std::nullopt) noexcept;

    /**
     * @brief Convenience overload used by GeoEdgeCaseHandler incident callback.
     *
     * Infers severity from the incident_id prefix convention:
     *   - *-DRIFT-PERSISTENT → CRITICAL
     *   - *-DRIFT             → WARNING
     *   - *-FALLBACK          → WARNING
     *   - *-INVALID           → ERROR
     *   - other               → INFO
     *
     * @param incident_id  Incident identifier.
     * @param description  Human-readable description.
     */
    void recordFromCallback(std::string_view incident_id,
                            std::string_view description) noexcept;

    // -------------------------------------------------------------------------
    // § 3.2  Querying
    // -------------------------------------------------------------------------

    /**
     * @brief Return the N most-recent incidents (newest first).
     *
     * @param max_count Maximum number of incidents to return; 0 = all.
     * @return Vector of incidents, newest first.
     */
    [[nodiscard]] std::vector<GeoIncident> recentIncidents(
        std::size_t max_count = 0) const noexcept;

    /**
     * @brief Return all incidents with severity >= min_severity.
     *
     * @param min_severity Minimum severity to include.
     * @return Matching incidents, newest first.
     */
    [[nodiscard]] std::vector<GeoIncident> incidentsBySeverity(
        GeoIncidentSeverity min_severity) const noexcept;

    /**
     * @brief Count incidents by severity level.
     *
     * @param severity Severity level to count.
     * @return Number of matching incidents currently in the buffer.
     */
    [[nodiscard]] std::size_t countBySeverity(
        GeoIncidentSeverity severity) const noexcept;

    /**
     * @brief Returns true when any CRITICAL incidents exist in the buffer.
     */
    [[nodiscard]] bool hasCriticalIncidents() const noexcept;

    /**
     * @brief Total number of incidents recorded since construction (monotonic).
     */
    [[nodiscard]] uint64_t totalIncidentCount() const noexcept;

    // -------------------------------------------------------------------------
    // § 3.3  Export
    // -------------------------------------------------------------------------

    /**
     * @brief Format recent incidents as an operator-facing summary string.
     *
     * Produces a multi-line string suitable for log output or runbook
     * diagnostic sections.
     *
     * @param max_count Maximum number of incidents to include (0 = all).
     * @return Formatted summary string.
     */
    [[nodiscard]] std::string formatSummary(std::size_t max_count = 10) const noexcept;

    /**
     * @brief Clear all incidents from the buffer.
     *
     * Should only be used after incidents have been persisted to an external
     * store or after operator sign-off.
     */
    void clearIncidents() noexcept;

private:
    mutable std::mutex          mutex_;
    std::vector<GeoIncident>    incidents_;   ///< Ring buffer (newest at back)
    uint64_t                    total_count_{0};

    static int64_t nowNs() noexcept;
    static GeoIncidentSeverity severityFromId(std::string_view id) noexcept;
    static std::string remediationForId(std::string_view id) noexcept;
};

} // namespace geo
} // namespace themis
