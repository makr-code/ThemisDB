// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file ts_operator_diagnostics.h
 * @brief Operator-facing diagnostics for the ThemisDB timeseries module.
 *
 * Provides structured incident reporting for:
 *   - Retention policy violations and flush timeout events
 *   - Remote-write failures and backoff state
 *   - Encryption key rotation lifecycle events
 *   - Buffer pressure and out-of-order flush triggers
 *   - Ingest/query concurrency incidents
 *
 * ## Design
 *
 * TsOperatorDiagnostics aggregates incidents from TsEdgeCaseHandler and other
 * timeseries sub-systems.  It provides query and export APIs for operator runbooks.
 *
 * Thread safety: all public methods are thread-safe.
 *
 * @see include/timeseries/ts_edge_case_handler.h
 * @see src/timeseries/ROADMAP.md — Phase 2/3 Q4 2026 items
 * @see src/timeseries/OPERATOR_GUIDE.md
 */

#pragma once

#include "timeseries/timeseries_api_contract.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace timeseries {

// ============================================================================
// § 1  Severity
// ============================================================================

/// @brief Incident severity levels for the timeseries module.
enum class TsIncidentSeverity : uint8_t {
    INFO     = 0, ///< Informational; no operator action required
    WARNING  = 1, ///< Degraded operation; operator should investigate
    ERROR    = 2, ///< Operation failed; automatic recovery attempted
    CRITICAL = 3, ///< Unrecoverable; operator intervention required
};

/// @brief Returns the string name of a severity level.
inline constexpr std::string_view tsSeverityName(TsIncidentSeverity s) noexcept {
    switch (s) {
        case TsIncidentSeverity::INFO:     return "INFO";
        case TsIncidentSeverity::WARNING:  return "WARNING";
        case TsIncidentSeverity::ERROR:    return "ERROR";
        case TsIncidentSeverity::CRITICAL: return "CRITICAL";
        default:                           return "UNKNOWN";
    }
}

// ============================================================================
// § 2  TsIncident
// ============================================================================

/**
 * @brief A single timeseries module incident record.
 */
struct TsIncident {
    std::string                     incident_id;    ///< Unique incident identifier
    TsIncidentSeverity              severity;       ///< Severity level
    std::string                     description;    ///< Human-readable description
    std::string                     remediation;    ///< Operator action hint
    int64_t                         timestamp_ns;   ///< Unix timestamp (nanoseconds)
    std::optional<TimeseriesErrorCode> error_code;  ///< Associated error code (optional)
};

// ============================================================================
// § 3  TsOperatorDiagnostics
// ============================================================================

/**
 * @brief Operator-facing diagnostics registry for the timeseries module.
 *
 * Collects and exposes timeseries incidents for operator runbooks and dashboards.
 *
 * Usage:
 * @code
 *   TsOperatorDiagnostics diag;
 *   diag.recordIncident("TS-ECH-RW-TIMEOUT", TsIncidentSeverity::WARNING,
 *                       "Remote-write timed out",
 *                       "Check prometheus endpoint availability");
 *   auto summary = diag.formatSummary(10);
 * @endcode
 */
class TsOperatorDiagnostics {
public:
    /// @brief Maximum incidents retained in the ring buffer.
    static constexpr std::size_t kMaxIncidents = 512;

    TsOperatorDiagnostics() = default;

    // Non-copyable; movable.
    TsOperatorDiagnostics(const TsOperatorDiagnostics&) = delete;
    TsOperatorDiagnostics& operator=(const TsOperatorDiagnostics&) = delete;
    TsOperatorDiagnostics(TsOperatorDiagnostics&&) noexcept noexcept = default;
    TsOperatorDiagnostics& operator=(TsOperatorDiagnostics&&) noexcept noexcept = default;

    // -------------------------------------------------------------------------
    // § 3.1  Recording
    // -------------------------------------------------------------------------

    /**
     * @brief Record a structured incident.
     *
     * @param incident_id  Unique incident identifier.
     * @param severity     Severity level.
     * @param description  Human-readable description.
     * @param remediation  Operator action hint.
     * @param error_code   Optional associated TimeseriesErrorCode.
     */
    void recordIncident(
        std::string_view     incident_id,
        TsIncidentSeverity   severity,
        std::string_view     description,
        std::string_view     remediation,
        std::optional<TimeseriesErrorCode> error_code = std::nullopt) noexcept;

    /**
     * @brief Convenience overload for use as a TsEdgeCaseHandler incident callback.
     *
     * Infers severity from the incident_id suffix convention:
     *   - *-CRITICAL / *-PERSISTENT → CRITICAL
     *   - *-TIMEOUT / *-ROTATION-INVALID → ERROR
     *   - *-UNAVAILABLE / *-FALLBACK → WARNING
     *   - other → INFO
     */
    void recordFromCallback(std::string_view incident_id,
                            std::string_view description) noexcept;

    // -------------------------------------------------------------------------
    // § 3.2  Querying
    // -------------------------------------------------------------------------

    /**
     * @brief Return the N most-recent incidents (newest first).
     *
     * @param max_count Maximum to return (0 = all).
     */
    [[nodiscard]] std::vector<TsIncident> recentIncidents(
        std::size_t max_count = 0) const noexcept;

    /**
     * @brief Return all incidents with severity >= min_severity (newest first).
     */
    [[nodiscard]] std::vector<TsIncident> incidentsBySeverity(
        TsIncidentSeverity min_severity) const noexcept;

    /**
     * @brief Count incidents with the given severity.
     */
    [[nodiscard]] std::size_t countBySeverity(TsIncidentSeverity severity) const noexcept;

    /// @brief True when any CRITICAL incidents are in the buffer.
    [[nodiscard]] bool hasCriticalIncidents() const noexcept;

    /// @brief Total incidents recorded since construction (monotonic).
    [[nodiscard]] uint64_t totalIncidentCount() const noexcept;

    // -------------------------------------------------------------------------
    // § 3.3  Export
    // -------------------------------------------------------------------------

    /**
     * @brief Format recent incidents as an operator summary string.
     *
     * @param max_count Maximum incidents to include (0 = all).
     * @return Multi-line formatted summary.
     */
    [[nodiscard]] std::string formatSummary(std::size_t max_count = 10) const noexcept;

    /// @brief Clear all incidents from the buffer.
    void clearIncidents() noexcept;

private:
    mutable std::mutex       mutex_;
    std::vector<TsIncident>  incidents_;
    uint64_t                 total_count_{0};

    static int64_t nowNs() noexcept;
    static TsIncidentSeverity severityFromId(std::string_view id) noexcept;
    static std::string remediationForId(std::string_view id) noexcept;
};

} // namespace timeseries
} // namespace themis
