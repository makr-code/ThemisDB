/**
 * @file diagnostics.h
 * @brief Structured failure diagnostics for importer operations
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note PHASE-3-ERROR-HANDLING: Unified diagnostics for all failure types
 * @date 2026-08-02
 *
 * This header defines the unified diagnostic system for Phase 3 error handling.
 * All importer failures (schema, conflict, connector, capacity, integrity) flow
 * through this system to produce actionable diagnostics for operators.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "importer_interface.h"

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Classification of failure types for diagnostic purposes.
 *
 * PHASE-3-ERROR-HANDLING: Unified failure categorization
 * Used to route failures to appropriate diagnostic producers.
 */
enum class FailureCategory {
    SCHEMA_FAILURE,      ///< Schema validation, type errors, malformed schema
    CONFLICT_FAILURE,    ///< Unresolvable conflict, quality gate failure
    CONNECTOR_FAILURE,   ///< Connection error, timeout, connector unavailable
    CAPACITY_FAILURE,    ///< Quota exceeded, buffer overflow, resource exhaustion
    INTEGRITY_FAILURE    ///< Constraint violation, data corruption, ForeignKey error
};

/**
 * @brief Convert FailureCategory to string representation.
 *
 * @param cat Category enum value
 * @return String name suitable for logging
 */
std::string failureCategoryToString(FailureCategory cat);

/**
 * @brief Single diagnostic record for one failure event.
 *
 * PHASE-3-ERROR-HANDLING: Structured diagnostic record
 * Contains all information needed by operators to understand and fix the failure.
 */
struct DiagnosticRecord {
    /// Classification of the failure type
    FailureCategory category;

    /// Nanosecond-precision timestamp when failure was detected
    uint64_t timestamp_ns;

    /// Error code from ImporterErrorCode enum
    ImportErrorCode error_code;

    /// Brief one-line error description
    std::string message;

    /// Structured context fields (connector_name, table_name, row_id, etc.)
    std::map<std::string, std::string> context;

    /// Analysis of why this failure occurred (root cause)
    std::string root_cause;

    /// Ordered list of remediation steps (most likely to work first)
    std::vector<std::string> remediation_steps;

    /// Supporting log lines from audit trail or detailed trace
    std::vector<std::string> logs;

    /**
     * @brief Convert diagnostic record to JSON for monitoring integration.
     * @return JSON representation suitable for APIs
     */
    json toJson() const {
        json remediation_json = json::array();
        for (const auto& step : remediation_steps) {
            remediation_json.push_back(step);
        }

        json logs_json = json::array();
        for (const auto& log : logs) {
            logs_json.push_back(log);
        }

        return json{
            {"category",           failureCategoryToString(category)},
            {"timestamp_ns",       timestamp_ns},
            {"error_code",         static_cast<uint32_t>(error_code)},
            {"message",            message},
            {"context",            context},
            {"root_cause",         root_cause},
            {"remediation_steps",  remediation_json},
            {"logs",               logs_json}
        };
    }
};

/**
 * @brief Aggregated diagnostic summary for an import session.
 *
 * PHASE-3-ERROR-HANDLING: Diagnostic aggregation
 * Provides high-level overview of all failures in an import session,
 * with top root causes and deduplicated remediation steps.
 */
struct DiagnosticSummary {
    /// Unique import session identifier
    std::string import_id;

    /// Total elapsed time for import session (ms)
    uint64_t import_duration_ms;

    /// Total rows attempted to import
    uint64_t total_records_attempted;

    /// Number of failures (errors that stopped processing)
    uint64_t failure_count;

    /// Number of warnings (non-fatal issues)
    uint64_t warning_count;

    /// Failure count breakdown by category
    std::map<FailureCategory, uint32_t> failures_by_category;

    /// Top 5 root causes (cause → frequency count)
    std::vector<std::pair<std::string, uint32_t>> top_5_root_causes;

    /// Deduplicated remediation steps (prioritized by frequency)
    std::vector<std::string> common_remediation;

    /// All individual diagnostic records (full details)
    std::vector<DiagnosticRecord> all_diagnostics;

