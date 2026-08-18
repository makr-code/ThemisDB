/**
 * @file error_contracts.h
 * @brief Unified error handling framework for utils module
 * 
 * Provides standardized error contract definitions, categories, and diagnostic
 * information for all utils subsystems (observability, privacy, crypto, compression, concurrency).
 * 
 * This framework ensures:
 * - Consistent error semantics across all utils helpers
 * - Explicit error recovery paths
 * - Bounded, predictable diagnostic logging
 * - Operator-visible categorization for quick incident response
 * 
 * @version 1.0.0
 * @date 2026-08-08
 * @note Maturity: 🟡 BETA (Phase 3 implementation)
 * 
 * @see ROADMAP.md - Phase 3: Error Handling and Edge Cases
 */

#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <memory>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace themis {
namespace utils {

// ─────────────────────────────────────────────────────────────────────────────
// Error Categories (organized by subsystem)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ErrorCategory
 * @brief Categorizes errors by utils subsystem for diagnostic and routing purposes.
 * 
 * Each category aligns with a major plane in the utils module:
 * - Observability: audit_logger, logger, tracing, saga_logger
 * - Privacy: pii_detector, pii_detection_engine, regex_detection_engine
 * - Crypto: hkdf_helper, hkdf_cache, pki_client, lek_manager
 * - Compression: zstd_codec, lz4_codec
 * - Concurrency: thread_pool_manager, rate_limiter, channel/connection pools
 * - Serialization: serialization helpers
 * 
 * Used in ErrorContext and incident categorization for operator diagnostics.
 */
enum class ErrorCategory : uint16_t {
    // Observability subsystem
    AuditLog          = 0x0100,  ///< audit_logger failures
    StructuredLogging = 0x0101,  ///< logger failures
    Tracing           = 0x0102,  ///< tracing failures
    SagaLogging       = 0x0103,  ///< saga_logger failures
    
    // Privacy & Detection subsystem
    PrivacyDetection  = 0x0200,  ///< pii_detector, pii_detection_engine failures
    PatternDetection  = 0x0201,  ///< regex_detection_engine failures
    NERDetection      = 0x0202,  ///< Named Entity Recognition failures
    PrivacyFilter     = 0x0203,  ///< pii_pseudonymizer, obfuscation failures
    
    // Cryptography & Key Management subsystem
    KeyDerivation     = 0x0300,  ///< hkdf_helper failures
    KeyCache          = 0x0301,  ///< hkdf_cache failures
    PublicKeyInfra    = 0x0302,  ///< pki_client certificate/key loading
    LocalEncryption   = 0x0303,  ///< lek_manager failures
    
    // Compression & Encoding subsystem
    ZstdCodec         = 0x0400,  ///< zstd compression/decompression
    LZ4Codec          = 0x0401,  ///< lz4 compression/decompression
    SerializationErr  = 0x0402,  ///< Serialization framework
    
    // Runtime Services subsystem
    ThreadPool        = 0x0500,  ///< thread_pool_manager failures
    RateLimiting      = 0x0501,  ///< rate_limiter failures
    ConnectionPool    = 0x0502,  ///< grpc_channel_pool, http_client_pool
    
