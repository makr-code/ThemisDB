/**
 * @file prompt_safety_utils.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Utility Functions**: Header-only utility functions.
 *       No .cpp implementation needed. Functions are inline or header-only.
 */


#pragma once

#include "llm/prompt_policy.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <utility>

namespace {

inline std::string normalizePromptForSafety(std::string_view text) {
    std::string normalized = {};
    normalized.reserve(text.size());
    for (char ch : text) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalnum(uch)) {
            normalized.push_back(static_cast<char>(std::tolower(uch)));
        } else if (std::isspace(uch)) {
            normalized.push_back(' ');
        } else {
            normalized.push_back(' ');
        }
    }
    return normalized;
}

inline bool containsBlockedInstructionPattern(std::string_view text) {
    const std::string normalized = normalizePromptForSafety(text);
    return normalized.find("ignore all previous instructions") != std::string::npos ||
           normalized.find("ignore previous instructions") != std::string::npos ||
           normalized.find("disregard all previous instructions") != std::string::npos ||
           normalized.find("disregard previous instructions") != std::string::npos;
}

inline void redactLiteralToken(std::string& text, std::string_view token,
                              std::string_view replacement) {
    std::size_t pos = 0;
    while ((pos = text.find(token.data(), pos, token.size())) != std::string::npos) {
        text.replace(pos, token.size(), replacement.data(), replacement.size());
        pos += replacement.size();
    }
}

inline void redactControlTokens(std::string& text) {
    redactLiteralToken(text, "<|im_start|>", "[CONTROL_TOKEN]");
    redactLiteralToken(text, "<|im_end|>", "[CONTROL_TOKEN]");
    redactLiteralToken(text, "[INST]", "[CONTROL_TOKEN]");
    redactLiteralToken(text, "[/INST]", "[CONTROL_TOKEN]");
    redactLiteralToken(text, "<<SYS>>", "[CONTROL_TOKEN]");
    redactLiteralToken(text, "<</SYS>>", "[CONTROL_TOKEN]");
}

} // namespace

namespace themis::llm::prompt_safety {

/**
 * @brief Shared prompt-safety policy used across LLM, RAG, and training paths.
 *
 * The policy is initialized once and reused to ensure consistent rule behavior
 * for blocking and control-token redaction.
 */
inline PromptPolicy& sharedPromptSafetyPolicy() {
    static PromptPolicy policy = [] {
        PromptPolicy p;
        p.addBlockRule("prompt_override_ignore_instructions",
                       R"((?:ignore|disregard)\s+(?:all\s+)?(?:previous|prior)?\s*instructions)");
        p.addBlockRule("prompt_role_escalation_system",
                       R"((?:^|\n)\s*(?:system\s*:|###\s*system\b|<\|im_start\|>\s*system))");
        p.addRedactRule("prompt_control_tokens",
                        R"(<\|im_start\|>|<\|im_end\|>|\[/?INST\]|<<SYS>>|<</SYS>>)",
                        "[CONTROL_TOKEN]");
        return p;
    }();
    return policy;
}

/**
 * @brief Apply shared prompt safety policy to input text.
 * @param input Raw input text.
 * @param sanitized Receives sanitized text when allowed.
 * @param blocked_rule Optional receiver for triggering rule id.
 * @param blocked_reason Optional receiver for human-readable reason.
 * @return true when prompt is allowed; false when blocked.
 */
inline bool sanitizePromptWithSharedPolicy(
    const std::string& input,
    std::string& sanitized,
    std::string* blocked_rule,
    std::string* blocked_reason)
{
    if (containsBlockedInstructionPattern(input)) {
        if (blocked_rule) {
            *blocked_rule = "prompt_override_ignore_instructions";
        }
        if (blocked_reason) {
            *blocked_reason = "Prompt blocked by safety policy rule 'prompt_override_ignore_instructions'";
        }
        return false;
    }

    sanitized = input;
    redactControlTokens(sanitized);

    auto result = sharedPromptSafetyPolicy().apply(sanitized);
    if (!result.allowed) {
        if (blocked_rule) {
            *blocked_rule = result.rule_name;
        }
        if (blocked_reason) {
            *blocked_reason = result.reason;
        }
        return false;
    }
    sanitized = std::move(result.sanitized_prompt);
    return true;
}

} // namespace themis::llm::prompt_safety
