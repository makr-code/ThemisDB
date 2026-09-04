/**
 * @file content_logger.cpp
 * @brief Content module logging infrastructure with structured logging capabilities.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=2, L=0
 * @note Status: Production Ready; Structured logging complete; real-time metrics streaming deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_logger.h"
#include <regex>
#include <sstream>
#include <iomanip>

namespace themis {
namespace content {

// ============================================================================
// ContentLogger
// ============================================================================

ContentLogger::ContentLogger(const std::string& correlation_id)
    : correlation_id_(correlation_id)
{}

void ContentLogger::setCorrelationId(const std::string& correlation_id) {
    correlation_id_ = correlation_id;
}

// ============================================================================
// Content Operation Logging
// ============================================================================

void ContentLogger::logIngestion(
    const std::string& content_id,
    const std::string& mime_type,
    uint64_t size_bytes,
    const std::string& filename
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["mime_type"] = mime_type;
    metadata["size_bytes"] = size_bytes;
    
    if (!filename.empty()) {
        metadata["filename"] = pii_sanitization_ ? sanitizeFilename(filename) : filename;
    }
    
    info("content.ingestion", "Content ingested", metadata);
}

void ContentLogger::logValidation(
    const std::string& content_id,
    const std::string& mime_type,
    uint64_t size_bytes,
    bool success,
    int error_code,
    double duration_ms
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["mime_type"] = mime_type;
    metadata["size_bytes"] = size_bytes;
    metadata["success"] = success;
    metadata["duration_ms"] = duration_ms;
    
    if (!success && error_code != 0) {
        metadata["error_code"] = error_code;
    }
    
    if (success) {
        info("content.validation.success", "Content validation succeeded", metadata);
    } else {
        warn("content.validation.failure", "Content validation failed", metadata);
    }
}

void ContentLogger::logProcessing(
    const std::string& content_id,
    const std::string& operation,
    double duration_ms,
    bool success,
    int error_code
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["operation"] = operation;
    metadata["duration_ms"] = duration_ms;
    metadata["success"] = success;
    
    if (!success && error_code != 0) {
        metadata["error_code"] = error_code;
    }
    
    std::string event = "content.processing." + operation + (success ? ".success" : ".failure");
    std::string message = "Content " + operation + (success ? " succeeded" : " failed");
    
    if (success) {
        info(event, message, metadata);
    } else {
        warn(event, message, metadata);
    }
}

void ContentLogger::logError(
    const std::string& content_id,
    const std::string& operation,
    int error_code,
    const std::string& error_message,
    const std::string& error_category
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["operation"] = operation;
    metadata["error_code"] = error_code;
    metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;
    
    if (!error_category.empty()) {
        metadata["error_category"] = error_category;
    }
    
    error("content.error", "Content processing error", metadata);
}

void ContentLogger::logTimeout(
    const std::string& content_id,
    const std::string& operation,
    double timeout_seconds,
    double elapsed_seconds
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["operation"] = operation;
    metadata["timeout_seconds"] = timeout_seconds;
    metadata["elapsed_seconds"] = elapsed_seconds;
    
    warn("content.timeout", "Content operation timed out", metadata);
}

void ContentLogger::logCache(
    const std::string& content_id,
    bool hit
) {
    json metadata;
    metadata["content_id"] = content_id;
    metadata["hit"] = hit;
    
    debug("content.cache." + std::string(hit ? "hit" : "miss"),
          "Content cache " + std::string(hit ? "hit" : "miss"),
          metadata);
}

// ============================================================================
// Structured Logging with Metadata
// ============================================================================

void ContentLogger::log(
    utils::Logger::Level level,
    const std::string& event,
    const std::string& message,
    const json& metadata
) {
    std::string log_message = formatLogMessage(level, event, message, metadata);
    
    switch (level) {
        case utils::Logger::Level::TRACE:
            utils::Logger::trace(log_message);
            break;
        case utils::Logger::Level::DEBUG:
            utils::Logger::debug(log_message);
            break;
        case utils::Logger::Level::INFO:
            utils::Logger::info(log_message);
            break;
        case utils::Logger::Level::WARN:
            utils::Logger::warn(log_message);
            break;
        case utils::Logger::Level::ERROR:
            utils::Logger::error(log_message);
            break;
        case utils::Logger::Level::CRITICAL:
            utils::Logger::critical(log_message);
            break;
    }
}

void ContentLogger::info(const std::string& event, const std::string& message, const json& metadata) {
    log(utils::Logger::Level::INFO, event, message, metadata);
}

void ContentLogger::warn(const std::string& event, const std::string& message, const json& metadata) {
    log(utils::Logger::Level::WARN, event, message, metadata);
}

void ContentLogger::error(const std::string& event, const std::string& message, const json& metadata) {
    log(utils::Logger::Level::ERROR, event, message, metadata);
}

void ContentLogger::debug(const std::string& event, const std::string& message, const json& metadata) {
    log(utils::Logger::Level::DEBUG, event, message, metadata);
}

// ============================================================================
// Helper Methods
// ============================================================================

std::string ContentLogger::sanitizeFilename(const std::string& filename) const {
    // Static regex patterns compiled once
    static const std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    static const std::regex phone_regex(R"(\+?[0-9]{1,4}?[-.\s]?\(?[0-9]{1,3}?\)?[-.\s]?[0-9]{1,4}[-.\s]?[0-9]{1,4}[-.\s]?[0-9]{1,9})");
    static const std::regex ssn_regex(R"(\b\d{3}-?\d{2}-?\d{4}\b)");
    
    // Remove directory path, keep only basename
    size_t pos = filename.find_last_of("/\\");
    std::string basename = (pos != std::string::npos) ? filename.substr(pos + 1) : filename;
    
    // Replace email addresses with [EMAIL]
    basename = std::regex_replace(basename, email_regex, "[EMAIL]");
    
    // Replace phone numbers with [PHONE]
    basename = std::regex_replace(basename, phone_regex, "[PHONE]");
    
    // Replace SSN patterns with [SSN]
    basename = std::regex_replace(basename, ssn_regex, "[SSN]");
    
    return basename;
}

std::string ContentLogger::sanitizeMessage(const std::string& message) const {
    // Static regex patterns compiled once
    static const std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    static const std::regex phone_regex(R"(\+?[0-9]{1,4}?[-.\s]?\(?[0-9]{1,3}?\)?[-.\s]?[0-9]{1,4}[-.\s]?[0-9]{1,4}[-.\s]?[0-9]{1,9})");
    static const std::regex ssn_regex(R"(\b\d{3}-?\d{2}-?\d{4}\b)");
    static const std::regex cc_regex(R"(\b\d{4}[-\s]?\d{4}[-\s]?\d{4}[-\s]?\d{4}\b)");
    
    std::string sanitized = message;
    
    // Replace email addresses
    sanitized = std::regex_replace(sanitized, email_regex, "[EMAIL]");
    
    // Replace phone numbers
    sanitized = std::regex_replace(sanitized, phone_regex, "[PHONE]");
    
    // Replace SSN patterns
    sanitized = std::regex_replace(sanitized, ssn_regex, "[SSN]");
    
    // Replace credit card patterns (4 groups of 4 digits)
    sanitized = std::regex_replace(sanitized, cc_regex, "[CREDIT_CARD]");
    
    // Replace IP addresses (optional - may be needed for diagnostics)
    // static const std::regex ip_regex(R"(\b\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}\b)");
    // sanitized = std::regex_replace(sanitized, ip_regex, "[IP]");
    
    return sanitized;
}

std::string ContentLogger::formatLogMessage(
    utils::Logger::Level level,
    const std::string& event,
    const std::string& message,
    const json& metadata
) const {
    if (!json_formatting_) {
        // Simple text format
        std::ostringstream oss = {};
        oss << "[" << event << "] " << message;
        if (!correlation_id_.empty()) {
            oss << " (correlation_id=" << correlation_id_ << ")";
        }
        return oss.str();
    }
    
    // JSON format
    json log_entry;
    log_entry["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    log_entry["level"] = utils::Logger::levelToString(level);
    log_entry["event"] = event;
    log_entry["message"] = message;
    
    if (!correlation_id_.empty()) {
        log_entry["correlation_id"] = correlation_id_;
    }
    
    // Merge metadata
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        log_entry[it.key()] = it.value();
    }
    
    return log_entry.dump();
}

json ContentLogger::buildBaseMetadata() const {
    json metadata = {};
    
    if (!correlation_id_.empty()) {
        metadata["correlation_id"] = correlation_id_;
    }
    
    return metadata;
}

// ============================================================================
// ContentOperationTimer
// ============================================================================

ContentOperationTimer::ContentOperationTimer(
    ContentLogger& logger,
    const std::string& content_id,
    const std::string& operation
)
    : logger_(logger)
    , content_id_(content_id)
    , operation_(operation)
    , start_time_(std::chrono::steady_clock::now())
{}

ContentOperationTimer::~ContentOperationTimer() {
    double duration_ms = getElapsedMs();
    logger_.logProcessing(content_id_, operation_, duration_ms, success_, error_code_);
}

double ContentOperationTimer::getElapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<double>(duration.count());
}

} // namespace content
} // namespace themis
