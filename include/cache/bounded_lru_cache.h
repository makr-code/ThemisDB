/**
 * @file bounded_lru_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <atomic>
#include <nlohmann/json.hpp>
#include "cache/cache_interfaces.h"

namespace themis {
namespace cache {

class BoundedLRUCache : public ICacheBackend<std::string, nlohmann::json> {
public:
    struct Config {
        size_t max_entries = 100'000;           // Maximum entries in cache
        std::chrono::seconds ttl{3600};         // Time-to-live (1 hour default)
        bool enable_statistics = true;          // Track hits/misses

        size_t   max_entry_size_bytes = 67108864U; // 64 MiB

        uint32_t max_ttl_seconds      = 86400U;    // 24 hours
    };
    
    /**
     * @brief Bounded LRUCache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit BoundedLRUCache(const Config& config);
    
    ~BoundedLRUCache();
    
    std::optional<nlohmann::json> get(const std::string& key) override;
    
    void put(const std::string& key, nlohmann::json value, uint32_t ttl_seconds = 0) override;
    
    bool remove(const std::string& key) override;

    bool contains(const std::string& key) const override;

    void clear() override;

    std::size_t size() const override;

    /**
     * @brief Evict LRUIf Needed.
     * @return True when the operation succeeds.
     */
    bool evictLRUIfNeeded();

    struct Statistics {
        size_t current_size = 0;
        size_t hits = {};
        size_t misses = {};
        
        double hit_ratio() const {
            size_t total = hits + misses;
            if (total == 0) {
              return 0.0;
            }
            return static_cast<double>(hits) / static_cast<double>(total);
        }
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    
private:
    struct CacheEntry {
        nlohmann::json value;
        std::chrono::steady_clock::time_point expiry;
        std::chrono::steady_clock::time_point last_access;
    };
    
    struct Node {
        std::string key;
        CacheEntry entry;
        std::shared_ptr<Node> prev;
        std::shared_ptr<Node> next;
    };
    
    /**
     * @brief Move To Front.
     * @param[in] node Input parameter.
     */
    void moveToFront(std::shared_ptr<Node> node);
    
    /**
     * @brief Remove Node.
     * @param[in] node Input parameter.
     */
    void removeNode(std::shared_ptr<Node> node);
    
    /**
     * @brief Add To Front.
     * @param[in] node Input parameter.
     */
    void addToFront(std::shared_ptr<Node> node);
    
    /**
     * @brief Remove LRU.
     */
    void removeLRU();
    
    /**
     * @brief Is Expired.
     * @param[in] entry Input parameter.
     * @return True when the operation succeeds.
     */
    bool isExpired(const CacheEntry& entry) const;
    
    // LRU list (most recent at front)
    std::shared_ptr<Node> head_;
    std::shared_ptr<Node> tail_;
    
    // Hash map for O(1) lookup
    std::unordered_map<std::string, std::shared_ptr<Node>> cache_;
    
    Config config_;
    mutable std::shared_mutex mutex_;
    
    std::atomic<uint64_t> hits_{0};
    std::atomic<uint64_t> misses_{0};
};

} // namespace cache
} // namespace themis
