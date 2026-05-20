/*
 * ThemisDB | File: tensor_aware_query_optimizer.cpp | Version: 1.0.0 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 89/100 | Lines: 165
 * Open Issues: TODOs=1, Stubs=4, Gaps=7, Unimpl=0, Mock=1, Sim=1, Debt=0
 * Gap Correlation: internal=7 | external_v3=38 | delta=31 | status=divergent
 * External Severity (v3): C=5, H=31, M=2
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file query/tensor_aware_query_optimizer.cpp
 * @brief TensorAwareQueryOptimizer implementation.
 *
 * ### Stub log
 * - TAQO-01  Full AQL AST traversal (not just description-string scan)
 *            deferred to Phase 3 integration with AQL runner (Q1 2027).
 *
 * STUB/SIMULATION NOTE (stub #275):
 * Purpose: Detection is based on presence of function names in the plan
 *          node `description` field, which is available from the existing
 *          `QueryPlanNode` serialization path.  A deeper AST-level rewrite
 *          (replacing function call nodes in the AQL IR) requires coupling
 *          to the AQL runner's internal IR and is Phase 3 Phase-C work.
 * Activation: Always active; used by any caller with a QueryPlanNode tree.
 * Production Delta: Phase 3 will wire directly into AQL AST nodes so that
 *                   the rewrite bypasses string scanning entirely.
 * Removal Plan: Replace description-scan with AQL IR visitor in Q1 2027.
 */

#include "query/tensor_aware_query_optimizer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>
#include <string>

namespace themis {
namespace query {

// ============================================================================
// Static data
// ============================================================================

const std::unordered_set<std::string> TensorAwareQueryOptimizer::kTensorFunctions = {
    "TENSOR_SIMILARITY",
    "TENSOR_NORM",
    "TENSOR_SLICE",
    "TENSOR_COMPRESS",
    "TENSOR_INFO",
    "TENSOR_CONTRACT",
    "TENSOR_PROJECT",
    "TENSOR_DECOMPOSE",
};

// ============================================================================
// isTensorFunction
// ============================================================================

bool TensorAwareQueryOptimizer::isTensorFunction(const std::string& name) noexcept {
    // Accept both upper and mixed case.
    std::string upper;
    upper.reserve(name.size());
    for (char c : name) upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return kTensorFunctions.count(upper) > 0;
}

// ============================================================================
// estimateTTCost
// ============================================================================

double TensorAwareQueryOptimizer::estimateTTCost(
    const std::string& function_name,
    std::size_t order,
    std::size_t mode_size,
    std::size_t max_rank) noexcept {

    // Complexity estimates from Holtz et al. (2012) / paper §AQL.
    const double d = static_cast<double>(order    > 0 ? order    : 4);
    const double n = static_cast<double>(mode_size > 0 ? mode_size : 4);
    const double r = static_cast<double>(max_rank  > 0 ? max_rank  : 8);

    if (function_name == "TENSOR_SIMILARITY" ||
        function_name == "TENSOR_NORM"        ||
        function_name == "TENSOR_CONTRACT") {
        // Inner-product / transfer-matrix: O(d·n·r³)
        return d * n * r * r * r;
    }
    if (function_name == "TENSOR_SLICE" ||
        function_name == "TENSOR_PROJECT") {
        // Slice / marginalize one core: O(d·n·r²)
        return d * n * r * r;
    }
    if (function_name == "TENSOR_COMPRESS" ||
        function_name == "TENSOR_DECOMPOSE") {
        // TT-rounding / decomposition: O(d·r²·n)
        return d * r * r * n * std::log2(n + 1.0);
    }
    if (function_name == "TENSOR_INFO") {
        return d * n;
    }
    // Unknown — use a generic linear estimate.
    return d * n * r;
}

// ============================================================================
// rewriteNode — depth-first DFS
// ============================================================================

void TensorAwareQueryOptimizer::rewriteNode(QueryPlanNode& node) {
    ++last_stats_.nodes_visited;

    // Check whether this node's description mentions a tensor function.
    std::string upper_desc;
    upper_desc.reserve(node.description.size());
    for (char c : node.description)
        upper_desc += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    for (const auto& fn : kTensorFunctions) {
        if (upper_desc.find(fn) != std::string::npos) {
            // Estimate costs.
            // Use heuristic parameters; in Phase 3 these will be derived from
            // the AQL IR's actual tensor operand metadata.
            constexpr std::size_t kDefaultOrder    = 4;
            constexpr std::size_t kDefaultModeSize = 16;
            constexpr std::size_t kDefaultRank     = 8;

            const double tt_cost = estimateTTCost(fn,
                                                   kDefaultOrder,
                                                   kDefaultModeSize,
                                                   kDefaultRank);
            // Baseline: dense reconstruction cost O(n^d)
            const double dense_cost = std::pow(
                static_cast<double>(kDefaultModeSize),
                static_cast<double>(kDefaultOrder));

            last_stats_.total_baseline_cost  += dense_cost;
            last_stats_.total_optimized_cost += tt_cost;

            node.type          = PlanNodeType::TensorContraction;
            node.estimated_cost = tt_cost;
            node.description   = "[TT-domain] " + node.description;
            ++last_stats_.nodes_rewritten;
            break;  // only rewrite once per node
        }
    }

    // Recurse into children.
    for (auto& child : node.children) {
        if (child) rewriteNode(*child);
    }
}

// ============================================================================
// rewrite — public entry point
// ============================================================================

std::shared_ptr<QueryPlanNode>
TensorAwareQueryOptimizer::rewrite(std::shared_ptr<QueryPlanNode> root) {
    last_stats_ = {};
    if (root) rewriteNode(*root);
    return root;
}

} // namespace query
} // namespace themis
