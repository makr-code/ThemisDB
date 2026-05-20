/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            query/tensor_aware_query_optimizer.h               ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file query/tensor_aware_query_optimizer.h
 * @brief TensorAwareQueryOptimizer — Phase 3 AQL plan node routing.
 *
 * ## Role
 *
 * This optimizer post-processes AQL query plans to detect expressions
 * that invoke tensor functions (`TENSOR_SIMILARITY`, `TENSOR_CONTRACT`,
 * `TENSOR_NORM`, `TENSOR_SLICE`, `TENSOR_PROJECT`, `TENSOR_DECOMPOSE`)
 * and rewrites them as `PlanNodeType::TensorContraction` plan nodes.
 *
 * Without this optimizer the AQL runner would call `TensorTrainDecomposer`
 * to reconstruct flat vectors and then operate on them in O(∏ nk) space.
 * The optimizer routes these calls to `TensorContractionEngine` which
 * operates directly in the compressed TT domain at O(d·n·r²).
 *
 * ## Design
 *
 * ```
 *  AQL plan (from QueryOptimizer)
 *       │
 *       ▼
 *  TensorAwareQueryOptimizer::rewrite(plan)
 *       │
 *       ├─ detects function calls: TENSOR_SIMILARITY / CONTRACT / …
 *       │
 *       ├─ wraps them in TensorContractionPlanNode
 *       │    ├─ estimated_cost   (lower than naive reconstruction)
 *       │    └─ description      (function + mode summary)
 *       │
 *       └─ returns rewritten QueryPlanNode tree
 * ```
 *
 * ## References
 * - Paper §AQL: structure-oriented query language; topology is the primary object
 * - Paper §TensorAwareQueryOptimizer plan-node routing (Q1 2027)
 * - `include/query/query_plan_visualizer.h` — PlanNodeType::TensorContraction
 * - `include/query/tensor_contraction_engine.h` — computation backend
 */

#pragma once

#include "query/query_plan_visualizer.h"

#include <memory>
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
 */
class TensorAwareQueryOptimizer {
public:
    TensorAwareQueryOptimizer() = default;

    // ─── Plan rewriting ───────────────────────────────────────────────────

    /**
     * @brief Rewrite a QueryPlanNode tree, replacing tensor function nodes.
     *
     * Traverses the tree depth-first.  Any node whose `description` contains
     * a recognized tensor function call is:
     *  1. Classified as `PlanNodeType::TensorContraction`.
     *  2. Given an updated `estimated_cost` reflecting TT-domain complexity.
     *  3. Annotated in `description` with "[TT-domain]" prefix.
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

    // ─── Statistics ───────────────────────────────────────────────────────

    struct RewriteStats {
        std::size_t nodes_visited         = 0; ///< Total plan nodes examined
        std::size_t nodes_rewritten       = 0; ///< Nodes converted to TensorContraction
        double      total_baseline_cost   = 0.0;
        double      total_optimized_cost  = 0.0;
        /// Estimated cost reduction factor (baseline / optimized), ≥ 1.0.
        double costReductionFactor() const noexcept {
            return total_optimized_cost > 0.0
                ? total_baseline_cost / total_optimized_cost
                : 1.0;
        }
    };

    /**
     * @brief Returns statistics from the most recent `rewrite()` call.
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
     */
    static void clearAstVisitorFn();

private:
    void rewriteNode(QueryPlanNode& node);

    RewriteStats last_stats_;

    // Set of function names routed to TensorContractionEngine.
    static const std::unordered_set<std::string> kTensorFunctions;
};

} // namespace query
} // namespace themis
