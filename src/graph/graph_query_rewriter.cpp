/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_query_rewriter.cpp                           ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-04-09                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     370                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • (initial)  2026-04-09  feat(graph): graph query rewriter implementation ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "graph/graph_query_rewriter.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace graph {

// =============================================================================
// Construction
// =============================================================================

GraphQueryRewriter::GraphQueryRewriter()
{
    active_rules_.insert(static_cast<int>(RewriteRule::PREDICATE_PUSHDOWN));
    active_rules_.insert(static_cast<int>(RewriteRule::COMMON_SUBEXPRESSION_ELIMINATION));
    active_rules_.insert(static_cast<int>(RewriteRule::JOIN_REORDERING));
    active_rules_.insert(static_cast<int>(RewriteRule::MATERIALIZED_VIEW_UTILIZATION));
    active_rules_.insert(static_cast<int>(RewriteRule::QUERY_DECOMPOSITION));
    active_rules_.insert(static_cast<int>(RewriteRule::EDGE_TYPE_FILTER_PUSHDOWN));
}

GraphQueryRewriter::GraphQueryRewriter(std::vector<RewriteRule> enabled_rules)
{
    for (auto rule : enabled_rules)
        active_rules_.insert(static_cast<int>(rule));
}

// =============================================================================
// Rule management
// =============================================================================

bool GraphQueryRewriter::isRuleEnabled(RewriteRule rule) const noexcept
{
    return active_rules_.count(static_cast<int>(rule)) > 0;
}

bool GraphQueryRewriter::enableRule(RewriteRule rule)
{
    auto [it, inserted] = active_rules_.insert(static_cast<int>(rule));
    return inserted;
}

bool GraphQueryRewriter::disableRule(RewriteRule rule)
{
    return active_rules_.erase(static_cast<int>(rule)) > 0;
}

std::vector<RewriteRule> GraphQueryRewriter::activeRules() const
{
    std::vector<RewriteRule> result;
    result.reserve(active_rules_.size());
    for (int r : active_rules_)
        result.push_back(static_cast<RewriteRule>(r));
    std::sort(result.begin(), result.end(),
              [](RewriteRule a, RewriteRule b) {
                  return static_cast<int>(a) < static_cast<int>(b);
              });
    return result;
}

// =============================================================================
// View registration
// =============================================================================

void GraphQueryRewriter::registerView(
    const std::string& view_name,
    const std::string& graph_id,
    const std::string& edge_type,
    int max_depth)
{
    views_.push_back({view_name, graph_id, edge_type, max_depth});
}

// =============================================================================
// Statistics
// =============================================================================

const RewriterStats& GraphQueryRewriter::stats() const noexcept { return stats_; }

void GraphQueryRewriter::resetStats() noexcept { stats_ = {}; }

// =============================================================================
// Internal rule implementations
// All apply* methods are const — they only modify output parameters (applied,
// out, expl, speedup), never 'this' member state. Stats are updated by the
// non-const rewrite() caller after collecting applied rules.
// =============================================================================

// ── PREDICATE_PUSHDOWN ────────────────────────────────────────────────────────

void GraphQueryRewriter::applyPredicatePushdown(
    const GraphQuery& /*in*/, GraphQuery& out,
    std::vector<RewriteRule>& applied,
    std::string& expl, double& speedup) const
{
    if (!out.node_filters.empty() || !out.edge_filters.empty()) {
        applied.push_back(RewriteRule::PREDICATE_PUSHDOWN);
        expl += "PREDICATE_PUSHDOWN: " +
                std::to_string(out.node_filters.size()) + " node filter(s) and " +
                std::to_string(out.edge_filters.size()) + " edge filter(s) pushed into traversal frontier; ";
        double filter_count = static_cast<double>(
            out.node_filters.size() + out.edge_filters.size());
        speedup *= 1.0 + 0.2 * filter_count;
    }
}

