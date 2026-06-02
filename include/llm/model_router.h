/*
 * ThemisDB | File: model_router.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 173
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #3269 feat(llm): multi-model rout... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <mutex>
#include <nlohmann/json.hpp>

/**
 * @file model_router.h
 * @brief Content-based and metadata-tag-based multi-model routing for the LLM module.
 *
 * `ModelRouter` evaluates an ordered list of `RoutingRule` entries against a
 * prompt string and a JSON metadata object.  The first matching rule's
 * `target_model_id` is returned.  Rules are sorted by priority (descending)
 * and then insertion order for stability.
 *
 * Integration point:
 *   `InferenceEngineEnhanced::selectModel()` calls `ModelRouter::route()` before
 *   falling back to load-balancing strategies.
 *
 * Routing criteria (both are optional per rule; at least one must be non-empty):
 * - **prompt_patterns**  – ECMAScript-syntax regex strings matched against the
 *   prompt text (case-insensitive by default).
 * - **metadata_tags**    – String values matched against the `tags` array inside
 *   `InferenceRequest::metadata` (exact, case-sensitive).
 *
 * Match semantics are controlled by `MatchMode`:
 * - `ANY`  – rule matches when *at least one* pattern/tag matches.
 * - `ALL`  – rule matches only when *every* pattern and every required tag matches.
 */

namespace themis {
namespace llm {

/**
 * @brief Single routing rule that maps a set of match criteria to a model.
 */
struct RoutingRule {
    virtual ~RoutingRule() = default;
    /// Unique identifier for the rule (used by removeRule()).
    std::string id;

    /// Higher priority rules are evaluated first.  Rules with equal priority
    /// are evaluated in insertion order.  Negative values are allowed.
    int priority = 0;

    /// Model to route to when the rule matches.
    std::string target_model_id;

    /// ECMAScript-syntax regex patterns matched against the prompt (case-insensitive).
    /// An empty vector means "do not match on prompt content".
    std::vector<std::string> prompt_patterns;

    /// Tag values that must appear in `InferenceRequest::metadata["tags"]`.
    /// An empty vector means "do not match on metadata tags".
    std::vector<std::string> metadata_tags;

    /// Controls how multiple criteria within the rule are combined.
    enum class MatchMode {
        ANY, ///< Rule matches when at least one pattern or tag matches.
        ALL  ///< Rule matches only when every pattern and every tag matches.
    };
    MatchMode match_mode = MatchMode::ANY;

    /// Human-readable description for logging and diagnostics.
    std::string description;
};

/**
 * @brief Result returned by `ModelRouter::route()`.
 */
struct RoutingResult {
    /// Model ID selected by the matching rule.  Empty when no rule matched.
    std::string model_id;
    /// ID of the rule that produced this result.
    std::string rule_id;
    /// True when a rule matched; false when routing fell through.
    bool matched = false;
};

/**
 * @brief Thread-safe multi-model router based on prompt content and metadata tags.
 *
 * Rules are stored in a priority-sorted list.  Concurrent calls to `route()` are
 * safe via a shared reader lock; rule mutations acquire an exclusive lock.
 */
class ModelRouter {
public:
    ModelRouter() = default;

    /**
     * @brief Add or replace a routing rule.
     *
     * If a rule with the same `id` already exists it is replaced in-place
     * (priority re-sort is applied).
     *
     * @param rule  Rule to add.
     * @throws std::invalid_argument if `rule.id` or `rule.target_model_id` is empty,
     *         or if neither `prompt_patterns` nor `metadata_tags` is non-empty.
     */
    void addRule(const RoutingRule& rule);

    /**
     * @brief Remove a routing rule by its ID.
     *
     * @param rule_id  ID of the rule to remove.
     * @return true if the rule existed and was removed; false otherwise.
     */
    bool removeRule(const std::string& rule_id);

    /**
     * @brief Return a snapshot of all registered rules in priority order.
     */
    std::vector<RoutingRule> getRules() const;

    /**
     * @brief Remove all registered rules.
     */
    void clearRules();

    /**
     * @brief Evaluate routing rules for the given prompt and metadata.
     *
     * Rules are tested in descending priority order (then insertion order).
     * The first matching rule wins.
     *
     * @param prompt    The inference prompt text.
     * @param metadata  JSON metadata from `InferenceRequest::metadata`.
     *                  The router inspects the `"tags"` key when present
     *                  (expected type: JSON array of strings).
     * @return `RoutingResult` with `matched == false` when no rule fires.
     */
    RoutingResult route(const std::string& prompt,
                        const nlohmann::json& metadata) const;

    /**
     * @brief Return the number of registered rules.
     */
    size_t ruleCount() const;

private:
    struct CompiledRule {
        RoutingRule             rule;
        std::vector<std::regex> compiled_patterns;
    };

    /// Compile regex patterns from a RoutingRule; throws std::invalid_argument
    /// on invalid regex syntax.
    static std::vector<std::regex> compilePatterns(const RoutingRule& rule);

    /// Evaluate a single compiled rule.
    static bool evaluate(const CompiledRule& cr,
                         const std::string& prompt,
                         const std::vector<std::string>& tags);

    mutable std::mutex mutex_;
    /// Stored in descending priority order (stable; index breaks ties).
    std::vector<CompiledRule> rules_;
};

} // namespace llm
} // namespace themis
