/**
 * @file query_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Query Result Caching System
 * 
 * Implements a comprehensive caching layer for query results with:
 * - LRU (Least Recently Used) and LFU (Least Frequently Used) eviction policies
 * - TTL (Time-To-Live) based expiration
 * - Deterministic cache key generation (SHA256 fingerprinting)
 * - Dependency tracking and invalidation
 * - Thread-safe operations
 * - Memory-aware eviction
 * - Comprehensive statistics and monitoring
 * 
 * Thread Safety:
 * - All public methods are thread-safe
 * - Internal mutexes protect shared data structures
 * 
 * Performance Targets:
 * - >60% cache hit rate for workloads with repeated queries
 * - <1ms overhead for cache lookup
 * - Automatic invalidation on data changes
 */
class QueryCache {
public:
    /**
     * @brief Eviction policy for cache entries
     */
    enum class EvictionPolicy {
        LRU,  // Least Recently Used - evict entries not accessed recently
        LFU   // Least Frequently Used - evict entries accessed least often
    };
    
    /**
     * @brief Configuration for the query cache
     */
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
    
    /**
     * @brief Cache entry metadata
     */
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
    
    /**
     * @brief Cache statistics for monitoring
     */
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
    
    /**
     * @brief Result of a cache lookup
     */
    struct LookupResult {
        bool found = false;                      // Whether entry was found
        nlohmann::json result;                   // Cached result (if found)
        std::string query_fingerprint;           // Fingerprint of matched query
        
        explicit LookupResult(bool f = false) : found(f) {}
    };

public:
    /**
     * @brief Construct a new Query Cache
     * 
     * @param config Cache configuration
     */
    explicit QueryCache(const Config& config);
    
    /**
     * @brief Destructor - cleanup resources
     */
    ~QueryCache();
    
    // Non-copyable, moveable
    QueryCache(const QueryCache&) = delete;
    QueryCache& operator=(const QueryCache&) = delete;
    QueryCache(QueryCache&&) noexcept = default;
    QueryCache& operator=(QueryCache&&) noexcept = default;
    
    /**
     * @brief Generate deterministic fingerprint for a query
     * 
     * Creates a SHA256 hash from the query string and parameters.
     * Identical queries with identical parameters produce identical fingerprints.
     * 
     * @param query Query string
     * @param params Query parameters (optional)
     * @return SHA256 fingerprint as hex string
     */
    std::string generateFingerprint(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    ) const;
    
    /**
     * @brief Store query result in cache
     * 
     * @param query Original query string
     * @param params Query parameters
     * @param result Query result to cache
     * @param dependencies Data dependencies (tables/collections accessed)
     * @param ttl Optional TTL override (uses default if not specified)
     * @return Result<void> Success or error
     */
    Result<void> put(
        const std::string& query,
        const nlohmann::json& params,
        const nlohmann::json& result,
        const std::vector<std::string>& dependencies = {},
        std::optional<std::chrono::seconds> ttl = std::nullopt
    );
    
    /**
     * @brief Retrieve cached query result
     * 
     * @param query Query string
     * @param params Query parameters
     * @return Result<LookupResult> Lookup result or error
     */
    Result<LookupResult> get(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Invalidate cache entries by dependency
     * 
     * Invalidates all cached queries that depend on the specified resource.
     * Useful for invalidating caches when data is modified.
     * 
     * @param dependency Dependency identifier (e.g., "users", "orders")
     * @return Result<size_t> Number of entries invalidated
     */
    Result<size_t> invalidateByDependency(const std::string& dependency);
    
    /**
     * @brief Invalidate specific cached query
     * 
     * @param query Query string
     * @param params Query parameters
     * @return Result<bool> True if entry was found and removed
     */
    Result<bool> invalidate(
        const std::string& query,
        const nlohmann::json& params = nlohmann::json::object()
    );
    
    /**
     * @brief Clear all cache entries
     * 
     * @return Result<void> Success or error
     */
    Result<void> clear();
    
    /**
     * @brief Remove expired entries
     * 
     * @return Result<size_t> Number of entries removed
     */
    Result<size_t> clearExpired();
    
    /**
     * @brief Get current cache statistics
     * 
     * @return CacheStats Current statistics
     */
    CacheStats getStats() const;
    
    /**
     * @brief Get detailed cache information
     * 
     * @return nlohmann::json Detailed cache info for monitoring
     */
    nlohmann::json getDetailedInfo() const;
    
    /**
     * @brief Reset statistics counters
     */
    void resetStats();
    
    /**
     * @brief Update cache configuration
     * 
     * @param config New configuration
     * @return Result<void> Success or error
     */
    Result<void> setConfig(const Config& config);
    
    /**
     * @brief Get current configuration
     * 
     * @return Config Current configuration
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
    void evictLRU();
    void evictLFU();
    void evictOne();
    void updateLRU(const std::string& fingerprint);
    bool shouldEvict() const;
    size_t estimateEntrySize(const CacheEntry& entry) const;
    void updateStats(bool hit);
    void addToDependencyIndex(const std::string& fingerprint, 
                              const std::vector<std::string>& dependencies);
    void removeFromDependencyIndex(const std::string& fingerprint,
                                   const std::vector<std::string>& dependencies);
};

} // namespace query
} // namespace themis
