/**
 * @file content_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
     * @param[in] correlation_id Input parameter.
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
     * @details Implements setJsonFormatting without additional internal calls.
     */
    void setJsonFormatting(bool enabled) { json_formatting_ = enabled; }
    
    /**
     * @brief Check if JSON formatting is enabled
     */
    bool isJsonFormatting() const { return json_formatting_; }
    
    /**
     * @brief Enable/disable PII sanitization
     * @param enabled If true, filenames and messages are sanitized
     * @details Implements setPiiSanitization without additional internal calls.
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
    
    /**
     * @brief Helper methods
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    std::string sanitizeFilename(const std::string& filename) const;
    /**
     * @brief TBD: Describe sanitizeMessage.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    std::string sanitizeMessage(const std::string& message) const;
    /**
     * @brief TBD: Describe formatLogMessage.
     * @param[in] level Input parameter.
     * @param[in] event Input parameter.
     * @param[in] message Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    std::string formatLogMessage(
        utils::Logger::Level level,
        const std::string& event,
        const std::string& message,
        const json& metadata
    ) const;
    /**
     * @brief TBD: Describe buildBaseMetadata.
     * @return Return value.
     */
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
     * @param[in] success Input parameter.
     * @details Implements setSuccess without additional internal calls.
     */
    void setSuccess(bool success) { success_ = success; }
    
    /**
     * @brief Set error code
     * @param[in] error_code Input parameter.
     * @details Implements setErrorCode without additional internal calls.
     */
    void setErrorCode(int error_code) { error_code_ = error_code; }
    
    /**
     * @brief Get elapsed time in milliseconds
     * @return Return value.
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
