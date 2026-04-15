/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_policy.cpp                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:17:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/prompt_policy.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace llm {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void PromptPolicy::addRule(PolicyRule rule) {
    // Compile the regex eagerly so callers get an error at configuration time,
    // not at request time.
    try {
        std::regex compiled(rule.pattern,
                            std::regex::ECMAScript | std::regex::icase);
        rules_.push_back({std::move(rule), std::move(compiled)});
    } catch (const std::regex_error& e) {
        throw std::invalid_argument(
            "PromptPolicy: invalid regex pattern for rule '" + rule.name +
            "': " + e.what());
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PromptPolicy::addBlockRule(const std::string& name,
                                const std::string& pattern) {
    addRule({name, pattern, /*block=*/true});
}

void PromptPolicy::addRedactRule(const std::string& name,
                                 const std::string& pattern,
                                 const std::string& replacement) {
    PolicyRule r(name, pattern, /*block=*/false, replacement);
    addRule(std::move(r));
}

bool PromptPolicy::removeRule(const std::string& name) {
    auto it = std::find_if(rules_.begin(), rules_.end(),
                           [&](const CompiledRule& cr) {
                               return cr.rule.name == name;
                           });
    if (it == rules_.end()) {
        return false;
    }
    rules_.erase(it);
    return true;
}

size_t PromptPolicy::ruleCount() const {
    return rules_.size();
}

PolicyResult PromptPolicy::apply(const std::string& prompt) const {
    PolicyResult result;
    result.sanitized_prompt = prompt;  // start with a copy; redacts accumulate

    for (const auto& cr : rules_) {
        if (cr.rule.block) {
            // Block rule: check for a match; return immediately on first hit
            if (std::regex_search(result.sanitized_prompt, cr.regex)) {
                result.allowed       = false;
                result.rule_name     = cr.rule.name;
                result.reason        = "Prompt blocked by policy rule '" +
                                       cr.rule.name + "'";
                spdlog::warn("PromptPolicy: prompt blocked by rule '{}' — request rejected",
                             cr.rule.name);
                return result;
            }
        } else {
            // Redact rule: replace all matches in the accumulated prompt
            const std::string& replacement =
                cr.rule.redact_with.empty() ? "[REDACTED]" : cr.rule.redact_with;

            std::string replaced =
                std::regex_replace(result.sanitized_prompt, cr.regex, replacement);

            if (replaced != result.sanitized_prompt) {
                spdlog::debug("PromptPolicy: rule '{}' redacted content from prompt",
                              cr.rule.name);
                result.sanitized_prompt = std::move(replaced);
            }
        }
    }

    return result;
}

} // namespace llm
} // namespace themis
