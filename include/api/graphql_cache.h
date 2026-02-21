/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_cache.h                                    ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:35:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f381961e0  2026-02-20  Add comprehensive production readiness review and validat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>

namespace themis {
namespace graphql {

/**
 * @brief Simple LRU cache with time-based expiration
 * 
 * Thread-safe cache for storing query plans and results.
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
        if (it->second.isExpired(ttl_)) {
            cache_.erase(it);
            stats_.misses++;
            return nullptr;
        }
        
        // Update access info
        it->second.access_count++;
        
        stats_.hits++;
        return std::make_shared<T>(it->second.value);
    }
    
    /**
     * @brief Put a value in the cache
     * @param key Cache key
     * @param value Value to cache
     */
    void put(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Evict if at capacity
        if (cache_.size() >= max_size_ && cache_.find(key) == cache_.end()) {
            evictLRU();
        }
        
        CacheEntry entry;
        entry.value = value;
        entry.created_at = std::chrono::steady_clock::now();
        entry.access_count = 1;
        
        cache_[key] = std::move(entry);
    }
    
    /**
     * @brief Invalidate a cache entry
     */
    void invalidate(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(key);
    }
    
    /**
     * @brief Clear all cache entries
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
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
    void evictLRU() {
        // Find least recently used entry (lowest access count + oldest)
        if (cache_.empty()) return;
        
        auto lru_it = cache_.begin();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.access_count < lru_it->second.access_count ||
                (it->second.access_count == lru_it->second.access_count &&
                 it->second.created_at < lru_it->second.created_at)) {
                lru_it = it;
            }
        }
        
        cache_.erase(lru_it);
    }
    
    size_t max_size_;
    std::chrono::seconds ttl_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CacheEntry> cache_;
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
        size_t depth;
        size_t field_count;
        size_t ast_node_count;
        bool validation_passed;
        
        // Could store the parsed Document here in the future
        // Document parsed_document;
    };
    
    static QueryPlanCache& instance() {
        static QueryPlanCache instance;
        return instance;
    }
    
    /**
     * @brief Get a cached query plan
     */
    std::shared_ptr<QueryPlan> get(const std::string& query) {
        return cache_.get(computeHash(query));
    }
    
    /**
     * @brief Cache a query plan
     */
    void put(const std::string& query, const QueryPlan& plan) {
        cache_.put(computeHash(query), plan);
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
    
    std::string computeHash(const std::string& query) const {
        // Simple hash for now - could use a better hash function
        std::hash<std::string> hasher;
        return std::to_string(hasher(query));
    }
    
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
    };
    
    static ResponseCache& instance() {
        static ResponseCache instance;
        return instance;
    }
    
    /**
     * @brief Get a cached response
     */
    std::shared_ptr<CachedResponse> get(const std::string& query) {
        return cache_.get(computeHash(query));
    }
    
    /**
     * @brief Cache a response
     */
    void put(const std::string& query, const CachedResponse& response) {
        cache_.put(computeHash(query), response);
    }
    
    /**
     * @brief Invalidate responses for a specific collection/type
     */
    void invalidatePattern(const std::string& pattern) {
        // TODO: Implement pattern-based invalidation
        // For now, just clear the entire cache
        cache_.clear();
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
    
    std::string computeHash(const std::string& query) const {
        std::hash<std::string> hasher;
        return std::to_string(hasher(query));
    }
    
    Cache<CachedResponse> cache_;
};

} // namespace graphql
} // namespace themis
