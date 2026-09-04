/**
 * @file bounded_lru_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Bounded LRU Cache with TTL support
 * 
 * Thread-safe LRU cache with:
 * - Configurable maximum size
 * - LRU eviction when capacity is reached
 * - TTL-based expiration
 * - Hit/miss statistics for monitoring
 */
class BoundedLRUCache : public ICacheBackend<std::string, nlohmann::json> {
public:
    /**
     * @brief Configuration for the cache
     */
    struct Config {
        size_t max_entries = 100'000;           // Maximum entries in cache
        std::chrono::seconds ttl{3600};         // Time-to-live (1 hour default)
        bool enable_statistics = true;          // Track hits/misses

        /// @brief C4: AI/LLM safety — maximum serialised entry size in bytes.
        ///        Entries whose JSON dump exceeds this limit are rejected.
        ///        Default: 64 MiB.
        size_t   max_entry_size_bytes = 67108864U; // 64 MiB

        /// @brief C4: AI/LLM safety — maximum per-entry TTL in seconds.
        ///        Entries with ttl_seconds > this value are rejected.
        ///        Default: 86 400 s (24 hours).
        uint32_t max_ttl_seconds      = 86400U;    // 24 hours
    };
    
    /**
     * @brief Constructor
     * @param config Cache configuration
     */
    explicit BoundedLRUCache(const Config& config);
    
    /**
     * @brief Destructor
     */
    ~BoundedLRUCache();
    
    /**
     * @brief Get value with TTL check
     * @param key Key to retrieve
     * @return Value if present and not expired, nullopt otherwise
     */
    std::optional<nlohmann::json> get(const std::string& key) override;
    
    /**
     * @brief Put value with optional per-entry TTL
     * @param key        Cache key
     * @param value      Value to store
     * @param ttl_seconds Per-entry TTL in seconds; 0 = use Config::ttl
     */
    void put(const std::string& key, nlohmann::json value, uint32_t ttl_seconds = 0) override;
    
    /**
     * @brief Remove entry from cache
     * @param key Key to remove
     * @return true if entry was found and removed
     */
    bool remove(const std::string& key) override;

    /**
     * @brief Check whether @p key is present (and not expired) without touching LRU order.
     */
    bool contains(const std::string& key) const override;

    /**
     * @brief Clear all entries
     */
    void clear() override;

    /**
     * @brief Return the number of entries currently in the cache.
     */
    std::size_t size() const override;

    /**
     * @brief Evict LRU entry if at capacity
     * @return true if an entry was evicted
     */
    bool evictLRUIfNeeded();

    /**
     * @brief Cache statistics
     */
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
     * @brief Get cache statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
private:
    /**
     * @brief Cache entry with metadata
     */
    struct CacheEntry {
        nlohmann::json value;
        std::chrono::steady_clock::time_point expiry;
        std::chrono::steady_clock::time_point last_access;
    };
    
    /**
     * @brief Node in the LRU doubly-linked list
     */
    struct Node {
        std::string key;
        CacheEntry entry;
        std::shared_ptr<Node> prev;
        std::shared_ptr<Node> next;
    };
    
    /**
     * @brief Move node to front (most recently used)
     */
    void moveToFront(std::shared_ptr<Node> node);
    
    /**
     * @brief Remove node from list
     */
    void removeNode(std::shared_ptr<Node> node);
    
    /**
     * @brief Add node to front
     */
    void addToFront(std::shared_ptr<Node> node);
    
    /**
     * @brief Remove LRU node (tail)
     */
    void removeLRU();
    
    /**
     * @brief Check if entry is expired
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
