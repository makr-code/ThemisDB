#include "sharding/rebalance_strategy.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace sharding {

// ============================================================================
// LoadBalancingStrategy Implementation
// ============================================================================

std::vector<LoadImbalanceResult::RebalanceRecommendation> 
LoadBalancingStrategy::generatePlan(
    const std::vector<ShardLoad>& shard_loads,
    const ShardTopology& topology) const {
    
    THEMIS_INFO("LoadBalancingStrategy: Generating rebalance plan");
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> recommendations;
    
    if (shard_loads.empty()) {
        return recommendations;
    }
    
    // Calculate mean CPU usage
    double total_cpu = 0.0;
    for (const auto& load : shard_loads) {
        total_cpu += load.cpu_usage;
    }
    double mean_cpu = total_cpu / shard_loads.size();
    
    // Find overloaded and underloaded shards
    std::vector<const ShardLoad*> overloaded;
    std::vector<const ShardLoad*> underloaded;
    
    for (const auto& load : shard_loads) {
        if (load.cpu_usage > mean_cpu * 1.2) {  // 20% above mean
            overloaded.push_back(&load);
        } else if (load.cpu_usage < mean_cpu * 0.8) {  // 20% below mean
            underloaded.push_back(&load);
        }
    }
    
    // Generate recommendations to balance load
    for (const auto* overloaded_shard : overloaded) {
        if (underloaded.empty()) break;
        
        LoadImbalanceResult::RebalanceRecommendation rec;
        rec.source_shard = overloaded_shard->shard_id;
        rec.target_shard = underloaded[0]->shard_id;
        rec.reason = "Balance CPU load";
        rec.estimated_improvement = (overloaded_shard->cpu_usage - mean_cpu) / 2.0;
        
        // Assign token range (simplified - half of source's range)
        rec.token_range_start = 0;
        rec.token_range_end = 500000;
        
        recommendations.push_back(rec);
    }
    
    THEMIS_INFO("LoadBalancingStrategy: Generated {} recommendations", recommendations.size());
    
    return recommendations;
}

RebalanceImpact LoadBalancingStrategy::estimateImpact(
    const LoadImbalanceResult::RebalanceRecommendation& recommendation,
    const std::vector<ShardLoad>& shard_loads) const {
    
    RebalanceImpact impact;
    
    // Find source and target loads
    const ShardLoad* source = nullptr;
    const ShardLoad* target = nullptr;
    
    for (const auto& load : shard_loads) {
        if (load.shard_id == recommendation.source_shard) {
            source = &load;
        }
        if (load.shard_id == recommendation.target_shard) {
            target = &load;
        }
    }
    
    if (!source || !target) {
        impact.risk_level = 1.0;  // High risk if shards not found
        return impact;
    }
    
    // Calculate current variance
    double current_variance = calculateLoadVariance(shard_loads);
    
    // Estimate new variance after rebalancing
    // Simplified: assume moving 50% of load from source to target
    double load_to_move = source->cpu_usage * 0.5;
    double new_source_cpu = source->cpu_usage - load_to_move;
    double new_target_cpu = target->cpu_usage + load_to_move;
    
    // Calculate improvement
    impact.estimated_cost_reduction = current_variance * 0.2;  // 20% improvement estimate
    
    // Estimate bytes moved (simplified)
    impact.estimated_bytes_moved = source->storage_bytes / 2;
    
    // Estimate duration (1MB per second)
    uint64_t mb_to_move = impact.estimated_bytes_moved / (1024 * 1024);
    impact.estimated_duration = std::chrono::milliseconds(mb_to_move * 1000);
    
    // Affected shards
    impact.affected_shards = {recommendation.source_shard, recommendation.target_shard};
    
    // Calculate risk level based on load difference
    double load_difference = std::abs(source->cpu_usage - target->cpu_usage);
    impact.risk_level = std::min(1.0, load_difference);  // Already 0-1 range
    
    return impact;
}

double LoadBalancingStrategy::calculateLoadVariance(const std::vector<ShardLoad>& loads) const {
    if (loads.empty()) return 0.0;
    
    double mean = 0.0;
    for (const auto& load : loads) {
        mean += load.cpu_usage;
    }
    mean /= loads.size();
    
    double variance = 0.0;
    for (const auto& load : loads) {
        double diff = load.cpu_usage - mean;
        variance += diff * diff;
    }
    variance /= loads.size();
    
    return std::sqrt(variance);
}

