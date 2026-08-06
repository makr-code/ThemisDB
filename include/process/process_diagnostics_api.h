// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

/**
 * @file process_diagnostics_api.h
 * @brief Extended diagnostics framework with incident context and trace support.
 * @version 1.1.0
 *
 * @section purpose Purpose
 * Extends the base diagnostics framework (process_diagnostics.h) with additional
 * context for high-churn scenarios, distributed tracing, and advanced incident
 * classification. Enables operator-facing diagnostics for process incident triage.
 *
 * @section context_layers Context Layers
 *
 * Diagnostics now include four layers of context:
 * 1. **Incident Context:** Classification of failure class (CHURN_DETECTION, etc.)
 * 2. **Trace Context:** Correlation IDs for multi-step operations (trace_id, span_id)
 * 3. **Churn Metric:** Concurrent operation count at time of incident
 * 4. **Conflict Analysis:** Count and type of detected conflicts
 *
 * @section usage Usage Example
 *
 * @code{.cpp}
 * // Create a churn-related incident with trace context
 * auto diag = ProcessDiagnostics::createChurnIncident(
 *     ProcError::kValidationFailed,
 *     "model_large_v2",
 *     "Model update failed due to high concurrent churn (>500 ops/sec). "
 *     "Retry with exponential backoff.",
 *     concurrent_ops = 625,
 *     trace_id = "trace-2026-08-06-t1234567890"
 * );
 *
 * // Create a conflict-detection incident
 * auto diag2 = ProcessDiagnostics::createConflictIncident(
 *     ProcError::kValidationFailed,
 *     "model_v1",
 *     "Write-write conflict detected. Retry with latest version.",
 *     conflict_count = 3,
 *     trace_id = "trace-xyz"
 * );
 *
 * // Access trace context for distributed tracing
 * if (diag.trace_id) {
 *     correlate_with_distributed_trace(diag.trace_id.value());
 * }
 *
 * // Check churn metric to detect high-contention scenarios
 * if (diag.churn_metric && diag.churn_metric.value() > 500) {
 *     alert("High model churn detected; consider request throttling");
 * }
 * @endcode
 *
 * @section backward_compatibility Backward Compatibility
 *
 * This version extends process_diagnostics.h with optional fields. Existing
 * code using base DiagnosticRecord continues to work; new context fields
 * are optional (std::optional).
 *
 * @section contract_version Contract Version
 * v1.1.0 – Extends v1.0.0 with optional context fields (backward compatible)
 */

#include "process/process_api_contract.h"
#include "process/process_diagnostics.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace themis::process {

/**
 * @brief Incident context classification for high-churn scenarios.
 *
 * Provides finer-grained classification of failure modes beyond the base
 * DiagnosticIncidentType enum.
 */
enum class IncidentContext : int32_t {
    /// High model update rate (>100 updates/sec) detected
    CHURN_DETECTION = 3700,
    /// Memory, file handles, or timeout limit exceeded
    RESOURCE_EXHAUSTION = 3701,
    /// Write-write conflict or link staleness detected
    CONFLICT_DETECTED = 3702,
    /// Constraint or schema validation failure
    VALIDATION_FAILURE = 3703,
    /// Operation deadline exceeded
    TIMEOUT_EXCEEDED = 3704,
};

/**
 * @brief Convert IncidentContext to human-readable name.
 * @param c The incident context.
 * @return String representation.
 */
std::string_view toString(IncidentContext c);

/**
 * @brief Trace context for correlating multi-step operations.
 *
 * Enables distributed tracing and request correlation across multiple
 * process module operations (import→validate→link).
 *
 * Fields are optional to support both traced and non-traced deployments.
 *
 * @invariant trace_id and span_id, if present, are globally unique identifiers
 * @invariant trace_id is shared across all operations in a trace
 * @invariant span_id is unique within a trace
 */
struct TraceContext {
    /// Globally unique trace identifier (e.g., UUID, OpenTelemetry trace ID)
    std::optional<std::string> trace_id;
    /// Operation-specific span identifier within the trace
    std::optional<std::string> span_id;

    /**
     * @brief Format trace context as a string for logging.
     * @return "[trace_id=X, span_id=Y]" or empty string if no context
     */
    [[nodiscard]] std::string format() const;

    /**
     * @brief Check if trace context is present.
     * @return true if either trace_id or span_id is set
     */
    [[nodiscard]] bool isPresent() const {
        return trace_id.has_value() || span_id.has_value();
    }
};

