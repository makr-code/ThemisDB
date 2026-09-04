/**
 * @file cdc_error.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC Error Codes and Structured Error Handling
 * 
 * Provides structured error codes for Change Data Capture operations.
 * Enables better error handling, monitoring, and debugging.
 * 
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// Guard against the Windows SDK '#define ERROR 0' (and similar macros)
// for the entire header. push_macro/undef/pop_macro is the MSVC-idiomatic
// approach; it also applies to all inline factory functions below.
#ifdef _WIN32
#  pragma push_macro("ERROR")
#  undef ERROR
#endif

#include <string>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace themis {
namespace cdc {

/**
 * @brief Error severity levels
 */
enum class ErrorSeverity {
    INFO,      // Informational (e.g., normal operation logged)
    WARNING,   // Warning (e.g., compression failed, using fallback)
    ERROR,     // Error (e.g., failed to record event, will retry)
    CRITICAL   // Critical (e.g., sequence generation failed, data loss risk)
};

/**
 * @brief CDC error codes
 */
enum class ErrorCode {
    // Success
    SUCCESS = 0,
    
    // Sequence generation errors (100-199)
    SEQUENCE_GENERATION_FAILED = 100,
    SEQUENCE_WRITE_FAILED = 101,
    SEQUENCE_READ_FAILED = 102,
    
    // Event recording errors (200-299)
    EVENT_RECORD_FAILED = 200,
    EVENT_SERIALIZATION_FAILED = 201,
    EVENT_DESERIALIZATION_FAILED = 202,
    EVENT_KEY_EMPTY = 203,
    EVENT_KEY_TOO_LONG = 204,
    EVENT_PAYLOAD_TOO_LARGE = 205,
    
    // Buffer errors (300-399)
    BUFFER_OVERFLOW = 300,
    BUFFER_FLUSH_FAILED = 301,
    BUFFER_NOT_RUNNING = 302,
    BUFFER_ALREADY_RUNNING = 303,
    
    // Compression errors (400-499)
    COMPRESSION_FAILED = 400,
    DECOMPRESSION_FAILED = 401,
    COMPRESSION_RATIO_TOO_LOW = 402,
    
    // Retry errors (500-599)
    RETRY_EXHAUSTED = 500,
    RETRY_TIMEOUT = 501,
    
    // Rate limiting errors (600-699)
    RATE_LIMIT_EXCEEDED = 600,
    RATE_LIMIT_CONFIG_INVALID = 601,
    
    // Retention errors (700-799)
    RETENTION_CLEANUP_FAILED = 700,
    RETENTION_POLICY_INVALID = 701,
    RETENTION_THREAD_FAILED = 702,
    
    // Database errors (800-899)
    DB_OPERATION_FAILED = 800,
    DB_ITERATOR_ERROR = 801,
    DB_WRITE_FAILED = 802,
    DB_READ_FAILED = 803,
    
    // Tenant isolation errors (900-999)
    TENANT_NOT_FOUND = 900,
    TENANT_QUOTA_EXCEEDED = 901,
    TENANT_UNAUTHORIZED = 902,
    
    // Generic errors (1000+)
    INVALID_ARGUMENT = 1000,
    INVALID_CONFIGURATION = 1001,
    INTERNAL_ERROR = 1002,
    NOT_IMPLEMENTED = 1003
};

/**
 * @brief Structured CDC exception
 */
class CDCException : public std::runtime_error {
public:
    CDCException(ErrorCode code, 
                 ErrorSeverity severity,
                 const std::string& message,
                 const std::string& context = "")
        : std::runtime_error(formatMessage(code, severity, message, context)),
          code_(code),
          severity_(severity),
          message_(message),
          context_(context) {}
    
    ErrorCode code() const { return code_; }
    ErrorSeverity severity() const { return severity_; }
    const std::string& message() const { return message_; }
    const std::string& context() const { return context_; }
    
    /**
     * @brief Get error code as integer
     */
    int codeValue() const { return static_cast<int>(code_); }
    
    /**
     * @brief Convert to JSON for logging/monitoring
     */
    nlohmann::json toJson() const {
        return {
            {"code", codeValue()},
            {"code_name", errorCodeToString(code_)},
            {"severity", severityToString(severity_)},
            {"message", message_},
            {"context", context_}
        };
    }
    
