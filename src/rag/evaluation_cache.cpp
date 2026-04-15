/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            evaluation_cache.cpp                               ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:50:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     308                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 6efaebce20  2026-03-09  feat(rag): implement BatchEvaluator, CalibrationManager, ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file evaluation_cache.cpp
 * @brief LRU evaluation cache with TTL for RAG Judge results
 *
 * Implements an LRU cache with TTL-based expiration, thread-safe access,
 * and detailed statistics tracking.
 */

#include "rag/evaluation_cache.h"
#include "utils/logger.h"

#include <functional>
#include <sstream>

namespace themis::rag::judge {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

EvaluationCache::EvaluationCache()
    : config_{} {
    stats_.last_reset = std::chrono::system_clock::now();
}

EvaluationCache::EvaluationCache(const CacheConfig& config)
    : config_(config) {
    stats_.last_reset = std::chrono::system_clock::now();
}

EvaluationCache::~EvaluationCache() = default;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

EvaluationCache::CacheKey EvaluationCache::computeKey(
    const std::string& query, const std::string& answer) {
    // Simple deterministic key: hash query+answer concatenation
    std::size_t h1 = std::hash<std::string>{}(query);
    std::size_t h2 = std::hash<std::string>{}(answer);
    // Combine hashes with a mixing constant (golden ratio)
    std::size_t combined = h1 ^ (h2 * 0x9e3779b9ULL + (h1 << 6) + (h1 >> 2));
    std::ostringstream oss;
    oss << combined;
    return oss.str();
}

bool EvaluationCache::isExpired(const CacheEntry& entry) const {
    auto age = std::chrono::system_clock::now() - entry.timestamp;
    return age > config_.ttl;
}

void EvaluationCache::evictLRU() {
    if (lru_list_.empty()) return;

    // The back of lru_list_ holds the least-recently-used key.
    const CacheKey& lru_key = lru_list_.back();

    cache_.erase(lru_key);
    lru_map_.erase(lru_key);
    lru_list_.pop_back();

    ++stats_.evictions;
}

void EvaluationCache::updateLRU(const CacheKey& key) {
    auto it = lru_map_.find(key);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }
    lru_list_.push_front(key);
    lru_map_[key] = lru_list_.begin();
}

void EvaluationCache::removeFromLRU(const CacheKey& key) {
    auto it = lru_map_.find(key);
    if (it != lru_map_.end()) {
        lru_list_.erase(it->second);
        lru_map_.erase(it);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

const EvaluationResult* EvaluationCache::get(
    const std::string& query, const std::string& answer) {
    std::lock_guard<std::mutex> lock(mutex_);

    ++stats_.total_requests;

    const auto start = std::chrono::steady_clock::now();

    const CacheKey key = computeKey(query, answer);
    auto it = cache_.find(key);

    if (it == cache_.end()) {
        ++stats_.cache_misses;
        return nullptr;
    }

    // Check TTL expiry
    if (isExpired(it->second)) {
        removeFromLRU(key);
        cache_.erase(it);
        ++stats_.cache_misses;
        ++stats_.invalidations;
        return nullptr;
    }

    // Cache hit: promote to front of LRU
    updateLRU(key);
    ++it->second.access_count;
    ++stats_.cache_hits;

    const auto elapsed = std::chrono::steady_clock::now() - start;
    stats_.average_lookup_time =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);

    // Update hit rate
    stats_.hit_rate = static_cast<double>(stats_.cache_hits) /
                      static_cast<double>(stats_.total_requests);

    return &it->second.result;
}

void EvaluationCache::put(
    const std::string& query,
    const std::string& answer,
    const EvaluationResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);

    const CacheKey key = computeKey(query, answer);

    // If key already present, update in place
    auto existing = cache_.find(key);
    if (existing != cache_.end()) {
        existing->second.result       = result;
        existing->second.timestamp    = std::chrono::system_clock::now();
        existing->second.access_count = 0;
        existing->second.confidence   = result.confidence;
        updateLRU(key);
        return;
    }

    // Evict LRU entry if at capacity
    while (cache_.size() >= config_.max_entries) {
        evictLRU();
    }

    CacheEntry entry;
    entry.result       = result;
    entry.timestamp    = std::chrono::system_clock::now();
    entry.access_count = 0;
    entry.confidence   = result.confidence;

    cache_.emplace(key, std::move(entry));
    updateLRU(key);

    stats_.current_size = cache_.size();
    stats_.max_size     = std::max(stats_.max_size, stats_.current_size);
}

