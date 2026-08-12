/**
 * @file content_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "content/content_errors.h"
#include "content/content_policy.h"
#include "content/content_type.h"
#include <string>
#include <optional>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Configuration for content validation
 */
struct ContentValidationConfig {
    // Size limits
    uint64_t max_content_size = 100 * 1024 * 1024;  // 100 MB default
    uint64_t max_text_length = 10 * 1024 * 1024;     // 10 MB for text content
    
    // Processing limits
    std::chrono::seconds max_processing_time{300};    // 5 minutes default
    std::chrono::seconds extraction_timeout{60};      // 1 minute for extraction
    std::chrono::seconds chunking_timeout{120};       // 2 minutes for chunking
    std::chrono::seconds embedding_timeout{180};      // 3 minutes for embeddings
    
    // Format validation
    bool enforce_mime_type_validation = true;
    bool enforce_format_verification = true;
    bool check_file_magic_bytes = true;
    
    // Content safety
    bool check_for_malware = true;
    bool scan_for_pii = false;  // Optional, may have performance impact
    bool check_for_abuse = false;
    
    // Schema validation
    bool enable_schema_validation = false;
    std::string schema_path;  // Path to JSON schema for validation
    
    json toJson() const;
    static ContentValidationConfig fromJson(const json& j);
};

/**
 * @brief Result of content validation
 */
struct ContentValidationResult {
    ContentError error;  // Error details (OK if validation passed)
    
    // Validation metadata
    std::string mime_type;
    ContentCategory category;
    uint64_t content_size = 0;
    std::chrono::milliseconds validation_time{0};
    
    // Specific validation flags
    bool mime_validated = false;
    bool size_validated = false;
    bool format_validated = false;
    bool policy_validated = false;
    bool malware_checked = false;
    
    json toJson() const;
};

/**
 * @brief Content Validator
 * 
 * Provides comprehensive input validation for content ingestion.
 * Validates:
 * - Content size limits
 * - MIME type and format
 * - Policy compliance
 * - File integrity
 * - Optional: Schema validation, malware scanning, PII detection
 * 
 * Usage:
 *   ContentValidator validator(config, policy);
 *   auto result = validator.validate(data, filename, correlation_id);
 *   if (result.error.failed()) {
 *       // Handle validation error
 *   }
 */
class ContentValidator {
public:
    explicit ContentValidator(
        const ContentValidationConfig& config = ContentValidationConfig{},
        const ContentPolicy* policy = nullptr
    );
    
    /**
     * @brief Validate content before processing
     * 
     * @param data Binary content to validate
     * @param filename Optional filename hint
     * @param correlation_id Optional correlation ID for tracing
     * @return ContentValidationResult with detailed validation status
     */
    ContentValidationResult validate(
        const std::string& data,
        const std::string& filename = "",
        const std::string& correlation_id = ""
    );
    
    /**
     * @brief Validate just the MIME type
     * 
     * @param mime_type MIME type to validate
     * @return ContentError (OK if valid)
     */
    ContentError validateMimeType(const std::string& mime_type);
    
    /**
     * @brief Validate content size
     * 
     * @param size Content size in bytes
     * @param mime_type Optional MIME type for type-specific limits
     * @return ContentError (OK if valid)
     */
    ContentError validateSize(uint64_t size, const std::string& mime_type = "");
    
    /**
     * @brief Validate content format (magic bytes check)
     * 
     * @param data Content data
     * @param expected_mime MIME type to verify against
     * @return ContentError (OK if valid)
     */
    ContentError validateFormat(
        const std::string& data,
        const std::string& expected_mime
    );
    
    /**
     * @brief Validate filename for security issues
     * 
     * Rejects filenames that contain:
     * - Path traversal sequences ("../" or "..\")
     * - Absolute path prefixes ("/" or "C:\")
     * - Null bytes or ASCII control characters
     * - Excessively long names
     * 
     * @param filename Filename to validate
     * @return ContentError (OK if safe)
     */
    ContentError validateFilename(const std::string& filename);
    
    /**
     * @brief Check if processing timeout has been exceeded
     * 
     * @param start_time Processing start time
     * @param operation_type Type of operation (extraction, chunking, etc.)
     * @return ContentError (OK if within timeout, CONTENT_TIMEOUT otherwise)
     */
    ContentError checkTimeout(
        const std::chrono::steady_clock::time_point& start_time,
        const std::string& operation_type = "processing"
    );
    
    /**
     * @brief Get timeout for specific operation
     * 
     * @param operation_type Operation type (extraction, chunking, embedding, processing)
     * @return Timeout duration
     */
    std::chrono::seconds getOperationTimeout(const std::string& operation_type) const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const ContentValidationConfig& config);
    const ContentValidationConfig& getConfig() const;
    
    /**
     * @brief Set content policy
     */
    void setPolicy(const ContentPolicy* policy);
    
    /**
     * @brief Validation statistics
     */
    struct Stats {
        uint64_t total_validations = 0;
        uint64_t successful_validations = 0;
        uint64_t failed_validations = 0;
        uint64_t size_violations = 0;
        uint64_t format_violations = 0;
        uint64_t policy_violations = 0;
        uint64_t timeouts = 0;
        
        json toJson() const;
    };
    const Stats& getStats() const;
    void resetStats();
    
private:
    ContentValidationConfig config_;
    const ContentPolicy* policy_;
    mutable Stats stats_;
    
    // Helper methods
    std::string detectMimeType(const std::string& data, const std::string& filename) const;
    ContentCategory mimeToCategory(const std::string& mime_type) const;
    bool checkMagicBytes(const std::string& data, const std::string& mime_type) const;
    ContentError validateWithPolicy(const std::string& mime_type, uint64_t size);
};

/**
 * @brief RAII helper for timeout checking
 * 
 * Usage:
 *   ContentValidator validator(config);
 *   TimeoutGuard guard(validator, "extraction");
 *   // ... perform extraction ...
 *   if (auto err = guard.check(); err.failed()) {
 *       // Handle timeout
 *   }
 */
class TimeoutGuard {
public:
    TimeoutGuard(ContentValidator& validator, const std::string& operation_type);
    
    /**
     * @brief Check if operation has timed out
     */
    ContentError check() const;
    
    /**
     * @brief Get elapsed time
     */
    std::chrono::milliseconds elapsed() const;
    
private:
    ContentValidator& validator_;
    std::string operation_type_;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace content
} // namespace themis
