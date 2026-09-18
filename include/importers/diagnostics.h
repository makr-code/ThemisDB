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

enum class FailureCategory {
    SCHEMA_FAILURE,      ///< Schema validation, type errors, malformed schema
    CONFLICT_FAILURE,    ///< Unresolvable conflict, quality gate failure
    CONNECTOR_FAILURE,   ///< Connection error, timeout, connector unavailable
    CAPACITY_FAILURE,    ///< Quota exceeded, buffer overflow, resource exhaustion
    INTEGRITY_FAILURE    ///< Constraint violation, data corruption, ForeignKey error
};

/**
 * @brief Failure Category To String.
 * @param[in] cat Input parameter.
 * @return Return value.
 */
std::string failureCategoryToString(FailureCategory cat);

struct DiagnosticRecord {
    FailureCategory category;

    uint64_t timestamp_ns;

    ImportErrorCode error_code;

    std::string message;

    std::map<std::string, std::string> context;

    std::string root_cause;

    std::vector<std::string> remediation_steps;

    std::vector<std::string> logs;

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

struct DiagnosticSummary {
    std::string import_id;

    uint64_t import_duration_ms;

    uint64_t total_records_attempted;

    uint64_t failure_count;

    uint64_t warning_count;

    std::map<FailureCategory, uint32_t> failures_by_category;

    std::vector<std::pair<std::string, uint32_t>> top_5_root_causes;

    std::vector<std::string> common_remediation;

    std::vector<DiagnosticRecord> all_diagnostics;

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

DiagnosticRecord produceSchemaDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context);

DiagnosticRecord produceConflictDiagnostic(
    const std::string& reason,
    const std::map<std::string, std::string>& context);

DiagnosticRecord produceConnectorDiagnostic(
    ImportErrorCode error_code,
    const std::map<std::string, std::string>& context);

DiagnosticRecord produceCapacityDiagnostic(
    const std::string& limit_name,
    uint64_t limit_value,
    uint64_t used_value,
    const std::map<std::string, std::string>& context);

DiagnosticRecord produceIntegrityDiagnostic(
    const std::string& violation_type,
    const std::map<std::string, std::string>& context);

/**
 * @brief Aggregate Diagnostics.
 * @param[in] import_id Identifier of the import.
 * @param[in] import_duration_ms Input parameter.
 * @param[in] total_records_attempted Input parameter.
 * @param[in] all_diagnostics Input parameter.
 * @return Return value.
 */
DiagnosticSummary aggregateDiagnostics(
    const std::string& import_id,
    uint64_t import_duration_ms,
    uint64_t total_records_attempted,
    const std::vector<DiagnosticRecord>& all_diagnostics);

}  // namespace importers
}  // namespace themis
