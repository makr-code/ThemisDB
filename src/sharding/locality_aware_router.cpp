/**
 * @file locality_aware_router.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/locality_aware_router.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <climits>

namespace themis::sharding {

// Load score calculation constants
namespace {
    constexpr float LOAD_CPU_WEIGHT = 0.4f;
    constexpr float LOAD_RAM_WEIGHT = 0.3f;
    constexpr float LOAD_HEALTH_WEIGHT = 0.3f;
    constexpr float CACHE_CLEANUP_PERCENT = 0.10f;
}

// ShardAffinity JSON serialization
nlohmann::json LocalityAwareRouter::ShardAffinity::toJson() const {
    return nlohmann::json{
        {"shard_id", shard_id},
        {"locality_score", locality_score},
        {"load_score", load_score},
        {"network_score", network_score},
        {"combined_score", combined_score}
    };
}

// Constructor
LocalityAwareRouter::LocalityAwareRouter(
    const std::string& local_shard_id,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardResourceManager> resource_mgr,
    const Config& config)
    : local_shard_id_(local_shard_id),
      topology_(topology),
      resource_mgr_(resource_mgr),
      config_(config) {
    
    // Validate that weights sum to approximately 1.0
    float weight_sum = config_.locality_weight + config_.load_weight + config_.network_weight;
    if (std::abs(weight_sum - 1.0f) > 0.01f) {
        // Auto-normalize weights if they don't sum to 1.0
        config_.locality_weight /= weight_sum;
        config_.load_weight /= weight_sum;
        config_.network_weight /= weight_sum;
    }
}

LocalityAwareRouter::LocalityAwareRouter(
    const std::string& local_shard_id,
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<ShardResourceManager> resource_mgr)
    : LocalityAwareRouter(local_shard_id, topology, resource_mgr, Config{})
{
}

// Destructor
LocalityAwareRouter::~LocalityAwareRouter() = default;

// Main routing interface
/**
 * @brief Route Query.
 * @param[in] spec Input parameter.
 * @return Return value.
 * @details Calls: fetch_add(), computeAffinity(), empty(), std::sort(), begin(), end(), store(), size().
 */
std::string LocalityAwareRouter::routeQuery(const QuerySpec& spec) {
    stats_.queries_routed.fetch_add(1, std::memory_order_relaxed);
    
    // Compute affinity for all shards
    auto affinities = computeAffinity(spec);
    
    if (affinities.empty()) {
        // Fallback to local shard if no affinity data
        stats_.local_routes.fetch_add(1, std::memory_order_relaxed);
        return local_shard_id_;
    }
    
    // Sort by combined score (highest first)
    std::sort(affinities.begin(), affinities.end(),
              [](const ShardAffinity& a, const ShardAffinity& b) {
                  return a.combined_score > b.combined_score;
              });
    
    // Update statistics
    double total_locality = 0.0;
    double total_combined = 0.0;
    for (const auto& affinity : affinities) {
        total_locality += affinity.locality_score;
        total_combined += affinity.combined_score;
    }
    
    if (!affinities.empty()) {
        stats_.avg_locality_score.store(
            total_locality / affinities.size(),
            std::memory_order_relaxed
        );
        stats_.avg_combined_score.store(
            total_combined / affinities.size(),
            std::memory_order_relaxed
        );
    }
    
    // Select best shard
    std::string target_shard = affinities[0].shard_id;
    
    if (target_shard == local_shard_id_) {
        stats_.local_routes.fetch_add(1, std::memory_order_relaxed);
    } else {
        stats_.remote_routes.fetch_add(1, std::memory_order_relaxed);
    }
    
    return target_shard;
}

/**
 * @brief Route Multi Shard Query.
 * @param[in] spec Input parameter.
 * @return Return value.
 * @details Calls: fetch_add(), computeAffinity(), empty(), push_back(), std::sort(), begin(), end().
 */
