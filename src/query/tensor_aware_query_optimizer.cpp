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
    for (char c : name) {
      upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return kTensorFunctions.count(upper) > 0;
}

// ============================================================================
// Safe multiplication helper
// ============================================================================

namespace {
    // [WAVE1-VERIFIED: multiplication_overflow — tensor_aware_query_optimizer.cpp:113,118,123]
    // safeMul() implements IEEE 754-safe double multiplication with overflow
    // detection.  All std::size_t inputs to estimateTTCost() are converted to
    // double before reaching safeMul(); the cast is safe (at most 53 bits of
    // precision are lost for size_t > 2^53, which cannot occur in practice
    // given the kMaxDim = 1e6 cap applied immediately after the cast).
    // The integer multiplication overflow gap is fully resolved by this design:
    // no integer arithmetic overflow path exists in these three functions.
    //
    /// Safe multiplication with overflow detection for doubles.
    /// Returns the product, clamped to DBL_MAX if overflow would occur.
    inline double safeMul(double a, double b) noexcept {
        constexpr double kMaxDouble = std::numeric_limits<double>::max();
        if (a == 0.0 || b == 0.0) {
          return 0.0;
        }
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
                        if (child) {
                          rewriteNode(*child);
                        }
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
        if (child) {
          rewriteNode(*child);
        }
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
    if (root) {
      rewriteNode(*root);
    }
    return root;
}

} // namespace query
} // namespace themis


// ============================================================================
// W9-12: planAnnGraphHybrid — Hybrid ANN+graph planner
// ============================================================================

#include "index/ann_frontdoor.h"
#include "themis/rag/kg/knowledge_graph_interface.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace query {

std::vector<HybridAnnGraphResult> planAnnGraphHybrid(
    const HybridAnnGraphQuery&              query,
    const index::AnnFrontdoor*              frontdoor,
    const themis::rag::kg::IKnowledgeGraph* kg)
{
    if (frontdoor && query.query_vector.empty()) {
        throw std::invalid_argument(
            "planAnnGraphHybrid: query_vector must not be empty when frontdoor is provided");
    }

    const auto t_start = std::chrono::steady_clock::now();
    auto elapsed_ms = [&t_start]() -> double {
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - t_start).count();
    };
    auto check_timeout = [&]() {
        if (query.timeout_ms.count() > 0 &&
            elapsed_ms() >= static_cast<double>(query.timeout_ms.count())) {
            throw std::runtime_error(
                "planAnnGraphHybrid: deadline exceeded (" +
                std::to_string(query.timeout_ms.count()) + "ms)");
        }
    };

    // ── Step 1: ANN retrieval ─────────────────────────────────────────────
    // ann_ranked: node_id → 0-based rank in ANN result list
    std::vector<std::string> ann_list;       // ordered by ANN rank
    std::unordered_map<std::string, int> ann_rank_map;

    if (frontdoor) {
        try {
            index::AnnFrontdoorResult ann_res = frontdoor->search(
                query.query_vector.data(),
                query.query_vector.size(),
                static_cast<int>(query.ann_k),
                query.ann_context);

            ann_list.reserve(ann_res.candidates.size());
            int rank = 0;
            for (const auto& cand : ann_res.candidates) {
                std::string id = std::to_string(cand.id);
                ann_list.push_back(id);
                ann_rank_map.emplace(std::move(id), rank++);
            }
            THEMIS_DEBUG("planAnnGraphHybrid: ANN step returned {} candidates in {:.1f}ms",
                         ann_list.size(), elapsed_ms());
        } catch (const std::exception& ex) {
            THEMIS_WARN("planAnnGraphHybrid: ANN step failed: {}; proceeding with graph-only", ex.what());
        }
    }

    check_timeout();

    // ── Step 2: Graph expansion ───────────────────────────────────────────
    // Expand neighbours from top ANN hits; collect ordered graph_list.
    std::vector<std::string> graph_list;    // ordered by graph discovery rank
    std::unordered_map<std::string, int> graph_rank_map;

    if (kg && !ann_list.empty()) {
        // Expand from the top min(ann_k, 32) ANN hits to bound graph cost.
        const std::size_t expand_limit = std::min(ann_list.size(), std::size_t{32});
        for (std::size_t i = 0; i < expand_limit; ++i) {
            check_timeout();
            const std::string& seed_id = ann_list[i];
            try {
                auto nbrs = kg->neighbours(
                    seed_id,
                    query.graph_max_depth,
                    query.graph_min_edge_weight,
                    query.graph_max_nodes);

                for (const auto& nbr_id : nbrs) {
                    if (graph_rank_map.count(nbr_id) == 0) {
                        int grank = static_cast<int>(graph_list.size());
                        graph_rank_map.emplace(nbr_id, grank);
                        graph_list.push_back(nbr_id);
                    }
                }
            } catch (const std::exception& ex) {
                THEMIS_WARN("planAnnGraphHybrid: graph expansion failed for seed='{}': {}",
                            seed_id, ex.what());
            }
        }
        THEMIS_DEBUG("planAnnGraphHybrid: graph expansion returned {} nodes in {:.1f}ms",
                     graph_list.size(), elapsed_ms());
    }

    check_timeout();

    // ── Step 3: RRF fusion ────────────────────────────────────────────────
    // RRF score(d) = Σ 1 / (rrf_k + rank_i(d))
    // Collect all unique node IDs and compute RRF score from both lists.
    std::unordered_map<std::string, HybridAnnGraphResult> fused;
    fused.reserve(ann_list.size() + graph_list.size());

    // Seed from ANN list
    for (int r = 0; r < static_cast<int>(ann_list.size()); ++r) {
        const auto& id = ann_list[r];
        auto& entry = fused[id];
        entry.node_id   = id;
        entry.ann_rank  = r;
        entry.rrf_score += 1.0 / (query.rrf_k + r + 1.0);
    }
    // Add from graph list
    for (int r = 0; r < static_cast<int>(graph_list.size()); ++r) {
        const auto& id = graph_list[r];
        auto& entry = fused[id];
        if (entry.node_id.empty()) {
            entry.node_id    = id;
            entry.from_graph = true;
        }
        entry.graph_rank  = r;
        entry.rrf_score  += 1.0 / (query.rrf_k + r + 1.0);
    }

    // ── Step 4: Sort + truncate ───────────────────────────────────────────
    std::vector<HybridAnnGraphResult> results;
    results.reserve(fused.size());
    for (auto& [id, entry] : fused) {
        results.push_back(std::move(entry));
    }
    std::sort(results.begin(), results.end(),
              [](const HybridAnnGraphResult& a, const HybridAnnGraphResult& b) {
                  return a.rrf_score > b.rrf_score;  // descending
              });
    if (results.size() > query.top_k) {
        results.resize(query.top_k);
    }

    THEMIS_INFO("planAnnGraphHybrid: fused {} ANN + {} graph → {} results in {:.1f}ms",
                ann_list.size(), graph_list.size(), results.size(), elapsed_ms());

    return results;
}

} // namespace query
} // namespace themis

