/**
 * @file distributed_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: distributed_graph.h | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 322
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5119 [Docs][Module] graph - Sync... (2026-05-13) | #4299 feat(graph): DistributedGra... (2026-03-16)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "graph/graph_query_optimizer.h"
#include "index/graph_index.h"
#include "utils/expected.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <functional>

namespace themis {
namespace graph {

/**
 * @brief Graph partitioning strategy for distributed graph execution.
 */
enum class PartitionStrategy {
    HASH,   ///< Uniform distribution by node ID hash (default)
    RANGE,  ///< Partition by node ID lexicographic ranges
    GEO,    ///< Partition by geographic region tag
    CUSTOM  ///< User-supplied partition function
};

/**
 * @brief Consistency level for distributed graph queries.
 */
enum class ConsistencyLevel {
    EVENTUAL,  ///< Best-effort: read from any replica (lower latency)
    STRONG     ///< Read from primary: guarantees up-to-date data (higher latency)
};

/**
 * @brief Configuration for the distributed graph manager.
 */
struct DistributedGraphConfig {
    PartitionStrategy partitioning = PartitionStrategy::HASH;
    int replication_factor = 1;
    ConsistencyLevel consistency = ConsistencyLevel::EVENTUAL;
    /// Maximum wall-clock time for a distributed query in milliseconds (0 = no limit).
    uint32_t timeout_ms = 5000;
    /// Maximum number of shards to query in parallel (0 = all).
    uint32_t max_parallel_shards = 0;
    /// Enable affinity caching for shard resolution results (QW-027).
    bool enable_affinity_cache = true;
    /// Maximum number of affinity cache entries (LRU eviction).
    size_t affinity_cache_size = 10000;

    DistributedGraphConfig() = default;
};

// ---------------------------------------------------------------------------
// ShardGraphExecutor interface
// ---------------------------------------------------------------------------

/**
 * @brief Abstract interface for executing graph traversals on a single shard.
 *
 * Implementors:
 *   - LocalShardGraphExecutor – wraps an in-process GraphIndexManager (tests /
 *                               single-node embedded mode).
 *   - RemoteShardGraphExecutor – serialises the query and sends it to a remote
 *                                shard over the existing RPC transport (production).
 *
 * All methods must be thread-safe: DistributedGraphManager may call them from
 * multiple threads simultaneously.
 */
class ShardGraphExecutor {
public:
    virtual ~ShardGraphExecutor() = default;

    /// Returns the unique identifier of this shard.
    [[nodiscard]] virtual std::string shardId() const = 0;

    /**
     * @brief Execute a BFS traversal on this shard and return visited vertex IDs.
     *
     * @param start_vertex Starting vertex ID (local, without shard qualifier).
     * @param max_depth    Maximum BFS depth.
     * @param constraints  Optional query constraints (edge type, forbidden vertices, …).
     * @return Visited vertex IDs, each qualified as "<id>@<shardId>".
     */
    [[nodiscard]] virtual Result<std::vector<std::string>> executeBFS(
        const std::string& start_vertex,
        int max_depth,
        const GraphQueryOptimizer::QueryConstraints& constraints) = 0;

    /**
     * @brief Execute Dijkstra's shortest path on this shard.
     *
     * @param start_vertex  Start vertex (local, without shard qualifier).
     * @param target_vertex Target vertex (local, without shard qualifier).
     * @param constraints   Optional query constraints.
     * @return PathResult with node IDs qualified as "<id>@<shardId>" and total cost.
     *         Returns ERR_GRAPH_PATH_NOT_FOUND when no path exists on this shard.
     */
    [[nodiscard]] virtual Result<GraphIndexManager::PathResult> executeDijkstra(
        const std::string& start_vertex,
        const std::string& target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints) = 0;

    /**
     * @brief Returns false when the shard is known to be unreachable.
     *
     * DistributedGraphManager skips unhealthy shards without paying a timeout
     * penalty.
     */
    virtual bool isHealthy() const { return true; }