// ── EDGE_TYPE_FILTER_PUSHDOWN ─────────────────────────────────────────────────

void GraphQueryRewriter::applyEdgeTypeFilterPushdown(
    const GraphQuery& /*in*/, GraphQuery& out,
    std::vector<RewriteRule>& applied,
    std::string& expl, double& speedup) const
{
    if (!out.edge_type.empty()) {
        applied.push_back(RewriteRule::EDGE_TYPE_FILTER_PUSHDOWN);
        expl += "EDGE_TYPE_FILTER_PUSHDOWN: edge_type='" + out.edge_type +
                "' pushed to adjacency lookup level; ";
        speedup *= 1.3;
    }
}

// ── COMMON_SUBEXPRESSION_ELIMINATION ─────────────────────────────────────────

void GraphQueryRewriter::applyCSE(
    GraphQuery& out,
    std::vector<RewriteRule>& applied,
    std::string& expl) const
{
    auto dedup = [](std::vector<std::string>& filters) -> size_t {
        std::unordered_set<std::string> seen;
        std::vector<std::string> unique;
        unique.reserve(filters.size());
        for (auto& f : filters) {
            if (seen.insert(f).second) unique.push_back(f);
        }
        size_t removed = filters.size() - unique.size();
        filters = std::move(unique);
        return removed;
    };

    size_t removed = dedup(out.node_filters) + dedup(out.edge_filters);
    if (removed > 0) {
        applied.push_back(RewriteRule::COMMON_SUBEXPRESSION_ELIMINATION);
        expl += "CSE: removed " + std::to_string(removed) + " duplicate predicate(s); ";
    }
}

// ── JOIN_REORDERING ───────────────────────────────────────────────────────────

void GraphQueryRewriter::applyJoinReordering(
    GraphQuery& out,
    std::vector<RewriteRule>& applied,
    std::string& expl, double& speedup) const
{
    if (out.node_filters.size() > 1) {
        bool was_sorted = std::is_sorted(
            out.node_filters.begin(), out.node_filters.end(),
            [](const std::string& a, const std::string& b) {
                return a.size() < b.size();
            });
        std::stable_sort(
            out.node_filters.begin(), out.node_filters.end(),
            [](const std::string& a, const std::string& b) {
                return a.size() < b.size();
            });
        if (!was_sorted) {
            applied.push_back(RewriteRule::JOIN_REORDERING);
            expl += "JOIN_REORDERING: " + std::to_string(out.node_filters.size()) +
                    " node filter(s) reordered by selectivity estimate; ";
            speedup *= 1.1;
        }
    }
}

// ── QUERY_DECOMPOSITION ───────────────────────────────────────────────────────

void GraphQueryRewriter::applyQueryDecomposition(
    const GraphQuery& in, GraphQuery& out,
    std::vector<RewriteRule>& applied,
    std::string& expl, double& speedup) const
{
    if (in.end_vertex.empty() &&
        in.min_depth > 0 &&
        in.min_depth == in.max_depth &&
        out.graph_id.find(":decompose") == std::string::npos)
    {
        out.graph_id += ":decompose";
        applied.push_back(RewriteRule::QUERY_DECOMPOSITION);
        expl += "QUERY_DECOMPOSITION: fixed-depth fan-out annotated for "
                "parallel per-source execution; ";
        speedup *= 1.5;
    }
}

// ── MATERIALIZED_VIEW_UTILIZATION ─────────────────────────────────────────────

void GraphQueryRewriter::applyMaterializedViewUtilization(
    const GraphQuery& in, GraphQuery& /*out*/,
    std::vector<RewriteRule>& applied,
    std::string& expl) const
{
    for (const auto& view : views_) {
        bool graph_match = view.graph_id.empty() || view.graph_id == in.graph_id;
        bool edge_match  = view.edge_type.empty() || view.edge_type == in.edge_type;
        bool depth_match = in.max_depth <= view.max_depth;
        if (graph_match && edge_match && depth_match) {
            applied.push_back(RewriteRule::MATERIALIZED_VIEW_UTILIZATION);
            expl += "MATERIALIZED_VIEW: view '" + view.view_name +
                    "' covers this query scope; ";
            break;
        }
    }
}

