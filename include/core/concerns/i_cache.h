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

    // Core cache operations
    virtual std::optional<CacheEntry> get(std::string_view key) const = 0;
    virtual bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) = 0;
    virtual void invalidate(std::string_view key) = 0;
    virtual void clear() = 0;

    // Batch operations
    virtual void invalidatePattern(std::string_view pattern) = 0;
    
    // Cache statistics
    virtual size_t size() const = 0;
    virtual uint64_t hitCount() const = 0;
    virtual uint64_t missCount() const = 0;
    virtual double hitRate() const = 0;

    // Configuration
    virtual void setMaxSize(size_t maxSize) = 0;
    virtual void setDefaultTTL(uint64_t ttl_ms) = 0;
    
    // Optional: Strategy pattern support (not all implementations need this)
    // Return nullptr if strategy pattern is not supported
    virtual IEvictionStrategy* getEvictionStrategy() { return nullptr; }
    virtual const IEvictionStrategy* getEvictionStrategy() const { return nullptr; }
    
    // Optional: Metrics support (not all implementations need this)
    // Return nullptr if detailed metrics are not available
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