    /**
     * @brief Compute per-vertex betweenness centrality on this shard.
     *
     * Uses a distributed variant of Brandes' algorithm: BFS from each sampled
     * source vertex to build the shortest-path DAG, then accumulates pair
     * dependencies back along the DAG.
     *
     * @param sample_fraction Fraction of vertices to use as BFS sources [0.01, 1.0].
     * @return Map from local vertex ID to raw (unnormalized) betweenness score.
     *         The default implementation returns an empty map so that existing
     *         shard implementors do not need to override this method.
     */
    [[nodiscard]] virtual Result<std::unordered_map<std::string, double>>
    computeLocalBetweenness(double /*sample_fraction*/) const {
        return Ok(std::unordered_map<std::string, double>{});
    }
};

// ---------------------------------------------------------------------------
// LocalShardGraphExecutor – thin wrapper around GraphQueryOptimizer / GraphIndexManager
// ---------------------------------------------------------------------------

/**
 * @brief Executes graph traversals against an in-process GraphIndexManager.
 *
 * Suitable for single-node embedded mode and unit tests.
 */
class LocalShardGraphExecutor final : public ShardGraphExecutor {
public:
    /**
     * @brief Construct a local executor.
     *
     * @param shard_id    Unique shard identifier.
     * @param graph_mgr   GraphIndexManager that owns this shard's data.
     *                    Must outlive this executor.
     */
    LocalShardGraphExecutor(std::string shard_id, GraphIndexManager& graph_mgr);

    std::string shardId() const override { return shard_id_; }

    Result<std::vector<std::string>> executeBFS(
        const std::string& start_vertex,
        int max_depth,
        const GraphQueryOptimizer::QueryConstraints& constraints) override;

    Result<GraphIndexManager::PathResult> executeDijkstra(
        const std::string& start_vertex,
        const std::string& target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints) override;

    /**
     * @brief Compute betweenness centrality for all vertices on this local shard.
     *
     * Implements Brandes' BFS-based algorithm over the shard's in-process graph.
     * Samples ceil(sample_fraction * |V|) source vertices in insertion order for
     * deterministic, reproducible results.
     *
     * @param sample_fraction Fraction of vertices used as BFS sources [0.01, 1.0].
     * @return Map from local vertex ID to raw (unnormalized) betweenness score.
     */
    [[nodiscard]] Result<std::unordered_map<std::string, double>>
    computeLocalBetweenness(double sample_fraction) const override;

private:
    std::string shard_id_;
    /**
     * @brief Reference to the graph manager for this shard.
     * 
     * @note **Ownership**: Non-owning reference. The GraphIndexManager must
     *       outlive this executor. Caller is responsible for managing lifetime.
     *       This is a reference (not smart pointer) for performance; the owner
     *       is typically a DistributedGraphManager or test harness.
     */
    GraphIndexManager& graph_mgr_;
    GraphQueryOptimizer optimizer_;

    /// Qualify a vertex ID returned by the local optimizer as "<id>@<shard_id_>".
    std::string qualify(const std::string& vertex_id) const;
};

// ---------------------------------------------------------------------------
// DistributedGraphManager
// ---------------------------------------------------------------------------

/**
 * @brief Coordinator for distributed graph query execution across shards.
 *
 * DistributedGraphManager fans out graph traversals to registered
 * ShardGraphExecutor instances in parallel, collects per-shard results, and
 * merges them into a single coherent answer that is transparent to the caller.
 *
 * Vertex IDs can optionally carry a shard qualifier ("node_A@shard1") to
 * hint the router towards a specific shard.  When the qualifier is absent the
 * manager uses the configured PartitionStrategy to choose the owning shard.
 *
 * Fault tolerance: unhealthy shards (isHealthy() == false) are skipped
 * automatically; the result is still returned from the remaining shards.
 *
 * Thread safety: all public methods are fully thread-safe.  Multiple
 * threads may call shortestPath(), kHopNeighbors(), shardIds(), shardCount(),
 * and resolveShardForVertex() concurrently without blocking each other
 * (shared_lock).  addShard() and removeShard() take an exclusive lock and
 * will briefly pause concurrent readers while the shard map is modified.
 *
 * Usage:
 * @code
 *   DistributedGraphConfig cfg;
 *   cfg.partitioning = PartitionStrategy::HASH;
 *   DistributedGraphManager mgr(cfg);
 *   mgr.addShard("shard1", std::make_shared<LocalShardGraphExecutor>("shard1", db1));
 *   mgr.addShard("shard2", std::make_shared<LocalShardGraphExecutor>("shard2", db2));
 *
 *   // Vertex IDs may carry a shard qualifier:
 *   auto result = mgr.shortestPath("node_A@shard1", "node_B@shard2");
 * @endcode
 */
