/**
 * @file rewrite_rule_base.cpp
 * @brief Implementation of concrete rewrite rule types (Phase 2 delivery).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 rule implementations (Q4 2026)
 *
 * Concrete implementations of:
 * - RegexRewriteRule: lexical pattern-based rewrites
 * - DictionaryRewriteRule: synonym/alias mapping
 * - PolicyRewriteRule: policy enforcement and terminal blocking
 * - SemanticRewriteRule: base for semantic rule implementations
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "prompt_engineering/rewrite_rule.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace themis {
namespace prompt_engineering {

// ============================================================================
// RegexRewriteRule Implementation
// ============================================================================

RegexRewriteRule::RegexRewriteRule(
    const std::string& rule_id,
    uint8_t priority,
    RewritePhase phase,
    const std::string& pattern,
    const std::string& replacement,
    const std::string& description,
    uint32_t max_replacements
)
    : rule_id_(rule_id),
      priority_(priority),
      phase_(phase),
      pattern_string_(pattern),
      replacement_(replacement),
      description_(description),
      max_replacements_(max_replacements) {
    try {
        // Precompile regex for efficiency
        pattern_ = std::regex(pattern_string_);
    } catch (const std::regex_error& e) {
        auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");
        logger->error("Failed to compile regex pattern for rule {}: {}", rule_id_, e.what());
        pattern_ = std::regex("(?!)");  // Pattern that never matches
    }
}

std::string RegexRewriteRule::rule_id() const {
    return rule_id_;
}

RewriteRuleType RegexRewriteRule::rule_type() const {
    return RewriteRuleType::LEXICAL;
}

RewritePhase RegexRewriteRule::execution_phase() const {
    return phase_;
}

uint8_t RegexRewriteRule::priority() const {
    return priority_;
}

bool RegexRewriteRule::matches(const RewriteDocument& doc, const RewriteContext& ctx) const {
    return std::regex_search(doc.content, pattern_);
}

RewriteResult RegexRewriteRule::apply(
    RewriteDocument& doc,
    const RewriteContext& ctx,
    RewriteTrace& trace
) {
    auto logger = spdlog::get("prompt_engineering") ?: spdlog::stderr_color_mt("prompt_engineering");

    RewriteResult result;
    result.success = true;
    result.error_code = PromptEngineeringErrorCode::TEMPLATE_INVALID_ID;
    result.was_blocked = false;
    result.transformed_text = doc.content;

    try {
        std::string transformed = doc.content;
        uint32_t replacements = 0;
        const uint32_t limit = (max_replacements_ == 0) ? UINT32_MAX : max_replacements_;

        // Apply replacements with limit
        std::smatch match;
        std::string::const_iterator search_start(transformed.cbegin());

        while (std::regex_search(search_start, transformed.cend(), match, pattern_) &&
               replacements < limit) {
            trace.match_count++;

            if (trace.matched_text.empty() && match.size() > 0) {
                // Record first match (up to 1024 bytes)
                std::string matched = match[0].str();
                trace.matched_text = matched.substr(0, 1024);
                trace.text_offset = std::distance(transformed.cbegin(), match[0].first);
            }

            std::string replacement_result = std::regex_replace(match[0].str(), pattern_, replacement_);
            if (replacement_result != match[0].str()) {
                // Update document and continue search
                transformed = transformed.substr(0, std::distance(transformed.cbegin(), match[0].first))
                            + replacement_result
                            + transformed.substr(std::distance(transformed.cbegin(), match[0].second));
                replacements++;
            }

            // Move search position past this match
            size_t next_pos = std::distance(transformed.cbegin(), match[0].first) + replacement_result.length();
            search_start = transformed.cbegin() + next_pos;
        }

        if (replacements > 0) {
            trace.replacement_text = transformed.substr(
                std::min(trace.text_offset, (uint64_t)(transformed.length() - 1)),
                std::min((size_t)1024, transformed.length())
            );
            trace.transformation_applied = true;
            doc.content = transformed;
            result.transformed_text = transformed;
        }

    } catch (const std::exception& e) {
        result.success = false;
        result.error_code = PromptEngineeringErrorCode::REWRITE_REGEX_PATHOLOGICAL;
        result.error_message = std::string("Regex transformation failed: ") + e.what();
        logger->error("Regex rule {} failed: {}", rule_id_, result.error_message);
    }

    return result;
}

bool RegexRewriteRule::is_idempotent() const {
    // Regex replacement is idempotent if:
    // - The replacement string doesn't contain the pattern
    // - Or max_replacements == 1
    return max_replacements_ == 1;
}

std::string RegexRewriteRule::description() const {
    return description_;
}

// ============================================================================
// DictionaryRewriteRule Implementation
// ============================================================================

DictionaryRewriteRule::DictionaryRewriteRule(
    const std::string& rule_id,
    uint8_t priority,
    RewritePhase phase,
    const std::unordered_map<std::string, std::string>& mappings,
    const std::string& description,
    bool case_sensitive,
    uint32_t max_replacements
)
    : rule_id_(rule_id),
      priority_(priority),
      phase_(phase),
      mappings_(mappings),
      description_(description),
      case_sensitive_(case_sensitive),
      max_replacements_(max_replacements) {
}

std::string DictionaryRewriteRule::rule_id() const {
    return rule_id_;
}

RewriteRuleType DictionaryRewriteRule::rule_type() const {
    return RewriteRuleType::LEXICAL;
}

RewritePhase DictionaryRewriteRule::execution_phase() const {
    return phase_;
}

uint8_t DictionaryRewriteRule::priority() const {
    return priority_;
}

bool DictionaryRewriteRule::matches(const RewriteDocument& doc, const RewriteContext& ctx) const {
    std::string content = case_sensitive_ ? doc.content : doc.content;

    for (const auto& [source, _] : mappings_) {
        std::string search_term = case_sensitive_ ? source : source;
        if (doc.content.find(source) != std::string::npos) {
            return true;
        }
    }

    return false;
}

RewriteResult DictionaryRewriteRule::apply(
    RewriteDocument& doc,
    const RewriteContext& ctx,
    RewriteTrace& trace
) {
    RewriteResult result;
    result.success = true;
    result.error_code = PromptEngineeringErrorCode::TEMPLATE_INVALID_ID;
    result.was_blocked = false;
    result.transformed_text = doc.content;

    std::string transformed = doc.content;
    uint32_t replacements = 0;
    const uint32_t limit = (max_replacements_ == 0) ? UINT32_MAX : max_replacements_;

    for (const auto& [source, target] : mappings_) {
        if (replacements >= limit) {
          break;
        }

        size_t pos = 0;
        while ((pos = transformed.find(source, pos)) != std::string::npos) {
            if (replacements >= limit) {
              break;
            }

            if (trace.match_count == 0) {
                trace.matched_text = source.substr(0, 1024);
                trace.text_offset = pos;
            }

            transformed.replace(pos, source.length(), target);
            replacements++;
            pos += target.length();
            trace.match_count++;
        }
    }

    if (replacements > 0) {
        trace.replacement_text = transformed.substr(
            std::min(trace.text_offset, (uint64_t)(transformed.length() - 1)),
            std::min((size_t)1024, transformed.length())
        );
        trace.transformation_applied = true;
        doc.content = transformed;
        result.transformed_text = transformed;
    }

    return result;
}

bool DictionaryRewriteRule::is_idempotent() const {
    // Dictionary substitution is idempotent only if replacements don't create new matches
    return max_replacements_ == 1;
}

std::string DictionaryRewriteRule::description() const {
    return description_;
}

// ============================================================================
// PolicyRewriteRule Implementation
// ============================================================================

PolicyRewriteRule::PolicyRewriteRule(
    const std::string& rule_id,
    uint8_t priority,
    RewritePhase phase,
    MatchFunction match_fn,
    ApplyFunction apply_fn,
    const std::string& description,
    bool is_terminal
)
    : rule_id_(rule_id),
      priority_(priority),
      phase_(phase),
      match_fn_(match_fn),
      apply_fn_(apply_fn),
      description_(description),
      is_terminal_(is_terminal) {
}

std::string PolicyRewriteRule::rule_id() const {
    return rule_id_;
}

RewriteRuleType PolicyRewriteRule::rule_type() const {
    return is_terminal_ ? RewriteRuleType::POLICY_TERMINAL : RewriteRuleType::POLICY_ALLOW_LIST;
}

RewritePhase PolicyRewriteRule::execution_phase() const {
    return phase_;
}

uint8_t PolicyRewriteRule::priority() const {
    return priority_;
}

bool PolicyRewriteRule::matches(const RewriteDocument& doc, const RewriteContext& ctx) const {
    if (!match_fn_) {
      return false;
    }
    return match_fn_(doc, ctx);
}

RewriteResult PolicyRewriteRule::apply(
    RewriteDocument& doc,
    const RewriteContext& ctx,
    RewriteTrace& trace
) {
    trace.rule_id = rule_id_;
    trace.phase = phase_;

    RewriteResult result = apply_fn_(doc, ctx);

    if (is_terminal_) {
        result.was_blocked = true;
    }

    return result;
}

bool PolicyRewriteRule::is_idempotent() const {
    return false;
}

std::string PolicyRewriteRule::description() const {
    return description_;
}

// ============================================================================
// SemanticRewriteRule Implementation
// ============================================================================

std::string SemanticRewriteRule::rule_id() const {
    return get_rule_id();
}

RewriteRuleType SemanticRewriteRule::rule_type() const {
    return RewriteRuleType::SEMANTIC;
}

RewritePhase SemanticRewriteRule::execution_phase() const {
    return get_phase();
}

uint8_t SemanticRewriteRule::priority() const {
    return get_priority();
}

bool SemanticRewriteRule::matches(const RewriteDocument& doc, const RewriteContext& ctx) const {
    return match_impl(doc, ctx);
}

RewriteResult SemanticRewriteRule::apply(
    RewriteDocument& doc,
    const RewriteContext& ctx,
    RewriteTrace& trace
) {
    return apply_impl(doc, ctx, trace);
}

bool SemanticRewriteRule::is_idempotent() const {
    return get_is_idempotent();
}

std::string SemanticRewriteRule::description() const {
    return get_description();
}

} // namespace prompt_engineering
} // namespace themis