std::vector<std::string> LocalityAwareRouter::routeMultiShardQuery(const QuerySpec& spec) {
    stats_.queries_routed.fetch_add(1, std::memory_order_relaxed);
    
    // Compute affinity for all shards
    auto affinities = computeAffinity(spec);
    
    std::vector<std::string> result;
    
    if (affinities.empty()) {
        // Return local shard as default
        result.push_back(local_shard_id_);
        return result;
    }
    
    // Sort by combined score (highest first)
    std::sort(affinities.begin(), affinities.end(),
              [](const ShardAffinity& a, const ShardAffinity& b) {
                  return a.combined_score > b.combined_score;
              });
    
    // Return shards with positive locality scores
    for (const auto& affinity : affinities) {
        if (affinity.locality_score > 0.0f) {
            result.push_back(affinity.shard_id);
        }
    }
    
    return result;
}

// Affinity calculation
std::vector<LocalityAwareRouter::ShardAffinity> 
LocalityAwareRouter::computeAffinity(const QuerySpec& spec) {
    std::vector<ShardAffinity> result;
    
    // Get all healthy shards
    auto shards = topology_->getHealthyShards();
    
    for (const auto& shard : shards) {
        auto affinity = computeShardAffinity(shard.shard_id, spec);
        result.push_back(affinity);
    }
    
    return result;
}

LocalityAwareRouter::ShardAffinity 
LocalityAwareRouter::computeShardAffinity(
    const std::string& shard_id,
    const QuerySpec& spec) {
    
    ShardAffinity affinity;
    affinity.shard_id = shard_id;
    
    // Calculate individual scores
    affinity.locality_score = calculateLocalityScore(shard_id, spec);
    affinity.load_score = calculateLoadScore(shard_id);
    affinity.network_score = calculateNetworkScore(shard_id);
    
    // Calculate combined score with weights
    // Load score is inverted: lower load is better
    affinity.combined_score = 
        (config_.locality_weight * affinity.locality_score) +
        (config_.load_weight * (1.0f - affinity.load_score)) +
        (config_.network_weight * affinity.network_score);
    
    // Apply local shard bonus
    if (config_.prefer_local_shard && shard_id == local_shard_id_) {
        affinity.combined_score += config_.local_shard_bonus;
    }
    
    // Clamp combined score to [0, 1]
    affinity.combined_score = std::min(1.0f, std::max(0.0f, affinity.combined_score));
    
    return affinity;
}

// Data placement tracking
/**
 * @brief Update Data Placement.
 * @param[in] collection Input parameter.
 * @param[in] key Input parameter.
 * @param[in] shard_id Identifier of the shard.
 * @details Calls: lock(), makeCacheKey(), insert(), size(), cleanupStaleEntries().
 */
