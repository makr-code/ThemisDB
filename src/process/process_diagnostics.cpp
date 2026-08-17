// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file process_diagnostics.cpp
 * @brief Implementation of unified diagnostics framework for process module.
 * @version 1.0.0
 */

#include "process/process_diagnostics.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>

namespace themis::process {

// ─────────────────────────────────────────────────────────────────────────────
// toString helper
// ─────────────────────────────────────────────────────────────────────────────

std::string_view toString(DiagnosticIncidentType t) {
    switch (t) {
        case DiagnosticIncidentType::IMPORT_INCIDENT:
            return "IMPORT_INCIDENT";
        case DiagnosticIncidentType::VALIDATION_INCIDENT:
            return "VALIDATION_INCIDENT";
        case DiagnosticIncidentType::RETRIEVAL_INCIDENT:
            return "RETRIEVAL_INCIDENT";
        case DiagnosticIncidentType::LINKING_INCIDENT:
            return "LINKING_INCIDENT";
        case DiagnosticIncidentType::RESOURCE_INCIDENT:
            return "RESOURCE_INCIDENT";
        case DiagnosticIncidentType::CONCURRENCY_INCIDENT:
            return "CONCURRENCY_INCIDENT";
        case DiagnosticIncidentType::CYCLE_INCIDENT:
            return "CYCLE_INCIDENT";
        case DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT:
            return "MALFORMED_INPUT_INCIDENT";
        case DiagnosticIncidentType::MISSING_TARGET_INCIDENT:
            return "MISSING_TARGET_INCIDENT";
    }
    return "UNKNOWN_INCIDENT";
}

// ─────────────────────────────────────────────────────────────────────────────
// DiagnosticRecord implementation
// ─────────────────────────────────────────────────────────────────────────────

DiagnosticRecord::DiagnosticRecord(
    DiagnosticIncidentType incident_type_,
    ProcError error_code_,
    std::string_view operation_,
    std::string_view input_identifier_,
    std::string_view actionable_message_
)
    : timestamp_ms(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    ),
      incident_type(incident_type_),
      error_code(error_code_),
      operation(operation_),
      input_identifier(input_identifier_),
      actionable_message(actionable_message_)
{
}

std::string DiagnosticRecord::toFormattedMessage() const {
    std::ostringstream oss;

    // Format timestamp as ISO8601
    auto seconds = timestamp_ms / 1000;
    auto ms_part = timestamp_ms % 1000;
    std::time_t t = static_cast<std::time_t>(seconds);
    std::tm* tm_info = std::gmtime(&t);

    oss << "[" << toString(incident_type) << "] ";
    oss << input_identifier << " ";
    oss << "(error=" << static_cast<int32_t>(error_code);
    oss << ", ts=";

    // ISO8601 format: YYYY-MM-DDTHH:MM:SS.mmmZ
    char timestamp_buf[32];
    if (tm_info) {
        std::strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        oss << timestamp_buf << "." << std::setw(3) << std::setfill('0') << ms_part << "Z";
    } else {
        oss << "INVALID";
    }
    oss << ")\n";

    oss << "  Operation: " << operation << "\n";
    oss << "  Message: " << actionable_message;

    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// ProcessDiagnostics factory methods
// ─────────────────────────────────────────────────────────────────────────────

DiagnosticRecord ProcessDiagnostics::createImportIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::IMPORT_INCIDENT,
        error,
        "import_process_model",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createValidationIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::VALIDATION_INCIDENT,
        error,
        "validate_process_model",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createRetrievalIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::RETRIEVAL_INCIDENT,
        error,
        "retrieve_process_context",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createLinkingIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::LINKING_INCIDENT,
        error,
        "link_process_instance",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createResourceIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::RESOURCE_INCIDENT,
        error,
        "process_resource_limit",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createConcurrencyIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::CONCURRENCY_INCIDENT,
        error,
        "concurrent_update_conflict",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createCycleIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::CYCLE_INCIDENT,
        error,
        "cyclic_dependency_detected",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createMalformedInputIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::MALFORMED_INPUT_INCIDENT,
        error,
        "parse_malformed_input",
        input_id,
        message
    );
}

