/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            evaluation_cache.h                                 ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:25:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     219                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file evaluation_cache.h
 * @brief Advanced evaluation cache for RAG Judge
 * 
 * Phase 6: Implements LRU cache with TTL, cache warming, and invalidation
 */

#pragma once

#include "rag/rag_judge.h"
#include <unordered_map>
#include <list>
#include <chrono>
#include <mutex>
#include <functional>

namespace themis::rag::judge {

/**
 * @brief Cache entry with metadata
 */
struct CacheEntry {
    EvaluationResult result;
    std::chrono::system_clock::time_point timestamp;
    size_t access_count;
    double confidence;
};

/**
 * @brief Cache invalidation trigger
 */
enum class InvalidationTrigger {
    MODEL_UPDATE,        ///< LLM model was updated
    CONFIG_CHANGE,       ///< Configuration changed
    MANUAL,              ///< Manual purge requested
    TTL_EXPIRED,         ///< Entry exceeded TTL
    CAPACITY_EXCEEDED    ///< Cache full, LRU eviction
};

/**
 * @brief Cache statistics
 */
struct CacheStatistics {
    size_t total_requests;
    size_t cache_hits;
    size_t cache_misses;
    double hit_rate;
    size_t current_size;
    size_t max_size;
    size_t evictions;
    size_t invalidations;
    
    std::chrono::milliseconds average_lookup_time;
    std::chrono::system_clock::time_point last_reset;
};

/**
 * @brief Configuration for evaluation cache
 */
struct CacheConfig {
    size_t max_entries = 1000;                   ///< Maximum cache entries
    std::chrono::seconds ttl = std::chrono::seconds(3600); ///< Time-to-live
    bool enable_warming = false;                  ///< Enable cache warming
    bool enable_auto_invalidation = true;         ///< Auto-invalidate on config change
    
    // Cache warming settings
    std::vector<std::string> warm_queries;        ///< Queries to pre-compute
    std::chrono::seconds warming_interval = std::chrono::seconds(300);
};

/**
 * @brief Advanced LRU cache with TTL for evaluation results
 * 
 * Features:
 * - LRU eviction policy
 * - TTL-based expiration
 * - Cache warming for common queries
 * - Thread-safe operations
 * - Detailed statistics tracking
 * - Multiple invalidation triggers
 */
class EvaluationCache {
public:
    /**
     * @brief Construct evaluation cache
     * @param config Cache configuration
     */
    EvaluationCache();
    explicit EvaluationCache(const CacheConfig& config);
    
    /**
     * @brief Destructor
     */
    ~EvaluationCache();
    
    /**
     * @brief Get cached evaluation result
     * @param query Query string
     * @param answer Answer string
     * @return Cached result if found, nullptr otherwise
     */
    const EvaluationResult* get(const std::string& query, const std::string& answer);
    
    /**
     * @brief Put evaluation result in cache
     * @param query Query string
     * @param answer Answer string
     * @param result Evaluation result to cache
     */
    void put(const std::string& query, const std::string& answer, const EvaluationResult& result);
    
    /**
     * @brief Check if entry exists in cache
     * @param query Query string
     * @param answer Answer string
     * @return true if cached
     */
    bool contains(const std::string& query, const std::string& answer);
    
    /**
     * @brief Clear entire cache
     */
    void clear();
    
    /**
     * @brief Invalidate cache entries based on trigger
     * @param trigger Invalidation trigger
     * @param metadata Optional metadata for selective invalidation
     */
    void invalidate(InvalidationTrigger trigger, const std::string& metadata = "");
    
    /**
     * @brief Warm cache with pre-computed evaluations
     * @param judge Judge instance to use for warming
     * @param queries Queries to pre-compute
     */
    void warmCache(RAGJudge& judge, const std::vector<EvaluationInput>& queries);
    
    /**
     * @brief Get cache statistics
     * @return Current cache statistics
     */
    CacheStatistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set cache configuration
     * @param config New configuration
     */
    void setConfig(const CacheConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    CacheConfig getConfig() const;
    
    /**
     * @brief Register invalidation callback
     * 
     * Called when cache is invalidated. Useful for logging/monitoring.
     * 
     * @param callback Callback function
     */
    void registerInvalidationCallback(
        std::function<void(InvalidationTrigger, size_t)> callback
    );

private:
    CacheConfig config_;
    mutable std::mutex mutex_;
    
    // LRU cache implementation
    using CacheKey = std::string;  // Combined query + answer hash
    std::unordered_map<CacheKey, CacheEntry> cache_;
    std::list<CacheKey> lru_list_;  // Most recently used at front
    std::unordered_map<CacheKey, std::list<CacheKey>::iterator> lru_map_;
    
    // Statistics
    CacheStatistics stats_;
    
    // Callbacks
    std::function<void(InvalidationTrigger, size_t)> invalidation_callback_;
    
    // Helper methods
    CacheKey computeKey(const std::string& query, const std::string& answer);
    bool isExpired(const CacheEntry& entry) const;
    void evictLRU();
    void updateLRU(const CacheKey& key);
    void removeFromLRU(const CacheKey& key);
};

} // namespace themis::rag::judge
