/**
 * @file query_plan_visualizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.25
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    TensorContraction,// TT-compressed tensor operation routed by TensorAwareQueryOptimizer
    LLMGenerate,      // Probabilistic LLM generation step (non-deterministic)
    Unknown
};

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

class QueryPlanVisualizer {
public:
    // ------------------------------------------------------------------
    // Plan construction
    // ------------------------------------------------------------------

    /**
     * @brief Build Plan.
     * @param[in] query Input parameter.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    static QueryPlanNode buildPlan(const ConjunctiveQuery& query,
                                   const QueryOptimizer::Plan& plan);

    // ------------------------------------------------------------------
    // Rendering
    // ------------------------------------------------------------------

    static std::string toText(const QueryPlanNode& root, bool analyze = false);

    static nlohmann::json toJSON(const QueryPlanNode& root, bool analyze = false);

    /**
     * @brief To DOT.
     * @param[in] root Input parameter.
     * @return Return value.
     */
    static std::string toDOT(const QueryPlanNode& root);

    /**
     * @brief Plan Node Type Name.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static std::string planNodeTypeName(PlanNodeType type);

private:
    // Internal helpers
    /**
     * @brief To Text Impl.
     * @param[in] node Input parameter.
     * @param[in] analyze Input parameter.
     * @param[in,out] out Input/output parameter.
     * @param[in] depth Input parameter.
     */
    static void toTextImpl(const QueryPlanNode& node, bool analyze,
                           std::string& out, int depth);
    /**
     * @brief To JSONImpl.
     * @param[in] node Input parameter.
     * @param[in] analyze Input parameter.
     * @return Return value.
     */
    static nlohmann::json toJSONImpl(const QueryPlanNode& node, bool analyze);
    /**
     * @brief To JSONImpl.
     * @param[in] node Input parameter.
     * @param[in] analyze Input parameter.
     * @param[in] depth Input parameter.
     * @return Return value.
     */
    static nlohmann::json toJSONImpl(const QueryPlanNode& node, bool analyze, int depth);
    /**
     * @brief To DOTImpl.
     * @param[in] node Input parameter.
     * @param[in,out] id_counter Input/output parameter.
     * @param[in,out] nodes_out Input/output parameter.
     * @param[in,out] edges_out Input/output parameter.
     */
    static void toDOTImpl(const QueryPlanNode& node, int& id_counter,
                          std::string& nodes_out, std::string& edges_out);
    /**
     * @brief To DOTImpl.
     * @param[in] node Input parameter.
     * @param[in,out] id_counter Input/output parameter.
     * @param[in,out] nodes_out Input/output parameter.
     * @param[in,out] edges_out Input/output parameter.
     * @param[in] depth Input parameter.
     */
    static void toDOTImpl(const QueryPlanNode& node, int& id_counter,
                          std::string& nodes_out, std::string& edges_out, int depth);

    
};

} // namespace query
} // namespace themis