DiagnosticRecord ProcessDiagnostics::createMissingTargetIncident(
    ProcError error,
    std::string_view input_id,
    std::string_view message
) {
    return DiagnosticRecord(
        DiagnosticIncidentType::MISSING_TARGET_INCIDENT,
        error,
        "resolve_missing_target",
        input_id,
        message
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// DiagnosticContext Implementation
// ─────────────────────────────────────────────────────────────────────────────

void DiagnosticContext::recordResourceMetric(std::string_view metric_name, int64_t value) {
    resource_metrics_[std::string(metric_name)] = value;
}

void DiagnosticContext::recordLimitExceeded(
    std::string_view limit_name,
    int64_t limit_value,
    int64_t actual_value)
{
    limit_records_.push_back({
        std::string(limit_name),
        {limit_value, actual_value}
    });
}

void DiagnosticContext::setRemediationSuggestion(std::string_view suggestion) {
    remediation_suggestion_ = std::string(suggestion);
}

void DiagnosticContext::recordConflictingOperation(
    uint64_t operation_id,
    std::string_view conflicting_key)
{
    conflicts_.push_back({operation_id, std::string(conflicting_key)});
}

nlohmann::json DiagnosticContext::toJson() const {
    nlohmann::json ctx = nlohmann::json::object();
    
    // Resource metrics
    if (!resource_metrics_.empty()) {
        nlohmann::json metrics = nlohmann::json::object();
        for (const auto& [name, value] : resource_metrics_) {
            metrics[name] = value;
        }
        ctx["resource_metrics"] = metrics;
    }
    
    // Limit records
    if (!limit_records_.empty()) {
        nlohmann::json limits = nlohmann::json::array();
        for (const auto& [name, values] : limit_records_) {
            nlohmann::json record = nlohmann::json::object();
            record["limit_name"] = name;
            record["configured_limit"] = values.first;
            record["actual_value"] = values.second;
            limits.push_back(record);
        }
        ctx["limits_exceeded"] = limits;
    }
    
    // Conflicts
    if (!conflicts_.empty()) {
        nlohmann::json conflict_list = nlohmann::json::array();
        for (const auto& [op_id, key] : conflicts_) {
            nlohmann::json conflict = nlohmann::json::object();
            conflict["operation_id"] = op_id;
            conflict["conflicting_key"] = key;
            conflict_list.push_back(conflict);
        }
        ctx["conflicts"] = conflict_list;
    }
    
    // Remediation
    if (!remediation_suggestion_.empty()) {
        ctx["remediation"] = remediation_suggestion_;
    }
    
    return ctx;
}

std::string DiagnosticContext::getRemediationSummary() const {
    std::ostringstream oss;
    
    if (!remediation_suggestion_.empty()) {
        oss << "SUGGESTED ACTION: " << remediation_suggestion_ << "\n";
    }
    
    if (!limit_records_.empty()) {
        oss << "LIMITS EXCEEDED:\n";
        for (const auto& [name, values] : limit_records_) {
            oss << "  - " << name << ": limit=" << values.first
                << ", actual=" << values.second << "\n";
        }
    }
    
    if (!resource_metrics_.empty()) {
        oss << "RESOURCE SNAPSHOT:\n";
        for (const auto& [name, value] : resource_metrics_) {
            oss << "  - " << name << "=" << value << "\n";
        }
    }
    
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// DiagnosticMetricsCollector Implementation
// ─────────────────────────────────────────────────────────────────────────────

void DiagnosticMetricsCollector::recordIncident(DiagnosticIncidentType incident_type) {
    std::unique_lock<std::shared_mutex> lock(metrics_lock_);
    incident_counts_[incident_type]++;
}

uint64_t DiagnosticMetricsCollector::getIncidentCount(DiagnosticIncidentType incident_type) const {
    std::shared_lock<std::shared_mutex> lock(metrics_lock_);
    auto it = incident_counts_.find(incident_type);
    return (it != incident_counts_.end()) ? it->second : 0;
}

uint64_t DiagnosticMetricsCollector::getTotalIncidentCount() const {
    std::shared_lock<std::shared_mutex> lock(metrics_lock_);
    uint64_t total = 0;
    for (const auto& [type, count] : incident_counts_) {
        total += count;
    }
    return total;
}

void DiagnosticMetricsCollector::reset() {
    std::unique_lock<std::shared_mutex> lock(metrics_lock_);
    incident_counts_.clear();
}

nlohmann::json DiagnosticMetricsCollector::toJson() const {
    std::shared_lock<std::shared_mutex> lock(metrics_lock_);
    nlohmann::json metrics = nlohmann::json::object();
    
    for (const auto& [type, count] : incident_counts_) {
        std::string type_name = std::string(toString(type));
        metrics[type_name] = count;
    }
    
    return metrics;
}

} // namespace themis::process
