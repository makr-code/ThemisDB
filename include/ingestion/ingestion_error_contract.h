/**
 * @file ingestion_error_contract.h
 * @brief Error contracts and fail-safe semantics for ingestion components.
 *
 * Phase 3 (Error Handling & Edge Cases) — Error Contracts & Diagnostics
 *
 * Defines:
 * - Unified error codes across all ingestion components
 * - Error categories (transient, permanent, resource exhaustion)
 * - Retry semantics and escape valve policies
 * - ErrorContext with structured information
 * - Fail-safe behavior specifications
 *
 * @see src/ingestion/ROADMAP.md — Phase 3 item
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Unified error codes across all ingestion components
// ============================================================================

/**
 * @brief Comprehensive error code enumeration for all ingestion operations.
 *
 * Error codes are organized by component/category in ranges:
 * - 1000-1099: Source/Connector errors
 * - 1100-1199: Network/HTTP errors
 * - 1200-1299: File/IO errors
 * - 1300-1399: Database errors
 * - 1400-1499: Validation/Schema errors
 * - 1500-1599: Quality/Judge errors
 * - 1600-1699: Workflow/Step errors
 * - 1700-1799: Buffer/Queue/Saturation errors
 * - 1800-1899: Resource exhaustion errors
 * - 1900-1999: Authentication/Authorization errors
 * - 2000-2099: Configuration errors
 * - 2100-2199: Internal/System errors
 *
 * @see isTransientError(), isPermanentError(), isResourceExhaustionError()
 */
enum class IngestionErrorCode : std::uint32_t {
    OK = 0,

    // ── Source / Connector errors (1000-1099) ──────────────────────────────
    SOURCE_NOT_FOUND        = 1000,
    SOURCE_UNAVAILABLE      = 1001,
    SOURCE_NOT_CONFIGURED   = 1002,
    SOURCE_DISABLED         = 1003,
    CONNECTOR_INIT_FAILED   = 1004,
    CONNECTOR_NOT_SUPPORTED = 1005,
    CONNECTOR_DEGRADED      = 1006,  // Running in degraded mode (Phase 3)
    CONNECTOR_ESCAPE_VALVE  = 1007,  // Escape valve activated (Phase 3)

    // ── Network / HTTP errors (1100-1199) ──────────────────────────────────
    HTTP_REQUEST_FAILED     = 1100,
    HTTP_UNAUTHORIZED       = 1101,
    HTTP_NOT_FOUND          = 1102,
    HTTP_RATE_LIMITED       = 1103,
    HTTP_SERVER_ERROR       = 1104,
    HTTP_TIMEOUT            = 1105,
    HTTP_CONNECTION_REFUSED = 1106,
    HTTP_DNS_RESOLUTION_FAILED = 1107,

    // ── File / IO errors (1200-1299) ────────────────────────────────────────
    FILE_NOT_FOUND          = 1200,
    FILE_READ_ERROR         = 1201,
    FILE_FORMAT_UNSUPPORTED = 1202,
    FILE_ENCODING_ERROR     = 1203,
    FILE_PERMISSION_DENIED  = 1204,
    FILE_SIZE_EXCEEDED      = 1205,
    DISK_SPACE_LOW          = 1206,  // Phase 3

    // ── Database errors (1300-1399) ──────────────────────────────────────────
    DATABASE_CONNECTION_FAILED = 1300,
    DATABASE_QUERY_FAILED      = 1301,
    DATABASE_TRANSACTION_FAILED = 1302,
    DATABASE_CONSTRAINT_VIOLATION = 1303,
    DATABASE_DEADLOCK           = 1304,
    DATABASE_TIMEOUT            = 1305,
    DATABASE_READONLY           = 1306,  // Phase 3

    // ── Validation / Schema errors (1400-1499) ──────────────────────────────
    SCHEMA_INVALID            = 1400,
    SCHEMA_MISMATCH           = 1401,
    VALIDATION_FAILED         = 1402,
    VALIDATION_TIMEOUT        = 1403,  // Phase 3
    TYPE_COERCION_FAILED      = 1404,
    DUPLICATE_KEY             = 1405,
    CONSTRAINT_CHECK_FAILED   = 1406,