    // Unknown/Uncategorized
    Unknown           = 0xFFFF   ///< Unmapped error category
};

// ─────────────────────────────────────────────────────────────────────────────
// Error Severity Levels
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ErrorSeverity
 * @brief Indicates the impact and recovery strategy for an error.
 * 
 * - Fatal: System state corrupted; graceful shutdown recommended; no recovery
 * - Error: Functional failure; recovery possible but not automatic
 * - Warning: Degradation detected; fallback available; service continues
 * - Degraded: Performance or quality impact; service remains functional
 * 
 * Used to prioritize alerting and determine operator response urgency.
 */
enum class ErrorSeverity : uint8_t {
    Fatal      = 0,  ///< Unrecoverable; cascading failure risk
    Error      = 1,  ///< Functional failure; requires explicit intervention
    Warning    = 2,  ///< Degradation; fallback available
    Degraded   = 3,  ///< Performance impact; service nominal
    Critical   = Fatal  ///< Compatibility alias for fail-closed callers
};

// ─────────────────────────────────────────────────────────────────────────────
// Error Codes (unified across all utils subsystems)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum ErrorCode
 * @brief Unified error code enumeration for utils module.
 * 
 * Range: 9000-9099 (reserved for utils in global error_registry.h)
 * 
 * Organized by subsystem:
 * - 9000-9009: General/utility errors
 * - 9010-9019: Audit logging errors
 * - 9020-9029: Structured logging errors
 * - 9030-9039: Tracing errors
 * - 9040-9049: Privacy detection errors
 * - 9050-9059: Cryptography errors
 * - 9060-9069: Compression errors
 * - 9070-9079: Concurrency errors
 * - 9080-9089: Serialization errors
 * 
 * @see error_registry.h for global ErrorCode enum
 */
enum class ErrorCode : uint16_t {
    // General Utility Errors
    UTILS_INVALID_ARGUMENT          = 9000,
    UTILS_ALLOCATION_FAILED         = 9001,
    UTILS_TIMEOUT                   = 9002,
    UTILS_NOT_INITIALIZED           = 9003,
    UTILS_ALREADY_INITIALIZED       = 9004,
    UTILS_RESOURCE_EXHAUSTED        = 9005,
    UTILS_UNSUPPORTED_OPERATION     = 9006,
    UTILS_INTERNAL_ERROR            = 9007,
    UTILS_NOT_IMPLEMENTED           = 9008,
    UTILS_INVALID_STATE             = 9009,
    
    // Audit Logging Errors (9010-9019)
    AUDIT_BUFFER_OVERFLOW           = 9010,  ///< Audit queue/buffer at capacity
    AUDIT_WRITE_FAILED              = 9011,  ///< Failed to write audit event
    AUDIT_PERSISTENCE_FAILED        = 9012,  ///< Failed to persist to storage
    AUDIT_ROTATION_FAILED           = 9013,  ///< Log rotation operation failed
    AUDIT_FORMAT_ERROR              = 9014,  ///< Event formatting failed
    AUDIT_ENCRYPTION_FAILED         = 9015,  ///< Audit event encryption failed
    AUDIT_SIGNATURE_FAILED          = 9016,  ///< Cryptographic signing failed
    AUDIT_VALIDATION_FAILED         = 9017,  ///< Event validation failed
    AUDIT_QUEUE_FULL                = 9018,  ///< Bounded queue capacity exceeded
    AUDIT_FLUSH_FAILED              = 9019,  ///< Flush operation failed
    
    // Structured Logging Errors (9020-9029)
    LOG_BUFFER_OVERFLOW             = 9020,  ///< Log queue exceeded
    LOG_WRITE_FAILED                = 9021,  ///< Failed to write log entry
    LOG_INVALID_FORMAT              = 9022,  ///< Log format validation failed
    LOG_INITIALIZATION_FAILED       = 9023,  ///< Logger initialization failed
    LOG_LEVEL_INVALID               = 9024,  ///< Invalid log level specified
    LOG_SINK_FAILED                 = 9025,  ///< Log sink unavailable/failed
    LOG_ASYNC_OVERFLOW              = 9026,  ///< Async logger queue overflow
    LOG_PATTERN_ERROR               = 9027,  ///< Log pattern parsing failed
    LOG_ARGUMENT_ERROR              = 9028,  ///< Invalid log format arguments
    LOG_ROTATION_ERROR              = 9029,  ///< File rotation failed
    
    // Tracing Errors (9030-9039)
    TRACE_SPAN_CREATE_FAILED        = 9030,  ///< Failed to create span
    TRACE_EXPORT_FAILED             = 9031,  ///< Failed to export traces
    TRACE_BUFFER_OVERFLOW           = 9032,  ///< Trace buffer exhausted
    TRACE_INVALID_CONTEXT           = 9033,  ///< Invalid trace context
    TRACE_SAMPLING_FAILED           = 9034,  ///< Sampler decision failed
    TRACE_BATCH_FAILED              = 9035,  ///< Batch processing failed
    
