// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file process_diagnostics.h
 * @brief Unified diagnostics framework for process module incidents.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * Provides structured incident classification and diagnostic records for
 * consistent error reporting across process import, validation, retrieval,
 * and linking operations.  All error paths in the process module use this
 * framework to ensure actionable operator-facing diagnostics.
 *
 * @section usage Usage Pattern
 * @code{.cpp}
 *   auto diag = ProcessDiagnostics::createImportIncident(
 *       ProcError::kDeserialiserFailed,
 *       "model_v1.bpmn",
 *       "Invalid gateway type: COMPLEX_AND not supported in v2.0"
 *   );
 *   log_error(diag.toActionableMessage());
 * @endcode
 *
 * @section design Design Constraints
 * - All incidents must include an error code from process_api_contract.h
 * - All incidents must include actionable context (operation, input identifier)
 * - Timestamps are UTC and captured at incident creation
 * - No silent failures; all error paths explicitly signal via incident
 */

#include "process/process_api_contract.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>
#include <map>
#include <shared_mutex>
#include <utility>
#include <vector>

namespace themis::process {

/**
 * @brief Incident classification for process module faults.
 */
enum class DiagnosticIncidentType : int32_t {
    /// Process model import or deserialization failed
    IMPORT_INCIDENT = 3600,
    /// Process model validation or constraint check failed
    VALIDATION_INCIDENT = 3601,
    /// Process retrieval, linking, or context lookup failed
    RETRIEVAL_INCIDENT = 3602,
    /// Process linking state transition or consistency check failed
    LINKING_INCIDENT = 3603,
    /// Parser resource limit exceeded (depth, element count, timeout)
    RESOURCE_INCIDENT = 3604,
    /// Concurrent modification conflict detected during operation
    CONCURRENCY_INCIDENT = 3605,
    /// Cyclic dependency detected in process graph or linking
    CYCLE_INCIDENT = 3606,
    /// Malformed input detected (invalid schema, syntax error)
    MALFORMED_INPUT_INCIDENT = 3607,
    /// Referenced target not found (missing link target, model, etc.)
    MISSING_TARGET_INCIDENT = 3608,
};

/**
 * @brief Convert incident type to human-readable name.
 * @param t The incident type.
 * @return String representation.
 */
std::string_view toString(DiagnosticIncidentType t);

/**
 * @brief Structured diagnostic record for a process module incident.
 *
 * Captures the complete context needed for incident triage:
 * - when it occurred (UTC timestamp)
 * - what class of failure it represents
 * - which operation and input triggered it
 * - the error code from ProcError enum
 * - an actionable message for the operator
 *
 * @invariant All fields are immutable after construction.
 */
class DiagnosticRecord {
public:
    /// UTC timestamp when incident was recorded (milliseconds since epoch)
    int64_t timestamp_ms;
    /// Incident classification
    DiagnosticIncidentType incident_type;
    /// ProcError code from process_api_contract.h
    ProcError error_code;
    /// Operation context (e.g., "deserialize_bpmn", "link_process_instance")
    std::string operation;
    /// Input identifier (e.g., filename, model ID, instance ID)
    std::string input_identifier = {};
    /// Actionable message describing the failure and remediation steps
    std::string actionable_message = {};

    /**
     * @brief Construct a diagnostic record.
     * @param incident_type Classification of the incident.
     * @param error_code ProcError from process_api_contract.h.
     * @param operation Human-readable operation name.
     * @param input_identifier Input identifier (filename, ID, etc.).
     * @param actionable_message Actionable message for operator.
     */
    DiagnosticRecord(
        DiagnosticIncidentType incident_type,
        ProcError error_code,
        std::string_view operation,
        std::string_view input_identifier,
        std::string_view actionable_message
    );

    /**
     * @brief Format the diagnostic record as a structured log message.
     * @return A formatted string suitable for logging.
     *
     * Example output:
     * @code
     * [IMPORT_INCIDENT] model_v1.bpmn (error=7603, ts=2026-08-05T17:53:26Z)
     * Operation: deserialize_bpmn
     * Message: Invalid gateway type: COMPLEX_AND not supported in v2.0
     * @endcode
     */
    virtual ~DiagnosticRecord() = default;

    virtual std::string toFormattedMessage() const;

    /**
     * @brief Get the actionable message (operator-facing).
     * @return The actionable message string.
     */
    std::string_view getActionableMessage() const {
        return actionable_message;
    }

    /**
     * @brief Get the error code.
     * @return The ProcError code.
     */
    ProcError getErrorCode() const {
        return error_code;
    }

