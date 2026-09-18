/**
 * @file prompt_policy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct PolicyResult {
    bool allowed = true;            ///< false if the prompt was blocked
    std::string sanitized_prompt;   ///< sanitized text (may differ from input)
    std::string rule_name;          ///< name of the rule that triggered, if any
    std::string reason;             ///< human-readable explanation
};

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

    struct CompiledRule {
        PolicyRule rule;
        std::regex  regex = {};
    };

    /**
     * @brief Add Block Rule.
     * @param[in] name Input parameter.
     * @param[in] pattern Input parameter.
     */
    void addBlockRule(const std::string& name, const std::string& pattern);

    void addRedactRule(const std::string& name, const std::string& pattern,
                       const std::string& replacement = "[REDACTED]");

    /**
     * @brief Remove Rule.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool removeRule(const std::string& name);

    /**
     * @brief Rule Count.
     * @return Return value.
     */
    size_t ruleCount() const;

    /**
     * @brief Apply.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    PolicyResult apply(const std::string& prompt) const;

    const std::vector<CompiledRule>& rules() const { return rules_; }

private:
    std::vector<CompiledRule> rules_;

    /**
     * @brief Add Rule.
     * @param[in] rule Input parameter.
     */
    void addRule(PolicyRule rule);
};

} // namespace llm
} // namespace themis
