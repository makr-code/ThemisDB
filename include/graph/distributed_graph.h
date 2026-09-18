/**
 * @file distributed_graph.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class PartitionStrategy {
    HASH,   ///< Uniform distribution by node ID hash (default)
    RANGE,  ///< Partition by node ID lexicographic ranges
    GEO,    ///< Partition by geographic region tag
    CUSTOM  ///< User-supplied partition function
};

enum class ConsistencyLevel {
    EVENTUAL,  ///< Best-effort: read from any replica (lower latency)
    STRONG     ///< Read from primary: guarantees up-to-date data (higher latency)
};

struct DistributedGraphConfig {
    PartitionStrategy partitioning = PartitionStrategy::HASH;
    int replication_factor = 1;
    ConsistencyLevel consistency = ConsistencyLevel::EVENTUAL;
    uint32_t timeout_ms = 5000;
    uint32_t max_parallel_shards = 0;

    DistributedGraphConfig() = default;
};

// ---------------------------------------------------------------------------
// ShardGraphExecutor interface
// ---------------------------------------------------------------------------

class ShardGraphExecutor {
public:
    /**
     * @brief Shard Graph Executor.
     * @return Return value.
     */
    virtual ~ShardGraphExecutor() = default;

    ShardGraphExecutor(ShardGraphExecutor&&) noexcept = default;

    ShardGraphExecutor& operator=(ShardGraphExecutor&&) noexcept = default;

    ShardGraphExecutor(const ShardGraphExecutor&) = delete;
    ShardGraphExecutor& operator=(const ShardGraphExecutor&) = delete;

protected:
    ShardGraphExecutor() = default;

public:
    [[nodiscard]] virtual std::string shardId() const = 0;

    [[nodiscard]] virtual Result<std::vector<std::string>> executeBFS(
        const std::string& start_vertex,
        int max_depth,
        const GraphQueryOptimizer::QueryConstraints& constraints) = 0;

    [[nodiscard]] virtual Result<GraphIndexManager::PathResult> executeDijkstra(
        const std::string& start_vertex,
        const std::string& target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints) = 0;

    virtual bool isHealthy() const { return true; }
};

// ---------------------------------------------------------------------------
// LocalShardGraphExecutor – thin wrapper around GraphQueryOptimizer / GraphIndexManager
// ---------------------------------------------------------------------------

class LocalShardGraphExecutor final : public ShardGraphExecutor {
public:
    LocalShardGraphExecutor(std::string shard_id, GraphIndexManager& graph_mgr);

    LocalShardGraphExecutor(LocalShardGraphExecutor&&) noexcept = default;

    LocalShardGraphExecutor& operator=(LocalShardGraphExecutor&&) noexcept = default;

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
    std::string shard_id_ = {};
    GraphQueryOptimizer optimizer_;

    /**
     * @brief Qualify.
     * @param[in] vertex_id Identifier of the vertex.
     * @return Return value.
     */
    std::string qualify(const std::string& vertex_id) const;
};

// ---------------------------------------------------------------------------
// DistributedGraphManager
// ---------------------------------------------------------------------------

class DistributedGraphManager {
public:
    explicit DistributedGraphManager(const DistributedGraphConfig& config = {});

    // -----------------------------------------------------------------------
    // Shard registry
    // -----------------------------------------------------------------------

    /**
     * @brief Add Shard.
     * @param[in] shard_id Identifier of the shard.
     * @param[in] executor Input parameter.
     */
    void addShard(const std::string& shard_id,
                  std::shared_ptr<ShardGraphExecutor> executor);

    /**
     * @brief Remove Shard.
     * @param[in] shard_id Identifier of the shard.
     */
    void removeShard(const std::string& shard_id);

    /**
     * @brief Shard Ids.
     * @return Return value.
     */
    std::vector<std::string> shardIds() const;

    /**
     * @brief Shard Count.
     * @return Return value.
     */
    size_t shardCount() const;

    // -----------------------------------------------------------------------
    // Distributed query API
    // -----------------------------------------------------------------------

    Result<GraphIndexManager::PathResult> shortestPath(
        std::string_view start_vertex,
        std::string_view target_vertex,
        const GraphQueryOptimizer::QueryConstraints& constraints = {});

    Result<std::vector<std::string>> kHopNeighbors(
        std::string_view start_vertex,
        int k,
        const GraphQueryOptimizer::QueryConstraints& constraints = {});

    Result<GraphQueryOptimizer::OptimizationPlan> optimizePlan(
        std::string_view start_vertex,
        std::string_view target_vertex,
        GraphQueryOptimizer::QueryPattern pattern,
        const GraphQueryOptimizer::QueryConstraints& constraints = {});

    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    static std::pair<std::string, std::string> parseVertexId(
        std::string_view qualified_id);

    /**
     * @brief Resolve Shard For Vertex.
     * @param[in] local_vertex_id Identifier of the local vertex.
     * @return Return value.
     */
    std::string resolveShardForVertex(const std::string& local_vertex_id) const;

private:
    DistributedGraphConfig config_;

    // Registered shards: shard_id -> executor
    std::unordered_map<std::string, std::shared_ptr<ShardGraphExecutor>> shards_;
    mutable std::shared_mutex shards_mutex_;

    std::vector<std::pair<std::string, std::shared_ptr<ShardGraphExecutor>>>
    healthyShards() const;

    /**
     * @brief Effective Parallelism.
     * @param[in] num_shards Input parameter.
     * @return Return value.
     */
    size_t effectiveParallelism(size_t num_shards) const;
};

} // namespace graph
} // namespace themis

