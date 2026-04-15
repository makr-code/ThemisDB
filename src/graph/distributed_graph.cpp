/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            distributed_graph.cpp                              ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:12:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     386                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • 5bfa861df6  2026-03-23  Add runtime DLL copying functionality and error handling ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Distributed graph query execution across shards.

#include "graph/distributed_graph.h"
#include "utils/error_registry.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <limits>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_set>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// LocalShardGraphExecutor
// ─────────────────────────────────────────────────────────────────────────────

LocalShardGraphExecutor::LocalShardGraphExecutor(std::string shard_id,
                                                 GraphIndexManager& graph_mgr)
    : shard_id_(std::move(shard_id)), optimizer_(graph_mgr) {}

std::string LocalShardGraphExecutor::qualify(const std::string& vertex_id) const {
    // Already qualified – avoid double-qualifying.
    if (vertex_id.find('@') != std::string::npos) return vertex_id;
    return vertex_id + "@" + shard_id_;
}

Result<std::vector<std::string>> LocalShardGraphExecutor::executeBFS(
    const std::string& start_vertex,
    int max_depth,
    const GraphQueryOptimizer::QueryConstraints& constraints) {

    auto res = optimizer_.executeBFS(start_vertex, max_depth, constraints);
    if (!res) return res;

    // Qualify every returned vertex ID with this shard's tag.
    std::vector<std::string> qualified;
    qualified.reserve(res->size());
    for (const auto& v : *res) {
        qualified.push_back(qualify(v));
    }
    return Ok(std::move(qualified));
}