    /**
     * @brief Get the incident type.
     * @return The DiagnosticIncidentType.
     */
    DiagnosticIncidentType getIncidentType() const {
        return incident_type;
    }
};

/**
 * @brief Factory for creating diagnostic records.
 *
 * Provides semantic constructors for each incident class to ensure
 * consistent error reporting across the process module.
 */
class ProcessDiagnostics {
public:
    /**
     * @brief Create an import incident diagnostic.
     * @param error ProcError code (typically kDeserialiserFailed).
     * @param input_id Input identifier (filename, model ID).
     * @param message Actionable message for the operator.
     * @return DiagnosticRecord with IMPORT_INCIDENT classification.
     */
    static DiagnosticRecord createImportIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a validation incident diagnostic.
     * @param error ProcError code (typically kSerialiserFailed or kInvalidTransition).
     * @param input_id Input identifier.
     * @param message Actionable message for the operator.
     * @return DiagnosticRecord with VALIDATION_INCIDENT classification.
     */
    static DiagnosticRecord createValidationIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a retrieval incident diagnostic.
     * @param error ProcError code.
     * @param input_id Input identifier (instance ID, model ID).
     * @param message Actionable message for the operator.
     * @return DiagnosticRecord with RETRIEVAL_INCIDENT classification.
     */
    static DiagnosticRecord createRetrievalIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a linking incident diagnostic.
     * @param error ProcError code (typically kInvalidTransition).
     * @param input_id Input identifier (link ID, instance ID).
     * @param message Actionable message for the operator.
     * @return DiagnosticRecord with LINKING_INCIDENT classification.
     */
    static DiagnosticRecord createLinkingIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a resource limit incident diagnostic.
     * @param error ProcError code (typically kExecutionTimeout).
     * @param input_id Input identifier.
     * @param message Actionable message describing which limit was exceeded.
     * @return DiagnosticRecord with RESOURCE_INCIDENT classification.
     *
     * Example message:
     * "Max nesting depth (100) exceeded in sub-process definitions. "
     * "Reduce nesting or split model into separate definitions."
     */
    static DiagnosticRecord createResourceIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a concurrency incident diagnostic.
     * @param error ProcError code (typically kInvalidTransition).
     * @param input_id Input identifier (model ID, instance ID).
     * @param message Actionable message describing the concurrent update conflict.
     * @return DiagnosticRecord with CONCURRENCY_INCIDENT classification.
     */
    static DiagnosticRecord createConcurrencyIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a cyclic dependency incident diagnostic.
     * @param error ProcError code (typically kInvalidTransition).
     * @param input_id Input identifier (link ID, path).
     * @param message Actionable message describing the cycle.
     * @return DiagnosticRecord with CYCLE_INCIDENT classification.
     */
    static DiagnosticRecord createCycleIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a malformed input incident diagnostic.
     * @param error ProcError code (typically kDeserialiserFailed).
     * @param input_id Input identifier (filename, model ID).
     * @param message Actionable message describing the malformation.
     * @return DiagnosticRecord with MALFORMED_INPUT_INCIDENT classification.
     */
    static DiagnosticRecord createMalformedInputIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

    /**
     * @brief Create a missing target incident diagnostic.
     * @param error ProcError code (typically kInvalidTransition).
     * @param input_id Input identifier (link ID, reference).
     * @param message Actionable message describing the missing target.
     * @return DiagnosticRecord with MISSING_TARGET_INCIDENT classification.
     */
    static DiagnosticRecord createMissingTargetIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message
    );

private:
    ProcessDiagnostics() = delete;
};

/**
 * @brief Enhanced diagnostic context for incident triage and operator reporting.
 *
 * Captures the operational state at the time an incident occurs, including
 * resource metrics, stack information, and suggestions for remediation.
 *
 * @section usage Usage Pattern
 * @code{.cpp}
 *   DiagnosticContext ctx;
 *   ctx.recordResourceMetric("parser_depth", 150);
 *   ctx.recordResourceMetric("element_count", 5000);
 *   ctx.setRemediationSuggestion("Consider splitting the model into sub-processes");
 *   LOGGER_ERROR("Incident occurred", ctx.toJson());
 * @endcode
 */
class DiagnosticContext {
public:
    /**
     * @brief Record a resource metric at incident time.
     * @param metric_name The name of the metric (e.g., "parser_depth").
     * @param value The current value of the metric.
     */
    void recordResourceMetric(std::string_view metric_name, int64_t value);

    /**
     * @brief Record a resource limit that was exceeded.
     * @param limit_name The name of the limit (e.g., "max_depth").
     * @param limit_value The configured limit value.
     * @param actual_value The actual value that exceeded the limit.
     */
    void recordLimitExceeded(std::string_view limit_name, int64_t limit_value, int64_t actual_value);

    /**
     * @brief Set a remediation suggestion for the operator.
     * @param suggestion An actionable suggestion to resolve the incident.
     */
    void setRemediationSuggestion(std::string_view suggestion);

    /**
     * @brief Record a conflicting operation that contributed to the incident.
     * @param operation_id The operation ID that caused the conflict.
     * @param conflicting_key The key/resource that was in conflict.
     */
    void recordConflictingOperation(uint64_t operation_id, std::string_view conflicting_key);

    /**
     * @brief Get the full diagnostic context as JSON for logging.
     * @return JSON object with all captured context.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /**
     * @brief Get human-readable remediation summary.
     * @return Formatted string with suggestions for the operator.
     */
    [[nodiscard]] std::string getRemediationSummary() const;

private:
    std::map<std::string, int64_t> resource_metrics_;
    std::vector<std::pair<std::string, std::pair<int64_t, int64_t>>> limit_records_;  // (name, (limit, actual))
    std::vector<std::pair<uint64_t, std::string>> conflicts_;  // (op_id, key)
    std::string remediation_suggestion_;
};

/**
 * @brief Metrics collector for process module performance and incident tracking.
 *
 * Aggregates incident statistics for monitoring and diagnosis.
 */
class DiagnosticMetricsCollector {
public:
    /**
     * @brief Record an incident occurrence.
     * @param incident_type The type of incident.
     */
    void recordIncident(DiagnosticIncidentType incident_type);

    /**
     * @brief Get count of incidents by type.
     * @param incident_type The incident type to query.
     * @return Count of incidents of this type since collector creation.
     */
    [[nodiscard]] uint64_t getIncidentCount(DiagnosticIncidentType incident_type) const;

    /**
     * @brief Get total incident count across all types.
     * @return Total count.
     */
    [[nodiscard]] uint64_t getTotalIncidentCount() const;

    /**
     * @brief Reset all metrics.
     */
    void reset();

    /**
     * @brief Get metrics as JSON.
     * @return JSON object with per-incident-type counts.
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::map<DiagnosticIncidentType, uint64_t> incident_counts_;
    mutable std::shared_mutex metrics_lock_;
};

} // namespace themis::process
