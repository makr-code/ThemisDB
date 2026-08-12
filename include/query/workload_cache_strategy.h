/**
 * @file workload_cache_strategy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <chrono>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "query/query_cache.h"
#include "cache/adaptive_query_cache.h"

namespace themis {
namespace query {

/**
 * @brief Workload types for cache optimization
 * 
 * Different workload patterns benefit from different caching strategies:
 * - OLTP: High-frequency, small results, short TTL
 * - OLAP: Low-frequency, large results, long TTL
 * - MIXED: Adaptive behavior based on query patterns
 * - STREAMING: Minimal caching with very short TTL (real-time data)
 */
enum class WorkloadType {
    OLTP,         // Online Transaction Processing - frequent small queries
    OLAP,         // Online Analytical Processing - infrequent large queries
    MIXED,        // Mixed workload - adaptive strategy
    STREAMING,    // Streaming/real-time - minimal or no caching
    UNKNOWN       // Not yet classified
};

/**
 * @brief Query characteristics for workload detection
 */
struct QueryCharacteristics {
    size_t result_size_bytes = 0;        // Size of query result
    size_t rows_scanned = 0;             // Number of rows scanned
    size_t rows_returned = 0;            // Number of rows returned
    int64_t execution_time_ms = 0;       // Execution time
    int64_t access_count = 0;            // How often this query pattern seen
    std::chrono::system_clock::time_point first_seen;
    std::chrono::system_clock::time_point last_accessed;
    
    // Derived metrics
    double selectivity() const {
        return rows_scanned > 0 
            ? static_cast<double>(rows_returned) / rows_scanned 
            : 1.0;
    }
    
    double frequency_per_minute() const {
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(
            last_accessed - first_seen);
        // Handle case where query was just added (duration ~0)
        // Use minimum duration of 1 minute to avoid division by zero
        int64_t duration_min = std::max(static_cast<int64_t>(duration.count()), int64_t(1));
        return static_cast<double>(access_count) / duration_min;
    }
};

/**
 * @brief Workload-specific cache configuration
 */
struct WorkloadCacheConfig {
    WorkloadType type = WorkloadType::UNKNOWN;
    
    // Cache size settings
    size_t max_entries = 10000;
    size_t max_memory_bytes = 100 * 1024 * 1024;  // 100MB
    size_t max_entry_size = 10 * 1024 * 1024;     // 10MB per entry
    
    // TTL settings
    std::chrono::seconds default_ttl{3600};        // 1 hour
    std::chrono::seconds min_ttl{60};              // 1 minute
    std::chrono::seconds max_ttl{86400};           // 24 hours
    
    // Adaptive settings
    bool enable_adaptive_ttl = true;
    bool enable_frequency_weighting = true;
    
    // Eviction policy
    QueryCache::EvictionPolicy eviction_policy = QueryCache::EvictionPolicy::LRU;
    
    // Workload-specific thresholds
    double high_frequency_threshold = 10.0;  // queries per minute
    double low_frequency_threshold = 0.1;    // queries per minute
    size_t large_result_threshold = 1024 * 1024;  // 1MB
    size_t small_result_threshold = 10 * 1024;    // 10KB
    
    /**
     * @brief Create configuration optimized for specific workload type
     */
    static WorkloadCacheConfig forWorkload(WorkloadType type);
};

/**
 * @brief Detects workload patterns and provides optimized cache strategies
 * 
 * This class analyzes query patterns over time to classify workloads and
 * automatically configure caching strategies for optimal performance.
 * 
 * Features:
 * - Automatic workload detection (OLTP, OLAP, Mixed)
 * - Dynamic cache configuration adjustment
 * - Query pattern tracking and analysis
 * - Cache warming for frequently accessed queries
 * - Performance metrics and monitoring
 */
class WorkloadCacheStrategy {
public:
    struct Config {
        bool enable_workload_detection = true;
        double detection_sample_rate = 0.1;        // Sample 10% of queries
        std::chrono::seconds detection_window{300};  // 5 minute window
        size_t min_samples_for_detection = 100;    // Min queries before classification
        
