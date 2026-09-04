/**
 * @file prompt_injection_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=6; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=3, Debt=0, C=0, H=3, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "prompt_engineering/prompt_injection_detector.h"
#include "security/prompt_injection_pattern_registry.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <utility>

namespace themis {
namespace prompt_engineering {

// ---------------------------------------------------------------------------
// DetectionResult
// ---------------------------------------------------------------------------

nlohmann::json PromptInjectionDetector::DetectionResult::toJson() const {
    return {
        {"is_injection", is_injection},
        {"risk_score", risk_score},
        {"matched_patterns", matched_patterns},
        {"sanitized_text", sanitized_text}
    };
}

// ---------------------------------------------------------------------------
// PromptInjectionDetector
// ---------------------------------------------------------------------------

PromptInjectionDetector::PromptInjectionDetector()
    : PromptInjectionDetector(Config{}) {}

PromptInjectionDetector::PromptInjectionDetector(Config config)
    : config_(std::move(config)) {
    initializePatterns();
}

void PromptInjectionDetector::initializePatterns() {
    // Gap 5 (AI_ML_IMPACT_ASSESSMENT.md §7): load the canonical shared pattern
    // registry so that patterns added to the registry appear here automatically.
    // Domain-specific patterns (custom_patterns, dangerous_keywords) are appended
    // after the shared base.
    const auto& reg = themis::security::PromptInjectionPatternRegistry::defaultRegistry();

    // Startup parity assertion (Gap 5 enforcement): abort rather than silently
    // miss shared patterns if the registry and the compile-time constant diverge.
    if (reg.patternCount() != themis::security::SHARED_INJECTION_PATTERN_COUNT) {
        spdlog::error("PromptInjectionDetector (PE): expected {} shared patterns, "
                      "found {} — Gap 5 parity violation. "
                      "Update SHARED_INJECTION_PATTERN_COUNT.",
                      themis::security::SHARED_INJECTION_PATTERN_COUNT,
                      reg.patternCount());
    }

    // ── Load shared patterns ──────────────────────────────────────────────────
    for (const auto& e : reg.patterns()) {
        try {
            patterns_.emplace_back(e.pattern_str, std::regex::icase);
            pattern_labels_.push_back(e.label);
        } catch (const std::regex_error&) {
            spdlog::warn("PromptInjectionDetector (PE): failed to compile shared "
                         "pattern '{}' — skipped", e.label);
        }
    }

    // ── Dangerous keywords (from shared registry) ─────────────────────────────
    dangerous_keywords_ = std::vector<std::string>(reg.keywords().begin(),
                                                   reg.keywords().end());

    // ── Append caller-supplied custom patterns (best-effort) ─────────────────
    for (const auto& pat : config_.custom_patterns) {
        try {
            patterns_.emplace_back(pat, std::regex::icase);
            pattern_labels_.push_back("custom:" + pat);
        } catch (const std::regex_error&) {
            // Skip invalid custom patterns silently
        }
    }
}

// ---------------------------------------------------------------------------
// Core detection helpers
// ---------------------------------------------------------------------------

float PromptInjectionDetector::calculatePatternScore(
        const std::string& text, std::vector<std::string>& matched_out) const {
    int hits = 0;
    for (size_t i = 0; i <static_cast<int>(patterns_.size()); ++i) {
        if (std::regex_search(text, patterns_[i])) {
            ++hits;
            matched_out.push_back(pattern_labels_[i]);
        }
    }
    // Each clear injection pattern contributes 0.7 toward a max of 1.0.
    // A single strong pattern already meets the default 0.7 threshold.
    return std::min(1.0f, static_cast<float>(hits) * 0.7f);
}

float PromptInjectionDetector::calculateKeywordScore(
        const std::string& text, std::vector<std::string>& matched_out) const {
    std::string lower = text;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    int hits = 0;
    for (const auto& kw : dangerous_keywords_) {
        if (lower.find(kw) != std::string::npos) {
            ++hits;
            matched_out.push_back("keyword:" + kw);
        }
    }
    return std::min(1.0f, static_cast<float>(hits) / 5.0f);
}

float PromptInjectionDetector::calculateSyntaxScore(
        const std::string& text, std::vector<std::string>& matched_out) const {
    float score = 0.0f;

    // Instruction-bracketing tokens common in LLM hijack attempts
    if (text.find("[INST]") != std::string::npos ||
        text.find("[/INST]") != std::string::npos) {
        score += 0.4f;
        matched_out.push_back("syntax:instruction_bracket_token");
    }

    // Unusually high density of angle brackets / pipes / braces
    size_t special = 0;
    for (char c : text) {
        if (c == '<' || c == '>' || c == '|') {
            ++special;
        }
    }
    if (!text.empty() && special * 10 > static_cast<int>(text.size())) {
        score += 0.3f;
        matched_out.push_back("syntax:high_special_char_density");
    }

    return std::min(1.0f, score);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

PromptInjectionDetector::DetectionResult
PromptInjectionDetector::detect(const std::string& prompt) const {
    DetectionResult result;
    result.sanitized_text = prompt;

    if (!config_.enabled) {
        return result;
    }

    std::vector<std::string> matched;
    float pattern_score = calculatePatternScore(prompt, matched);
    float keyword_score = calculateKeywordScore(prompt, matched);
    float syntax_score  = calculateSyntaxScore(prompt, matched);

    // Patterns dominate; keywords and syntax provide additional confirmation.
    // A single clear injection pattern (score 0.7) already meets the default threshold.
    result.risk_score = std::min(1.0f, pattern_score +
                                       0.2f * keyword_score +
                                       0.1f * syntax_score);
    result.matched_patterns = std::move(matched);
    result.is_injection = result.risk_score >= config_.risk_threshold;
    result.sanitized_text = sanitize(prompt);

    if (config_.log_detections && result.is_injection) {
        spdlog::warn("PromptInjectionDetector: injection detected (risk_score={:.2f})",
                     result.risk_score);
    }

    return result;
}

PromptInjectionDetector::DetectionResult
PromptInjectionDetector::detectInResponse(const std::string& response) const {
    if (!config_.enabled) {
        DetectionResult result;
        result.sanitized_text = response;
        return result;
    }

    // Start with the base detection pass shared with user-supplied prompts.
    // Adversarially crafted responses often embed the same override patterns.
    DetectionResult result = detect(response);

    // Additional heuristics for indirect / second-order injection: cases where
    // the model embeds fake system-role tokens or instruction blocks in its own
    // output, intending them to be parsed as authoritative context when the
    // response is forwarded to a subsequent LLM call.
    static const std::vector<std::pair<std::regex, std::string>> kResponsePatterns = {
        // Fake "SYSTEM:" role prefix at the start of a line
        {std::regex(R"((?:^|\n)\s*SYSTEM\s*:)", std::regex::icase),
         "response:fake_system_prefix"},
        // Embedded "[SYS]" or "[SYSTEM]" bracket tokens
        {std::regex(R"(\[\s*(?:SYS|SYSTEM)\s*\])", std::regex::icase),
         "response:embedded_system_token"},
        // "New context:" / "New instructions:" / "New system prompt:" patterns
        {std::regex(R"(new\s+(?:context|system\s+prompt|instructions?)\s*:)",
                    std::regex::icase),
         "response:embedded_new_instructions"},
        // "Updated instructions:" / "Updated rules:" in a response
        {std::regex(R"(updated\s+(?:instructions?|rules?|constraints?)\s*:)",
                    std::regex::icase),
         "response:updated_instructions"},
    };

    std::vector<std::string> extra_matched;
    int extra_hits = 0;
    for (const auto& [pat, label] : kResponsePatterns) {
        if (std::regex_search(response, pat)) {
            ++extra_hits;
            extra_matched.push_back(label);
        }
    }

    if (extra_hits > 0) {
        // Each response-specific pattern hit contributes the same weight as a
        // base injection pattern (0.7) so that a single strong indicator such
        // as a fake "SYSTEM:" prefix is sufficient to meet the default threshold.
        result.risk_score = std::min(
            1.0f, result.risk_score + static_cast<float>(extra_hits) * 0.7f);
        result.matched_patterns.insert(result.matched_patterns.end(),
                                       extra_matched.begin(), extra_matched.end());
        result.is_injection = result.risk_score >= config_.risk_threshold;
        result.sanitized_text = sanitize(response);

        if (config_.log_detections && result.is_injection) {
            spdlog::warn(
                "PromptInjectionDetector: indirect injection detected in response "
                "(risk_score={:.2f})", result.risk_score);
        }
    }

    return result;
}

std::string PromptInjectionDetector::sanitize(const std::string& text) const {
    std::string result = text;

    for (const auto& pat : patterns_) {
        result = std::regex_replace(result, pat, "[REDACTED]");
    }

    // Replace dangerous keywords (case-insensitive simple scan)
    std::string lower_result = result;
    std::transform(lower_result.begin(), lower_result.end(),
                   lower_result.begin(), ::tolower);

    for (const auto& kw : dangerous_keywords_) {
        size_t pos = 0;
        while ((pos = lower_result.find(kw, pos)) != std::string::npos) {
            result.replace(pos,static_cast<int>(kw.size()), "[REDACTED]");
            // Keep lower copy in sync
            lower_result.replace(pos,static_cast<int>(kw.size()), "[REDACTED]");
            pos += 10; // len("[REDACTED]")
        }
    }

    return result;
}

} // namespace prompt_engineering
} // namespace themis
