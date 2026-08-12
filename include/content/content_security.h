/**
 * @file content_security.h
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
#include "content/abuse_detector.h"
#include "security/malware_scanner.h"
#include "utils/pii_detector.h"
#include "utils/audit_logger.h"
#include <string>
#include <memory>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Configuration for content security checks
 */
struct ContentSecurityConfig {
    // Malware scanning
    bool enable_malware_scan = true;
    bool block_on_malware = true;
    security::ThreatLevel malware_block_threshold = security::ThreatLevel::MEDIUM;
    
    // PII detection
    bool enable_pii_detection = false;  // Optional, performance impact
    bool block_on_pii = false;          // Usually just warn, don't block
    bool redact_pii_in_logs = true;     // Redact PII from log output
    
    // Abuse detection
    bool enable_abuse_detection = false;  ///< Enable abuse detection via registered IAbuseDetector instances
    bool block_on_abuse = false;
    
    // Error sanitization
    bool sanitize_error_messages = true;
    bool hide_internal_paths = true;
    bool hide_system_info = true;
    
    // Zip-bomb protection for archive ingestion
    bool enable_zip_bomb_check = true;
    uint64_t max_zip_bomb_ratio = 100;   // Max decompressed/compressed ratio (100×)
    size_t max_zip_file_count = 1000;    // Max number of files per archive
    
    json toJson() const;
    static ContentSecurityConfig fromJson(const json& j);
};

/**
 * @brief Result of security checks
 */
struct SecurityCheckResult {
    ContentError error;  // OK if all checks passed
    
    // Check results
    bool malware_checked = false;
    bool malware_clean = true;
    std::string malware_threat;
    
    bool pii_checked = false;
    bool pii_found = false;
    std::vector<std::string> pii_types;
    
    bool abuse_checked = false;
    bool abuse_detected = false;
    std::string abuse_action;        ///< "ALLOW", "FLAG", or "BLOCK"
    std::string abuse_detector_type; ///< which detector matched (e.g. "PhotoDNA", "Text")
    std::string abuse_pattern_name;  ///< name of the matched pattern/hash (empty if ALLOW)
    
    bool zip_bomb_checked = false;
    bool zip_bomb_detected = false;
    
    json toJson() const;
};

/**
 * @brief Content Security Manager
 * 
 * Central orchestration point for security checks on content operations.
 * Integrates:
 * - Malware scanning (via MalwareFilterManager)
 * - PII detection (via PIIDetector)
 * - Content abuse detection (PhotoDNA + text pattern detectors)
 * - Error sanitization
 * 
 * Usage:
 *   ContentSecurityManager security(config);
 *   security.setMalwareFilter(malware_filter);
 *   security.setPiiDetector(pii_detector);
 *   security.setPhotoAbuseDetector(photo_detector);
 *   security.setTextAbuseDetector(text_detector);
 *   security.setAuditLogger(audit_logger);  // optional; non-owning
 *   
 *   auto result = security.checkContent(data, mime_type, content_id);
 *   if (result.error.failed()) {
 *       // Handle security violation
 *   }
 */
class ContentSecurityManager {
public:
    explicit ContentSecurityManager(
        const ContentSecurityConfig& config = ContentSecurityConfig{}
    );
    
    /**
     * @brief Set malware filter
     */
    void setMalwareFilter(std::shared_ptr<security::MalwareFilterManager> filter);
    
    /**
     * @brief Set PII detector
     */
    void setPiiDetector(std::shared_ptr<utils::PIIDetector> detector);
    
    /**
     * @brief Set perceptual-hash abuse detector for image content (PhotoDNA)
     */
    void setPhotoAbuseDetector(std::shared_ptr<IAbuseDetector> detector);
    
    /**
     * @brief Set text pattern abuse detector
     */
    void setTextAbuseDetector(std::shared_ptr<IAbuseDetector> detector);
    
    /**
     * @brief Attach an audit logger for abuse detection events.
     *
     * Non-owning; the caller is responsible for keeping the logger alive.
     * Pass nullptr to detach.
     */
    void setAuditLogger(utils::AuditLogger* logger);
    
