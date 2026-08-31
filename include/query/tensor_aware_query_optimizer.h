/**
 * @file tensor_aware_query_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "query/query_plan_visualizer.h"

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

/**
 * @brief Describes a single tensor function call detected during plan rewrite.
 */
struct TensorContractionPlanNode {
    /// Function name as it appears in the AQL query (e.g. "TENSOR_SIMILARITY").
    std::string function_name;

    /// Human-readable summary of the operation and operands.
    std::string description;

    /// Estimated cost in the TT domain (O(d·n·r²)).
    double estimated_cost = 0.0;

    /// Estimated cost without optimization (dense reconstruction, O(∏ nk)).
    double baseline_cost = 0.0;

    /// Whether the node was successfully rewritten (false = unknown function).
    bool rewritten = false;
};

// ============================================================================
// TensorAwareQueryOptimizer
// ============================================================================

/**
 * @brief Post-processes AQL query plan nodes to route tensor functions to
 *        `TensorContractionEngine` instead of flat-vector reconstruction.
 *
 * ### Usage
 *
 * ```cpp
 * TensorAwareQueryOptimizer opt;
 *
 * // After initial planning:
 * auto plan = query_optimizer.plan(aql_query);
 *
 * // Rewrite tensor function nodes:
 * auto rewritten = opt.rewrite(plan);
 *
 * // The rewritten tree has TensorContraction nodes wherever tensor
 * // functions appeared. Use stats() to log the cost savings.
 * auto stats = opt.lastStats();
 * ```
 *
 * ### AQL-IR visitor bridge (stub #275 resolution)
 *
 * An AQL runner that has access to its internal IR can inject a real AST
 * visitor via `setIRVisitorFn()`.  The visitor is called before the
 * string-scan fallback; if it returns `true` the string scan is skipped
 * for that node.
 */
class TensorAwareQueryOptimizer {
public:
    using TensorNodeDetectorFn = std::function<std::optional<std::string>(const QueryPlanNode&)>;

    TensorAwareQueryOptimizer() = default;

    // ─── AQL-IR visitor bridge ────────────────────────────────────────────

    /**
     * @brief Type alias for an AQL-IR-level AST tensor-node visitor.
     *
     * When the visitor detects a tensor expression in `node`, it must:
     *  - Set `node.type = PlanNodeType::TensorContraction`.
     *  - Set `node.estimated_cost` to the TT-domain cost estimate.
     *  - Set `node.description` to a human-readable summary.
     *  - Populate `baseline_cost_out` with the equivalent dense-reconstruction
     *    cost so that `RewriteStats::costReductionFactor()` is meaningful.
     *  - Return `true`.
     *
     * Returning `false` (or leaving the node unchanged) causes the
     * string-scan heuristic to run as a fallback.
     *
     * On exception the visitor's changes to `node` are rolled back and the
     * string-scan fallback is used.
     */
    using IRVisitorFn = std::function<bool(QueryPlanNode& node,
                                           double& baseline_cost_out)>;

    /**
     * @brief Register an AQL-IR visitor for AST-level tensor-node detection.
     *
     * Replaces any previously registered visitor.  Pass an empty/null
     * function to revert to the string-scan heuristic.
     *
     * Thread-safe.
     *
     * @param fn  Visitor to register; may be empty to clear.
     */
    static void setIRVisitorFn(IRVisitorFn fn);

    /**
     * @brief Clear any previously registered AQL-IR visitor.
     *
     * After this call the optimizer reverts to the string-scan heuristic.
     * Thread-safe.
     *
     * @return Nothing.
     */
    static void clearIRVisitorFn();

    // ─── Plan rewriting ───────────────────────────────────────────────────

    /**
     * @brief Rewrite a QueryPlanNode tree, replacing tensor function nodes.
     *
     * Traverses the tree depth-first.  For each node:
     *  1. If an IR visitor is registered, it is called first; on success
     *     (return `true`) the string-scan step is skipped.
     *  2. Otherwise (or on visitor exception/false return), the node's
     *     `description` is scanned for known tensor function names.
     *  3. Detected nodes are classified as `PlanNodeType::TensorContraction`
     *     and annotated with "[TT-domain]" in the description.
     *
     * The tree is modified in place and a copy of the root is returned.
     *
     * @param root  Root of the query plan tree (may be modified in place).
     * @return      The (possibly modified) plan root.
     */
    std::shared_ptr<QueryPlanNode>
        rewrite(std::shared_ptr<QueryPlanNode> root);

    // ─── Function detection ───────────────────────────────────────────────

    /**
     * @brief Check whether a function name is a recognized tensor function.
     *
     * @param name  Upper-case AQL function name (e.g. "TENSOR_SIMILARITY").
     * @return      true if the optimizer handles this function.
     */
    [[nodiscard]] static bool isTensorFunction(const std::string& name) noexcept;

    /**
     * @brief Estimate the cost of executing `function_name` in the TT domain.
     *
     * @param function_name   AQL tensor function name.
     * @param order           Tensor order d.
     * @param mode_size       Typical mode size n.
     * @param max_rank        Maximum TT-rank r.
     * @return                Estimated floating-point operations.
     */
    [[nodiscard]] static double estimateTTCost(const std::string& function_name,
                                               std::size_t order,
                                               std::size_t mode_size,
                                               std::size_t max_rank) noexcept;

    // ─── Detector bridge ──────────────────────────────────────────────────