    // ── Quality / Judge errors (1500-1599) ──────────────────────────────────
    QUALITY_THRESHOLD_FAILED  = 1500,
    QUALITY_CHECK_TIMEOUT     = 1501,  // Phase 3
    QUALITY_INCOMPLETE        = 1502,
    QUALITY_INFERENCE_FAILED  = 1503,
    QUALITY_ESCAPE_VALVE      = 1504,  // Phase 3: fall_open or fail_closed

    // ── Workflow / Step errors (1600-1699) ─────────────────────────────────
    WORKFLOW_STEP_FAILED      = 1600,
    WORKFLOW_STEP_TIMEOUT     = 1601,
    WORKFLOW_CONDITIONAL_FALSE = 1602,
    WORKFLOW_ADAPTER_FAILED   = 1603,
    WORKFLOW_STATE_INVALID    = 1604,

    // ── Buffer / Queue / Saturation errors (1700-1799) ─────────────────────
    BUFFER_FULL              = 1700,
    QUEUE_SATURATED          = 1701,
    BACKPRESSURE_APPLIED     = 1702,  // Not an error, but an advisory
    ENQUEUE_TIMEOUT          = 1703,

    // ── Resource exhaustion errors (1800-1899) ────────────────────────────
    MEMORY_EXHAUSTION        = 1800,
    DISK_QUOTA_EXCEEDED      = 1801,
    CONNECTION_POOL_EXHAUSTED = 1802,  // Phase 3
    THREAD_POOL_EXHAUSTED    = 1803,   // Phase 3
    PROCESS_LIMIT_EXCEEDED   = 1804,   // Phase 3
    CPU_THROTTLE             = 1805,   // Phase 3

    // ── Authentication / Authorization errors (1900-1999) ─────────────────
    AUTH_REQUIRED            = 1900,
    AUTH_FAILED              = 1901,
    AUTH_EXPIRED             = 1902,
    AUTH_INSUFFICIENT_SCOPE  = 1903,
    PERMISSION_DENIED        = 1904,

    // ── Configuration errors (2000-2099) ──────────────────────────────────
    CONFIG_INVALID           = 2000,
    CONFIG_MISSING_REQUIRED  = 2001,
    CONFIG_TYPE_MISMATCH     = 2002,
    CONFIG_OUT_OF_RANGE      = 2003,

    // ── Internal / System errors (2100-2199) ──────────────────────────────
    INTERNAL_ERROR           = 2100,
    NOT_IMPLEMENTED          = 2101,
    UNKNOWN_ERROR            = 2199,
};

// ============================================================================
// Error classification functions
// ============================================================================

/**
 * @brief Check if an error is transient (safe to retry).
 *
 * Transient errors include network timeouts, temporary resource exhaustion,
 * and temporary service unavailability. Retrying may succeed.
 *
 * @param code Error code to classify
 * @return true if the error is transient
 */
bool isTransientError(IngestionErrorCode code);

/**
 * @brief Check if an error is permanent (unsafe to retry).
 *
 * Permanent errors include schema violations, authentication failures,
 * and invalid configuration. Retrying will fail with the same error.
 *
 * @param code Error code to classify
 * @return true if the error is permanent
 */
bool isPermanentError(IngestionErrorCode code);

/**
 * @brief Check if an error is due to resource exhaustion.
 *
 * Resource exhaustion errors include queue saturation, memory limits,
 * and connection pool exhaustion.
 *
 * @param code Error code to classify
 * @return true if the error is resource-related
 */
bool isResourceExhaustionError(IngestionErrorCode code);

/**
 * @brief Check if an error requires backpressure / flow control.
 *
 * Backpressure errors indicate that the component cannot accept more
 * work and should temporarily reduce input rate.
 *
 * @param code Error code to classify
 * @return true if the error indicates backpressure
 */
bool isBackpressureError(IngestionErrorCode code);

/**
 * @brief Get human-readable error message for a code.
 * @param code Error code
 * @return Descriptive message
 */
std::string getErrorMessage(IngestionErrorCode code);

// ============================================================================
// Error context with structured information
// ============================================================================

/**
 * @brief Structured error context captured at point of error.
 *
 * Contains all relevant information about an error for debugging,
 * logging, and diagnostics. Can be serialized to JSON.
 */
struct ErrorContext {
    IngestionErrorCode error_code = IngestionErrorCode::OK;
    std::string error_message;

    // Component context
    std::string component_name;      ///< Component that generated the error
    std::string source_id;           ///< Source being processed
    std::string connector_type;      ///< Connector type (api, filesystem, kafka, etc.)

