/*
 * ThemisDB | File: model_router.cpp | Version: 0.0.15 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 196
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=38 | delta=35 | status=divergent
 * External Severity (v3): C=0, H=33, M=5
 * PR: #4370 [WIP] Update llm documentation to match source code (2026-03-21T18:21:30Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "llm/model_router.h"
#include <algorithm>
#include <stdexcept>
#include <spdlog/spdlog.h>

namespace themis {
namespace llm {

// ═══════════════════════════════════════════════════════════
// Private helpers
// ═══════════════════════════════════════════════════════════

/*static*/
std::vector<std::regex> ModelRouter::compilePatterns(const RoutingRule& rule) {
    std::vector<std::regex> compiled;
    compiled.reserve(rule.prompt_patterns.size());
    for (const auto& pattern : rule.prompt_patterns) {
        try {
            compiled.emplace_back(pattern,
                std::regex_constants::ECMAScript | std::regex_constants::icase);
        } catch (const std::regex_error& e) {
            throw std::invalid_argument(
                "Invalid regex in routing rule '" + rule.id + "': " + e.what());
        }
    }
    return compiled;
}

/*static*/
bool ModelRouter::evaluate(const CompiledRule& cr,
                            const std::string& prompt,
                            const std::vector<std::string>& tags) {
    const RoutingRule& rule = cr.rule;
    const bool any_mode = (rule.match_mode == RoutingRule::MatchMode::ANY);

    // Helper: check prompt patterns
    auto matchesAnyPattern = [&]() -> bool {
        for (const auto& re : cr.compiled_patterns) {
            if (std::regex_search(prompt, re)) return true;
        }
        return false;
    };
    auto matchesAllPatterns = [&]() -> bool {
        for (const auto& re : cr.compiled_patterns) {
            if (!std::regex_search(prompt, re)) return false;
        }
        return true;
    };

    // Helper: check metadata tags
    auto matchesAnyTag = [&]() -> bool {
        for (const auto& required : rule.metadata_tags) {
            for (const auto& t : tags) {
                if (t == required) return true;
            }
        }
        return false;
    };
    auto matchesAllTags = [&]() -> bool {
        for (const auto& required : rule.metadata_tags) {
            bool found = false;
            for (const auto& t : tags) {
                if (t == required) { found = true; break; }
            }
            if (!found) return false;
        }
        return true;
    };

    const bool has_patterns = !cr.compiled_patterns.empty();
    const bool has_tags     = !rule.metadata_tags.empty();

    if (any_mode) {
        if (has_patterns && matchesAnyPattern()) return true;
        if (has_tags     && matchesAnyTag())     return true;
        return false;
    } else {
        // ALL mode: every non-empty criterion group must fully match
        if (has_patterns && !matchesAllPatterns()) return false;
        if (has_tags     && !matchesAllTags())     return false;
        return true;
    }
}

// ═══════════════════════════════════════════════════════════
// Public API
// ═══════════════════════════════════════════════════════════

void ModelRouter::addRule(const RoutingRule& rule) {
    if (rule.id.empty()) {
        throw std::invalid_argument("RoutingRule::id must not be empty");
    }
    if (rule.target_model_id.empty()) {
        throw std::invalid_argument("RoutingRule::target_model_id must not be empty");
    }
    if (rule.prompt_patterns.empty() && rule.metadata_tags.empty()) {
        throw std::invalid_argument(
            "RoutingRule '" + rule.id + "' must specify at least one prompt_pattern or metadata_tag");
    }

    // Compile before acquiring the lock (throws on invalid regex)
    std::vector<std::regex> compiled = compilePatterns(rule);

    std::lock_guard<std::mutex> lk(mutex_);

    // Replace existing rule with the same id
    for (auto& cr : rules_) {
        if (cr.rule.id == rule.id) {
            cr.rule = rule;
            cr.compiled_patterns = std::move(compiled);
            // Re-sort to account for possible priority change
            std::stable_sort(rules_.begin(), rules_.end(),
                [](const CompiledRule& a, const CompiledRule& b) {
                    return a.rule.priority > b.rule.priority;
                });
            spdlog::debug("ModelRouter: updated rule '{}' -> model '{}'",
                          rule.id, rule.target_model_id);
            return;
        }
    }

    // Insert new rule and keep sorted by priority (descending, stable)
    CompiledRule cr{ rule, std::move(compiled) };
    auto pos = std::lower_bound(rules_.begin(), rules_.end(), cr,
        [](const CompiledRule& a, const CompiledRule& b) {
            return a.rule.priority > b.rule.priority;
        });
    rules_.insert(pos, std::move(cr));

    spdlog::debug("ModelRouter: added rule '{}' (priority={}) -> model '{}'",
                  rule.id, rule.priority, rule.target_model_id);
}

bool ModelRouter::removeRule(const std::string& rule_id) {
    std::lock_guard<std::mutex> lk(mutex_);
    auto it = std::find_if(rules_.begin(), rules_.end(),
        [&](const CompiledRule& cr) { return cr.rule.id == rule_id; });
    if (it == rules_.end()) return false;
    rules_.erase(it);
    spdlog::debug("ModelRouter: removed rule '{}'", rule_id);
    return true;
}

std::vector<RoutingRule> ModelRouter::getRules() const {
    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<RoutingRule> out;
    out.reserve(rules_.size());
    for (const auto& cr : rules_) out.push_back(cr.rule);
    return out;
}

void ModelRouter::clearRules() {
    std::lock_guard<std::mutex> lk(mutex_);
    rules_.clear();
    spdlog::debug("ModelRouter: all rules cleared");
}

size_t ModelRouter::ruleCount() const {
    std::lock_guard<std::mutex> lk(mutex_);
    return rules_.size();
}

RoutingResult ModelRouter::route(const std::string& prompt,
                                  const nlohmann::json& metadata) const {
    // Extract tags from metadata["tags"] (array of strings)
    std::vector<std::string> tags;
    if (metadata.is_object() && metadata.contains("tags")) {
        const auto& jtags = metadata.at("tags");
        if (jtags.is_array()) {
            for (const auto& t : jtags) {
                if (t.is_string()) tags.push_back(t.get<std::string>());
            }
        }
    }

    std::lock_guard<std::mutex> lk(mutex_);
    for (const auto& cr : rules_) {
        if (evaluate(cr, prompt, tags)) {
            spdlog::debug("ModelRouter: rule '{}' matched -> model '{}'",
                          cr.rule.id, cr.rule.target_model_id);
            return { cr.rule.target_model_id, cr.rule.id, true };
        }
    }
    return {};  // no match
}

} // namespace llm
} // namespace themis