    /**
     * @brief Check if error is retryable
     */
    bool isRetryable() const {
        switch (code_) {
            case ErrorCode::DB_OPERATION_FAILED:
            case ErrorCode::DB_WRITE_FAILED:
            case ErrorCode::DB_READ_FAILED:
            case ErrorCode::BUFFER_FLUSH_FAILED:
            case ErrorCode::EVENT_RECORD_FAILED:
                return true;
            default:
                return false;
        }
    }
    
    /**
     * @brief Check if error indicates data loss risk
     */
    bool isDataLossRisk() const {
        return severity_ == ErrorSeverity::CRITICAL ||
               code_ == ErrorCode::SEQUENCE_GENERATION_FAILED ||
               code_ == ErrorCode::RETRY_EXHAUSTED;
    }

private:
    ErrorCode code_;
    ErrorSeverity severity_;
    std::string message_;
    std::string context_ = {};
    
    static std::string formatMessage(ErrorCode code, 
                                     ErrorSeverity severity,
                                     const std::string& message,
                                     const std::string& context) {
        std::string result = "[CDC:" + errorCodeToString(code) + "] ";
        result += "(" + severityToString(severity) + ") ";
        result += message;
        if (!context.empty()) {
            result += " | Context: " + context;
        }
        return result;
    }
    
    static std::string errorCodeToString(ErrorCode code) {
        switch (code) {
            case ErrorCode::SUCCESS: return "SUCCESS";
            case ErrorCode::SEQUENCE_GENERATION_FAILED: return "SEQUENCE_GENERATION_FAILED";
            case ErrorCode::SEQUENCE_WRITE_FAILED: return "SEQUENCE_WRITE_FAILED";
            case ErrorCode::SEQUENCE_READ_FAILED: return "SEQUENCE_READ_FAILED";
            case ErrorCode::EVENT_RECORD_FAILED: return "EVENT_RECORD_FAILED";
            case ErrorCode::EVENT_SERIALIZATION_FAILED: return "EVENT_SERIALIZATION_FAILED";
            case ErrorCode::EVENT_DESERIALIZATION_FAILED: return "EVENT_DESERIALIZATION_FAILED";
            case ErrorCode::EVENT_KEY_EMPTY: return "EVENT_KEY_EMPTY";
            case ErrorCode::EVENT_KEY_TOO_LONG: return "EVENT_KEY_TOO_LONG";
            case ErrorCode::EVENT_PAYLOAD_TOO_LARGE: return "EVENT_PAYLOAD_TOO_LARGE";
            case ErrorCode::BUFFER_OVERFLOW: return "BUFFER_OVERFLOW";
            case ErrorCode::BUFFER_FLUSH_FAILED: return "BUFFER_FLUSH_FAILED";
            case ErrorCode::BUFFER_NOT_RUNNING: return "BUFFER_NOT_RUNNING";
            case ErrorCode::BUFFER_ALREADY_RUNNING: return "BUFFER_ALREADY_RUNNING";
            case ErrorCode::COMPRESSION_FAILED: return "COMPRESSION_FAILED";
            case ErrorCode::DECOMPRESSION_FAILED: return "DECOMPRESSION_FAILED";
            case ErrorCode::COMPRESSION_RATIO_TOO_LOW: return "COMPRESSION_RATIO_TOO_LOW";
            case ErrorCode::RETRY_EXHAUSTED: return "RETRY_EXHAUSTED";
            case ErrorCode::RETRY_TIMEOUT: return "RETRY_TIMEOUT";
            case ErrorCode::RATE_LIMIT_EXCEEDED: return "RATE_LIMIT_EXCEEDED";
            case ErrorCode::RATE_LIMIT_CONFIG_INVALID: return "RATE_LIMIT_CONFIG_INVALID";
            case ErrorCode::RETENTION_CLEANUP_FAILED: return "RETENTION_CLEANUP_FAILED";
            case ErrorCode::RETENTION_POLICY_INVALID: return "RETENTION_POLICY_INVALID";
            case ErrorCode::RETENTION_THREAD_FAILED: return "RETENTION_THREAD_FAILED";
            case ErrorCode::DB_OPERATION_FAILED: return "DB_OPERATION_FAILED";
            case ErrorCode::DB_ITERATOR_ERROR: return "DB_ITERATOR_ERROR";
            case ErrorCode::DB_WRITE_FAILED: return "DB_WRITE_FAILED";
            case ErrorCode::DB_READ_FAILED: return "DB_READ_FAILED";
            case ErrorCode::TENANT_NOT_FOUND: return "TENANT_NOT_FOUND";
            case ErrorCode::TENANT_QUOTA_EXCEEDED: return "TENANT_QUOTA_EXCEEDED";
            case ErrorCode::TENANT_UNAUTHORIZED: return "TENANT_UNAUTHORIZED";
            case ErrorCode::INVALID_ARGUMENT: return "INVALID_ARGUMENT";
            case ErrorCode::INVALID_CONFIGURATION: return "INVALID_CONFIGURATION";
            case ErrorCode::INTERNAL_ERROR: return "INTERNAL_ERROR";
            case ErrorCode::NOT_IMPLEMENTED: return "NOT_IMPLEMENTED";
            default: return "UNKNOWN_ERROR";
        }
    }
    
