#pragma once

#include "llm/prompt_policy.h"

#include <string>

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
    auto result = sharedPromptSafetyPolicy().apply(input);
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
