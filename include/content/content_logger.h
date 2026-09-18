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

class ContentLogger {
public:
    explicit ContentLogger(const std::string& correlation_id = "");
    
    /**
     * @brief Set Correlation Id.
     * @param[in] correlation_id Identifier of the correlation.
     */
    void setCorrelationId(const std::string& correlation_id);
    
    const std::string& getCorrelationId() const { return correlation_id_; }
    
    // ========================================================================
    // Content Operation Logging
    // ========================================================================
    
    void logIngestion(
        const std::string& content_id,
        const std::string& mime_type,
        uint64_t size_bytes,
        const std::string& filename = ""
    );
    
    void logValidation(
        const std::string& content_id,
        const std::string& mime_type,
        uint64_t size_bytes,
        bool success,
        int error_code = 0,
        double duration_ms = 0.0
    );
    
    void logProcessing(
        const std::string& content_id,
        const std::string& operation,
        double duration_ms,
        bool success,
        int error_code = 0
    );
    
    void logError(
        const std::string& content_id,
        const std::string& operation,
        int error_code,
        const std::string& error_message,
        const std::string& error_category = ""
    );
    
    /**
     * @brief Log Timeout.
     * @param[in] content_id Identifier of the content.
     * @param[in] operation Input parameter.
     * @param[in] timeout_seconds Input parameter.
     * @param[in] elapsed_seconds Input parameter.
     */
    void logTimeout(
        const std::string& content_id,
        const std::string& operation,
        double timeout_seconds,
        double elapsed_seconds
    );
    
    /**
     * @brief Log Cache.
     * @param[in] content_id Identifier of the content.
     * @param[in] hit Input parameter.
     */
    void logCache(
        const std::string& content_id,
        bool hit
    );
    
    // ========================================================================
    // Structured Logging with Metadata
    // ========================================================================
    
    void log(
        utils::Logger::Level level,
        const std::string& event,
        const std::string& message,
        const json& metadata = json::object()
    );
    
    void info(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    void warn(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    void error(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    void debug(const std::string& event, const std::string& message, const json& metadata = json::object());
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    /**
     * @brief Set Json Formatting.
     * @param[in] enabled Input parameter.
     * @details Implements setJsonFormatting without additional internal calls.
     */
    void setJsonFormatting(bool enabled) { json_formatting_ = enabled; }
    
    bool isJsonFormatting() const { return json_formatting_; }
    
    /**
     * @brief Set Pii Sanitization.
     * @param[in] enabled Input parameter.
     * @details Implements setPiiSanitization without additional internal calls.
     */
    void setPiiSanitization(bool enabled) { pii_sanitization_ = enabled; }
    
    bool isPiiSanitization() const { return pii_sanitization_; }
    
private:
    std::string correlation_id_;
    bool json_formatting_ = true;
    bool pii_sanitization_ = true;
    
    // Helper methods
    /**
     * @brief Sanitize Filename.
     * @param[in] filename Input parameter.
     * @return Return value.
     */
    std::string sanitizeFilename(const std::string& filename) const;
    /**
     * @brief Sanitize Message.
     * @param[in] message Input parameter.
     * @return Return value.
     */
    std::string sanitizeMessage(const std::string& message) const;
    /**
     * @brief Format Log Message.
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
     * @brief Build Base Metadata.
     * @return Return value.
     */
    json buildBaseMetadata() const;
};

class ContentOperationTimer {
public:
    ContentOperationTimer(
        ContentLogger& logger,
        const std::string& content_id,
        const std::string& operation
    );
    
    ~ContentOperationTimer();
    
    /**
     * @brief Set Success.
     * @param[in] success Input parameter.
     * @details Implements setSuccess without additional internal calls.
     */
    void setSuccess(bool success) { success_ = success; }
    
    /**
     * @brief Set Error Code.
     * @param[in] error_code Input parameter.
     * @details Implements setErrorCode without additional internal calls.
     */
    void setErrorCode(int error_code) { error_code_ = error_code; }
    
    /**
     * @brief Get Elapsed Ms.
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
