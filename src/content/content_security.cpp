/**
 * @file content_security.cpp
 * @brief Content security framework for threat detection, sanitization, and PII handling.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=5; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=4, L=0
 * @note Status: Production Ready; PII detection + sanitization complete; advanced rules deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_security.h"

#include <iomanip>
#include <openssl/sha.h>
#include <regex>
#include <sstream>
#include <iomanip>
#include <unordered_set>

// ============================================================================
// Helpers (file-local)
// ============================================================================

namespace {

/// Compute a short SHA-256 hex digest of @p data (first 16 hex chars = 8 bytes).
std::string contentHash(const std::string &data) {
    unsigned char digest[SHA256_DIGEST_LENGTH] = {};
    SHA256(reinterpret_cast<const unsigned char *>(data.data()), data.size(), digest);
    std::ostringstream oss = {};
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

namespace themis {
namespace content {

// ============================================================================
// ContentSecurityConfig
// ============================================================================

json ContentSecurityConfig::toJson() const {
    json j;
    j["enable_malware_scan"]     = enable_malware_scan;
    j["block_on_malware"]        = block_on_malware;
    j["malware_block_threshold"] = static_cast<int>(malware_block_threshold);
    j["enable_pii_detection"]    = enable_pii_detection;
    j["block_on_pii"]            = block_on_pii;
    j["redact_pii_in_logs"]      = redact_pii_in_logs;
    j["enable_abuse_detection"]  = enable_abuse_detection;
    j["block_on_abuse"]          = block_on_abuse;
    j["sanitize_error_messages"] = sanitize_error_messages;
    j["hide_internal_paths"]     = hide_internal_paths;
    j["hide_system_info"]        = hide_system_info;
    j["enable_zip_bomb_check"]   = enable_zip_bomb_check;
    j["max_zip_bomb_ratio"]      = max_zip_bomb_ratio;
    j["max_zip_file_count"]      = max_zip_file_count;
    return j;
}

ContentSecurityConfig ContentSecurityConfig::fromJson(const json &j) {
    ContentSecurityConfig config = {};

    if (j.contains("enable_malware_scan")) {
        config.enable_malware_scan = j["enable_malware_scan"];
    }
    if (j.contains("block_on_malware")) {
        config.block_on_malware = j["block_on_malware"];
    }
    if (j.contains("malware_block_threshold")) {
        config.malware_block_threshold = static_cast<security::ThreatLevel>(j["malware_block_threshold"].get<int>());
    }
    if (j.contains("enable_pii_detection")) {
        config.enable_pii_detection = j["enable_pii_detection"];
    }
    if (j.contains("block_on_pii")) {
        config.block_on_pii = j["block_on_pii"];
    }
    if (j.contains("redact_pii_in_logs")) {
        config.redact_pii_in_logs = j["redact_pii_in_logs"];
    }
    if (j.contains("enable_abuse_detection")) {
        config.enable_abuse_detection = j["enable_abuse_detection"];
    }
    if (j.contains("block_on_abuse")) {
        config.block_on_abuse = j["block_on_abuse"];
    }
    if (j.contains("sanitize_error_messages")) {
        config.sanitize_error_messages = j["sanitize_error_messages"];
    }
    if (j.contains("hide_internal_paths")) {
        config.hide_internal_paths = j["hide_internal_paths"];
    }
    if (j.contains("hide_system_info")) {
        config.hide_system_info = j["hide_system_info"];
    }
    if (j.contains("enable_zip_bomb_check")) {
        config.enable_zip_bomb_check = j["enable_zip_bomb_check"];
    }
    if (j.contains("max_zip_bomb_ratio")) {
        config.max_zip_bomb_ratio = j["max_zip_bomb_ratio"].get<uint64_t>();
    }
    if (j.contains("max_zip_file_count")) {
        config.max_zip_file_count = j["max_zip_file_count"].get<size_t>();
    }

    return config;
}

// ============================================================================
// SecurityCheckResult
// ============================================================================

json SecurityCheckResult::toJson() const {
    json j;
    j["error"]               = error.toJson();
    j["malware_checked"]     = malware_checked;
    j["malware_clean"]       = malware_clean;
    j["malware_threat"]      = malware_threat;
    j["pii_checked"]         = pii_checked;
    j["pii_found"]           = pii_found;
    j["pii_types"]           = pii_types;
    j["abuse_checked"]       = abuse_checked;
    j["abuse_detected"]      = abuse_detected;
    j["abuse_action"]        = abuse_action;
    j["abuse_detector_type"] = abuse_detector_type;
    j["abuse_pattern_name"]  = abuse_pattern_name;
    j["zip_bomb_checked"]    = zip_bomb_checked;
    j["zip_bomb_detected"]   = zip_bomb_detected;
    return j;
}

// ============================================================================
// ContentSecurityManager
// ============================================================================

ContentSecurityManager::ContentSecurityManager(const ContentSecurityConfig &config) : config_(config) {}

void ContentSecurityManager::setMalwareFilter(std::shared_ptr<security::MalwareFilterManager> filter) {
    malware_filter_ = filter;
}

void ContentSecurityManager::setPiiDetector(std::shared_ptr<utils::PIIDetector> detector) {
    pii_detector_ = detector;
}

void ContentSecurityManager::setPhotoAbuseDetector(std::shared_ptr<IAbuseDetector> detector) {
    photo_abuse_detector_ = detector;
}

void ContentSecurityManager::setTextAbuseDetector(std::shared_ptr<IAbuseDetector> detector) {
    text_abuse_detector_ = detector;
}

void ContentSecurityManager::setAuditLogger(utils::AuditLogger *logger) {
    audit_logger_ = logger;
}

SecurityCheckResult ContentSecurityManager::checkContent(const std::string &data, const std::string &mime_type,
                                                         const std::string &content_id, const std::string &filename) {
    metrics_.total_checks++;

    SecurityCheckResult result;
    result.error = ContentError::ok();

    // Check 1: Malware scanning
    if (config_.enable_malware_scan) {
        auto malware_result    = checkMalware(data, filename, mime_type, content_id);
        result.malware_checked = malware_result.malware_checked;
        result.malware_clean   = malware_result.malware_clean;
        result.malware_threat  = malware_result.malware_threat;

        if (malware_result.error.failed()) {
            result.error = malware_result.error;
            return result;
        }
    }

    // Check 2: PII detection (for text-based content)
    if (config_.enable_pii_detection && (mime_type.find("text/") == 0 || mime_type == "application/json")) {
        auto pii_result    = checkPii(data, content_id);
        result.pii_checked = pii_result.pii_checked;
        result.pii_found   = pii_result.pii_found;
        result.pii_types   = pii_result.pii_types;

        if (pii_result.error.failed()) {
            result.error = pii_result.error;
            return result;
        }
    }

    // Check 3: Abuse detection
    if (config_.enable_abuse_detection) {
        auto abuse_result          = checkAbuse(data, mime_type, content_id);
        result.abuse_checked       = abuse_result.abuse_checked;
        result.abuse_detected      = abuse_result.abuse_detected;
        result.abuse_action        = abuse_result.abuse_action;
        result.abuse_detector_type = abuse_result.abuse_detector_type;
        result.abuse_pattern_name  = abuse_result.abuse_pattern_name;

        if (abuse_result.error.failed()) {
            result.error = abuse_result.error;
            return result;
        }
    }

    return result;
}

SecurityCheckResult ContentSecurityManager::checkZipBomb(uint64_t compressed_size, uint64_t uncompressed_size,
                                                         size_t file_count, const std::string &content_id) {
    SecurityCheckResult result;
    result.error = ContentError::ok();

    if (!config_.enable_zip_bomb_check) {
        // Check explicitly disabled; zip_bomb_checked remains false to signal skip
        result.zip_bomb_checked = false;
        return result;
    }

    metrics_.zip_bomb_scans++;

    // Mark the check as having been performed
    result.zip_bomb_checked = true;

    // Check compression ratio: uncompressed / compressed must not exceed the threshold
    if (compressed_size > 0) {
        uint64_t ratio = uncompressed_size / compressed_size;
        if (ratio > config_.max_zip_bomb_ratio) {
            metrics_.zip_bomb_blocked++;
            result.zip_bomb_detected = true;
            result.error = ContentError::error(ContentErrorCode::CONTENT_MALWARE_DETECTED,
                                               "Archive rejected: compression ratio exceeds limit (possible zip bomb)");
            result.error.content_id = content_id;
            result.error.metadata   = {{"ratio", ratio},
                                       {"max_ratio", config_.max_zip_bomb_ratio},
                                       {"compressed_size", compressed_size},
                                       {"uncompressed_size", uncompressed_size}};
            return result;
        }
    }

    // Check file count
    if (file_count > config_.max_zip_file_count) {
        metrics_.zip_bomb_blocked++;
        result.zip_bomb_detected = true;
        result.error            = ContentError::error(ContentErrorCode::CONTENT_SIZE_EXCEEDED,
                                                      "Archive rejected: file count exceeds limit (possible zip bomb)");
        result.error.content_id = content_id;
        result.error.metadata   = {{"file_count", file_count}, {"max_file_count", config_.max_zip_file_count}};
        return result;
    }

    return result;
}

SecurityCheckResult ContentSecurityManager::checkTextForPii(const std::string &text, const std::string &content_id) {
    SecurityCheckResult result;
    result.error = ContentError::ok();

    if (config_.enable_pii_detection) {
        return checkPii(text, content_id);
    }

    return result;
}

ContentError ContentSecurityManager::sanitizeError(const ContentError &error) const {
    if (!config_.sanitize_error_messages) {
        return error;
    }

    metrics_.errors_sanitized++;

    ContentError sanitized = error;
    sanitized.message      = sanitizeErrorMessage(error.message);
    sanitized.details      = ""; // Always remove internal details for external exposure

    // Remove sensitive metadata
    if (!sanitized.metadata.is_null()) {
        json safe_metadata;
        // Only keep non-sensitive fields
        if (sanitized.metadata.contains("size")) {
            safe_metadata["size"] = sanitized.metadata["size"];
        }
        if (sanitized.metadata.contains("mime_type")) {
            safe_metadata["mime_type"] = sanitized.metadata["mime_type"];
        }
        sanitized.metadata = safe_metadata;
    }

    return sanitized;
}

std::string ContentSecurityManager::sanitizeErrorMessage(const std::string &message) const {
    std::string sanitized = message;

    if (config_.hide_internal_paths) {
        sanitized = sanitizePath(sanitized);
    }

    if (config_.hide_system_info) {
        sanitized = sanitizeSystemInfo(sanitized);
    }

    return sanitized;
}

void ContentSecurityManager::setConfig(const ContentSecurityConfig &config) {
    config_ = config;
}

const ContentSecurityConfig &ContentSecurityManager::getConfig() const {
    return config_;
}

const ContentSecurityManager::Metrics &ContentSecurityManager::getMetrics() const {
    return metrics_;
}

void ContentSecurityManager::resetMetrics() {
    metrics_.total_checks.store(0, std::memory_order_relaxed);
    metrics_.malware_scans.store(0, std::memory_order_relaxed);
    metrics_.malware_detected.store(0, std::memory_order_relaxed);
    metrics_.malware_blocked.store(0, std::memory_order_relaxed);
    metrics_.pii_scans.store(0, std::memory_order_relaxed);
    metrics_.pii_detected.store(0, std::memory_order_relaxed);
    metrics_.pii_blocked.store(0, std::memory_order_relaxed);
    metrics_.abuse_scans.store(0, std::memory_order_relaxed);
    metrics_.abuse_detected.store(0, std::memory_order_relaxed);
    metrics_.abuse_blocked.store(0, std::memory_order_relaxed);
    metrics_.errors_sanitized.store(0, std::memory_order_relaxed);
    metrics_.zip_bomb_scans.store(0, std::memory_order_relaxed);
    metrics_.zip_bomb_blocked.store(0, std::memory_order_relaxed);
}

// ============================================================================
// Private Helper Methods
// ============================================================================

SecurityCheckResult ContentSecurityManager::checkMalware(const std::string &data, const std::string &filename,
                                                         const std::string &mime_type, const std::string &content_id) {
    SecurityCheckResult result;
    result.error           = ContentError::ok();
    result.malware_checked = true;

    metrics_.malware_scans++;

    if (!malware_filter_) {
        // No malware filter configured, skip check
        result.malware_clean = true;
        return result;
    }

    auto scan_result = malware_filter_->scan(data, filename, mime_type, content_id);

    if (!scan_result.clean) {
        metrics_.malware_detected++;
        result.malware_clean = false;
        result.malware_threat
            = scan_result.scanner_results.empty() ? "Unknown threat" : scan_result.scanner_results[0].threat_name;

        if (config_.block_on_malware && scan_result.highest_threat >= config_.malware_block_threshold) {
            metrics_.malware_blocked++;

            result.error
                = ContentError::error(ContentErrorCode::CONTENT_MALWARE_DETECTED, "Malware detected in content");
            result.error.content_id = content_id;
            result.error.metadata   = {{"threat_level", security::threatLevelToString(scan_result.highest_threat)},
                                       {"scanner_count", scan_result.scanners_used}};
        }
    } else {
        result.malware_clean = true;
    }

    return result;
}

SecurityCheckResult ContentSecurityManager::checkPii(const std::string &text, const std::string &content_id) {
    SecurityCheckResult result;
    result.error       = ContentError::ok();
    result.pii_checked = true;

    metrics_.pii_scans++;

    if (!pii_detector_) {
        // No PII detector configured, skip check
        result.pii_found = false;
        return result;
    }

    auto findings = pii_detector_->detectInText(text);

    if (!findings.empty()) {
        metrics_.pii_detected++;
        result.pii_found = true;
        
        // Collect unique PII types — use unordered_set for O(1) dedup instead
        // of std::find on the growing pii_types vector (avoids O(n²) behaviour
        // when many findings share the same PII type).
        std::unordered_set<std::string> seen_types = {};

        for (const auto& finding : findings) {
            std::string type_str = utils::PIITypeUtils::toString(finding.type);
            if (seen_types.insert(type_str).second) {
                result.pii_types.push_back(type_str);
            }
        }

        if (config_.block_on_pii) {
            metrics_.pii_blocked++;

            result.error            = ContentError::error(ContentErrorCode::CONTENT_PII_DETECTED,
                                                          "Personally identifiable information detected in content");
            result.error.content_id = content_id;
            result.error.metadata   = {{"pii_count", findings.size()}, {"pii_types", result.pii_types}};
        }
    } else {
        result.pii_found = false;
    }

    return result;
}

SecurityCheckResult ContentSecurityManager::checkAbuse(const std::string &data, const std::string &mime_type,
                                                       const std::string &content_id) {
    SecurityCheckResult result;
    result.error          = ContentError::ok();
    result.abuse_checked  = true;
    result.abuse_detected = false;
    result.abuse_action   = abuseActionToString(AbuseAction::ALLOW);

    metrics_.abuse_scans++;

    // Build metadata once; shared across all detectors.
    AbuseDetectorMetadata meta;
    meta.content_id   = content_id;
    meta.mime_type    = mime_type;
    meta.content_hash = contentHash(data);

    // Run the photo-DNA detector first (images only).
    if (photo_abuse_detector_) {
        auto det_result = photo_abuse_detector_->detect(data, meta);
        if (det_result.action != AbuseAction::ALLOW) {
            metrics_.abuse_detected++;
            result.abuse_detected      = true;
            result.abuse_action        = abuseActionToString(det_result.action);
            result.abuse_detector_type = det_result.detector_type;
            result.abuse_pattern_name  = det_result.pattern_name;

            // Audit-log the detection event.
            if (audit_logger_) {
                audit_logger_->logEvent({{"event", "abuse_detection"},
                                         {"content_hash", meta.content_hash},
                                         {"content_id", content_id},
                                         {"detector_type", det_result.detector_type},
                                         {"pattern_name", det_result.pattern_name},
                                         {"action", result.abuse_action},
                                         {"reason", det_result.reason}});
            }

            if (det_result.action == AbuseAction::BLOCK && config_.block_on_abuse) {
                metrics_.abuse_blocked++;
                result.error = ContentError::error(ContentErrorCode::CONTENT_ABUSE_DETECTED,
                                                   "Content blocked by abuse detector (" + det_result.detector_type
                                                       + "): " + det_result.reason);
                result.error.content_id = content_id;
                result.error.metadata   = {{"detector_type", det_result.detector_type},
                                           {"pattern_name", det_result.pattern_name},
                                           {"action", result.abuse_action}};
                return result;
            }

            // FLAG: continue processing; the caller stores the content with the flag.
            return result;
        }
    }

    // Run the text pattern detector for all content types.
    if (text_abuse_detector_) {
        auto det_result = text_abuse_detector_->detect(data, meta);
        if (det_result.action != AbuseAction::ALLOW) {
            metrics_.abuse_detected++;
            result.abuse_detected      = true;
            result.abuse_action        = abuseActionToString(det_result.action);
            result.abuse_detector_type = det_result.detector_type;
            result.abuse_pattern_name  = det_result.pattern_name;

            // Audit-log the detection event.
            if (audit_logger_) {
                audit_logger_->logEvent({{"event", "abuse_detection"},
                                         {"content_hash", meta.content_hash},
                                         {"content_id", content_id},
                                         {"detector_type", det_result.detector_type},
                                         {"pattern_name", det_result.pattern_name},
                                         {"action", result.abuse_action},
                                         {"reason", det_result.reason}});
            }

            if (det_result.action == AbuseAction::BLOCK && config_.block_on_abuse) {
                metrics_.abuse_blocked++;
                result.error = ContentError::error(ContentErrorCode::CONTENT_ABUSE_DETECTED,
                                                   "Content blocked by abuse detector (" + det_result.detector_type
                                                       + "): " + det_result.reason);
                result.error.content_id = content_id;
                result.error.metadata   = {{"detector_type", det_result.detector_type},
                                           {"pattern_name", det_result.pattern_name},
                                           {"action", result.abuse_action}};
                return result;
            }

            // FLAG: continue processing.
            return result;
        }
    }

    // No detector matched; content is clean.
    return result;
}

std::string ContentSecurityManager::sanitizePath(const std::string &text) const {
    // Static regex patterns for path sanitization
    // More specific patterns to avoid false positives
    // Require at least two slash-separated segments and a non-numeric first
    // segment to avoid false positives like dates (2024/01/15), fractions
    // (3/4), or MIME subtypes (application/json).
    static const std::regex unix_path_regex(R"((/(?:[a-zA-Z_][a-zA-Z0-9_\-]*)(?:/[a-zA-Z0-9_\-.]+)+))");
    static const std::regex windows_path_regex(R"([A-Z]:\\([a-zA-Z0-9_\-]+\\)+[a-zA-Z0-9_\-./]*)");
    static const std::regex home_path_regex(R"(~/[a-zA-Z0-9_\-./]+)");

    std::string sanitized = text;

    // Replace Unix paths (requires at least two path segments)
    sanitized = std::regex_replace(sanitized, unix_path_regex, "[PATH]");

    // Replace Windows paths
    sanitized = std::regex_replace(sanitized, windows_path_regex, "[PATH]");

    // Replace home directory paths
    sanitized = std::regex_replace(sanitized, home_path_regex, "[PATH]");

    return sanitized;
}

std::string ContentSecurityManager::sanitizeSystemInfo(const std::string &text) const {
    // Static regex patterns for system info sanitization
    // More specific hostname pattern - requires subdomain and common TLD
    static const std::regex hostname_regex(
        R"(\b(?:[a-zA-Z0-9](?:[a-zA-Z0-9\-]{0,61}[a-zA-Z0-9])?\.){2,}[a-zA-Z]{2,}\b)");
    static const std::regex username_regex(R"(\buser:\s*[a-zA-Z0-9_\-]+)");
    static const std::regex version_regex(R"(\bversion\s*[0-9]+\.[0-9]+\.[0-9]+)");

    std::string sanitized = text;

    // Replace hostnames with better context checking
    // Only sanitize if it looks like a hostname in context (after common prepositions)
    if (sanitized.find("://") != std::string::npos || sanitized.find(" from ") != std::string::npos
        || sanitized.find(" to ") != std::string::npos || sanitized.find(" at ") != std::string::npos
        || sanitized.find("connect") != std::string::npos) {
        sanitized = std::regex_replace(sanitized, hostname_regex, "[HOSTNAME]");
    }

    // Replace username references
    sanitized = std::regex_replace(sanitized, username_regex, "user: [USERNAME]");

    // Keep version info as it might be useful for debugging
    // sanitized = std::regex_replace(sanitized, version_regex, "version [VERSION]");

    return sanitized;
}

// ============================================================================
// Metrics
// ============================================================================

json ContentSecurityManager::Metrics::toJson() const {
    json j;
    j["total_checks"]     = total_checks.load();
    j["malware_scans"]    = malware_scans.load();
    j["malware_detected"] = malware_detected.load();
    j["malware_blocked"]  = malware_blocked.load();
    j["pii_scans"]        = pii_scans.load();
    j["pii_detected"]     = pii_detected.load();
    j["pii_blocked"]      = pii_blocked.load();
    j["abuse_scans"]      = abuse_scans.load();
    j["abuse_detected"]   = abuse_detected.load();
    j["abuse_blocked"]    = abuse_blocked.load();
    j["errors_sanitized"] = errors_sanitized.load();
    j["zip_bomb_scans"]   = zip_bomb_scans.load();
    j["zip_bomb_blocked"] = zip_bomb_blocked.load();
    return j;
}

} // namespace content
} // namespace themis
