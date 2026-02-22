/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_query_cache.h                             ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:38:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     353                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "cache/cache_metrics.h"

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
        
        // Eviction policy
        bool enable_frequency_weighting = true;
        float frequency_weight = 0.3f;         // Weight for frequency in LRU score
        
        // Size limits (Phase 1: Security)
        size_t max_total_entry_size = 10485760; // 10MB absolute max per entry
        bool enable_size_limits = true;         // Enable size validation
        
        // Circuit breaker configuration (Phase 1: Fault Isolation)
        bool enable_circuit_breaker = true;
        uint32_t cb_failure_threshold = 5;      // Failures before opening
        uint32_t cb_timeout_ms = 60000;         // 1 minute timeout
        
        // Phase 2: Rate limiting & backpressure
        bool enable_rate_limiting = false;       // Enable rate limiting (opt-in)
        uint32_t max_requests_per_second = 10000; // Global rate limit
        bool enable_backpressure = true;         // Enable backpressure
        size_t l3_write_queue_size = 1000;       // Max queued L3 writes
        
        // Phase 2: Tenant isolation
        bool enable_tenant_isolation = false;    // Enable tenant namespacing (opt-in)
        size_t per_tenant_max_bytes = 104857600; // 100MB per tenant default
        
        // Phase 3: Adaptive TTL tuning
        bool enable_adaptive_ttl = false;        // Enable adaptive TTL based on access patterns
        int min_ttl_seconds = 60;                // Legacy alias for adaptive_ttl_min_seconds
        int max_ttl_seconds = 86400;             // Legacy alias for adaptive_ttl_max_seconds
        int adaptive_ttl_min_seconds = 60;       // Minimum TTL (1 minute)
        int adaptive_ttl_max_seconds = 86400;    // Maximum TTL (24 hours)
        double adaptive_ttl_scaling_factor = 5.0; // Scaling factor for logarithmic growth
        
        /**
         * @brief Validate configuration parameters
         * @return true if config is valid, false otherwise
         */
        bool validate(std::string* error_msg = nullptr) const;
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
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @return SHA256 fingerprint as hex string
     */
    std::string generateFingerprint(const std::string& query, 
                                    const nlohmann::json& params = {},
                                    const std::string& tenant_id = "") const;
    
    /**
     * @brief Get cached query result
     * 
     * Searches all three cache levels (L1 -> L2 -> L3).
     * Automatically promotes frequently accessed entries to higher levels.
     * 
     * @param fingerprint Query fingerprint (from generateFingerprint)
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @return Cached result if found and not expired, nullopt otherwise
     */
    std::optional<CacheEntry> get(const std::string& fingerprint,
                                   const std::string& tenant_id = "");
    
    /**
     * @brief Store query result in cache
     * 
     * Automatically selects cache level based on result size and config.
     * L1 for <1KB, L2 for <10KB (compressed), L3 for larger results.
     * 
     * @param fingerprint Query fingerprint
     * @param query_params Original query parameters (for debugging)
     * @param result Query result to cache
     * @param tenant_id Optional tenant ID for namespace isolation (Phase 2)
     * @return True if successfully cached
     */
    bool put(const std::string& fingerprint,
             const nlohmann::json& query_params,
             const nlohmann::json& result,
             const std::string& tenant_id = "");
    
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
     * @brief Get enhanced metrics (Phase 1: Observability)
     */
    const cache::CacheMetrics& getEnhancedMetrics() const {
        return enhanced_metrics_;
    }
    
    /**
     * @brief Get detailed cache information (for monitoring)
     */
    nlohmann::json getDetailedInfo() const;
    
    // ========================================================================
    // Phase 3: Admin API & Operational Tooling
    // ========================================================================
    
    /**
     * @brief Get statistics by cache tier
     * @return JSON with per-tier statistics
     */
    nlohmann::json getStatsByTier() const;
    
    /**
     * @brief Get cache health status
     * @return JSON with health information and warnings
     */
    nlohmann::json getHealthStatus() const;
    
    /**
     * @brief Export cache keys for debugging
     * @param max_keys Maximum number of keys to export (default: 100)
     * @return Vector of cache keys
     */
    std::vector<std::string> exportKeys(size_t max_keys = 100) const;
    
    /**
     * @brief Get tenant usage statistics
     * @return JSON with per-tenant size usage
     */
    nlohmann::json getTenantStats() const;
    
    /**
     * @brief Bulk put for cache warmup
     * @param entries Vector of {fingerprint, params, result, tenant_id} tuples
     * @return Number of successfully cached entries
     */
    size_t bulkPut(const std::vector<std::tuple<std::string, nlohmann::json, nlohmann::json, std::string>>& entries);
    
    /**
     * @brief Invalidate all entries for a specific tenant
     * @param tenant_id Tenant ID to invalidate
     * @return Number of entries invalidated
     */
    size_t invalidateTenant(const std::string& tenant_id);

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
    mutable cache::CacheMetrics enhanced_metrics_;  // Enhanced metrics (Phase 1)
    mutable CacheStats stats_;  // Kept for backward compatibility
    
    // Circuit breaker for L3 (RocksDB) operations (Phase 1)
    std::unique_ptr<cache::CircuitBreaker> l3_circuit_breaker_;
    
    // Phase 2: Rate limiter
    std::unique_ptr<cache::RateLimiter> rate_limiter_;
    
    // Phase 2: Tenant isolation - track per-tenant sizes
    std::unordered_map<std::string, size_t> tenant_sizes_;
    mutable std::mutex tenant_mutex_;
    
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
    
    // Phase 1: Size validation and security
    bool validateEntrySize(size_t size, CacheLevel level) const;
    bool isWithinSizeLimit(size_t size) const;
    
    // Phase 2: Tenant isolation helpers
    std::string makeTenantKey(const std::string& fingerprint, const std::string& tenant_id) const;
    bool checkTenantQuota(const std::string& tenant_id, size_t additional_bytes);
};

} // namespace themis
