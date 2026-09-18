/**
 * @file abuse_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <regex>
#include <cstdint>

namespace themis {
namespace content {

enum class AbuseAction {
    ALLOW,  ///< Content is clean; no action required
    FLAG,   ///< Content is suspicious; store with a flag for review
    BLOCK   ///< Content is clearly abusive; reject/block it
};

/**
 * @brief Abuse Action To String.
 * @param[in] action Input parameter.
 * @return Return value.
 * @details Implements abuseActionToString without additional internal calls.
 */
inline std::string abuseActionToString(AbuseAction action) {
    switch (action) {
        case AbuseAction::ALLOW: return "ALLOW";
        case AbuseAction::FLAG:  return "FLAG";
        case AbuseAction::BLOCK: return "BLOCK";
        default:                 return "UNKNOWN";
    }
}

struct AbuseDetectionResult {
    AbuseAction action = AbuseAction::ALLOW; ///< Action to take
    std::string detector_type;              ///< e.g. "PhotoDNA", "Text"
    std::string pattern_name;               ///< Which pattern/hash matched (empty if ALLOW)
    std::string reason;                     ///< Human-readable reason (empty if ALLOW)
};

struct AbuseDetectorMetadata {
    std::string content_id;    ///< Unique content identifier
    std::string mime_type;     ///< MIME type of the content
    std::string content_hash;  ///< SHA-256 hex digest of the raw bytes
};

class IAbuseDetector {
public:
    /**
     * @brief IAbuse Detector.
     * @return Return value.
     */
    virtual ~IAbuseDetector() = default;

    [[nodiscard]] virtual AbuseDetectionResult detect(
        const std::string& content_data,
        const AbuseDetectorMetadata& metadata
    ) const = 0;

    [[nodiscard]] virtual std::string detectorType() const = 0;
};

// ============================================================================
// PhotoDNAAbuseDetector
// ============================================================================

class PhotoDNAAbuseDetector : public IAbuseDetector {
public:
    struct BlocklistEntry {
        uint64_t hash = 0;          ///< 64-bit perceptual hash
        std::string label;      ///< Descriptive label (e.g. "CSAM_HASH_001")
        AbuseAction action;     ///< BLOCK or FLAG
    };

    explicit PhotoDNAAbuseDetector(
        std::vector<BlocklistEntry> blocklist,
        int match_threshold = 10
    );

    AbuseDetectionResult detect(
        const std::string& content_data,
        const AbuseDetectorMetadata& metadata
    ) const override;

    std::string detectorType() const override { return "PhotoDNA"; }

    /**
     * @brief Compute Hash.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static uint64_t computeHash(const std::string& data);

    /**
     * @brief Hamming Distance.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static int hammingDistance(uint64_t a, uint64_t b);

private:
    std::vector<BlocklistEntry> blocklist_;
    int match_threshold_;
};

// ============================================================================
// TextAbuseDetector
// ============================================================================

class TextAbuseDetector : public IAbuseDetector {
public:
    struct Pattern {
        std::string name;
        std::regex  compiled = {};
        AbuseAction action;
    };

    /**
     * @brief Text Abuse Detector.
     * @param[in] patterns Input parameter.
     * @return Return value.
     */
    explicit TextAbuseDetector(std::vector<Pattern> patterns);

    /**
     * @brief Load From YAML.
     * @param[in] yaml_path Path to the yaml.
     * @param[in,out] error Input/output parameter.
     * @return Return value.
     */
    static std::unique_ptr<TextAbuseDetector> loadFromYAML(
        const std::string& yaml_path,
        std::string& error
    );

    AbuseDetectionResult detect(
        const std::string& content_data,
        const AbuseDetectorMetadata& metadata
    ) const override;

    std::string detectorType() const override { return "Text"; }

    std::size_t patternCount() const { return patterns_.size(); }

private:
    std::vector<Pattern> patterns_;
};

} // namespace content
} // namespace themis
