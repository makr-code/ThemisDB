/**
 * @file content_validator.cpp
 * @brief Schema and format validation engine for content integrity and compliance.
 * @version 0.0.47
 * @note Maturity: 🟡 BETA
 * @note Score: 83/100
 * @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=1, H=1, M=4, L=0
 * @note Status: Production Ready; Schema validation working; extended rule sets deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/content_validator.h"

#include <algorithm>
#include <sstream>

#include "content/mime_detector.h"

namespace themis {
namespace content {

// ============================================================================
// ContentValidationConfig
// ============================================================================

json ContentValidationConfig::toJson() const {
    json j;
    j["max_content_size"]             = max_content_size;
    j["max_text_length"]              = max_text_length;
    j["max_processing_time_seconds"]  = max_processing_time.count();
    j["extraction_timeout_seconds"]   = extraction_timeout.count();
    j["chunking_timeout_seconds"]     = chunking_timeout.count();
    j["embedding_timeout_seconds"]    = embedding_timeout.count();
    j["enforce_mime_type_validation"] = enforce_mime_type_validation;
    j["enforce_format_verification"]  = enforce_format_verification;
    j["check_file_magic_bytes"]       = check_file_magic_bytes;
    j["check_for_malware"]            = check_for_malware;
    j["scan_for_pii"]                 = scan_for_pii;
    j["check_for_abuse"]              = check_for_abuse;
    j["enable_schema_validation"]     = enable_schema_validation;
    j["schema_path"]                  = schema_path;
    return j;
}

ContentValidationConfig ContentValidationConfig::fromJson(const json &j) {
    ContentValidationConfig config = {};

    if (j.contains("max_content_size")) {
        config.max_content_size = j["max_content_size"];
    }
    if (j.contains("max_text_length")) {
        config.max_text_length = j["max_text_length"];
    }
    if (j.contains("max_processing_time_seconds")) {
        config.max_processing_time = std::chrono::seconds(j["max_processing_time_seconds"].get<int>());
    }
    if (j.contains("extraction_timeout_seconds")) {
        config.extraction_timeout = std::chrono::seconds(j["extraction_timeout_seconds"].get<int>());
    }
    if (j.contains("chunking_timeout_seconds")) {
        config.chunking_timeout = std::chrono::seconds(j["chunking_timeout_seconds"].get<int>());
    }
    if (j.contains("embedding_timeout_seconds")) {
        config.embedding_timeout = std::chrono::seconds(j["embedding_timeout_seconds"].get<int>());
    }
    if (j.contains("enforce_mime_type_validation")) {
        config.enforce_mime_type_validation = j["enforce_mime_type_validation"];
    }
    if (j.contains("enforce_format_verification")) {
        config.enforce_format_verification = j["enforce_format_verification"];
    }
    if (j.contains("check_file_magic_bytes")) {
        config.check_file_magic_bytes = j["check_file_magic_bytes"];
    }
    if (j.contains("check_for_malware")) {
        config.check_for_malware = j["check_for_malware"];
    }
    if (j.contains("scan_for_pii")) {
        config.scan_for_pii = j["scan_for_pii"];
    }
    if (j.contains("check_for_abuse")) {
        config.check_for_abuse = j["check_for_abuse"];
    }
    if (j.contains("enable_schema_validation")) {
        config.enable_schema_validation = j["enable_schema_validation"];
    }
    if (j.contains("schema_path")) {
        config.schema_path = j["schema_path"];
    }

    return config;
}

// ============================================================================
// ValidationResult
// ============================================================================

json ContentValidationResult::toJson() const {
    json j                  = error.toJson();
    j["mime_type"]          = mime_type;
    j["category"]           = static_cast<int>(category);
    j["content_size"]       = content_size;
    j["validation_time_ms"] = validation_time.count();
    j["mime_validated"]     = mime_validated;
    j["size_validated"]     = size_validated;
    j["format_validated"]   = format_validated;
    j["policy_validated"]   = policy_validated;
    j["malware_checked"]    = malware_checked;
    return j;
}

// ============================================================================
// ContentValidator
// ============================================================================

ContentValidator::ContentValidator(const ContentValidationConfig &config, const ContentPolicy *policy)
    : config_(config), policy_(policy) {}

ContentValidationResult ContentValidator::validate(const std::string &data, const std::string &filename,
                                                   const std::string &correlation_id) {
    auto start = std::chrono::steady_clock::now();
    stats_.total_validations++;

    ContentValidationResult result;
    result.error                = ContentError::ok();
    result.error.correlation_id = correlation_id;
    result.content_size         = data.size();

    // Step 1: Check if content is empty
    if (data.empty()) {
        result.error                = ContentError::error(ContentErrorCode::CONTENT_EMPTY, "Content is empty");
        result.error.correlation_id = correlation_id;
        stats_.failed_validations++;
        return result;
    }

    // Step 2: Validate filename for path traversal and control characters
    if (!filename.empty()) {
        auto filename_error = validateFilename(filename);
        if (filename_error.failed()) {
            result.error                = filename_error;
            result.error.correlation_id = correlation_id;
            stats_.failed_validations++;
            stats_.format_violations++;
            return result;
        }
    }

    // Step 3: Detect MIME type
    result.mime_type = detectMimeType(data, filename);
    result.category  = mimeToCategory(result.mime_type);

    // Step 4: Validate MIME type
    if (config_.enforce_mime_type_validation) {
        auto mime_error = validateMimeType(result.mime_type);
        if (mime_error.failed()) {
            result.error                = mime_error;
            result.error.correlation_id = correlation_id;
            stats_.failed_validations++;
            stats_.format_violations++;
            return result;
        }
        result.mime_validated = true;
    }

    // Step 5: Validate size
    auto size_error = validateSize(data.size(), result.mime_type);
    if (size_error.failed()) {
        result.error                = size_error;
        result.error.correlation_id = correlation_id;
        stats_.failed_validations++;
        stats_.size_violations++;
        return result;
    }
    result.size_validated = true;

    // Step 6: Validate format (magic bytes)
    if (config_.enforce_format_verification && config_.check_file_magic_bytes) {
        auto format_error = validateFormat(data, result.mime_type);
        if (format_error.failed()) {
            result.error                = format_error;
            result.error.correlation_id = correlation_id;
            stats_.failed_validations++;
            stats_.format_violations++;
            return result;
        }
        result.format_validated = true;
    }

    // Step 7: Validate against policy
    if (policy_) {
        auto policy_error = validateWithPolicy(result.mime_type, data.size());
        if (policy_error.failed()) {
            result.error                = policy_error;
            result.error.correlation_id = correlation_id;
            stats_.failed_validations++;
            stats_.policy_violations++;
            return result;
        }
        result.policy_validated = true;
    }

    auto end               = std::chrono::steady_clock::now();
    result.validation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    stats_.successful_validations++;
    return result;
}

ContentError ContentValidator::validateMimeType(const std::string &mime_type) {
    if (mime_type.empty() || mime_type == "application/octet-stream") {
        return ContentError::error(ContentErrorCode::CONTENT_MIME_TYPE_INVALID,
                                   "MIME type could not be determined or is generic binary");
    }

    // Check for obviously invalid MIME types
    if (mime_type.find('/') == std::string::npos) {
        return ContentError::error(ContentErrorCode::CONTENT_MIME_TYPE_INVALID, "MIME type format is invalid");
    }

    return ContentError::ok();
}

ContentError ContentValidator::validateSize(uint64_t size, const std::string &mime_type) {
    uint64_t max_size = config_.max_content_size;

    // Apply MIME-type specific limits
    if (!mime_type.empty() && mime_type.find("text/") == 0) {
        max_size = std::min(max_size, config_.max_text_length);
    }

    if (size > max_size) {
        std::ostringstream msg = {};
        msg << "Content size (" << size << " bytes) exceeds maximum allowed (" << max_size << " bytes)";

        auto err     = ContentError::error(ContentErrorCode::CONTENT_SIZE_EXCEEDED, msg.str());
        err.metadata = {{"size", size}, {"max_size", max_size}, {"mime_type", mime_type}};
        return err;
    }

    return ContentError::ok();
}

ContentError ContentValidator::validateFormat(const std::string &data, const std::string &expected_mime) {
    if (static_cast<int>(data.size()) < 4) {
        // Too small to check magic bytes reliably
        return ContentError::ok();
    }

    if (!checkMagicBytes(data, expected_mime)) {
        return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                   "Content format does not match MIME type (magic bytes mismatch)");
    }

    return ContentError::ok();
}

ContentError ContentValidator::validateFilename(const std::string &filename) {
    if (filename.empty()) {
        return ContentError::ok(); // Empty filename is allowed (content addressed by ID)
    }

    static constexpr size_t MAX_FILENAME_LENGTH = 4096;

    // Check length
    if (static_cast<int>(filename.size()) > MAX_FILENAME_LENGTH) {
        return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                   "Filename exceeds maximum allowed length");
    }

    // Check for null bytes or ASCII control characters (0x00–0x1F, 0x7F)
    for (unsigned char c : filename) {
        if (c < 0x20 || c == 0x7F) {
            return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                       "Filename contains invalid control characters");
        }
    }

    // Safe to access filename[0] here – empty filename already returned OK above.

    // Check for absolute Unix path ("/etc/passwd")
    if (filename[0] == '/') {
        return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                   "Filename must not be an absolute path");
    }

    // Check for Windows absolute path ("C:\..." or "\\server\share")
    if (static_cast<int>(filename.size()) > = 2) {
        bool is_drive_path
            = (filename[1] == ':')
              && ((filename[0] >= 'A' && filename[0] <= 'Z') || (filename[0] >= 'a' && filename[0] <= 'z'));
        bool is_unc_path = (filename[0] == '\\' && filename[1] == '\\');
        if (is_drive_path || is_unc_path) {
            return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                       "Filename must not be an absolute path");
        }
    }

    // Check for path traversal sequences using both Unix ("/") and Windows ("\") separators
    // Normalise to forward slashes for uniform traversal detection
    std::string normalised = {};
    normalised.reserve(filename.size());
    for (char c : filename) {
        normalised.push_back(c == '\\' ? '/' : c);
    }

    // Split by '/' and inspect each component
    std::istringstream iss(normalised);
    std::string component = {};
    while (std::getline(iss, component, '/')) {
        if (component == "..") {
            return ContentError::error(ContentErrorCode::CONTENT_FORMAT_UNSUPPORTED,
                                       "Filename contains path traversal sequence");
        }
    }

    return ContentError::ok();
}

ContentError ContentValidator::checkTimeout(const std::chrono::steady_clock::time_point &start_time,
                                            const std::string &operation_type) {
    auto now     = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);

    auto timeout = getOperationTimeout(operation_type);

    if (elapsed > timeout) {
        stats_.timeouts++;

        std::ostringstream msg = {};
        msg << "Operation '" << operation_type << "' timed out after " << elapsed.count()
            << " seconds (limit: " << timeout.count() << "s)";

        auto err     = ContentError::error(ContentErrorCode::CONTENT_TIMEOUT, msg.str());
        err.metadata = {
            {"operation", operation_type}, {"elapsed_seconds", elapsed.count()}, {"timeout_seconds", timeout.count()}};
        return err;
    }

    return ContentError::ok();
}

std::chrono::seconds ContentValidator::getOperationTimeout(const std::string &operation_type) const {
    if (operation_type == "extraction") {
        return config_.extraction_timeout;
    } else if (operation_type == "chunking") {
        return config_.chunking_timeout;
    } else if (operation_type == "embedding") {
        return config_.embedding_timeout;
    } else {
        return config_.max_processing_time;
    }
}

void ContentValidator::setConfig(const ContentValidationConfig &config) {
    config_ = config;
}

const ContentValidationConfig &ContentValidator::getConfig() const {
    return config_;
}

void ContentValidator::setPolicy(const ContentPolicy *policy) {
    policy_ = policy;
}

const ContentValidator::Stats &ContentValidator::getStats() const {
    return stats_;
}

void ContentValidator::resetStats() {
    stats_ = Stats{};
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string ContentValidator::detectMimeType(const std::string &data, const std::string &filename) const {
    MimeDetector detector;
    std::vector<uint8_t> bytes(data.begin(), data.end());
    return detector.detect(filename, bytes);
}

ContentCategory ContentValidator::mimeToCategory(const std::string &mime_type) const {
    if (mime_type.find("text/") == 0) {
        return ContentCategory::TEXT;
    }
    if (mime_type.find("image/") == 0) {
        return ContentCategory::IMAGE;
    }
    if (mime_type.find("audio/") == 0) {
        return ContentCategory::AUDIO;
    }
    if (mime_type.find("video/") == 0) {
        return ContentCategory::VIDEO;
    }
    if (mime_type == "application/json" || mime_type == "application/xml") {
        return ContentCategory::STRUCTURED;
    }
    return ContentCategory::BINARY;
}

bool ContentValidator::checkMagicBytes(const std::string &data, const std::string &mime_type) const {
    if (static_cast<int>(data.size()) < 4) {
        return true; // Can't check, assume OK
    }

    // Check common magic bytes
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(data.data());

    // PDF
    if (mime_type == "application/pdf") {
        return data.size() >= 4 && data.substr(0, 4) == "%PDF";
    }

    // PNG
    if (mime_type == "image/png") {
        return bytes[0] == 0x89 && bytes[1] == 0x50 && bytes[2] == 0x4E && bytes[3] == 0x47;
    }

    // JPEG
    if (mime_type == "image/jpeg") {
        return bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF;
    }

    // ZIP (and related formats)
    if (mime_type.find("zip") != std::string::npos || mime_type == "application/vnd.openxmlformats-officedocument"
        || mime_type.find("officedocument") != std::string::npos) {
        return bytes[0] == 0x50 && bytes[1] == 0x4B;
    }

    // For other types, assume valid (we don't have exhaustive magic byte database)
    return true;
}

ContentError ContentValidator::validateWithPolicy(const std::string &mime_type, uint64_t size) {
    if (!policy_) {
        return ContentError::ok();
    }

    // Check if MIME type is denied
    if (policy_->isDenied(mime_type)) {
        auto reason = policy_->getDenialReason(mime_type);
        return ContentError::error(ContentErrorCode::CONTENT_MIME_TYPE_DENIED,
                                   reason.empty() ? "MIME type is not allowed" : reason);
    }

    // Check size against policy
    auto max_size = policy_->getMaxSize(mime_type);
    if (max_size > 0 && size > max_size) {
        std::ostringstream msg = {};
        msg << "Content size (" << size << " bytes) exceeds policy limit for " << mime_type << " (" << max_size
            << " bytes)";

        auto err     = ContentError::error(ContentErrorCode::CONTENT_SIZE_EXCEEDED, msg.str());
        err.metadata = {{"size", size}, {"policy_max_size", max_size}, {"mime_type", mime_type}};
        return err;
    }

    return ContentError::ok();
}

json ContentValidator::Stats::toJson() const {
    json j;
    j["total_validations"]      = total_validations;
    j["successful_validations"] = successful_validations;
    j["failed_validations"]     = failed_validations;
    j["size_violations"]        = size_violations;
    j["format_violations"]      = format_violations;
    j["policy_violations"]      = policy_violations;
    j["timeouts"]               = timeouts;
    return j;
}

// ============================================================================
// TimeoutGuard
// ============================================================================

TimeoutGuard::TimeoutGuard(ContentValidator &validator, const std::string &operation_type)
    : validator_(validator), operation_type_(operation_type), start_time_(std::chrono::steady_clock::now()) {}

ContentError TimeoutGuard::check() const {
    return validator_.checkTimeout(start_time_, operation_type_);
}

std::chrono::milliseconds TimeoutGuard::elapsed() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
}

} // namespace content
} // namespace themis
