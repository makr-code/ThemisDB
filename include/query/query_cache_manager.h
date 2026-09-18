/**
 * @file query_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <optional>
#include <chrono>
#include <map>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>
#include "query/query_cache.h"
#include "cache/adaptive_query_cache.h"
#include "query/workload_cache_strategy.h"
#include "utils/expected.h"

namespace themis {
namespace query {

class QueryCacheManager {
public:
    struct Config {
        // Enable/disable caching globally
        bool enable_caching = true;
        
        // Cache implementation to use
        enum class CacheType {
            BASIC,      // Basic QueryCache (single-level LRU/LFU)
            ADAPTIVE    // AdaptiveQueryCache (3-tier HOT/WARM/COLD)
        };
        CacheType cache_type = CacheType::ADAPTIVE;
        
        // Workload detection
        bool enable_workload_detection = true;
        double workload_detection_sample_rate = 0.1;  // Sample 10% of queries
        
        // Cache warming
        bool enable_cache_warming = true;
        size_t cache_warm_top_k = 100;  // Warm top 100 queries on startup
        
        // Statistics
        bool enable_detailed_stats = true;
        std::chrono::seconds stats_report_interval{300};  // Report every 5 minutes
        
        Config() = default;
    };
    
    struct CacheStatistics {
        // Overall statistics
        uint64_t total_requests = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        uint64_t cache_stores = 0;
        uint64_t cache_evictions = 0;
        uint64_t cache_invalidations = 0;
        
        // Workload statistics
        WorkloadType detected_workload = WorkloadType::UNKNOWN;
        size_t oltp_queries = 0;
        size_t olap_queries = 0;
        size_t mixed_queries = 0;
        
        // Performance metrics
        int64_t avg_cache_hit_time_us = 0;     // Microseconds
        int64_t avg_cache_miss_time_us = 0;    // Microseconds
        int64_t total_time_saved_ms = 0;       // Time saved by cache hits
        
        // Memory usage
        size_t current_memory_bytes = 0;
        size_t max_memory_bytes = 0;
        
        double hitRate() const {
            return total_requests > 0 
                ? static_cast<double>(cache_hits) / total_requests 
                : 0.0;
        }
        
        double memoryUtilization() const {
            return max_memory_bytes > 0 
                ? static_cast<double>(current_memory_bytes) / max_memory_bytes 
                : 0.0;
        }
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Query Cache Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit QueryCacheManager(const Config& config);
    
    ~QueryCacheManager();
    
    // Non-copyable, moveable
    QueryCacheManager(const QueryCacheManager&) = delete;
    QueryCacheManager& operator=(const QueryCacheManager&) = delete;
    QueryCacheManager(QueryCacheManager&&) noexcept = default;
    QueryCacheManager& operator=(QueryCacheManager&&) noexcept = default;
    
    std::optional<nlohmann::json> get(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    bool put(
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const QueryCharacteristics& characteristics,
        const std::vector<std::string>& dependencies = {}
    );
    
    /**
     * @brief Invalidate By Dependency.
     * @param[in] dependency Input parameter.
     * @return Return value.
     */
    size_t invalidateByDependency(const std::string& dependency);
    
    bool invalidate(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Clear.
     */
    void clear();
    
    void warmCache(const std::map<std::string, nlohmann::json>& query_results);
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    CacheStatistics getStatistics() const;
    
    std::vector<std::string> getHotQueries(size_t limit = 100) const;
    
    /**
     * @brief Get Current Workload.
     * @return Return value.
     */
    WorkloadType getCurrentWorkload() const;
    
    /**
     * @brief Get Monitoring Info.
     * @return Return value.
     */
    nlohmann::json getMonitoringInfo() const;
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    Config getConfig() const;

private:
    Config config_;
    
    // Cache implementations
    std::unique_ptr<QueryCache> basic_cache_;
    std::unique_ptr<AdaptiveQueryCache> adaptive_cache_;
    
    // Workload detection strategy
    std::unique_ptr<WorkloadCacheStrategy> workload_strategy_;
    
    // Statistics
    mutable CacheStatistics stats_;
    mutable std::mutex stats_mutex_;
    
    // Last statistics report time
    std::chrono::system_clock::time_point last_stats_report_;
    
    // Helper methods
    /**
     * @brief Generate Fingerprint.
     * @param[in] query Input parameter.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    std::string generateFingerprint(
        const std::string& query,
        const nlohmann::json& params
    ) const;
    
    /**
     * @brief Update Hit Stats.
     * @param[in] hit Input parameter.
     * @param[in] lookup_time_us Input parameter.
     */
    void updateHitStats(bool hit, int64_t lookup_time_us);
    /**
     * @brief Update Memory Stats.
     */
    void updateMemoryStats();
    /**
     * @brief Report Stats If Needed.
     */
    void reportStatsIfNeeded();
    
    /**
     * @brief Cache operations on selected implementation
     * @param[in] fingerprint Input parameter.
     * @param[in] query Input parameter.
     * @param[in] params Input parameter.
     * @param[in] result Input parameter.
     * @param[in] dependencies Input parameter.
     * @param[in] ttl Input parameter.
     * @return True when the operation succeeds.
     */
    bool putInBasicCache(
        const std::string& fingerprint,
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const std::vector<std::string>& dependencies,
        std::chrono::seconds ttl
    );
    
    /**
     * @brief Put In Adaptive Cache.
     * @param[in] fingerprint Input parameter.
     * @param[in] params Input parameter.
     * @param[in] result Input parameter.
     * @param[in] ttl Input parameter.
     * @return True when the operation succeeds.
     */
    bool putInAdaptiveCache(
        const std::string& fingerprint,
        const nlohmann::json& params,
        const nlohmann::json& result,
        std::chrono::seconds ttl
    );
};

} // namespace query
} // namespace themis
