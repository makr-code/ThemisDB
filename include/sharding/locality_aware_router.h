/**
 * @file locality_aware_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    
    explicit LocalityAwareRouter(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardResourceManager> resource_mgr,
        const Config& config
    );

    explicit LocalityAwareRouter(
        const std::string& local_shard_id,
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<ShardResourceManager> resource_mgr
    );
    
    ~LocalityAwareRouter();
    
    // Main routing interface
    std::string routeQuery(const QuerySpec& spec);
    std::vector<std::string> routeMultiShardQuery(const QuerySpec& spec);
    
    // Affinity calculation
    std::vector<ShardAffinity> computeAffinity(const QuerySpec& spec);
    ShardAffinity computeShardAffinity(const std::string& shard_id, 
                                        const QuerySpec& spec);
    
    // Data placement tracking
    void updateDataPlacement(const std::string& collection,
                             const std::string& key,
                             const std::string& shard_id);
    void removeDataPlacement(const std::string& collection,
                             const std::string& key);
    bool hasData(const std::string& shard_id,
                 const std::string& collection,
                 const std::string& key) const;
    
    // Optimization hints
    std::vector<std::string> suggestCoLocation(
        const std::vector<std::string>& collections);
    
    // Statistics
    Statistics getStatistics() const;
    nlohmann::json getStatisticsJson() const;
    
private:
    std::string local_shard_id_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ShardResourceManager> resource_mgr_;
    Config config_;
    
    // Data placement cache (collection: key -> set<shard_id>)
    std::map<std::string, std::set<std::string>> placement_cache_;
    mutable std::shared_mutex cache_mutex_;
    
    // Bloom filter for space-efficient tracking (optional)
    // std::unique_ptr<BloomFilter> bloom_filter_;
    
    // Statistics
    Statistics stats_;
    
    // Scoring helpers
    float calculateLocalityScore(const std::string& shard_id,
                                  const QuerySpec& spec) const;
    float calculateLoadScore(const std::string& shard_id) const;
    float calculateNetworkScore(const std::string& shard_id) const;
    
    // Cache helpers
    std::string makeCacheKey(const std::string& collection,
                             const std::string& key) const;
    void cleanupStaleEntries();
};

} // namespace themis::sharding
