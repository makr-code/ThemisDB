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
    /// High-churn or concurrent update conflict
    CONCURRENCY_INCIDENT = 3605,
    /// Cyclic dependency or circular reference detected
    CYCLE_INCIDENT = 3606,
    /// Malformed input (truncated, invalid structure, bad encoding)
    MALFORMED_INPUT_INCIDENT = 3607,
    /// Missing or invalid target in cross-reference
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
    std::string input_identifier;
    /// Actionable message describing the failure and remediation steps
    std::string actionable_message;

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
    std::string toFormattedMessage() const;

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

} // namespace themis::process