    /**
     * @brief Register a node detector that can classify tensor plan nodes
     *        without relying on description-string scanning.
     *
     * The detector receives the current plan node and may return the matched
     * tensor function name. Returning std::nullopt falls back to the normal
     * description-scan heuristic. Exceptions are caught and also fall back.
     *
     * Thread-safe.
     *
     * @param fn Detector callable, or empty to clear.
     */
    void setTensorNodeDetectorFn(TensorNodeDetectorFn fn);

    /**
     * @brief Clear the custom tensor node detector.
     *
     * After this call, rewrite() uses only the IR visitor bridge and the
     * description-scan heuristic.
     *
     * Thread-safe.
     */
    void clearTensorNodeDetectorFn();

    // ─── Statistics ───────────────────────────────────────────────────────

    struct RewriteStats {
        std::size_t nodes_visited         = 0; ///< Total plan nodes examined
        std::size_t nodes_rewritten       = 0; ///< Nodes converted to TensorContraction
        double      total_baseline_cost   = 0.0;
        double      total_optimized_cost  = 0.0;
        /**
         * @brief Return the estimated cost reduction factor.
         * @return Baseline-cost / optimized-cost ratio, clamped to at least 1.0.
         */
        double costReductionFactor() const noexcept {
            return total_optimized_cost > 0.0
                ? total_baseline_cost / total_optimized_cost
                : 1.0;
        }
    };

    /**
     * @brief Returns statistics from the most recent `rewrite()` call.
     * @return Snapshot of the last rewrite pass statistics.
     */
    [[nodiscard]] RewriteStats lastStats() const noexcept { return last_stats_; }

    // ─── AST visitor bridge (stub #275) ──────────────────────────────────────

    /// @brief Type alias for AST visitor injection.
    using AstVisitorFn = std::function<void(QueryPlanNode&)>;

    /**
     * @brief Install an AST visitor called after the description-scan on each node.
     *
     * When set, the visitor is invoked depth-first after each node is processed
     * by the description-scan rewrite pass.  Useful for Phase-3 AQL IR coupling.
     * @param fn Callable receiving a mutable reference to each visited node.
     */
    static void setAstVisitorFn(AstVisitorFn fn);

    /**
     * @brief Remove the AST visitor (reverts to description-scan only).
     * @return Nothing.
     */
    static void clearAstVisitorFn();

private:
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

#include "index/ann_frontdoor.h"
#include "themis/rag/kg/knowledge_graph_interface.h"

#include <chrono>
#include <optional>

namespace themis {
namespace query {

/**
 * @brief Parameters for a single hybrid ANN+graph query.
 *
 * The planner retrieves ANN candidates via AnnFrontdoor then expands
 * graph neighbours for the top ANN hits, and fuses the two ranked lists
 * using Reciprocal Rank Fusion (RRF, k=60).
 *
 * Performance gate (W9-12): planAnnGraphHybrid() must complete in ≤500ms
 * for ann_k=1000 candidates merged with graph_max_depth=1, graph_max_nodes=100.
 */
struct HybridAnnGraphQuery {
    std::vector<float> query_vector;          ///< Dense query vector (required)
    std::size_t        ann_k              = 100; ///< ANN candidate count
    std::size_t        graph_max_depth    = 1;   ///< BFS depth for graph expansion
    std::size_t        graph_max_nodes    = 50;  ///< Max graph nodes per ANN hit
    double             graph_min_edge_weight = 0.0;
    std::size_t        top_k             = 20;  ///< Final result count after fusion
    double             rrf_k             = 60.0; ///< RRF constant
    AnnQueryContext    ann_context;              ///< Routing hints for AnnFrontdoor
    std::chrono::milliseconds timeout_ms{500};  ///< Hard wall-clock timeout
};

/** @brief One result entry from the hybrid planner. */
struct HybridAnnGraphResult {
    std::string node_id;          ///< Stable node identifier
    double      rrf_score = 0.0;  ///< RRF-fused relevance score (higher is better)
    int         ann_rank  = -1;   ///< Rank in ANN list (-1 = absent)
    int         graph_rank = -1;  ///< Rank in graph-expansion list (-1 = absent)
    bool        from_graph = false; ///< True if first seen via graph expansion
};

/**
 * @brief Execute a hybrid ANN+graph retrieval plan.
 *
 * 1. Retrieve ann_k ANN candidates via @p frontdoor.
 * 2. Expand graph neighbours for the top ANN hits via @p kg->neighbours().
 * 3. Fuse both ranked lists with RRF (k = query.rrf_k).
 * 4. Return top query.top_k results sorted by descending rrf_score.
 *
 * @param query     Query parameters.
 * @param frontdoor ANN frontdoor (may be null → no ANN step; graph-only).
 * @param kg        Knowledge graph (may be null → no graph step; ANN-only).
 * @return          Fused result list, sorted by descending rrf_score.
 *
 * @throws std::invalid_argument when query.query_vector is empty and
 *         frontdoor is non-null.
 * @throws std::runtime_error    when the timeout is exceeded.
 */
[[nodiscard]] std::vector<HybridAnnGraphResult> planAnnGraphHybrid(
    const HybridAnnGraphQuery&              query,
    const index::AnnFrontdoor*              frontdoor,
    const themis::rag::kg::IKnowledgeGraph* kg);

} // namespace query
} // namespace themis
