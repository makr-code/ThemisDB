/**
 * @file content_policy.h
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
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

namespace themis {
namespace content {

/// Policy rule for a specific MIME type
struct MimePolicy {
    std::string mime_type;
    uint64_t max_size = 0;  ///< Maximum file size in bytes (0 = unlimited)
    std::string description;
    std::string reason;     ///< Reason for denial (for blacklist entries)
};

/// Category-based policy rule
struct CategoryPolicy {
    std::string category;
    bool action;  ///< true = allow, false = deny
    uint64_t max_size = 0;
    std::string reason;
};

/// Content upload validation policy
struct ContentPolicy {
    uint64_t default_max_size = 104857600;  ///< 100 MB default
    bool default_action = true;   ///< true = allow, false = deny
    
    std::vector<MimePolicy> allowed;
    std::vector<MimePolicy> denied;
    std::map<std::string, CategoryPolicy> category_rules;

    /// Local model identifier used to activate the embedding generation pipeline
    /// for ingested text content.  An empty string disables embedding generation.
    /// Matches FUTURE_ENHANCEMENTS.md: "activated when ContentPolicy::embeddingModel is set".
    std::string embedding_model;

    /// Enable near-duplicate detection for this collection.
    /// When true, `ContentManager::ingestRawBlob()` computes a perceptual hash
    /// (pHash for images, MinHash for text) and rejects near-duplicates before
    /// committing to storage.  Default: false (opt-in per collection).
    bool enable_deduplication = false;

    /// Enable automatic OCR extraction for image content in this collection.
    /// When true, MimeDetector::shouldTriggerOcr() returns true for image/png,
    /// image/jpeg, and image/tiff MIME types, routing them through OcrProcessor.
    /// Default: false (opt-in per collection).
    bool ocr_enabled = false;

    /// Returns true when automatic OCR is enabled for this policy.
    bool ocrEnabled() const { return ocr_enabled; }

    /// Check if a MIME type is explicitly allowed
    bool isAllowed(const std::string& mime_type) const;
    
    /// Check if a MIME type is explicitly denied
    bool isDenied(const std::string& mime_type) const;
    
    /// Get max size for a specific MIME type (returns 0 if unlimited)
    uint64_t getMaxSize(const std::string& mime_type) const;
    
    /// Get max size for a category
    uint64_t getCategoryMaxSize(const std::string& category) const;
    
    /// Get denial reason (empty if allowed)
    std::string getDenialReason(const std::string& mime_type) const;
};

/// Validation result for file upload
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
