/**
 * @file semantic_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class SemanticCache {
public:
    struct CacheEntry {
        std::string response;
        nlohmann::json metadata;
        int64_t timestamp_ms;
        int ttl_seconds;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static std::optional<CacheEntry> fromJson(const nlohmann::json& j);
    };

    struct Stats {
        uint64_t hit_count = 0;
        uint64_t miss_count = 0;
        uint64_t total_entries = 0;
        uint64_t total_size_bytes = 0;
        double hit_rate = 0.0;
        double avg_latency_ms = 0.0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        nlohmann::json toJson() const;
    };

    SemanticCache(
    rocksdb::TransactionDB* db,
        rocksdb::ColumnFamilyHandle* cf_handle,
        int default_ttl_seconds = 3600,
        int bg_expiry_interval_s = 300
    );

    ~SemanticCache();

    bool put(
        const std::string& prompt,
        const nlohmann::json& params,
        const std::string& response,
        const nlohmann::json& metadata = {},
        int ttl_seconds = 0
    );

    /**
     * @brief Query.
     * @param[in] prompt Input parameter.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    std::optional<CacheEntry> query(
        const std::string& prompt,
        const nlohmann::json& params
    );

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    /**
     * @brief Clear Expired.
     * @return Return value.
     */
    uint64_t clearExpired();

    /**
     * @brief Clear.
     * @return True when the operation succeeds.
     */
    bool clear();

private:
    /**
     * @brief Compute Key.
     * @param[in] prompt Input parameter.
     * @param[in] params Input parameter.
     * @return Return value.
     */
    std::string computeKey(const std::string& prompt, const nlohmann::json& params) const;

    /**
     * @brief Is Expired.
     * @param[in] entry Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExpired(const CacheEntry& entry) const;

    /**
     * @brief Get Current Timestamp Ms.
     * @return Return value.
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
