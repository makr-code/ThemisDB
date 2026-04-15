/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_strategies.h                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:06:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     113                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <memory>
#include <optional>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Eviction strategy interface for cache implementations.
 * 
 * Defines the contract for cache eviction policies (LRU, LFU, TTL, etc.).
 * Allows caches to swap eviction strategies at runtime or compile-time.
 */
class IEvictionStrategy {
public:
    virtual ~IEvictionStrategy() = default;

    /**
     * @brief Called when an entry is accessed (get operation).
     * @param key The key that was accessed
     */
    virtual void onAccess(std::string_view key) = 0;

    /**
     * @brief Called when an entry is inserted or updated.
     * @param key The key being inserted/updated
     * @param timestamp_ms Timestamp of insertion
     */
    virtual void onInsert(std::string_view key, uint64_t timestamp_ms) = 0;

    /**
     * @brief Called when an entry is removed from cache.
     * @param key The key being removed
     */
    virtual void onRemove(std::string_view key) = 0;

    /**
     * @brief Select a victim key for eviction.
     * @return Key to evict, or empty if no eviction needed
     */
    virtual std::optional<std::string> selectVictim() = 0;

    /**
     * @brief Clear all tracking data.
     */
    virtual void clear() = 0;

    /**
     * @brief Get current size of tracked entries.
     */
    virtual size_t size() const = 0;

    /**
     * @brief Get strategy name for debugging/metrics.
     */
    virtual std::string_view getName() const = 0;
};

/**
 * @brief Metrics collection interface for caches.
 * 
 * Provides standard cache metrics that can be exposed to monitoring systems.
 */
struct CacheMetrics {
    uint64_t hit_count{0};
    uint64_t miss_count{0};
    uint64_t eviction_count{0};
    uint64_t insertion_count{0};
    size_t current_size{0};
    size_t max_size{0};
    uint64_t total_latency_ns{0};  // Cumulative operation latency
    
    double hitRate() const {
        uint64_t total = hit_count + miss_count;
        return total > 0 ? static_cast<double>(hit_count) / total : 0.0;
    }
    
    double avgLatencyNs() const {
        uint64_t total_ops = hit_count + miss_count;
        return total_ops > 0 ? static_cast<double>(total_latency_ns) / total_ops : 0.0;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
