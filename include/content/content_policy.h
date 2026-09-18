/**
 * @file content_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

namespace themis {
namespace content {

struct MimePolicy {
    std::string mime_type;
    uint64_t max_size = 0;  ///< Maximum file size in bytes (0 = unlimited)
    std::string description;
    std::string reason;     ///< Reason for denial (for blacklist entries)
};

struct CategoryPolicy {
    std::string category;
    bool action;  ///< true = allow, false = deny
    uint64_t max_size = 0;
    std::string reason;
};

struct ContentPolicy {
    uint64_t default_max_size = 104857600;  ///< 100 MB default
    bool default_action = true;   ///< true = allow, false = deny
    
    std::vector<MimePolicy> allowed;
    std::vector<MimePolicy> denied;
    std::map<std::string, CategoryPolicy> category_rules;

    std::string embedding_model;

    bool enable_deduplication = false;

    bool ocr_enabled = false;

    bool ocrEnabled() const { return ocr_enabled; }

    /**
     * @brief Is Allowed.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAllowed(const std::string& mime_type) const;
    
    /**
     * @brief Is Denied.
     * @param[in] mime_type Input parameter.
     * @return True when the operation succeeds.
     */
    bool isDenied(const std::string& mime_type) const;
    
    /**
     * @brief Get Max Size.
     * @param[in] mime_type Input parameter.
     * @return Return value.
     */
    uint64_t getMaxSize(const std::string& mime_type) const;
    
    /**
     * @brief Get Category Max Size.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    uint64_t getCategoryMaxSize(const std::string& category) const;
    
    /**
     * @brief Get Denial Reason.
     * @param[in] mime_type Input parameter.
     * @return Return value.
     */
    std::string getDenialReason(const std::string& mime_type) const;
};

struct ValidationResult {
    bool allowed = false;
    std::string mime_type;
    uint64_t file_size = 0;
    uint64_t max_allowed_size = 0;
    std::string reason;  ///< Human-readable explanation
    
    // Additional context
    bool size_exceeded = false;
    bool blacklisted = false;
    bool not_whitelisted = false;
    bool ocr_recommended = false;  ///< OCR should be triggered (image MIME type + policy ocr_enabled)
};

} // namespace content
} // namespace themis
