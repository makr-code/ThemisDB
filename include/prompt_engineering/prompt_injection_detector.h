/**
 * @file prompt_injection_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <regex>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

/**
 * @brief Detects prompt injection attacks in user-supplied prompts and model responses.
 *
 * This class is the prompt injection attack detection layer for the prompt_engineering
 * module. It performs pattern-based and keyword-based analysis to identify attempts
 * to override system instructions, reveal internal prompts, or bypass safety guidelines.
 *
 * Callers are responsible for invoking the detector before dispatching prompts and
 * after receiving model responses.
 *
 * Usage:
 * @code
 * PromptInjectionDetector detector;
 * auto result = detector.detect(user_input);
 * if (result.is_injection) {
 *     // reject or sanitize
 *     auto safe = detector.sanitize(user_input);
 * }
 * @endcode
 */
class PromptInjectionDetector {
public:
    /**
     * @brief Configuration for injection detection.
     */
    struct Config {
        bool enabled = true;
        /// Risk score threshold above which a text is flagged (0.0–1.0).
        float risk_threshold = 0.7f;
        /// Emit warnings to the logger when a detection occurs.
        bool log_detections = true;
        /// Optional additional regex patterns (case-insensitive) supplied by the caller.
        std::vector<std::string> custom_patterns;
    };

    /**
     * @brief Result of an injection detection pass.
     */
    struct DetectionResult {
        /// True when the overall risk score meets or exceeds the configured threshold.
        bool is_injection = false;
        /// Composite risk score in [0.0, 1.0].
        float risk_score = 0.0f;
        /// Human-readable list of patterns that contributed to the score.
        std::vector<std::string> matched_patterns;
        /// Version of the input with injection patterns replaced by "[REDACTED]".
        std::string sanitized_text;

        /// Serialise to JSON for audit logging.
        nlohmann::json toJson() const;
    };

    PromptInjectionDetector();
    explicit PromptInjectionDetector(Config config);
    ~PromptInjectionDetector() = default;

    /**
     * @brief Analyse a user-supplied prompt for injection attempts.
     *
     * @param prompt  The text to analyse.
     * @return        DetectionResult with risk score, matched patterns and sanitized copy.
     */
    DetectionResult detect(const std::string& prompt) const;

    /**
     * @brief Analyse a model response for embedded injection / override instructions.
     *
     * Model responses may themselves contain adversarially crafted instructions that
     * attempt to override the next system prompt (indirect injection). This method
     * applies the same detection heuristics to the response text.
     *
     * @param response  Model response text.
     * @return          DetectionResult with risk score, matched patterns and sanitized copy.
     */
    DetectionResult detectInResponse(const std::string& response) const;

    /**
     * @brief Return a sanitized copy of @p text with injection patterns replaced.
     *
     * @param text  Original text.
     * @return      Text with detected patterns replaced by "[REDACTED]".
     */
    std::string sanitize(const std::string& text) const;

private:
    Config config_;
    std::vector<std::regex> patterns_;
    std::vector<std::string> pattern_labels_;
    std::vector<std::string> dangerous_keywords_;

    void initializePatterns();

    float calculatePatternScore(const std::string& text,
                                std::vector<std::string>& matched_out) const;
    float calculateKeywordScore(const std::string& text,
                                std::vector<std::string>& matched_out) const;
    float calculateSyntaxScore(const std::string& text,
                               std::vector<std::string>& matched_out) const;
};

} // namespace prompt_engineering
} // namespace themis
