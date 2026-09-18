/**
 * @file timeseries_incident_taxonomy.h
 * @brief Unified incident taxonomy and structured diagnostics for timeseries module.
 *
 * This header defines:
 *   - Incident classification (IngestIncident, QueryIncident, LifecycleIncident, IntegrationIncident)
 *   - Severity levels and error codes for each incident class
 *   - Structured incident emission helpers
 *   - Incident metadata and diagnostic context
 *
 * The taxonomy enables uniform error handling across all timeseries paths:
 *   - Ingest: buffer pressure, validation failures, flush timeouts
 *   - Query: range errors, timeout, consistency violations
 *   - Lifecycle: retention policy violations, key rotation failures
 *   - Integration: remote-write failures, metrics export errors
 *
 * ## Design Principles
 *
 * 1. **Explicit Classification**: Every error maps to exactly one incident class.
 * 2. **No Silent Failures**: All errors emit incidents with severity and recovery hints.
 * 3. **Actionable Messages**: Operators can diagnose and recover from incidents.
 * 4. **Bounded Performance**: Incident emission has bounded latency and allocation.
 * 5. **Structured Output**: Incidents carry diagnostic context for logging/tracing.
 *
 * @see include/timeseries/timeseries_api_contract.h — base error taxonomy
 * @see src/timeseries/ROADMAP.md — Phase 3 requirements
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <climits>
#include <string_view>
#include <optional>
#include <chrono>

namespace themis {
namespace timeseries {

// ============================================================================
// § 1  Severity Levels
// ============================================================================

/**
 * @brief Incident severity classification.
 *
 * Severity guides operational response:
 *   - CRITICAL: Immediate action required (e.g., data loss risk)
 *   - ERROR: Failure that degrades service (e.g., buffer overflow, key rotation)
 *   - WARN: Degraded behavior but service continues (e.g., high backpressure)
 *   - INFO: Normal operational event (e.g., retention GC, flush completion)
 */
enum class IncidentSeverity : int {
    CRITICAL  = 0,  ///< Requires immediate remediation (data loss, auth failure)
    ERROR     = 1,  ///< Operation failed, service degraded
    WARN      = 2,  ///< Unusual but recoverable condition
    INFO      = 3,  ///< Normal operational event
};

/**
 * @brief Returns a human-readable string for the given severity.
 */
[[nodiscard]] inline constexpr std::string_view severityName(IncidentSeverity sev) noexcept {
    switch (sev) {
        case IncidentSeverity::CRITICAL: return "CRITICAL";
        case IncidentSeverity::ERROR:    return "ERROR";
        case IncidentSeverity::WARN:     return "WARN";
        case IncidentSeverity::INFO:     return "INFO";
    }
    return "UNKNOWN";
}

// ============================================================================
// § 2  Ingest Incident — Buffer pressure, validation, flush timeouts
// ============================================================================

/**
 * @brief Error codes for ingest-path incidents.
 *
 * Ingest incidents cover:
 *   - Timestamp validation (out-of-order, null)
 *   - Buffer overflow and backpressure
 *   - Flush timeouts and failures
 *   - Series quota violations
 */
enum class IngestIncidentCode : int {
    /// Point timestamp not greater than series tail.
    TIMESTAMP_OUT_OF_ORDER          = 410,

    /// Buffer capacity reached; backpressure applied to writer.
    BUFFER_PRESSURE_HIGH            = 411,

    /// Write queue is overflowing despite backpressure.
    BUFFER_OVERFLOW_IMMINENT        = 412,

    /// Flush operation failed to complete within timeout.
    FLUSH_TIMEOUT                   = 413,

    /// Series limit exceeded for this store.
    SERIES_QUOTA_EXCEEDED           = 414,

    /// Per-series quota exceeded (e.g., max points).
    SERIES_CAPACITY_EXCEEDED        = 415,

    /// Timestamp validation error (e.g., null, negative).
    TIMESTAMP_INVALID               = 416,

    /// Unclassified ingest error.
    INGEST_INTERNAL_ERROR           = 499,
};

/**
 * @brief Returns true when the ingest error is unrecoverable (not worth retry).
 */
[[nodiscard]] inline constexpr bool isHardIngestError(IngestIncidentCode code) noexcept {
    return code == IngestIncidentCode::TIMESTAMP_OUT_OF_ORDER
        || code == IngestIncidentCode::TIMESTAMP_INVALID
        || code == IngestIncidentCode::SERIES_QUOTA_EXCEEDED
        || code == IngestIncidentCode::SERIES_CAPACITY_EXCEEDED
        || code == IngestIncidentCode::INGEST_INTERNAL_ERROR;
}

/**
 * @brief Returns true when backpressure should be applied to writer.
 */
