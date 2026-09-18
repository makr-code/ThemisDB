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

enum class GeoIncidentSeverity : uint8_t {
    INFO     = 0, ///< Informational event, no operator action required
    WARNING  = 1, ///< Degraded operation, operator should investigate
    ERROR    = 2, ///< Operation failed with automatic recovery
    CRITICAL = 3, ///< Unrecoverable failure, operator intervention required
};

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

class GeoOperatorDiagnostics {
public:
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

    void recordIncident(
        std::string_view    incident_id,
        GeoIncidentSeverity severity,
        std::string_view    description,
        std::string_view    remediation,
        std::optional<GeoErrorCode> error_code = std::nullopt) noexcept;

    /**
     * @brief Record From Callback.
     * @param[in] incident_id Identifier of the incident.
     * @param[in] description Input parameter.
     * @note Exception safety: noexcept.
     */
    void recordFromCallback(std::string_view incident_id,
                            std::string_view description) noexcept;

    // -------------------------------------------------------------------------
    // § 3.2  Querying
    // -------------------------------------------------------------------------

    [[nodiscard]] std::vector<GeoIncident> recentIncidents(
        std::size_t max_count = 0) const noexcept;

    [[nodiscard]] std::vector<GeoIncident> incidentsBySeverity(
        GeoIncidentSeverity min_severity) const noexcept;

    [[nodiscard]] std::size_t countBySeverity(
        GeoIncidentSeverity severity) const noexcept;

    [[nodiscard]] bool hasCriticalIncidents() const noexcept;

    [[nodiscard]] uint64_t totalIncidentCount() const noexcept;

    // -------------------------------------------------------------------------
    // § 3.3  Export
    // -------------------------------------------------------------------------

    [[nodiscard]] std::string formatSummary(std::size_t max_count = 10) const noexcept;

    /**
     * @brief Clear Incidents.
     * @note Exception safety: noexcept.
     */
    void clearIncidents() noexcept;

private:
    mutable std::mutex          mutex_;
    std::vector<GeoIncident>    incidents_;   ///< Ring buffer (newest at back)
    uint64_t                    total_count_{0};

    /**
     * @brief Now Ns.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static int64_t nowNs() noexcept;
    /**
     * @brief Severity From Id.
     * @param[in] id Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static GeoIncidentSeverity severityFromId(std::string_view id) noexcept;
    /**
     * @brief Remediation For Id.
     * @param[in] id Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static std::string remediationForId(std::string_view id) noexcept;
};

} // namespace geo
} // namespace themis