    /**
     * @brief Check content security
     * 
     * Runs configured security checks on content.
     * 
     * @param data Binary content data
     * @param mime_type MIME type
     * @param content_id Content identifier for logging
     * @param filename Optional filename
     * @return Security check result
     */
    SecurityCheckResult checkContent(
        const std::string& data,
        const std::string& mime_type,
        const std::string& content_id,
        const std::string& filename = ""
    );
    
    /**
     * @brief Check archive for zip-bomb patterns
     * 
     * Validates that the archive's decompressed/compressed size ratio does not
     * exceed max_zip_bomb_ratio (default 100×) and that the file count does not
     * exceed max_zip_file_count (default 1,000). Must be called before extraction.
     * 
     * @param compressed_size   Total compressed size of the archive in bytes
     * @param uncompressed_size Total uncompressed size reported in archive headers
     * @param file_count        Number of file entries in the archive
     * @param content_id        Content identifier for logging
     * @return SecurityCheckResult with error set if a zip-bomb pattern is detected
     */
    SecurityCheckResult checkZipBomb(
        uint64_t compressed_size,
        uint64_t uncompressed_size,
        size_t file_count,
        const std::string& content_id
    );
    
    /**
     * @brief Check text for PII
     * 
     * @param text Text to check
     * @param content_id Content identifier
     * @return Security check result (PII-specific)
     */
    SecurityCheckResult checkTextForPii(
        const std::string& text,
        const std::string& content_id
    );
    
    /**
     * @brief Sanitize error for external exposure
     * 
     * Removes internal details, paths, and system information from errors.
     * 
     * @param error Original error
     * @return Sanitized error safe for external exposure
     */
    ContentError sanitizeError(const ContentError& error) const;
    
    /**
     * @brief Sanitize error message
     * 
     * @param message Original message
     * @return Sanitized message
     */
    std::string sanitizeErrorMessage(const std::string& message) const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const ContentSecurityConfig& config);
    const ContentSecurityConfig& getConfig() const;
    
    /**
     * @brief Security metrics
     */
    struct Metrics {
        std::atomic<uint64_t> total_checks{0};
        std::atomic<uint64_t> malware_scans{0};
        std::atomic<uint64_t> malware_detected{0};
        std::atomic<uint64_t> malware_blocked{0};
        std::atomic<uint64_t> pii_scans{0};
        std::atomic<uint64_t> pii_detected{0};
        std::atomic<uint64_t> pii_blocked{0};
        std::atomic<uint64_t> abuse_scans{0};
        std::atomic<uint64_t> abuse_detected{0};
        std::atomic<uint64_t> abuse_blocked{0};
        std::atomic<uint64_t> errors_sanitized{0};
        std::atomic<uint64_t> zip_bomb_scans{0};
        std::atomic<uint64_t> zip_bomb_blocked{0};
        
        json toJson() const;
    };
    const Metrics& getMetrics() const;
    void resetMetrics();
    
private:
    ContentSecurityConfig config_;
    std::shared_ptr<security::MalwareFilterManager> malware_filter_;
    std::shared_ptr<utils::PIIDetector> pii_detector_;
    std::shared_ptr<IAbuseDetector> photo_abuse_detector_;
    std::shared_ptr<IAbuseDetector> text_abuse_detector_;
    utils::AuditLogger* audit_logger_ = nullptr;
    mutable Metrics metrics_;
    
    // Helper methods
    SecurityCheckResult checkMalware(
        const std::string& data,
        const std::string& filename,
        const std::string& mime_type,
        const std::string& content_id
    );
    
    SecurityCheckResult checkPii(
        const std::string& text,
        const std::string& content_id
    );
    
    SecurityCheckResult checkAbuse(
        const std::string& data,
        const std::string& mime_type,
        const std::string& content_id
    );
    
    std::string sanitizePath(const std::string& text) const;
    std::string sanitizeSystemInfo(const std::string& text) const;
};

} // namespace content
} // namespace themis
