/**
 * @file semantic_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis {

/**
 * @brief Semantic cache for LLM responses with TTL support
 * 
 * Provides exact-match caching based on hash(prompt+params).
 * Optionally supports similarity-based retrieval using embeddings.
 * 
 * Storage: RocksDB Column Family "semantic_cache"
 * Key: SHA256(prompt + params)
 * Value: JSON {response, metadata, timestamp, ttl_seconds}
 */
class SemanticCache {
public:
    /**
     * @brief Cache entry metadata
     */
    struct CacheEntry {
        std::string response;
        nlohmann::json metadata;
        int64_t timestamp_ms;
        int ttl_seconds;
        
        nlohmann::json toJson() const;
        static std::optional<CacheEntry> fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Cache statistics
     */
    struct Stats {
        uint64_t hit_count = 0;
        uint64_t miss_count = 0;
        uint64_t total_entries = 0;
        uint64_t total_size_bytes = 0;
        double hit_rate = 0.0;
        double avg_latency_ms = 0.0;
        
        nlohmann::json toJson() const;
    };

    /**
     * @brief Construct a new Semantic Cache object
     * 
    * @param db RocksDB TransactionDB instance
     * @param cf_handle Column family handle for semantic_cache
     * @param default_ttl_seconds Default TTL for cache entries (0 = no expiry)
     * @param bg_expiry_interval_s  Seconds between automatic background expiry sweeps.
     *                              0 disables the background thread (call clearExpired() manually).
     */
    SemanticCache(
    rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf_handle,
        int default_ttl_seconds = 3600,
        int bg_expiry_interval_s = 300
    );

    ~SemanticCache();

    /**
     * @brief Put a response into the cache
     * 
     * @param prompt The prompt text
     * @param params Additional parameters (model, temperature, etc.)
     * @param response The LLM response to cache
     * @param metadata Optional metadata (model version, token count, etc.)
     * @param ttl_seconds TTL in seconds (0 = use default, -1 = no expiry)
     * @return true if successful
     */
    bool put(
        const std::string& prompt,
        const nlohmann::json& params,
        const std::string& response,
        const nlohmann::json& metadata = {},
        int ttl_seconds = 0
    );

    /**
     * @brief Query the cache for a matching response
     * 
     * @param prompt The prompt text
     * @param params Additional parameters
     * @return std::optional<CacheEntry> The cached entry if found and not expired
     */
    std::optional<CacheEntry> query(
        const std::string& prompt,
        const nlohmann::json& params
    );

    /**
     * @brief Get cache statistics
     * 
     * @return Stats Current cache metrics
     */
    Stats getStats() const;

    /**
     * @brief Clear all expired entries (manual compaction trigger)
     * 
     * @return Number of entries removed
     */
    uint64_t clearExpired();

    /**
     * @brief Clear entire cache
     * 
     * @return true if successful
     */
    bool clear();

private:
    /**
     * @brief Compute cache key from prompt and params
     * 
     * @param prompt The prompt text
     * @param params Additional parameters
     * @return std::string SHA256 hash as hex string
     */
    std::string computeKey(const std::string& prompt, const nlohmann::json& params) const;

    /**
     * @brief Check if an entry is expired
     * 
     * @param entry The cache entry
     * @return true if expired
     */
    bool isExpired(const CacheEntry& entry) const;

    /**
     * @brief Get current timestamp in milliseconds
     */
    int64_t getCurrentTimestampMs() const;

    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_handle_;
    int default_ttl_seconds_;

    // Metrics — all updated from concurrent threads; must be atomic.
    // memory_order_relaxed is sufficient: exact ordering between individual
    // hit/miss increments and latency accumulation is not required for
    // statistical aggregation (no happens-before dependency across counters).
    mutable std::atomic<uint64_t> hit_count_{0};
    mutable std::atomic<uint64_t> miss_count_{0};
    // Accumulated in microseconds (integer) to allow lock-free fetch_add.
    mutable std::atomic<uint64_t> total_query_latency_us_{0};
    // Maintained in put() / clearExpired() / clear() to avoid full RocksDB scans in getStats().
    mutable std::atomic<uint64_t> entry_count_{0};
    mutable std::atomic<uint64_t> total_bytes_{0};

    // F-014: Background expiry thread — periodically sweeps expired entries so
    // callers don't need to invoke clearExpired() manually.
    std::thread            bg_expiry_thread_;
    std::atomic<bool>      bg_stop_{false};
    std::mutex             bg_cv_mutex_;
    std::condition_variable bg_cv_;
};

} // namespace themis
