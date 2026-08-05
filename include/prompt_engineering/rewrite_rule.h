/**
 * @file rewrite_rule.h
 * @brief Base interfaces for rewrite rule implementations (Phase 2 design).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 rule infrastructure (Q4 2026)
 *
 * This header defines the concrete base classes and rule implementations
 * that inherit from IRewriteRule (defined in rewrite_engine.h).
 *
 * Supported rule types:
 * - RegexRewriteRule: YAML-loadable regex-based lexical rules
 * - DictionaryRewriteRule: YAML-loadable synonym/alias mapping
 * - PolicyRewriteRule: C++-only terminal/allow-list policy enforcement
 * - SemanticRewriteRule: C++-only semantic transformation base
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "rewrite_engine.h"
#include <string>
#include <vector>
#include <memory>
#include <regex>
#include <unordered_map>

namespace themis {
namespace prompt_engineering {

/**
 * @class RegexRewriteRule
 * @brief YAML-loadable regex-based lexical rewrite rule.
 *
 * Applies a single regex pattern-to-replacement transformation.
 * Thread-safe after construction. Regex is precompiled for efficiency.
 */
class RegexRewriteRule : public IRewriteRule {
public:
    /**
     * @brief Create a regex rewrite rule.
     *
     * @param rule_id Unique rule identifier
     * @param priority Priority within phase (lower = earlier)
     * @param phase Execution phase
     * @param pattern Regex pattern to match
     * @param replacement Replacement string (may include $1, $2 for capture groups)
     * @param description Human-readable description
     * @param max_replacements Maximum number of replacements per document (0 = unlimited)
     */
    RegexRewriteRule(
        const std::string& rule_id,
        uint8_t priority,
        RewritePhase phase,
        const std::string& pattern,
        const std::string& replacement,
        const std::string& description,
        uint32_t max_replacements = 0
    );

    std::string rule_id() const override;
    RewriteRuleType rule_type() const override;
    RewritePhase execution_phase() const override;
    uint8_t priority() const override;
    bool matches(const RewriteDocument& doc, const RewriteContext& ctx) const override;
    RewriteResult apply(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) override;
    bool is_idempotent() const override;
    std::string description() const override;

private:
    std::string rule_id_;
    uint8_t priority_;
    RewritePhase phase_;
    std::string pattern_string_;
    std::regex pattern_;
    std::string replacement_;
    std::string description_;
    uint32_t max_replacements_;
};

/**
 * @class DictionaryRewriteRule
 * @brief YAML-loadable dictionary-based substitution rule.
 *
 * Maps exact terms or phrases to canonical replacements.
 * Thread-safe after construction. Supports case-sensitive or case-insensitive matching.
 */
class DictionaryRewriteRule : public IRewriteRule {
public:
    /**
     * @brief Create a dictionary rewrite rule.
     *
     * @param rule_id Unique rule identifier
     * @param priority Priority within phase (lower = earlier)
     * @param phase Execution phase
     * @param mappings Dictionary mapping source → replacement (e.g., "kunden" → "customers")
     * @param description Human-readable description
     * @param case_sensitive Whether matching is case-sensitive (default: false)
     * @param max_replacements Maximum number of replacements per document (0 = unlimited)
     */
    DictionaryRewriteRule(
        const std::string& rule_id,
        uint8_t priority,
        RewritePhase phase,
        const std::unordered_map<std::string, std::string>& mappings,
        const std::string& description,
        bool case_sensitive = false,
        uint32_t max_replacements = 0
    );

    std::string rule_id() const override;
    RewriteRuleType rule_type() const override;
    RewritePhase execution_phase() const override;
    uint8_t priority() const override;
    bool matches(const RewriteDocument& doc, const RewriteContext& ctx) const override;
    RewriteResult apply(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) override;
    bool is_idempotent() const override;
    std::string description() const override;

private:
    std::string rule_id_;
    uint8_t priority_;
    RewritePhase phase_;
    std::unordered_map<std::string, std::string> mappings_;
    std::string description_;
    bool case_sensitive_;
    uint32_t max_replacements_;
};

