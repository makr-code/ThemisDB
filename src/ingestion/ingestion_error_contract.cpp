/**
 * @file ingestion_error_contract.cpp
 * @brief Implementation of error contracts and fail-safe semantics.
 *
 * Phase 3 (Error Handling & Edge Cases) — Error Contracts & Diagnostics
 */

#include "ingestion/ingestion_error_contract.h"

#include <sstream>

namespace themis {
namespace ingestion {

// ============================================================================
// Error classification functions
// ============================================================================

bool isTransientError(IngestionErrorCode code) {
    switch (code) {
        // Network timeouts and temporary failures (retryable)
        case IngestionErrorCode::HTTP_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_CONNECTION_REFUSED:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_SERVER_ERROR:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_RATE_LIMITED:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_DNS_RESOLUTION_FAILED:
        [[fallthrough]];
        case IngestionErrorCode::DATABASE_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::DATABASE_DEADLOCK:
        [[fallthrough]];
        case IngestionErrorCode::SOURCE_UNAVAILABLE:
        [[fallthrough]];
        case IngestionErrorCode::CONNECTOR_DEGRADED:

        // Temporary resource constraints
        [[fallthrough]];
        case IngestionErrorCode::QUEUE_SATURATED:
        [[fallthrough]];
        case IngestionErrorCode::BUFFER_FULL:
        [[fallthrough]];
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::CPU_THROTTLE:

        // Quality check timeout (potentially retryable)
        [[fallthrough]];
        case IngestionErrorCode::QUALITY_CHECK_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::VALIDATION_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::WORKFLOW_STEP_TIMEOUT:
            return true;

        default:
            return false;
    }
}

bool isPermanentError(IngestionErrorCode code) {
    switch (code) {
        // Validation errors (permanent)
        case IngestionErrorCode::SCHEMA_INVALID:
        [[fallthrough]];
        case IngestionErrorCode::SCHEMA_MISMATCH:
        [[fallthrough]];
        case IngestionErrorCode::VALIDATION_FAILED:
        [[fallthrough]];
        case IngestionErrorCode::DUPLICATE_KEY:
        [[fallthrough]];
        case IngestionErrorCode::CONSTRAINT_CHECK_FAILED:
        [[fallthrough]];
        case IngestionErrorCode::TYPE_COERCION_FAILED:

        // Configuration errors (permanent)
        [[fallthrough]];
        case IngestionErrorCode::CONFIG_INVALID:
        [[fallthrough]];
        case IngestionErrorCode::CONFIG_MISSING_REQUIRED:
        [[fallthrough]];
        case IngestionErrorCode::CONFIG_TYPE_MISMATCH:
        [[fallthrough]];
        case IngestionErrorCode::CONFIG_OUT_OF_RANGE:

        // Authentication/Authorization errors (permanent)
        [[fallthrough]];
        case IngestionErrorCode::AUTH_REQUIRED:
        [[fallthrough]];
        case IngestionErrorCode::AUTH_FAILED:
        [[fallthrough]];
        case IngestionErrorCode::AUTH_EXPIRED:
        [[fallthrough]];
        case IngestionErrorCode::AUTH_INSUFFICIENT_SCOPE:
        [[fallthrough]];
        case IngestionErrorCode::PERMISSION_DENIED:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_UNAUTHORIZED:

        // File/Source not found (permanent)
        [[fallthrough]];
        case IngestionErrorCode::FILE_NOT_FOUND:
        [[fallthrough]];
        case IngestionErrorCode::SOURCE_NOT_FOUND:
        [[fallthrough]];
        case IngestionErrorCode::SOURCE_NOT_CONFIGURED:
        [[fallthrough]];
        case IngestionErrorCode::CONNECTOR_NOT_SUPPORTED:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_NOT_FOUND:

        // Format/Codec errors (permanent for this item)
        [[fallthrough]];
        case IngestionErrorCode::FILE_FORMAT_UNSUPPORTED:
        [[fallthrough]];
        case IngestionErrorCode::FILE_ENCODING_ERROR:

        // Quality failures (permanent for this item)
        [[fallthrough]];
        case IngestionErrorCode::QUALITY_THRESHOLD_FAILED:

        // Workflow errors (permanent for this item)
        [[fallthrough]];
        case IngestionErrorCode::WORKFLOW_CONDITIONAL_FALSE:
        [[fallthrough]];
        case IngestionErrorCode::WORKFLOW_ADAPTER_FAILED:

        // System errors (permanent)
        [[fallthrough]];
        case IngestionErrorCode::NOT_IMPLEMENTED:
        [[fallthrough]];
        case IngestionErrorCode::INTERNAL_ERROR:
            return true;

        default:
            return false;
    }
}

bool isResourceExhaustionError(IngestionErrorCode code) {
    switch (code) {
        case IngestionErrorCode::MEMORY_EXHAUSTION:
        [[fallthrough]];
        case IngestionErrorCode::DISK_QUOTA_EXCEEDED:
        [[fallthrough]];
        case IngestionErrorCode::DISK_SPACE_LOW:
        [[fallthrough]];
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::PROCESS_LIMIT_EXCEEDED:
        [[fallthrough]];
        case IngestionErrorCode::CPU_THROTTLE:
        [[fallthrough]];
        case IngestionErrorCode::QUEUE_SATURATED:
        [[fallthrough]];
        case IngestionErrorCode::BUFFER_FULL:
        [[fallthrough]];
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
            return true;

        default:
            return false;
    }
}

bool isBackpressureError(IngestionErrorCode code) {
    switch (code) {
        case IngestionErrorCode::QUEUE_SATURATED:
        [[fallthrough]];
        case IngestionErrorCode::BUFFER_FULL:
        [[fallthrough]];
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
        [[fallthrough]];
        case IngestionErrorCode::BACKPRESSURE_APPLIED:
        [[fallthrough]];
        case IngestionErrorCode::HTTP_RATE_LIMITED:
        [[fallthrough]];
        case IngestionErrorCode::MEMORY_EXHAUSTION:
        [[fallthrough]];
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
        [[fallthrough]];
        case IngestionErrorCode::CPU_THROTTLE:
            return true;

        default:
            return false;
    }
}

std::string getErrorMessage(IngestionErrorCode code) {
    switch (code) {
        case IngestionErrorCode::OK:
            return "Success";

        // Source / Connector errors
        case IngestionErrorCode::SOURCE_NOT_FOUND:
            return "Source not found";
        case IngestionErrorCode::SOURCE_UNAVAILABLE:
            return "Source temporarily unavailable";
        case IngestionErrorCode::SOURCE_NOT_CONFIGURED:
            return "Source not configured";
        case IngestionErrorCode::SOURCE_DISABLED:
            return "Source is disabled";
        case IngestionErrorCode::CONNECTOR_INIT_FAILED:
            return "Connector initialization failed";
        case IngestionErrorCode::CONNECTOR_NOT_SUPPORTED:
            return "Connector type not supported";
        case IngestionErrorCode::CONNECTOR_DEGRADED:
            return "Connector running in degraded mode";
        case IngestionErrorCode::CONNECTOR_ESCAPE_VALVE:
            return "Connector escape valve activated";

        // Network / HTTP errors
        case IngestionErrorCode::HTTP_REQUEST_FAILED:
            return "HTTP request failed";
        case IngestionErrorCode::HTTP_UNAUTHORIZED:
            return "HTTP 401: Unauthorized";
        case IngestionErrorCode::HTTP_NOT_FOUND:
            return "HTTP 404: Not found";
        case IngestionErrorCode::HTTP_RATE_LIMITED:
            return "HTTP 429: Rate limited";
        case IngestionErrorCode::HTTP_SERVER_ERROR:
            return "HTTP 5xx: Server error";
        case IngestionErrorCode::HTTP_TIMEOUT:
            return "HTTP request timeout";
        case IngestionErrorCode::HTTP_CONNECTION_REFUSED:
            return "HTTP connection refused";
        case IngestionErrorCode::HTTP_DNS_RESOLUTION_FAILED:
            return "HTTP DNS resolution failed";

        // File / IO errors
        case IngestionErrorCode::FILE_NOT_FOUND:
            return "File not found";
        case IngestionErrorCode::FILE_READ_ERROR:
            return "File read error";
        case IngestionErrorCode::FILE_FORMAT_UNSUPPORTED:
            return "File format not supported";
        case IngestionErrorCode::FILE_ENCODING_ERROR:
            return "File encoding error";
        case IngestionErrorCode::FILE_PERMISSION_DENIED:
            return "File permission denied";
        case IngestionErrorCode::FILE_SIZE_EXCEEDED:
            return "File size exceeded";
        case IngestionErrorCode::DISK_SPACE_LOW:
            return "Disk space low";

        // Database errors
        case IngestionErrorCode::DATABASE_CONNECTION_FAILED:
            return "Database connection failed";
        case IngestionErrorCode::DATABASE_QUERY_FAILED:
            return "Database query failed";
        case IngestionErrorCode::DATABASE_TRANSACTION_FAILED:
            return "Database transaction failed";
        case IngestionErrorCode::DATABASE_CONSTRAINT_VIOLATION:
            return "Database constraint violation";
        case IngestionErrorCode::DATABASE_DEADLOCK:
            return "Database deadlock detected";
        case IngestionErrorCode::DATABASE_TIMEOUT:
            return "Database operation timeout";
        case IngestionErrorCode::DATABASE_READONLY:
            return "Database is read-only";

        // Validation / Schema errors
        case IngestionErrorCode::SCHEMA_INVALID:
            return "Schema validation failed";
        case IngestionErrorCode::SCHEMA_MISMATCH:
            return "Schema mismatch";
        case IngestionErrorCode::VALIDATION_FAILED:
            return "Validation failed";
        case IngestionErrorCode::VALIDATION_TIMEOUT:
            return "Validation timeout";
        case IngestionErrorCode::TYPE_COERCION_FAILED:
            return "Type coercion failed";
        case IngestionErrorCode::DUPLICATE_KEY:
            return "Duplicate key";
        case IngestionErrorCode::CONSTRAINT_CHECK_FAILED:
            return "Constraint check failed";

        // Quality / Judge errors
        case IngestionErrorCode::QUALITY_THRESHOLD_FAILED:
            return "Quality threshold not met";
        case IngestionErrorCode::QUALITY_CHECK_TIMEOUT:
            return "Quality check timeout";
        case IngestionErrorCode::QUALITY_INCOMPLETE:
            return "Quality assessment incomplete";
        case IngestionErrorCode::QUALITY_INFERENCE_FAILED:
            return "Quality inference failed";
        case IngestionErrorCode::QUALITY_ESCAPE_VALVE:
            return "Quality escape valve activated";

        // Workflow / Step errors
        case IngestionErrorCode::WORKFLOW_STEP_FAILED:
            return "Workflow step failed";
        case IngestionErrorCode::WORKFLOW_STEP_TIMEOUT:
            return "Workflow step timeout";
        case IngestionErrorCode::WORKFLOW_CONDITIONAL_FALSE:
            return "Workflow conditional is false";
        case IngestionErrorCode::WORKFLOW_ADAPTER_FAILED:
            return "Workflow adapter failed";
        case IngestionErrorCode::WORKFLOW_STATE_INVALID:
            return "Workflow state invalid";

        // Buffer / Queue / Saturation errors
        case IngestionErrorCode::BUFFER_FULL:
            return "Buffer is full";
        case IngestionErrorCode::QUEUE_SATURATED:
            return "Queue is saturated";
        case IngestionErrorCode::BACKPRESSURE_APPLIED:
            return "Backpressure applied";
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
            return "Enqueue timeout";

        // Resource exhaustion errors
        case IngestionErrorCode::MEMORY_EXHAUSTION:
            return "Memory exhausted";
        case IngestionErrorCode::DISK_QUOTA_EXCEEDED:
            return "Disk quota exceeded";
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
            return "Connection pool exhausted";
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
            return "Thread pool exhausted";
        case IngestionErrorCode::PROCESS_LIMIT_EXCEEDED:
            return "Process limit exceeded";
        case IngestionErrorCode::CPU_THROTTLE:
            return "CPU throttle applied";

        // Authentication / Authorization errors
        case IngestionErrorCode::AUTH_REQUIRED:
            return "Authentication required";
        case IngestionErrorCode::AUTH_FAILED:
            return "Authentication failed";
        case IngestionErrorCode::AUTH_EXPIRED:
            return "Authentication token expired";
        case IngestionErrorCode::AUTH_INSUFFICIENT_SCOPE:
            return "Insufficient authentication scope";
        case IngestionErrorCode::PERMISSION_DENIED:
            return "Permission denied";

        // Configuration errors
        case IngestionErrorCode::CONFIG_INVALID:
            return "Configuration invalid";
        case IngestionErrorCode::CONFIG_MISSING_REQUIRED:
            return "Required configuration missing";
        case IngestionErrorCode::CONFIG_TYPE_MISMATCH:
            return "Configuration type mismatch";
        case IngestionErrorCode::CONFIG_OUT_OF_RANGE:
            return "Configuration value out of range";

        // Internal / System errors
        case IngestionErrorCode::INTERNAL_ERROR:
            return "Internal error";
        case IngestionErrorCode::NOT_IMPLEMENTED:
            return "Feature not implemented";
        case IngestionErrorCode::UNKNOWN_ERROR:
            return "Unknown error";

        default:
            return "Unrecognized error code: " + std::to_string(static_cast<uint32_t>(code));
    }
}

// ============================================================================
// ErrorContext JSON serialization
// ============================================================================

std::string ErrorContext::toJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"error_code\":" << static_cast<uint32_t>(error_code) << ","
        << "\"error_message\":\"" << error_message << "\","
        << "\"component_name\":\"" << component_name << "\","
        << "\"source_id\":\"" << source_id << "\","
        << "\"connector_type\":\"" << connector_type << "\","
        << "\"operation\":\"" << operation << "\","
        << "\"retry_count\":" << retry_count << ","
        << "\"item_index\":" << item_index << ","
        << "\"memory_usage_percent\":" << memory_usage_percent << ","
        << "\"available_memory_bytes\":" << available_memory_bytes << ","
        << "\"hostname\":\"" << hostname << "\""
        << "}";
    return oss.str();
}

}  // namespace ingestion
}  // namespace themis
