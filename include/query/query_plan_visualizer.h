/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query_plan_visualizer.h                            ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-04-15 18:04:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     141                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "query/query_engine.h"
#include "query/query_optimizer.h"

namespace themis {
namespace query {

// ============================================================================
// QueryPlanNode - A single operator node in the query execution plan tree
// ============================================================================

enum class PlanNodeType {
    SeqScan,          // Full collection scan
    IndexScan,        // Index-backed lookup
    Filter,           // Predicate evaluation
    Sort,             // ORDER BY / SORT
    Limit,            // LIMIT clause
    Return,           // RETURN clause
    Aggregate,        // COLLECT / GROUP BY
    HashJoin,         // Hash-based join
    NestedLoopJoin,   // Nested-loop join
    GraphTraversal,   // Graph BFS/DFS
    VectorSearch,     // ANN vector similarity search
    SpatialFilter,    // Geospatial predicate
    CTE,              // Common Table Expression
    Subquery,         // Inline subquery
    Unknown
};

/// A node in the query execution plan tree.
/// Children represent inputs (sub-operators) consumed by this operator.
struct QueryPlanNode {
    PlanNodeType type = PlanNodeType::Unknown;
    std::string description;                  ///< Human-readable operator label

    // Cost estimates (always present)
    double estimated_cost = 0.0;
    size_t estimated_rows = 0;

    // Runtime statistics (populated during EXPLAIN ANALYZE; -1 / 0 = not measured)
    double actual_time_ms = -1.0;
    size_t actual_rows = 0;

    // Optional operator metadata
    std::optional<std::string> index_name;    ///< Index used by IndexScan
    double selectivity = 1.0;                 ///< Filter selectivity estimate [0,1]
    std::vector<std::string> attributes;      ///< E.g. predicate columns, sort keys

    std::vector<std::shared_ptr<QueryPlanNode>> children;
};

// ============================================================================
// QueryPlanVisualizer - Build and render query execution plans
// ============================================================================

/// Builds a QueryPlanNode tree from an optimized query/plan and renders it in
/// multiple formats:
///   * Text  – PostgreSQL-style EXPLAIN / EXPLAIN ANALYZE output
///   * JSON  – Machine-readable format for programmatic analysis
///   * DOT   – Graphviz DOT for diagram generation
class QueryPlanVisualizer {
public:
    // ------------------------------------------------------------------
    // Plan construction
    // ------------------------------------------------------------------

    /// Build a plan tree from a ConjunctiveQuery and its optimized Plan.
    /// @param query  The logical query.
    /// @param plan   The optimizer plan with predicate ordering.
    /// @returns Root node of the execution plan tree.
    static QueryPlanNode buildPlan(const ConjunctiveQuery& query,
                                   const QueryOptimizer::Plan& plan);

    // ------------------------------------------------------------------
    // Rendering
    // ------------------------------------------------------------------

    /// Render the plan as indented text (EXPLAIN format).
    /// @param root     Root plan node.
    /// @param analyze  When true, include actual_time_ms / actual_rows columns.
    /// @returns Multi-line string representation.
    static std::string toText(const QueryPlanNode& root, bool analyze = false);

    /// Render the plan as a JSON object.
    /// @param root     Root plan node.
    /// @param analyze  When true, include runtime statistics in the output.
    /// @returns nlohmann::json object.
    static nlohmann::json toJSON(const QueryPlanNode& root, bool analyze = false);

    /// Render the plan as a Graphviz DOT digraph string.
    /// Can be piped to `dot -Tpng -o plan.png` for visualisation.
    /// @param root  Root plan node.
    /// @returns DOT source string.
    static std::string toDOT(const QueryPlanNode& root);

private:
    // Internal helpers
    static void toTextImpl(const QueryPlanNode& node, bool analyze,
                           std::string& out, int depth);
    static nlohmann::json toJSONImpl(const QueryPlanNode& node, bool analyze);
    static nlohmann::json toJSONImpl(const QueryPlanNode& node, bool analyze, int depth);
    static void toDOTImpl(const QueryPlanNode& node, int& id_counter,
                          std::string& nodes_out, std::string& edges_out);
    static void toDOTImpl(const QueryPlanNode& node, int& id_counter,
                          std::string& nodes_out, std::string& edges_out, int depth);

    static std::string planNodeTypeName(PlanNodeType type);
};

} // namespace query
} // namespace themis
