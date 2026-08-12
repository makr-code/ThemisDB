/**
 * @file content_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/logger.h"
#include <string>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Structured logger for content operations
 * 
 * Provides structured logging with:
 * - JSON formatting for log aggregation (ELK, Loki, Splunk)
 * - Correlation ID tracking for distributed tracing
 * - PII sanitization
 * - Content metadata logging
 * - Performance timing
 * 
 * Usage:
 *   ContentLogger logger(correlation_id);
 *   logger.logValidation(content_id, mime_type, size, success);
 *   logger.logProcessing(content_id, operation, duration_ms, success);
 */
class ContentLogger {
public:
    /**
     * @brief Construct logger with optional correlation ID
     * @param correlation_id Optional correlation ID for request tracing
     */
    explicit ContentLogger(const std::string& correlation_id = "");
    
    /**
     * @brief Set correlation ID
     */
    void setCorrelationId(const std::string& correlation_id);
    
    /**
     * @brief Get current correlation ID
     */
    const std::string& getCorrelationId() const { return correlation_id_; }
    
    // ========================================================================
    // Content Operation Logging
    // ========================================================================
    
    /**
     * @brief Log content ingestion
     * @param content_id Content identifier
     * @param mime_type MIME type
     * @param size_bytes Size in bytes
     * @param filename Original filename (will be sanitized)
     */
    void logIngestion(
        const std::string& content_id,
        const std::string& mime_type,
        uint64_t size_bytes,
        const std::string& filename = ""
    );
    
    /**
     * @brief Log content validation
     * @param content_id Content identifier
     * @param mime_type MIME type
     * @param size_bytes Size in bytes
     * @param success Whether validation passed
     * @param error_code Optional error code if validation failed
     * @param duration_ms Validation duration in milliseconds
     */
    void logValidation(
        const std::string& content_id,
        const std::string& mime_type,
        uint64_t size_bytes,
        bool success,
        int error_code = 0,
        double duration_ms = 0.0
    );
    
    /**
     * @brief Log content processing
     * @param content_id Content identifier
     * @param operation Operation name (extraction, chunking, embedding)
     * @param duration_ms Operation duration in milliseconds
     * @param success Whether operation succeeded
     * @param error_code Optional error code if failed
     */
    void logProcessing(
        const std::string& content_id,
        const std::string& operation,
        double duration_ms,
        bool success,
        int error_code = 0
    );
    
    /**
     * @brief Log error
     * @param content_id Content identifier
     * @param operation Operation that failed
     * @param error_code Error code
     * @param error_message Error message (will be sanitized)
     * @param error_category Error category
     */
    void logError(
        const std::string& content_id,
        const std::string& operation,
        int error_code,
        const std::string& error_message,
        const std::string& error_category = ""
    );
    
    /**
     * @brief Log timeout
     * @param content_id Content identifier
     * @param operation Operation that timed out
     * @param timeout_seconds Timeout value in seconds
     * @param elapsed_seconds Elapsed time in seconds
     */
    void logTimeout(
        const std::string& content_id,
        const std::string& operation,
        double timeout_seconds,
        double elapsed_seconds
    );
    
    /**
     * @brief Log cache operation
     * @param content_id Content identifier
     * @param hit Whether cache hit or miss
     */
    void logCache(
        const std::string& content_id,
        bool hit
    );
    
    // ========================================================================
    // Structured Logging with Metadata
    // ========================================================================
    
    /**
     * @brief Log with custom metadata
     * @param level Log level
     * @param event Event name
     * @param message Human-readable message
     * @param metadata Additional structured metadata
     */
    void log(
        utils::Logger::Level level,
        const std::string& event,
        const std::string& message,
        const json& metadata = json::object()
    );
    
    /**
     * @brief Log informational message
     */
    void info(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    /**
     * @brief Log warning message
     */
    void warn(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    /**
     * @brief Log error message
     */
    void error(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    /**
     * @brief Log debug message
     */
    void debug(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * @brief Enable/disable JSON formatting
     * @param enabled If true, logs are formatted as JSON
     */
    void setJsonFormatting(bool enabled) { json_formatting_ = enabled; }
    
    /**
     * @brief Check if JSON formatting is enabled
     */
    bool isJsonFormatting() const { return json_formatting_; }
    
    /**
     * @brief Enable/disable PII sanitization
     * @param enabled If true, filenames and messages are sanitized
     */
    void setPiiSanitization(bool enabled) { pii_sanitization_ = enabled; }
    
    /**
     * @brief Check if PII sanitization is enabled
     */
    bool isPiiSanitization() const { return pii_sanitization_; }
    
private:
    std::string correlation_id_;
    bool json_formatting_ = true;
    bool pii_sanitization_ = true;
    
    // Helper methods
    std::string sanitizeFilename(const std::string& filename) const;
    std::string sanitizeMessage(const std::string& message) const;
    std::string formatLogMessage(
        utils::Logger::Level level,
        const std::string& event,
        const std::string& message,
        const json& metadata
    ) const;
    json buildBaseMetadata() const;
};

/**
 * @brief RAII helper for operation timing and logging
 * 
 * Usage:
 *   ContentLogger logger(correlation_id);
 *   {
 *       auto timer = ContentOperationTimer(logger, content_id, "extraction");
 *       // ... perform extraction ...
 *       timer.setSuccess(true);
 *   }  // Automatically logs on destruction
 */
class ContentOperationTimer {
public:
    ContentOperationTimer(
        ContentLogger& logger,
        const std::string& content_id,
        const std::string& operation
    );
    
    ~ContentOperationTimer();
    
    /**
     * @brief Set whether operation succeeded
     */
    void setSuccess(bool success) { success_ = success; }
    
    /**
     * @brief Set error code
     */
    void setErrorCode(int error_code) { error_code_ = error_code; }
    
    /**
     * @brief Get elapsed time in milliseconds
     */
    double getElapsedMs() const;
    
private:
    ContentLogger& logger_;
    std::string content_id_;
    std::string operation_;
    std::chrono::steady_clock::time_point start_time_;
    bool success_ = true;
    int error_code_ = 0;
};

} // namespace content
} // namespace themis
