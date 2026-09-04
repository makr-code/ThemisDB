/**
 * @file error_contracts.cpp
 * @brief Implementation of unified error handling framework
 * 
 * Provides diagnostic logging, error categorization, and helper functions
 * for error context creation and presentation.
 * 
 * @version 1.0.0
 * @date 2026-08-08
 */

#include "utils/error_contracts.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <iomanip>
#include <sstream>

namespace themis {
namespace utils {

using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// ErrorContext Implementation
// ─────────────────────────────────────────────────────────────────────────────

std::string ErrorContext::toJSON() const {
    json j;
    
    // Error identification
    j["error_code"] = static_cast<uint16_t>(code);
    j["error_code_name"] = errorCodeName(code);
    j["category"] = categoryName(category);
    j["severity"] = severityName(severity);
    
    // Timing - format timestamp as ISO8601 string
    auto ts = std::chrono::system_clock::to_time_t(timestamp);
    std::stringstream timestamp_ss;
    timestamp_ss << std::put_time(std::gmtime(&ts), "%Y-%m-%dT%H:%M:%SZ");
    j["timestamp"] = timestamp_ss.str();
    j["elapsed_ms"] = elapsed_ms.count();
    
    // Message and context
    j["message"] = message;
    j["component"] = component;
    j["context"] = context_info;
    
    // Recovery information
    j["recoverable"] = is_recoverable;
    j["recovery_hint"] = recovery_hint;
    j["retry_count"] = retry_count;
    
    // Resource state
    if (resource_limit > 0) {
        j["resource_state"] = {
            {"limit", resource_limit},
            {"current", resource_current},
            {"available", resource_limit - resource_current}
        };
    }
    
    return j.dump();
}

std::string ErrorContext::toFormattedString() const {
    std::ostringstream oss;
    
    oss << "[" << severityName(severity) << "] "
        << "ErrorCode=" << errorCodeName(code) << " "
        << "Category=" << categoryName(category) << "\n";
    
    oss << "Component: " << component << "\n";
    oss << "Message: " << message << "\n";
    
    if (!context_info.empty()) {
        oss << "Context: " << context_info << "\n";
    }
    
    if (!recovery_hint.empty()) {
        oss << "Recovery: " << recovery_hint << "\n";
    }
    
    if (resource_limit > 0) {
        oss << "Resources: " << resource_current << " / " << resource_limit
            << " (" << (resource_current * 100 / resource_limit) << "%)\n";
    }
    
    oss << "Elapsed: " << elapsed_ms.count() << "ms\n";
    oss << "Recoverable: " << (is_recoverable ? "yes" : "no") << "\n";
    
    if (retry_count > 0) {
        oss << "Retries: " << retry_count << "\n";
    }
    
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Code Naming
// ─────────────────────────────────────────────────────────────────────────────

std::string errorCodeName(ErrorCode code) {
    switch (code) {
        // General utility errors
        case ErrorCode::UTILS_INVALID_ARGUMENT:           return "UTILS_INVALID_ARGUMENT";
        case ErrorCode::UTILS_ALLOCATION_FAILED:          return "UTILS_ALLOCATION_FAILED";
        case ErrorCode::UTILS_TIMEOUT:                    return "UTILS_TIMEOUT";
        case ErrorCode::UTILS_NOT_INITIALIZED:            return "UTILS_NOT_INITIALIZED";
        case ErrorCode::UTILS_ALREADY_INITIALIZED:        return "UTILS_ALREADY_INITIALIZED";
        case ErrorCode::UTILS_RESOURCE_EXHAUSTED:         return "UTILS_RESOURCE_EXHAUSTED";
        case ErrorCode::UTILS_UNSUPPORTED_OPERATION:      return "UTILS_UNSUPPORTED_OPERATION";
        case ErrorCode::UTILS_INTERNAL_ERROR:             return "UTILS_INTERNAL_ERROR";
        case ErrorCode::UTILS_NOT_IMPLEMENTED:            return "UTILS_NOT_IMPLEMENTED";
        case ErrorCode::UTILS_INVALID_STATE:              return "UTILS_INVALID_STATE";
        
        // Audit logging errors
        case ErrorCode::AUDIT_BUFFER_OVERFLOW:            return "AUDIT_BUFFER_OVERFLOW";
        case ErrorCode::AUDIT_WRITE_FAILED:               return "AUDIT_WRITE_FAILED";
        case ErrorCode::AUDIT_PERSISTENCE_FAILED:         return "AUDIT_PERSISTENCE_FAILED";
        case ErrorCode::AUDIT_ROTATION_FAILED:            return "AUDIT_ROTATION_FAILED";
        case ErrorCode::AUDIT_FORMAT_ERROR:               return "AUDIT_FORMAT_ERROR";
        case ErrorCode::AUDIT_ENCRYPTION_FAILED:          return "AUDIT_ENCRYPTION_FAILED";
        case ErrorCode::AUDIT_SIGNATURE_FAILED:           return "AUDIT_SIGNATURE_FAILED";
        case ErrorCode::AUDIT_VALIDATION_FAILED:          return "AUDIT_VALIDATION_FAILED";
        case ErrorCode::AUDIT_QUEUE_FULL:                 return "AUDIT_QUEUE_FULL";
        case ErrorCode::AUDIT_FLUSH_FAILED:               return "AUDIT_FLUSH_FAILED";
        
        // Structured logging errors
        case ErrorCode::LOG_BUFFER_OVERFLOW:              return "LOG_BUFFER_OVERFLOW";
        case ErrorCode::LOG_WRITE_FAILED:                 return "LOG_WRITE_FAILED";
        case ErrorCode::LOG_INVALID_FORMAT:               return "LOG_INVALID_FORMAT";
        case ErrorCode::LOG_INITIALIZATION_FAILED:        return "LOG_INITIALIZATION_FAILED";
        case ErrorCode::LOG_LEVEL_INVALID:                return "LOG_LEVEL_INVALID";
        case ErrorCode::LOG_SINK_FAILED:                  return "LOG_SINK_FAILED";
        case ErrorCode::LOG_ASYNC_OVERFLOW:               return "LOG_ASYNC_OVERFLOW";
        case ErrorCode::LOG_PATTERN_ERROR:                return "LOG_PATTERN_ERROR";
        case ErrorCode::LOG_ARGUMENT_ERROR:               return "LOG_ARGUMENT_ERROR";
        case ErrorCode::LOG_ROTATION_ERROR:               return "LOG_ROTATION_ERROR";
        
        // Tracing errors
        case ErrorCode::TRACE_SPAN_CREATE_FAILED:         return "TRACE_SPAN_CREATE_FAILED";
        case ErrorCode::TRACE_EXPORT_FAILED:              return "TRACE_EXPORT_FAILED";
        case ErrorCode::TRACE_BUFFER_OVERFLOW:            return "TRACE_BUFFER_OVERFLOW";
        case ErrorCode::TRACE_INVALID_CONTEXT:            return "TRACE_INVALID_CONTEXT";
        case ErrorCode::TRACE_SAMPLING_FAILED:            return "TRACE_SAMPLING_FAILED";
        case ErrorCode::TRACE_BATCH_FAILED:               return "TRACE_BATCH_FAILED";
        
        // Privacy detection errors
        case ErrorCode::PRIVACY_INVALID_INPUT:            return "PRIVACY_INVALID_INPUT";
        case ErrorCode::PRIVACY_PATTERN_OVERFLOW:         return "PRIVACY_PATTERN_OVERFLOW";
        case ErrorCode::PRIVACY_DETECTION_TIMEOUT:        return "PRIVACY_DETECTION_TIMEOUT";
        case ErrorCode::PRIVACY_BUFFER_OVERFLOW:          return "PRIVACY_BUFFER_OVERFLOW";
        case ErrorCode::PRIVACY_ENGINE_LOAD_FAILED:       return "PRIVACY_ENGINE_LOAD_FAILED";
        case ErrorCode::PRIVACY_CONFIG_INVALID:           return "PRIVACY_CONFIG_INVALID";
        case ErrorCode::PRIVACY_UNICODE_ERROR:            return "PRIVACY_UNICODE_ERROR";
        case ErrorCode::PRIVACY_MEMORY_EXCEEDED:          return "PRIVACY_MEMORY_EXCEEDED";
        case ErrorCode::PRIVACY_NO_ENGINE:                return "PRIVACY_NO_ENGINE";
        case ErrorCode::PRIVACY_ENGINE_FAILED:            return "PRIVACY_ENGINE_FAILED";
        
        // Cryptography errors
        case ErrorCode::CRYPTO_KEY_DERIVATION_FAILED:     return "CRYPTO_KEY_DERIVATION_FAILED";
        case ErrorCode::CRYPTO_KEY_INVALID:               return "CRYPTO_KEY_INVALID";
        case ErrorCode::CRYPTO_KEY_EXPIRED:               return "CRYPTO_KEY_EXPIRED";
        case ErrorCode::CRYPTO_KEY_NOT_FOUND:             return "CRYPTO_KEY_NOT_FOUND";
        case ErrorCode::CRYPTO_CACHE_MISS:                return "CRYPTO_CACHE_MISS";
        case ErrorCode::CRYPTO_CERT_LOAD_FAILED:          return "CRYPTO_CERT_LOAD_FAILED";
        case ErrorCode::CRYPTO_CERT_INVALID:              return "CRYPTO_CERT_INVALID";
        case ErrorCode::CRYPTO_CERT_EXPIRED:              return "CRYPTO_CERT_EXPIRED";
        case ErrorCode::CRYPTO_ENCRYPTION_FAILED:         return "CRYPTO_ENCRYPTION_FAILED";
        case ErrorCode::CRYPTO_DECRYPTION_FAILED:         return "CRYPTO_DECRYPTION_FAILED";
        
        // Compression errors
        case ErrorCode::COMPRESSION_FAILED:               return "COMPRESSION_FAILED";
        case ErrorCode::DECOMPRESSION_FAILED:             return "DECOMPRESSION_FAILED";
        case ErrorCode::COMPRESSION_BUFFER_SMALL:         return "COMPRESSION_BUFFER_SMALL";
        case ErrorCode::COMPRESSION_INPUT_INVALID:        return "COMPRESSION_INPUT_INVALID";
        case ErrorCode::COMPRESSION_BOMB_DETECTED:        return "COMPRESSION_BOMB_DETECTED";
        case ErrorCode::COMPRESSION_RATIO_EXCEEDED:       return "COMPRESSION_RATIO_EXCEEDED";
        case ErrorCode::CODEC_INITIALIZATION_FAILED:      return "CODEC_INITIALIZATION_FAILED";
        case ErrorCode::CODEC_NOT_SUPPORTED:              return "CODEC_NOT_SUPPORTED";
        
        // Concurrency errors
        case ErrorCode::THREADPOOL_QUEUE_FULL:            return "THREADPOOL_QUEUE_FULL";
        case ErrorCode::THREADPOOL_SHUTDOWN:              return "THREADPOOL_SHUTDOWN";
        case ErrorCode::THREADPOOL_INVALID_STATE:         return "THREADPOOL_INVALID_STATE";
        case ErrorCode::RATELIMIT_EXCEEDED:               return "RATELIMIT_EXCEEDED";
        case ErrorCode::RATELIMIT_WINDOW_ERROR:           return "RATELIMIT_WINDOW_ERROR";
        case ErrorCode::CONNECTION_POOL_EXHAUSTED:        return "CONNECTION_POOL_EXHAUSTED";
        case ErrorCode::CONNECTION_POOL_TIMEOUT:          return "CONNECTION_POOL_TIMEOUT";
        case ErrorCode::LOCK_ACQUISITION_FAILED:          return "LOCK_ACQUISITION_FAILED";
        case ErrorCode::LOCK_TIMEOUT:                     return "LOCK_TIMEOUT";
        case ErrorCode::CONCURRENT_MODIFICATION:          return "CONCURRENT_MODIFICATION";
        
        // Serialization errors
        case ErrorCode::SERIALIZATION_FAILED:             return "SERIALIZATION_FAILED";
        case ErrorCode::DESERIALIZATION_FAILED:           return "DESERIALIZATION_FAILED";
        case ErrorCode::SERIALIZATION_FORMAT_INVALID:     return "SERIALIZATION_FORMAT_INVALID";
        case ErrorCode::SERIALIZATION_VERSION_MISMATCH:   return "SERIALIZATION_VERSION_MISMATCH";
        case ErrorCode::SERIALIZATION_SIZE_EXCEEDED:      return "SERIALIZATION_SIZE_EXCEEDED";
        
        case ErrorCode::UNKNOWN_ERROR:
        [[fallthrough]];\n        default:
            return fmt::format("UNKNOWN_ERROR({})", static_cast<uint16_t>(code));
    }
}

std::string categoryName(ErrorCategory category) {
    switch (category) {
        case ErrorCategory::AuditLog:           return "AuditLog";
        case ErrorCategory::StructuredLogging:  return "StructuredLogging";
        case ErrorCategory::Tracing:            return "Tracing";
        case ErrorCategory::SagaLogging:        return "SagaLogging";
        case ErrorCategory::PrivacyDetection:   return "PrivacyDetection";
        case ErrorCategory::PatternDetection:   return "PatternDetection";
        case ErrorCategory::NERDetection:       return "NERDetection";
        case ErrorCategory::PrivacyFilter:      return "PrivacyFilter";
        case ErrorCategory::KeyDerivation:      return "KeyDerivation";
        case ErrorCategory::KeyCache:           return "KeyCache";
        case ErrorCategory::PublicKeyInfra:     return "PublicKeyInfra";
        case ErrorCategory::LocalEncryption:    return "LocalEncryption";
        case ErrorCategory::ZstdCodec:          return "ZstdCodec";
        case ErrorCategory::LZ4Codec:           return "LZ4Codec";
        case ErrorCategory::SerializationErr:   return "Serialization";
        case ErrorCategory::ThreadPool:         return "ThreadPool";
        case ErrorCategory::RateLimiting:       return "RateLimiting";
        case ErrorCategory::ConnectionPool:     return "ConnectionPool";
        case ErrorCategory::Unknown:            return "Unknown";
        default:                                return "UnmappedCategory";
    }
}

std::string severityName(ErrorSeverity severity) {
    switch (severity) {
        case ErrorSeverity::Fatal:      return "Fatal";
        case ErrorSeverity::Error:      return "Error";
        case ErrorSeverity::Warning:    return "Warning";
        case ErrorSeverity::Degraded:   return "Degraded";
        default:                        return "Unknown";
    }
}

std::string incidentName(IncidentCategory incident) {
    switch (incident) {
        case IncidentCategory::BufferOverflow:           return "BufferOverflow";
        case IncidentCategory::MemoryExhaustion:         return "MemoryExhaustion";
        case IncidentCategory::ConnectionPoolExhausted:  return "ConnectionPoolExhausted";
        case IncidentCategory::ThreadPoolOverload:       return "ThreadPoolOverload";
        case IncidentCategory::DetectionTimeout:         return "DetectionTimeout";
        case IncidentCategory::OperationTimeout:         return "OperationTimeout";
        case IncidentCategory::KeyDerivationFailure:     return "KeyDerivationFailure";
        case IncidentCategory::PrivacyDetectionFailure:  return "PrivacyDetectionFailure";
        case IncidentCategory::CompressionFailure:       return "CompressionFailure";
        case IncidentCategory::FallbackActivated:        return "FallbackActivated";
        case IncidentCategory::RateLimitExhausted:       return "RateLimitExhausted";
        case IncidentCategory::InvalidConfiguration:     return "InvalidConfiguration";
        case IncidentCategory::DataCorruption:           return "DataCorruption";
        case IncidentCategory::ExternalServiceUnavailable: return "ExternalServiceUnavailable";
        case IncidentCategory::UnclassifiedIncident:     return "UnclassifiedIncident";
        default:                                         return "UnmappedIncident";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Incident Categorization
// ─────────────────────────────────────────────────────────────────────────────

IncidentCategory categorizeIncident(ErrorCode code) {
    // Categorize based on error code patterns and semantics
    switch (code) {
        // Buffer overflow incidents
        case ErrorCode::AUDIT_BUFFER_OVERFLOW:
        [[fallthrough]];\n        case ErrorCode::AUDIT_QUEUE_FULL:
        [[fallthrough]];\n        case ErrorCode::LOG_BUFFER_OVERFLOW:
        [[fallthrough]];\n        case ErrorCode::TRACE_BUFFER_OVERFLOW:
        [[fallthrough]];\n        case ErrorCode::PRIVACY_BUFFER_OVERFLOW:
            return IncidentCategory::BufferOverflow;
        
        // Memory exhaustion incidents
        case ErrorCode::PRIVACY_MEMORY_EXCEEDED:
        [[fallthrough]];\n        case ErrorCode::UTILS_ALLOCATION_FAILED:
        [[fallthrough]];\n        case ErrorCode::UTILS_RESOURCE_EXHAUSTED:
            return IncidentCategory::MemoryExhaustion;
        
        // Connection pool exhaustion incidents
        case ErrorCode::CONNECTION_POOL_EXHAUSTED:
            return IncidentCategory::ConnectionPoolExhausted;
        
        // Thread pool overload incidents
        case ErrorCode::THREADPOOL_QUEUE_FULL:
            return IncidentCategory::ThreadPoolOverload;
        
        // Detection timeout incidents
        case ErrorCode::PRIVACY_DETECTION_TIMEOUT:
            return IncidentCategory::DetectionTimeout;
        
        // General operation timeout incidents
        case ErrorCode::UTILS_TIMEOUT:
        [[fallthrough]];\n        case ErrorCode::CONNECTION_POOL_TIMEOUT:
        [[fallthrough]];\n        case ErrorCode::LOCK_TIMEOUT:
            return IncidentCategory::OperationTimeout;
        
        // Key derivation failures
        case ErrorCode::CRYPTO_KEY_DERIVATION_FAILED:
        [[fallthrough]];\n        case ErrorCode::CRYPTO_CACHE_MISS:
            return IncidentCategory::KeyDerivationFailure;
        
        // Privacy detection failures
        case ErrorCode::PRIVACY_ENGINE_FAILED:
        [[fallthrough]];\n        case ErrorCode::PRIVACY_ENGINE_LOAD_FAILED:
        [[fallthrough]];\n        case ErrorCode::PRIVACY_NO_ENGINE:
            return IncidentCategory::PrivacyDetectionFailure;
        
        // Compression failures
        case ErrorCode::COMPRESSION_FAILED:
        [[fallthrough]];\n        case ErrorCode::DECOMPRESSION_FAILED:
        [[fallthrough]];\n        case ErrorCode::COMPRESSION_BOMB_DETECTED:
        [[fallthrough]];\n        case ErrorCode::CODEC_INITIALIZATION_FAILED:
            return IncidentCategory::CompressionFailure;
        
        // Fallback activated indicators
        case ErrorCode::PRIVACY_PATTERN_OVERFLOW:  // Fall back to simpler detection
        [[fallthrough]];\n        case ErrorCode::LOG_SINK_FAILED:           // Fall back to console logging
            return IncidentCategory::FallbackActivated;
        
        // Rate limit exhaustion
        case ErrorCode::RATELIMIT_EXCEEDED:
            return IncidentCategory::RateLimitExhausted;
        
        // Invalid configuration
        case ErrorCode::PRIVACY_CONFIG_INVALID:
        [[fallthrough]];\n        case ErrorCode::LOG_LEVEL_INVALID:
        [[fallthrough]];\n        case ErrorCode::LOG_PATTERN_ERROR:
        [[fallthrough]];\n        case ErrorCode::UTILS_INVALID_ARGUMENT:
            return IncidentCategory::InvalidConfiguration;
        
        // Data corruption indicators
        case ErrorCode::COMPRESSION_INPUT_INVALID:
        [[fallthrough]];\n        case ErrorCode::SERIALIZATION_FORMAT_INVALID:
        [[fallthrough]];\n        case ErrorCode::SERIALIZATION_VERSION_MISMATCH:
            return IncidentCategory::DataCorruption;
        
        // External service unavailable
        case ErrorCode::CRYPTO_CERT_LOAD_FAILED:
            return IncidentCategory::ExternalServiceUnavailable;
        
        default:
            return IncidentCategory::UnclassifiedIncident;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Error Context Creation
// ─────────────────────────────────────────────────────────────────────────────

ErrorContext makeErrorContext(ErrorCode code,
                              const std::string& message,
                              const std::string& component,
                              ErrorSeverity severity,
                              bool is_recoverable) {
    ErrorContext ctx;
    ctx.code = code;
    ctx.message = message;
    ctx.component = component;
    ctx.severity = severity;
    ctx.is_recoverable = is_recoverable;
    ctx.timestamp = std::chrono::system_clock::now();
    ctx.elapsed_ms = std::chrono::milliseconds(0);
    ctx.retry_count = 0;
    ctx.resource_limit = 0;
    ctx.resource_current = 0;
    
    // Infer category from error code
    if (code >= ErrorCode::AUDIT_BUFFER_OVERFLOW && 
        code <= ErrorCode::AUDIT_FLUSH_FAILED) {
        ctx.category = ErrorCategory::AuditLog;
    } else if (code >= ErrorCode::LOG_BUFFER_OVERFLOW && 
               code <= ErrorCode::LOG_ROTATION_ERROR) {
        ctx.category = ErrorCategory::StructuredLogging;
    } else if (code >= ErrorCode::TRACE_SPAN_CREATE_FAILED && 
               code <= ErrorCode::TRACE_BATCH_FAILED) {
        ctx.category = ErrorCategory::Tracing;
    } else if (code >= ErrorCode::PRIVACY_INVALID_INPUT && 
               code <= ErrorCode::PRIVACY_ENGINE_FAILED) {
        ctx.category = ErrorCategory::PrivacyDetection;
    } else if (code >= ErrorCode::CRYPTO_KEY_DERIVATION_FAILED && 
               code <= ErrorCode::CRYPTO_DECRYPTION_FAILED) {
        ctx.category = ErrorCategory::KeyDerivation;
    } else if (code >= ErrorCode::COMPRESSION_FAILED && 
               code <= ErrorCode::CODEC_NOT_SUPPORTED) {
        ctx.category = ErrorCategory::ZstdCodec; // Default to Zstd
    } else if (code >= ErrorCode::THREADPOOL_QUEUE_FULL && 
               code <= ErrorCode::CONCURRENT_MODIFICATION) {
        ctx.category = ErrorCategory::ThreadPool;
    } else if (code >= ErrorCode::SERIALIZATION_FAILED && 
               code <= ErrorCode::SERIALIZATION_SIZE_EXCEEDED) {
        ctx.category = ErrorCategory::SerializationErr;
    } else {
        ctx.category = ErrorCategory::Unknown;
    }
    
    // Set recovery hints based on error code
    switch (code) {
        case ErrorCode::AUDIT_BUFFER_OVERFLOW:
        [[fallthrough]];\n        case ErrorCode::LOG_BUFFER_OVERFLOW:
        [[fallthrough]];\n        case ErrorCode::TRACE_BUFFER_OVERFLOW:
            ctx.recovery_hint = "Increase buffer size or reduce logging volume";
            break;
        
        case ErrorCode::PRIVACY_DETECTION_TIMEOUT:
            ctx.recovery_hint = "Increase timeout or reduce input size";
            break;
        
        case ErrorCode::THREADPOOL_QUEUE_FULL:
            ctx.recovery_hint = "Increase queue size or reduce submission rate";
            break;
        
        case ErrorCode::CRYPTO_KEY_DERIVATION_FAILED:
            ctx.recovery_hint = "Check key derivation parameters and retry";
            break;
        
        case ErrorCode::COMPRESSION_FAILED:
        [[fallthrough]];\n        case ErrorCode::DECOMPRESSION_FAILED:
            ctx.recovery_hint = "Verify input data and retry with different compression level";
            break;
        
        default:
            ctx.recovery_hint = "Review logs and retry operation";
            break;
    }
    
    return ctx;
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostic Logging
// ─────────────────────────────────────────────────────────────────────────────

void logErrorWithContext(const ErrorContext& ctx,
                         std::shared_ptr<spdlog::logger> logger) {
    if (!logger) {
        logger = spdlog::get("themis_utils");
        if (!logger) {
            logger = spdlog::default_logger();
        }
    }
    
    if (!logger) {
        return; // Silently fail if no logger available
    }
    
    // Log at appropriate level based on severity
    switch (ctx.severity) {
        case ErrorSeverity::Fatal:
            logger->critical("Error: {}", ctx.toFormattedString());
            break;
        case ErrorSeverity::Error:
            logger->error("Error: {}", ctx.toFormattedString());
            break;
        case ErrorSeverity::Warning:
            logger->warn("Warning: {}", ctx.toFormattedString());
            break;
        case ErrorSeverity::Degraded:
            logger->info("Degraded: {}", ctx.toFormattedString());
            break;
    }
    
    // Also try to log as structured JSON if supported
    try {
        logger->debug("Error context JSON: {}", ctx.toJSON());
    } catch (...) {
        // Silently ignore JSON logging failures
    }
}

} // namespace utils
} // namespace themis