class DistributedGraphManager {
public:
    explicit DistributedGraphManager(const DistributedGraphConfig& config = {});

    // -----------------------------------------------------------------------
    // Shard registry
    // -----------------------------------------------------------------------

    /**
     * @brief Register a new shard executor.
     *
     * @param shard_id  Unique shard identifier.
     * @param executor  Executor implementation for the shard.
     */
    void addShard(const std::string& shard_id,
                  std::shared_ptr<ShardGraphExecutor> executor);

    /**
     * @brief Deregister a shard.  Active queries may still reference its results.
     */
    void removeShard(const std::string& shard_id);

    /// Returns the IDs of all currently registered shards.
    std::vector<std::string> shardIds() const;

    /// Returns the number of registered shards.
    size_t shardCount() const;

    // -----------------------------------------------------------------------
    // Distributed query API
    // -----------------------------------------------------------------------

    /**
     * @brief Find the shortest weighted path between two vertices across all shards.
     *
     * Executes Dijkstra on each healthy shard in parallel and returns the globally
     * cheapest path.  Cross-shard edges are not modelled at this layer; the function
     * returns the best intra-shard path found.  For graphs that span shard boundaries
     * without explicit cross-shard edges, use kHopNeighbors to gather the reachable
     * frontier and then submit a targeted follow-up query.
     *
     * @param start_vertex  Source vertex ID (optionally qualified: "<id>@<shard>").
     * @param target_vertex Target vertex ID (optionally qualified: "<id>@<shard>").
     * @param constraints   Optional per-query execution constraints.
     * @return The lowest-cost PathResult found across all shards, or
     *         ERR_GRAPH_PATH_NOT_FOUND if no shard contains the path.
     */
    Result<GraphIndexManager::PathResult> shortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints = {});

    /**
     * @brief Collect all vertices reachable within k hops from a start vertex,
     *        spanning all healthy shards.
     *
     * BFS is executed on every healthy shard; results are merged (de-duplicated)
     * and returned as a flat list.
     *
     * @param start_vertex  Source vertex ID (optionally qualified: "<id>@<shard>").
     * @param k             Maximum hop count.
     * @param constraints   Optional per-query execution constraints.
     * @return Merged list of reachable vertex IDs, each qualified as "<id>@<shard>".
     *
     * @note **Move Semantics**: The returned vector is moved (not copied) to the caller
     *       via RVO. Caller owns the vector and may efficiently transfer ownership
     *       downstream without triggering allocations via std::move.
     */
    Result<std::vector<std::string>> kHopNeighbors(
       std::string_view start_vertex,
       int k,
       const GraphQueryOptimizer::QueryConstraints& constraints = {});

    /**
     * @brief Generate a shard-aware OptimizationPlan for a distributed query.
     *
     * The returned plan is backward-compatible with single-node plans: the new
     * `is_distributed`, `shard_ids`, and `recommended_parallelism` fields are
     * set only when more than one healthy shard participates.
     *
     * @param start_vertex  Source vertex ID.
     * @param target_vertex Target vertex ID (ignored for K_HOP_NEIGHBORS).
     * @param pattern       Query pattern type.
     * @param constraints   Query constraints.
     * @return Shard-aware OptimizationPlan.
     */
    Result<GraphQueryOptimizer::OptimizationPlan> optimizePlan(
        std::string_view start_vertex,
        std::string_view target_vertex,
        GraphQueryOptimizer::QueryPattern pattern,
        const GraphQueryOptimizer::QueryConstraints& constraints = {});

    // -----------------------------------------------------------------------
    // Distributed analytics
    // -----------------------------------------------------------------------

    /// @brief Result of a distributed betweenness centrality computation.
    struct BetweennessResult {
        /// Map from qualified vertex ID ("<id>@<shardId>") to normalized
        /// betweenness centrality score in [0, 1].
        std::unordered_map<std::string, double> scores;
        /// Number of shards that successfully contributed results.
        uint32_t shards_queried{0};
        /// Wall-clock computation time in milliseconds.
        uint64_t elapsed_ms{0};
    };

    /**
     * @brief Compute approximate distributed betweenness centrality across all shards.
     *
     * Uses a distributed variant of Brandes' algorithm: each shard independently
     * computes per-vertex pair dependencies via BFS, then results are aggregated
     * and normalized across all shards.
     *
     * Scores for vertices appearing in multiple shard results are summed before
     * normalization (cross-shard accumulation).  The maximum score across all
     * vertices is used as the denominator so all output scores lie in [0, 1].
     * When sample_fraction < 1.0 an additional 1/sample_fraction scaling is
     * applied before the [0, 1] clamp to approximate the true betweenness value.
     *
     * @param sample_fraction Fraction of vertices to use as BFS sources [0.01, 1.0].
     *        Use 1.0 for exact betweenness, <1.0 for approximate (faster on large
     *        graphs).
     * @param timeout_override_ms Optional per-call timeout in milliseconds
     *        (0 = use DistributedGraphConfig::timeout_ms).
     * @return BetweennessResult with per-vertex scores, shards_queried count,
     *         and elapsed_ms, or an error when no healthy shards are available or
     *         sample_fraction is out of range.
     */
    [[nodiscard]] Result<BetweennessResult> computeBetweennessCentrality(
        double sample_fraction = 1.0,
        uint32_t timeout_override_ms = 0) const;

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    /**
     * @brief Parse a qualified vertex ID into its local ID and shard tag.
     *
     * Format: "<local_id>@<shard_id>"  (shard_id is "" when no qualifier present).
     *
     * @param qualified_id  Vertex ID, optionally with shard qualifier.
     * @return {local_id, shard_id}
     */
    static std::pair<std::string, std::string> parseVertexId(
        std::string_view qualified_id);

    /**
     * @brief Determine the owning shard for a vertex using the configured strategy.
     *
     * @param local_vertex_id  Vertex ID without shard qualifier.
     * @return Shard ID that should own this vertex, or "" if no shards registered.
     */
    std::string resolveShardForVertex(const std::string& local_vertex_id) const;

