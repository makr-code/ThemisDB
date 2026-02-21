/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            content_security.h                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:36:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     227                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 08942ea45  2026-02-19  Production hardening: error taxonomy, validation, observa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "content/content_errors.h"
#include "security/malware_scanner.h"
#include "utils/pii_detector.h"
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
    bool enable_abuse_detection = false;  // Stub for future implementation
    bool block_on_abuse = false;
    
    // Error sanitization
    bool sanitize_error_messages = true;
    bool hide_internal_paths = true;
    bool hide_system_info = true;
    
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
    
    json toJson() const;
};

/**
 * @brief Content Security Manager
 * 
 * Central orchestration point for security checks on content operations.
 * Integrates:
 * - Malware scanning (via MalwareFilterManager)
 * - PII detection (via PIIDetector)
 * - Content abuse detection (stub)
 * - Error sanitization
 * 
 * Usage:
 *   ContentSecurityManager security(config);
 *   security.setMalwareFilter(malware_filter);
 *   security.setPiiDetector(pii_detector);
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
        std::atomic<uint64_t> errors_sanitized{0};
        
        json toJson() const;
    };
    const Metrics& getMetrics() const;
    void resetMetrics();
    
private:
    ContentSecurityConfig config_;
    std::shared_ptr<security::MalwareFilterManager> malware_filter_;
    std::shared_ptr<utils::PIIDetector> pii_detector_;
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
        const std::string& text,
        const std::string& content_id
    );
    
    std::string sanitizePath(const std::string& text) const;
    std::string sanitizeSystemInfo(const std::string& text) const;
};

} // namespace content
} // namespace themis
