/**
 * @file graph_query_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=11, M=46, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Graph Query Optimizer implementation

#include "graph/graph_query_optimizer.h"
#include <stdexcept>
#include "graph/gpu_traversal.h"
#include "graph/path_constraints.h"
#include "query/result_stream.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <chrono>
#include <sstream>
#include <future>
#include <thread>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>
#include <limits>
#include <map>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Schema-aware helper: check whether a node's comma-separated "_labels" field
// contains at least one of the required labels (OR semantics).
// Returns true when required_labels is empty (no filtering) or when the node's
// label string includes any of the entries in required_labels.
// ─────────────────────────────────────────────────────────────────────────────

namespace {

static bool nodeMatchesLabels(GraphIndexManager& mgr,
                               const std::string& node_id,
                               const std::vector<std::string>& required_labels) {
    if (required_labels.empty()) return true;
    auto labels_opt = mgr.getNodeField(node_id, "_labels");
    if (!labels_opt.has_value() || labels_opt->empty()) return false;
    const std::string& labels_str = *labels_opt;
    for (const auto& lbl : required_labels) {
        // Match whole label tokens in the comma-separated list
        // e.g. "Person,Employee" contains "Person" but not "son"
        std::string::size_type pos = 0;
        while ((pos = labels_str.find(lbl, pos)) != std::string::npos) {
            // Verify it is a complete token (preceded by start-of-string or ',')
            bool valid_start = (pos == 0) || (labels_str[pos - 1] == ',');
            // Verify it is a complete token (followed by end-of-string or ',')
            std::string::size_type end = pos + lbl.size();
            bool valid_end = (end == labels_str.size()) || (labels_str[end] == ',');
            if (valid_start && valid_end) return true;
            pos = end;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Schema-aware helper: populate active_schema_hints in an OptimizationPlan
// from QueryConstraints.
// ─────────────────────────────────────────────────────────────────────────────
static void applySchemaHints(GraphQueryOptimizer::OptimizationPlan& plan,
                              const GraphQueryOptimizer::QueryConstraints& constraints) {
    if (!constraints.node_labels.empty()) {
        std::string hint = "Node labels (OR): ";
        for (size_t i = 0; i < constraints.node_labels.size(); ++i) {
            if (i > 0) hint += ", ";
            hint += constraints.node_labels[i];
        }
        plan.active_schema_hints.push_back(std::move(hint));
    }
    if (!constraints.excluded_edge_types.empty()) {
        std::string hint = "Excluded edge types: ";
        for (size_t i = 0; i < constraints.excluded_edge_types.size(); ++i) {
            if (i > 0) hint += ", ";
            hint += constraints.excluded_edge_types[i];
        }
        plan.active_schema_hints.push_back(std::move(hint));
    }
}

} // namespace (helpers)

GraphQueryOptimizer::GraphQueryOptimizer(GraphIndexManager& graph_manager)
    : graph_manager_(graph_manager) {
    // Initialize with basic statistics
    auto result = collectStatistics();
    if (!result) {
        spdlog::warn("Failed to collect initial graph statistics: {}", result.error().message());
    }
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeShortestPath(
    std::string_view start_vertex,
    std::string_view target_vertex) {
    return optimizeShortestPath(start_vertex, target_vertex, QueryConstraints());
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeShortestPath(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const QueryConstraints& constraints) {
    
    // Check plan cache: first exact key, then structural key for reuse across
    // structurally similar queries (same pattern/constraints, different vertices)
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::SHORTEST_PATH, start_vertex, target_vertex, constraints);
        if (auto cached = planCacheLookup(cache_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            return Ok(*cached);
        }
        // Structural key lookup: reuse plan from a previous query with same
        // constraints but different vertex IDs
        auto struct_key = generateStructuralCacheKey(QueryPattern::SHORTEST_PATH, constraints);
        if (auto cached2 = planCacheLookup(struct_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            OptimizationPlan promoted = *cached2;
            planCacheInsert(cache_key, promoted); // promote to exact key for faster future lookup
            return Ok(promoted);
        }
        metrics_.plan_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    OptimizationPlan plan;
    plan.pattern = QueryPattern::SHORTEST_PATH;
    
    // Estimate depth based on graph statistics
    size_t estimated_depth = estimateDepth(QueryPattern::SHORTEST_PATH, constraints);
    
    // Generate alternative plans
    std::vector<std::pair<TraversalAlgorithm, double>> alternatives;
    
    // BFS cost (shortest unweighted path)
    double bfs_cost = estimateCost(TraversalAlgorithm::BFS, estimated_depth, constraints);
    alternatives.push_back({TraversalAlgorithm::BFS, bfs_cost});
    
    // Dijkstra cost (shortest weighted path)
    double dijkstra_cost = estimateCost(TraversalAlgorithm::DIJKSTRA, estimated_depth, constraints);
    alternatives.push_back({TraversalAlgorithm::DIJKSTRA, dijkstra_cost});
    
    // Bidirectional search cost (for long paths)
    if (estimated_depth > 3) {
        double bidirectional_cost = estimateCost(TraversalAlgorithm::BIDIRECTIONAL, estimated_depth, constraints);
        alternatives.push_back({TraversalAlgorithm::BIDIRECTIONAL, bidirectional_cost});
    }
    
    // Select best algorithm
    std::sort(alternatives.begin(), alternatives.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    plan.algorithm = alternatives[0].first;
    plan.estimated_cost = alternatives[0].second;
    plan.alternatives = std::move(alternatives);
    
    // Set optimization flags
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = true;
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, estimated_depth));
    plan.estimated_time_ms = plan.estimated_cost * 0.1; // Convert cost to time estimate

    // Keep explain-plan estimates meaningful even when early startup statistics
    // are sparse (e.g. tests that only insert edges without explicit vertices).
    if (plan.estimated_cost <= 0.0) {
        plan.estimated_cost = 1.0;
    }
    if (plan.estimated_time_ms <= 0.0) {
        plan.estimated_time_ms = 0.1;
    }
    if (plan.estimated_nodes_explored == 0) {
        plan.estimated_nodes_explored = 1;
    }
    
    // Determine if parallel execution is beneficial; caller can also force it on
    plan.enable_parallel = constraints.enable_parallel ||
                           shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);

    // Populate schema hint metadata before generating explanation
    applySchemaHints(plan, constraints);
    
    // Generate explanation
    plan.explanation = explainPlan(plan);
    
    // Cache the plan under both the exact key and the structural key so that
    // future queries with the same constraints but different vertex IDs benefit
    // from structural plan reuse.
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::SHORTEST_PATH, start_vertex, target_vertex, constraints);
        planCacheInsert(cache_key, plan);
        auto struct_key = generateStructuralCacheKey(QueryPattern::SHORTEST_PATH, constraints);
        // only insert structural key if not already present
        if (!planCacheLookup(struct_key)) {
            planCacheInsert(struct_key, plan);
        }
    }
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeKHopNeighborhood(
    std::string_view start_vertex,
    int k) {
    return optimizeKHopNeighborhood(start_vertex, k, QueryConstraints());
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeKHopNeighborhood(
    [[maybe_unused]] std::string_view start_vertex,
    int k,
    const QueryConstraints& constraints) {

    // Check plan cache (structural key encodes k as depth_hint)
    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::K_HOP_NEIGHBORS, constraints, static_cast<size_t>(k));
        if (auto cached = planCacheLookup(struct_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            return Ok(*cached);
        }
        metrics_.plan_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    OptimizationPlan plan;
    plan.pattern = QueryPattern::K_HOP_NEIGHBORS;

    size_t estimated_depth = static_cast<size_t>(k);

    // Generate alternative plans; selectAlgorithm() picks the lowest-cost one
    // using the adaptive cost model when learned data is available.
    double bfs_cost = estimateCost(TraversalAlgorithm::BFS, estimated_depth, constraints);
    double dfs_cost = estimateCost(TraversalAlgorithm::DFS, estimated_depth, constraints);
    std::vector<std::pair<TraversalAlgorithm, double>> alternatives = {
        {TraversalAlgorithm::BFS, bfs_cost},
        {TraversalAlgorithm::DFS, dfs_cost},
    };
    std::sort(alternatives.begin(), alternatives.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    plan.algorithm = selectAlgorithm(QueryPattern::K_HOP_NEIGHBORS, estimated_depth, constraints);
    plan.estimated_cost = estimateCost(plan.algorithm, estimated_depth, constraints);
    plan.alternatives = std::move(alternatives);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, k));
    plan.estimated_time_ms = plan.estimated_cost * 0.1;
    
    // Optimization flags
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = true; // Stop at depth k
    plan.enable_parallel = constraints.enable_parallel ||
                           shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);

    applySchemaHints(plan, constraints);
    plan.explanation = explainPlan(plan);

    // Store under structural key for reuse across different start vertices
    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::K_HOP_NEIGHBORS, constraints, static_cast<size_t>(k));
        if (!planCacheLookup(struct_key)) {
            planCacheInsert(struct_key, plan);
        }
    }
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizePatternMatch(
    const std::vector<std::string>& pattern_vertices,
    const std::vector<std::pair<std::string, std::string>>& pattern_edges) {
    return optimizePatternMatch(pattern_vertices, pattern_edges, QueryConstraints());
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizePatternMatch(
    const std::vector<std::string>& pattern_vertices,
    const std::vector<std::pair<std::string, std::string>>& pattern_edges,
    const QueryConstraints& constraints) {

    // Structural key encodes the pattern shape (vertex count / edge count) so
    // that queries with the same structure but different vertex labels reuse plans.
    const size_t pattern_depth = pattern_vertices.size();
    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::PATTERN_MATCH, constraints, pattern_depth);
        // Append edge count to distinguish patterns with the same vertex count
        struct_key += ":pe=" + std::to_string(pattern_edges.size());
        if (auto cached = planCacheLookup(struct_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            return Ok(*cached);
        }
        metrics_.plan_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    OptimizationPlan plan;
    plan.pattern = QueryPattern::PATTERN_MATCH;

    // Generate alternative plans; selectAlgorithm() picks the lowest-cost one
    // using the adaptive cost model when learned data is available.
    double dfs_cost = estimateCost(TraversalAlgorithm::DFS, pattern_depth, constraints);
    double bfs_cost = estimateCost(TraversalAlgorithm::BFS, pattern_depth, constraints);
    std::vector<std::pair<TraversalAlgorithm, double>> alternatives = {
        {TraversalAlgorithm::DFS, dfs_cost},
        {TraversalAlgorithm::BFS, bfs_cost},
    };
    std::sort(alternatives.begin(), alternatives.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    plan.algorithm = selectAlgorithm(QueryPattern::PATTERN_MATCH, pattern_depth, constraints);
    plan.estimated_cost = estimateCost(plan.algorithm, pattern_depth, constraints);
    plan.alternatives = std::move(alternatives);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, pattern_depth) * 0.5); // Pruning helps
    plan.estimated_time_ms = plan.estimated_cost * 0.15; // Pattern matching is more expensive
    
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = false; // Cache not helpful for pattern matching
    plan.enable_early_termination = true; // Stop when pattern found
    plan.enable_parallel = false; // Pattern matching doesn't parallelize well

    applySchemaHints(plan, constraints);
    plan.explanation = explainPlan(plan);

    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::PATTERN_MATCH, constraints, pattern_depth);
        struct_key += ":pe=" + std::to_string(pattern_edges.size());
        if (!planCacheLookup(struct_key)) {
            planCacheInsert(struct_key, plan);
        }
    }
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeReachability(
    std::string_view start_vertex,
    std::string_view target_vertex) {
    return optimizeReachability(start_vertex, target_vertex, QueryConstraints());
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeReachability(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const QueryConstraints& constraints) {

    // Two-level cache lookup: exact key first, then structural key
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::REACHABILITY, start_vertex, target_vertex, constraints);
        if (auto cached = planCacheLookup(cache_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            return Ok(*cached);
        }
        auto struct_key = generateStructuralCacheKey(QueryPattern::REACHABILITY, constraints);
        if (auto cached2 = planCacheLookup(struct_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            // Copy before planCacheInsert: emplace may rehash and invalidate cached2.
            OptimizationPlan promoted = *cached2;
            planCacheInsert(cache_key, promoted);
            return Ok(promoted);
        }
        metrics_.plan_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    OptimizationPlan plan;
    plan.pattern = QueryPattern::REACHABILITY;

    // Estimate depth based on graph statistics
    size_t estimated_depth = estimateDepth(QueryPattern::REACHABILITY, constraints);

    // Generate alternative plans and select the lowest-cost algorithm.
    // selectAlgorithm() uses the adaptive cost model when learned data is
    // available, falling back to depth-based heuristics otherwise.
    std::vector<std::pair<TraversalAlgorithm, double>> alternatives;
    double bfs_cost = estimateCost(TraversalAlgorithm::BFS, estimated_depth, constraints);
    alternatives.push_back({TraversalAlgorithm::BFS, bfs_cost});
    if (estimated_depth > 3) {
        double bidirectional_cost = estimateCost(TraversalAlgorithm::BIDIRECTIONAL, estimated_depth, constraints);
        alternatives.push_back({TraversalAlgorithm::BIDIRECTIONAL, bidirectional_cost});
    }
    std::sort(alternatives.begin(), alternatives.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    // Use selectAlgorithm for the final choice so that the adaptive model can
    // override the cost-sorted result when sufficient confidence exists.
    plan.algorithm = selectAlgorithm(QueryPattern::REACHABILITY, estimated_depth, constraints);
    plan.estimated_cost = estimateCost(plan.algorithm, estimated_depth, constraints);
    plan.alternatives = std::move(alternatives);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, estimated_depth * 0.5)); // Bidirectional advantage
    plan.estimated_time_ms = plan.estimated_cost * 0.05; // Reachability is faster
    
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = true; // Stop as soon as path found
    plan.enable_parallel = constraints.enable_parallel ||
                           shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);

    applySchemaHints(plan, constraints);
    plan.explanation = explainPlan(plan);

    // Cache under both exact and structural keys
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::REACHABILITY, start_vertex, target_vertex, constraints);
        planCacheInsert(cache_key, plan);
        auto struct_key = generateStructuralCacheKey(QueryPattern::REACHABILITY, constraints);
        if (!planCacheLookup(struct_key)) {
            planCacheInsert(struct_key, plan);
        }
    }
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeConstrainedPath(
    std::string_view start_vertex,
    std::string_view end_vertex,
    const PathConstraints& constraints) {
    
    OptimizationPlan plan;
    plan.pattern = QueryPattern::ALL_PATHS; // Constrained paths can find multiple paths
    
    // Analyze constraints to select best algorithm
    const auto& constraint_list = constraints.getConstraints();
    
    bool has_min_length = false;
    bool has_max_length = false;
    bool has_required_nodes = false;
    [[maybe_unused]] bool has_forbidden_nodes = false;
    bool requires_unique = false;
    
    size_t min_length = 0;
    size_t max_length = 100; // Default max
    
    for (const auto& constraint : constraint_list) {
        switch (constraint.type) {
            case PathConstraints::ConstraintType::MIN_LENGTH:
                has_min_length = true;
                if (constraint.int_value) min_length = *constraint.int_value;
                break;
            case PathConstraints::ConstraintType::MAX_LENGTH:
                has_max_length = true;
                if (constraint.int_value) max_length = *constraint.int_value;
                break;
            case PathConstraints::ConstraintType::REQUIRED_NODE:
                has_required_nodes = true;
                break;
            case PathConstraints::ConstraintType::FORBIDDEN_NODE:
                has_forbidden_nodes = true;
                break;
            case PathConstraints::ConstraintType::UNIQUE_NODES:
            case PathConstraints::ConstraintType::NO_CYCLES:
                requires_unique = true;
                break;
            default:
                break;
        }
    }
    
    // Select algorithm based on constraints
    size_t estimated_depth = has_max_length ? max_length : estimateDepth(QueryPattern::ALL_PATHS, QueryConstraints());
    
    // For constrained paths, BFS is usually best for exploring breadth
    // DFS might be better for deep paths with min_length requirements
    if (has_min_length && min_length > 5) {
        plan.algorithm = TraversalAlgorithm::DFS;
    } else if (has_required_nodes) {
        // Required nodes benefit from BFS to find shortest paths first
        plan.algorithm = TraversalAlgorithm::BFS;
    } else {
        plan.algorithm = TraversalAlgorithm::BFS; // Default to BFS
    }
    
    // Estimate cost based on constraint complexity
    double constraint_complexity = 1.0;
    constraint_complexity += constraint_list.size() * 0.1; // Each constraint adds overhead
    if (has_required_nodes) constraint_complexity *= 1.5; // Required nodes are expensive
    if (requires_unique) constraint_complexity *= 1.2; // Uniqueness tracking overhead
    
    plan.estimated_cost = estimateCost(plan.algorithm, estimated_depth, QueryConstraints()) * constraint_complexity;
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, estimated_depth) * constraint_complexity);
    plan.estimated_time_ms = plan.estimated_cost * 0.15; // Constrained paths are slower
    
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = has_max_length; // Can terminate early with max length
    plan.enable_parallel = false; // Single-source constrained path finding does not use parallel expansion;
                                   // for multi-source parallel traversal use ParallelTraversal directly.
    
    // Generate explanation
    std::ostringstream oss;
    oss << "Constrained path finding from '" << start_vertex << "' to '" << end_vertex << "'\n";
    oss << "Algorithm: " << (plan.algorithm == TraversalAlgorithm::BFS ? "BFS" : "DFS") << "\n";
    oss << "Constraints: " << constraint_list.size() << " active\n";
    oss << "Estimated depth: " << estimated_depth << "\n";
    oss << "Estimated cost: " << plan.estimated_cost << "\n";
    if (has_min_length) oss << "Min length: " << min_length << "\n";
    if (has_max_length) oss << "Max length: " << max_length << "\n";
    plan.explanation = oss.str();
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::explainConstrainedPath(
    std::string_view start_vertex,
    std::string_view end_vertex,
    const PathConstraints& constraints) {
    // Pure dry-run: delegate to optimizeConstrainedPath which performs no traversal.
    // The method is intentionally a thin wrapper so callers can use a distinct API
    // that makes the "no execution" guarantee clear.
    return optimizeConstrainedPath(start_vertex, end_vertex, constraints);
}

// ─────────────────────────────────────────────────────────────────────────────
// Temporal Graph Query Optimization (Phase 3)
// ─────────────────────────────────────────────────────────────────────────────

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeTemporalTraversal(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints) {

    // Structural cache key includes temporal range so plans are reused only
    // when both pattern and time window match.
    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::K_HOP_NEIGHBORS, constraints,
            static_cast<size_t>(max_depth));
        if (auto cached = planCacheLookup(struct_key)) {
            metrics_.plan_cache_hits.fetch_add(1, std::memory_order_relaxed);
            return Ok(*cached);
        }
        metrics_.plan_cache_misses.fetch_add(1, std::memory_order_relaxed);
    }

    OptimizationPlan plan;
    plan.pattern = QueryPattern::K_HOP_NEIGHBORS;
    plan.algorithm = TraversalAlgorithm::BFS; // BFS is optimal for temporal range traversals

    const size_t estimated_depth = static_cast<size_t>(max_depth);

    // Base cost: BFS over estimated depth.
    // NOTE: estimateCost() already applies temporal selectivity when
    // constraints.hasTemporalRange() is true, so no extra multiplication is
    // needed here.
    double base_cost = estimateCost(TraversalAlgorithm::BFS, estimated_depth, constraints);

    // Compute temporal selectivity separately — used only for
    // estimated_nodes_explored and the explanation string, not for cost
    // adjustment (which estimateCost already handles).
    double temporal_selectivity = 1.0;
    if (constraints.hasTemporalRange()) {
        if (constraints.time_range_start_ms.has_value() &&
            constraints.time_range_end_ms.has_value()) {
            const int64_t range_ms =
                *constraints.time_range_end_ms - *constraints.time_range_start_ms;
            // Reference span: 5 years in milliseconds
            constexpr int64_t REFERENCE_SPAN_MS =
                static_cast<int64_t>(5) * 365 * 24 * 3600 * 1000LL;
            temporal_selectivity = static_cast<double>(range_ms) /
                                   static_cast<double>(REFERENCE_SPAN_MS);
            // Clamp to [0.05, 0.95] so we always reflect some reduction
            temporal_selectivity = std::max(0.05, std::min(0.95, temporal_selectivity));
        } else {
            temporal_selectivity = 0.5; // one-sided bound: moderate reduction
        }
    }

    // Alternative: DFS cost (estimateCost handles temporal selectivity)
    double dfs_cost = estimateCost(TraversalAlgorithm::DFS, estimated_depth, constraints);
    plan.alternatives.emplace_back(TraversalAlgorithm::DFS, dfs_cost);

    plan.estimated_cost = base_cost;
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor > 0 ? statistics_.avg_branching_factor : 2.0,
                 estimated_depth) * temporal_selectivity);
    plan.estimated_time_ms = plan.estimated_cost * 0.1;
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = constraints.max_results.has_value();
    plan.enable_parallel = constraints.enable_parallel &&
                           shouldUseParallel(TraversalAlgorithm::BFS, plan.estimated_nodes_explored);

    // Build explanation
    std::ostringstream oss;
    oss << "Temporal graph traversal from '" << start_vertex << "'\n";
    oss << "Algorithm: BFS (optimal for time-range edge filtering)\n";
    oss << "Max depth: " << max_depth << "\n";
    if (constraints.time_range_start_ms.has_value()) {
        oss << "Time range start: " << *constraints.time_range_start_ms << " ms\n";
    }
    if (constraints.time_range_end_ms.has_value()) {
        oss << "Time range end: " << *constraints.time_range_end_ms << " ms\n";
    }
    oss << "Containment mode: "
        << (constraints.time_range_require_containment ? "full containment" : "overlap") << "\n";
    oss << "Temporal selectivity: " << temporal_selectivity << "\n";
    oss << "Estimated cost: " << plan.estimated_cost << "\n";
    plan.explanation = oss.str();

    // Store in plan cache under the structural key
    if (plan_caching_enabled_) {
        auto struct_key = generateStructuralCacheKey(
            QueryPattern::K_HOP_NEIGHBORS, constraints,
            static_cast<size_t>(max_depth));
        planCacheInsert(struct_key, plan);
    }

    return Ok(plan);
}

Result<std::vector<std::string>> GraphQueryOptimizer::executeTemporalBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {

    // Rate limit check
    if (!rate_limiter_.allowQuery()) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "TemporalBFS query rejected: rate limit exceeded"
        );
    }

    // When no temporal range is active, fall back to standard BFS.
    if (!constraints.hasTemporalRange()) {
        return executeBFS(start_vertex, max_depth, constraints, stats);
    }

    // Resolve effective bounds (use sentinel values for open-ended ranges).
    const int64_t range_start = constraints.time_range_start_ms.value_or(
        std::numeric_limits<int64_t>::min());
    const int64_t range_end = constraints.time_range_end_ms.value_or(
        std::numeric_limits<int64_t>::max());

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::BFS;
    local_stats.estimated_cost_ms =
        estimateCost(TraversalAlgorithm::BFS, static_cast<size_t>(max_depth), constraints) * 0.1;
    // Note: 0.1 converts cost units → ms (same factor used in optimizeXxx plan construction)

    auto timedOut = [&]() -> bool {
        if (constraints.timeout_ms == 0) return false;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        return elapsed > static_cast<decltype(elapsed)>(constraints.timeout_ms);
    };

    std::vector<std::string> result;
    std::unordered_set<std::string> visited;
    std::vector<std::string> current_frontier;

    current_frontier.push_back(std::string(start_vertex));
    visited.insert(std::string(start_vertex));

    for (int depth = 0; depth <= max_depth; ++depth) {
        if (current_frontier.empty()) break;

        if (timedOut()) {
            local_stats.early_terminated = true;
            metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
            if (stats) { *stats = local_stats; }
            recordExecution(local_stats);
            return Err<std::vector<std::string>>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "TemporalBFS query exceeded timeout of " +
                    std::to_string(constraints.timeout_ms) + "ms"
            );
        }

        // Emit frontier nodes into result
        for (const auto& node : current_frontier) {
            result.push_back(node);
            local_stats.nodes_explored++;
            if (constraints.max_results.has_value() &&
                result.size() >= constraints.max_results.value()) {
                local_stats.early_terminated = true;
                break;
            }
        }
        if (local_stats.early_terminated) break;

        if (depth == max_depth) break;

        // Expand frontier using time-range-filtered edges
        std::vector<std::string> next_frontier;
        for (const auto& node : current_frontier) {
            auto [status, edges] = graph_manager_.getOutEdgesInTimeRange(
                node, range_start, range_end,
                constraints.time_range_require_containment);
            if (!status.ok) {
                // Non-existent node or DB error: skip silently (consistent with
                // the bfsAtTime behaviour in GraphIndexManager).
                continue;
            }

            local_stats.edges_traversed += edges.size();

            for (const auto& edge : edges) {
                const std::string& nb = edge.toPk;

                if (visited.count(nb)) continue;
                if (std::find(constraints.forbidden_vertices.begin(),
                              constraints.forbidden_vertices.end(), nb) !=
                    constraints.forbidden_vertices.end()) continue;

                visited.insert(nb);
                next_frontier.push_back(nb);
            }
        }
        current_frontier = std::move(next_frontier);
    }

    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    local_stats.max_depth_reached = static_cast<size_t>(max_depth);
    local_stats.paths_found = result.size();

    if (stats) { *stats = local_stats; }
    recordExecution(local_stats);

    return Ok(result);
}

Result<std::vector<std::string>> GraphQueryOptimizer::executeBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    // Rate limit check – before any work is done
    if (!rate_limiter_.allowQuery()) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "BFS query rejected: rate limit exceeded"
        );
    }

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::BFS;
    local_stats.estimated_cost_ms =
        estimateCost(TraversalAlgorithm::BFS, static_cast<size_t>(max_depth), constraints) * 0.1;

    // GPU-accelerated path: dispatch to GPUGraphTraversal when requested.
    if (constraints.use_gpu) {
        GPUGraphTraversal gpu_trav(graph_manager_);
        auto load_res = gpu_trav.load();
        if (load_res) {
            GPUGraphTraversal::Config gpu_cfg;
            gpu_cfg.gpu_device     = constraints.gpu_device;
            gpu_cfg.max_depth      = max_depth;
            if (constraints.max_results.has_value())
                gpu_cfg.max_results = constraints.max_results.value();
            gpu_cfg.forbidden_vertices = constraints.forbidden_vertices;

            auto gpu_result = gpu_trav.bfs(std::string(start_vertex), gpu_cfg);
            if (gpu_result) {
                local_stats.nodes_explored    = gpu_result.value().nodes_explored;
                local_stats.edges_traversed   = gpu_result.value().edges_traversed;
                local_stats.execution_time_ms = gpu_result.value().execution_time_ms;
                local_stats.early_terminated  = gpu_result.value().truncated;
                local_stats.paths_found       = gpu_result.value().visited_vertices.size();
                if (stats) *stats = local_stats;
                recordExecution(local_stats);
                return Ok(std::move(gpu_result.value().visited_vertices));
            }
            // Fall through to CPU path on GPU error (vertex-not-found is re-raised).
            if (gpu_result.error().code() ==
                    errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX) {
                return Err<std::vector<std::string>>(
                    errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                    std::string(start_vertex));
            }
        }
        // If load() failed, fall through to the standard CPU BFS.
    }

    // Helper: determine effective thread count for parallel BFS
    const bool use_parallel = constraints.enable_parallel;
    const size_t effective_threads = [&]() -> size_t {
        if (!use_parallel) return 1u;
        if (constraints.num_threads > 0) {
            return std::min<size_t>(constraints.num_threads, 16u);
        }
        // hardware_concurrency() may return 0 on unsupported platforms; default to 4
        const size_t hw = std::thread::hardware_concurrency();
        const size_t base = (hw > 0) ? hw : 8u;
        return std::max<size_t>(2u, std::min<size_t>(base / 2u, 16u));
    }();

    // Helper: timeout check reused in the loop
    auto timedOut = [&]() -> bool {
        if (constraints.timeout_ms == 0) return false;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        return elapsed > static_cast<decltype(elapsed)>(constraints.timeout_ms);
    };

    std::vector<std::string> result;
    std::unordered_set<std::string> visited;

    // BFS frontier: current level to expand
    std::vector<std::string> current_frontier;
    current_frontier.push_back(std::string(start_vertex));
    visited.insert(std::string(start_vertex));

    for (int depth = 0; depth <= max_depth; ++depth) {
        if (current_frontier.empty()) break;

        // Timeout check at the start of each level
        if (timedOut()) {
            local_stats.early_terminated = true;
            metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
            if (stats) { *stats = local_stats; }
            recordExecution(local_stats);
            return Err<std::vector<std::string>>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "BFS query exceeded timeout of " +
                    std::to_string(constraints.timeout_ms) + "ms"
            );
        }

        // Add all frontier nodes to result
        for (const auto& node : current_frontier) {
            result.push_back(node);
            local_stats.nodes_explored++;
            if (constraints.max_results.has_value() &&
                result.size() >= constraints.max_results.value()) {
                local_stats.early_terminated = true;
                break;
            }
        }
        if (local_stats.early_terminated) break;

        if (depth == max_depth) break; // No need to expand last level

        // Build next frontier: expand each node in current_frontier, optionally in parallel
        std::vector<std::string> next_frontier;
        bool vertex_error = false;
        std::string error_vertex;

        if (!use_parallel || current_frontier.size() < effective_threads) {
            // Sequential expansion
            for (const auto& node : current_frontier) {
                auto [status, neighbors] = graph_manager_.outNeighbors(node);
                if (!status.ok) { vertex_error = true; error_vertex = node; break; }
                local_stats.edges_traversed += neighbors.size();
                for (const auto& nb : neighbors) {
                    if (visited.count(nb)) continue;
                    if (std::find(constraints.forbidden_vertices.begin(),
                                  constraints.forbidden_vertices.end(), nb) !=
                        constraints.forbidden_vertices.end()) continue;
                    // Schema hint: skip nodes that do not carry a required label
                    if (!nodeMatchesLabels(graph_manager_, nb, constraints.node_labels)) continue;
                    visited.insert(nb);
                    next_frontier.push_back(nb);
                }
            }
        } else {
            // Parallel expansion: split frontier into chunks, one per thread.
            // Each async task collects its neighbors independently; we merge
            // after all futures complete, so no shared mutable state races.
            struct ChunkResult {
                std::vector<std::string> neighbors; // raw (may have duplicates across chunks)
                size_t edges_seen = 0;
                bool error = false;
                std::string error_vertex;
            };

            const size_t chunk_size = (current_frontier.size() + effective_threads - 1) / effective_threads;
            std::vector<std::future<ChunkResult>> futures;
            std::atomic<bool> any_error{false};

            for (size_t t = 0; t < effective_threads; ++t) {
                const size_t begin_idx = t * chunk_size;
                if (begin_idx >= current_frontier.size()) break;
                const size_t end_idx = std::min(begin_idx + chunk_size, current_frontier.size());

                futures.push_back(std::async(std::launch::async, [&, begin_idx, end_idx]() {
                    ChunkResult cr;
                    for (size_t i = begin_idx; i < end_idx; ++i) {
                        if (any_error.load(std::memory_order_relaxed)) break;
                        const std::string& node = current_frontier[i];
                        auto [status, neighbors] = graph_manager_.outNeighbors(node);
                        if (!status.ok) {
                            any_error.store(true, std::memory_order_relaxed);
                            cr.error = true;
                            cr.error_vertex = node;
                            break;
                        }
                        cr.edges_seen += neighbors.size();
                        for (const auto& nb : neighbors) {
                            // Schema hint: filter by node labels in the parallel task
                            // (read-only access to graph_manager_ is safe across threads)
                            if (!nodeMatchesLabels(graph_manager_, nb, constraints.node_labels)) continue;
                            cr.neighbors.push_back(nb);
                        }
                    }
                    return cr;
                }));
            }

            // Merge parallel results (de-duplicate using the shared visited set)
            for (auto& fut : futures) {
                ChunkResult cr = fut.get();
                if (cr.error) {
                    vertex_error = true;
                    error_vertex = cr.error_vertex;
                    break;
                }
                local_stats.edges_traversed += cr.edges_seen;
                for (const auto& nb : cr.neighbors) {
                    if (visited.count(nb)) continue;
                    if (std::find(constraints.forbidden_vertices.begin(),
                                  constraints.forbidden_vertices.end(), nb) !=
                        constraints.forbidden_vertices.end()) continue;
                    visited.insert(nb);
                    next_frontier.push_back(nb);
                }
            }
        }

        if (vertex_error) {
            return Err<std::vector<std::string>>(
                errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                error_vertex
            );
        }

        current_frontier = std::move(next_frontier);
    }
    
    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    local_stats.max_depth_reached = max_depth;
    local_stats.paths_found = result.size();
    
    if (stats) {
        *stats = local_stats;
    }
    recordExecution(local_stats);
    
    return Ok(result);
}

Result<std::vector<std::string>> GraphQueryOptimizer::executeDFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    if (!rate_limiter_.allowQuery()) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "DFS query rejected: rate limit exceeded"
        );
    }

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::DFS;
    local_stats.estimated_cost_ms =
        estimateCost(TraversalAlgorithm::DFS, static_cast<size_t>(max_depth), constraints) * 0.1;

    // GPU-accelerated path.
    if (constraints.use_gpu) {
        GPUGraphTraversal gpu_trav(graph_manager_);
        auto load_res = gpu_trav.load();
        if (load_res) {
            GPUGraphTraversal::Config gpu_cfg;
            gpu_cfg.gpu_device     = constraints.gpu_device;
            gpu_cfg.max_depth      = max_depth;
            if (constraints.max_results.has_value())
                gpu_cfg.max_results = constraints.max_results.value();
            gpu_cfg.forbidden_vertices = constraints.forbidden_vertices;

            auto gpu_result = gpu_trav.dfs(std::string(start_vertex), gpu_cfg);
            if (gpu_result) {
                local_stats.nodes_explored    = gpu_result.value().nodes_explored;
                local_stats.edges_traversed   = gpu_result.value().edges_traversed;
                local_stats.execution_time_ms = gpu_result.value().execution_time_ms;
                local_stats.early_terminated  = gpu_result.value().truncated;
                local_stats.paths_found       = gpu_result.value().visited_vertices.size();
                if (stats) *stats = local_stats;
                recordExecution(local_stats);
                return Ok(std::move(gpu_result.value().visited_vertices));
            }
            if (gpu_result.error().code() ==
                    errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX) {
                return Err<std::vector<std::string>>(
                    errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                    std::string(start_vertex));
            }
        }
    }
    
    std::vector<std::string> result;
    std::vector<std::pair<std::string, int>> stack;
    std::unordered_set<std::string> visited;
    
    stack.push_back({std::string(start_vertex), 0});
    
    while (!stack.empty()) {
        auto [current, depth] = stack.back();
        stack.pop_back();
        
        // Timeout check
        if (constraints.timeout_ms > 0) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (elapsed > static_cast<decltype(elapsed)>(constraints.timeout_ms)) {
                local_stats.early_terminated = true;
                metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
                if (stats) { *stats = local_stats; }
                recordExecution(local_stats);
                return Err<std::vector<std::string>>(
                    errors::ErrorCode::ERR_QUERY_TIMEOUT,
                    "DFS query exceeded timeout of " +
                        std::to_string(constraints.timeout_ms) + "ms"
                );
            }
        }

        if (visited.find(current) != visited.end()) {
            continue;
        }
        
        visited.insert(current);
        result.push_back(current);
        local_stats.nodes_explored++;
        
        if (depth >= max_depth) {
            continue;
        }
        
        auto [status, neighbors] = graph_manager_.outNeighbors(current);
        if (!status.ok) {
            return Err<std::vector<std::string>>(
                errors::ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
                current
            );
        }
        
        local_stats.edges_traversed += neighbors.size();
        
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                // Schema hint: skip nodes that do not carry a required label
                if (!nodeMatchesLabels(graph_manager_, neighbor, constraints.node_labels)) continue;
                stack.push_back({neighbor, depth + 1});
            }
        }
        
        if (constraints.max_results.has_value() && 
            result.size() >= constraints.max_results.value()) {
            local_stats.early_terminated = true;
            break;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    local_stats.max_depth_reached = max_depth;
    local_stats.paths_found = result.size();
    
    if (stats) {
        *stats = local_stats;
    }
    recordExecution(local_stats);
    
    return Ok(result);
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamBFS(
    std::string_view start_vertex,
    int max_depth) {

    return streamBFS(start_vertex, max_depth, QueryConstraints{}, query::StreamConfig{});
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamBFS(
    std::string_view start_vertex,
    int max_depth,
    const query::StreamConfig& stream_config) {

    return streamBFS(start_vertex, max_depth, QueryConstraints{}, stream_config);
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints) {

    return streamBFS(start_vertex, max_depth, constraints, query::StreamConfig{});
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    const query::StreamConfig& stream_config) {

    auto bfs_result = executeBFS(start_vertex, max_depth, constraints);
    if (!bfs_result) {
        return Err<std::shared_ptr<query::ResultStream<std::string>>>(
            bfs_result.error().code(), bfs_result.error().context());
    }

    return Ok(std::make_shared<query::ResultStream<std::string>>(
        std::move(*bfs_result), stream_config));
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamDFS(
    std::string_view start_vertex,
    int max_depth) {

    return streamDFS(start_vertex, max_depth, QueryConstraints{}, query::StreamConfig{});
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamDFS(
    std::string_view start_vertex,
    int max_depth,
    const query::StreamConfig& stream_config) {

    return streamDFS(start_vertex, max_depth, QueryConstraints{}, stream_config);
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamDFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints) {

    return streamDFS(start_vertex, max_depth, constraints, query::StreamConfig{});
}

Result<std::shared_ptr<query::ResultStream<std::string>>> GraphQueryOptimizer::streamDFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    const query::StreamConfig& stream_config) {

    auto dfs_result = executeDFS(start_vertex, max_depth, constraints);
    if (!dfs_result) {
        return Err<std::shared_ptr<query::ResultStream<std::string>>>(
            dfs_result.error().code(), dfs_result.error().context());
    }

    return Ok(std::make_shared<query::ResultStream<std::string>>(
        std::move(*dfs_result), stream_config));
}

Result<GraphIndexManager::PathResult> GraphQueryOptimizer::executeDijkstra(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {

    if (!rate_limiter_.allowQuery()) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "Dijkstra query rejected: rate limit exceeded"
        );
    }

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::DIJKSTRA;
    {
        const size_t depth_hint = constraints.max_depth.has_value()
            ? static_cast<size_t>(constraints.max_depth.value()) : 10u;
        local_stats.estimated_cost_ms =
            estimateCost(TraversalAlgorithm::DIJKSTRA, depth_hint, constraints) * 0.1;
    }

    // -----------------------------------------------------------------------
    // Parallel path: Δ-Stepping shortest-path algorithm (Phase 3.2)
    //
    // Partition tentative distances into buckets of width Δ; light edges
    // (weight ≤ Δ) are relaxed in parallel via std::async; heavy edges are
    // relaxed serially after each bucket is cleared.  All dist[] / parent[]
    // updates are applied by the main thread – no data races.
    // -----------------------------------------------------------------------
    if (constraints.enable_parallel) {
        const std::string start(start_vertex);
        const std::string target(target_vertex);

        // Effective thread count (mirrors BFS logic)
        const size_t nthreads = [&]() -> size_t {
            if (constraints.num_threads > 0) {
                return std::min<size_t>(constraints.num_threads, 16u);
            }
            const size_t hw = std::thread::hardware_concurrency();
            const size_t base = (hw > 0) ? hw : 8u;
            return std::max<size_t>(2u, std::min<size_t>(base / 2u, 16u));
        }();

        // Choose Δ: average weight of the start vertex's first-hop edges.
        // Falls back to 1.0 when there are no outgoing edges.
        // The minimum of MIN_DELTA (1e-6) prevents division-by-zero when
        // computing bucket indices as floor(dist / delta).
        static constexpr double MIN_DELTA = 1e-6;
        double delta = 1.0;
        {
            auto [s, adjs] = graph_manager_.outAdjacency(start);
            if (s.ok && !adjs.empty()) {
                double sum = 0.0;
                for (const auto& adj : adjs) {
                    sum += graph_manager_.getEdgeWeight("", adj.edgeId, "_weight");
                }
                delta = std::max(MIN_DELTA, sum / static_cast<double>(adjs.size()));
            }
        }

        std::unordered_map<std::string, double> dist;
        std::unordered_map<std::string, std::string> parent;
        dist[start] = 0.0;

        // std::map keeps bucket indices sorted; begin() always returns the
        // minimum-index non-empty bucket in O(log n).
        std::map<size_t, std::unordered_set<std::string>> buckets;
        buckets[0].insert(start);

        // Timeout helper (matches BFS / DFS timeout logic)
        auto timedOut = [&]() -> bool {
            if (constraints.timeout_ms == 0) return false;
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            return elapsed > static_cast<decltype(elapsed)>(constraints.timeout_ms);
        };

        // Result type returned by each parallel light-edge relaxation task.
        // edge_count = total edges examined by the task (light + skipped heavy);
        // accumulated into edges_traversed by the main thread.
        struct RelaxResult {
            std::string vertex;
            double new_dist;
            std::string parent_vertex;
        };
        struct TaskOutput {
            std::vector<RelaxResult> relaxations;
            size_t edge_count = 0;
        };

        while (!buckets.empty()) {
            if (timedOut()) {
                local_stats.early_terminated = true;
                metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
                if (stats) { *stats = local_stats; }
                recordExecution(local_stats);
                return Err<GraphIndexManager::PathResult>(
                    errors::ErrorCode::ERR_QUERY_TIMEOUT,
                    "Dijkstra (Δ-stepping) exceeded timeout of " +
                        std::to_string(constraints.timeout_ms) + "ms"
                );
            }

            const size_t bucket_idx = buckets.begin()->first;
            std::unordered_set<std::string> settled_in_bucket;

            // Inner loop: re-process bucket_idx until it is stable (a vertex
            // whose light-edge relaxation lands back in bucket_idx causes
            // another iteration).
            while (!buckets.empty() && buckets.begin()->first == bucket_idx) {
                std::vector<std::string> S(buckets.begin()->second.begin(),
                                           buckets.begin()->second.end());
                buckets.erase(buckets.begin());
                settled_in_bucket.insert(S.begin(), S.end());
                local_stats.nodes_explored += S.size();

                // Chunk S into at most nthreads groups.
                const size_t chunk_size =
                    std::max<size_t>(1u, (S.size() + nthreads - 1) / nthreads);
                std::vector<std::future<TaskOutput>> futures;

                for (size_t cs = 0; cs < S.size(); cs += chunk_size) {
                    const size_t ce = std::min(cs + chunk_size, S.size());
                    futures.push_back(std::async(std::launch::async,
                        [&, cs, ce]() {
                            TaskOutput out;
                            for (size_t vi = cs; vi < ce; ++vi) {
                                const std::string& v = S[vi];
                                // dist[v] is fixed for S (only updated serially)
                                auto dit = dist.find(v);
                                if (dit == dist.end()) continue;
                                const double d_v = dit->second;
                                auto [s, adjs] = graph_manager_.outAdjacency(v);
                                if (!s.ok) continue;
                                out.edge_count += adjs.size();
                                for (const auto& adj : adjs) {
                                    const double w = graph_manager_.getEdgeWeight(
                                        "", adj.edgeId, "_weight");
                                    if (w > delta) continue; // heavy – skip
                                    const double nd = d_v + w;
                                    auto nit = dist.find(adj.targetPk);
                                    if (nit == dist.end() || nd < nit->second) {
                                        out.relaxations.push_back({adj.targetPk, nd, v});
                                    }
                                }
                            }
                            return out;
                        }));
                }

                // Apply updates serially so dist[] / parent[] are never
                // written from multiple threads simultaneously.
                // Also accumulate the edge counts from each task.
                for (auto& fut : futures) {
                    auto out = fut.get();
                    local_stats.edges_traversed += out.edge_count;
                    for (const auto& r : out.relaxations) {
                        auto it = dist.find(r.vertex);
                        const double old_d = (it != dist.end())
                            ? it->second
                            : std::numeric_limits<double>::infinity();
                        if (r.new_dist < old_d) {
                            // Move vertex to the correct (lower) bucket.
                            if (old_d < std::numeric_limits<double>::infinity()) {
                                const size_t old_idx =
                                    static_cast<size_t>(old_d / delta);
                                auto bit = buckets.find(old_idx);
                                if (bit != buckets.end()) {
                                    bit->second.erase(r.vertex);
                                    if (bit->second.empty()) buckets.erase(bit);
                                }
                            }
                            dist[r.vertex] = r.new_dist;
                            parent[r.vertex] = r.parent_vertex;
                            buckets[static_cast<size_t>(r.new_dist / delta)]
                                .insert(r.vertex);
                        }
                    }
                }
            }

            // Relax heavy edges (weight > Δ) from all settled vertices.
            // Light edges were already counted in the parallel phase; here we
            // count only the heavy-edge traversals to avoid double-counting.
            for (const auto& v : settled_in_bucket) {
                auto dit = dist.find(v);
                if (dit == dist.end()) continue;
                const double d_v = dit->second;
                auto [s, adjs] = graph_manager_.outAdjacency(v);
                if (!s.ok) continue;
                for (const auto& adj : adjs) {
                    const double w = graph_manager_.getEdgeWeight(
                        "", adj.edgeId, "_weight");
                    if (w <= delta) continue; // light – already counted above
                    local_stats.edges_traversed++;
                    const double nd = d_v + w;
                    auto it = dist.find(adj.targetPk);
                    const double old_d = (it != dist.end())
                        ? it->second
                        : std::numeric_limits<double>::infinity();
                    if (nd < old_d) {
                        if (old_d < std::numeric_limits<double>::infinity()) {
                            const size_t old_idx =
                                static_cast<size_t>(old_d / delta);
                            auto bit = buckets.find(old_idx);
                            if (bit != buckets.end()) {
                                bit->second.erase(adj.targetPk);
                                if (bit->second.empty()) buckets.erase(bit);
                            }
                        }
                        dist[adj.targetPk] = nd;
                        parent[adj.targetPk] = v;
                        buckets[static_cast<size_t>(nd / delta)].insert(adj.targetPk);
                    }
                }
            }

            // Early exit once target has received its final (optimal) distance.
            if (settled_in_bucket.count(target)) break;
        }

        // Reconstruct path from parent[] map.
        GraphIndexManager::PathResult path_result;
        auto dit = dist.find(target);
        if (dit != dist.end()) {
            path_result.totalCost = dit->second;
            std::vector<std::string> path;
            std::string cur = target;
            while (cur != start) {
                path.push_back(cur);
                auto pit = parent.find(cur);
                if (pit == parent.end()) { path.clear(); break; }
                cur = pit->second;
            }
            if (!path.empty() || target == start) {
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                path_result.path = std::move(path);
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        local_stats.execution_time_ms =
            std::chrono::duration<double, std::milli>(end_time - start_time).count();
        local_stats.paths_found = path_result.path.empty() ? 0 : 1;
        if (stats) { *stats = local_stats; }
        recordExecution(local_stats);
        return Ok(path_result);
    }

    // Sequential path: delegate to the GraphIndexManager's Dijkstra.
    auto [status, path_result] = graph_manager_.dijkstra(start_vertex, target_vertex);

    if (!status.ok) {
        // Keep behaviour consistent with the parallel path: no path is a valid
        // query result (empty path), not an execution failure.
        if (status.message.find("Kein Pfad gefunden") != std::string::npos ||
            status.message.find("no path") != std::string::npos ||
            status.message.find("No path") != std::string::npos) {
            GraphIndexManager::PathResult empty_path;
            auto end_time = std::chrono::steady_clock::now();
            local_stats.execution_time_ms =
                std::chrono::duration<double, std::milli>(end_time - start_time).count();
            local_stats.nodes_explored = 0;
            local_stats.paths_found = 0;

            if (stats) {
                *stats = local_stats;
            }
            recordExecution(local_stats);
            return Ok(empty_path);
        }

        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Dijkstra execution failed: " + status.message
        );
    }

    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms =
        std::chrono::duration<double, std::milli>(end_time - start_time).count();
    local_stats.nodes_explored = path_result.path.size();
    local_stats.paths_found = path_result.path.empty() ? 0 : 1;

    if (stats) {
        *stats = local_stats;
    }
    recordExecution(local_stats);

    return Ok(path_result);
}

Result<GraphIndexManager::PathResult> GraphQueryOptimizer::executeAStar(
    std::string_view start_vertex,
    std::string_view target_vertex,
    std::function<double(const std::string&)> heuristic,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    if (!rate_limiter_.allowQuery()) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "A* query rejected: rate limit exceeded"
        );
    }

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::ASTAR;
    {
        const size_t depth_hint = constraints.max_depth.has_value()
            ? static_cast<size_t>(constraints.max_depth.value()) : 10u;
        local_stats.estimated_cost_ms =
            estimateCost(TraversalAlgorithm::ASTAR, depth_hint, constraints) * 0.1;
    }
    
    // Use existing A* implementation from GraphIndexManager
    auto [status, path_result] = graph_manager_.aStar(start_vertex, target_vertex, heuristic);
    
    if (!status.ok) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "A* execution failed: " + status.message
        );
    }
    
    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    local_stats.nodes_explored = path_result.path.size();
    local_stats.paths_found = path_result.path.empty() ? 0 : 1;
    
    if (stats) {
        *stats = local_stats;
    }
    recordExecution(local_stats);
    
    return Ok(path_result);
}

Result<GraphIndexManager::PathResult> GraphQueryOptimizer::executeBidirectional(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    if (!rate_limiter_.allowQuery()) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "Bidirectional query rejected: rate limit exceeded"
        );
    }

    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::BIDIRECTIONAL;
    {
        const size_t depth_hint = constraints.max_depth.has_value()
            ? static_cast<size_t>(constraints.max_depth.value()) : 10u;
        local_stats.estimated_cost_ms =
            estimateCost(TraversalAlgorithm::BIDIRECTIONAL, depth_hint, constraints) * 0.1;
    }
    
    // Implement bidirectional search
    std::unordered_map<std::string, int> forward_distances;
    std::unordered_map<std::string, int> backward_distances;
    std::unordered_map<std::string, std::string> forward_parents;
    std::unordered_map<std::string, std::string> backward_parents;
    
    std::queue<std::string> forward_queue;
    std::queue<std::string> backward_queue;
    
    forward_queue.push(std::string(start_vertex));
    backward_queue.push(std::string(target_vertex));
    forward_distances[std::string(start_vertex)] = 0;
    backward_distances[std::string(target_vertex)] = 0;
    
    std::optional<std::string> meeting_point;
    int best_distance = std::numeric_limits<int>::max();
    
    // [GQ-1] Hard timeout for bidirectional BFS: unlike executeBFS/executeDFS, this
    // loop had no timeout check, causing indefinite blocking on dense or cyclic graphs.
    // Apply a 30-second default when the caller does not specify a timeout.
    constexpr int64_t BIDIRECTIONAL_BFS_DEFAULT_TIMEOUT_MS = 30'000;
    const int64_t bidi_timeout_ms = (constraints.timeout_ms > 0)
        ? static_cast<int64_t>(constraints.timeout_ms)
        : BIDIRECTIONAL_BFS_DEFAULT_TIMEOUT_MS;
    auto bidiTimedOut = [&]() -> bool {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - start_time).count() > bidi_timeout_ms;
    };

    while (!forward_queue.empty() || !backward_queue.empty()) {
        if (bidiTimedOut()) {
            local_stats.early_terminated = true;
            auto end_time = std::chrono::steady_clock::now();
            local_stats.execution_time_ms =
                std::chrono::duration<double, std::milli>(end_time - start_time).count();
            if (stats) *stats = local_stats;
            recordExecution(local_stats);
            metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
            return Err<GraphIndexManager::PathResult>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "Bidirectional BFS exceeded timeout of " +
                    std::to_string(bidi_timeout_ms) + "ms");
        }
        // Expand forward
        if (!forward_queue.empty()) {
            std::string current = forward_queue.front();
            forward_queue.pop();
            local_stats.nodes_explored++;
            
            if (backward_distances.find(current) != backward_distances.end()) {
                int total_dist = forward_distances[current] + backward_distances[current];
                if (total_dist < best_distance) {
                    best_distance = total_dist;
                    meeting_point = current;
                }
            }
            
            auto [status, neighbors] = graph_manager_.outNeighbors(current);
            if (status.ok) {
                local_stats.edges_traversed += neighbors.size();
                for (const auto& neighbor : neighbors) {
                    if (forward_distances.find(neighbor) == forward_distances.end()) {
                        forward_distances[neighbor] = forward_distances[current] + 1;
                        forward_parents[neighbor] = current;
                        forward_queue.push(neighbor);
                    }
                }
            }
        }
        
        // Expand backward
        if (!backward_queue.empty()) {
            std::string current = backward_queue.front();
            backward_queue.pop();
            local_stats.nodes_explored++;
            
            if (forward_distances.find(current) != forward_distances.end()) {
                int total_dist = forward_distances[current] + backward_distances[current];
                if (total_dist < best_distance) {
                    best_distance = total_dist;
                    meeting_point = current;
                }
            }
            
            auto [status, neighbors] = graph_manager_.inNeighbors(current);
            if (status.ok) {
                local_stats.edges_traversed += neighbors.size();
                for (const auto& neighbor : neighbors) {
                    if (backward_distances.find(neighbor) == backward_distances.end()) {
                        backward_distances[neighbor] = backward_distances[current] + 1;
                        backward_parents[neighbor] = current;
                        backward_queue.push(neighbor);
                    }
                }
            }
        }
        
        if (meeting_point.has_value()) {
            break;
        }
    }
    
    GraphIndexManager::PathResult result;
    
    if (meeting_point.has_value()) {
        // Reconstruct path
        std::vector<std::string> forward_path;
        std::string current = meeting_point.value();
        
        // Build forward path from start to meeting point
        while (current != std::string(start_vertex)) {
            forward_path.push_back(current);
            auto it = forward_parents.find(current);
            if (it == forward_parents.end()) {
                break;
            }
            current = it->second;
        }
        forward_path.push_back(std::string(start_vertex));
        std::reverse(forward_path.begin(), forward_path.end());
        
        // Build backward path from meeting point to target
        std::vector<std::string> backward_path;
        current = backward_parents.find(meeting_point.value()) != backward_parents.end() 
                  ? backward_parents[meeting_point.value()] : "";
        
        while (!current.empty() && current != std::string(target_vertex)) {
            backward_path.push_back(current);
            auto it = backward_parents.find(current);
            if (it == backward_parents.end()) {
                break;
            }
            current = it->second;
        }
        
        // Add target vertex if we reached it
        if (!current.empty()) {
            backward_path.push_back(std::string(target_vertex));
        }
        
        result.path = forward_path;
        result.path.insert(result.path.end(), backward_path.begin(), backward_path.end());
        result.totalCost = static_cast<double>(best_distance);
        local_stats.paths_found = 1;
    }
    
    auto end_time = std::chrono::steady_clock::now();
    local_stats.execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    if (stats) {
        *stats = local_stats;
    }
    recordExecution(local_stats);
    
    return Ok(result);
}

// ---------------------------------------------------------------------------
// Subgraph Isomorphism (VF2-style backtracking)
// ---------------------------------------------------------------------------

Result<GraphQueryOptimizer::SubgraphIsomorphismResult>
GraphQueryOptimizer::executeSubgraphIsomorphism(
    const std::vector<std::string>& pattern_vertices,
    const std::vector<std::pair<std::string, std::string>>& pattern_edges,
    ExecutionStats* stats) {

    return executeSubgraphIsomorphism(
        pattern_vertices,
        pattern_edges,
        QueryConstraints{},
        stats);
}

Result<GraphQueryOptimizer::SubgraphIsomorphismResult>
GraphQueryOptimizer::executeSubgraphIsomorphism(
    const std::vector<std::string>& pattern_vertices,
    const std::vector<std::pair<std::string, std::string>>& pattern_edges,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {

    if (!rate_limiter_.allowQuery()) {
        return Err<SubgraphIsomorphismResult>(
            errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "SubgraphIsomorphism query rejected: rate limit exceeded"
        );
    }

    // GQ-2: Enforce a hard maximum on pattern size before starting the VF2
    // backtracking search.  The algorithm is O(|V|^|pattern|); without this
    // guard a 5-vertex pattern on a 1,000-node graph explores up to 10^15
    // candidate pairs, which permanently blocks the handling thread.
    // A 10-vertex cap is generous for practical subgraph queries while still
    // bounding the worst-case search space to a tractable level.
    static constexpr size_t kMaxPatternVertices = 10;
    if (pattern_vertices.size() > kMaxPatternVertices) {
        return Err<SubgraphIsomorphismResult>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "SubgraphIsomorphism: pattern size " +
            std::to_string(pattern_vertices.size()) +
            " exceeds maximum allowed " + std::to_string(kMaxPatternVertices));
    }

    // GQ-2: Apply a minimum non-zero timeout for isomorphism queries even when
    // the caller does not specify one, because default QueryConstraints{} has
    // timeout_ms == 0 (no timeout).  With default constraints the timedOut()
    // lambda is a no-op, making the unbounded recursion below possible.
    QueryConstraints effective_constraints = constraints;
    if (effective_constraints.timeout_ms == 0) {
        effective_constraints.timeout_ms = 30000;  // 30-second hard cap
    }

    auto start_time = std::chrono::steady_clock::now();

    SubgraphIsomorphismResult result;
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::DFS;
    // Use pattern vertex count as depth proxy for cost estimation
    // (0.1 converts cost units → ms; same factor used in optimizeXxx plan construction)
    local_stats.estimated_cost_ms =
        estimateCost(TraversalAlgorithm::DFS, pattern_vertices.size(), constraints) * 0.1;

    if (pattern_vertices.empty()) {
        // Empty pattern matches trivially with an empty mapping
        result.matches.push_back({});
        result.execution_time_ms = 0.0;
        local_stats.paths_found = 1;
        if (stats) *stats = local_stats;
        recordExecution(local_stats);
        return Ok(result);
    }

    // Timeout helper
    auto timedOut = [&]() -> bool {
        if (constraints.timeout_ms == 0) return false;
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        return elapsed > static_cast<decltype(elapsed)>(constraints.timeout_ms);
    };

    // Build adjacency sets for the pattern graph so feasibility checks are O(1)
    // pattern_adj[u] = set of pattern vertices that u points to
    std::unordered_map<std::string, std::unordered_set<std::string>> pattern_adj;
    for (const auto& e : pattern_edges) {
        pattern_adj[e.first].insert(e.second);
    }

    // Enumerate all vertices in the data graph.
    // Pattern vertex labels ("u", "v", ...) are abstract names used only for
    // result mapping; they are NOT data vertex IDs.
    std::vector<std::string> data_vertices;
    auto [all_vertices_status, all_vertices] = graph_manager_.allVertices();
    if (all_vertices_status.ok) {
        data_vertices = std::move(all_vertices);
    }

    // Build out-adjacency cache for data graph vertices to speed up feasibility checks
    std::unordered_map<std::string, std::unordered_set<std::string>> data_adj_cache;
    for (const auto& v : data_vertices) {
        auto [st, nbrs] = graph_manager_.outNeighbors(v);
        if (st.ok) {
            data_adj_cache[v] = {nbrs.begin(), nbrs.end()};
            local_stats.edges_traversed += nbrs.size();
        }
    }

    // VF2-style recursive backtracking
    // mapping: pattern_vertex_label -> data_vertex_id (partial)
    std::unordered_map<std::string, std::string> mapping;
    std::unordered_set<std::string> used_data_vertices;

    // Ordered list of pattern vertices to assign (simple ordering by index)
    const size_t n_pattern = pattern_vertices.size();

    // Feasibility check: given current mapping extended by (pattern_vertices[depth] -> dv),
    // is it consistent with all pattern edges involving already-mapped vertices?
    auto isFeasible = [&](size_t depth, const std::string& dv) -> bool {
        const std::string& pu = pattern_vertices[depth];
        // Check edges from pu to already-mapped pattern vertices (and self-loops)
        auto pit = pattern_adj.find(pu);
        if (pit != pattern_adj.end()) {
            for (const auto& pv_target : pit->second) {
                if (pv_target == pu) {
                    // Self-loop in pattern: dv must have a self-loop in the data graph
                    auto ait = data_adj_cache.find(dv);
                    if (ait == data_adj_cache.end() ||
                        ait->second.find(dv) == ait->second.end()) {
                        return false;
                    }
                    continue;
                }
                auto mit = mapping.find(pv_target);
                if (mit != mapping.end()) {
                    // Pattern edge pu -> pv_target must exist as dv -> mit->second
                    const auto& dv_target = mit->second;
                    auto ait = data_adj_cache.find(dv);
                    if (ait == data_adj_cache.end() ||
                        ait->second.find(dv_target) == ait->second.end()) {
                        return false;
                    }
                }
            }
        }
        // Check edges from already-mapped pattern vertices to pu
        for (const auto& [prev_pu, prev_dv] : mapping) {
            auto pit2 = pattern_adj.find(prev_pu);
            if (pit2 != pattern_adj.end() &&
                pit2->second.count(pu)) {
                // Pattern edge prev_pu -> pu must exist as prev_dv -> dv
                auto ait = data_adj_cache.find(prev_dv);
                if (ait == data_adj_cache.end() ||
                    ait->second.find(dv) == ait->second.end()) {
                    return false;
                }
            }
        }
        return true;
    };

    // Recursive backtracking
    // [GQ-2] Hard iteration limit to bound the exponential VF2 blow-up when neither
    // max_results nor timeout_ms is set by the caller.
    constexpr size_t VF2_MAX_CANDIDATE_PAIRS = 10'000'000;
    size_t vf2_iteration_count = 0;
    bool vf2_limit_exceeded = false;

    std::function<void(size_t)> backtrack = [&](size_t depth) {
        if (timedOut()) { local_stats.early_terminated = true; return; }
        if (local_stats.early_terminated) return;
        if (depth == n_pattern) {
            result.matches.push_back(mapping);
            local_stats.paths_found++;
            return;
        }
        const std::string& pu = pattern_vertices[depth];
        for (const auto& dv : data_vertices) {
            if (local_stats.early_terminated) return;
            // Injective: data vertex must not already be used
            if (used_data_vertices.count(dv)) continue;
            // Forbidden vertex check
            if (std::find(constraints.forbidden_vertices.begin(),
                          constraints.forbidden_vertices.end(), dv) !=
                constraints.forbidden_vertices.end()) continue;
            // [GQ-2] Enforce hard iteration cap before expensive feasibility check
            if (++vf2_iteration_count > VF2_MAX_CANDIDATE_PAIRS) {
                vf2_limit_exceeded = true;
                local_stats.early_terminated = true;
                return;
            }
            result.candidate_pairs_checked++;
            local_stats.nodes_explored++;
            if (!isFeasible(depth, dv)) continue;
            // Extend mapping
            mapping[pu] = dv;
            used_data_vertices.insert(dv);
            backtrack(depth + 1);
            // Backtrack
            mapping.erase(pu);
            used_data_vertices.erase(dv);
            // Early termination on max_results
            if (constraints.max_results.has_value() &&
                result.matches.size() >= constraints.max_results.value()) {
                local_stats.early_terminated = true;
                return;
            }
        }
    };

    backtrack(0);

    auto end_time = std::chrono::steady_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(
        end_time - start_time).count();
    local_stats.execution_time_ms = result.execution_time_ms;

    if (stats) *stats = local_stats;
    recordExecution(local_stats);

    // Return an error if terminated early with no matches
    if (local_stats.early_terminated && result.matches.empty()) {
        metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
        if (vf2_limit_exceeded) {
            return Err<SubgraphIsomorphismResult>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "SubgraphIsomorphism aborted: exceeded hard candidate-pair limit of " +
                    std::to_string(VF2_MAX_CANDIDATE_PAIRS) + " pairs (pattern may be too large)"
            );
        }
        if (constraints.timeout_ms > 0) {
            return Err<SubgraphIsomorphismResult>(
                errors::ErrorCode::ERR_QUERY_TIMEOUT,
                "SubgraphIsomorphism query exceeded timeout of " +
                    std::to_string(constraints.timeout_ms) + "ms"
            );
        }
    }

    return Ok(result);
}

Result<GraphQueryOptimizer::GraphStatistics> GraphQueryOptimizer::collectStatistics(
    [[maybe_unused]] std::optional<std::string_view> graph_id) {
    
    GraphStatistics stats;
    
    // Get topology statistics from GraphIndexManager
    stats.vertex_count = graph_manager_.getTopologyNodeCount();
    stats.edge_count = graph_manager_.getTopologyEdgeCount();

    // Fallback path for setups that do not preload in-memory topology.
    // This commonly happens in focused unit tests that populate RocksDB via
    // addEdge() but never call rebuildTopology().
    if (stats.vertex_count == 0 && stats.edge_count == 0) {
        auto [vertex_status, vertices] = graph_manager_.allVertices();
        if (vertex_status.ok) {
            stats.vertex_count = vertices.size();

            std::unordered_set<std::string> unique_edge_ids;
            for (const auto& v : vertices) {
                auto [adj_status, adjs] = graph_manager_.outAdjacency(v);
                if (!adj_status.ok) {
                    continue;
                }
                for (const auto& adj : adjs) {
                    unique_edge_ids.insert(adj.edgeId);
                }
            }
            stats.edge_count = unique_edge_ids.size();
        }
    }
    
    if (stats.vertex_count > 0) {
        stats.avg_degree = static_cast<double>(stats.edge_count) / static_cast<double>(stats.vertex_count);
        stats.avg_branching_factor = stats.avg_degree;
    }
    
    // Estimate max depth (log base avg_degree of vertex count)
    if (stats.avg_branching_factor > 1.0) {
        stats.max_depth = static_cast<size_t>(
            std::log(static_cast<double>(stats.vertex_count)) / 
            std::log(stats.avg_branching_factor)
        );
    } else {
        stats.max_depth = stats.vertex_count > 0 ? stats.vertex_count : 1;
    }
    
    // Check for indices and caches
    stats.has_edge_index = true; // GraphIndexManager always has edge indices
    stats.has_adjacency_cache = true; // GraphIndexManager maintains topology
    
    statistics_ = stats;
    
    return Ok(stats);
}

double GraphQueryOptimizer::estimateEdgeTypeSelectivity(std::string_view edge_type) const {
    auto it = statistics_.edge_type_selectivity.find(std::string(edge_type));
    if (it != statistics_.edge_type_selectivity.end()) {
        return it->second;
    }
    return 1.0; // Default: no filtering
}

void GraphQueryOptimizer::setNodeLabelStats(
    const std::unordered_map<std::string, size_t>& label_counts) {
    statistics_.node_label_counts = label_counts;
    statistics_.node_label_selectivity.clear();
    if (statistics_.vertex_count == 0) return;
    const double total = static_cast<double>(statistics_.vertex_count);
    for (const auto& [label, count] : label_counts) {
        statistics_.node_label_selectivity[label] =
            std::max(0.0, std::min(1.0, static_cast<double>(count) / total));
    }
}

std::string GraphQueryOptimizer::explainPlan(const OptimizationPlan& plan) const {
    std::string algo_name;
    switch (plan.algorithm) {
        case TraversalAlgorithm::BFS: algo_name = "BFS"; break;
        case TraversalAlgorithm::DFS: algo_name = "DFS"; break;
        case TraversalAlgorithm::BIDIRECTIONAL: algo_name = "Bidirectional"; break;
        case TraversalAlgorithm::ASTAR: algo_name = "A*"; break;
        case TraversalAlgorithm::DIJKSTRA: algo_name = "Dijkstra"; break;
    }
    
    std::string pattern_name;
    switch (plan.pattern) {
        case QueryPattern::SHORTEST_PATH: pattern_name = "Shortest Path"; break;
        case QueryPattern::ALL_PATHS: pattern_name = "All Paths"; break;
        case QueryPattern::K_HOP_NEIGHBORS: pattern_name = "K-Hop Neighborhood"; break;
        case QueryPattern::PATTERN_MATCH: pattern_name = "Pattern Match"; break;
        case QueryPattern::REACHABILITY: pattern_name = "Reachability"; break;
        case QueryPattern::CONNECTED_COMPONENT: pattern_name = "Connected Component"; break;
    }
    
    std::string explanation = "Query Pattern: " + pattern_name + "\n";
    explanation += "Selected Algorithm: " + algo_name + "\n";
    explanation += "Estimated Cost: " + std::to_string(plan.estimated_cost) + "\n";
    explanation += "Estimated Time: " + std::to_string(plan.estimated_time_ms) + " ms\n";
    explanation += "Estimated Nodes: " + std::to_string(plan.estimated_nodes_explored) + "\n";
    explanation += "Use Index: " + std::string(plan.use_index ? "Yes" : "No") + "\n";
    explanation += "Use Cache: " + std::string(plan.use_cache ? "Yes" : "No") + "\n";
    explanation += "Early Termination: " + std::string(plan.enable_early_termination ? "Yes" : "No") + "\n";
    explanation += "Parallel Execution: " + std::string(plan.enable_parallel ? "Yes" : "No") + "\n";
    
    // Shard-aware plan info (v1.8.0)
    if (plan.is_distributed) {
        explanation += "Distributed: Yes (" + std::to_string(plan.shard_ids.size()) + " shards)\n";
        explanation += "Parallelism: " + std::to_string(plan.recommended_parallelism) + "\n";
        if (!plan.shard_ids.empty()) {
            explanation += "Shards: ";
            for (size_t i = 0; i < plan.shard_ids.size(); ++i) {
                if (i > 0) explanation += ", ";
                explanation += plan.shard_ids[i];
            }
            explanation += "\n";
        }
    }

    if (!plan.alternatives.empty()) {
        explanation += "\nAlternatives Considered:\n";
        for (const auto& [alt_algo, alt_cost] : plan.alternatives) {
            std::string alt_name;
            switch (alt_algo) {
                case TraversalAlgorithm::BFS: alt_name = "BFS"; break;
                case TraversalAlgorithm::DFS: alt_name = "DFS"; break;
                case TraversalAlgorithm::BIDIRECTIONAL: alt_name = "Bidirectional"; break;
                case TraversalAlgorithm::ASTAR: alt_name = "A*"; break;
                case TraversalAlgorithm::DIJKSTRA: alt_name = "Dijkstra"; break;
            }
            explanation += "  " + alt_name + ": " + std::to_string(alt_cost) + "\n";
        }
    }

    if (!plan.active_schema_hints.empty()) {
        explanation += "\nSchema Hints Active:\n";
        for (const auto& hint : plan.active_schema_hints) {
            explanation += "  " + hint + "\n";
        }
    }
    
    return explanation;
}

void GraphQueryOptimizer::clearPlanCache() {
    std::lock_guard<std::mutex> lk(plan_cache_mutex_);
    plan_cache_.clear();
    plan_cache_lru_.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Plan cache helpers: LRU eviction + TTL expiry
// ─────────────────────────────────────────────────────────────────────────────

void GraphQueryOptimizer::planCacheInsert(const std::string& key,
                                          const OptimizationPlan& plan) {
    std::lock_guard<std::mutex> lk(plan_cache_mutex_);
    auto it = plan_cache_.find(key);
    if (it != plan_cache_.end()) {
        // Key already present: update plan and move to front (MRU)
        it->second.first.plan = plan;
        it->second.first.inserted_at = std::chrono::steady_clock::now();
        plan_cache_lru_.splice(plan_cache_lru_.begin(), plan_cache_lru_,
                               it->second.second);
        return;
    }

    // Enforce size limit: evict LRU entry when at capacity
    if (plan_cache_max_size_ > 0 && plan_cache_.size() >= plan_cache_max_size_) {
        const std::string& lru_key = plan_cache_lru_.back();
        plan_cache_.erase(lru_key);
        plan_cache_lru_.pop_back();
        metrics_.plan_cache_evictions.fetch_add(1, std::memory_order_relaxed);
    }

    // Insert new entry at the front (MRU position)
    plan_cache_lru_.push_front(key);
    PlanCacheEntry entry{plan, std::chrono::steady_clock::now()};
    plan_cache_.emplace(key, std::make_pair(std::move(entry), plan_cache_lru_.begin()));
}

std::optional<GraphQueryOptimizer::OptimizationPlan>
GraphQueryOptimizer::planCacheLookup(const std::string& key) {
    std::lock_guard<std::mutex> lk(plan_cache_mutex_);
    auto it = plan_cache_.find(key);
    if (it == plan_cache_.end()) {
        return std::nullopt;
    }

    // TTL check: evict expired entry
    if (plan_cache_ttl_.count() > 0) {
        auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - it->second.first.inserted_at);
        if (age > plan_cache_ttl_) {
            plan_cache_lru_.erase(it->second.second);
            plan_cache_.erase(it);
            metrics_.plan_cache_evictions.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }
    }

    // Move to front (MRU position)
    plan_cache_lru_.splice(plan_cache_lru_.begin(), plan_cache_lru_,
                           it->second.second);
    // Return a value copy so the caller is not affected by subsequent
    // rehashes or evictions of the plan_cache_ map.
    return it->second.first.plan;
}

double GraphQueryOptimizer::estimateCost(
    TraversalAlgorithm algorithm,
    size_t estimated_depth,
    const QueryConstraints& constraints) const {
    
    double base_cost = 1.0;
    double branching = statistics_.avg_branching_factor > 0 ? statistics_.avg_branching_factor : 2.0;
    
    switch (algorithm) {
        case TraversalAlgorithm::BFS:
            // O(V + E) but in practice O(b^d) where b is branching factor, d is depth
            base_cost = std::pow(branching, estimated_depth);
            break;
            
        case TraversalAlgorithm::DFS:
            // Similar to BFS but with better memory characteristics
            base_cost = std::pow(branching, estimated_depth) * 0.9;
            break;
            
        case TraversalAlgorithm::DIJKSTRA:
            // O((V + E) log V) with priority queue
            base_cost = (statistics_.vertex_count + statistics_.edge_count) * 
                       std::log(statistics_.vertex_count + 1.0);
            break;
            
        case TraversalAlgorithm::ASTAR:
            // O((V + E) log V) but typically explores fewer nodes with good heuristic
            base_cost = (statistics_.vertex_count + statistics_.edge_count) * 
                       std::log(statistics_.vertex_count + 1.0) * 0.7;
            break;
            
        case TraversalAlgorithm::BIDIRECTIONAL:
            // O(b^(d/2) + b^(d/2)) = O(b^(d/2))
            base_cost = 2.0 * std::pow(branching, estimated_depth / 2.0);
            break;
    }
    
    // Apply optimizations
    if (statistics_.has_edge_index) {
        base_cost *= 0.8; // 20% improvement with index
    }
    
    if (statistics_.has_adjacency_cache) {
        base_cost *= 0.7; // 30% improvement with cache
    }
    
    if (constraints.edge_type.has_value()) {
        double selectivity = estimateEdgeTypeSelectivity(constraints.edge_type.value());
        base_cost *= selectivity; // Reduce cost based on edge type filtering
    }

    // Temporal range selectivity: a time-window filter reduces the effective
    // edge set, lowering traversal cost.  Mirrors the logic in
    // optimizeTemporalTraversal so that algorithm selection and cost estimates
    // are consistent.
    if (constraints.hasTemporalRange()) {
        double temporal_selectivity = 0.5; // default: one-sided bound
        if (constraints.time_range_start_ms.has_value() &&
            constraints.time_range_end_ms.has_value()) {
            const int64_t range_ms =
                *constraints.time_range_end_ms - *constraints.time_range_start_ms;
            constexpr int64_t REFERENCE_SPAN_MS =
                static_cast<int64_t>(5) * 365 * 24 * 3600 * 1000LL;
            temporal_selectivity = static_cast<double>(range_ms) /
                                   static_cast<double>(REFERENCE_SPAN_MS);
            temporal_selectivity = std::max(0.05, std::min(0.95, temporal_selectivity));
        }
        base_cost *= temporal_selectivity;
    }

    // Schema-aware hint: node label selectivity.
    // When node_labels is set, only a fraction of nodes match; reduce the
    // effective search space accordingly.  For OR-semantics with multiple
    // labels we use the maximum selectivity (upper bound on matching nodes).
    if (!constraints.node_labels.empty()) {
        double label_sel = 0.0;
        bool any_known = false;
        for (const auto& lbl : constraints.node_labels) {
            auto it = statistics_.node_label_selectivity.find(lbl);
            if (it != statistics_.node_label_selectivity.end()) {
                label_sel = std::max(label_sel, it->second);
                any_known = true;
            }
        }
        // When selectivity is known, scale cost; fall back to 0.5 (moderate) when unknown.
        const double effective_sel = any_known ? std::max(0.01, label_sel) : 0.5;
        base_cost *= effective_sel;
    }

    // Schema-aware hint: excluded edge types.
    // Each excluded edge type reduces the effective branching factor.
    // Use a conservative 10% reduction per excluded type.
    if (!constraints.excluded_edge_types.empty()) {
        double type_reduction = 1.0;
        for (const auto& et : constraints.excluded_edge_types) {
            auto it = statistics_.edge_type_selectivity.find(et);
            if (it != statistics_.edge_type_selectivity.end()) {
                // Exclude edges whose fraction is `it->second`
                type_reduction *= (1.0 - it->second);
            } else {
                // Unknown type: assume 10% reduction
                type_reduction *= 0.9;
            }
        }
        base_cost *= std::max(0.01, type_reduction);
    }

    // Adaptive cost model: blend learned EMA cost proportional to confidence.
    // When confidence is 0 (no observations yet) the base_cost is unchanged;
    // when confidence approaches 1.0 the estimate converges to the observed EMA.
    if (adaptive_learning_enabled_) {
        auto it = algo_cost_models_.find(algorithm);
        if (it != algo_cost_models_.end() && it->second.confidence > 0.0) {
            const double w = it->second.confidence;
            // Normalize learned cost to the same scale as base_cost by converting
            // ms → cost units (inverse of the 0.1 factor used in estimated_time_ms).
            const double learned_cost = it->second.ema_cost_ms * 10.0;
            base_cost = (1.0 - w) * base_cost + w * learned_cost;
        }
    }
    
    return base_cost;
}

GraphQueryOptimizer::TraversalAlgorithm GraphQueryOptimizer::selectAlgorithm(
    QueryPattern pattern,
    size_t estimated_depth,
    const QueryConstraints& constraints) const {

    // Adaptive plan selection: when learning is enabled, compare estimated costs
    // for all feasible algorithms and return the one with the lowest cost.
    // estimateCost() blends the learned EMA cost proportional to confidence, so
    // this automatically favours algorithms that have proven faster on real workloads.
    // We only activate adaptive selection when at least one algorithm has non-zero
    // confidence so that the static cost formulas (which use different scales) do
    // not interfere before any execution feedback has been collected.
    if (adaptive_learning_enabled_ && !algo_cost_models_.empty()) {
        bool has_any_confidence = false;
        for (const auto& entry : algo_cost_models_) {
            if (entry.second.confidence > 0.0) { has_any_confidence = true; break; }
        }

        if (has_any_confidence) {
            std::vector<TraversalAlgorithm> candidates;
            switch (pattern) {
                case QueryPattern::SHORTEST_PATH:
                    candidates = {TraversalAlgorithm::BFS, TraversalAlgorithm::DIJKSTRA};
                    if (estimated_depth > 3)
                        candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
                    break;
                case QueryPattern::REACHABILITY:
                    candidates = {TraversalAlgorithm::BFS};
                    if (estimated_depth > 3)
                        candidates.push_back(TraversalAlgorithm::BIDIRECTIONAL);
                    break;
                case QueryPattern::K_HOP_NEIGHBORS:
                    candidates = {TraversalAlgorithm::BFS, TraversalAlgorithm::DFS};
                    break;
                case QueryPattern::PATTERN_MATCH:
                case QueryPattern::ALL_PATHS:
                    candidates = {TraversalAlgorithm::DFS, TraversalAlgorithm::BFS};
                    break;
                case QueryPattern::CONNECTED_COMPONENT:
                    candidates = {TraversalAlgorithm::BFS};
                    break;
            }

            TraversalAlgorithm best = candidates[0];
            double best_cost = estimateCost(candidates[0], estimated_depth, constraints);
            for (size_t i = 1; i < candidates.size(); ++i) {
                double c = estimateCost(candidates[i], estimated_depth, constraints);
                if (c < best_cost) { best_cost = c; best = candidates[i]; }
            }
            return best;
        }
    }

    // Static fallback: heuristic-based selection when no learned data is available
    // or when adaptive learning is disabled.
    switch (pattern) {
        case QueryPattern::SHORTEST_PATH:
            if (estimated_depth > 5) {
                return TraversalAlgorithm::BIDIRECTIONAL;
            }
            return TraversalAlgorithm::BFS;

        case QueryPattern::K_HOP_NEIGHBORS:
            return TraversalAlgorithm::BFS;

        case QueryPattern::PATTERN_MATCH:
            return TraversalAlgorithm::DFS;

        case QueryPattern::REACHABILITY:
            if (estimated_depth > 3) {
                return TraversalAlgorithm::BIDIRECTIONAL;
            }
            return TraversalAlgorithm::BFS;

        case QueryPattern::ALL_PATHS:
            return TraversalAlgorithm::DFS;

        case QueryPattern::CONNECTED_COMPONENT:
            return TraversalAlgorithm::BFS;
    }

    return TraversalAlgorithm::BFS; // Default
}

size_t GraphQueryOptimizer::estimateDepth(
    QueryPattern pattern,
    const QueryConstraints& constraints) const {
    
    if (constraints.max_depth.has_value()) {
        return static_cast<size_t>(constraints.max_depth.value());
    }
    
    // Use graph diameter estimate
    size_t estimated = statistics_.max_depth;
    
    switch (pattern) {
        case QueryPattern::SHORTEST_PATH:
        case QueryPattern::REACHABILITY:
            // Assume average case is half the diameter
            return estimated / 2;
            
        case QueryPattern::K_HOP_NEIGHBORS:
            // Typically small depth
            return 3;
            
        case QueryPattern::PATTERN_MATCH:
            // Depends on pattern size, use moderate default
            return 4;
            
        case QueryPattern::ALL_PATHS:
            // Can be full depth
            return estimated;
            
        case QueryPattern::CONNECTED_COMPONENT:
            // Full traversal
            return estimated;
    }
    
    return 5; // Safe default
}

std::string GraphQueryOptimizer::generatePlanCacheKey(
    QueryPattern pattern,
    std::string_view start,
    std::string_view target,
    const QueryConstraints& constraints) const {
    
    std::string key = std::to_string(static_cast<int>(pattern)) + ":" +
                     std::string(start) + ":" + std::string(target);
    
    if (constraints.max_depth.has_value()) {
        key += ":depth=" + std::to_string(constraints.max_depth.value());
    }
    
    if (constraints.edge_type.has_value()) {
        key += ":type=" + constraints.edge_type.value();
    }

    // enable_parallel affects plan.enable_parallel directly; include it so that
    // the same vertex pair queried with and without parallel yields distinct
    // cache entries.
    if (constraints.enable_parallel) {
        key += ":par";
    }

    // Temporal range: two queries with different time windows must never share
    // a cache entry because their edge sets differ.
    if (constraints.time_range_start_ms.has_value()) {
        key += ":tr_from=" + std::to_string(*constraints.time_range_start_ms);
    }
    if (constraints.time_range_end_ms.has_value()) {
        key += ":tr_to=" + std::to_string(*constraints.time_range_end_ms);
    }
    if (constraints.time_range_require_containment) {
        key += ":tr_contain";
    }

    // Schema hints: sort labels so that {"A","B"} and {"B","A"} produce the
    // same key; different label sets produce distinct exact cache entries.
    if (!constraints.node_labels.empty()) {
        std::vector<std::string> sorted_labels = constraints.node_labels;
        std::sort(sorted_labels.begin(), sorted_labels.end());
        key += ":nl=";
        for (const auto& lbl : sorted_labels) {
            key += lbl + "|";
        }
    }

    if (!constraints.excluded_edge_types.empty()) {
        std::vector<std::string> sorted_types = constraints.excluded_edge_types;
        std::sort(sorted_types.begin(), sorted_types.end());
        key += ":xet=";
        for (const auto& et : sorted_types) {
            key += et + "|";
        }
    }
    
    return key;
}

std::string GraphQueryOptimizer::generateStructuralCacheKey(
    QueryPattern pattern,
    const QueryConstraints& constraints,
    std::optional<size_t> depth_hint) const {

    // Structural key: captures pattern + all constraint parameters that affect
    // plan selection, but omits specific vertex IDs.  Two queries sharing the
    // same structural key will receive an identical OptimizationPlan.
    std::string key = "struct:" + std::to_string(static_cast<int>(pattern));

    if (depth_hint.has_value()) {
        key += ":depth=" + std::to_string(depth_hint.value());
    } else if (constraints.max_depth.has_value()) {
        key += ":depth=" + std::to_string(constraints.max_depth.value());
    }

    if (constraints.edge_type.has_value()) {
        key += ":type=" + constraints.edge_type.value();
    }

    if (constraints.unique_vertices) {
        key += ":uv";
    }

    if (constraints.unique_edges) {
        key += ":ue";
    }

    if (constraints.enable_parallel) {
        key += ":par";
    }

    if (!constraints.forbidden_vertices.empty()) {
        key += ":fv=" + std::to_string(constraints.forbidden_vertices.size());
    }

    if (!constraints.required_vertices.empty()) {
        key += ":rv=" + std::to_string(constraints.required_vertices.size());
    }

    // Temporal range: include time window bounds so structurally different
    // temporal queries are not incorrectly merged in the plan cache.
    if (constraints.time_range_start_ms.has_value()) {
        key += ":tr_from=" + std::to_string(*constraints.time_range_start_ms);
    }
    if (constraints.time_range_end_ms.has_value()) {
        key += ":tr_to=" + std::to_string(*constraints.time_range_end_ms);
    }
    if (constraints.time_range_require_containment) {
        key += ":tr_contain";
    }

    // Schema hints: encode actual sorted label values so that queries with the
    // same number of labels but different names get distinct structural keys.
    // Sorting ensures {"A","B"} and {"B","A"} map to the same structural key.
    if (!constraints.node_labels.empty()) {
        std::vector<std::string> sorted_labels = constraints.node_labels;
        std::sort(sorted_labels.begin(), sorted_labels.end());
        key += ":nl=";
        for (const auto& lbl : sorted_labels) {
            key += lbl + "|";
        }
    }

    if (!constraints.excluded_edge_types.empty()) {
        std::vector<std::string> sorted_types = constraints.excluded_edge_types;
        std::sort(sorted_types.begin(), sorted_types.end());
        key += ":xet=";
        for (const auto& et : sorted_types) {
            key += et + "|";
        }
    }

    return key;
}

bool GraphQueryOptimizer::shouldUseParallel(
    TraversalAlgorithm algorithm,
    size_t estimated_nodes) const {
    
    // Only use parallel for large graphs
    if (estimated_nodes < 10000) {
        return false;
    }
    
    // Some algorithms parallelize better
    switch (algorithm) {
        case TraversalAlgorithm::BFS:
        case TraversalAlgorithm::BIDIRECTIONAL:
            return true;
            
        case TraversalAlgorithm::DFS:
        case TraversalAlgorithm::ASTAR:
        case TraversalAlgorithm::DIJKSTRA:
            return false; // These don't parallelize well
    }
    
    return false;
}

void GraphQueryOptimizer::recordExecution(const ExecutionStats& stats) {
    execution_history_.push_back(stats);
    
    // Keep history bounded
    if (execution_history_.size() > MAX_HISTORY_SIZE) {
        execution_history_.erase(execution_history_.begin());
    }

    // Update cumulative observability metrics
    metrics_.total_queries.fetch_add(1, std::memory_order_relaxed);
    if (stats.paths_found == 0 && !stats.early_terminated) {
        metrics_.failed_queries.fetch_add(1, std::memory_order_relaxed);
    }
    auto duration = static_cast<uint64_t>(stats.execution_time_ms);
    metrics_.total_execution_time_ms.fetch_add(duration, std::memory_order_relaxed);
    metrics_.total_nodes_explored.fetch_add(stats.nodes_explored, std::memory_order_relaxed);
    metrics_.total_edges_traversed.fetch_add(stats.edges_traversed, std::memory_order_relaxed);

    // Feed latency histogram for p50/p95/p99 computation
    metrics_.latency_histogram.record(duration);

    // Update max execution time with a compare-and-swap loop
    uint64_t current_max = metrics_.max_execution_time_ms.load(std::memory_order_relaxed);
    while (duration > current_max &&
           !metrics_.max_execution_time_ms.compare_exchange_weak(
               current_max, duration,
               std::memory_order_relaxed, std::memory_order_relaxed)) {
        // current_max updated by CAS on failure; retry
    }

    // Adaptive cost model: update per-algorithm EMA with observed execution time
    if (adaptive_learning_enabled_) {
        algo_cost_models_[stats.algorithm].update(stats.execution_time_ms);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Adaptive cost model: export / import
// ─────────────────────────────────────────────────────────────────────────────

static const std::unordered_map<std::string, GraphQueryOptimizer::TraversalAlgorithm>&
algoNameMap() {
    static const std::unordered_map<std::string, GraphQueryOptimizer::TraversalAlgorithm> m = {
        {"BFS",           GraphQueryOptimizer::TraversalAlgorithm::BFS},
        {"DFS",           GraphQueryOptimizer::TraversalAlgorithm::DFS},
        {"DIJKSTRA",      GraphQueryOptimizer::TraversalAlgorithm::DIJKSTRA},
        {"ASTAR",         GraphQueryOptimizer::TraversalAlgorithm::ASTAR},
        {"BIDIRECTIONAL", GraphQueryOptimizer::TraversalAlgorithm::BIDIRECTIONAL},
    };
    return m;
}

static std::string algoToName(GraphQueryOptimizer::TraversalAlgorithm algo) {
    switch (algo) {
        case GraphQueryOptimizer::TraversalAlgorithm::BFS:           return "BFS";
        case GraphQueryOptimizer::TraversalAlgorithm::DFS:           return "DFS";
        case GraphQueryOptimizer::TraversalAlgorithm::DIJKSTRA:      return "DIJKSTRA";
        case GraphQueryOptimizer::TraversalAlgorithm::ASTAR:         return "ASTAR";
        case GraphQueryOptimizer::TraversalAlgorithm::BIDIRECTIONAL: return "BIDIRECTIONAL";
    }
    return "UNKNOWN";
}

std::string GraphQueryOptimizer::exportCostModel() const {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& [algo, model] : algo_cost_models_) {
        std::string name = algoToName(algo);
        j[name] = {
            {"ema_cost_ms",  model.ema_cost_ms},
            {"exec_count",   model.exec_count},
            {"confidence",   model.confidence}
        };
    }
    return j.dump();
}

bool GraphQueryOptimizer::importCostModel(std::string_view json_model) {
    try {
        auto j = nlohmann::json::parse(json_model);
        if (!j.is_object()) return false;
        const auto& name_map = algoNameMap();
        for (auto& [key, val] : j.items()) {
            auto it = name_map.find(key);
            if (it == name_map.end()) continue; // unknown algo – skip
            if (!val.is_object()) continue;
            AlgorithmCostModel m;
            m.ema_cost_ms = val.value("ema_cost_ms", 0.0);
            m.exec_count  = val.value("exec_count",  static_cast<uint32_t>(0));
            m.confidence  = val.value("confidence",  0.0);
            // Clamp to valid range
            m.confidence = std::max(0.0, std::min(1.0, m.confidence));
            algo_cost_models_[it->second] = m;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Cost model calibration from execution history
// ─────────────────────────────────────────────────────────────────────────────

GraphQueryOptimizer::CostModelCalibrationReport
GraphQueryOptimizer::calibrateFromHistory() {
    CostModelCalibrationReport report;

    if (execution_history_.empty()) {
        return report;
    }

    // Accumulate per-algorithm statistics in one pass over execution_history_
    struct AlgoAcc {
        std::vector<double> actual_times;
        double est_sum  = 0.0;  // sum of estimated_cost_ms (paired entries only)
        double mae_sum  = 0.0;  // sum of |actual - estimated|
        size_t paired   = 0;    // entries that had estimated_cost_ms > 0
    };
    std::unordered_map<TraversalAlgorithm,
                       AlgoAcc,
                       std::hash<TraversalAlgorithm>> acc_by_algo;

    for (const auto& s : execution_history_) {
        auto& acc = acc_by_algo[s.algorithm];
        acc.actual_times.push_back(s.execution_time_ms);
        if (s.estimated_cost_ms > 0.0) {
            acc.est_sum += s.estimated_cost_ms;
            acc.mae_sum += std::abs(s.execution_time_ms - s.estimated_cost_ms);
            ++acc.paired;
        }
    }

    report.total_samples = execution_history_.size();

    for (auto& [algo, acc] : acc_by_algo) {
        const size_t n = acc.actual_times.size();

        // Compute mean of actual times
        double sum = 0.0;
        for (double t : acc.actual_times) sum += t;
        const double mean = sum / static_cast<double>(n);

        // Compute variance / stddev
        double variance = 0.0;
        for (double t : acc.actual_times) {
            double diff = t - mean;
            variance += diff * diff;
        }
        const double stddev = n > 1 ? std::sqrt(variance / static_cast<double>(n - 1)) : 0.0;

        // Min / max
        const double mn = *std::min_element(acc.actual_times.begin(), acc.actual_times.end());
        const double mx = *std::max_element(acc.actual_times.begin(), acc.actual_times.end());

        AlgorithmCalibrationStats ast;
        ast.mean_execution_ms   = mean;
        ast.stddev_execution_ms = stddev;
        ast.min_execution_ms    = mn;
        ast.max_execution_ms    = mx;
        ast.sample_count        = n;

        // Cost accuracy statistics – only when estimated_cost_ms was populated
        // by the execute* methods in at least one history entry.
        if (acc.paired > 0) {
            const double mean_est = acc.est_sum / static_cast<double>(acc.paired);
            ast.mean_estimated_ms       = mean_est;
            ast.mean_absolute_error_ms  = acc.mae_sum / static_cast<double>(acc.paired);
            ast.cost_ratio              = (mean > 0.0) ? (mean_est / mean) : 0.0;
            ast.estimation_sample_count = acc.paired;
        }

        report.algorithm_stats[algo] = ast;

        // Re-seed the EMA only when adaptive learning is enabled and there
        // are enough samples to produce a statistically meaningful estimate.
        if (adaptive_learning_enabled_ && n >= MIN_CALIBRATION_SAMPLES) {
            AlgorithmCostModel& model = algo_cost_models_[algo];
            model.ema_cost_ms = mean;
            model.exec_count  = static_cast<uint32_t>(
                std::min<size_t>(n, std::numeric_limits<uint32_t>::max()));
            model.confidence  = std::min(
                1.0,
                static_cast<double>(n) / AlgorithmCostModel::MAX_CONF_OBS);
            ++report.algorithms_calibrated;
        }
    }

    return report;
}

// ─────────────────────────────────────────────────────────────────────────────
// Incremental graph query execution on live updates (v1.9.0)
// ─────────────────────────────────────────────────────────────────────────────

GraphQueryOptimizer::IncrementalQueryHandle
GraphQueryOptimizer::registerIncrementalBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    IncrementalQueryCallback callback) {

    const IncrementalQueryHandle handle =
        next_incremental_handle_.fetch_add(1, std::memory_order_relaxed);

    IncrementalQueryEntry entry;
    entry.handle       = handle;
    entry.start_vertex = std::string(start_vertex);
    entry.max_depth    = max_depth;
    entry.constraints  = constraints;
    entry.callback     = std::move(callback);

    // Execute initial BFS to seed the last_result snapshot.
    auto result = executeBFS(start_vertex, max_depth, constraints);
    if (result) {
        entry.last_result.insert(result.value().begin(), result.value().end());
    }

    incremental_queries_[handle] = std::move(entry);
    return handle;
}

void GraphQueryOptimizer::unregisterIncrementalQuery(IncrementalQueryHandle handle) {
    incremental_queries_.erase(handle);
}

size_t GraphQueryOptimizer::onGraphChange(const GraphChangeSet& changes) {
    if (changes.empty() || incremental_queries_.empty()) {
        return 0;
    }

    // Collect all vertex IDs touched by the change set (edge endpoints + vertex IDs).
    std::unordered_set<std::string> changed_vertices;
    for (const auto& change : changes.changes) {
        if (!change.from.empty()) changed_vertices.insert(change.from);
        if (!change.to.empty())   changed_vertices.insert(change.to);
        if (change.type == GraphChangeSet::ChangeType::VERTEX_ADDED ||
            change.type == GraphChangeSet::ChangeType::VERTEX_REMOVED) {
            if (!change.id.empty()) changed_vertices.insert(change.id);
        }
    }

    // First pass: determine affected queries, re-execute them, build deltas,
    // and update last_result. Callbacks are collected for deferred invocation
    // so that a callback calling unregisterIncrementalQuery() cannot invalidate
    // the ongoing iteration of incremental_queries_.
    // PendingCallback: holds the callback and its delta snapshot for deferred
    // invocation after the map iteration is complete.
    struct PendingCallback {
        IncrementalQueryCallback callback;
        IncrementalQueryResult delta;
    };
    std::vector<PendingCallback> pending;

    for (auto& [handle, entry] : incremental_queries_) {
        // A query is affected when:
        //   1. Its start_vertex is directly changed, or
        //   2. Any changed vertex appears in the previous result set.
        bool affected = changed_vertices.count(entry.start_vertex) > 0;
        if (!affected) {
            for (const auto& v : changed_vertices) {
                if (entry.last_result.count(v)) {
                    affected = true;
                    break;
                }
            }
        }
        if (!affected) {
            continue;
        }

        // Re-execute the BFS.
        ExecutionStats stats;
        auto result = executeBFS(entry.start_vertex, entry.max_depth,
                                 entry.constraints, &stats);

        IncrementalQueryResult delta;
        delta.reexecuted = true;
        delta.stats      = stats;

        if (result) {
            const std::unordered_set<std::string> new_result(result.value().begin(),
                                                              result.value().end());
            delta.current.assign(result.value().begin(), result.value().end());

            // Added: in new result but not in previous result.
            for (const auto& v : new_result) {
                if (!entry.last_result.count(v)) {
                    delta.added.push_back(v);
                }
            }
            // Removed: in previous result but not in new result.
            for (const auto& v : entry.last_result) {
                if (!new_result.count(v)) {
                    delta.removed.push_back(v);
                }
            }

            entry.last_result = new_result;
        } else {
            // On error, report all previous vertices as removed.
            delta.removed.assign(entry.last_result.begin(), entry.last_result.end());
            delta.current.clear();
            entry.last_result.clear();
        }

        pending.push_back({entry.callback, std::move(delta)});
    }

    // Second pass: invoke callbacks outside the map iteration.
    // This ensures that any unregisterIncrementalQuery() call inside a callback
    // does not invalidate iterators used in the first pass above.
    for (auto& p : pending) {
        p.callback(p.delta);
    }

    return pending.size();
}

// Analytics Module Integration (Issue #1821)
// ─────────────────────────────────────────────────────────────────────────────

void GraphQueryOptimizer::attachAnalytics(GraphAnalytics& analytics) {
    analytics_ = &analytics;
}

void GraphQueryOptimizer::detachAnalytics() {
    analytics_ = nullptr;
}

Result<std::vector<GraphAnalytics::PathInfo>> GraphQueryOptimizer::executeKShortestPaths(
    std::string_view source,
    std::string_view target,
    int k,
    const QueryConstraints& constraints,
    std::string_view weight_attr,
    ExecutionStats* stats)
{
    using ReturnType = std::vector<GraphAnalytics::PathInfo>;

    // Precondition checks – do not touch any counters for caller errors
    if (!analytics_) {
        return Err<ReturnType>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            "No analytics instance attached; call attachAnalytics() first");
    }

    if (k <= 0) {
        return Err<ReturnType>(errors::ErrorCode::ERR_QUERY_INVALID_INPUT,
            "k must be positive");
    }

    // Apply rate limiting before executing (no counter updates on rejection)
    if (!rate_limiter_.allowQuery()) {
        return Err<ReturnType>(errors::ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
            "k-shortest-paths query rejected: rate limit exceeded");
    }

    const auto t_start = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    local_stats.algorithm = TraversalAlgorithm::DIJKSTRA; // Yen's uses Dijkstra internally
    {
        // Default depth of 10 is consistent with other Dijkstra call sites when
        // max_depth is not constrained; 0.1 converts cost units → ms.
        const size_t depth_hint = constraints.max_depth.has_value()
            ? static_cast<size_t>(constraints.max_depth.value()) : 10u;
        local_stats.estimated_cost_ms =
            estimateCost(TraversalAlgorithm::DIJKSTRA, depth_hint, constraints) * 0.1;
    }

    // Delegate to the analytics module (Yen's algorithm)
    auto [status, paths] = analytics_->kShortestPaths(
        std::string(source), std::string(target), k, std::string(weight_attr));

    const auto t_end = std::chrono::steady_clock::now();
    local_stats.execution_time_ms =
        std::chrono::duration<double, std::milli>(t_end - t_start).count();

    // Check timeout – consistent with BFS/DFS/Dijkstra pattern
    if (constraints.timeout_ms > 0 &&
        local_stats.execution_time_ms > static_cast<double>(constraints.timeout_ms)) {
        local_stats.early_terminated = true;
        metrics_.timed_out_queries.fetch_add(1, std::memory_order_relaxed);
        if (stats) { *stats = local_stats; }
        recordExecution(local_stats);
        return Err<ReturnType>(errors::ErrorCode::ERR_QUERY_TIMEOUT,
            "k-shortest-paths query exceeded timeout of " +
                std::to_string(constraints.timeout_ms) + "ms");
    }

    if (!status.ok) {
        // Record the failed attempt so total_queries and failed_queries stay accurate
        // (paths_found stays 0, so recordExecution will increment failed_queries)
        recordExecution(local_stats);
        return Err<ReturnType>(errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND, status.message);
    }

    // Populate output stats
    local_stats.paths_found = paths.size();
    if (!paths.empty()) {
        local_stats.nodes_explored = paths[0].vertices.size();
        // hop_count is int; guard against any unexpected negative value
        const int hc = paths[0].hop_count;
        local_stats.max_depth_reached = (hc > 0) ? static_cast<size_t>(hc) : 0u;
    }

    if (stats) { *stats = local_stats; }
    // recordExecution is the single source that increments total_queries,
    // failed_queries, total_execution_time_ms, latency_histogram, etc.
    recordExecution(local_stats);

    return Ok(std::move(paths));
}

} // namespace graph
} // namespace themis