        // Workload classification thresholds
        double oltp_frequency_threshold = 10.0;    // >10 queries/min = OLTP
        double olap_frequency_threshold = 0.5;     // <0.5 queries/min = OLAP
        size_t oltp_result_size_threshold = 50 * 1024;   // <50KB = OLTP-like
        size_t olap_result_size_threshold = 1024 * 1024; // >1MB = OLAP-like
    };
    
    struct WorkloadStats {
        WorkloadType detected_type = WorkloadType::UNKNOWN;
        size_t total_queries = 0;
        size_t cached_queries = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        double avg_query_frequency = 0.0;
        size_t avg_result_size = 0;
        int64_t avg_execution_time_ms = 0;
        
        double hit_rate() const {
            return (cache_hits + cache_misses) > 0 
                ? static_cast<double>(cache_hits) / (cache_hits + cache_misses)
                : 0.0;
        }
        
        nlohmann::json toJson() const;
    };
    
    explicit WorkloadCacheStrategy(const Config& config);
    ~WorkloadCacheStrategy() = default;
    
    // Non-copyable, moveable
    WorkloadCacheStrategy(const WorkloadCacheStrategy&) = delete;
    WorkloadCacheStrategy& operator=(const WorkloadCacheStrategy&) = delete;
    WorkloadCacheStrategy(WorkloadCacheStrategy&&) = default;
    WorkloadCacheStrategy& operator=(WorkloadCacheStrategy&&) = default;
    
    /**
     * @brief Record query execution characteristics
     * 
     * @param query_fingerprint Unique identifier for query pattern
     * @param characteristics Query execution metrics
     */
    void recordQuery(
        const std::string& query_fingerprint,
        const QueryCharacteristics& characteristics
    );
    
    /**
     * @brief Detect current workload type based on recorded patterns
     * 
     * Analyzes recent query patterns to classify workload as OLTP, OLAP, or Mixed.
     * This may trigger reconfiguration of cache strategies.
     * 
     * @return Detected workload type
     */
    WorkloadType detectWorkload();
    
    /**
     * @brief Get cache configuration for current workload
     * 
     * Returns an optimized cache configuration based on detected workload patterns.
     * 
     * @return Workload-specific cache configuration
     */
    WorkloadCacheConfig getCacheConfig() const;
    
    /**
     * @brief Get cache configuration for specific query
     * 
     * Analyzes individual query characteristics to determine optimal caching strategy.
     * 
     * @param characteristics Query characteristics
     * @return Recommended cache configuration for this query
     */
    WorkloadCacheConfig getCacheConfigForQuery(
        const QueryCharacteristics& characteristics
    ) const;
    
    /**
     * @brief Check if query should be cached
     * 
     * Some queries (e.g., very large results, streaming queries) should not be cached.
     * 
     * @param characteristics Query characteristics
     * @return True if query should be cached
     */
    bool shouldCache(const QueryCharacteristics& characteristics) const;
    
    /**
     * @brief Calculate optimal TTL for a query
     * 
     * TTL is calculated based on query frequency and workload type:
     * - High frequency queries: shorter TTL
     * - Low frequency queries: longer TTL
     * 
     * @param characteristics Query characteristics
     * @return Optimal TTL in seconds
     */
    std::chrono::seconds calculateTTL(
        const QueryCharacteristics& characteristics
    ) const;
    
    /**
     * @brief Get frequently accessed queries for cache warming
     * 
     * @param limit Maximum number of queries to return
     * @return List of query fingerprints sorted by frequency
     */
    std::vector<std::string> getHotQueries(size_t limit = 100) const;
    
    /**
     * @brief Get current workload statistics
     */
    WorkloadStats getStats() const;
    
    /**
     * @brief Reset workload detection state
     */
    void reset();
    
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
    mutable std::mutex mutex_;
    
    // Current detected workload
    std::atomic<WorkloadType> current_workload_{WorkloadType::UNKNOWN};
    
    // Query pattern tracking
    std::unordered_map<std::string, QueryCharacteristics> query_patterns_;
    
    // Workload statistics
    WorkloadStats stats_;
    
    // Last detection time
    std::chrono::system_clock::time_point last_detection_;
    
    // Helper methods
    WorkloadType classifyWorkload() const;
    void updateStats();
    bool shouldRunDetection() const;
};

} // namespace query
} // namespace themis