/**
 * @class PolicyRewriteRule
 * @brief C++-only policy enforcement rule (terminal or allow-list).
 *
 * Used for security/policy decision-making. Can be terminal (blocks further processing)
 * or allow-list only (only matching content is permitted).
 * Not loadable from YAML.
 */
class PolicyRewriteRule : public IRewriteRule {
public:
    /**
     * @brief Callback function for custom matching logic.
     *
     * @param doc Document to match against
     * @param ctx Execution context
     * @return true if policy matches
     */
    using MatchFunction = std::function<bool(const RewriteDocument&, const RewriteContext&)>;

    /**
     * @brief Callback function for custom transformation logic.
     *
     * @param doc Document to transform (may be modified)
     * @param ctx Execution context
     * @return RewriteResult with transformation outcome
     */
    using ApplyFunction = std::function<RewriteResult(RewriteDocument&, const RewriteContext&)>;

    /**
     * @brief Create a policy rewrite rule.
     *
     * @param rule_id Unique rule identifier
     * @param priority Priority within phase (lower = earlier)
     * @param phase Execution phase
     * @param match_fn Custom matching function
     * @param apply_fn Custom transformation function
     * @param description Human-readable description
     * @param is_terminal Whether this rule is terminal (blocks further processing)
     */
    PolicyRewriteRule(
        const std::string& rule_id,
        uint8_t priority,
        RewritePhase phase,
        MatchFunction match_fn,
        ApplyFunction apply_fn,
        const std::string& description,
        bool is_terminal = false
    );

    std::string rule_id() const override;
    RewriteRuleType rule_type() const override;
    RewritePhase execution_phase() const override;
    uint8_t priority() const override;
    bool matches(const RewriteDocument& doc, const RewriteContext& ctx) const override;
    RewriteResult apply(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) override;
    bool is_idempotent() const override;
    std::string description() const override;

private:
    std::string rule_id_;
    uint8_t priority_;
    RewritePhase phase_;
    MatchFunction match_fn_;
    ApplyFunction apply_fn_;
    std::string description_;
    bool is_terminal_;
};

/**
 * @class SemanticRewriteRule
 * @brief C++-only base class for semantic/advanced rewrite rules.
 *
 * Useful for rules that require language understanding, context awareness,
 * or multiple attribute modifications.
 * Not loadable from YAML.
 */
class SemanticRewriteRule : public IRewriteRule {
public:
    virtual ~SemanticRewriteRule() = default;

    std::string rule_id() const override;
    RewriteRuleType rule_type() const override;
    RewritePhase execution_phase() const override;
    uint8_t priority() const override;
    bool matches(const RewriteDocument& doc, const RewriteContext& ctx) const override;
    RewriteResult apply(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) override;
    bool is_idempotent() const override;
    std::string description() const override;

    /**
     * @brief Subclasses override this to provide custom rule ID.
     */
    virtual std::string get_rule_id() const = 0;

    /**
     * @brief Subclasses override this to provide custom priority.
     */
    virtual uint8_t get_priority() const = 0;

    /**
     * @brief Subclasses override this to provide custom phase.
     */
    virtual RewritePhase get_phase() const = 0;

    /**
     * @brief Subclasses override this for custom matching logic.
     */
    virtual bool match_impl(const RewriteDocument& doc, const RewriteContext& ctx) const = 0;

    /**
     * @brief Subclasses override this for custom transformation logic.
     */
    virtual RewriteResult apply_impl(
        RewriteDocument& doc,
        const RewriteContext& ctx,
        RewriteTrace& trace
    ) = 0;

    /**
     * @brief Subclasses override this for idempotence declaration.
     * Default: false (not idempotent).
     */
    virtual bool get_is_idempotent() const { return false; }

    /**
     * @brief Subclasses override this for description.
     */
    virtual std::string get_description() const = 0;

protected:
    SemanticRewriteRule() = default;
};

} // namespace prompt_engineering
} // namespace themis