    static std::string severityToString(ErrorSeverity severity) {
        switch (severity) {
            case ErrorSeverity::INFO: return "INFO";
            case ErrorSeverity::WARNING: return "WARNING";
            case ErrorSeverity::ERROR: return "ERROR";
            case ErrorSeverity::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

/**
 * @brief Helper functions for creating CDC exceptions
 */
namespace error {
    inline CDCException sequenceGenerationFailed(const std::string& details) {
        return CDCException(ErrorCode::SEQUENCE_GENERATION_FAILED, 
                           ErrorSeverity::CRITICAL,
                           "Sequence generation failed", 
                           details);
    }
    
    inline CDCException eventRecordFailed(const std::string& details) {
        return CDCException(ErrorCode::EVENT_RECORD_FAILED,
                           ErrorSeverity::ERROR,
                           "Failed to record event",
                           details);
    }
    
    inline CDCException bufferOverflow(size_t currentSize, size_t maxSize) {
        return CDCException(ErrorCode::BUFFER_OVERFLOW,
                           ErrorSeverity::WARNING,
                           "Buffer overflow",
                           "current=" + std::to_string(currentSize) + 
                           ", max=" + std::to_string(maxSize));
    }
    
    inline CDCException compressionFailed(const std::string& details) {
        return CDCException(ErrorCode::COMPRESSION_FAILED,
                           ErrorSeverity::WARNING,
                           "Compression failed",
                           details);
    }
    
    inline CDCException decompressionFailed(const std::string& details) {
        return CDCException(ErrorCode::DECOMPRESSION_FAILED,
                           ErrorSeverity::ERROR,
                           "Decompression failed",
                           details);
    }
    
    inline CDCException retryExhausted(int attempts, const std::string& lastError) {
        return CDCException(ErrorCode::RETRY_EXHAUSTED,
                           ErrorSeverity::ERROR,
                           "Retry attempts exhausted",
                           "attempts=" + std::to_string(attempts) + 
                           ", last_error=" + lastError);
    }
    
    inline CDCException rateLimitExceeded(size_t current, size_t limit) {
        return CDCException(ErrorCode::RATE_LIMIT_EXCEEDED,
                           ErrorSeverity::WARNING,
                           "Rate limit exceeded",
                           "current=" + std::to_string(current) + 
                           ", limit=" + std::to_string(limit));
    }
    
    inline CDCException dbOperationFailed(const std::string& operation, const std::string& details) {
        return CDCException(ErrorCode::DB_OPERATION_FAILED,
                           ErrorSeverity::ERROR,
                           "Database operation failed: " + operation,
                           details);
    }
    
    inline CDCException invalidArgument(const std::string& argName, const std::string& reason) {
        return CDCException(ErrorCode::INVALID_ARGUMENT,
                           ErrorSeverity::ERROR,
                           "Invalid argument: " + argName,
                           reason);
    }

    inline CDCException invalidArgument(const std::string& message) {
        return CDCException(ErrorCode::INVALID_ARGUMENT,
                           ErrorSeverity::ERROR,
                           message,
                           "");
    }

    inline CDCException internalError(const std::string& message) {
        return CDCException(ErrorCode::INTERNAL_ERROR,
                           ErrorSeverity::ERROR,
                           message,
                           "");
    }
}

} // namespace cdc
} // namespace themis

#ifdef _WIN32
#  pragma pop_macro("ERROR")
#endif