    // Privacy Detection Errors (9040-9049)
    PRIVACY_INVALID_INPUT           = 9040,  ///< Input validation failed
    PRIVACY_PATTERN_OVERFLOW        = 9041,  ///< Pattern complexity exceeded limit
    PRIVACY_DETECTION_TIMEOUT       = 9042,  ///< Detection exceeded time budget
    PRIVACY_BUFFER_OVERFLOW         = 9043,  ///< Detection result buffer full
    PRIVACY_ENGINE_LOAD_FAILED      = 9044,  ///< Failed to load detection engine
    PRIVACY_CONFIG_INVALID          = 9045,  ///< Invalid privacy configuration
    PRIVACY_UNICODE_ERROR           = 9046,  ///< Unicode/encoding handling failed
    PRIVACY_MEMORY_EXCEEDED         = 9047,  ///< Memory limit exceeded for detection
    PRIVACY_NO_ENGINE               = 9048,  ///< No suitable detection engine available
    PRIVACY_ENGINE_FAILED           = 9049,  ///< Detection engine threw exception
    
    // Cryptography Errors (9050-9059)
    CRYPTO_KEY_DERIVATION_FAILED    = 9050,  ///< HKDF derivation failed
    CRYPTO_KEY_INVALID              = 9051,  ///< Key validation failed
    CRYPTO_KEY_EXPIRED              = 9052,  ///< Key has expired
    CRYPTO_KEY_NOT_FOUND            = 9053,  ///< Key lookup failed
    CRYPTO_CACHE_MISS               = 9054,  ///< Derived key cache miss + derivation failed
    CRYPTO_CERT_LOAD_FAILED         = 9055,  ///< Certificate loading failed
    CRYPTO_CERT_INVALID             = 9056,  ///< Certificate validation failed
    CRYPTO_CERT_EXPIRED             = 9057,  ///< Certificate has expired
    CRYPTO_ENCRYPTION_FAILED        = 9058,  ///< Encryption operation failed
    CRYPTO_DECRYPTION_FAILED        = 9059,  ///< Decryption operation failed
    
    // Compression Errors (9060-9069)
    COMPRESSION_FAILED              = 9060,  ///< Compression operation failed
    DECOMPRESSION_FAILED            = 9061,  ///< Decompression operation failed
    COMPRESSION_BUFFER_SMALL        = 9062,  ///< Output buffer too small
    COMPRESSION_INPUT_INVALID       = 9063,  ///< Invalid compressed input
    COMPRESSION_BOMB_DETECTED       = 9064,  ///< Decompression explosion detected
    COMPRESSION_RATIO_EXCEEDED      = 9065,  ///< Compression ratio limit exceeded
    CODEC_INITIALIZATION_FAILED     = 9066,  ///< Codec setup failed
    CODEC_NOT_SUPPORTED             = 9067,  ///< Requested codec not available
    
    // Concurrency Errors (9070-9079)
    THREADPOOL_QUEUE_FULL           = 9070,  ///< Task queue at capacity
    THREADPOOL_SHUTDOWN             = 9071,  ///< Pool is shutting down
    THREADPOOL_INVALID_STATE        = 9072,  ///< Invalid thread pool state
    RATELIMIT_EXCEEDED              = 9073,  ///< Rate limit quota exhausted
    RATELIMIT_WINDOW_ERROR          = 9074,  ///< Rate limit window computation failed
    CONNECTION_POOL_EXHAUSTED       = 9075,  ///< Connection pool at capacity
    CONNECTION_POOL_TIMEOUT         = 9076,  ///< Connection acquisition timeout
    LOCK_ACQUISITION_FAILED         = 9077,  ///< Failed to acquire lock
    LOCK_TIMEOUT                    = 9078,  ///< Lock acquisition timeout
    CONCURRENT_MODIFICATION         = 9079,  ///< Concurrent modification detected
    