[[nodiscard]] inline constexpr bool isBackpressureError(IngestIncidentCode code) noexcept {
    return code == IngestIncidentCode::BUFFER_PRESSURE_HIGH
        || code == IngestIncidentCode::BUFFER_OVERFLOW_IMMINENT;
}

// ============================================================================
// § 3  Query Incident — Range errors, timeout, consistency
// ============================================================================

/**
 * @brief Error codes for query-path incidents.
 *
 * Query incidents cover:
 *   - Series not found
 *   - Range boundary violations
 *   - Query timeout
 *   - Consistency errors
 *   - Retention boundary crossings
 */
enum class QueryIncidentCode : int {
    /// Named series does not exist or has been dropped.
    SERIES_NOT_FOUND                = 420,

    /// Query range is invalid (e.g., start > end).
    RANGE_INVALID                   = 421,

    /// Data outside retention boundary may be missing.
    RETENTION_BOUNDARY_CROSSED      = 422,

    /// Query execution exceeded timeout.
    QUERY_TIMEOUT                   = 423,

    /// Consistency check failed (e.g., aggregation mismatch).
    CONSISTENCY_CHECK_FAILED        = 424,

    /// Downsampling parameters invalid or unsupported.
    DOWNSAMPLING_INVALID            = 425,

    /// Unclassified query error.
    QUERY_INTERNAL_ERROR            = 499,
};

/**
 * @brief Returns true when the query error is unrecoverable.
 */
[[nodiscard]] inline constexpr bool isHardQueryError(QueryIncidentCode code) noexcept {
    return code == QueryIncidentCode::SERIES_NOT_FOUND
        || code == QueryIncidentCode::RANGE_INVALID
        || code == QueryIncidentCode::DOWNSAMPLING_INVALID
        || code == QueryIncidentCode::QUERY_INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error indicates missing data but query is valid.
 */
[[nodiscard]] inline constexpr bool isDataGapError(QueryIncidentCode code) noexcept {
    return code == QueryIncidentCode::RETENTION_BOUNDARY_CROSSED;
}

// ============================================================================
// § 4  Lifecycle Incident — Retention, key rotation, GC
// ============================================================================

/**
 * @brief Error codes for lifecycle-path incidents.
 *
 * Lifecycle incidents cover:
 *   - Retention policy violations
 *   - Safe deletion failures
 *   - Key rotation errors
 *   - Encryption state validation
 *   - GC and compaction events
 */
enum class LifecycleIncidentCode : int {
    /// Data point is beyond retention and has been deleted.
    RETENTION_EXPIRED               = 430,

    /// Retention policy violation detected (premature removal attempted).
    RETENTION_POLICY_VIOLATION      = 431,

    /// Disk error during safe deletion.
    DELETION_FAILED                 = 432,

    /// Key rotation failed; encryption state invalid.
    ENCRYPTION_ROTATION_FAILURE     = 433,

    /// Encryption key not found for decryption.
    ENCRYPTION_KEY_NOT_FOUND        = 434,

    /// Encryption state validation failed (e.g., IV, ciphertext mismatch).
    ENCRYPTION_STATE_INVALID        = 435,

    /// Garbage collection or compaction failed.
    GC_FAILED                       = 436,

    /// Unclassified lifecycle error.
    LIFECYCLE_INTERNAL_ERROR        = 499,
};

/**
 * @brief Returns true when lifecycle error is unrecoverable.
 */
[[nodiscard]] inline constexpr bool isHardLifecycleError(LifecycleIncidentCode code) noexcept {
    return code == LifecycleIncidentCode::ENCRYPTION_KEY_NOT_FOUND
        || code == LifecycleIncidentCode::ENCRYPTION_STATE_INVALID
        || code == LifecycleIncidentCode::LIFECYCLE_INTERNAL_ERROR;
}

/**
 * @brief Returns true when lifecycle error requires administrative action.
 */
[[nodiscard]] inline constexpr bool isAdminIncident(LifecycleIncidentCode code) noexcept {
    return code == LifecycleIncidentCode::ENCRYPTION_ROTATION_FAILURE
        || code == LifecycleIncidentCode::RETENTION_POLICY_VIOLATION;
}

// ============================================================================
// § 5  Integration Incident — Remote-write, metrics export
// ============================================================================

/**
 * @brief Error codes for integration-path incidents.
 *
 * Integration incidents cover:
 *   - Remote-write validation and transmission
 *   - Prometheus export failures
 *   - Metrics export errors
 *   - Bounded retry behavior
 */
enum class IntegrationIncidentCode : int {
    /// Remote endpoint returned a client error (4xx).
    REMOTE_WRITE_CLIENT_ERROR       = 440,

    /// Remote endpoint returned a server error (5xx).
    REMOTE_WRITE_SERVER_ERROR       = 441,

    /// Remote endpoint unreachable (connection, DNS).
    REMOTE_WRITE_NETWORK_ERROR      = 442,

    /// Remote write validation failed (e.g., schema mismatch).
    REMOTE_WRITE_VALIDATION_ERROR   = 443,

    /// Retry limit exceeded for remote-write operation.
    REMOTE_WRITE_RETRIES_EXHAUSTED  = 444,

    /// Metrics export failed (e.g., format error, quota).
    METRICS_EXPORT_FAILED           = 445,

    /// Unclassified integration error.
    INTEGRATION_INTERNAL_ERROR      = 499,
};

/**
 * @brief Returns true when integration error is retryable (transient).
 */
[[nodiscard]] inline constexpr bool isRetryableIntegrationError(IntegrationIncidentCode code) noexcept {
    return code == IntegrationIncidentCode::REMOTE_WRITE_SERVER_ERROR
        || code == IntegrationIncidentCode::REMOTE_WRITE_NETWORK_ERROR;
}

/**
 * @brief Returns true when integration error is non-retryable (permanent).
 */
[[nodiscard]] inline constexpr bool isPermanentIntegrationError(IntegrationIncidentCode code) noexcept {
    return code == IntegrationIncidentCode::REMOTE_WRITE_CLIENT_ERROR
        || code == IntegrationIncidentCode::REMOTE_WRITE_VALIDATION_ERROR
        || code == IntegrationIncidentCode::REMOTE_WRITE_RETRIES_EXHAUSTED
        || code == IntegrationIncidentCode::INTEGRATION_INTERNAL_ERROR;
}

// ============================================================================
// § 6  Incident Metadata and Context
// ============================================================================

/**
 * @brief Timestamp of incident emission (nanoseconds since epoch).
 */
struct IncidentTimestamp {
    std::int64_t ns_since_epoch;

    /// Constructs with current time.
    [[nodiscard]] static IncidentTimestamp now() noexcept {
        auto tp = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            tp.time_since_epoch()).count();
        return {ns};
    }
};

