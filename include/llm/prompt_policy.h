/**
 * @file prompt_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
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
#include <functional>
#include <memory>

namespace themis {
namespace llm {

/**
 * @brief Result of applying a PromptPolicy to a prompt string.
 */
struct PolicyResult {
    bool allowed = true;            ///< false if the prompt was blocked
    std::string sanitized_prompt;   ///< sanitized text (may differ from input)
    std::string rule_name;          ///< name of the rule that triggered, if any
    std::string reason;             ///< human-readable explanation
};

/**
 * @brief A single policy rule that can block or sanitize a prompt.
 *
 * Rules are evaluated in order.  The first rule that matches either blocks the
 * prompt (if block == true) or redacts the matched portion with redact_with.
 */
struct PolicyRule {
    std::string name;           ///< unique rule identifier (used in audit log)
    std::string pattern;        ///< ECMAScript regex pattern string
    bool block = false;         ///< if true, matched prompt is blocked outright
    std::string redact_with;    ///< replacement text when block==false (default "[REDACTED]")

    PolicyRule() = default;
    PolicyRule(std::string name_, std::string pattern_, bool block_,
               std::string redact = "[REDACTED]")
        : name(std::move(name_))
        , pattern(std::move(pattern_))
        , block(block_)
        , redact_with(std::move(redact)) {}
};

/**
 * @brief Prompt safety policy with configurable keyword/regex rules.
 *
 * PromptPolicy evaluates an ordered list of PolicyRule entries against the
 * input prompt.  Each rule can either:
 *
 *   - Block the request entirely (PolicyResult::allowed == false), or
 *   - Redact the matched substring in-place.
 *
 * Rules are compiled on construction (or when mutated) and applied left-to-
 * right.  Block rules short-circuit — the first match that blocks returns
 * immediately.  Redact rules continue after applying their substitution so
 * multiple redactions can accumulate.
 *
 * Usage (Q1 minimum viable path):
 * @code
 *   PromptPolicy policy;
 *   policy.addBlockRule("no_jailbreak",  "(?i)ignore (all |previous )?instructions");
 *   policy.addRedactRule("phone_number", R"(\b\d{3}[-.\s]\d{3}[-.\s]\d{4}\b)");
 *
 *   auto result = policy.apply(user_prompt);
 *   if (!result.allowed) {
 *       // Reject request; log result.rule_name and result.reason
 *   } else {
 *       // Use result.sanitized_prompt for inference
 *   }
 * @endcode
 *
 * @see docs/llm_roadmap.md — Q1 Safety/Policy checklist
 */
class PromptPolicy {
public:
    PromptPolicy() = default;
    ~PromptPolicy() = default;

    // Not copyable (compiled regex objects are non-trivial)
    PromptPolicy(const PromptPolicy&) = delete;
    PromptPolicy& operator=(const PromptPolicy&) = delete;

    // Movable
    PromptPolicy(PromptPolicy&&) noexcept = default;
    PromptPolicy& operator=(PromptPolicy&&) noexcept = default;

    /**
     * @brief A compiled rule entry: the original PolicyRule plus its compiled
     *        std::regex.  Exposed publicly so the rules() accessor is type-
     *        correct and callers can inspect both the rule definition and its
     *        compiled form.
     */
    struct CompiledRule {
        PolicyRule rule;
        std::regex  regex = {};
    };

    /**
     * @brief Add a rule that blocks the prompt when the pattern matches.
     * @param name    Unique rule name used in audit/log messages.
     * @param pattern ECMAScript regex pattern (std::regex).
     * @throws std::invalid_argument if @p pattern is not a valid regex.
     */
    void addBlockRule(const std::string& name, const std::string& pattern);

    /**
     * @brief Add a rule that redacts (replaces) matched text rather than
     *        blocking the prompt.
     * @param name        Unique rule name.
     * @param pattern     ECMAScript regex pattern.
     * @param replacement Replacement string (default "[REDACTED]").
     * @throws std::invalid_argument if @p pattern is not a valid regex.
     */
    void addRedactRule(const std::string& name, const std::string& pattern,
                       const std::string& replacement = "[REDACTED]");

    /**
     * @brief Remove a rule by name.
     * @return true if a rule with that name was found and removed.
     */
    bool removeRule(const std::string& name);

    /**
     * @brief Return the number of registered rules.
     */
    size_t ruleCount() const;

    /**
     * @brief Apply all registered policy rules to @p prompt.
     *
     * Rules are evaluated in insertion order.  Block rules short-circuit on
     * first match.  Redact rules are applied sequentially and their effects
     * accumulate in PolicyResult::sanitized_prompt.
     *
     * @param prompt  The raw user-supplied prompt text.
     * @return PolicyResult with allowed flag, sanitized text, and rule info.
     */
    PolicyResult apply(const std::string& prompt) const;

    /**
     * @brief Read-only view of the registered compiled rules
     *        (for inspection/testing).
     */
    const std::vector<CompiledRule>& rules() const { return rules_; }

private:
    std::vector<CompiledRule> rules_;

    void addRule(PolicyRule rule);
};

} // namespace llm
} // namespace themis
