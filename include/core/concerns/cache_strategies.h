/**
 * @file cache_strategies.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 *
 * Strategy implementations track metadata only; they do not own cache values.
 * The cache calls onAccess()/onInsert()/onRemove() to keep metadata in sync,
 * and selectVictim() to request the next candidate key for eviction.
 */
class IEvictionStrategy {
public:
    virtual ~IEvictionStrategy() = default;

    /**
     * @brief Called when an entry is accessed (get operation).
        *
        * Called only for successful cache lookups in most implementations.
        *
        * @param key The key that was accessed.
     */
    virtual void onAccess(std::string_view key) = 0;

    /**
     * @brief Called when an entry is inserted or updated.
        *
        * @param key The key being inserted or updated.
        * @param timestamp_ms Insertion/update timestamp in milliseconds.
     */
    virtual void onInsert(std::string_view key, uint64_t timestamp_ms) = 0;

    /**
     * @brief Called when an entry is removed from cache.
        *
        * @param key The key being removed.
     */
    virtual void onRemove(std::string_view key) = 0;

    /**
     * @brief Select a victim key for eviction.
        *
        * @return Key to evict, or std::nullopt if no eviction is needed.
     */
    virtual std::optional<std::string> selectVictim() = 0;

    /**
     * @brief Clear all tracking data.
     */
    virtual void clear() = 0;

    /**
     * @brief Get current size of tracked entries.
        * @return Number of live keys currently tracked by the strategy.
     */
    virtual size_t size() const = 0;

    /**
     * @brief Get strategy name for debugging/metrics.
     * @return Stable short strategy identifier (e.g. "LRU", "LFU").
     */
    virtual std::string_view getName() const = 0;
};

/**
 * @brief Metrics collection interface for caches.
 *
 * Provides standard cache metrics that can be exposed to monitoring systems.
 * Values are cumulative for the lifetime of the metrics snapshot unless
 * explicitly reset by the owning cache implementation.
 */
struct CacheMetrics {
    uint64_t hit_count{0};
    uint64_t miss_count{0};
    uint64_t eviction_count{0};
    uint64_t insertion_count{0};
    size_t current_size{0};
    size_t max_size{0};
    uint64_t total_latency_ns{0};  // Cumulative operation latency
    
    /**
     * @brief Compute the hit ratio in the range [0.0, 1.0].
     * @return 0.0 when no lookups were recorded.
     */
    double hitRate() const {
        uint64_t total = hit_count + miss_count;
        return total > 0 ? static_cast<double>(hit_count) / total : 0.0;
    }
    
    /**
     * @brief Compute average lookup latency in nanoseconds.
     * @return 0.0 when no hit/miss events were recorded.
     */
    double avgLatencyNs() const {
        uint64_t total_ops = hit_count + miss_count;
        return total_ops > 0 ? static_cast<double>(total_latency_ns) / total_ops : 0.0;
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
