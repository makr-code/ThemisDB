#pragma once

#include "sharding/shard_load_detector.h"
#include "sharding/shard_topology.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace themis {
namespace sharding {

/**
 * Impact assessment for a rebalance operation
 */
struct RebalanceImpact {
    double estimated_cost_reduction;    // Expected improvement in load balance
    uint64_t estimated_bytes_moved;     // Data transfer size
    std::chrono::milliseconds estimated_duration;  // Expected completion time
    std::vector<std::string> affected_shards;      // Shards involved
    double risk_level;                  // 0.0 (low) to 1.0 (high)
};

/**
 * RebalanceStrategy
 * 
 * Base class for rebalance planning strategies.
 * Implementations generate rebalance plans based on different optimization goals.
 */
class RebalanceStrategy {
public:
    virtual ~RebalanceStrategy() = default;
    
    /**
     * Generate rebalance plan based on current load
     * @param shard_loads Current shard load metrics
     * @param topology Shard topology
     * @return Vector of rebalance recommendations
     */
    virtual std::vector<LoadImbalanceResult::RebalanceRecommendation> generatePlan(
        const std::vector<ShardLoad>& shard_loads,
        const ShardTopology& topology
    ) const = 0;
    
    /**
     * Estimate impact of operation
     * @param recommendation Rebalance recommendation
     * @param shard_loads Current shard loads
     * @return Impact assessment
     */
    virtual RebalanceImpact estimateImpact(
        const LoadImbalanceResult::RebalanceRecommendation& recommendation,
        const std::vector<ShardLoad>& shard_loads
    ) const = 0;
};

/**
 * LoadBalancingStrategy
 * 
 * Minimizes CPU/memory/network variance across shards.
 * Goal: Even distribution of load
 */
class LoadBalancingStrategy : public RebalanceStrategy {
public:
    LoadBalancingStrategy() = default;
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> generatePlan(
        const std::vector<ShardLoad>& shard_loads,
        const ShardTopology& topology
    ) const override;
    
    RebalanceImpact estimateImpact(
        const LoadImbalanceResult::RebalanceRecommendation& recommendation,
        const std::vector<ShardLoad>& shard_loads
    ) const override;
    
private:
    double calculateLoadVariance(const std::vector<ShardLoad>& loads) const;
};

/**
 * CapacityPlanningStrategy
 * 
 * Ensures no shard exceeds capacity thresholds.
 * Goal: Prevent capacity issues
 */
class CapacityPlanningStrategy : public RebalanceStrategy {
public:
    struct Config {
        double max_cpu_threshold{0.8};      // 80%
        double max_memory_threshold{0.85};  // 85%
        uint64_t max_storage_bytes{1ULL << 40};  // 1TB
    };
    
    explicit CapacityPlanningStrategy(const Config& config = Config{});
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> generatePlan(
        const std::vector<ShardLoad>& shard_loads,
        const ShardTopology& topology
    ) const override;
    
    RebalanceImpact estimateImpact(
        const LoadImbalanceResult::RebalanceRecommendation& recommendation,
        const std::vector<ShardLoad>& shard_loads
    ) const override;
    
private:
    Config config_;
    
    bool isOverCapacity(const ShardLoad& load) const;
    double calculateCapacityUtilization(const ShardLoad& load) const;
};

/**
 * CostOptimizationStrategy
 * 
 * Minimizes data movement and network cost.
 * Goal: Lowest operational cost
 */
class CostOptimizationStrategy : public RebalanceStrategy {
public:
    CostOptimizationStrategy() = default;
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> generatePlan(
        const std::vector<ShardLoad>& shard_loads,
        const ShardTopology& topology
    ) const override;
    
    RebalanceImpact estimateImpact(
        const LoadImbalanceResult::RebalanceRecommendation& recommendation,
        const std::vector<ShardLoad>& shard_loads
    ) const override;
    
private:
    double calculateMovementCost(
        const LoadImbalanceResult::RebalanceRecommendation& rec,
        const ShardTopology& topology
    ) const;
};

} // namespace sharding
} // namespace themis
