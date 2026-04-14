/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bounded_lru_cache.h                                ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:50:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     188                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
class BoundedLRUCache {
public:
    /**
     * @brief Configuration for the cache
     */
    struct Config {
        size_t max_entries = 100'000;           // Maximum entries in cache
        std::chrono::seconds ttl{3600};         // Time-to-live (1 hour default)
        bool enable_statistics = true;          // Track hits/misses
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
    std::optional<nlohmann::json> get(const std::string& key);
    
    /**
     * @brief Put value with TTL
     * @param key Key to store
     * @param value Value to store
     */
    void put(const std::string& key, const nlohmann::json& value);
    
    /**
     * @brief Remove entry from cache
     * @param key Key to remove
     * @return true if entry was found and removed
     */
    bool remove(const std::string& key);
    
    /**
     * @brief Evict LRU entry if at capacity
     * @return true if an entry was evicted
     */
    bool evictLRUIfNeeded();
    
    /**
     * @brief Cache statistics
     */
    struct Statistics {
        size_t current_size;
        size_t hits;
        size_t misses;
        
        double hit_ratio() const {
            size_t total = hits + misses;
            if (total == 0) return 0.0;
            return static_cast<double>(hits) / static_cast<double>(total);
        }
    };
    
    /**
     * @brief Get cache statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Clear all entries
     */
    void clear();
    
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
