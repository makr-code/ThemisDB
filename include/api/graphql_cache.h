/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_cache.h                                    ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:13:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     316                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • d1b7d6452c  2026-02-22  Code audit bugfixes: eliminate hash collision, add defaul... ║
    • 54d4803715  2026-02-22  Improve GraphQL API layer performance: O(1) LRU, parse ca... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <mutex>
#include <memory>
#include "api/graphql.h"

namespace themis {
namespace graphql {

/**
 * @brief LRU cache with time-based expiration
 *
 * Thread-safe cache for storing query plans and results.
 * Eviction is O(1) using a doubly-linked list to track access order.
 */
template<typename T>
class Cache {
public:
    struct CacheEntry {
        T value;
        std::chrono::steady_clock::time_point created_at;
        size_t access_count = 0;
        
        bool isExpired(std::chrono::seconds ttl) const {
            auto now = std::chrono::steady_clock::now();
            return (now - created_at) > ttl;
        }
    };
    
    /**
     * @brief Create a cache with specified size and TTL
     * @param max_size Maximum number of entries
     * @param ttl Time-to-live for entries in seconds
     */
    Cache(size_t max_size = 1000, std::chrono::seconds ttl = std::chrono::seconds(300))
        : max_size_(max_size), ttl_(ttl) {}
    
    /**
     * @brief Get a value from the cache
     * @param key Cache key
     * @return Pointer to value if found and not expired, nullptr otherwise
     */
    std::shared_ptr<T> get(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it == cache_.end()) {
            stats_.misses++;
            return nullptr;
        }
        
        // Check if expired
        if (it->second.first.isExpired(ttl_)) {
            lru_order_.erase(it->second.second);
            cache_.erase(it);
            stats_.misses++;
            return nullptr;
        }
        
        // Move to front of LRU list (most recently used)
        lru_order_.splice(lru_order_.begin(), lru_order_, it->second.second);
        it->second.first.access_count++;
        
        stats_.hits++;
        return std::make_shared<T>(it->second.first.value);
    }
    
    /**
     * @brief Put a value in the cache
     * @param key Cache key
     * @param value Value to cache
     */
    void put(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            // Update existing entry and move to front
            it->second.first.value = value;
            it->second.first.created_at = std::chrono::steady_clock::now();
            it->second.first.access_count = 1;
            lru_order_.splice(lru_order_.begin(), lru_order_, it->second.second);
            return;
        }
        
        // Evict if at capacity
        if (cache_.size() >= max_size_) {
            evictLRU();
        }
        
        lru_order_.push_front(key);
        
        CacheEntry entry;
        entry.value = value;
        entry.created_at = std::chrono::steady_clock::now();
        entry.access_count = 1;
        
        cache_[key] = {std::move(entry), lru_order_.begin()};
    }
    
    /**
     * @brief Invalidate a cache entry
     */
    void invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            lru_order_.erase(it->second.second);
            cache_.erase(it);
        }
    }
    
    /**
     * @brief Erase all entries for which the predicate returns true
     * @param pred Callable with signature `bool(const T& value)`
     */
    template<typename Predicate>
    void eraseIf(Predicate pred) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = cache_.begin(); it != cache_.end(); ) {
            if (pred(it->second.first.value)) {
                lru_order_.erase(it->second.second);
                it = cache_.erase(it);
            } else {
                ++it;
            }
        }
    }

    /**
     * @brief Clear all cache entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        lru_order_.clear();
        stats_ = CacheStats{};
    }
    
    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        uint64_t hits = 0;
        uint64_t misses = 0;
        
        double hitRate() const {
            uint64_t total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };
    
    CacheStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_;
    }
    
    /**
     * @brief Get current cache size
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }
    
private:
    // Evict the least recently used entry (back of lru_order_). O(1).
    void evictLRU() {
        if (lru_order_.empty()) return;
        const std::string& lru_key = lru_order_.back();
        cache_.erase(lru_key);
        lru_order_.pop_back();
    }
    
    size_t max_size_;
    std::chrono::seconds ttl_;
    mutable std::mutex mutex_;
    // Maps key -> (entry, iterator into lru_order_)
    std::unordered_map<std::string, std::pair<CacheEntry, typename std::list<std::string>::iterator>> cache_;
    std::list<std::string> lru_order_;  // Front = most recently used, back = LRU
    CacheStats stats_;
};

/**
 * @brief Query plan cache for parsed and validated queries
 * 
 * Caches the parsed AST and validation results to avoid
 * re-parsing the same queries.
 */
class QueryPlanCache {
public:
    struct QueryPlan {
        std::string query_hash;
        size_t depth = 0;
        size_t field_count = 0;
        size_t ast_node_count = 0;
        bool validation_passed = false;
        
        Document parsed_document;
    };
    
    static QueryPlanCache& instance() {
        static QueryPlanCache instance;
        return instance;
    }
    
    /**
     * @brief Get a cached query plan
     */
    std::shared_ptr<QueryPlan> get(const std::string& query) {
        return cache_.get(query);
    }
    
    /**
     * @brief Cache a query plan
     */
    void put(const std::string& query, const QueryPlan& plan) {
        cache_.put(query, plan);
    }
    
    /**
     * @brief Get cache statistics
     */
    Cache<QueryPlan>::CacheStats getStats() const {
        return cache_.getStats();
    }
    
    /**
     * @brief Clear the cache
     */
    void clear() {
        cache_.clear();
    }
    
private:
    QueryPlanCache() : cache_(1000, std::chrono::seconds(600)) {}  // 10 minute TTL
    
    Cache<QueryPlan> cache_;
};

/**
 * @brief Response cache for query results
 * 
 * Caches complete query responses with configurable TTL.
 * Useful for read-heavy workloads with relatively static data.
 */
class ResponseCache {
public:
    struct CachedResponse {
        std::string data;           // Serialized response data
        std::string etag;           // ETag for conditional requests
        std::chrono::steady_clock::time_point last_modified;
        std::unordered_set<std::string> collections;  // Collections read by this query
    };
    
    static ResponseCache& instance() {
        static ResponseCache instance;
        return instance;
    }
    
    /**
     * @brief Get a cached response
     */
    std::shared_ptr<CachedResponse> get(const std::string& query) {
        return cache_.get(query);
    }
    
    /**
     * @brief Cache a response
     */
    void put(const std::string& query, const CachedResponse& response) {
        cache_.put(query, response);
    }
    
    /**
     * @brief Invalidate responses for a specific collection/type
     *
     * Only evicts entries whose tag set includes @p pattern, leaving
     * responses that reference other collections untouched.
     */
    void invalidatePattern(const std::string& pattern) {
        cache_.eraseIf([&pattern](const CachedResponse& response) {
            return response.collections.count(pattern) > 0;
        });
    }
    
    /**
     * @brief Get cache statistics
     */
    Cache<CachedResponse>::CacheStats getStats() const {
        return cache_.getStats();
    }
    
    /**
     * @brief Clear the cache
     */
    void clear() {
        cache_.clear();
    }
    
private:
    ResponseCache() : cache_(500, std::chrono::seconds(60)) {}  // 1 minute TTL for responses
    
    Cache<CachedResponse> cache_;
};

} // namespace graphql
} // namespace themis
