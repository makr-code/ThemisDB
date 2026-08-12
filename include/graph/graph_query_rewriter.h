/**
 * @file graph_query_rewriter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.9
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace graph {

/**
 * @brief Statistics recorded by a single GraphQueryRewriter::rewrite() call.
 */
struct GraphRewriteStats {
    /// Number of distinct rewrite rules that fired at least once.
    size_t rules_applied = 0;

    /// Names of every rule that produced at least one transformation.
    std::vector<std::string> applied_rule_names;

    /// Total number of individual plan-node transformations performed.
    size_t total_transformations = 0;
};

/**
 * @brief Automatic rewriter that transforms graph query plans for better
 *        traversal performance.
 *
 * The rewriter operates on query plans encoded as JSON documents and applies
 * a configurable set of algebraic rewrite rules to lower execution cost.
 * All rules are stateless and produce deterministic results.
 *
 * Supported plan node types
 * ─────────────────────────
 * - `graph_traversal`  – graph BFS/DFS traversal from a start vertex
 * - `filter_scan`      – vertex pre-filter followed by a child traversal
 * - `traversal_join`   – nested-loop join between two traversal results
 * - `multi_traversal`  – fan-out traversal over multiple start vertices
 * - `let`              – variable binding (inserted by CSE rewrite)
 * - `ref`              – reference to a LET-bound sub-expression (CSE)
 *
 * Rewrite rules
 * ─────────────
 * PREDICATE_PUSHDOWN   Push vertex filters into traversal prune conditions so
 *                      branches are pruned as early as possible.
 * PRUNE_EARLY          Alias for PREDICATE_PUSHDOWN (both fire together).
 * COMMON_SUBEXPRESSION Detect identical traversal sub-expressions and replace
 *                      subsequent occurrences with cached-ref nodes.
 * JOIN_REORDERING      Swap left/right operands of a traversal_join when the
 *                      right side has lower expected cardinality.
 * MATERIALIZED_VIEW    Tag high-selectivity sub-graph traversals for
 *                      materialisation and replace them with mat-view refs.
 * QUERY_DECOMPOSITION  Split multi-start traversals into independent parallel
 *                      sub-queries for concurrent execution.
 *
 * Usage
 * ─────
 * @code
 *   GraphQueryRewriter rewriter;
 *   nlohmann::json plan = buildTraversalPlan(...);
 *   auto [rewritten, stats] = rewriter.rewrite(plan);
 *   std::cout << rewriter.explainRewrites(plan, rewritten) << "\n";
 * @endcode
 */
class GraphQueryRewriter {
public:
    // ─── Rewrite rule enumeration ────────────────────────────────────────────

    /**
     * @brief Individual rewrite rules that can be selectively enabled.
     *
     * When `RewriteConfig::enabled_rules` is empty the default set
     * (all rules) is applied.
     */
    enum class RewriteRule {
        PREDICATE_PUSHDOWN,   ///< Push filters into traversal prune conditions
        COMMON_SUBEXPRESSION, ///< Eliminate repeated identical sub-traversals
        JOIN_REORDERING,      ///< Reorder join operands by estimated selectivity
        MATERIALIZED_VIEW,    ///< Tag sub-graphs for materialisation
        QUERY_DECOMPOSITION,  ///< Decompose multi-start traversals for parallelism
        PRUNE_EARLY           ///< Alias – early branch pruning (= PREDICATE_PUSHDOWN)
    };

    // ─── Configuration ───────────────────────────────────────────────────────

    struct RewriteConfig {
        /**
         * Rules to apply.  When empty, all built-in rules fire.
         * Populate to restrict the rewriter to a specific subset.
         */
        std::set<RewriteRule> enabled_rules;

        /**
         * When true the rewriter applies more aggressive transformations
         * (e.g. materialises views even without access-frequency evidence).
         */
        bool aggressive_optimization = false;

        /**
         * Upper bound (ms) for the total rewrite phase.
         * Rewriting is abandoned after this budget is consumed and the
         * partial plan returned as-is.  0 = no limit.
         */
        double rewrite_time_limit_ms = 100.0;
    };

    // ─── Result type ─────────────────────────────────────────────────────────

    struct RewriteResult {
        nlohmann::json plan;    ///< Rewritten plan (may equal original if no rule fired)
        GraphRewriteStats stats; ///< Statistics about applied rules
    };

    // ─── Construction ────────────────────────────────────────────────────────

    /**
     * Constructs a rewriter with the given configuration.
     * @param config  Rewrite configuration; defaults to all rules enabled.
     */
    explicit GraphQueryRewriter(const std::optional<RewriteConfig>& config = std::nullopt);

    // ─── Core API ────────────────────────────────────────────────────────────

    /**
     * @brief Rewrite a graph query plan for improved traversal performance.
     *
     * Applies all enabled rules to a fixed point (up to 5 iterations) and
     * returns the rewritten plan together with statistics about what changed.
     *
     * @param plan  JSON-encoded graph query plan.
     * @return RewriteResult with the transformed plan and stats.
     */
    RewriteResult rewrite(const nlohmann::json& plan) const;