/**
 * @brief Conflict analysis captured at time of incident.
 *
 * Tracks write-write conflicts and retry counts to help operators
 * understand contention patterns.
 *
 * @invariant conflict_count >= 0
 * @invariant retry_count >= 0
 * @invariant Both fields are optional for non-conflict incidents
 */
struct ConflictAnalysis {
    /// Number of conflicts detected during this operation
    std::optional<int32_t> conflict_count;
    /// Number of retry attempts before failure
    std::optional<int32_t> retry_count;
    /// Monotonic version clock of conflicting update (if available)
    std::optional<int64_t> conflicting_version;

    /**
     * @brief Format conflict analysis as a string.
     * @return "conflicts=N, retries=M, version=V" or empty string if no data
     */
    [[nodiscard]] std::string format() const;
};

/**
 * @brief Extended diagnostic record with high-churn and tracing context.
 *
 * Backward-compatible extension of DiagnosticRecord from process_diagnostics.h.
 * Adds optional context for high-churn scenarios and distributed tracing.
 *
 * @invariant Base DiagnosticRecord fields (timestamp_ms, incident_type, error_code, etc.) are always present
 * @invariant Extended fields (trace_context, churn_metric, conflict_analysis) are optional
 * @invariant All fields are immutable after construction
 */
class ExtendedDiagnosticRecord : public DiagnosticRecord {
public:
    /**
     * @brief Construct an extended diagnostic record.
     *
     * @param incident_type Classification from DiagnosticIncidentType
     * @param error_code ProcError from process_api_contract.h
     * @param operation Operation name (e.g., "update_model")
     * @param input_identifier Input identifier (model ID, instance ID)
     * @param actionable_message Message for the operator
     * @param trace_context Optional trace context for distributed tracing
     * @param incident_context Optional finer-grained context (CHURN_DETECTION, etc.)
     * @param churn_metric Optional concurrent operation count at time of incident
     * @param conflict_analysis Optional conflict tracking data
     */
    ExtendedDiagnosticRecord(
        DiagnosticIncidentType incident_type,
        ProcError error_code,
        std::string_view operation,
        std::string_view input_identifier,
        std::string_view actionable_message,
        const TraceContext& trace_context = TraceContext{},
        std::optional<IncidentContext> incident_context = std::nullopt,
        std::optional<int32_t> churn_metric = std::nullopt,
        const ConflictAnalysis& conflict_analysis = ConflictAnalysis{}
    );

    /// Finer-grained incident context (CHURN_DETECTION, CONFLICT_DETECTED, etc.)
    std::optional<IncidentContext> incident_context;
    /// Trace context for distributed tracing
    TraceContext trace_context;
    /// Concurrent operation count at time of incident (>100 indicates high churn)
    std::optional<int32_t> churn_metric;
    /// Conflict analysis (conflict_count, retry_count, conflicting_version)
    ConflictAnalysis conflict_analysis;

    /**
     * @brief Format the extended diagnostic record as a structured log message.
     * @return A formatted string with all available context layers
     *
     * Example output:
     * @code
     * [IMPORT_INCIDENT] model_v1.bpmn (error=7603, ts=2026-08-06T17:53:26Z)
     * Context: CHURN_DETECTION [churn=625 ops/sec]
     * Trace: trace-2026-08-06-t1234567890 / span-xyz
     * Operation: deserialize_bpmn
     * Message: High model churn (>500 ops/sec). Retry with exponential backoff.
     * Conflicts: 3 detected, 2 retries
     * @endcode
     */
    [[nodiscard]] std::string toFormattedMessage() const override;

    /**
     * @brief Check if this is a high-churn incident.
     * @return true if churn_metric > 500
     */
    [[nodiscard]] bool isHighChurn() const {
        return churn_metric.has_value() && churn_metric.value() > 500;
    }

    /**
     * @brief Check if this is a conflict-related incident.
     * @return true if incident_context == CONFLICT_DETECTED
     */
    [[nodiscard]] bool isConflictIncident() const {
        return incident_context.has_value() &&
               incident_context.value() == IncidentContext::CONFLICT_DETECTED;
    }
};

/**
 * @brief Extended factory for creating diagnostics with high-churn and tracing context.
 *
 * Extends ProcessDiagnostics from process_diagnostics.h with additional factory
 * methods for churn, conflict, and traced incidents.
 */
