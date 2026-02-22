/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_injection_detector.cpp                      ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 12:27:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "prompt_engineering/prompt_injection_detector.h"
#include <spdlog/spdlog.h>
#include <algorithm>

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

PromptInjectionDetector::PromptInjectionDetector(const Config& config)
    : config_(config) {
    initializePatterns();
}

void PromptInjectionDetector::initializePatterns() {
    // Patterns that attempt to override or disregard existing instructions
    auto add = [this](const std::string& label, const std::string& expr) {
        patterns_.emplace_back(expr, std::regex::icase);
        pattern_labels_.push_back(label);
    };

    add("ignore_instructions",
        R"(ignore\s+(?:previous|all|prior|your|these)\s+(?:instructions?|prompts?|rules?|context|constraints?))");
    add("disregard_instructions",
        R"(disregard\s+(?:previous|all|prior|your|these)\s+(?:instructions?|prompts?|rules?|context|constraints?))");
    add("forget_instructions",
        R"(forget\s+(?:everything|all|prior|previous|your)\s*(?:previous\s+)?(?:instructions?|rules?|context|training))");
    add("reveal_system_prompt",
        R"(reveal\s+(?:your\s+|the\s+|this\s+|all\s+)?(?:system|hidden|original|internal)\s+(?:prompt|instruction|config))");
    add("tell_system_prompt",
        R"(tell\s+me\s+(?:your|the)\s+(?:system\s+)?(?:prompt|instructions?|directives?))");
    add("print_system_prompt",
        R"((?:print|show|output|display|repeat|share)\s+(?:your\s+|the\s+)?(?:system\s+)?(?:prompt|instructions?|directives?))");
    add("special_system_token",
        R"(\[\s*system\s*\]|\[INST\]|\[\/INST\]|<\|system\|>|<\|user\|>|<\|assistant\|>)");
    add("jailbreak_mode",
        R"((?:enter|activate|enable|switch\s+to)\s+(?:DAN|jailbreak|developer|god|unrestricted|free)\s+mode)");
    add("act_as_unrestricted",
        R"(act\s+as\s+(?:if\s+you\s+(?:have\s+no|are\s+without)\s+(?:restrictions?|guidelines?|filters?)|an?\s+unfiltered))");
    add("override_safety",
        R"((?:override|bypass|disable|ignore)\s+(?:safety|content|moderation|ethical?)\s+(?:filter|guideline|restriction|check))");

    // Dangerous keywords scored separately
    dangerous_keywords_ = {
        "jailbreak", "pwned", "hacked",
        "exploit", "privilege", "root access",
        "execute arbitrary", "eval(", "require(",
        "bypass filter", "bypass safety"
    };

    // Append caller-supplied custom patterns (best-effort; skip on compile error)
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
    for (size_t i = 0; i < patterns_.size(); ++i) {
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
    if (!text.empty() && special * 10 > text.size()) {
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
    // Responses are subject to the same heuristics – an adversarially crafted
    // response may attempt to inject instructions that will be forwarded to a
    // subsequent LLM call (indirect / second-order injection).
    return detect(response);
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
            result.replace(pos, kw.size(), "[REDACTED]");
            // Keep lower copy in sync
            lower_result.replace(pos, kw.size(), "[REDACTED]");
            pos += 10; // len("[REDACTED]")
        }
    }

    return result;
}

} // namespace prompt_engineering
} // namespace themis
