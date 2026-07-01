/**
 * @file distributed_graph.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=7, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_graph.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 375
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=10, M=12, L=0
 * PR History (last 5): #4299 feat(graph): DistributedGra... (2026-03-16) | #3571 feat(graph): register missi... (2026-03-12) | #2955 fix(graph): distributed que... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Distributed graph query execution across shards.

#include "graph/distributed_graph.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <queue>
#include <functional>
#include <future>
#include <limits>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <thread>
#include <unordered_set>

#include "utils/error_registry.h"

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// LocalShardGraphExecutor
// ─────────────────────────────────────────────────────────────────────────────

LocalShardGraphExecutor::LocalShardGraphExecutor(std::string shard_id, GraphIndexManager &graph_mgr)
    : shard_id_(std::move(shard_id)), graph_mgr_(graph_mgr), optimizer_(graph_mgr) {}

std::string LocalShardGraphExecutor::qualify(const std::string &vertex_id) const {
    // Already qualified – avoid double-qualifying.
    if (vertex_id.find('@') != std::string::npos) {
        return vertex_id;
    }
    return vertex_id + "@" + shard_id_;
}

Result<std::vector<std::string>>
LocalShardGraphExecutor::executeBFS(const std::string &start_vertex, int max_depth,
                                    const GraphQueryOptimizer::QueryConstraints &constraints) {
    auto res = optimizer_.executeBFS(start_vertex, max_depth, constraints);
    if (!res) {
        return res;
    }

    // Qualify every returned vertex ID with this shard's tag.
    std::vector<std::string> qualified;
    qualified.reserve(res->size());
    for (const auto &v : *res) {
        qualified.push_back(qualify(v));
    }
    return Ok(std::move(qualified));
}

Result<GraphIndexManager::PathResult>
LocalShardGraphExecutor::executeDijkstra(const std::string &start_vertex, const std::string &target_vertex,
                                         const GraphQueryOptimizer::QueryConstraints &constraints) {
    auto res = optimizer_.executeDijkstra(start_vertex, target_vertex, constraints);
    if (!res) {
        return res;
    }

    // Qualify every node in the returned path.
    GraphIndexManager::PathResult qualified_path;
    qualified_path.totalCost = res->totalCost;
    qualified_path.path.reserve(res->path.size());
    for (const auto &v : res->path) {
        qualified_path.path.push_back(qualify(v));
    }
    return Ok(std::move(qualified_path));
}

// ─────────────────────────────────────────────────────────────────────────────
// LocalShardGraphExecutor::computeLocalBetweenness
// ─────────────────────────────────────────────────────────────────────────────

Result<std::unordered_map<std::string, double>>
LocalShardGraphExecutor::computeLocalBetweenness(double sample_fraction) const {
    // Collect all vertices known to this shard's graph.
    const std::vector<std::string> all_vertices = graph_mgr_.getAllVertices();
    if (all_vertices.empty()) {
        return Ok(std::unordered_map<std::string, double>{});
    }

    // Initialise betweenness scores at 0 for every known vertex.
    std::unordered_map<std::string, double> betweenness;
    betweenness.reserve(all_vertices.size());
    for (const auto &v : all_vertices) {
        betweenness[v] = 0.0;
    }

    // Determine the number of source vertices to use as BFS origins.
    const std::size_t n = all_vertices.size();
    const std::size_t sample_count =
        std::max(std::size_t{1},
                 static_cast<std::size_t>(std::ceil(sample_fraction * static_cast<double>(n))));
    const std::size_t effective_count = std::min(sample_count, n);

    // Brandes' BFS-based betweenness centrality (directed graph).
    for (std::size_t si = 0; si < effective_count; ++si) {
        const std::string &s = all_vertices[si];

        // S   : vertices in order of non-increasing distance from s (for back-propagation).
        // P   : predecessor lists on shortest-path DAG.
        // sigma : number of shortest paths from s to each vertex.
        // d   : BFS distance from s (-1 = not yet visited).
        // delta : dependency accumulator.
        std::vector<std::string> S;
        S.reserve(n);
        std::unordered_map<std::string, std::vector<std::string>> P;
        std::unordered_map<std::string, double> sigma;
        std::unordered_map<std::string, int>    d;
        std::unordered_map<std::string, double> delta;

        // Pre-seed with all known vertices so the maps are dense.
        for (const auto &v : all_vertices) {
            P[v]     = {};
            sigma[v] = 0.0;
            d[v]     = -1;
            delta[v] = 0.0;
        }

        sigma[s] = 1.0;
        d[s]     = 0;

        std::queue<std::string> Q;
        Q.push(s);

        while (!Q.empty()) {
            const std::string v = std::move(Q.front());
            Q.pop();
            S.push_back(v);

            auto [status, neighbors] = graph_mgr_.outNeighbors(v);
            if (!status.ok) {
                continue;
            }

            for (const auto &w : neighbors) {
                // Ensure w is tracked (it may be a vertex only appearing as a target).
                if (betweenness.find(w) == betweenness.end()) {
                    betweenness[w] = 0.0;
                }
                if (P.find(w) == P.end()) {
                    P[w]     = {};
                    sigma[w] = 0.0;
                    d[w]     = -1;
                    delta[w] = 0.0;
                }

                // First discovery of w via BFS.
                if (d[w] < 0) {
                    Q.push(w);
                    d[w] = d[v] + 1;
                }

                // w reachable via v on a shortest path.
                if (d[w] == d[v] + 1) {
                    sigma[w] += sigma[v];
                    P[w].push_back(v);
                }
            }
        }

        // Back-propagate dependencies along the shortest-path DAG.
        while (!S.empty()) {
            const std::string w = std::move(S.back());
            S.pop_back();
            for (const auto &pred : P[w]) {
                if (sigma[w] > 0.0) {
                    delta[pred] += (sigma[pred] / sigma[w]) * (1.0 + delta[w]);
                }
            }
            if (w != s) {
                betweenness[w] += delta[w];
            }
        }
    }

    return Ok(std::move(betweenness));
}

// ─────────────────────────────────────────────────────────────────────────────
// DistributedGraphManager helpers
// ─────────────────────────────────────────────────────────────────────────────

DistributedGraphManager::DistributedGraphManager(const DistributedGraphConfig &config) : config_(config) {}

void DistributedGraphManager::addShard(const std::string &shard_id, std::shared_ptr<ShardGraphExecutor> executor) {
    std::unique_lock<std::shared_mutex> lock(shards_mutex_);
    shards_[shard_id] = std::move(executor);
}

void DistributedGraphManager::removeShard(const std::string &shard_id) {
    std::unique_lock<std::shared_mutex> lock(shards_mutex_);
    shards_.erase(shard_id);
}

std::vector<std::string> DistributedGraphManager::shardIds() const {
    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    std::vector<std::string> ids;
    ids.reserve(shards_.size());
    for (const auto &[id, _] : shards_) {
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
    for (const auto &[id, exec] : shards_) {
        if (exec && exec->isHealthy()) {
            result.emplace_back(id, exec);
        }
    }
    return result;
}

size_t DistributedGraphManager::effectiveParallelism(size_t num_shards) const {
    if (config_.max_parallel_shards == 0) {
        return num_shards;
    }
    return std::min(num_shards, static_cast<size_t>(config_.max_parallel_shards));
}

// ─────────────────────────────────────────────────────────────────────────────
// Vertex ID utilities
// ─────────────────────────────────────────────────────────────────────────────

/*static*/
std::pair<std::string, std::string> DistributedGraphManager::parseVertexId(std::string_view qualified_id) {
    auto pos = qualified_id.rfind('@');
    if (pos == std::string_view::npos) {
        return {std::string(qualified_id), ""};
    }
    return {std::string(qualified_id.substr(0, pos)), std::string(qualified_id.substr(pos + 1))};
}

