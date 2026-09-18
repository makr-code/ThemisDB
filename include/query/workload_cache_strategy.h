/**
 * @file workload_cache_strategy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class WorkloadType {
    OLTP,         // Online Transaction Processing - frequent small queries
    OLAP,         // Online Analytical Processing - infrequent large queries
    MIXED,        // Mixed workload - adaptive strategy
    STREAMING,    // Streaming/real-time - minimal or no caching
    UNKNOWN       // Not yet classified
};

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
     * @brief For Workload.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static WorkloadCacheConfig forWorkload(WorkloadType type);
};

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
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };
    
    /**
     * @brief Workload Cache Strategy.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit WorkloadCacheStrategy(const Config& config);
    ~WorkloadCacheStrategy() = default;
    
    // Non-copyable, moveable
    WorkloadCacheStrategy(const WorkloadCacheStrategy&) = delete;
    WorkloadCacheStrategy& operator=(const WorkloadCacheStrategy&) = delete;
    WorkloadCacheStrategy(WorkloadCacheStrategy&&) noexcept = default;
    WorkloadCacheStrategy& operator=(WorkloadCacheStrategy&&) noexcept = default;
    
    /**
     * @brief Record Query.
     * @param[in] query_fingerprint Input parameter.
     * @param[in] characteristics Input parameter.
     */
    void recordQuery(
        const std::string& query_fingerprint,
        const QueryCharacteristics& characteristics
    );
    
    /**
     * @brief Detect Workload.
     * @return Return value.
     */
    WorkloadType detectWorkload();
    
    /**
     * @brief Get Cache Config.
     * @return Return value.
     */
    WorkloadCacheConfig getCacheConfig() const;
    
    /**
     * @brief Get Cache Config For Query.
     * @param[in] characteristics Input parameter.
     * @return Return value.
     */
    WorkloadCacheConfig getCacheConfigForQuery(
        const QueryCharacteristics& characteristics
    ) const;
    
    /**
     * @brief Should Cache.
     * @param[in] characteristics Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldCache(const QueryCharacteristics& characteristics) const;
    
    /**
     * @brief Calculate TTL.
     * @param[in] characteristics Input parameter.
     * @return Return value.
     */
    std::chrono::seconds calculateTTL(
        const QueryCharacteristics& characteristics
    ) const;
    
    std::vector<std::string> getHotQueries(size_t limit = 100) const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    WorkloadStats getStats() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
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
    /**
     * @brief Classify Workload.
     * @return Return value.
     */
    WorkloadType classifyWorkload() const;
    /**
     * @brief Update Stats.
     */
    void updateStats();
    /**
     * @brief Should Run Detection.
     * @return True when the operation succeeds.
     */
    bool shouldRunDetection() const;
};

} // namespace query
} // namespace themis
