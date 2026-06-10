/**
 * @file i_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 *
 * The payload is an opaque serialized blob owned by the caller at insertion
 * time and by the cache after copying. Version and timestamp are exposed so
 * higher-level code can implement invalidation and staleness policies without
 * needing backend-specific metadata.
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
 * Implementations may choose to provide strong or best-effort consistency,
 * but must document the behavior of misses, TTL expiry, and shutdown.
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
        * @note Implementations may treat backend connection failures as misses
        *       instead of throwing, but they should document that choice.
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
        * @note TTL expiration semantics are backend-specific only in terms of
        *       precision; expired entries must not be returned by get().
     */
    [[nodiscard]] virtual bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) = 0;

    /**
     * @brief Remove a single entry from the cache.
        *
        * @param key Key of the entry to remove. No-op if not present.
     */
    virtual void invalidate(std::string_view key) = 0;

    /**
     * @brief Remove all entries from the cache.
        *
        * Implementations should prefer a best-effort full clear over silently
        * leaving the cache in a partially invalidated state.
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
        * @note Pattern matching behavior should be documented if the backend does
        *       not use Redis-style glob semantics.
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
        * @note A value of zero may mean unlimited capacity or backend-defined
        *       behavior; implementations must document which applies.
     */
    virtual void setMaxSize(size_t maxSize) = 0;

    /**
     * @brief Set the default TTL applied to entries that specify ttl_ms = 0.
     * @param ttl_ms TTL in milliseconds.  0 disables TTL-based expiration.
        * @note Callers should not assume previously inserted entries are updated
        *       retroactively when the default TTL changes.
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
