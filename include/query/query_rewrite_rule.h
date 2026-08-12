/**
 * @file query_rewrite_rule.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace query {

/**
 * @brief Context passed to each rewrite rule during query optimization.
 *
 * Carries statistics and configuration that rules use to decide whether
 * and how to transform a query plan represented as a JSON document.
 */
struct RewriteContext {
    /// Estimated row counts per collection; used for selectivity decisions.
    std::unordered_map<std::string, size_t> collection_row_counts;

    /// Maximum number of values an OR chain may have before being rewritten
    /// to an IN expression (default: 3).
    size_t or_to_in_threshold = 3;

    /// If true, constant sub-expressions are evaluated at plan time.
    bool enable_constant_folding = true;
};

/**
 * @brief Statistics accumulated by a QueryRewritePipeline run.
 */
struct RewriteStats {
    /// Number of rules that reported they were applicable.
    size_t rules_applied = 0;

    /// Names of the rules that fired, in application order.
    std::vector<std::string> applied_rule_names;

    /// Total number of plan node transformations across all rules.
    size_t total_transformations = 0;
};

/**
 * @brief Abstract interface for a single algebraic query-rewrite rule.
 *
 * Implementations inspect a query plan (represented as a mutable JSON
 * document) and return a transformed copy if the rule is applicable.
 * Rules are stateless and must be safe to apply concurrently.
 */
class IQueryRewriteRule {
public:
    virtual ~IQueryRewriteRule() = default;

    /// Human-readable identifier, e.g. "PredicatePushdown".
    [[nodiscard]] virtual std::string name() const = 0;

    /// Returns true when the rule can improve the given plan.
    [[nodiscard]] virtual bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const = 0;

    /**
     * @brief Apply the rule to a plan.
     * @param plan Input query plan; modified in-place.
     * @param ctx  Rewrite context with statistics.
     * @return Number of transformations performed (0 if none).
     */
    [[nodiscard]] virtual size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Concrete rules
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Push FILTER nodes closer to their data source.
 *
 * Moves filter conditions from a higher plan node to a lower one when
 * the filter references only columns available at the lower node, reducing
 * the number of rows that flow upward through the plan.
 *
 * Impact: 10–50 % query speedup for selective filters on large collections.
 */
class PredicatePushdownRule : public IQueryRewriteRule {
public:
    ~PredicatePushdownRule() override = default;
    std::string name() const override { return "PredicatePushdown"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

/**
 * @brief Push PROJECT (column selection) nodes as early as possible.
 *
 * Eliminates columns that are not needed by higher plan nodes before
 * data flows upward, reducing memory pressure and I/O.
 */
class ProjectionPushdownRule : public IQueryRewriteRule {
public:
    ~ProjectionPushdownRule() override = default;
    std::string name() const override { return "ProjectionPushdown"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

/**
 * @brief Rewrite OR chains over the same field to IN expressions.
 *
 * Transforms:
 *   FILTER x == 'A' OR x == 'B' OR x == 'C'
 * into:
 *   FILTER x IN ['A', 'B', 'C']
 *
 * The rewrite is applied when the OR chain length exceeds
 * `RewriteContext::or_to_in_threshold` (default 3).
 *
 * Benefit: enables bitmap-index scans and avoids repeated index lookups.
 */
class OrToInRewriteRule : public IQueryRewriteRule {
public:
    ~OrToInRewriteRule() override = default;
    std::string name() const override { return "OrToIn"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

/**
 * @brief Evaluate constant sub-expressions at plan time.
 *
 * Folds expressions whose operands are all literals into their result,
 * e.g. `2 + 3` → `5`, `LENGTH("hello") > 3` → `true`.
 */
class ConstantFoldingRule : public IQueryRewriteRule {
public:
    ~ConstantFoldingRule() override = default;
    std::string name() const override { return "ConstantFolding"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

/**
 * @brief Eliminate common sub-expressions (CSE).
 *
 * Detects identical sub-expressions computed more than once in the plan
 * and replaces duplicates with references to a single computed result,
 * reducing redundant work.
 */
class CommonSubexpressionRule : public IQueryRewriteRule {
public:
    ~CommonSubexpressionRule() override = default;
    std::string name() const override { return "CommonSubexpressionElimination"; }
    bool applies(const nlohmann::json& plan, const RewriteContext& ctx) const override;
    size_t apply(nlohmann::json& plan, const RewriteContext& ctx) const override;
};

// ─────────────────────────────────────────────────────────────────────────────
// Pipeline
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Chains multiple IQueryRewriteRule instances into a single pass.
 *
 * Rules are applied in registration order. After all rules have been
 * applied once, the pipeline iterates until no rule reports a change
 * (up to `max_iterations`), ensuring the plan reaches a fixed point.
 *
 * Usage:
 * @code
 *   QueryRewritePipeline pipeline;
 *   pipeline.addRule(std::make_shared<PredicatePushdownRule>());
 *   pipeline.addRule(std::make_shared<OrToInRewriteRule>());
 *
 *   RewriteContext ctx;
 *   ctx.collection_row_counts["users"] = 1'000'000;
 *
 *   auto [rewritten_plan, stats] = pipeline.run(plan, ctx);
 * @endcode
 */
class QueryRewritePipeline {
public:
    /// Maximum fixed-point iterations (prevents infinite loops on cyclic rules).
    static constexpr size_t kDefaultMaxIterations = 5;

    explicit QueryRewritePipeline(size_t max_iterations = kDefaultMaxIterations);

    /// Append a rule to the pipeline.
    void addRule(std::shared_ptr<IQueryRewriteRule> rule);

    /// Remove all registered rules.
    void clearRules();

    /// Number of registered rules.
    size_t ruleCount() const;

    /**
     * @brief Run all rules to a fixed point.
     * @param plan Input query plan (JSON); modified in-place.
     * @param ctx  Rewrite context.
     * @return Accumulated statistics across all rule applications.
     */
    RewriteStats run(nlohmann::json& plan, const RewriteContext& ctx) const;

    /**
     * @brief Convenience: create a pipeline with all built-in rules.
     */
    static QueryRewritePipeline createDefault();

private:
    std::vector<std::shared_ptr<IQueryRewriteRule>> rules_;
    size_t max_iterations_{kDefaultMaxIterations};
};

} // namespace query
} // namespace themis
