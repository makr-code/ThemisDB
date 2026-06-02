/*
 * ThemisDB | File: tensor_aware_query_optimizer.cpp | Version: 1.0.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 89/100 | Lines: 281
 * Gap Summary: total=9; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=1, Debt=0, C=4, H=1, M=2, L=0
 * PR History (last 5): none
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
 * STUB/SIMULATION NOTE (stub #275): RESOLVED via IRVisitorFn injection bridge.
 * Purpose: Detection was based on presence of function names in the plan
 *          node `description` field, which is available from the existing
 *          `QueryPlanNode` serialization path.  A deeper AST-level rewrite
 *          (replacing function call nodes in the AQL IR) requires coupling
 *          to the AQL runner's internal IR and is Phase 3 Phase-C work.
 * Activation: String-scan fallback is always active; IR visitor is used
 *             first when registered via setIRVisitorFn().
 * Production Delta: Phase 3 AQL runner wires a real IRVisitorFn that
 *                   traverses the AST IR directly, bypassing string scanning.
 * Removal Plan: String-scan fallback can be removed once all callers supply
 *               an IR visitor (Phase 3 completion, Q1 2027).
 */

#include "query/tensor_aware_query_optimizer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <mutex>
#include <sstream>
#include <string>
#include <stdexcept>

namespace themis {
namespace query {

// ============================================================================
// AST visitor bridge (stub #275)
// ============================================================================

namespace {
    static std::mutex s_ast_visitor_fn_mutex;
    static TensorAwareQueryOptimizer::AstVisitorFn s_ast_visitor_fn;
} // namespace

void TensorAwareQueryOptimizer::setAstVisitorFn(AstVisitorFn fn) {
    std::lock_guard<std::mutex> lock(s_ast_visitor_fn_mutex);
    s_ast_visitor_fn = std::move(fn);
}

void TensorAwareQueryOptimizer::clearAstVisitorFn() {
    std::lock_guard<std::mutex> lock(s_ast_visitor_fn_mutex);
    s_ast_visitor_fn = nullptr;
}

static TensorAwareQueryOptimizer::AstVisitorFn getAstVisitorFn() {
    std::lock_guard<std::mutex> lock(s_ast_visitor_fn_mutex);
    return s_ast_visitor_fn;
}

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

// IR visitor bridge — process-wide singleton, guarded by ir_visitor_mutex_.
TensorAwareQueryOptimizer::IRVisitorFn TensorAwareQueryOptimizer::ir_visitor_fn_;
std::mutex TensorAwareQueryOptimizer::ir_visitor_mutex_;

// ============================================================================
// setIRVisitorFn / clearIRVisitorFn
// ============================================================================

void TensorAwareQueryOptimizer::setIRVisitorFn(IRVisitorFn fn) {
    std::lock_guard<std::mutex> lock(ir_visitor_mutex_);
    ir_visitor_fn_ = std::move(fn);
}

void TensorAwareQueryOptimizer::clearIRVisitorFn() {
    std::lock_guard<std::mutex> lock(ir_visitor_mutex_);
    ir_visitor_fn_ = nullptr;
}

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

void TensorAwareQueryOptimizer::setTensorNodeDetectorFn(TensorNodeDetectorFn fn) {
    std::unique_lock lock(detector_mutex_);
    tensor_node_detector_fn_ = std::move(fn);
}

void TensorAwareQueryOptimizer::clearTensorNodeDetectorFn() {
    std::unique_lock lock(detector_mutex_);
    tensor_node_detector_fn_ = nullptr;
}

// ============================================================================
// rewriteNode — depth-first DFS
// ============================================================================

void TensorAwareQueryOptimizer::rewriteNode(QueryPlanNode& node) {
    ++last_stats_.nodes_visited;

    // ── Step 1: AQL-IR visitor bridge (stub #275 resolution) ──────────────
    // If a real AQL-IR visitor is registered, let it detect and rewrite the
    // node before falling back to the description-string scan.
    {
        IRVisitorFn visitor_snap;
        {
            std::lock_guard<std::mutex> lock(ir_visitor_mutex_);
            visitor_snap = ir_visitor_fn_;
        }
        if (visitor_snap) {
            double baseline_cost_out = 0.0;
            try {
                if (visitor_snap(node, baseline_cost_out)) {
                    // Visitor rewrote the node; record stats and skip string scan.
                    last_stats_.total_baseline_cost  += baseline_cost_out;
                    last_stats_.total_optimized_cost += node.estimated_cost;
                    ++last_stats_.nodes_rewritten;

                    // Recurse into children and return early.
                    for (auto& child : node.children) {
                        if (child) rewriteNode(*child);
                    }
                    return;
                }
            } catch (...) {
                // Visitor threw; fall through to string-scan heuristic.
            }
        }
    }

    TensorNodeDetectorFn detector;
    {
        std::shared_lock lock(detector_mutex_);
        detector = tensor_node_detector_fn_;
    }

    std::optional<std::string> detected_fn;
    if (detector) {
        try {
            detected_fn = detector(node);
        } catch (...) {
            // Fail closed to deterministic description scan below.
            detected_fn.reset();
        }
    }

    std::string matched_function;
    if (detected_fn.has_value() && isTensorFunction(*detected_fn)) {
        matched_function = *detected_fn;
    } else {
        // Check whether this node's description mentions a tensor function.
        std::string upper_desc;
        upper_desc.reserve(node.description.size());
        for (char c : node.description)
            upper_desc += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        for (const auto& fn : kTensorFunctions) {
            if (upper_desc.find(fn) != std::string::npos) {
                matched_function = fn;
                break;
            }
        }
    }

    if (!matched_function.empty()) {
        // Estimate costs.
        // Use heuristic parameters; in Phase 3 these will be derived from
        // the AQL IR's actual tensor operand metadata.
        constexpr std::size_t kDefaultOrder    = 4;
        constexpr std::size_t kDefaultModeSize = 16;
        constexpr std::size_t kDefaultRank     = 8;

        const double tt_cost = estimateTTCost(matched_function,
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
    }

    // Recurse into children.
    for (auto& child : node.children) {
        if (child) rewriteNode(*child);
    }

    // Invoke the injected AST visitor (bridge injected; fn-based path available).
    if (auto visitor = getAstVisitorFn()) {
        visitor(node);
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

