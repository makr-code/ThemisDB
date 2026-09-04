/**
 * @file strategic_cache_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_cache.h"
#include "core/concerns/cache_strategies.h"
#include "core/concerns/eviction_strategies.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <regex>
#include <memory>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Strategic in-memory cache with pluggable eviction strategies.
 *
 * Implements ICache with support for swappable eviction strategies (LRU, LFU, TTL, TwoTier).
 * Thread-safe implementation with comprehensive metrics tracking.
 *
 * This cache stores values in-process and delegates victim selection to the
 * active strategy. Metric counters are cumulative and intentionally not reset
 * by clear().
 *
 * Example usage:
 *   auto cache = std::make_unique<StrategicCacheImpl>(
 *       1000,  // max size
 *       std::make_unique<LRUEvictionStrategy>()
 *   );
 */
class StrategicCacheImpl : public ICache {
public:
    /**
     * @brief Construct with custom eviction strategy.
        * @param maxSize Maximum number of entries.
        * @param strategy Eviction strategy (takes ownership). When null, LRU is used.
        * @param defaultTTL Default TTL in milliseconds (0 = no expiry).
     */
    explicit StrategicCacheImpl(
        size_t maxSize = 1000,
        std::unique_ptr<IEvictionStrategy> strategy = nullptr,
        uint64_t defaultTTL = 0
    ) : maxSize_(maxSize), 
        defaultTTL_(defaultTTL),
        strategy_(strategy ? std::move(strategy) : std::make_unique<LRUEvictionStrategy>()) {
        
        metrics_.max_size = maxSize;
    }

    std::optional<CacheEntry> get(std::string_view key) const override {
        auto start = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(std::string(key));
        if (it == cache_.end()) {
            metrics_.miss_count++;
            updateLatency(start);
            return std::nullopt;
        }

        // Check TTL expiration
        if (it->second.ttl_ms > 0) {
            auto now = getCurrentTimeMs();
            
            if (now - it->second.entry.timestamp_ms > it->second.ttl_ms) {
                strategy_->onRemove(key);
                cache_.erase(it);
                metrics_.miss_count++;
                updateLatency(start);
                return std::nullopt;
            }
        }

        // Track access in strategy
        strategy_->onAccess(key);
        
        metrics_.hit_count++;
        metrics_.current_size = cache_.size();
        updateLatency(start);
        return it->second.entry;
    }

    bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) override {
        auto start = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key_str(key);
        
        // Check if key already exists
        bool is_update = cache_.find(key_str) != cache_.end();
        
        // Evict if at capacity and this is a new entry
        if (!is_update && cache_.size() >= maxSize_) {
            auto victim = strategy_->selectVictim();
            if (victim) {
                cache_.erase(*victim);
                strategy_->onRemove(*victim);
                metrics_.eviction_count++;
            }
        }

        uint64_t effectiveTTL = ttl_ms > 0 ? ttl_ms : defaultTTL_;
        
        // Update cache
        cache_[key_str] = {entry, effectiveTTL};
        
        // Track in strategy
        strategy_->onInsert(key, entry.timestamp_ms > 0 ? entry.timestamp_ms : getCurrentTimeMs());
        
        if (!is_update) {
            metrics_.insertion_count++;
        }
        metrics_.current_size = cache_.size();
        updateLatency(start);
        return true;
    }

    void invalidate(std::string_view key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(std::string(key));
        if (it != cache_.end()) {
            strategy_->onRemove(key);
            cache_.erase(it);
            metrics_.current_size = cache_.size();
        }
    }

    void clear() override {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        strategy_->clear();
        metrics_.current_size = 0;
        // Don't reset counters - they're cumulative metrics
    }

    void invalidatePattern(std::string_view pattern) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            std::regex pattern_regex = std::regex(std::string(pattern));
            std::smatch match_obj = {};
            for (auto it = cache_.begin(); it != cache_.end();) {
                if (std::regex_match(it->first, match_obj, pattern_regex)) {
                    strategy_->onRemove(it->first);
                    it = cache_.erase(it);
                } else {
                    ++it;
                }
            }
            metrics_.current_size = cache_.size();
        } catch (const std::regex_error&) {
            // Invalid regex pattern, do nothing
        }
    }

    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    uint64_t hitCount() const override {
        return metrics_.hit_count;
    }

    uint64_t missCount() const override {
        return metrics_.miss_count;
    }

    double hitRate() const override {
        uint64_t total = metrics_.hit_count + metrics_.miss_count;
        return total > 0 ? static_cast<double>(metrics_.hit_count) / total : 0.0;
    }

    void setMaxSize(size_t maxSize) override {
        std::lock_guard<std::mutex> lock(mutex_);
        maxSize_ = maxSize;
        metrics_.max_size = maxSize;
    }

    void setDefaultTTL(uint64_t ttl_ms) override {
        defaultTTL_ = ttl_ms;
    }

    // Strategy pattern support
    IEvictionStrategy* getEvictionStrategy() override {
        return strategy_.get();
    }
    const IEvictionStrategy* getEvictionStrategy() const override {
        return strategy_.get();
    }

    // Metrics support
    const CacheMetrics* getMetrics() const override {
        return &metrics_;
    }
    
    /**
     * @brief Replace eviction strategy at runtime.
     *
     * Existing cache entries are kept, and strategy metadata is rebuilt from
     * the current key set. Passing nullptr is invalid.
     *
     * @param strategy New strategy (takes ownership), must not be nullptr.
     */
    void setEvictionStrategy(std::unique_ptr<IEvictionStrategy> strategy) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Clear old strategy tracking
        strategy_->clear();
        
        // Set new strategy
        strategy_ = std::move(strategy);
        
        // Rebuild tracking for existing entries
        for (const auto& [key, cached_value] : cache_) {
            strategy_->onInsert(key, cached_value.entry.timestamp_ms);
        }
    }

private:
    struct CachedValue {
        CacheEntry entry;
        uint64_t ttl_ms;
    };

    mutable std::mutex mutex_;
    mutable std::unordered_map<std::string, CachedValue> cache_;
    size_t maxSize_;
    uint64_t defaultTTL_;
    mutable std::unique_ptr<IEvictionStrategy> strategy_;
    mutable CacheMetrics metrics_;

    uint64_t getCurrentTimeMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
    }
    
    void updateLatency(const std::chrono::steady_clock::time_point& start) const {
        auto end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        metrics_.total_latency_ns += latency;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