std::string DistributedGraphManager::resolveShardForVertex(const std::string &local_vertex_id) const {
    std::shared_lock<std::shared_mutex> lock(shards_mutex_);
    if (shards_.empty()) {
        return "";
    }

    // Collect shard IDs in a stable order for deterministic hashing.
    std::vector<std::string> ordered;
    ordered.reserve(shards_.size());
    for (const auto &[id, _] : shards_) {
        ordered.push_back(id);
    }
    std::sort(ordered.begin(), ordered.end());

    switch (config_.partitioning) {
        case PartitionStrategy::RANGE: {
            // Lexicographic: vertex goes to the first shard whose ID >= vertex_id,
            // wrapping around to the first shard if none qualifies.
            for (const auto &sid : ordered) {
                if (local_vertex_id <= sid) {
                    return sid;
                }
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

Result<GraphIndexManager::PathResult>
DistributedGraphManager::shortestPath(std::string_view start_vertex, std::string_view target_vertex,
                                      const GraphQueryOptimizer::QueryConstraints &constraints) {
    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<GraphIndexManager::PathResult>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                                  "No healthy shards available for distributed shortest path query");
    }

    // Parse shard qualifiers from vertex IDs.
    auto [start_local, start_shard]   = parseVertexId(start_vertex);
    auto [target_local, target_shard] = parseVertexId(target_vertex);

    // If both endpoints reside on the same shard (explicitly or via resolution),
    // route the query directly to that shard for minimum latency.
    if (start_shard.empty()) {
        start_shard = resolveShardForVertex(start_local);
    }
    if (target_shard.empty()) {
        target_shard = resolveShardForVertex(target_local);
    }

    // Build optional single-shard fast path.
    if (!start_shard.empty() && start_shard == target_shard) {
        for (auto &[sid, exec] : shards) {
            if (sid == start_shard) {
                auto res = exec->executeDijkstra(start_local, target_local, constraints);
                // Normalize empty path as "not found" at distributed layer so
                // callers receive ERR_GRAPH_PATH_NOT_FOUND consistently.
                if (res && !res->path.empty()) {
                    return res;
                }
                // Fall through to global search if the shard failed.
                break;
            }
        }
    }

    // Fan out to all healthy shards in parallel; keep the globally cheapest path.
    std::vector<std::future<Result<GraphIndexManager::PathResult>>> futures;
    futures.reserve(shards.size());

    for (auto &[sid, exec] : shards) {
        futures.push_back(
            std::async(std::launch::async, [exec_ptr = exec.get(), &start_local, &target_local, &constraints]() {
                return exec_ptr->executeDijkstra(start_local, target_local, constraints);
            }));
    }

    // Collect results; pick the globally cheapest path.
    GraphIndexManager::PathResult best;
    best.totalCost = std::numeric_limits<double>::infinity();
    bool found_any = false;

    for (auto &f : futures) {
        auto res = f.get();
        if (!res || res->path.empty()) {
            continue; // this shard has no path
        }
        if (res->totalCost < best.totalCost) {
            best      = *res;
            found_any = true;
        }
    }

    if (!found_any) {
        return Err<GraphIndexManager::PathResult>(errors::ErrorCode::ERR_GRAPH_PATH_NOT_FOUND,
                                                  "No path found from '" + std::string(start_vertex) + "' to '"
                                                      + std::string(target_vertex) + "' across all shards");
    }

    return Ok(std::move(best));
}

// ─────────────────────────────────────────────────────────────────────────────
// kHopNeighbors – distributed BFS across shards
// ─────────────────────────────────────────────────────────────────────────────

Result<std::vector<std::string>>
DistributedGraphManager::kHopNeighbors(std::string_view start_vertex, int k,
                                       const GraphQueryOptimizer::QueryConstraints &constraints) {
    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<std::vector<std::string>>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                             "No healthy shards available for distributed k-hop neighbors query");
    }

    auto [start_local, start_shard] = parseVertexId(start_vertex);
    if (start_shard.empty()) {
        start_shard = resolveShardForVertex(start_local);
    }

    // Fan out BFS to all healthy shards in parallel.
    std::vector<std::future<Result<std::vector<std::string>>>> futures;
    futures.reserve(shards.size());

    for (auto &[sid, exec] : shards) {
        futures.push_back(std::async(std::launch::async, [exec_ptr = exec.get(), start_local, k, &constraints]() {
            return exec_ptr->executeBFS(start_local, k, constraints);
        }));
    }

    // Merge results: de-duplicate qualified vertex IDs.
    std::unordered_set<std::string> seen;
    std::vector<std::string> merged;

    for (auto &f : futures) {
        auto res = f.get();
        if (!res) {
            spdlog::debug("distributed_graph: k-hop BFS shard returned error, skipping");
            continue;
        }
        for (const auto &v : *res) {
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

Result<GraphQueryOptimizer::OptimizationPlan>
DistributedGraphManager::optimizePlan([[maybe_unused]] std::string_view start_vertex,
                                      [[maybe_unused]] std::string_view target_vertex,
                                      GraphQueryOptimizer::QueryPattern pattern,
                                      [[maybe_unused]] const GraphQueryOptimizer::QueryConstraints &constraints) {
    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<GraphQueryOptimizer::OptimizationPlan>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "No healthy shards available to generate distributed query plan");
    }

    // Use the first available shard's optimizer to generate a base plan, then
    // annotate it with shard-aware distribution metadata.
    auto &[first_shard_id, first_exec] = shards.front();
    // reserved for future remote-plan generation

    // Build a base plan with sensible defaults for distributed execution.
    GraphQueryOptimizer::OptimizationPlan plan;
    plan.pattern                  = pattern;
    plan.algorithm                = GraphQueryOptimizer::TraversalAlgorithm::DIJKSTRA;
    plan.estimated_cost           = 1.0 * static_cast<double>(shards.size());
    plan.estimated_time_ms        = 10.0 * static_cast<double>(shards.size());
    plan.estimated_nodes_explored = 100 * shards.size();
    plan.use_index                = true;
    plan.use_cache                = false;
    plan.enable_early_termination = true;
    plan.enable_parallel          = shards.size() > 1;

    // Shard-aware plan fields (v1.8.0).
    plan.is_distributed          = shards.size() > 1;
    plan.recommended_parallelism = effectiveParallelism(shards.size());
    for (auto &[sid, _] : shards) {
        plan.shard_ids.push_back(sid);
    }

    plan.explanation = "Distributed plan across " + std::to_string(shards.size())
                       + " shard(s).\n"
                         "Partition strategy: "
                       + [&]() -> std::string {
        switch (config_.partitioning) {
            case PartitionStrategy::HASH:
                return "HASH";
            case PartitionStrategy::RANGE:
                return "RANGE";
            case PartitionStrategy::GEO:
                return "GEO";
            case PartitionStrategy::CUSTOM:
                return "CUSTOM";
        }
        return "UNKNOWN";
    }() + "\n" + "Parallelism: " + std::to_string(plan.recommended_parallelism)
                                      + "\n";

    return Ok(std::move(plan));
}

// ─────────────────────────────────────────────────────────────────────────────
// computeBetweennessCentrality – distributed Brandes across shards
// ─────────────────────────────────────────────────────────────────────────────

Result<DistributedGraphManager::BetweennessResult>
DistributedGraphManager::computeBetweennessCentrality(double sample_fraction,
                                                      uint32_t timeout_override_ms) const {
    // Validate sample_fraction range.
    if (sample_fraction < 0.01 || sample_fraction > 1.0) {
        return Err<BetweennessResult>(
            errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
            "sample_fraction must be in [0.01, 1.0], got: " + std::to_string(sample_fraction));
    }

    auto shards = healthyShards();
    if (shards.empty()) {
        return Err<BetweennessResult>(errors::ErrorCode::ERR_QUERY_EXECUTION_FAILED,
                                      "No healthy shards available for betweenness centrality computation");
    }

    const auto t_start = std::chrono::steady_clock::now();

    // Resolve effective per-call timeout.
    const uint32_t effective_timeout_ms =
        (timeout_override_ms > 0) ? timeout_override_ms : config_.timeout_ms;

    // Fan out computeLocalBetweenness to every healthy shard in parallel.
    std::vector<std::future<Result<std::unordered_map<std::string, double>>>> futures;
    futures.reserve(shards.size());

    for (auto &[sid, exec] : shards) {
        futures.push_back(
            std::async(std::launch::async,
                       [exec_ptr = exec.get(), sample_fraction]() {
                           return exec_ptr->computeLocalBetweenness(sample_fraction);
                       }));
    }

    // Collect per-shard results and merge by summing scores for each qualified vertex.
    std::unordered_map<std::string, double> merged;
    uint32_t shards_queried = 0;

    for (std::size_t i = 0; i < futures.size(); ++i) {
        // Enforce timeout before waiting on the next future.
        if (effective_timeout_ms > 0) {
            const auto elapsed_now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::steady_clock::now() - t_start)
                                         .count();
            if (static_cast<uint32_t>(elapsed_now) >= effective_timeout_ms) {
                spdlog::warn("distributed_graph: betweenness centrality timeout after {}ms "
                             "({}/{} shards queried)",
                             elapsed_now, shards_queried, shards.size());
                break;
            }
        }

        auto res = futures[i].get();
        if (!res) {
            spdlog::debug("distributed_graph: shard '{}' betweenness failed, skipping",
                          shards[i].first);
            continue;
        }

        ++shards_queried;

        // Qualify each vertex ID with its originating shard tag and accumulate.
        for (const auto &[vid, score] : *res) {
            const std::string qualified = vid + "@" + shards[i].first;
            merged[qualified] += score;
        }
    }

    // Normalize: find global maximum and scale all scores to [0, 1].
    double max_score = 0.0;
    for (const auto &[vid, score] : merged) {
        max_score = std::max(max_score, score);
    }

    if (max_score > 0.0) {
        for (auto &[vid, score] : merged) {
            score /= max_score; // now in [0, 1]

            // Apply sample fraction compensation: if sample_fraction < 1.0 the
            // partial-BFS under-counts betweenness; scale up to approximate the
            // true value, then re-clamp to [0, 1].
            if (sample_fraction < 1.0) {
                score = std::min(1.0, score / sample_fraction);
            }
        }
    }

    const auto t_end = std::chrono::steady_clock::now();
    const uint64_t elapsed_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());

    BetweennessResult result;
    result.scores        = std::move(merged);
    result.shards_queried = shards_queried;
    result.elapsed_ms    = elapsed_ms;

    return Ok(std::move(result));
}

} // namespace graph
} // namespace themis