/**
 * @brief Optional context attached to an incident for diagnostics.
 *
 * @warning **Lifetime contract**: All `std::string_view` fields are non-owning.
 *   Callers MUST ensure the underlying string storage outlives the incident and any
 *   handler invocation that may persist it (e.g., into a log buffer or metrics system).
 *   Passing a temporary `std::string` or a local char array whose lifetime ends before
 *   the handler returns will result in undefined behaviour.
 *   If the incident may outlive the emitting stack frame, store the strings in a
 *   stable buffer (e.g., a `static constexpr` literal or a heap-allocated string) and
 *   construct `IncidentContext` from that stable storage.
 */
struct IncidentContext {
    /// Series name or identifier (if applicable).
    /// Must remain valid for the full duration of any handler that consumes this incident.
    std::string_view series_id;

    /// Human-readable recovery hint (e.g., "increase buffer_capacity" or "check encryption keys").
    /// Must remain valid for the full duration of any handler that consumes this incident.
    std::string_view recovery_hint;

    /// Caller-provided tag for tracing.
    /// Must remain valid for the full duration of any handler that consumes this incident.
    std::string_view caller_tag;

    /// Additional structured data (e.g., metric names, retry count).
    /// Must remain valid for the full duration of any handler that consumes this incident.
    std::string_view extra_info;
};

/**
 * @brief Represents a single incident emission.
 *
 * This struct is used to record incidents uniformly across all paths.
 * Incidents are designed to be lightweight (stack-allocatable) and diagnostic-friendly.
 */
struct Incident {
    /// Which incident class this belongs to (4 classes).
    enum class Class : int {
        INGEST      = 0,
        QUERY       = 1,
        LIFECYCLE   = 2,
        INTEGRATION = 3,
    } incident_class;

    /// Severity of the incident (CRITICAL, ERROR, WARN, INFO).
    IncidentSeverity severity;

    /// Specific error code (mapped to incident class).
    union {
        IngestIncidentCode      ingest_code;
        QueryIncidentCode       query_code;
        LifecycleIncidentCode   lifecycle_code;
        IntegrationIncidentCode integration_code;
    } code;

    /// When the incident was emitted.
    IncidentTimestamp when;

    /// Optional diagnostic context.
    IncidentContext context;

    /// HTTP status code for remote-write/integration errors (0 if N/A).
    int http_status = 0;

    /// Number of retries attempted (for integration incidents).
    int retry_count = 0;

    /// Bounded latency of operation that triggered incident (microseconds).
    std::int64_t operation_duration_us = 0;