    // Serialization Errors (9080-9089)
    SERIALIZATION_FAILED            = 9080,  ///< Object serialization failed
    DESERIALIZATION_FAILED          = 9081,  ///< Object deserialization failed
    SERIALIZATION_FORMAT_INVALID    = 9082,  ///< Invalid serialization format
    SERIALIZATION_VERSION_MISMATCH  = 9083,  ///< Version incompatibility
    SERIALIZATION_SIZE_EXCEEDED     = 9084,  ///< Serialized size limit exceeded
     
    // SAGA Logging Errors (9090-9098)
    SAGA_EVENT_LOSS                 = 9090,  ///< SAGA event loss due to buffer overflow
    SAGA_SERIALIZATION_FAILED       = 9091,  ///< SAGA step serialization failed
     
    // Catchall
    UNKNOWN_ERROR                   = 9099
};

// ─────────────────────────────────────────────────────────────────────────────
// Error Context & Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct ErrorContext
 * @brief Complete error context for diagnostic and recovery decisions.
 * 
 * Captures the error code, category, severity, and rich diagnostic information
 * to enable operators to understand what went wrong and how to respond.
 * 
 * Thread-safe for read access; created and populated by error sources.
 */
struct ErrorContext {
    // Error identification
    ErrorCode code;                              ///< Specific error code
    ErrorCategory category;                      ///< Subsystem category
    ErrorSeverity severity;                      ///< Impact level
    
    // Timing information
    std::chrono::system_clock::time_point timestamp;  ///< When error occurred
    std::chrono::milliseconds elapsed_ms;       ///< Duration of failed operation
    
    // Diagnostic information
    std::string message;                         ///< Human-readable error message
    std::string component;                       ///< Component/function where error occurred
    std::string context_info;                    ///< Additional context (bounded size)
    
    // Recovery information
    std::string recovery_hint;                   ///< Suggested recovery action
    bool is_recoverable;                         ///< Whether recovery is possible
    uint32_t retry_count;                        ///< Number of retries attempted
    
    // Resource state at time of error
    uint64_t resource_limit;                     ///< Relevant resource limit (if applicable)
    uint64_t resource_current;                   ///< Current resource usage
    
    /**
     * @brief Convenient constructor for common error scenarios
     */
    ErrorContext(ErrorCode code_, const std::string& message_, 
                 const std::string& component_)
        : code(code_), severity(ErrorSeverity::Error), timestamp(std::chrono::system_clock::now()),
          elapsed_ms(0), message(message_), component(component_), 
          is_recoverable(false), retry_count(0),
          resource_limit(0), resource_current(0) {
        category = ErrorCategory::Unknown;
    }
    
    /// Default constructor for aggregation
    ErrorContext() : code(ErrorCode::UNKNOWN_ERROR), severity(ErrorSeverity::Fatal),
                     timestamp(std::chrono::system_clock::now()), elapsed_ms(0),
                     is_recoverable(false), retry_count(0),
                     resource_limit(0), resource_current(0) {
        category = ErrorCategory::Unknown;
    }
    
    /**
     * @brief Convert error context to structured log-friendly format
     * @return JSON representation for structured logging
     */
    std::string toJSON() const;
    
    /**
     * @brief Produce human-readable diagnostic summary
     * @return Formatted string suitable for error reports
     */
    std::string toFormattedString() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Error Contract Descriptors
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct ErrorContract
 * @brief Machine-readable contract describing error conditions, severity, and recovery.
 * 
 * Used in API documentation to specify:
 * - When an error occurs (preconditions/postconditions)
 * - What will be logged (diagnostic information)
 * - How to recover (fallback strategies)
 * - Downstream impact (who is affected)
 * 
 * This is the "machine-readable" complement to @error_contract Doxygen tags.
 */
struct ErrorContract {
    ErrorCode code;                       ///< Error code for this condition
    ErrorSeverity severity;               ///< Severity classification
    std::string when_occurs;              ///< Condition description
    std::string diagnostic_logged;        ///< What diagnostic info is logged
    std::string recovery_strategy;        ///< How to recover
    std::string downstream_impact;        ///< Who is affected downstream
    std::vector<ErrorCode> related_codes; ///< Other errors in same subsystem
};

// ─────────────────────────────────────────────────────────────────────────────
// Incident Categorization for Operator Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @enum IncidentCategory
 * @brief High-level incident categorization for operator dashboards and alerts.
 * 
 * Maps multiple error codes to operator-visible incident categories.
 * Used to quickly triage and respond to utils subsystem failures.
 * 
 * Each category corresponds to a distinct degradation mode that operators
 * should handle differently (e.g., "memory exhaustion" vs "timeout").
 */
enum class IncidentCategory : uint8_t {
    // Capacity/Resource exhaustion incidents
    BufferOverflow           = 0,  ///< Log/audit/trace queue full; potential data loss
    MemoryExhaustion         = 1,  ///< Memory limit exceeded; service degraded
    ConnectionPoolExhausted  = 2,  ///< Connection pool at capacity; requests queued
    ThreadPoolOverload       = 3,  ///< Task queue full; tasks being rejected
    
