/**
 * @file query_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/expected.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>
#include <chrono>
#include <optional>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <list>

namespace themis {
namespace query {

class QueryCache {
public:
    enum class EvictionPolicy {
        LRU,  // Least Recently Used - evict entries not accessed recently
        LFU   // Least Frequently Used - evict entries accessed least often
    };
    
    struct Config {
        // Cache size limits
        size_t max_entries = 10000;              // Maximum number of cached queries
        size_t max_memory_bytes = 100 * 1024 * 1024;  // 100MB default memory limit
        size_t max_entry_size = 10 * 1024 * 1024;     // 10MB max per entry
        
        // Eviction policy
        EvictionPolicy eviction_policy = EvictionPolicy::LRU;
        
        // TTL configuration
        std::chrono::seconds default_ttl{3600};  // 1 hour default TTL
        bool enable_ttl = true;                  // Enable TTL-based expiration
        
        // Memory management
        bool enable_memory_pressure_eviction = true;
        float memory_pressure_threshold = 0.9f;   // Evict when 90% full
        
        // Statistics
        bool track_statistics = true;
        
        Config() = default;
    };
    
    struct CacheEntry {
        std::string query_fingerprint;           // SHA256 hash of query
        std::string original_query;              // Original query string
        nlohmann::json query_params;             // Query parameters
        nlohmann::json result;                   // Cached result
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_accessed;
        size_t access_count = 0;                 // For LFU policy
        size_t result_size_bytes = 0;            // Memory footprint
        std::chrono::seconds ttl;                // Time-to-live
        std::vector<std::string> dependencies;   // Data dependencies (tables, collections)
        
        bool isExpired() const {
            auto now = std::chrono::system_clock::now();
            auto age = std::chrono::duration_cast<std::chrono::seconds>(now - created_at);
            return age > ttl;
        }
    };
    
    struct CacheStats {
        uint64_t total_requests = 0;             // Total cache lookups
        uint64_t hits = 0;                       // Cache hits
        uint64_t misses = 0;                     // Cache misses
        uint64_t evictions = 0;                  // Entries evicted
        uint64_t expirations = 0;                // Entries expired
        uint64_t invalidations = 0;              // Manual invalidations
        size_t current_entries = 0;              // Current number of entries
        size_t current_memory_bytes = 0;         // Current memory usage
        
        double hitRate() const {
            return total_requests > 0 
                ? static_cast<double>(hits) / total_requests 
                : 0.0;
        }
        
        double memoryUtilization(size_t max_memory) const {
            return max_memory > 0 
                ? static_cast<double>(current_memory_bytes) / max_memory 
                : 0.0;
        }
    };
    
    struct LookupResult {
        bool found = false;                      // Whether entry was found
        nlohmann::json result;                   // Cached result (if found)
        std::string query_fingerprint;           // Fingerprint of matched query
        
        explicit LookupResult(bool f = false) : found(f) {}
    };

public:
    /**
     * @brief Query Cache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit QueryCache(const Config& config);
    
    ~QueryCache();
    
    // Non-copyable, moveable
    QueryCache(const QueryCache&) = delete;
    QueryCache& operator=(const QueryCache&) = delete;
    QueryCache(QueryCache&&) noexcept = default;
    QueryCache& operator=(QueryCache&&) noexcept = default;
    
    std::string generateFingerprint(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    ) const;
    
    Result<void> put(
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const std::vector<std::string>& dependencies = {},
        std::optional<std::chrono::seconds> ttl = std::nullopt
    );
    
    Result<LookupResult> get(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Invalidate By Dependency.
     * @param[in] dependency Input parameter.
     * @return Return value.
     */
    Result<size_t> invalidateByDependency(const std::string& dependency);
    
    Result<bool> invalidate(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Clear.
     * @return Return value.
     */
    Result<void> clear();
    
    /**
     * @brief Clear Expired.
     * @return Return value.
     */
    Result<size_t> clearExpired();
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    CacheStats getStats() const;
    
    /**
     * @brief Get Detailed Info.
     * @return Return value.
     */
    nlohmann::json getDetailedInfo() const;
    
    /**
     * @brief Reset Stats.
     */
    void resetStats();
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    Result<void> setConfig(const Config& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    Config getConfig() const;

private:
    // Internal cache entry with LRU/LFU tracking
    struct InternalCacheEntry {
        CacheEntry entry;
        std::list<std::string>::iterator lru_it;  // Iterator for LRU list
        
        InternalCacheEntry() = default;
        InternalCacheEntry(CacheEntry e) : entry(std::move(e)) {}
    };
    
    // Configuration
    Config config_;
    
    // Cache storage
    std::unordered_map<std::string, InternalCacheEntry> cache_;
    mutable std::mutex cache_mutex_;
    
    // LRU tracking (most recent at front)
    std::list<std::string> lru_list_;
    
    // Dependency index (dependency -> set of fingerprints)
    std::unordered_map<std::string, std::vector<std::string>> dependency_index_;
    mutable std::mutex dependency_mutex_;
    
    // Statistics
    mutable CacheStats stats_;
    mutable std::mutex stats_mutex_;
    
    // Helper methods
    /**
     * @brief Evict LRU.
     */
    void evictLRU();
    /**
     * @brief Evict LFU.
     */
    void evictLFU();
    /**
     * @brief Evict One.
     */
    void evictOne();
    /**
     * @brief Update LRU.
     * @param[in] fingerprint Input parameter.
     */
    void updateLRU(const std::string& fingerprint);
    /**
     * @brief Should Evict.
     * @return True when the operation succeeds.
     */
    bool shouldEvict() const;
    /**
     * @brief Estimate Entry Size.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    size_t estimateEntrySize(const CacheEntry& entry) const;
    /**
     * @brief Update Stats.
     * @param[in] hit Input parameter.
     */
    void updateStats(bool hit);
    /**
     * @brief Add To Dependency Index.
     * @param[in] fingerprint Input parameter.
     * @param[in] dependencies Input parameter.
     */
    void addToDependencyIndex(const std::string& fingerprint, 
                              const std::vector<std::string>& dependencies);
    /**
     * @brief Remove From Dependency Index.
     * @param[in] fingerprint Input parameter.
     * @param[in] dependencies Input parameter.
     */
    void removeFromDependencyIndex(const std::string& fingerprint,
                                   const std::vector<std::string>& dependencies);
};

} // namespace query
} // namespace themis