void LocalityAwareRouter::updateDataPlacement(
    const std::string& collection,
    const std::string& key,
    const std::string& shard_id) {
    
    if (!config_.enable_placement_cache) {
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    std::string cache_key = makeCacheKey(collection, key);
    placement_cache_[cache_key].insert(shard_id);
    
    // Simple cache size management
    if (placement_cache_.size() > config_.max_cache_entries) {
        cleanupStaleEntries();
    }
}

/**
 * @brief Remove Data Placement.
 * @param[in] collection Input parameter.
 * @param[in] key Input parameter.
 * @details Calls: lock(), makeCacheKey(), erase().
 */
void LocalityAwareRouter::removeDataPlacement(
    const std::string& collection,
    const std::string& key) {
    
    if (!config_.enable_placement_cache) {
        return;
    }
    
    std::unique_lock<std::shared_mutex> lock(cache_mutex_);
    
    std::string cache_key = makeCacheKey(collection, key);
    placement_cache_.erase(cache_key);
}

bool LocalityAwareRouter::hasData(
    const std::string& shard_id,
    const std::string& collection,
    const std::string& key) const {
    
    if (!config_.enable_placement_cache) {
        return false;
    }
    
    /**
     * @brief Lock.
     * @param[in] cache_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    std::string cache_key = makeCacheKey(collection, key);
    auto it = placement_cache_.find(cache_key);
    
    if (it == placement_cache_.end()) {
        return false;
    }
    
    return it->second.find(shard_id) != it->second.end();
}

// Optimization hints
/**
 * @brief Suggest Co Location.
 * @param[in] collections Input parameter.
 * @return Return value.
 * @details Calls: size(), push_back(), std::accumulate(), begin(), end().
 */
std::vector<std::string> LocalityAwareRouter::suggestCoLocation(
    const std::vector<std::string>& collections) {
    
    std::vector<std::string> suggestions;
    
    // Simple heuristic: suggest co-locating collections that are accessed together
    if (collections.size() > 1) {
        suggestions.push_back(
            "Consider co-locating collections: " + 
            std::accumulate(collections.begin() + 1, collections.end(), 
                          collections[0],
                          [](const std::string& a, const std::string& b) {
                              return a + ", " + b;
                          })
        );
    }
    
    return suggestions;
}

// Statistics
LocalityAwareRouter::Statistics LocalityAwareRouter::getStatistics() const {
    Statistics stats;
    stats.queries_routed.store(
        stats_.queries_routed.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    stats.local_routes.store(
        stats_.local_routes.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    stats.remote_routes.store(
        stats_.remote_routes.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    stats.cross_shard_avoided.store(
        stats_.cross_shard_avoided.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    stats.avg_locality_score.store(
        stats_.avg_locality_score.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    stats.avg_combined_score.store(
        stats_.avg_combined_score.load(std::memory_order_relaxed),
        std::memory_order_relaxed
    );
    return stats;
}

nlohmann::json LocalityAwareRouter::getStatisticsJson() const {
    auto stats = getStatistics();
    return nlohmann::json{
        {"queries_routed", stats.queries_routed.load(std::memory_order_relaxed)},
        {"local_routes", stats.local_routes.load(std::memory_order_relaxed)},
        {"remote_routes", stats.remote_routes.load(std::memory_order_relaxed)},
        {"cross_shard_avoided", stats.cross_shard_avoided.load(std::memory_order_relaxed)},
        {"avg_locality_score", stats.avg_locality_score.load(std::memory_order_relaxed)},
        {"avg_combined_score", stats.avg_combined_score.load(std::memory_order_relaxed)}
    };
}

// Private helper methods
float LocalityAwareRouter::calculateLocalityScore(
    const std::string& shard_id,
    const QuerySpec& spec) const {
    
    if (spec.accessed_keys.empty()) {
        // No keys specified, assume uniform distribution
        return 0.5f;
    }
    
    /**
     * @brief Lock.
     * @param[in] cache_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(cache_mutex_);
    
    size_t local_keys = 0;
    for (const auto& key : spec.accessed_keys) {
        // Check each collection for this key
        for (const auto& collection : spec.accessed_collections) {
            std::string cache_key = makeCacheKey(collection, key);
            auto it = placement_cache_.find(cache_key);
            
            if (it != placement_cache_.end() && 
                it->second.find(shard_id) != it->second.end()) {
                local_keys++;
                break; // Found in this collection, move to next key
            }
        }
    }
    
    return static_cast<float>(local_keys) /
           static_cast<float>(spec.accessed_keys.size());
}

// Helper function to calculate load score from resource snapshot
namespace {
    /**
     * @brief Compute Load From Snapshot.
     * @param[in] cpu_percent Input parameter.
     * @param[in] ram_used Input parameter.
     * @param[in] ram_total Input parameter.
     * @param[in] health_score Input parameter.
     * @return Return value.
     * @details Implements computeLoadFromSnapshot without additional internal calls.
     */
    float computeLoadFromSnapshot(float cpu_percent, uint64_t ram_used, 
                                   uint64_t ram_total, float health_score) {
        float cpu_score = cpu_percent / 100.0f;
        float ram_score = (ram_total > 0) ?
            static_cast<float>(ram_used) / ram_total : 0.0f;
        float health_penalty = 1.0f - (health_score / 100.0f);
        
        return (cpu_score * LOAD_CPU_WEIGHT + 
                ram_score * LOAD_RAM_WEIGHT + 
                health_penalty * LOAD_HEALTH_WEIGHT);
    }
}

float LocalityAwareRouter::calculateLoadScore(const std::string& shard_id) const {
    if (!resource_mgr_) {
        return 0.0f; // No load information available
    }
    
    // Get resource snapshot for the shard
    auto peer_snapshot = resource_mgr_->getPeerResource(shard_id);
    
    if (!peer_snapshot.has_value()) {
        // If it's the local shard, get current snapshot
        if (shard_id == local_shard_id_) {
            auto local_snapshot = resource_mgr_->getCurrentSnapshot();
            return computeLoadFromSnapshot(
                local_snapshot.cpu_usage_percent,
                local_snapshot.ram_usage_bytes,
                local_snapshot.ram_total_bytes,
                local_snapshot.health_score
            );
        }
        return 0.5f; // Unknown load, assume medium
    }
    
    // Calculate composite load score from peer resource snapshot
    return computeLoadFromSnapshot(
        peer_snapshot->cpu_usage_percent,
        peer_snapshot->ram_usage_bytes,
        peer_snapshot->ram_total_bytes,
        peer_snapshot->health_score
    );
}

float LocalityAwareRouter::calculateNetworkScore(const std::string& shard_id) const {
    // Get shard information
    auto shard_info = topology_->getShard(shard_id);
    auto local_info = topology_->getShard(local_shard_id_);
    
    if (!shard_info.has_value() || !local_info.has_value()) {
        return 0.5f; // Unknown location
    }
    
    // 3-tier scoring using region, zone, and datacenter:
    // Same zone (most local)           = 1.0
    // Same region, different zone      = 0.8
    // Same datacenter, different region = 0.5
    // Different datacenter/region      = 0.1
    if (!shard_info->zone.empty() && !local_info->zone.empty() &&
        shard_info->zone == local_info->zone) {
        return 1.0f;
    }
    if (!shard_info->region.empty() && !local_info->region.empty() &&
        shard_info->region == local_info->region) {
        return 0.8f;
    }
    if (shard_info->datacenter == local_info->datacenter) {
        return 0.5f;
    }
    return 0.1f;
}

std::string LocalityAwareRouter::makeCacheKey(
    const std::string& collection,
    const std::string& key) const {
    return collection + ":" + key;
}

/**
 * @brief Cleanup Stale Entries.
 * @details Calls: empty(), size(), begin(), end(), erase().
 */
void LocalityAwareRouter::cleanupStaleEntries() {
    // Simple strategy: remove oldest percentage of entries
    // In a production system, this would use TTL timestamps
    
    if (placement_cache_.empty()) {
        return;
    }
    
    size_t entries_to_remove = static_cast<size_t>(
        placement_cache_.size() * CACHE_CLEANUP_PERCENT
    );
    if (entries_to_remove == 0) {
        entries_to_remove = 1;
    }
    
    auto it = placement_cache_.begin();
    for (size_t i = 0; i < entries_to_remove && it != placement_cache_.end(); ++i) {
        it = placement_cache_.erase(it);
    }
}

/**
 * @brief Record Replica Latency.
 * @param[in] replica_id Identifier of the replica.
 * @param[in] datacenter_id Identifier of the datacenter.
 * @param[in] rtt_ms Input parameter.
 * @details Calls: lock(), std::chrono::system_clock::now().
 */
void LocalityAwareRouter::recordReplicaLatency(const std::string& replica_id,
                                               const std::string& datacenter_id,
                                               uint64_t rtt_ms) {
    std::unique_lock<std::shared_mutex> lock(latency_mutex_);
    
    auto& dc_map = latency_records_[replica_id];
    dc_map[datacenter_id] = LatencyRecord{
        rtt_ms,
        std::chrono::system_clock::now()
    };
}

uint64_t LocalityAwareRouter::getReplicaLatency(const std::string& replica_id,
                                                const std::string& datacenter_id) const {
    /**
     * @brief Lock.
     * @param[in] latency_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(latency_mutex_);
    
    auto replica_it = latency_records_.find(replica_id);
    if (replica_it == latency_records_.end()) {
        return UINT64_MAX;
    }
    
    auto dc_it = replica_it->second.find(datacenter_id);
    if (dc_it == replica_it->second.end()) {
        return UINT64_MAX;
    }
    
    return dc_it->second.rtt_ms;
}

bool LocalityAwareRouter::isLatencyStale(const std::string& replica_id,
                                         const std::string& datacenter_id,
                                         uint64_t max_age_ms) const {
    /**
     * @brief Lock.
     * @param[in] latency_mutex_ Input parameter.
     * @return Return value.
     */
    std::shared_lock<std::shared_mutex> lock(latency_mutex_);
    
    auto replica_it = latency_records_.find(replica_id);
    if (replica_it == latency_records_.end()) {
        return true;
    }
    
    auto dc_it = replica_it->second.find(datacenter_id);
    if (dc_it == replica_it->second.end()) {
        return true;
    }
    
    auto now = std::chrono::system_clock::now();
    auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - dc_it->second.last_update).count();
    
    return age_ms > static_cast<int64_t>(max_age_ms);
}

/**
 * @brief Select Lowest RTTReplica.
 * @param[in] shard_id Identifier of the shard.
 * @param[in] requesting_datacenter_id Identifier of the requesting datacenter.
 * @param[in] timeout_ms Input parameter.
 * @return Return value.
 * @details Calls: find(), end(), push_back(), empty(), hasShard(), std::find(), begin(), getShard().
 */
std::string LocalityAwareRouter::selectLowestRTTReplica(const std::string& shard_id,
                                                        const std::string& requesting_datacenter_id,
                                                        uint64_t timeout_ms) {
    // Get topology information for this shard
    if (!topology_) {
        return shard_id; // Fallback to shard ID if no topology
    }

    const auto collectCandidates = [this](const ShardInfo& info, bool include_self) {
        std::vector<std::string> result;

        if (include_self && latency_records_.find(info.shard_id) != latency_records_.end()) {
            result.push_back(info.shard_id);
        }

        for (const auto& replica_id : info.replica_endpoints) {
            if (replica_id.empty()) {
                continue;
            }
            const bool known_shard = topology_->hasShard(replica_id);
            const bool measured_replica = latency_records_.find(replica_id) != latency_records_.end();
            if ((known_shard || measured_replica) &&
                std::find(result.begin(), result.end(), replica_id) == result.end()) {
                result.push_back(replica_id);
            }
        }

        return result;
    };

    std::vector<std::string> candidates;
    if (const auto shard_info = topology_->getShard(shard_id); shard_info.has_value()) {
        candidates = collectCandidates(*shard_info, false);
    }

    if (candidates.empty()) {
        if (const auto local_info = topology_->getShard(local_shard_id_); local_info.has_value()) {
            candidates = collectCandidates(*local_info, true);
        }
    }

    if (candidates.empty()) {
        return shard_id;
    }

    uint64_t min_latency = UINT64_MAX;
    std::string best_replica = candidates.front();
    
    // Query latency records for all potential replicas
    {
        std::shared_lock<std::shared_mutex> lock(latency_mutex_);
        
        for (const auto& replica_id : candidates) {
            const auto replica_it = latency_records_.find(replica_id);
            if (replica_it == latency_records_.end()) {
                continue;
            }

            const auto& dc_map = replica_it->second;
            auto it = dc_map.find(requesting_datacenter_id);
            if (it != dc_map.end()) {
                uint64_t rtt = it->second.rtt_ms;
                if (rtt < min_latency && rtt <= timeout_ms) {
                    min_latency = rtt;
                    best_replica = replica_id;
                }
            }
        }
    }
    
    return best_replica;
}

/**
 * @brief Compute Multi Shard Exact Consistency.
 * @param[in] shard_ids Input parameter.
 * @param[in] consistency_level Input parameter.
 * @return Return value.
 * @details Calls: empty(), size(), std::sort(), begin(), end(), calculateLoadScore(), result().
 */
std::vector<std::string> LocalityAwareRouter::computeMultiShardExactConsistency(
    const std::vector<std::string>& shard_ids,
    const std::string& consistency_level) {
    
    if (shard_ids.empty()) {
        return {};
    }
    
    // For strong consistency, we need quorum (majority of shards to respond)
    if (consistency_level == "strong") {
        size_t quorum_size = (shard_ids.size() / 2) + 1;
        
        // Sort shards by expected health/availability
        std::vector<std::string> sorted_shards = shard_ids;
        std::sort(sorted_shards.begin(), sorted_shards.end(),
                  [this](const std::string& a, const std::string& b) {
                      // Prioritize local shard
                      if (a == local_shard_id_) {
                        return true;
                      }
                      if (b == local_shard_id_) {
                        return false;
                      }
                      
                      // Then prioritize by load score
                      float score_a = calculateLoadScore(a);
                      float score_b = calculateLoadScore(b);
                      return score_a < score_b; // Lower load is better
                  });
        
        // Return quorum-size ordered shards for exact consistency
        std::vector<std::string> result(sorted_shards.begin(),
                                        sorted_shards.begin() + quorum_size);
        return result;
    }
    
    // For eventual consistency, return all shards
    return shard_ids;
}

} // namespace themis::sharding