private:
    DistributedGraphConfig config_;

    /**
     * @brief Registered shards: shard_id -> executor
     * 
     * @note **Ownership**: Uses std::shared_ptr for RAII compliance. Each executor
     *       is reference-counted; shards are automatically cleaned up when removed
     *       from the map and no other references exist. Thread-safe via shared_mutex.
     */
    std::unordered_map<std::string, std::shared_ptr<ShardGraphExecutor>> shards_;
    mutable std::shared_mutex shards_mutex_;

    /**
     * @brief QW-027: Affinity cache for shard resolution results (LRU via map).
     * 
     * @note **Ownership**: Cache entries are value-semantics strings; managed
     *       by the unordered_map container. LRU eviction is manual but RAII-safe.
     */
    mutable std::unordered_map<std::string, std::string> affinity_cache_;
    mutable std::shared_mutex affinity_cache_mutex_;
    mutable std::vector<std::string> affinity_cache_lru_;  ///< LRU eviction order

    /// Collect all healthy shard executors (snapshot under lock).
    std::vector<std::pair<std::string, std::shared_ptr<ShardGraphExecutor>>>
    healthyShards() const;

    /// Resolve per-query parallelism cap.
    size_t effectiveParallelism(size_t num_shards) const;

    /// QW-024: Execute kHopNeighbors on primary shard first, parallelize secondary shards.
    [[nodiscard]] Result<std::vector<std::string>> kHopNeighborsWithAffinity(
        std::string_view start_vertex,
        int k,
        const GraphQueryOptimizer::QueryConstraints& constraints,
        const std::string& primary_shard_id);

    /// QW-025: Merge multiple shard results using k-way merge for sorted results.
    void mergeShardResultsKWay(
        std::vector<std::vector<std::string>>& shard_results,
        std::vector<std::string>& merged_output,
        bool preserve_rank = false);

    /// QW-027: Get or update affinity cache for vertex shard resolution.
    std::string resolveShardForVertexWithCache(const std::string& local_vertex_id);

    /// QW-027: Invalidate all affinity cache entries (called when shards added/removed).
    void invalidateAffinityCache();
};

} // namespace graph
} // namespace themis