    /**
     * @brief Produce a human-readable summary of the transformations applied.
     *
     * Compares the original and rewritten plans and returns a multi-line
     * string describing each rule that fired and what it changed.
     *
     * @param original  Plan before rewriting.
     * @param rewritten Plan after rewriting.
     * @return Description of applied transformations.
     */
    std::string explainRewrites(const nlohmann::json& original,
                                const nlohmann::json& rewritten) const;

    /**
     * @brief Estimate the expected query speedup ratio for the rewritten plan.
     *
     * Returns a value > 1.0 when the rewrite is expected to be faster, 1.0
     * when no improvement is estimated, and < 1.0 (unlikely) when the rewrite
     * may add overhead.  The estimate is heuristic and based on the types of
     * rewrites applied.
     *
     * @param original  Original query plan.
     * @param rewritten Rewritten query plan.
     * @return Estimated speedup factor (e.g. 2.5 = 2.5× faster).
     */
    double estimateSpeedup(const nlohmann::json& original,
                           const nlohmann::json& rewritten) const;

    /**
     * @brief Register a custom rewrite rule.
     *
     * Custom rules are applied after all built-in rules in each iteration.
     * They receive the current plan and must return the (optionally modified)
     * plan and the number of transformations performed.
     *
     * @param name  Unique human-readable rule name.
     * @param rule  Function that transforms the plan in-place; returns number
     *              of node transformations made (0 = no change).
     */
    void addCustomRule(std::string_view name,
                       std::function<size_t(nlohmann::json&)> rule);

    /**
     * @brief Remove all registered custom rules.
     */
    void clearCustomRules();

    /**
     * @brief Return the active rewrite configuration.
     */
    const RewriteConfig& config() const { return config_; }

    // ─── Static helpers ──────────────────────────────────────────────────────

    /**
     * @brief Construct a graph traversal plan node.
     *
     * Convenience factory used by tests and by the AQL translator.
     *
     * @param graph_id     Graph name / identifier.
     * @param start_vertex Starting vertex key.
     * @param direction    Traversal direction: "OUTBOUND", "INBOUND", or "ANY".
     * @param min_depth    Minimum traversal depth (inclusive).
     * @param max_depth    Maximum traversal depth (inclusive).
     * @param vertex_filters  Array of filter conditions applied to visited vertices.
     * @return JSON plan node with type == "graph_traversal".
     */
    static nlohmann::json makeTraversalPlan(
        std::string_view graph_id,
        std::string_view start_vertex,
        std::string_view direction = "OUTBOUND",
        int min_depth = 1,
        int max_depth = 1,
        nlohmann::json vertex_filters = nlohmann::json::array());

    /**
     * @brief Construct a filter_scan plan node (pre-filter before traversal).
     *
     * @param filter   JSON condition object, e.g.
     *                 `{"field":"type","op":"eq","value":"Person"}`.
     * @param child    Child traversal plan.
     * @return JSON plan node with type == "filter_scan".
     */
    static nlohmann::json makeFilterScanPlan(nlohmann::json filter,
                                             nlohmann::json child);

    /**
     * @brief Construct a traversal_join plan node.
     *
     * @param left  Left-hand traversal plan.
     * @param right Right-hand traversal plan.
     * @param join_key Field used to join traversal results (default: "vertex_id").
     * @return JSON plan node with type == "traversal_join".
     */
    static nlohmann::json makeJoinPlan(nlohmann::json left,
                                       nlohmann::json right,
                                       std::string_view join_key = "vertex_id");

    /**
     * @brief Construct a multi_traversal plan node (fan-out over multiple starts).
     *
     * @param graph_id      Graph name.
     * @param start_vertices List of start vertex keys.
     * @param direction     Traversal direction.
     * @param max_depth     Maximum traversal depth.
     * @param vertex_filters Filter conditions applied to visited vertices.
     * @return JSON plan node with type == "multi_traversal".
     */
    static nlohmann::json makeMultiTraversalPlan(
        std::string_view graph_id,
        const std::vector<std::string>& start_vertices,
        std::string_view direction = "OUTBOUND",
        int max_depth = 1,
        nlohmann::json vertex_filters = nlohmann::json::array());

    /**
     * @brief Estimate the cardinality of a traversal plan node.
     *
     * Lower value = smaller expected result set = more selective.
     * Used by the JOIN_REORDERING rule to determine operand order.
     *
     * @param node  JSON plan node to estimate.
     * @return Heuristic cardinality estimate (higher = more rows).
     */
    static double estimateCardinality(const nlohmann::json& node);

private:
    RewriteConfig config_;

    struct CustomRule {
        std::string name;
        std::function<size_t(nlohmann::json&)> fn;
    };
    std::vector<CustomRule> custom_rules_;

    // Maximum fixed-point iterations to avoid infinite loops.
    static constexpr size_t kMaxIterations = 5;

    // ─── Individual rule implementations ─────────────────────────────────

    static size_t applyPredicatePushdown(nlohmann::json& plan);
    static size_t applyCommonSubexpressionElimination(nlohmann::json& plan);
    static size_t applyJoinReordering(nlohmann::json& plan);
    static size_t applyMaterializedView(nlohmann::json& plan, bool aggressive);
    static size_t applyQueryDecomposition(nlohmann::json& plan);

    bool isEnabled(RewriteRule rule) const;
};

} // namespace graph
} // namespace themis