    // Timeout incidents
    DetectionTimeout         = 4,  ///< Privacy/PII detection exceeded time budget
    OperationTimeout         = 5,  ///< Lock/connection/operation timeout
    
    // Failure incidents
    KeyDerivationFailure     = 6,  ///< Cryptographic key derivation failed
    PrivacyDetectionFailure  = 7,  ///< PII detection engine threw exception
    CompressionFailure       = 8,  ///< Compression/decompression operation failed
    
    // Degradation incidents
    FallbackActivated        = 9,  ///< Fallback strategy activated; reduced capability
    RateLimitExhausted       = 10, ///< Rate limit quota consumed; requests queued
    
    // Data/Configuration incidents
    InvalidConfiguration     = 11, ///< Bad config/policy/pattern data
    DataCorruption           = 12, ///< Integrity check failed; data invalid
    
    // External/Environment incidents
    ExternalServiceUnavailable = 13, ///< HSM, PKI, external service not responding
    
    // Unknown
    UnclassifiedIncident     = 255 ///< Unmapped incident
};

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostic Logging Helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Log an error with full diagnostic context
 * 
 * Bounded logging that includes error code, category, severity, and
 * suggested recovery action. Uses structured logging (spdlog/JSON) for
 * machine parsing.
 * 
 * @param ctx Error context to log
 * @param logger Logger instance (defaults to global logger)
 */
void logErrorWithContext(const ErrorContext& ctx,
                         std::shared_ptr<spdlog::logger> logger = nullptr);

/**
 * @brief Create ErrorContext from error code and message
 * 
 * Convenience factory to construct well-formed error contexts.
 * 
 * @param code Error code
 * @param message Human-readable message
 * @param component Component/function name
 * @param severity Error severity level
 * @param is_recoverable Whether recovery is possible
 * @return Populated ErrorContext
 */
ErrorContext makeErrorContext(ErrorCode code,
                              const std::string& message,
                              const std::string& component,
                              ErrorSeverity severity,
                              bool is_recoverable);

/**
 * @brief Categorize an error into an operator-visible incident
 * 
 * Maps error codes to high-level incident categories for monitoring
 * and operator dashboard display.
 * 
 * @param code Error code to categorize
 * @return Incident category (or UnclassifiedIncident if unmapped)
 */
IncidentCategory categorizeIncident(ErrorCode code);

/**
 * @brief Get human-readable name for error code
 * 
 * @param code Error code
 * @return String name (e.g., "AUDIT_BUFFER_OVERFLOW")
 */
std::string errorCodeName(ErrorCode code);

/**
 * @brief Get human-readable name for error category
 * 
 * @param category Category enum value
 * @return String name (e.g., "AuditLog")
 */
std::string categoryName(ErrorCategory category);

/**
 * @brief Get human-readable name for error severity
 * 
 * @param severity Severity enum value
 * @return String name (e.g., "Fatal", "Error", "Warning")
 */
std::string severityName(ErrorSeverity severity);

/**
 * @brief Get human-readable name for incident category
 * 
 * @param incident Incident category enum value
 * @return String name (e.g., "BufferOverflow")
 */
std::string incidentName(IncidentCategory incident);

} // namespace utils
} // namespace themis