    /**
     * @brief Convert summary to JSON for monitoring/dashboard integration.
     * @return JSON representation suitable for APIs
     */
    json toJson() const {
        json failures_by_cat = json::object();
        for (const auto& [cat, count] : failures_by_category) {
            failures_by_cat[failureCategoryToString(cat)] = count;
        }

        json top_causes = json::array();
        for (const auto& [cause, count] : top_5_root_causes) {
            top_causes.push_back({{"cause", cause}, {"count", count}});
        }

        json remediation = json::array();
        for (const auto& step : common_remediation) {
            remediation.push_back(step);
        }

        json all_diags = json::array();
        for (const auto& diag : all_diagnostics) {
            all_diags.push_back(diag.toJson());
        }

        return json{
            {"import_id",                import_id},
            {"import_duration_ms",       import_duration_ms},
            {"total_records_attempted",  total_records_attempted},
            {"failure_count",            failure_count},
            {"warning_count",            warning_count},
            {"failures_by_category",     failures_by_cat},
            {"top_5_root_causes",        top_causes},
            {"common_remediation",       remediation},
            {"all_diagnostics",          all_diags}
        };
    }
};

/**
 * @brief Diagnostic producer for schema validation failures.
 *
 * PHASE-3-ERROR-HANDLING: Schema failure diagnostics
 * Generates actionable diagnosis when schema validation fails.
 *
 * @param error_code    ImporterErrorCode that caused the failure
 * @param context       Additional context (table_name, column_name, etc.)
 * @return DiagnosticRecord with root cause and remediation steps
 */
DiagnosticRecord produceSchemaDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context);

/**
 * @brief Diagnostic producer for unresolvable conflicts.
 *
 * PHASE-3-ERROR-HANDLING: Conflict failure diagnostics
 * Generates diagnosis when conflict resolution strategy fails.
 *
 * @param reason        Human-readable reason (e.g., "no matching resolution strategy")
 * @param context       Conflict metadata (table_name, key_values, row_id, etc.)
 * @return DiagnosticRecord with root cause and remediation steps
 */
DiagnosticRecord produceConflictDiagnostic(
    const std::string& reason,
    const std::map<std::string, std::string>& context);

/**
 * @brief Diagnostic producer for connector availability issues.
 *
 * PHASE-3-ERROR-HANDLING: Connector failure diagnostics
 * Generates diagnosis when connector is unavailable or connection fails.
 *
 * @param error_code    ImporterErrorCode (usually IMPORT_CONNECTOR_UNAVAILABLE)
 * @param context       Connector details (connector_name, connection_string, etc.)
 * @return DiagnosticRecord with root cause and remediation steps
 */
DiagnosticRecord produceConnectorDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context);

/**
 * @brief Diagnostic producer for capacity and resource exhaustion failures.
 *
 * PHASE-3-ERROR-HANDLING: Capacity failure diagnostics
 * Generates diagnosis when quota, buffer, or other limits are exceeded.
 *
 * @param limit_name    Name of the limit (e.g., "import_quota", "buffer_size")
 * @param limit_value   Maximum allowed value
 * @param used_value    Actual value used
 * @param context       Additional context
 * @return DiagnosticRecord with root cause and remediation steps
 */
DiagnosticRecord produceCapacityDiagnostic(
    const std::string& limit_name,
    uint64_t limit_value,
    uint64_t used_value,
    const std::map<std::string, std::string>& context);

/**
 * @brief Diagnostic producer for data integrity failures.
 *
 * PHASE-3-ERROR-HANDLING: Integrity failure diagnostics
 * Generates diagnosis when constraint violations or data corruption detected.
 *
 * @param violation_type   Type of violation (e.g., "FOREIGN_KEY", "UNIQUE", "TYPE_MISMATCH")
 * @param context          Violation details (table_name, column, value, etc.)
 * @return DiagnosticRecord with root cause and remediation steps
 */
DiagnosticRecord produceIntegrityDiagnostic(
    const std::string& violation_type,
    const std::map<std::string, std::string>& context);

/**
 * @brief Aggregate all diagnostic records from an import session.
 *
 * PHASE-3-ERROR-HANDLING: Diagnostic aggregation
 * Collects all individual diagnostics and produces summary with:
 *   - Failure count by category
 *   - Top 5 root causes (by frequency)
 *   - Deduplicated remediation steps
 *
 * @param import_id               Unique identifier for import session
 * @param import_duration_ms      Total elapsed time for import
 * @param total_records_attempted Total records processed
 * @param all_diagnostics         All DiagnosticRecord instances from session
 * @return DiagnosticSummary with aggregated analysis
 */
DiagnosticSummary aggregateDiagnostics(
    const std::string& import_id,
    uint64_t import_duration_ms,
    uint64_t total_records_attempted,
    const std::vector<DiagnosticRecord>& all_diagnostics);

}  // namespace importers
}  // namespace themis
