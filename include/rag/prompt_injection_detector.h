/**
 * @file prompt_injection_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis::rag::security {

/**
 * @brief Severity level of a detected injection attempt.
 */
enum class InjectionSeverity {
    NONE,     ///< No injection detected
    LOW,      ///< Mildly suspicious (informational)
    MEDIUM,   ///< Likely injection attempt; flag for review
    HIGH,     ///< Clear injection attempt; block or sanitize
    CRITICAL, ///< Targeted escape / override of system instructions
};

/**
 * @brief A single detected injection finding.
 */
struct InjectionFinding {
    /// Severity of this finding.
    InjectionSeverity severity = InjectionSeverity::NONE;

    /// Short tag identifying the attack category.
    std::string       category;

    /// Human-readable description of the finding.
    std::string       description;

    /// The exact fragment that matched, or empty when pattern is positional.
    std::string       matched_fragment;

    /// Character offset in the input text where the match starts (0-based).
    size_t            offset = 0;
};

/**
 * @brief Result of scanning a single text fragment.
 */
struct InjectionScanResult {
    /// Highest severity across all findings.
    InjectionSeverity    max_severity = InjectionSeverity::NONE;

    /// All findings in detection order.
    std::vector<InjectionFinding> findings;

    /// True if any finding is MEDIUM or higher.
    bool is_suspicious() const;

    /// True if any finding is HIGH or CRITICAL.
    bool is_blocked() const;

    /// Number of findings at or above @p threshold.
    size_t countAtOrAbove(InjectionSeverity threshold) const;
};

/**
 * @brief Configuration for the detector.
 */
struct DetectorConfig {
    /// Maximum fraction of a document that may consist of suspected injection
    /// payload before the entire document is flagged HIGH (default 0.15 = 15%).
    double max_injection_density = 0.15;

    /// Enable detection of Unicode homoglyph / direction-override attacks
    /// (slightly more expensive; default true).
    bool check_unicode_attacks = true;

    /// Enable detection of HTML/Markdown injection that could affect rendered output.
    bool check_markup_injection = true;

    /// Enable detection of classic instruction-override phrases.
    bool check_instruction_overrides = true;

    /// Enable detection of delimiter-based escape attempts (e.g. ``---``, `###SYSTEM`).
    bool check_delimiter_escapes = true;

    /// Enable detection of role-play / persona injection ("act as", "jailbreak").
    bool check_role_injection = true;
};

/**
 * @brief Heuristic prompt-injection detector for retrieved RAG context.
 *
 * Thread-safe for concurrent `scan()` calls after construction.
 */
class PromptInjectionDetector {
public:
    explicit PromptInjectionDetector(const DetectorConfig& config = DetectorConfig{});
    ~PromptInjectionDetector();

    PromptInjectionDetector(const PromptInjectionDetector&)            = delete;
    PromptInjectionDetector& operator=(const PromptInjectionDetector&) = delete;
    PromptInjectionDetector(PromptInjectionDetector&&)                 noexcept = default;
    PromptInjectionDetector& operator=(PromptInjectionDetector&&)      noexcept = default;

    /**
     * @brief Scan a single text fragment for injection patterns.
     * @param text   Raw text to inspect (document content or answer).
     * @return       Scan result containing all findings and max severity.
     */
    InjectionScanResult scan(const std::string& text) const;

    /**
     * @brief Scan all retrieved documents in an EvaluationInput.
     * @param input  Full evaluation input.
     * @return       Per-document scan results indexed by document position.
     */
    std::vector<InjectionScanResult>
    scanDocuments(const judge::EvaluationInput& input) const;

    /**
     * @brief Return the current detector configuration.
     */
    DetectorConfig getConfig() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Configuration for the sanitizer.
 */
struct SanitizerConfig {
    /// Replace matched injection fragments with this placeholder.
    std::string placeholder = "[CONTENT REMOVED: injection risk]";

    /// Only remove findings at or above this threshold (default MEDIUM).
    InjectionSeverity removal_threshold = InjectionSeverity::MEDIUM;

    /// Limit maximum document length to this many characters (0 = no limit).
    size_t max_document_length = 0;

    /// Strip Unicode direction-override characters unconditionally.
    bool strip_unicode_overrides = true;
};

/**
 * @brief Sanitizer that neutralises injection patterns in retrieved text.
 *
 * Operates on scan results produced by PromptInjectionDetector to avoid
 * running detection twice.  Can also run detection internally via the
 * `sanitize(text)` overload.
 *
 * Thread-safe for concurrent `sanitize()` calls after construction.
 */
class PromptInjectionSanitizer {
public:
    explicit PromptInjectionSanitizer(
        const SanitizerConfig&  sanitizer_config = SanitizerConfig{},
        const DetectorConfig&   detector_config  = DetectorConfig{}
    );
    ~PromptInjectionSanitizer();

    PromptInjectionSanitizer(const PromptInjectionSanitizer&)            = delete;
    PromptInjectionSanitizer& operator=(const PromptInjectionSanitizer&) = delete;

    /**
     * @brief Sanitize a text string.
     * @param text    Input text (may be modified in-place equivalent).
     * @return        Sanitized copy with injection content neutralised.
     */
    std::string sanitize(const std::string& text) const;

    /**
     * @brief Sanitize all documents in an EvaluationInput.
     * @param input   Original evaluation input.
     * @return        Copy with document contents sanitized.
     */
    judge::EvaluationInput
    sanitizeInput(const judge::EvaluationInput& input) const;

    /**
     * @brief Sanitize using a pre-computed scan result (avoids double scan).
     * @param text    Original text.
     * @param scan    Pre-computed scan result.
     * @return        Sanitized copy.
     */
    std::string sanitize(
        const std::string&      text,
        const InjectionScanResult& scan
    ) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace themis::rag::security
