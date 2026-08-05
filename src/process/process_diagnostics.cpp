// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file process_diagnostics.cpp
 * @brief Implementation of unified diagnostics framework for process module.
 * @version 1.0.0
 */

#include "process/process_diagnostics.h"

#include <chrono>
#include <iomanip>
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

    // ISO8601 format: YYYY-MM-DDTHH:MM:SSZ
    char timestamp_buf[32];
    if (tm_info) {
        std::strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%dT%H:%M:%SZ", tm_info);
        oss << timestamp_buf;
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

} // namespace themis::process