// =============================================================================
// rewrite()
// =============================================================================

RewrittenQuery GraphQueryRewriter::rewrite(const GraphQuery& query)
{
    RewrittenQuery result;
    result.optimized_query   = query;
    result.estimated_speedup = 1.0;

    std::vector<RewriteRule>& applied = result.applied_rules;
    std::string& expl = result.explanation;
    double& speedup   = result.estimated_speedup;

    // Apply rules in fixed order for deterministic output.
    if (isRuleEnabled(RewriteRule::COMMON_SUBEXPRESSION_ELIMINATION))
        applyCSE(result.optimized_query, applied, expl);

    if (isRuleEnabled(RewriteRule::PREDICATE_PUSHDOWN))
        applyPredicatePushdown(query, result.optimized_query, applied, expl, speedup);

    if (isRuleEnabled(RewriteRule::EDGE_TYPE_FILTER_PUSHDOWN))
        applyEdgeTypeFilterPushdown(query, result.optimized_query, applied, expl, speedup);

    if (isRuleEnabled(RewriteRule::JOIN_REORDERING))
        applyJoinReordering(result.optimized_query, applied, expl, speedup);

    if (isRuleEnabled(RewriteRule::QUERY_DECOMPOSITION))
        applyQueryDecomposition(query, result.optimized_query, applied, expl, speedup);

    if (isRuleEnabled(RewriteRule::MATERIALIZED_VIEW_UTILIZATION))
        applyMaterializedViewUtilization(query, result.optimized_query, applied, expl);

    if (expl.empty()) expl = "No rewrites applied.";

    // Update statistics (non-const context only — not called from explainRewrite).
    stats_.total_queries++;
    if (!applied.empty()) {
        stats_.queries_rewritten++;
        stats_.cumulative_estimated_speedup += speedup;
        for (RewriteRule r : applied) {
            stats_.total_rule_applications++;
            stats_.rule_application_counts[static_cast<int>(r)]++;
        }
    }

    return result;
}

// =============================================================================
// explainRewrite() — dry-run (read-only, no stats side-effects)
// =============================================================================

std::string GraphQueryRewriter::explainRewrite(const GraphQuery& query) const
{
    GraphQuery dummy_out = query;
    std::string expl;
    double speedup = 1.0;
    std::vector<RewriteRule> applied;

    if (isRuleEnabled(RewriteRule::COMMON_SUBEXPRESSION_ELIMINATION))
        applyCSE(dummy_out, applied, expl);
    if (isRuleEnabled(RewriteRule::PREDICATE_PUSHDOWN))
        applyPredicatePushdown(query, dummy_out, applied, expl, speedup);
    if (isRuleEnabled(RewriteRule::EDGE_TYPE_FILTER_PUSHDOWN))
        applyEdgeTypeFilterPushdown(query, dummy_out, applied, expl, speedup);
    if (isRuleEnabled(RewriteRule::JOIN_REORDERING))
        applyJoinReordering(dummy_out, applied, expl, speedup);
    if (isRuleEnabled(RewriteRule::QUERY_DECOMPOSITION))
        applyQueryDecomposition(query, dummy_out, applied, expl, speedup);
    if (isRuleEnabled(RewriteRule::MATERIALIZED_VIEW_UTILIZATION))
        applyMaterializedViewUtilization(query, dummy_out, applied, expl);

    if (expl.empty()) return "No rewrites would be applied.";

    std::ostringstream oss;
    oss << "Estimated speedup: " << speedup << "x. Rules: " << expl;
    return oss.str();
}

} // namespace graph
} // namespace themis
