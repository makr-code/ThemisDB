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
        case IngestionErrorCode::HTTP_CONNECTION_REFUSED:
        case IngestionErrorCode::HTTP_SERVER_ERROR:
        case IngestionErrorCode::HTTP_RATE_LIMITED:
        case IngestionErrorCode::HTTP_DNS_RESOLUTION_FAILED:
        case IngestionErrorCode::DATABASE_TIMEOUT:
        case IngestionErrorCode::DATABASE_DEADLOCK:
        case IngestionErrorCode::SOURCE_UNAVAILABLE:
        case IngestionErrorCode::CONNECTOR_DEGRADED:

        // Temporary resource constraints
        case IngestionErrorCode::QUEUE_SATURATED:
        case IngestionErrorCode::BUFFER_FULL:
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
        case IngestionErrorCode::CPU_THROTTLE:

        // Quality check timeout (potentially retryable)
        case IngestionErrorCode::QUALITY_CHECK_TIMEOUT:
        case IngestionErrorCode::VALIDATION_TIMEOUT:
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
        case IngestionErrorCode::SCHEMA_MISMATCH:
        case IngestionErrorCode::VALIDATION_FAILED:
        case IngestionErrorCode::DUPLICATE_KEY:
        case IngestionErrorCode::CONSTRAINT_CHECK_FAILED:
        case IngestionErrorCode::TYPE_COERCION_FAILED:

        // Configuration errors (permanent)
        case IngestionErrorCode::CONFIG_INVALID:
        case IngestionErrorCode::CONFIG_MISSING_REQUIRED:
        case IngestionErrorCode::CONFIG_TYPE_MISMATCH:
        case IngestionErrorCode::CONFIG_OUT_OF_RANGE:

        // Authentication/Authorization errors (permanent)
        case IngestionErrorCode::AUTH_REQUIRED:
        case IngestionErrorCode::AUTH_FAILED:
        case IngestionErrorCode::AUTH_EXPIRED:
        case IngestionErrorCode::AUTH_INSUFFICIENT_SCOPE:
        case IngestionErrorCode::PERMISSION_DENIED:
        case IngestionErrorCode::HTTP_UNAUTHORIZED:

        // File/Source not found (permanent)
        case IngestionErrorCode::FILE_NOT_FOUND:
        case IngestionErrorCode::SOURCE_NOT_FOUND:
        case IngestionErrorCode::SOURCE_NOT_CONFIGURED:
        case IngestionErrorCode::CONNECTOR_NOT_SUPPORTED:
        case IngestionErrorCode::HTTP_NOT_FOUND:

        // Format/Codec errors (permanent for this item)
        case IngestionErrorCode::FILE_FORMAT_UNSUPPORTED:
        case IngestionErrorCode::FILE_ENCODING_ERROR:

        // Quality failures (permanent for this item)
        case IngestionErrorCode::QUALITY_THRESHOLD_FAILED:

        // Workflow errors (permanent for this item)
        case IngestionErrorCode::WORKFLOW_CONDITIONAL_FALSE:
        case IngestionErrorCode::ADAPTER_INCOMPATIBILITY:

        // System errors (permanent)
        case IngestionErrorCode::NOT_IMPLEMENTED:
        case IngestionErrorCode::INTERNAL_ERROR:
            return true;

        default:
            return false;
    }
}

bool isResourceExhaustionError(IngestionErrorCode code) {
    switch (code) {
        case IngestionErrorCode::MEMORY_EXHAUSTION:
        case IngestionErrorCode::DISK_QUOTA_EXCEEDED:
        case IngestionErrorCode::DISK_SPACE_LOW:
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
        case IngestionErrorCode::PROCESS_LIMIT_EXCEEDED:
        case IngestionErrorCode::CPU_THROTTLE:
        case IngestionErrorCode::QUEUE_SATURATED:
        case IngestionErrorCode::BUFFER_FULL:
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
            return true;

        default:
            return false;
    }
}

bool isBackpressureError(IngestionErrorCode code) {
    switch (code) {
        case IngestionErrorCode::QUEUE_SATURATED:
        case IngestionErrorCode::BUFFER_FULL:
        case IngestionErrorCode::ENQUEUE_TIMEOUT:
        case IngestionErrorCode::BACKPRESSURE_APPLIED:
        case IngestionErrorCode::HTTP_RATE_LIMITED:
        case IngestionErrorCode::MEMORY_EXHAUSTION:
        case IngestionErrorCode::CONNECTION_POOL_EXHAUSTED:
        case IngestionErrorCode::THREAD_POOL_EXHAUSTED:
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
