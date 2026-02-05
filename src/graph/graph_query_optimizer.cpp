// Graph Query Optimizer implementation

#include "graph/graph_query_optimizer.h"
#include "graph/path_constraints.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <chrono>
#include <sstream>

namespace themis {
namespace graph {

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
    
    // Check plan cache
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::SHORTEST_PATH, start_vertex, target_vertex, constraints);
        auto it = plan_cache_.find(cache_key);
        if (it != plan_cache_.end()) {
            return Ok(it->second);
        }
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
    
    // Determine if parallel execution is beneficial
    plan.enable_parallel = shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);
    
    // Generate explanation
    plan.explanation = explainPlan(plan);
    
    // Cache the plan
    if (plan_caching_enabled_) {
        auto cache_key = generatePlanCacheKey(QueryPattern::SHORTEST_PATH, start_vertex, target_vertex, constraints);
        plan_cache_[cache_key] = plan;
    }
    
    return Ok(plan);
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeKHopNeighborhood(
    std::string_view start_vertex,
    int k) {
    return optimizeKHopNeighborhood(start_vertex, k, QueryConstraints());
}

Result<GraphQueryOptimizer::OptimizationPlan> GraphQueryOptimizer::optimizeKHopNeighborhood(
    std::string_view start_vertex,
    int k,
    const QueryConstraints& constraints) {
    
    OptimizationPlan plan;
    plan.pattern = QueryPattern::K_HOP_NEIGHBORS;
    
    // For k-hop queries, BFS is typically optimal
    plan.algorithm = TraversalAlgorithm::BFS;
    
    size_t estimated_depth = static_cast<size_t>(k);
    plan.estimated_cost = estimateCost(TraversalAlgorithm::BFS, estimated_depth, constraints);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, k));
    plan.estimated_time_ms = plan.estimated_cost * 0.1;
    
    // Optimization flags
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = true; // Stop at depth k
    plan.enable_parallel = shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);
    
    plan.explanation = explainPlan(plan);
    
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
    
    OptimizationPlan plan;
    plan.pattern = QueryPattern::PATTERN_MATCH;
    
    // Pattern matching uses DFS for better memory efficiency
    plan.algorithm = TraversalAlgorithm::DFS;
    
    // Estimate depth based on pattern size
    size_t pattern_depth = pattern_vertices.size();
    plan.estimated_cost = estimateCost(TraversalAlgorithm::DFS, pattern_depth, constraints);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, pattern_depth) * 0.5); // Pruning helps
    plan.estimated_time_ms = plan.estimated_cost * 0.15; // Pattern matching is more expensive
    
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = false; // Cache not helpful for pattern matching
    plan.enable_early_termination = true; // Stop when pattern found
    plan.enable_parallel = false; // Pattern matching doesn't parallelize well
    
    plan.explanation = explainPlan(plan);
    
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
    
    OptimizationPlan plan;
    plan.pattern = QueryPattern::REACHABILITY;
    
    // For reachability, bidirectional BFS is often optimal
    size_t estimated_depth = estimateDepth(QueryPattern::REACHABILITY, constraints);
    
    if (estimated_depth > 3) {
        plan.algorithm = TraversalAlgorithm::BIDIRECTIONAL;
    } else {
        plan.algorithm = TraversalAlgorithm::BFS;
    }
    
    plan.estimated_cost = estimateCost(plan.algorithm, estimated_depth, constraints);
    plan.estimated_nodes_explored = static_cast<size_t>(
        std::pow(statistics_.avg_branching_factor, estimated_depth * 0.5)); // Bidirectional advantage
    plan.estimated_time_ms = plan.estimated_cost * 0.05; // Reachability is faster
    
    plan.use_index = statistics_.has_edge_index;
    plan.use_cache = statistics_.has_adjacency_cache;
    plan.enable_early_termination = true; // Stop as soon as path found
    plan.enable_parallel = shouldUseParallel(plan.algorithm, plan.estimated_nodes_explored);
    
    plan.explanation = explainPlan(plan);
    
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
    bool has_forbidden_nodes = false;
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
    plan.enable_parallel = false; // PathConstraints implementation doesn't support parallel yet
    
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