// ============================================================================
// CapacityPlanningStrategy Implementation
// ============================================================================

CapacityPlanningStrategy::CapacityPlanningStrategy(const Config& config)
    : config_(config) {
    THEMIS_INFO("CapacityPlanningStrategy initialized with max_cpu={}, max_memory={}",
                config_.max_cpu_threshold, config_.max_memory_threshold);
}

std::vector<LoadImbalanceResult::RebalanceRecommendation>
CapacityPlanningStrategy::generatePlan(
    const std::vector<ShardLoad>& shard_loads,
    const ShardTopology& topology) const {
    
    THEMIS_INFO("CapacityPlanningStrategy: Generating rebalance plan");
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> recommendations;
    
    // Find shards over capacity
    std::vector<const ShardLoad*> over_capacity;
    std::vector<const ShardLoad*> available;
    
    for (const auto& load : shard_loads) {
        if (isOverCapacity(load)) {
            over_capacity.push_back(&load);
        } else {
            available.push_back(&load);
        }
    }
    
    // Generate recommendations to offload over-capacity shards
    for (const auto* overloaded : over_capacity) {
        if (available.empty()) {
            THEMIS_WARN("No available shards to offload to");
            break;
        }
        
        // Find shard with most available capacity
        const ShardLoad* best_target = available[0];
        double best_capacity = calculateCapacityUtilization(*best_target);
        
        for (const auto* candidate : available) {
            double capacity = calculateCapacityUtilization(*candidate);
            if (capacity < best_capacity) {
                best_capacity = capacity;
                best_target = candidate;
            }
        }
        
        LoadImbalanceResult::RebalanceRecommendation rec;
        rec.source_shard = overloaded->shard_id;
        rec.target_shard = best_target->shard_id;
        rec.reason = "Prevent capacity overload";
        rec.estimated_improvement = calculateCapacityUtilization(*overloaded) - config_.max_cpu_threshold;
        
        // Assign token range
        rec.token_range_start = 0;
        rec.token_range_end = 500000;
        
        recommendations.push_back(rec);
    }
    
    THEMIS_INFO("CapacityPlanningStrategy: Generated {} recommendations", recommendations.size());
    
    return recommendations;
}

RebalanceImpact CapacityPlanningStrategy::estimateImpact(
    const LoadImbalanceResult::RebalanceRecommendation& recommendation,
    const std::vector<ShardLoad>& shard_loads) const {
    
    RebalanceImpact impact;
    
    // Find source load
    const ShardLoad* source = nullptr;
    for (const auto& load : shard_loads) {
        if (load.shard_id == recommendation.source_shard) {
            source = &load;
            break;
        }
    }
    
    if (!source) {
        impact.risk_level = 1.0;
        return impact;
    }
    
    // Capacity relief is the primary benefit
    double current_capacity = calculateCapacityUtilization(*source);
    impact.estimated_cost_reduction = (current_capacity - config_.max_cpu_threshold) * 100.0;
    
    // Estimate data movement
    impact.estimated_bytes_moved = source->storage_bytes / 2;
    
    // Duration estimate
    uint64_t mb_to_move = impact.estimated_bytes_moved / (1024 * 1024);
    impact.estimated_duration = std::chrono::milliseconds(mb_to_move * 1000);
    
    // Affected shards
    impact.affected_shards = {recommendation.source_shard, recommendation.target_shard};
    
    // Risk is higher if source is very overloaded (needs urgent action)
    impact.risk_level = current_capacity > 0.95 ? 0.8 : 0.5;
    
    return impact;
}

bool CapacityPlanningStrategy::isOverCapacity(const ShardLoad& load) const {
    return load.cpu_usage > config_.max_cpu_threshold ||
           load.memory_usage > config_.max_memory_threshold ||
           load.storage_bytes > config_.max_storage_bytes;
}

double CapacityPlanningStrategy::calculateCapacityUtilization(const ShardLoad& load) const {
    // Return maximum utilization across resources
    double cpu_util = load.cpu_usage;
    double mem_util = load.memory_usage;
    double storage_util = static_cast<double>(load.storage_bytes) / config_.max_storage_bytes;
    
    return std::max({cpu_util, mem_util, storage_util});
}

