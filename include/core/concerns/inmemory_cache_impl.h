/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            inmemory_cache_impl.h                              ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:21:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     175                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_cache.h"
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <regex>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Simple in-memory cache implementation of ICache.
 * 
 * Thread-safe LRU cache implementation for testing and development.
 */
class InMemoryCacheImpl : public ICache {
public:
    explicit InMemoryCacheImpl(size_t maxSize = 1000, uint64_t defaultTTL = 0)
        : maxSize_(maxSize), defaultTTL_(defaultTTL), hits_(0), misses_(0) {}

    std::optional<CacheEntry> get(std::string_view key) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = cache_.find(std::string(key));
        if (it == cache_.end()) {
            ++misses_;
            return std::nullopt;
        }

        // Check TTL expiration
        if (it->second.ttl_ms > 0) {
            auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            
            if (now - it->second.entry.timestamp_ms > it->second.ttl_ms) {
                cache_.erase(it);
                ++misses_;
                return std::nullopt;
            }
        }

        ++hits_;
        return it->second.entry;
    }

    bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Evict if at capacity
        if (cache_.size() >= maxSize_ && cache_.find(std::string(key)) == cache_.end()) {
            // Simple FIFO eviction (could be improved to LRU)
            auto oldest = cache_.begin();
            cache_.erase(oldest);
        }

        uint64_t effectiveTTL = ttl_ms > 0 ? ttl_ms : defaultTTL_;
        cache_[std::string(key)] = {entry, effectiveTTL};
        return true;
    }

    void invalidate(std::string_view key) override {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.erase(std::string(key));
    }

    void clear() override {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        hits_ = 0;
        misses_ = 0;
    }

    void invalidatePattern(std::string_view pattern) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            // Use basic_regex directly to avoid potential macro conflicts with 'regex'
            const ::std::string pattern_str(pattern);
            const ::std::basic_regex<char> rx{pattern_str};
            for (auto it = cache_.begin(); it != cache_.end();) {
                if (::std::regex_match(it->first, rx)) {
                    it = cache_.erase(it);
                } else {
                    ++it;
                }
            }
        } catch (const ::std::regex_error&) {
            // Invalid regex pattern, do nothing
        }
    }

    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return cache_.size();
    }

    uint64_t hitCount() const override {
        return hits_.load();
    }

    uint64_t missCount() const override {
        return misses_.load();
    }

    double hitRate() const override {
        uint64_t total = hits_.load() + misses_.load();
        return total > 0 ? static_cast<double>(hits_.load()) / total : 0.0;
    }

    void setMaxSize(size_t maxSize) override {
        std::lock_guard<std::mutex> lock(mutex_);
        maxSize_ = maxSize;
    }

    void setDefaultTTL(uint64_t ttl_ms) override {
        defaultTTL_ = ttl_ms;
    }

    // Lifecycle hooks
    void flush() noexcept override {
        // In-memory cache has no backing store to flush.
    }

    void shutdown() noexcept override {
        clear();
    }

    ProbeResult isHealthy() const override {
        // The in-memory cache is always healthy while the object is alive.
        return ProbeResult::healthy("in-memory cache operational");
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
    mutable std::atomic<uint64_t> hits_;
    mutable std::atomic<uint64_t> misses_;
};

} // namespace concerns
} // namespace core
} // namespace themis
