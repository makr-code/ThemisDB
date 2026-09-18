/**
 * @file tensor_aware_query_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "index/ann_frontdoor.h"
#include "query/query_plan_visualizer.h"
#include "themis/rag/kg/knowledge_graph_interface.h"

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace query {

// ============================================================================
// TensorContractionPlanNode — metadata for a rewritten tensor node
// ============================================================================

struct TensorContractionPlanNode {
    std::string function_name;

    std::string description;

    double estimated_cost = 0.0;

    double baseline_cost = 0.0;

    bool rewritten = false;
};

// ============================================================================
// TensorAwareQueryOptimizer
// ============================================================================

class TensorAwareQueryOptimizer {
public:
    using TensorNodeDetectorFn = std::function<std::optional<std::string>(const QueryPlanNode&)>;

    TensorAwareQueryOptimizer() = default;

    // ─── AQL-IR visitor bridge ────────────────────────────────────────────

    using IRVisitorFn = std::function<bool(QueryPlanNode& node,
                                           double& baseline_cost_out)>;

    /**
     * @brief Set IRVisitor Fn.
     * @param[in] fn Input parameter.
     */
    static void setIRVisitorFn(IRVisitorFn fn);

    /**
     * @brief Clear IRVisitor Fn.
     */
    static void clearIRVisitorFn();

    // ─── Plan rewriting ───────────────────────────────────────────────────

    std::shared_ptr<QueryPlanNode>
        rewrite(std::shared_ptr<QueryPlanNode> root);

    // ─── Function detection ───────────────────────────────────────────────

    [[nodiscard]] static bool isTensorFunction(const std::string& name) noexcept;

    [[nodiscard]] static double estimateTTCost(const std::string& function_name,
                                               std::size_t order,
                                               std::size_t mode_size,
                                               std::size_t max_rank) noexcept;

    /**
     * @brief ─── Detector bridge ──────────────────────────────────────────────────
     * @param[in] fn Input parameter.
     */

    void setTensorNodeDetectorFn(TensorNodeDetectorFn fn);

    /**
     * @brief Clear Tensor Node Detector Fn.
     */
    void clearTensorNodeDetectorFn();

    // ─── Statistics ───────────────────────────────────────────────────────

    struct RewriteStats {
        std::size_t nodes_visited         = 0; ///< Total plan nodes examined
        std::size_t nodes_rewritten       = 0; ///< Nodes converted to TensorContraction
        double      total_baseline_cost   = 0.0;
        double      total_optimized_cost  = 0.0;
        double costReductionFactor() const noexcept {
            return total_optimized_cost > 0.0
                ? total_baseline_cost / total_optimized_cost
                : 1.0;
        }
    };

    [[nodiscard]] RewriteStats lastStats() const noexcept { return last_stats_; }

    // ─── AST visitor bridge (stub #275) ──────────────────────────────────────

    using AstVisitorFn = std::function<void(QueryPlanNode&)>;

    /**
     * @brief Set Ast Visitor Fn.
     * @param[in] fn Input parameter.
     */
    static void setAstVisitorFn(AstVisitorFn fn);

    /**
     * @brief Clear Ast Visitor Fn.
     */
    static void clearAstVisitorFn();

private:
    /**
     * @brief Rewrite Node.
     * @param[in,out] node Input/output parameter.
     */
    void rewriteNode(QueryPlanNode& node);

    RewriteStats last_stats_;
    mutable std::shared_mutex detector_mutex_;
    TensorNodeDetectorFn tensor_node_detector_fn_;

    // Set of function names routed to TensorContractionEngine.
    static const std::unordered_set<std::string> kTensorFunctions;

    // Static IR visitor bridge (process-wide, guarded by ir_visitor_mutex_).
    static IRVisitorFn    ir_visitor_fn_;
    static std::mutex     ir_visitor_mutex_;
};

} // namespace query
} // namespace themis

// ============================================================================
// W9-12: ANN+Graph Hybrid Planner declarations
// ============================================================================

#include <chrono>
#include <optional>

namespace themis {
namespace query {

struct HybridAnnGraphQuery {
    std::vector<float> query_vector;          ///< Dense query vector (required)
    std::size_t        ann_k              = 100; ///< ANN candidate count
    std::size_t        graph_max_depth    = 1;   ///< BFS depth for graph expansion
    std::size_t        graph_max_nodes    = 50;  ///< Max graph nodes per ANN hit
    double             graph_min_edge_weight = 0.0;
    std::size_t        top_k             = 20;  ///< Final result count after fusion
    double             rrf_k             = 60.0; ///< RRF constant
    index::AnnQueryContext ann_context;         ///< Routing hints for AnnFrontdoor
    std::chrono::milliseconds timeout_ms{500};  ///< Hard wall-clock timeout
};

struct HybridAnnGraphResult {
    std::string node_id;          ///< Stable node identifier
    double      rrf_score = 0.0;  ///< RRF-fused relevance score (higher is better)
    int         ann_rank  = -1;   ///< Rank in ANN list (-1 = absent)
    int         graph_rank = -1;  ///< Rank in graph-expansion list (-1 = absent)
    bool        from_graph = false; ///< True if first seen via graph expansion
};

[[nodiscard]] std::vector<HybridAnnGraphResult> planAnnGraphHybrid(
    const HybridAnnGraphQuery&              query,
    const index::AnnFrontdoor*              frontdoor,
    const themis::rag::kg::IKnowledgeGraph* kg);

} // namespace query
} // namespace themis
