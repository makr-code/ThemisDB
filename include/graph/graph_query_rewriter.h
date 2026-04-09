/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_query_rewriter.h                             ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     310                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • (initial)  2026-04-09  feat(graph): add query rewriting interface ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "graph/graph_query_optimizer.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <optional>

namespace themis {
namespace graph {

/**
 * @brief Rewrite rules applied by the GraphQueryRewriter.
 *
 * Each rule transforms the logical query representation before the cost-based
 * optimizer (GraphQueryOptimizer) selects an execution algorithm.
 */
enum class RewriteRule {
    /// Push node/edge predicates into the traversal frontier to prune early.
    PREDICATE_PUSHDOWN,
    /// Eliminate sub-expressions that appear more than once in the same query.
    COMMON_SUBEXPRESSION_ELIMINATION,
    /// Reorder multi-pattern joins by estimated selectivity (lowest first).
    JOIN_REORDERING,
    /// Substitute frequently-computed subgraphs with precomputed views.
    MATERIALIZED_VIEW_UTILIZATION,
    /// Split independent query parts so they can run in parallel.
    QUERY_DECOMPOSITION,
    /// Push edge-type filter constraints as early as possible in the plan.
    EDGE_TYPE_FILTER_PUSHDOWN
};

/**
 * @brief Lightweight representation of a graph traversal query used as input
 *        to the rewriter.
 *
 * Mirrors the subset of GraphQueryOptimizer::QueryConstraints that is relevant
 * for logical rewriting.  After rewriting the optimised fields are fed back to
 * QueryConstraints before the cost-based optimizer runs.
 */
struct GraphQuery {
    /// Starting vertex primary key.
    std::string start_vertex;
    /// Target vertex primary key (empty = any).
    std::string end_vertex;
    /// Minimum traversal depth (inclusive). Must be >= 0.
    int min_depth{1};
    /// Maximum traversal depth (inclusive). Must be >= min_depth.
    int max_depth{10};
    /// Edge type filter (empty = accept all types).
    std::string edge_type;
    /// Node predicates in "field=value" format (AND-combined during traversal).
    std::vector<std::string> node_filters;
    /// Edge predicates in "field=value" format (AND-combined during traversal).
    std::vector<std::string> edge_filters;
    /// Optional graph_id to scope the query.
    std::string graph_id;
    /// Maximum number of results. 0 = unlimited.
    size_t max_results{0};
    /// Hint: require path uniqueness (no repeated vertices).
    bool unique_vertices{false};
    /// Hint: require edge uniqueness (no repeated edges).
    bool unique_edges{false};
};

/**
 * @brief Output of a query rewriting pass.
 *
 * Contains the optimised query, the list of rules that were applied, an
 * estimated speedup factor relative to the original query, and a human-readable
 * explanation of each transformation performed.
 */
struct RewrittenQuery {
    /// Semantically-equivalent optimised query.
    GraphQuery optimized_query;
    /// Rewrite rules that were applied (may be empty if no rewriting was useful).
    std::vector<RewriteRule> applied_rules;
    /// Estimated relative speedup vs the original query (>= 1.0).
    double estimated_speedup{1.0};
    /// Human-readable explanation of the transformations applied.
    std::string explanation;
};

/**
 * @brief Abstract interface for graph query rewriting.
 *
 * A concrete implementation analyses the logical query structure and applies
 * cost-reducing transformations before the GraphQueryOptimizer selects an
 * execution strategy.
 *
 * Thread-safety: rewrite() and explainRewrite() are logically read-only on
 * the query input; implementations that maintain rule state must synchronise
 * enableRule() / disableRule() with concurrent rewrite() calls.
 */
class IGraphQueryRewriter {
public:
    virtual ~IGraphQueryRewriter() = default;

    /**
     * @brief Apply all enabled rewrite rules to @p query.
     *
     * The returned RewrittenQuery contains a semantically equivalent but
     * potentially faster query along with provenance information.
     *
     * @param query Original query to transform.
     * @return RewrittenQuery with optimised fields and applied rule list.
     */
    virtual RewrittenQuery rewrite(const GraphQuery& query) = 0;

    /**
     * @brief Enable a rewrite rule.
     *
     * @return true if the rule was previously disabled, false if already on.
     */
    virtual bool enableRule(RewriteRule rule) = 0;

    /**
     * @brief Disable a rewrite rule.
     *
     * @return true if the rule was previously enabled, false if already off.
     */
    virtual bool disableRule(RewriteRule rule) = 0;

    /**
     * @brief Return the set of currently active rules.
     */
    virtual std::vector<RewriteRule> activeRules() const = 0;

