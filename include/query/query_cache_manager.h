/**
 * @file query_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Unified cache manager with workload-aware caching
 * 
 * This class provides a single interface for query result caching that
 * automatically adapts to workload patterns:
 * 
 * - Detects workload type (OLTP, OLAP, Mixed, Streaming)
 * - Selects optimal cache strategy per workload
 * - Manages cache warming and invalidation
 * - Provides unified monitoring and statistics
 * 
 * Integration Point:
 * This should be used in the query execution path (QueryEngine or API layer)
 * to cache query results with optimal strategies.
 * 
 * Example Usage:
 * ```cpp
 * // At query execution time
 * QueryCacheManager cache_mgr(config);
 * 
 * // Try to get from cache
 * auto cached = cache_mgr.get(query, params);
 * if (cached) {
 *     return *cached;
 * }
 * 
 * // Execute query
 * auto result = executeQuery(query, params);
 * 
 * // Store in cache with execution metrics
 * QueryCharacteristics char_;
 * char_.result_size_bytes = result.size();
 * char_.execution_time_ms = exec_time;
 * cache_mgr.put(query, params, result, char_, dependencies);
 * ```
 */
class QueryCacheManager {
public:
    /**
     * @brief Cache manager configuration
     */
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
    
    /**
     * @brief Unified cache statistics
     */
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
        
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Construct cache manager with configuration
     */
    explicit QueryCacheManager(const Config& config);
    
    /**
     * @brief Destructor - cleanup and final stats report
     */
    ~QueryCacheManager();
    
    // Non-copyable, moveable
    QueryCacheManager(const QueryCacheManager&) = delete;
    QueryCacheManager& operator=(const QueryCacheManager&) = delete;
    QueryCacheManager(QueryCacheManager&&) noexcept = default;
    QueryCacheManager& operator=(QueryCacheManager&&) noexcept = default;
    
    /**
     * @brief Retrieve cached query result
     * 
     * This method checks the cache for a previously executed query.
     * Records cache hit/miss metrics automatically.
     * 
     * @param query Query string
     * @param params Query parameters
     * @return Cached result if found, nullopt otherwise
     */
    std::optional<nlohmann::json> get(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Store query result in cache with execution metrics
     * 
     * This method stores a query result using workload-aware caching strategy.
     * It automatically:
     * - Records query characteristics for workload detection
     * - Determines if query should be cached
     * - Calculates optimal TTL
     * - Selects appropriate cache level (for adaptive cache)
     * 
     * @param query Query string
     * @param params Query parameters
     * @param result Query result to cache
     * @param characteristics Query execution metrics
     * @param dependencies Data dependencies (tables/collections accessed)
     * @return True if successfully cached
     */
    bool put(
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const QueryCharacteristics& characteristics,
        const std::vector<std::string>& dependencies = {}
    );
    
    /**
     * @brief Invalidate cache entries by dependency
     * 
     * Call this when data is modified to invalidate affected cached queries.
     * 
     * @param dependency Dependency identifier (e.g., table/collection name)
     * @return Number of entries invalidated
     */
    size_t invalidateByDependency(const std::string& dependency);
    
    /**
     * @brief Invalidate specific cached query
     * 
     * @param query Query string
     * @param params Query parameters
     * @return True if entry was found and removed
     */
    bool invalidate(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Clear all cache entries
     */
    void clear();
    
    /**
     * @brief Warm cache with hot queries
     * 
     * This method should be called on startup or periodically to pre-populate
     * the cache with frequently accessed queries.
     * 
     * @param query_results Map of query fingerprints to results
     */
    void warmCache(const std::map<std::string, nlohmann::json>& query_results);
    
    /**
     * @brief Get cache statistics
     */
    CacheStatistics getStatistics() const;
    
    /**
     * @brief Get hot query fingerprints for cache warming
     * 
     * Returns the most frequently accessed queries for external cache warming.
     * 
     * @param limit Maximum number of queries to return
     * @return List of query fingerprints sorted by frequency
     */
    std::vector<std::string> getHotQueries(size_t limit = 100) const;
    
    /**
     * @brief Get current workload type
     */
    WorkloadType getCurrentWorkload() const;
    
    /**
     * @brief Get detailed monitoring information
     */
    nlohmann::json getMonitoringInfo() const;
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Get current configuration
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
    std::string generateFingerprint(
        const std::string& query,
        const nlohmann::json& params
    ) const;
    
    void updateHitStats(bool hit, int64_t lookup_time_us);
    void updateMemoryStats();
    void reportStatsIfNeeded();
    
    // Cache operations on selected implementation
    bool putInBasicCache(
        const std::string& fingerprint,
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const std::vector<std::string>& dependencies,
        std::chrono::seconds ttl
    );
    
    bool putInAdaptiveCache(
        const std::string& fingerprint,
        const nlohmann::json& params,
        const nlohmann::json& result,
        std::chrono::seconds ttl
    );
};

} // namespace query
} // namespace themis