    /// Constructs a CRITICAL ingest incident.
    [[nodiscard]] static Incident criticalIngest(IngestIncidentCode code,
                                                  IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INGEST;
        inc.severity = IncidentSeverity::CRITICAL;
        inc.code.ingest_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an ERROR ingest incident.
    [[nodiscard]] static Incident errorIngest(IngestIncidentCode code,
                                              IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INGEST;
        inc.severity = IncidentSeverity::ERROR;
        inc.code.ingest_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a WARN ingest incident.
    [[nodiscard]] static Incident warnIngest(IngestIncidentCode code,
                                             IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INGEST;
        inc.severity = IncidentSeverity::WARN;
        inc.code.ingest_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an INFO ingest incident.
    [[nodiscard]] static Incident infoIngest(IngestIncidentCode code,
                                             IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INGEST;
        inc.severity = IncidentSeverity::INFO;
        inc.code.ingest_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a CRITICAL query incident.
    [[nodiscard]] static Incident criticalQuery(QueryIncidentCode code,
                                                IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::QUERY;
        inc.severity = IncidentSeverity::CRITICAL;
        inc.code.query_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an ERROR query incident.
    [[nodiscard]] static Incident errorQuery(QueryIncidentCode code,
                                             IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::QUERY;
        inc.severity = IncidentSeverity::ERROR;
        inc.code.query_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a WARN query incident.
    [[nodiscard]] static Incident warnQuery(QueryIncidentCode code,
                                            IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::QUERY;
        inc.severity = IncidentSeverity::WARN;
        inc.code.query_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an INFO query incident.
    [[nodiscard]] static Incident infoQuery(QueryIncidentCode code,
                                            IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::QUERY;
        inc.severity = IncidentSeverity::INFO;
        inc.code.query_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a CRITICAL lifecycle incident.
    [[nodiscard]] static Incident criticalLifecycle(LifecycleIncidentCode code,
                                                     IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::LIFECYCLE;
        inc.severity = IncidentSeverity::CRITICAL;
        inc.code.lifecycle_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an ERROR lifecycle incident.
    [[nodiscard]] static Incident errorLifecycle(LifecycleIncidentCode code,
                                                 IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::LIFECYCLE;
        inc.severity = IncidentSeverity::ERROR;
        inc.code.lifecycle_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a WARN lifecycle incident.
    [[nodiscard]] static Incident warnLifecycle(LifecycleIncidentCode code,
                                                IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::LIFECYCLE;
        inc.severity = IncidentSeverity::WARN;
        inc.code.lifecycle_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an INFO lifecycle incident.
    [[nodiscard]] static Incident infoLifecycle(LifecycleIncidentCode code,
                                                IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::LIFECYCLE;
        inc.severity = IncidentSeverity::INFO;
        inc.code.lifecycle_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a CRITICAL integration incident.
    [[nodiscard]] static Incident criticalIntegration(IntegrationIncidentCode code,
                                                      IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INTEGRATION;
        inc.severity = IncidentSeverity::CRITICAL;
        inc.code.integration_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an ERROR integration incident.
    [[nodiscard]] static Incident errorIntegration(IntegrationIncidentCode code,
                                                   IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INTEGRATION;
        inc.severity = IncidentSeverity::ERROR;
        inc.code.integration_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs a WARN integration incident.
    [[nodiscard]] static Incident warnIntegration(IntegrationIncidentCode code,
                                                  IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INTEGRATION;
        inc.severity = IncidentSeverity::WARN;
        inc.code.integration_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }

    /// Constructs an INFO integration incident.
    [[nodiscard]] static Incident infoIntegration(IntegrationIncidentCode code,
                                                  IncidentContext ctx = {}) noexcept {
        Incident inc;
        inc.incident_class = Class::INTEGRATION;
        inc.severity = IncidentSeverity::INFO;
        inc.code.integration_code = code;
        inc.when = IncidentTimestamp::now();
        inc.context = ctx;
        return inc;
    }
};

/**
 * @brief Optional callback function for incident handling.
 *
 * Applications can register an IncidentHandler to receive all emitted incidents
 * for logging, metrics, alerting, or tracing.
 *
 * @param incident The incident being emitted.
 * @see setIncidentHandler()
 */
using IncidentHandler = void (*)(const Incident& incident) noexcept;

/**
 * @brief Registers a global incident handler.
 *
 * The handler will be called for all incidents emitted by timeseries operations.
 * Only one handler is active at a time; registering a new handler replaces the old one.
 *
 * @param handler Callback function, or nullptr to disable incident reporting.
 */
void setIncidentHandler(IncidentHandler handler) noexcept;

/**
 * @brief Retrieves the current global incident handler.
 *
 * @return The active handler, or nullptr if none is registered.
 */
[[nodiscard]] IncidentHandler getIncidentHandler() noexcept;

/**
 * @brief Emits an incident via the registered handler (if any).
 *
 * This function has bounded latency and will not throw. It is safe to call
 * from any path (ingest, query, lifecycle, integration).
 *
 * @param incident The incident to emit.
 */
void emitIncident(const Incident& incident) noexcept;

} // namespace timeseries
} // namespace themis