    /**
     * @brief Return a dry-run explanation of the rewrites that would be applied.
     *
     * Does not mutate any state; can be called concurrently.
     */
    virtual std::string explainRewrite(const GraphQuery& query) const = 0;
};

/**
 * @brief Statistics accumulated across multiple rewrite() calls.
 */
struct RewriterStats {
    /// Total number of queries processed.
    uint64_t total_queries{0};
    /// Number of queries where at least one rule fired.
    uint64_t queries_rewritten{0};
    /// Total number of rule applications across all queries.
    uint64_t total_rule_applications{0};
    /// Per-rule application counts.
    std::unordered_map<int, uint64_t> rule_application_counts;
    /// Cumulative estimated speedup sum (divide by queries_rewritten for average).
    double cumulative_estimated_speedup{0.0};
};

/**
 * @brief Concrete graph query rewriter.
 *
 * Applies the following transformations in order:
 *
 * 1. **PREDICATE_PUSHDOWN** — Moves node_filters and edge_filters into the
 *    traversal context so that non-matching vertices/edges are pruned before
 *    they are expanded.  Estimated speedup: proportional to filter selectivity.
 *
 * 2. **EDGE_TYPE_FILTER_PUSHDOWN** — When edge_type is set, converts it to a
 *    traversal-level hint so that out-adjacency lookups are type-scoped.
 *
 * 3. **COMMON_SUBEXPRESSION_ELIMINATION** — Detects repeated predicate terms in
 *    node_filters / edge_filters and deduplicates them.
 *
 * 4. **JOIN_REORDERING** — When multiple node_filters are present, sorts them by
 *    estimated selectivity (alphabetically as a proxy when statistics are absent)
 *    so the most selective predicate runs first.
 *
 * 5. **QUERY_DECOMPOSITION** — When min_depth == max_depth and no end vertex is
 *    specified, the query can safely be decomposed into independent sub-queries
 *    (one per source shard) and the results merged.  Sets a decomposition hint.
 *
 * 6. **MATERIALIZED_VIEW_UTILIZATION** — (Advisory) Checks a user-supplied view
 *    registry and annotates the query when a matching precomputed view exists.
 *
 * All rules are enabled by default.  Individual rules can be toggled via
 * enableRule() / disableRule().
 *
 * Usage:
 * @code
 *   GraphQueryRewriter rewriter;
 *   rewriter.disableRule(RewriteRule::MATERIALIZED_VIEW_UTILIZATION);
 *
 *   GraphQuery q;
 *   q.start_vertex = "user:alice";
 *   q.max_depth    = 3;
 *   q.edge_type    = "FOLLOWS";
 *   q.node_filters = {"country=USA", "active=true"};
 *
 *   auto result = rewriter.rewrite(q);
 *   // result.applied_rules contains applied rules
 *   // result.optimized_query has node_filters in selectivity order
 *   // result.estimated_speedup reflects predicate pushdown gain
 * @endcode
 */
class GraphQueryRewriter : public IGraphQueryRewriter {
public:
    /**
     * @brief Construct with all rules enabled.
     */
    GraphQueryRewriter();

    /**
     * @brief Construct with an explicit set of initially enabled rules.
     */
    explicit GraphQueryRewriter(std::vector<RewriteRule> enabled_rules);

    ~GraphQueryRewriter() override = default;

    // Non-copyable, movable
    GraphQueryRewriter(const GraphQueryRewriter&) = delete;
    GraphQueryRewriter& operator=(const GraphQueryRewriter&) = delete;
    GraphQueryRewriter(GraphQueryRewriter&&) noexcept = default;
    GraphQueryRewriter& operator=(GraphQueryRewriter&&) noexcept = default;

    RewrittenQuery rewrite(const GraphQuery& query) override;
    bool enableRule(RewriteRule rule) override;
    bool disableRule(RewriteRule rule) override;
    std::vector<RewriteRule> activeRules() const override;
    std::string explainRewrite(const GraphQuery& query) const override;

    /**
     * @brief Register a precomputed view by name.
     *
     * When MATERIALIZED_VIEW_UTILIZATION is enabled and a query's graph_id
     * matches a registered view, the rewriter annotates the result explanation
     * with the view name so the caller can substitute the view.
     *
     * @param view_name   Identifier used for lookup.
     * @param graph_id    Graph scope the view covers.
     * @param edge_type   Edge type the view is restricted to (empty = all).
     * @param max_depth   Maximum depth the view covers.
     */
    void registerView(
        const std::string& view_name,
        const std::string& graph_id,
        const std::string& edge_type,
        int max_depth);

    /**
     * @brief Return accumulated rewriting statistics.
     */
    const RewriterStats& stats() const noexcept;

    /**
     * @brief Reset statistics counters.
     */
    void resetStats() noexcept;

private:
    std::unordered_set<int> active_rules_;
    RewriterStats stats_;

    struct ViewEntry {
        std::string view_name;
        std::string graph_id;
        std::string edge_type;
        int max_depth;
    };
    std::vector<ViewEntry> views_;

    bool isRuleEnabled(RewriteRule rule) const noexcept;

    // ── Rule implementations ────────────────────────────────────────────────
    void applyPredicatePushdown(
        const GraphQuery& in, GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl, double& speedup) const;

    void applyEdgeTypeFilterPushdown(
        const GraphQuery& in, GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl, double& speedup) const;

    void applyCSE(
        GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl) const;

    void applyJoinReordering(
        GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl, double& speedup) const;

    void applyQueryDecomposition(
        const GraphQuery& in, GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl, double& speedup) const;

    void applyMaterializedViewUtilization(
        const GraphQuery& in, GraphQuery& out,
        std::vector<RewriteRule>& applied,
        std::string& expl) const;
};

} // namespace graph
} // namespace themis
