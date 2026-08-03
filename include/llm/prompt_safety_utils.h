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

/*
 * ThemisDB | File: prompt_safety_utils.h | Version: 0.0.1 | Last Modified: 2026-05-31 20:06:47
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 58
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

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
