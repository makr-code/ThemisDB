/*
 * ThemisDB | File: i_cache.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 89/100 | Lines: 190
 * Open Issues: TODOs=1, Stubs=1, Gaps=4, Unimpl=0, Mock=2, Sim=0, Debt=0
 * Gap Correlation: internal=4 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #869 Cache abstraction: Unified ICache interface with pluggable eviction... (2026-03-11T21:56:12Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    [[nodiscard]] virtual std::optional<CacheEntry> get(std::string_view key) const = 0;

    /**
     * @brief Insert or replace a cache entry.
     *
     * @param key    Cache key.
     * @param entry  Value to store (copied into the cache).
     * @param ttl_ms Entry TTL in milliseconds.  0 means use the default TTL.
     * @return true on success, false if the entry could not be stored (e.g.
     *         the cache is full and no eviction is possible).
     */
    [[nodiscard]] virtual bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) = 0;

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
    [[nodiscard]] virtual size_t size() const = 0;

    /// @brief Return the cumulative number of successful cache hits.
    [[nodiscard]] virtual uint64_t hitCount() const = 0;

    /// @brief Return the cumulative number of cache misses.
    [[nodiscard]] virtual uint64_t missCount() const = 0;

    /**
     * @brief Return the cache hit rate in [0.0, 1.0].
     *
     * Returns 0.0 if no lookups have been performed yet.
     */
    [[nodiscard]] virtual double hitRate() const = 0;

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
