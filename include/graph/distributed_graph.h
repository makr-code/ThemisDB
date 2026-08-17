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

    /// @brief Move constructor for polymorphic shard executor base.
    /// @note Move semantics: abstract base carries no data; derived classes must delegate here.
    ShardGraphExecutor(ShardGraphExecutor&&) noexcept noexcept = default;

    /// @brief Move assignment operator for polymorphic shard executor base.
    /// @note Move semantics: abstract base carries no data; safe as no-op base move.
    ShardGraphExecutor& operator=(ShardGraphExecutor&&) noexcept noexcept = default;

    ShardGraphExecutor(const ShardGraphExecutor&) = delete;
    ShardGraphExecutor& operator=(const ShardGraphExecutor&) = delete;

protected:
    ShardGraphExecutor() = default;

public:
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

    /// @brief Move constructor — transfers shard_id and optimizer; source shard_id left empty.
    /// @note Move semantics: std::string move clears source shard_id_; optimizer moved via its own move.
    LocalShardGraphExecutor(LocalShardGraphExecutor&&) noexcept noexcept = default;

    /// @brief Move assignment operator.
    /// @note Move semantics: all members replaced; old shard_id and optimizer discarded.
    LocalShardGraphExecutor& operator=(LocalShardGraphExecutor&&) noexcept noexcept = default;

    LocalShardGraphExecutor(const LocalShardGraphExecutor&) = delete;
    LocalShardGraphExecutor& operator=(const LocalShardGraphExecutor&) = delete;

    std::string shardId() const override { return shard_id_; }

    Result<std::vector<std::string>> executeBFS(
        const std::string& start_vertex,
        int max_depth,
        const GraphQueryOptimizer::QueryConstraints& constraints) override;

    Result<GraphIndexManager::PathResult> executeDijkstra(
        const std::string& start_vertex,
        const std::string& target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints) override;

private:
    std::string shard_id_;
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

    // Registered shards: shard_id -> executor
    std::unordered_map<std::string, std::shared_ptr<ShardGraphExecutor>> shards_;
    mutable std::shared_mutex shards_mutex_;

    /// Collect all healthy shard executors (snapshot under lock).
    std::vector<std::pair<std::string, std::shared_ptr<ShardGraphExecutor>>>
    healthyShards() const;

    /// Resolve per-query parallelism cap.
    size_t effectiveParallelism(size_t num_shards) const;
};

} // namespace graph
} // namespace themis