    // Operation context
    std::string operation;           ///< Operation being performed (enqueue, validate, etc.)
    std::uint64_t retry_count = 0;   ///< Number of retries already attempted
    std::uint64_t item_index = 0;    ///< Index/ID of item being processed

    // Timing context
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();
    std::chrono::milliseconds operation_duration{0};

    // System context
    double memory_usage_percent = 0.0;      ///< System memory usage at time of error
    int64_t available_memory_bytes = 0;     ///< Available memory
    std::string hostname;                   ///< Host where error occurred

    // Nested context for cascading errors
    std::vector<ErrorContext> nested_errors;

    /**
     * @brief Serialize error context to JSON.
     * @return JSON string representation
     */
    std::string toJson() const;

    /**
     * @brief Add a nested error context (for cascading errors).
     * @param nested Error context to nest
     */
    void addNested(const ErrorContext& nested) {
        nested_errors.push_back(nested);
    }
};

// ============================================================================
// Retry semantics and escape valve policies
// ============================================================================

/**
 * @brief Policy for handling errors when retry limit is exceeded.
 */
enum class ErrorEscapeValvePolicy {
    FAIL_CLOSED,   ///< Reject the item (fail-safe behavior)
    FAIL_OPEN,     ///< Accept the item anyway (optimistic)
    DEFER,         ///< Defer to next batch
    QUARANTINE     ///< Move to quarantine queue for manual inspection
};

/**
 * @brief Configuration for error escape valve behavior.
 *
 * Determines what happens when an error occurs and the retry limit
 * is exceeded or a component is degraded.
 */
struct ErrorEscapeValveConfig {
    /**
     * @brief Policy for validation failures.
     * Default: FAIL_CLOSED (reject invalid documents).
     */
    ErrorEscapeValvePolicy validation_escape = ErrorEscapeValvePolicy::FAIL_CLOSED;

    /**
     * @brief Policy for quality check failures.
     * Default: FAIL_CLOSED (reject low-quality documents).
     */
    ErrorEscapeValvePolicy quality_escape = ErrorEscapeValvePolicy::FAIL_CLOSED;

    /**
     * @brief Policy for connector failures.
     * Default: FAIL_CLOSED (skip documents on connector error).
     */
    ErrorEscapeValvePolicy connector_escape = ErrorEscapeValvePolicy::FAIL_CLOSED;

    /**
     * @brief Policy for workflow step failures.
     * Default: DEFER (accumulate failed steps for retry).
     */
    ErrorEscapeValvePolicy workflow_escape = ErrorEscapeValvePolicy::DEFER;

    /**
     * @brief Maximum retries for transient errors.
     * Default: 3 retries.
     */
    int max_transient_retries = 3;

    /**
     * @brief Maximum items to queue for deferred processing.
     * Default: 1000 items.
     */
    std::size_t max_deferred_items = 1000;
};

// ============================================================================
// Fail-safe behavior contract
// ============================================================================

/**
 * @brief Specification of fail-safe behavior for a component.
 *
 * Describes how a component should behave when it encounters errors,
 * is degraded, or is in resource exhaustion conditions.
 */
struct FailSafeContract {
    /**
     * @brief Component name (e.g., "quality_judge", "api_connector").
     */
    std::string component_name;

    /**
     * @brief On unsupported feature, should we fail closed or open?
     *
     * Fail-closed: reject the item and log
     * Fail-open: try best-effort behavior and accept the item
     */
    ErrorEscapeValvePolicy unsupported_feature_policy = ErrorEscapeValvePolicy::FAIL_CLOSED;

    /**
     * @brief On degraded mode, should we continue or abort?
     *
     * Examples of degraded mode:
     * - Quality judge: inference model unavailable, using fallback metric
     * - Connector: API rate-limited, using slow synchronous mode
     * - Database: read-only mode due to maintenance
     */
    bool allow_degraded_mode = true;

    /**
     * @brief On resource exhaustion, should we shed load or fail?
     *
     * true: shed load (drop lower-priority items, apply backpressure)
     * false: fail the operation and return error
     */
    bool allow_load_shedding = true;

    /**
     * @brief On timeout, should we retry or use escape valve?
     *
     * true: retry with exponential backoff
     * false: apply escape valve immediately
     */
    bool allow_timeout_retry = true;
};

}  // namespace ingestion
}  // namespace themis
