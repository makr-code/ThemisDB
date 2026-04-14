/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_cache.h                                          ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:21:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     205                                            ║
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

#include "core/concerns/lifecycle.h"
#include <string>
#include <string_view>
#include <optional>
#include <cstdint>
#include <memory>

namespace themis {
namespace core {
namespace concerns {

// Forward declarations
class IEvictionStrategy;
struct CacheMetrics;

/**
 * @brief Value stored in cache with metadata.
 */
struct CacheEntry {
    std::string payload;      // Serialized data
    uint64_t version{0};      // Version number for cache invalidation
    uint64_t timestamp_ms{0}; // Creation/update timestamp

    CacheEntry() = default;
    CacheEntry(std::string data, uint64_t ver = 0, uint64_t ts = 0)
        : payload(std::move(data)), version(ver), timestamp_ms(ts) {}
};

/**
 * @brief Abstract cache interface for dependency injection.
 * 
 * Provides a unified caching interface that can be implemented by various
 * cache backends (in-memory, Redis, Memcached, no-op, etc.).
 * Enables testing with mock caches and runtime switching of implementations.
 */
class ICache {
public:
    virtual ~ICache() = default;

    // -----------------------------------------------------------------------
    // Core cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Retrieve a cache entry by key.
     *
     * @param key Cache key (UTF-8 string view; not required to be NUL-terminated).
     * @return The cached CacheEntry if present and not expired, or std::nullopt
     *         on a cache miss.
     */
    virtual std::optional<CacheEntry> get(std::string_view key) const = 0;

    /**
     * @brief Insert or replace a cache entry.
     *
     * @param key    Cache key.
     * @param entry  Value to store (copied into the cache).
     * @param ttl_ms Entry TTL in milliseconds.  0 means use the default TTL.
     * @return true on success, false if the entry could not be stored (e.g.
     *         the cache is full and no eviction is possible).
     */
    virtual bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) = 0;

    /**
     * @brief Remove a single entry from the cache.
     * @param key Key of the entry to remove.  No-op if not present.
     */
    virtual void invalidate(std::string_view key) = 0;

    /**
     * @brief Remove all entries from the cache.
     */
    virtual void clear() = 0;

    // -----------------------------------------------------------------------
    // Batch operations
    // -----------------------------------------------------------------------

    /**
     * @brief Remove all entries whose keys match a glob-style pattern.
     *
     * Supported wildcard: `*` matches any sequence of characters within the
     * key.  Implementations that do not support patterns may fall back to a
     * full clear() or ignore the call.
     *
     * @param pattern Glob pattern (e.g. `"user:*"` to evict all user entries).
     */
    virtual void invalidatePattern(std::string_view pattern) = 0;

    // -----------------------------------------------------------------------
    // Cache statistics
    // -----------------------------------------------------------------------

    /// @brief Return the current number of entries in the cache.
    virtual size_t size() const = 0;

    /// @brief Return the cumulative number of successful cache hits.
    virtual uint64_t hitCount() const = 0;

    /// @brief Return the cumulative number of cache misses.
    virtual uint64_t missCount() const = 0;

    /**
     * @brief Return the cache hit rate in [0.0, 1.0].
     *
     * Returns 0.0 if no lookups have been performed yet.
     */
    virtual double hitRate() const = 0;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set the maximum number of entries the cache will hold.
     *
     * When the limit is reached the eviction strategy determines which
     * entries are removed to make room.
     *
     * @param maxSize New capacity limit.
     */
    virtual void setMaxSize(size_t maxSize) = 0;

    /**
     * @brief Set the default TTL applied to entries that specify ttl_ms = 0.
     * @param ttl_ms TTL in milliseconds.  0 disables TTL-based expiration.
     */
    virtual void setDefaultTTL(uint64_t ttl_ms) = 0;

    // -----------------------------------------------------------------------
    // Optional extension points
    // -----------------------------------------------------------------------

    /**
     * @brief Return the active eviction strategy, or nullptr if not supported.
     *
     * The strategy object is owned by the cache; the caller must not delete it.
     */
    virtual IEvictionStrategy* getEvictionStrategy() { return nullptr; }

    /// @copydoc getEvictionStrategy()
    virtual const IEvictionStrategy* getEvictionStrategy() const { return nullptr; }

    /**
     * @brief Return detailed cache metrics, or nullptr if not supported.
     *
     * The returned pointer is owned by the cache and remains valid until the
     * cache is destroyed or shutdown() is called.
     */
    virtual const CacheMetrics* getMetrics() const { return nullptr; }

    // Lifecycle hooks
    /**
     * @brief Flush any pending writes to the backing store.
     *
     * For in-memory caches this is a no-op; for write-through or
     * write-behind caches, all dirty entries should be persisted.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the cache and release resources.
     *
     * Implementations should flush pending writes and free connections
     * (e.g. to Redis).  The cache is unusable after this call.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the cache backend is reachable and healthy.
     *
     * @return ProbeResult with ok=true when the cache is operational,
     *         ok=false with a descriptive message otherwise.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

} // namespace concerns
} // namespace core
} // namespace themis
