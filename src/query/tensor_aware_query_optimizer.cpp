/**
 * @file tensor_aware_query_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Gap Summary: total=9; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "query/tensor_aware_query_optimizer.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <stdexcept>
#include "utils/logger.h"

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
// Safe multiplication helper
// ============================================================================

namespace {
    /// Safe multiplication with overflow detection for doubles.
    /// Returns the product, clamped to DBL_MAX if overflow would occur.
    inline double safeMul(double a, double b) noexcept {
        constexpr double kMaxDouble = std::numeric_limits<double>::max();
        if (a == 0.0 || b == 0.0) return 0.0;
        if (a > 0.0 && b > 0.0 && a > kMaxDouble / b) {
            return kMaxDouble;
        }
        if (a < 0.0 && b < 0.0 && a < kMaxDouble / b) {
            return kMaxDouble;
        }
        if (a > 0.0 && b < 0.0 && b < -kMaxDouble / a) {
            return -kMaxDouble;
        }
        if (a < 0.0 && b > 0.0 && a < -kMaxDouble / b) {
            return -kMaxDouble;
        }
        return a * b;
    }
} // namespace

// ============================================================================
// estimateTTCost
// ============================================================================

double TensorAwareQueryOptimizer::estimateTTCost(
    const std::string& function_name,
    std::size_t order,
    std::size_t mode_size,
    std::size_t max_rank) noexcept {

    // Complexity estimates from Holtz et al. (2012) / paper §AQL.
    // Cap inputs to prevent intermediate products from overflowing to infinity.
    // Maximum realistic tensor rank / mode size in production: 1e6 per axis.
    constexpr double kMaxDim = 1.0e6;
    const double d = std::min(static_cast<double>(order    > 0 ? order    : 4), kMaxDim);
    const double n = std::min(static_cast<double>(mode_size > 0 ? mode_size : 4), kMaxDim);
    const double r = std::min(static_cast<double>(max_rank  > 0 ? max_rank  : 8), kMaxDim);

    double cost = 0.0;
    if (function_name == "TENSOR_SIMILARITY" ||
        function_name == "TENSOR_NORM"        ||
        function_name == "TENSOR_CONTRACT") {
        // Inner-product / transfer-matrix: O(d·n·r³)
        // Use safe multiplication to prevent overflow
        cost = safeMul(d, safeMul(n, safeMul(r, safeMul(r, r))));
    } else if (function_name == "TENSOR_SLICE" ||
               function_name == "TENSOR_PROJECT") {
        // Slice / marginalize one core: O(d·n·r²)
        // Use safe multiplication to prevent overflow
        cost = safeMul(d, safeMul(n, safeMul(r, r)));
    } else if (function_name == "TENSOR_COMPRESS" ||
               function_name == "TENSOR_DECOMPOSE") {
        // TT-rounding / decomposition: O(d·r²·n·log n)
        cost = safeMul(safeMul(d, safeMul(r, r)), safeMul(n, std::log2(n + 1.0)));
    } else if (function_name == "TENSOR_INFO") {
        cost = safeMul(d, n);
    } else {
        // Unknown — use a generic linear estimate.
        cost = safeMul(d, safeMul(n, r));
    }

    // Guard against infinity/NaN from extreme-but-capped inputs; return a
    // large but finite sentinel that cost-based planning can still compare.
    if (!std::isfinite(cost)) {
        return std::numeric_limits<double>::max() / 2.0;
    }
    return cost;
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
                THEMIS_WARN("tensor_aware_query_optimizer: unhandled exception caught");
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
            THEMIS_WARN("tensor_aware_query_optimizer: unhandled exception caught");
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