bool EvaluationCache::contains(
    const std::string& query, const std::string& answer) {
    std::lock_guard<std::mutex> lock(mutex_);

    const CacheKey key = computeKey(query, answer);
    auto it = cache_.find(key);
    if (it == cache_.end()) return false;
    if (isExpired(it->second)) {
        removeFromLRU(key);
        cache_.erase(it);
        return false;
    }
    return true;
}

void EvaluationCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    const size_t old_size = cache_.size();
    cache_.clear();
    lru_list_.clear();
    lru_map_.clear();

    stats_.current_size = 0;
    stats_.invalidations += old_size;

    THEMIS_INFO("EvaluationCache: cleared {} entries", old_size);

    if (invalidation_callback_) {
        invalidation_callback_(InvalidationTrigger::MANUAL, old_size);
    }
}

void EvaluationCache::invalidate(
    InvalidationTrigger trigger, const std::string& /*metadata*/) {
    std::lock_guard<std::mutex> lock(mutex_);

    const size_t old_size = cache_.size();

    if (trigger == InvalidationTrigger::TTL_EXPIRED) {
        // Sweep expired entries only
        size_t removed = 0;
        for (auto it = cache_.begin(); it != cache_.end(); ) {
            if (isExpired(it->second)) {
                removeFromLRU(it->first);
                it = cache_.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        stats_.invalidations += removed;
        stats_.current_size = cache_.size();
        THEMIS_INFO("EvaluationCache: TTL sweep removed {} expired entries", removed);
    } else {
        // Full invalidation for MODEL_UPDATE, CONFIG_CHANGE, MANUAL
        cache_.clear();
        lru_list_.clear();
        lru_map_.clear();
        stats_.invalidations += old_size;
        stats_.current_size = 0;
        THEMIS_INFO("EvaluationCache: full invalidation, removed {} entries (trigger={})",
                    old_size, static_cast<int>(trigger));
    }

    if (invalidation_callback_) {
        invalidation_callback_(trigger, old_size);
    }
}

void EvaluationCache::warmCache(
    RAGJudge& judge, const std::vector<EvaluationInput>& queries) {
    THEMIS_INFO("EvaluationCache: warming cache with {} queries", queries.size());

    for (const auto& input : queries) {
        // Skip if already cached
        if (contains(input.query, input.generated_answer)) continue;

        auto result = judge.evaluate(input);
        put(input.query, input.generated_answer, result);
    }

    THEMIS_INFO("EvaluationCache: warm-up complete, cache size={}", cache_.size());
}

CacheStatistics EvaluationCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    CacheStatistics s = stats_;
    s.current_size = cache_.size();
    if (s.total_requests > 0) {
        s.hit_rate = static_cast<double>(s.cache_hits) /
                     static_cast<double>(s.total_requests);
    }
    return s;
}

void EvaluationCache::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = CacheStatistics{};
    stats_.last_reset = std::chrono::system_clock::now();
    stats_.current_size = cache_.size();
}

void EvaluationCache::setConfig(const CacheConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

CacheConfig EvaluationCache::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void EvaluationCache::registerInvalidationCallback(
    std::function<void(InvalidationTrigger, size_t)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    invalidation_callback_ = std::move(callback);
}

} // namespace themis::rag::judge
