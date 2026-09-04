/**
 * @file abuse_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Actions that an abuse detector can take on matched content.
 */
enum class AbuseAction {
    ALLOW,  ///< Content is clean; no action required
    FLAG,   ///< Content is suspicious; store with a flag for review
    BLOCK   ///< Content is clearly abusive; reject/block it
};

/**
 * @brief Convert AbuseAction to a human-readable string.
 */
inline std::string abuseActionToString(AbuseAction action) {
    switch (action) {
        case AbuseAction::ALLOW: return "ALLOW";
        case AbuseAction::FLAG:  return "FLAG";
        case AbuseAction::BLOCK: return "BLOCK";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief Result returned by an IAbuseDetector implementation.
 */
struct AbuseDetectionResult {
    AbuseAction action = AbuseAction::ALLOW; ///< Action to take
    std::string detector_type;              ///< e.g. "PhotoDNA", "Text"
    std::string pattern_name;               ///< Which pattern/hash matched (empty if ALLOW)
    std::string reason;                     ///< Human-readable reason (empty if ALLOW)
};

/**
 * @brief Metadata passed to IAbuseDetector::detect().
 *
 * Provides contextual information without re-embedding it in the raw data.
 */
struct AbuseDetectorMetadata {
    std::string content_id;    ///< Unique content identifier
    std::string mime_type;     ///< MIME type of the content
    std::string content_hash;  ///< SHA-256 hex digest of the raw bytes
};

/**
 * @brief Abstract interface for content abuse detection.
 *
 * Implementations are injected into ContentSecurityManager and called from
 * ContentSecurityManager::checkAbuse().  Each implementation is responsible
 * for a specific category of abusive content (image hashes, text patterns …).
 *
 * Thread-safety: detect() MUST be safe to call concurrently from multiple
 * threads after construction.
 */
class IAbuseDetector {
public:
    virtual ~IAbuseDetector() = default;

    /**
     * @brief Examine @p content_data for abusive content.
     *
     * @param content_data Raw bytes of the content item.
     * @param metadata     Contextual metadata (mime_type, content_id, …).
     * @return AbuseDetectionResult with action ALLOW / FLAG / BLOCK.
     */
    [[nodiscard]] virtual AbuseDetectionResult detect(
        const std::string& content_data,
        const AbuseDetectorMetadata& metadata
    ) const = 0;

    /**
     * @brief Human-readable name of this detector type.
     */
    [[nodiscard]] virtual std::string detectorType() const = 0;
};

// ============================================================================
// PhotoDNAAbuseDetector
// ============================================================================

/**
 * @brief A perceptual-hash–based abuse detector for image content.
 *
 * Computes a 64-bit mean-hash of the image data and compares it against a
 * configured blocklist using Hamming distance.  Any distance within
 * @p match_threshold bits is considered a match.
 *
 * In production this can be replaced by or combined with the Microsoft
 * PhotoDNA SDK by swapping computeHash() for the SDK call and loading hashes
 * from the PhotoDNA hash database.
 *
 * Non-image MIME types (those not starting with "image/") are passed through
 * as ALLOW without hash computation.
 */
class PhotoDNAAbuseDetector : public IAbuseDetector {
public:
    /**
     * @brief Entry in the image hash blocklist.
     */
    struct BlocklistEntry {
        uint64_t hash = 0;          ///< 64-bit perceptual hash
        std::string label;      ///< Descriptive label (e.g. "CSAM_HASH_001")
        AbuseAction action;     ///< BLOCK or FLAG
    };

    /**
     * @brief Construct with a pre-loaded blocklist and optional Hamming threshold.
     *
     * @param blocklist        Known-bad image hashes with their intended actions.
     * @param match_threshold  Maximum Hamming distance to consider a match (default 10).
     */
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
     * @brief Compute the 64-bit mean perceptual hash of @p data.
     *
     * Samples 64 evenly-spaced bytes, computes their mean intensity, then
     * sets each bit in the result to 1 if the corresponding sample is ≥ mean.
     * This is a simplified substitute for a full DCT-based pHash; sufficient
     * for deterministic blocklist comparison.
     *
     * @param data  Raw image bytes (any format; treated as grayscale samples).
     * @return 64-bit perceptual hash.
     */
    static uint64_t computeHash(const std::string& data);

    /**
     * @brief Hamming distance between two 64-bit hashes.
     */
    static int hammingDistance(uint64_t a, uint64_t b);

private:
    std::vector<BlocklistEntry> blocklist_;
    int match_threshold_;
};

// ============================================================================
// TextAbuseDetector
// ============================================================================

/**
 * @brief Pattern-based abuse detector for text content.
 *
 * Loads a list of regex patterns from a YAML configuration file.  Each
 * pattern specifies an action (BLOCK or FLAG) that is applied when the
 * regex matches.  The first matching pattern wins; patterns are evaluated
 * in the order they appear in the YAML.
 *
 * Expected YAML structure (see config/security/abuse_patterns.yaml):
 * @code
 * version: "1.0"
 * patterns:
 *   - name: "spam_keywords"
 *     regex: "(buy now|click here)"
 *     flags: ["icase"]
 *     action: "FLAG"
 *     enabled: true
 * @endcode
 */
class TextAbuseDetector : public IAbuseDetector {
public:
    /**
     * @brief A single compiled abuse pattern.
     */
    struct Pattern {
        std::string name;
        std::regex  compiled;
        AbuseAction action;
    };

    /**
     * @brief Construct with an explicit list of already-compiled patterns.
     *
     * Prefer loadFromYAML() for production use.
     */
    explicit TextAbuseDetector(std::vector<Pattern> patterns);

    /**
     * @brief Load patterns from a YAML file and construct a TextAbuseDetector.
     *
     * If the file cannot be read or parsed the detector is constructed with
     * an empty pattern list (all content passes through as ALLOW) and the
     * error is surfaced via the returned error string.
     *
     * @param yaml_path  Path to the abuse_patterns.yaml file.
     * @param[out] error Populated with a description on failure; empty on success.
     * @return Owning pointer to the constructed detector.
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

    /**
     * @brief Number of compiled patterns currently loaded.
     */
    std::size_t patternCount() const { return patterns_.size(); }

private:
    std::vector<Pattern> patterns_;
};

} // namespace content
} // namespace themis
