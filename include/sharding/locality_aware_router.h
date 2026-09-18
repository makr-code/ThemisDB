/**
 * @file locality_aware_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/shard_topology.h"
#include "sharding/shard_resource_manager.h"
#include "sharding/urn.h"
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include <map>
#include <set>

namespace themis::sharding {

/** @brief Locality aware router component. */
class LocalityAwareRouter {
public:
    struct QuerySpec {
        std::string query_id;
        std::string query_aql;
        std::vector<std::string> accessed_collections;
        std::vector<std::string> accessed_keys;  // URNs or key patterns
        size_t estimated_result_size_bytes = 0;
        bool allow_cross_shard = true;  // Allow routing to remote shards
    };
    
    struct ShardAffinity {
        std::string shard_id;
        float locality_score;     // 0.0-1.0 (1.0 = all data local)
        float load_score;         // 0.0-1.0 (0.0 = no load, 1.0 = max load)
        float network_score;      // 0.0-1.0 (1.0 = same datacenter)
        float combined_score;     // Weighted sum
        
        /**
         * @brief TBD: Describe toJson.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    struct Config {
        // Scoring weights (must sum to 1.0)
        float locality_weight = 0.50f;      // 50% locality
        float load_weight = 0.30f;          // 30% load
        float network_weight = 0.20f;       // 20% network distance
        
        // Routing behavior
        bool enable_cross_shard_optimization = true;
        uint32_t max_cross_shard_hops = 2;
        bool prefer_local_shard = true;
        float local_shard_bonus = 0.2f;     // 20% bonus for local execution
        
        // Data placement tracking
        bool enable_placement_cache = true;
        size_t max_cache_entries = 100000;  // 100k entries
        uint32_t cache_ttl_seconds = 300;    // 5 minutes
        bool use_bloom_filter = true;       // Space-efficient tracking
    };
    
    struct Statistics {
        std::atomic<uint64_t> queries_routed{0};
        std::atomic<uint64_t> local_routes{0};
        std::atomic<uint64_t> remote_routes{0};
        std::atomic<uint64_t> cross_shard_avoided{0};
        std::atomic<double> avg_locality_score{0.0};
        std::atomic<double> avg_combined_score{0.0};

        Statistics() = default;
        Statistics(const Statistics& other) {
            queries_routed.store(other.queries_routed.load());
            local_routes.store(other.local_routes.load());
            remote_routes.store(other.remote_routes.load());
            cross_shard_avoided.store(other.cross_shard_avoided.load());
            avg_locality_score.store(other.avg_locality_score.load());
            avg_combined_score.store(other.avg_combined_score.load());
        }
        Statistics& operator=(const Statistics& other) {
            if (this != &other) {
                queries_routed.store(other.queries_routed.load());
                local_routes.store(other.local_routes.load());
                remote_routes.store(other.remote_routes.load());
                cross_shard_avoided.store(other.cross_shard_avoided.load());
                avg_locality_score.store(other.avg_locality_score.load());
                avg_combined_score.store(other.avg_combined_score.load());
            }
            return *this;
        }
    };
    
    /**
     * @brief TBD: Describe LocalityAwareRouter.
     * @param[in] local_shard_id Input parameter.
     * @param[in] topology Input parameter.
     * @param[in] resource_mgr Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LocalityAwareRouter(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardResourceManager> resource_mgr,
        const Config& config
    );

    /**
     * @brief TBD: Describe LocalityAwareRouter.
     * @param[in] local_shard_id Input parameter.
     * @param[in] topology Input parameter.
     * @param[in] resource_mgr Input parameter.
     * @return Return value.
     */
    explicit LocalityAwareRouter(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardResourceManager> resource_mgr
    );
    
    ~LocalityAwareRouter();
    
    /**
     * @brief Main routing interface
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    std::string routeQuery(const QuerySpec& spec);
    /**
     * @brief TBD: Describe routeMultiShardQuery.
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    std::vector<std::string> routeMultiShardQuery(const QuerySpec& spec);
    
    /**
     * @brief Affinity calculation
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    std::vector<ShardAffinity> computeAffinity(const QuerySpec& spec);
    /**
     * @brief TBD: Describe computeShardAffinity.
     * @param[in] shard_id Input parameter.
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    ShardAffinity computeShardAffinity(const std::string& shard_id, 
                                        const QuerySpec& spec);
    
    /**
     * @brief Data placement tracking
     * @param[in] collection Input parameter.
     * @param[in] key Input parameter.
     * @param[in] shard_id Input parameter.
     */
    void updateDataPlacement(const std::string& collection,
                             const std::string& key,
                             const std::string& shard_id);
    /**
     * @brief TBD: Describe removeDataPlacement.
     * @param[in] collection Input parameter.
     * @param[in] key Input parameter.
     */
    void removeDataPlacement(const std::string& collection,
                             const std::string& key);
    /**
     * @brief TBD: Describe hasData.
     * @param[in] shard_id Input parameter.
     * @param[in] collection Input parameter.
     * @param[in] key Input parameter.
     * @return True on success.
     */
    bool hasData(const std::string& shard_id,
                 const std::string& collection,
                 const std::string& key) const;
    