Result<std::vector<std::string>> GraphQueryOptimizer::executeBFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    
    std::vector<std::string> result;
    std::queue<std::pair<std::string, int>> queue;
    std::unordered_set<std::string> visited;
    
    queue.push({std::string(start_vertex), 0});
    visited.insert(std::string(start_vertex));
    
    while (!queue.empty()) {
        auto [current, depth] = queue.front();
        queue.pop();
        
        result.push_back(current);
        local_stats.nodes_explored++;
        
        if (depth >= max_depth) {
            continue;
        }
        
        // Get neighbors
        auto [status, neighbors] = graph_manager_.outNeighbors(current);
        if (!status.ok) {
            return Err<std::vector<std::string>>(
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                "Failed to get neighbors: " + status.message
            );
        }
        
        local_stats.edges_traversed += neighbors.size();
        
        // Apply edge type filter if specified
        std::vector<std::string> filtered_neighbors = neighbors;
        if (constraints.edge_type.has_value()) {
            // Edge type filtering would be done here
            // For now, we use all neighbors
        }
        
        for (const auto& neighbor : filtered_neighbors) {
            if (visited.find(neighbor) == visited.end()) {
                // Check forbidden vertices
                if (std::find(constraints.forbidden_vertices.begin(), 
                            constraints.forbidden_vertices.end(), neighbor) 
                    != constraints.forbidden_vertices.end()) {
                    continue;
                }
                
                visited.insert(neighbor);
                queue.push({neighbor, depth + 1});
                
                // Early termination check
                if (constraints.max_results.has_value() && 
                    result.size() >= constraints.max_results.value()) {
                    local_stats.early_terminated = true;
                    break;
                }
            }
        }
        
        if (local_stats.early_terminated) {
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

Result<std::vector<std::string>> GraphQueryOptimizer::executeDFS(
    std::string_view start_vertex,
    int max_depth,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    
    std::vector<std::string> result;
    std::vector<std::pair<std::string, int>> stack;
    std::unordered_set<std::string> visited;
    
    stack.push_back({std::string(start_vertex), 0});
    
    while (!stack.empty()) {
        auto [current, depth] = stack.back();
        stack.pop_back();
        
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
                errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                "Failed to get neighbors: " + status.message
            );
        }
        
        local_stats.edges_traversed += neighbors.size();
        
        for (const auto& neighbor : neighbors) {
            if (visited.find(neighbor) == visited.end()) {
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

Result<GraphIndexManager::PathResult> GraphQueryOptimizer::executeDijkstra(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    
    // Use existing Dijkstra implementation from GraphIndexManager
    auto [status, path_result] = graph_manager_.dijkstra(start_vertex, target_vertex);
    
    if (!status.ok) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "Dijkstra execution failed: " + status.message
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

Result<GraphIndexManager::PathResult> GraphQueryOptimizer::executeAStar(
    std::string_view start_vertex,
    std::string_view target_vertex,
    std::function<double(const std::string&)> heuristic,
    const QueryConstraints& constraints,
    ExecutionStats* stats) {
    
    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    
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
    
    auto start_time = std::chrono::steady_clock::now();
    ExecutionStats local_stats;
    
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
    
    while (!forward_queue.empty() || !backward_queue.empty()) {
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

Result<GraphQueryOptimizer::GraphStatistics> GraphQueryOptimizer::collectStatistics(
    std::optional<std::string_view> graph_id) {
    
    GraphStatistics stats;
    
    // Get topology statistics from GraphIndexManager
    stats.vertex_count = graph_manager_.getTopologyNodeCount();
    stats.edge_count = graph_manager_.getTopologyEdgeCount();
    
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
    
    return explanation;
}

void GraphQueryOptimizer::clearPlanCache() {
    plan_cache_.clear();
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
    
    return base_cost;
}

GraphQueryOptimizer::TraversalAlgorithm GraphQueryOptimizer::selectAlgorithm(
    QueryPattern pattern,
    size_t estimated_depth,
    const QueryConstraints& constraints) const {
    
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
}

} // namespace graph
} // namespace themis