// ============================================================================
// CostOptimizationStrategy Implementation
// ============================================================================

std::vector<LoadImbalanceResult::RebalanceRecommendation>
CostOptimizationStrategy::generatePlan(
    const std::vector<ShardLoad>& shard_loads,
    const ShardTopology& topology) const {
    
    THEMIS_INFO("CostOptimizationStrategy: Generating rebalance plan");
    
    std::vector<LoadImbalanceResult::RebalanceRecommendation> recommendations;
    
    // Cost optimization focuses on minimal data movement
    // Generate only critical rebalancing operations
    
    if (shard_loads.size() < 2) {
        return recommendations;
    }
    
    // Find most and least loaded shards
    const ShardLoad* most_loaded = &shard_loads[0];
    const ShardLoad* least_loaded = &shard_loads[0];
    
    for (const auto& load : shard_loads) {
        if (load.cpu_usage > most_loaded->cpu_usage) {
            most_loaded = &load;
        }
        if (load.cpu_usage < least_loaded->cpu_usage) {
            least_loaded = &load;
        }
    }
    
    // Only recommend rebalance if imbalance is significant
    // Handle zero CPU case explicitly
    double imbalance_ratio;
    if (least_loaded->cpu_usage < 1e-6) {
        // If least loaded is effectively zero, consider it highly imbalanced
        imbalance_ratio = 10.0;  // High imbalance
    } else {
        imbalance_ratio = most_loaded->cpu_usage / least_loaded->cpu_usage;
    }
    
    if (imbalance_ratio > 2.0) {  // More than 2x difference
        LoadImbalanceResult::RebalanceRecommendation rec;
        rec.source_shard = most_loaded->shard_id;
        rec.target_shard = least_loaded->shard_id;
        rec.reason = "Critical imbalance with minimal cost";
        rec.estimated_improvement = (most_loaded->cpu_usage - least_loaded->cpu_usage) / 2.0;
        
        // Move smallest possible range (25% of source)
        rec.token_range_start = 0;
        rec.token_range_end = 250000;
        
        recommendations.push_back(rec);
    }
    
    THEMIS_INFO("CostOptimizationStrategy: Generated {} recommendations", recommendations.size());
    
    return recommendations;
}

RebalanceImpact CostOptimizationStrategy::estimateImpact(
    const LoadImbalanceResult::RebalanceRecommendation& recommendation,
    const std::vector<ShardLoad>& shard_loads) const {
    
    RebalanceImpact impact;
    
    // Find source load
    const ShardLoad* source = nullptr;
    for (const auto& load : shard_loads) {
        if (load.shard_id == recommendation.source_shard) {
            source = &load;
            break;
        }
    }
    
    if (!source) {
        impact.risk_level = 1.0;
        return impact;
    }
    
    // Cost optimization: minimize bytes moved
    impact.estimated_bytes_moved = source->storage_bytes / 4;  // Move only 25%
    
    // Calculate movement cost
    double cost = calculateMovementCost(recommendation, ShardTopology());
    impact.estimated_cost_reduction = cost * 0.1;  // 10% cost improvement
    
    // Duration estimate (faster due to less data)
    uint64_t mb_to_move = impact.estimated_bytes_moved / (1024 * 1024);
    impact.estimated_duration = std::chrono::milliseconds(mb_to_move * 1000);
    
    // Affected shards
    impact.affected_shards = {recommendation.source_shard, recommendation.target_shard};
    
    // Lower risk due to minimal data movement
    impact.risk_level = 0.2;
    
    return impact;
}

double CostOptimizationStrategy::calculateMovementCost(
    const LoadImbalanceResult::RebalanceRecommendation& rec,
    const ShardTopology& topology) const {
    
    // Cost factors:
    // 1. Network bandwidth cost
    // 2. Storage I/O cost
    // 3. CPU overhead for data transfer
    
    // Simplified: assume base cost per GB
    double cost_per_gb = 0.01;  // $0.01 per GB
    
    // Placeholder: assume 1GB movement
    double gb_to_move = 1.0;
    
    return gb_to_move * cost_per_gb;
}

} // namespace sharding
} // namespace themis
