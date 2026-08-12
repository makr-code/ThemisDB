/**
 * @file content_errors.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Content operation error codes
 * 
 * Structured error taxonomy for content ingestion, processing,
 * and retrieval operations. Used for observability, debugging,
 * and client-friendly error responses.
 */
enum class ContentErrorCode {
    // Success
    OK = 0,
    
    // Input Validation Errors (1000-1099)
    CONTENT_INVALID_INPUT = 1000,
    CONTENT_SIZE_EXCEEDED = 1001,
    CONTENT_FORMAT_UNSUPPORTED = 1002,
    CONTENT_MIME_TYPE_INVALID = 1003,
    CONTENT_MIME_TYPE_DENIED = 1004,
    CONTENT_EMPTY = 1005,
    CONTENT_CORRUPT = 1006,
    CONTENT_SCHEMA_INVALID = 1007,
    
    // Processing Errors (1100-1199)
    CONTENT_PROCESSING_FAILED = 1100,
    CONTENT_EXTRACTION_FAILED = 1101,
    CONTENT_TIMEOUT = 1102,
    CONTENT_PROCESSOR_UNAVAILABLE = 1103,
    CONTENT_CHUNKING_FAILED = 1104,
    CONTENT_EMBEDDING_FAILED = 1105,
    CONTENT_INDEXING_FAILED = 1106,
    
    // Security Errors (1200-1299)
    CONTENT_MALWARE_DETECTED = 1200,
    CONTENT_PII_DETECTED = 1201,
    CONTENT_ABUSE_DETECTED = 1202,
    CONTENT_UNAUTHORIZED = 1203,
    CONTENT_SIGNATURE_INVALID = 1204,
    CONTENT_ENCRYPTION_FAILED = 1205,
    
    // Storage Errors (1300-1399)
    CONTENT_NOT_FOUND = 1300,
    CONTENT_STORAGE_FAILED = 1301,
    CONTENT_DEDUPLICATION_FAILED = 1302,
    CONTENT_COMPRESSION_FAILED = 1303,
    
    // Rate Limiting Errors (1400-1499)
    CONTENT_RATE_LIMIT_EXCEEDED = 1400,
    CONTENT_QUEUE_FULL = 1401,
    CONTENT_BACKPRESSURE = 1402,
    
    // Resource Errors (1500-1599)
    CONTENT_MEMORY_LIMIT = 1500,
    CONTENT_CPU_LIMIT = 1501,
    CONTENT_WORKER_UNAVAILABLE = 1502,
    
    // Internal Errors (1900-1999)
    CONTENT_INTERNAL_ERROR = 1900,
    CONTENT_CONFIGURATION_ERROR = 1901,
    CONTENT_DEPENDENCY_ERROR = 1902
};

/**
 * @brief Structured error result for content operations
 * 
 * Provides detailed error information with sanitized messages
 * for external exposure and internal debugging context.
 */
struct ContentError {
    ContentErrorCode code = ContentErrorCode::OK;
    std::string message;           // Human-readable error message (sanitized for external use)
    std::string details;           // Technical details (for internal debugging/logs)
    std::string correlation_id;    // Correlation ID for request tracing
    std::string content_id;        // Content ID if applicable
    json metadata;                 // Additional context (sanitized)
    
    /**
     * @brief Check if error represents success
     */
    bool isOk() const { return code == ContentErrorCode::OK; }
    
    /**
     * @brief Check if error represents a failure
     */
    bool failed() const { return code != ContentErrorCode::OK; }
    
    /**
     * @brief Check if error is retryable (transient)
     */
    bool isRetryable() const;
    
    /**
     * @brief Check if error is client error (4xx-like)
     */
    bool isClientError() const;
    
    /**
     * @brief Check if error is server error (5xx-like)
     */
    bool isServerError() const;
    
    /**
     * @brief Get HTTP status code equivalent
     */
    int getHttpStatus() const;
    
    /**
     * @brief Serialize to JSON (for API responses)
     */
    json toJson() const;
    
    /**
     * @brief Serialize to JSON with full details (for internal logging)
     */
    json toJsonVerbose() const;
    
    /**
     * @brief Create from JSON
     */
    static ContentError fromJson(const json& j);
    
    /**
     * @brief Create success result
     */
    static ContentError ok();
    
    /**
     * @brief Create error with code and message
     */
    static ContentError error(
        ContentErrorCode code,
        const std::string& message,
        const std::string& details = ""
    );
};

/**
 * @brief Convert error code to string
 */
std::string errorCodeToString(ContentErrorCode code);

/**
 * @brief Get error code category (validation, processing, security, etc.)
 */
std::string errorCodeCategory(ContentErrorCode code);

/**
 * @brief Get default message for error code
 */
std::string getDefaultErrorMessage(ContentErrorCode code);

/**
 * @brief Check if error code indicates a security issue
 */
bool isSecurityError(ContentErrorCode code);

/**
 * @brief Check if error code indicates validation failure
 */
bool isValidationError(ContentErrorCode code);

} // namespace content
} // namespace themis