Result<GraphIndexManager::PathResult> LocalShardGraphExecutor::executeDijkstra(
    const std::string& start_vertex,
    const std::string& target_vertex,
    const GraphQueryOptimizer::QueryConstraints& constraints) {

    auto res = optimizer_.executeDijkstra(start_vertex, target_vertex, constraints);
    if (!res) return res;

    // Qualify every node in the returned path.
    GraphIndexManager::PathResult qualified_path;
    qualified_path.totalCost = res->totalCost;
    qualified_path.path.reserve(res->path.size());
    for (const auto& v : res->path) {
        qualified_path.path.push_back(qualify(v));
    }
    return Ok(std::move(qualified_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// DistributedGraphManager helpers
// ─────────────────────────────────────────────────────────────────────────────

DistributedGraphManager::DistributedGraphManager(const DistributedGraphConfig& config)
    : config_(config) {}

void DistributedGraphManager::addShard(const std::string& shard_id,
                                       std::shared_ptr<ShardGraphExecutor> executor) {
    std::unique_lock<std::shared_mutex> lock(shards_mutex_);
    shards_[shard_id] = std::move(executor);
}

void DistributedGraphManager::removeShard(const std::string& shard_id) {
    std::unique_lock<std::shared_mutex> lock(shards_mutex_);
    shards_.erase(shard_id);
}

std::vector<std::string> DistributedGraphManager::shardIds() const {
    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    std::vector<std::string> ids;
    ids.reserve(shards_.size());
    for (const auto& [id, _] : shards_) {
        ids.push_back(id);
    }
    return ids;
}

size_t DistributedGraphManager::shardCount() const {
    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    return shards_.size();
}

std::vector<std::pair<std::string, std::shared_ptr<ShardGraphExecutor>>>
DistributedGraphManager::healthyShards() const {
    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    std::vector<std::pair<std::string, std::shared_ptr<ShardGraphExecutor>>> result;
    result.reserve(shards_.size());
    for (const auto& [id, exec] : shards_) {
        if (exec && exec->isHealthy()) {
            result.emplace_back(id, exec);
        }
    }
    return result;
}

size_t DistributedGraphManager::effectiveParallelism(size_t num_shards) const {
    if (config_.max_parallel_shards == 0) return num_shards;
    return std::min(num_shards, static_cast<size_t>(config_.max_parallel_shards));
}

// ─────────────────────────────────────────────────────────────────────────────
// Vertex ID utilities
// ─────────────────────────────────────────────────────────────────────────────

/*static*/
std::pair<std::string, std::string>
DistributedGraphManager::parseVertexId(std::string_view qualified_id) {
    auto pos = qualified_id.rfind('@');
    if (pos == std::string_view::npos) {
        return {std::string(qualified_id), ""};
    }
    return {
        std::string(qualified_id.substr(0, pos)),
        std::string(qualified_id.substr(pos + 1))
    };
}

std::string DistributedGraphManager::resolveShardForVertex(
    const std::string& local_vertex_id) const {

    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    if (shards_.empty()) return "";

    // Collect shard IDs in a stable order for deterministic hashing.
    std::vector<std::string> ordered;
    ordered.reserve(shards_.size());
    for (const auto& [id, _] : shards_) {
        ordered.push_back(id);
    }
    std::sort(ordered.begin(), ordered.end());

    switch (config_.partitioning) {
        case PartitionStrategy::RANGE: {
            // Lexicographic: vertex goes to the first shard whose ID >= vertex_id,
            // wrapping around to the first shard if none qualifies.
            for (const auto& sid : ordered) {
                if (local_vertex_id <= sid) return sid;
            }
            return ordered.front();
        }
        case PartitionStrategy::GEO:
            // Geographic partitioning requires external metadata; fall through to
            // HASH as a safe default when no metadata is available.
            [[fallthrough]];
        case PartitionStrategy::CUSTOM:
            [[fallthrough]];
        case PartitionStrategy::HASH:
        default: {
            // FNV-1a hash → uniform bucket assignment.
            uint64_t h = 14695981039346656037ULL;
            for (unsigned char c : local_vertex_id) {
                h ^= static_cast<uint64_t>(c);
                h *= 1099511628211ULL;
            }
            return ordered[h % ordered.size()];
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// shortestPath – distributed Dijkstra across shards
// ─────────────────────────────────────────────────────────────────────────────

Result<GraphIndexManager::PathResult> DistributedGraphManager::shortestPath(
    std::string_view start_vertex,
    std::string_view target_vertex,
    const GraphQueryOptimizer::QueryConstraints& constraints) {

    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "No healthy shards available for distributed shortest path query");
    }

    // Parse shard qualifiers from vertex IDs.
    auto [start_local, start_shard] = parseVertexId(start_vertex);
    auto [target_local, target_shard] = parseVertexId(target_vertex);

    // If both endpoints reside on the same shard (explicitly or via resolution),
    // route the query directly to that shard for minimum latency.
    if (start_shard.empty()) start_shard = resolveShardForVertex(start_local);
    if (target_shard.empty()) target_shard = resolveShardForVertex(target_local);

    // Build optional single-shard fast path.
    if (!start_shard.empty() && start_shard == target_shard) {
        for (auto& [sid, exec] : shards) {
            if (sid == start_shard) {
                auto res = exec->executeDijkstra(start_local, target_local, constraints);
                // Normalize empty path as "not found" at distributed layer so
                // callers receive ERR_GRAPH_PATH_NOT_FOUND consistently.
                if (res && !res->path.empty()) return res;
                // Fall through to global search if the shard failed.
                break;
            }
        }
    }

    // Fan out to all healthy shards in parallel; keep the globally cheapest path.
    std::vector<std::future<Result<GraphIndexManager::PathResult>>> futures;
    futures.reserve(shards.size());

    for (auto& [sid, exec] : shards) {
        futures.push_back(std::async(std::launch::async,
            [exec_ptr = exec.get(), &start_local, &target_local, &constraints]() {
                return exec_ptr->executeDijkstra(start_local, target_local, constraints);
            }));
    }

    // Collect results; pick the globally cheapest path.
    GraphIndexManager::PathResult best;
    best.totalCost = std::numeric_limits<double>::infinity();
    bool found_any = false;

    for (auto& f : futures) {
        auto res = f.get();
        if (!res || res->path.empty()) continue;  // this shard has no path
        if (res->totalCost < best.totalCost) {
            best = *res;
            found_any = true;
        }
    }

    if (!found_any) {
        return Err<GraphIndexManager::PathResult>(
            errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND,
            "No path found from '" + std::string(start_vertex) +
            "' to '" + std::string(target_vertex) + "' across all shards");
    }

    return Ok(std::move(best));
}

// ─────────────────────────────────────────────────────────────────────────────
// kHopNeighbors – distributed BFS across shards
// ─────────────────────────────────────────────────────────────────────────────

Result<std::vector<std::string>> DistributedGraphManager::kHopNeighbors(
    std::string_view start_vertex,
    int k,
    const GraphQueryOptimizer::QueryConstraints& constraints) {

    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<std::vector<std::string>>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "No healthy shards available for distributed k-hop neighbors query");
    }

    auto [start_local, start_shard] = parseVertexId(start_vertex);
    if (start_shard.empty()) start_shard = resolveShardForVertex(start_local);

    // Fan out BFS to all healthy shards in parallel.
    std::vector<std::future<Result<std::vector<std::string>>>> futures;
    futures.reserve(shards.size());

    for (auto& [sid, exec] : shards) {
        futures.push_back(std::async(std::launch::async,
            [exec_ptr = exec.get(), start_local, k, &constraints]() {
                return exec_ptr->executeBFS(start_local, k, constraints);
            }));
    }

    // Merge results: de-duplicate qualified vertex IDs.
    std::unordered_set<std::string> seen;
    std::vector<std::string> merged;

    for (auto& f : futures) {
        auto res = f.get();
        if (!res) {
            spdlog::debug("distributed_graph: k-hop BFS shard returned error, skipping");
            continue;
        }
        for (const auto& v : *res) {
            if (seen.insert(v).second) {
                merged.push_back(v);
            }
        }
    }

    return Ok(std::move(merged));
}

// ─────────────────────────────────────────────────────────────────────────────
// optimizePlan – shard-aware plan generation
// ─────────────────────────────────────────────────────────────────────────────

Result<GraphQueryOptimizer::OptimizationPlan> DistributedGraphManager::optimizePlan(
    std::string_view start_vertex,
    std::string_view target_vertex,
    GraphQueryOptimizer::QueryPattern pattern,
    const GraphQueryOptimizer::QueryConstraints& constraints) {

    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<GraphQueryOptimizer::OptimizationPlan>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "No healthy shards available to generate distributed query plan");
    }

    // Use the first available shard's optimizer to generate a base plan, then
    // annotate it with shard-aware distribution metadata.
    auto& [first_shard_id, first_exec] = shards.front();
    // reserved for future remote-plan generation

    // Build a base plan with sensible defaults for distributed execution.
    GraphQueryOptimizer::OptimizationPlan plan;
    plan.pattern = pattern;
    plan.algorithm = GraphQueryOptimizer::TraversalAlgorithm::DIJKSTRA;
    plan.estimated_cost = 1.0 * static_cast<double>(shards.size());
    plan.estimated_time_ms = 10.0 * static_cast<double>(shards.size());
    plan.estimated_nodes_explored = 100 * shards.size();
    plan.use_index = true;
    plan.use_cache = false;
    plan.enable_early_termination = true;
    plan.enable_parallel = shards.size() > 1;

    // Shard-aware plan fields (v1.8.0).
    plan.is_distributed = shards.size() > 1;
    plan.recommended_parallelism = effectiveParallelism(shards.size());
    for (auto& [sid, _] : shards) {
        plan.shard_ids.push_back(sid);
    }

    plan.explanation =
        "Distributed plan across " + std::to_string(shards.size()) + " shard(s).\n"
        "Partition strategy: " + [&]() -> std::string {
            switch (config_.partitioning) {
                case PartitionStrategy::HASH:   return "HASH";
                case PartitionStrategy::RANGE:  return "RANGE";
                case PartitionStrategy::GEO:    return "GEO";
                case PartitionStrategy::CUSTOM: return "CUSTOM";
            }
            return "UNKNOWN";
        }() + "\n" +
        "Parallelism: " + std::to_string(plan.recommended_parallelism) + "\n";

    return Ok(std::move(plan));
}

} // namespace graph
} // namespace themis