class ExtendedProcessDiagnostics {
public:
    /**
     * @brief Create a churn-detection incident diagnostic.
     *
     * Use when high model update rate is detected or causes operation failure.
     *
     * @param error ProcError code (typically kValidationFailed)
     * @param input_id Input identifier (model ID, instance ID)
     * @param message Actionable message (recommend backoff, batching, etc.)
     * @param concurrent_ops Concurrent operation count at time of incident
     * @param trace_id Optional trace ID for correlation
     * @return ExtendedDiagnosticRecord with CHURN_DETECTION context
     *
     * Example:
     * @code
     * auto diag = ExtendedProcessDiagnostics::createChurnIncident(
     *     ProcError::kValidationFailed,
     *     "model_large_v2",
     *     "Model update failed: high concurrent churn (625 ops/sec). "
     *     "Retry with exponential backoff (2^n seconds).",
     *     concurrent_ops = 625,
     *     trace_id = "trace-2026-08-06-xyz"
     * );
     * @endcode
     */
    [[nodiscard]] static ExtendedDiagnosticRecord createChurnIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message,
        int32_t concurrent_ops,
        std::optional<std::string_view> trace_id = std::nullopt
    );

    /**
     * @brief Create a conflict-detection incident diagnostic.
     *
     * Use when write-write conflicts, link staleness, or version mismatches occur.
     *
     * @param error ProcError code (typically kValidationFailed or kLinkingFailed)
     * @param input_id Input identifier (model ID, link ID)
     * @param message Actionable message (recommend retry with latest version)
     * @param conflict_count Number of conflicts detected
     * @param retry_count Number of retry attempts before failure
     * @param trace_id Optional trace ID for correlation
     * @return ExtendedDiagnosticRecord with CONFLICT_DETECTED context
     *
     * Example:
     * @code
     * auto diag = ExtendedProcessDiagnostics::createConflictIncident(
     *     ProcError::kValidationFailed,
     *     "model_v1",
     *     "Write-write conflict detected. Fetch latest version and retry.",
     *     conflict_count = 3,
     *     retry_count = 2,
     *     trace_id = "trace-conflict-xyz"
     * );
     * @endcode
     */
    [[nodiscard]] static ExtendedDiagnosticRecord createConflictIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message,
        int32_t conflict_count,
        int32_t retry_count,
        std::optional<std::string_view> trace_id = std::nullopt
    );

    /**
     * @brief Create a traced incident diagnostic.
     *
     * Use when an incident occurs within a distributed trace context.
     * Attaches trace_id and span_id for correlation.
     *
     * @param base_incident Base DiagnosticRecord (from ProcessDiagnostics factory)
     * @param trace_id Trace identifier (OpenTelemetry format recommended)
     * @param span_id Span identifier within the trace
     * @return ExtendedDiagnosticRecord with trace context attached
     *
     * Example:
     * @code
     * auto base = ProcessDiagnostics::createImportIncident(
     *     ProcError::kDeserialiserFailed,
     *     "model.bpmn",
     *     "Invalid gateway type"
     * );
     * auto traced = ExtendedProcessDiagnostics::createTracedIncident(
     *     base,
     *     "4bf92f3577b34da6a3ce929d0e0e4736",  // trace_id
     *     "00f067aa0ba902b7"                   // span_id
     * );
     * @endcode
     */
    [[nodiscard]] static ExtendedDiagnosticRecord createTracedIncident(
        const DiagnosticRecord& base_incident,
        std::string_view trace_id,
        std::string_view span_id
    );

    /**
     * @brief Create a resource-exhaustion incident diagnostic.
     *
     * Use when memory, file handles, or timeout limits are exceeded.
     *
     * @param error ProcError code (typically kExecutionTimeout, kMaxDepthExceeded, etc.)
     * @param input_id Input identifier (model ID, instance ID)
     * @param message Actionable message describing the limit
     * @param resource_type Name of resource that was exhausted ("memory", "timeout", "depth")
     * @param trace_id Optional trace ID for correlation
     * @return ExtendedDiagnosticRecord with RESOURCE_EXHAUSTION context
     */
    [[nodiscard]] static ExtendedDiagnosticRecord createResourceExhaustionIncident(
        ProcError error,
        std::string_view input_id,
        std::string_view message,
        std::string_view resource_type,
        std::optional<std::string_view> trace_id = std::nullopt
    );

private:
    ExtendedProcessDiagnostics() = delete;
};

} // namespace themis::process
