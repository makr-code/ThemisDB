#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

/**
 * @brief Adaptive Multi-Level Query Cache
 * 
 * Three-tier cache architecture optimized for 10B+ record datasets:
 * - Level 1 (HOT):  In-Memory HashMap, <1KB entries, TTL 5min
 * - Level 2 (WARM): Compressed (Zstd), <10KB entries, TTL 30min
 * - Level 3 (COLD): RocksDB, unbounded, TTL 24h
 * 
 * Features:
 * - Query fingerprinting (SHA256 hash of query + parameters)
 * - Adaptive TTL based on query frequency
 * - LRU eviction with frequency weighting
 * - Automatic level promotion/demotion
 * - Cache-aware query optimization hints
 * 
 * Performance Goals:
 * - 40-60% cache hit rate for typical OLAP workloads
 * - +60% throughput improvement for cached queries
 * - <1ms latency for L1 hits (HOT tier)
 * - <10ms latency for L2 hits (WARM tier)
 * 
 * Thread-Safety:
 * - All operations are thread-safe
 * - Internal mutexes protect cache structures
 * - Lock-free fast path for L1 hits
 */
class AdaptiveQueryCache {
public:
    enum class CacheLevel {
        HOT,   // L1: In-memory, fast, small
        WARM,  // L2: Compressed in-memory
        COLD   // L3: RocksDB persistent
    };
    
    struct Config {
        // L1 (HOT) configuration
        size_t l1_max_entries = 10000;         // Max entries in L1
        size_t l1_max_entry_size = 1024;       // 1KB max per entry
        int l1_ttl_seconds = 300;              // 5 minutes
        
        // L2 (WARM) configuration
        size_t l2_max_entries = 50000;         // Max entries in L2
        size_t l2_max_entry_size = 10240;      // 10KB max per entry
        int l2_ttl_seconds = 1800;             // 30 minutes
        int l2_compression_level = 3;          // Zstd compression level
        
        // L3 (COLD) configuration
        int l3_ttl_seconds = 86400;            // 24 hours
        std::string l3_db_path = "./themis_query_cache";
        
        // Adaptive TTL configuration
        bool enable_adaptive_ttl = true;
        int min_ttl_seconds = 60;              // 1 minute minimum
        int max_ttl_seconds = 86400;           // 24 hour maximum
        
        // Eviction policy
        bool enable_frequency_weighting = true;
        float frequency_weight = 0.3f;         // Weight for frequency in LRU score
    };
    
    struct CacheEntry {
        std::string query_fingerprint;
        nlohmann::json query_params;           // Original query parameters
        nlohmann::json result;                 // Cached query result
        CacheLevel level;
        int64_t created_at_ms;
        int64_t last_accessed_ms;
        int64_t access_count = 0;
        int ttl_seconds;
        size_t result_size_bytes = 0;
    };
    
    struct CacheStats {
        uint64_t l1_hits = 0;
        uint64_t l2_hits = 0;
        uint64_t l3_hits = 0;
        uint64_t misses = 0;
        uint64_t evictions = 0;
        uint64_t promotions = 0;
        uint64_t demotions = 0;
        
        double getHitRate() const {
            uint64_t total = l1_hits + l2_hits + l3_hits + misses;
            return total > 0 ? static_cast<double>(l1_hits + l2_hits + l3_hits) / total : 0.0;
        }
        
        double getL1HitRate() const {
            uint64_t total = l1_hits + l2_hits + l3_hits + misses;
            return total > 0 ? static_cast<double>(l1_hits) / total : 0.0;
        }
    };
    
    explicit AdaptiveQueryCache(const Config& config);
    ~AdaptiveQueryCache();
    
    // Non-copyable, moveable
    AdaptiveQueryCache(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache& operator=(const AdaptiveQueryCache&) = delete;
    AdaptiveQueryCache(AdaptiveQueryCache&&) = default;
    AdaptiveQueryCache& operator=(AdaptiveQueryCache&&) = default;
    
    /**
     * @brief Generate query fingerprint from query string and parameters
     * 
     * Uses SHA256 hash for consistent fingerprinting across runs.
     * 
     * @param query Query string (AQL, SQL, etc.)
     * @param params Query parameters (bind variables, limits, etc.)
     * @return SHA256 fingerprint as hex string
     */
    std::string generateFingerprint(const std::string& query, 
                                    const nlohmann::json& params = {}) const;
    
    /**
     * @brief Get cached query result
     * 
     * Searches all three cache levels (L1 -> L2 -> L3).
     * Automatically promotes frequently accessed entries to higher levels.
     * 
     * @param fingerprint Query fingerprint (from generateFingerprint)
     * @return Cached result if found and not expired, nullopt otherwise
     */
    std::optional<CacheEntry> get(const std::string& fingerprint);
    
    /**
     * @brief Store query result in cache
     * 
     * Automatically selects cache level based on result size and config.
     * L1 for <1KB, L2 for <10KB (compressed), L3 for larger results.
     * 
     * @param fingerprint Query fingerprint
     * @param query_params Original query parameters (for debugging)
     * @param result Query result to cache
     * @return True if successfully cached
     */
    bool put(const std::string& fingerprint,
             const nlohmann::json& query_params,
             const nlohmann::json& result);
    
    /**
     * @brief Invalidate cache entries matching a pattern
     * 
     * Useful for invalidating queries on a specific collection/table.
     * 
     * @param pattern Regex pattern to match query parameters
     * @return Number of entries invalidated
     */
    size_t invalidate(const std::string& pattern);
    
    /**
     * @brief Clear all cache entries
     */
    void clear();
    
    /**
     * @brief Clear expired entries from all levels
     * 
     * @return Number of entries cleared
     */
    uint64_t clearExpired();
    
    /**
     * @brief Get cache statistics
     */
    CacheStats getStats() const;
    
    /**
     * @brief Get detailed cache information (for monitoring)
     */
    nlohmann::json getDetailedInfo() const;

private:
    struct L1Entry {
        nlohmann::json result;
        int64_t created_at_ms;
        int64_t last_accessed_ms;
        int64_t access_count = 0;
        int ttl_seconds;
    };
    
    struct L2Entry {
        std::vector<uint8_t> compressed_result;  // Zstd compressed
        int64_t created_at_ms;
        int64_t last_accessed_ms;
        int64_t access_count = 0;
        int ttl_seconds;
    };
    
    Config config_;
    mutable CacheStats stats_;
    
    // L1: In-memory HashMap
    std::unordered_map<std::string, L1Entry> l1_cache_;
    mutable std::mutex l1_mutex_;
    
    // L2: Compressed in-memory
    std::unordered_map<std::string, L2Entry> l2_cache_;
    mutable std::mutex l2_mutex_;
    
    // L3: RocksDB persistent cache
    std::unique_ptr<RocksDBWrapper> l3_db_;
    mutable std::mutex l3_mutex_;
    
    // Internal helper methods
    int64_t getCurrentTimeMs() const;
    bool isExpired(int64_t created_at_ms, int ttl_seconds) const;
    int calculateAdaptiveTTL(int64_t access_count) const;
    CacheLevel selectCacheLevel(size_t result_size) const;
    void promoteEntry(const std::string& fingerprint, const CacheEntry& entry);
    void evictLRU(CacheLevel level);
    double calculateLRUScore(int64_t last_accessed_ms, int64_t access_count) const;
};

} // namespace themis