    /**
     * @brief Optimization hints
     * @param[in] collections Input parameter.
     * @return Return value.
     */
    std::vector<std::string> suggestCoLocation(
        const std::vector<std::string>& collections);
    
    /**
     * @brief Statistics
     * @return Return value.
     */
    Statistics getStatistics() const;
    /**
     * @brief TBD: Describe getStatisticsJson.
     * @return Return value.
     */
    nlohmann::json getStatisticsJson() const;
    
    /**
     * @brief Record latency (RTT) measurement for a replica in a specific DC.
     *
     * Updates the RTT tracking for cross-datacenter routing decisions.
     * Used by latency-aware routing to select the lowest-RTT replica.
     *
     * @param replica_id Replica node identifier.
     * @param datacenter_id Requesting datacenter ID.
     * @param rtt_ms Measured round-trip time in milliseconds.
     */
    void recordReplicaLatency(const std::string& replica_id,
                             const std::string& datacenter_id,
                             uint64_t rtt_ms);

    /**
     * @brief Select lowest-RTT replica for cross-datacenter read routing.
     *
     * For a given shard and requesting datacenter, returns the replica ID
     * with the lowest measured RTT, or falls back to nearest replica on timeout.
     *
     * @param shard_id Shard to route to.
     * @param requesting_datacenter_id Datacenter making the request.
     * @param timeout_ms Fallback timeout; if all replicas exceeded timeout, use nearest.
     * @return Replica ID with lowest RTT, or primary if all timed out.
     */
    std::string selectLowestRTTReplica(const std::string& shard_id,
                                     const std::string& requesting_datacenter_id,
                                     uint64_t timeout_ms = 1000);

    /**
     * @brief Compute deterministic multi-shard exact consistency under failure.
     *
     * Returns routing decisions for multi-shard queries that guarantee exact
     * consistency semantics even when some shards fail. Uses quorum-based
     * validation and deterministic fallback ordering.
     *
     * @param shard_ids Target shards for the query.
     * @param consistency_level Required consistency (e.g., "strong", "eventual").
     * @return Vector of shard IDs ordered by routing priority for exact consistency.
     */
    std::vector<std::string> computeMultiShardExactConsistency(
        const std::vector<std::string>& shard_ids,
        const std::string& consistency_level = "strong");

private:
    std::string local_shard_id_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ShardResourceManager> resource_mgr_;
    Config config_;
    
    // Data placement cache (collection: key -> set<shard_id>)
    std::map<std::string, std::set<std::string>> placement_cache_;
    mutable std::shared_mutex cache_mutex_;
    
    // Latency tracking: replica_id -> (datacenter_id -> rtt_ms, timestamp)
    struct LatencyRecord {
        uint64_t rtt_ms = 0;
        std::chrono::system_clock::time_point last_update;
    };
    std::map<std::string, std::map<std::string, LatencyRecord>> latency_records_;
    mutable std::shared_mutex latency_mutex_;
    
    // Bloom filter for space-efficient tracking (optional)
    // std::unique_ptr<BloomFilter> bloom_filter_;
    
    // Statistics
    Statistics stats_;
    
    /**
     * @brief Scoring helpers
     * @param[in] shard_id Input parameter.
     * @param[in] spec Input parameter.
     * @return Return value.
     */
    float calculateLocalityScore(const std::string& shard_id,
                                 const QuerySpec& spec) const;
    /**
     * @brief TBD: Describe calculateLoadScore.
     * @param[in] shard_id Input parameter.
     * @return Return value.
     */
    float calculateLoadScore(const std::string& shard_id) const;
    /**
     * @brief TBD: Describe calculateNetworkScore.
     * @param[in] shard_id Input parameter.
     * @return Return value.
     */
    float calculateNetworkScore(const std::string& shard_id) const;
    
    /**
     * @brief Latency helpers
     * @param[in] replica_id Input parameter.
     * @param[in] datacenter_id Input parameter.
     * @return Return value.
     */
    uint64_t getReplicaLatency(const std::string& replica_id,
                             const std::string& datacenter_id) const;
    /**
     * @brief TBD: Describe isLatencyStale.
     * @param[in] replica_id Input parameter.
     * @param[in] datacenter_id Input parameter.
     * @param[in] max_age_ms Input parameter.
     * @return True on success.
     */
    bool isLatencyStale(const std::string& replica_id,
                       const std::string& datacenter_id,
                       uint64_t max_age_ms) const;
    
    /**
     * @brief Cache helpers
     * @param[in] collection Input parameter.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string makeCacheKey(const std::string& collection,
                             const std::string& key) const;
    /**
     * @brief TBD: Describe cleanupStaleEntries.
     */
    void cleanupStaleEntries();
};

} // namespace themis::sharding
