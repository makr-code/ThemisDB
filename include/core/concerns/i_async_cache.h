/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_async_cache.h                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:44:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     184                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_cache.h"
#include <future>
#include &lt;optional&gt;
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Async variant of the cache interface for non-blocking cache operations.
 *
 * `IAsyncCache` extends `ICache` with `std::future`-returning methods so that
 * callers on hot paths can initiate a cache lookup or write without blocking
 * on network I/O (e.g. a Redis backend).
 *
 * ## Default implementations
 *
 * Every async method has a default body that dispatches the corresponding
 * synchronous `ICache` method on a separate thread via `std::async`.  Concrete
 * classes that already implement `ICache` (e.g. `InMemoryCacheImpl`) can
 * inherit from `IAsyncCache` and get working async variants for free:
 *
 * @code
 *   class MyRedisCache : public IAsyncCache {
 *       // Only needs to implement the ICache pure-virtual methods;
 *       // all IAsyncCache methods work via the defaults (or can be
 *       // overridden with a native async Redis client call).
 *   };
 * @endcode
 *
 * ## Thread Safety
 *
 * The underlying `ICache` implementation must be thread-safe if multiple
 * async operations can be in-flight concurrently.
 *
 * ## Usage
 *
 * @code
 *   IAsyncCache& cache = ...;
 *
 *   // Non-blocking get — do other work while cache I/O is in-flight
 *   auto future = cache.getAsync("user:42");
 *   // ... other work ...
 *   auto entry = future.get();
 *   if (entry) { ... }
 *
 *   // Fire-and-forget write
 *   cache.putAsync("user:42", entry, 30000);
 * @endcode
 */
class IAsyncCache : public ICache {
public:
    // -----------------------------------------------------------------------
    // Async cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Retrieve a cache entry asynchronously.
     *
     * @param key Cache key.
     * @return A future that resolves to the cached `CacheEntry` if present
     *         and not expired, or `std::nullopt` on a cache miss.
     */
    virtual std::future<std::optional<CacheEntry>> getAsync(std::string_view key) {
        return std::async(std::launch::async,
            [this, k = std::string(key)]() -> std::optional<CacheEntry> {
                return this->get(k);
            });
    }

    /**
     * @brief Insert or replace a cache entry asynchronously.
     *
     * @param key    Cache key.
     * @param entry  Value to store.
     * @param ttl_ms Entry TTL in milliseconds (0 = default TTL).
     * @return A future that resolves to true on success, false on failure.
     */
    virtual std::future<bool> putAsync(std::string_view key,
                                        const CacheEntry& entry,
                                        uint64_t ttl_ms = 0)
    {
        return std::async(std::launch::async,
            [this, k = std::string(key), e = entry, t = ttl_ms]() -> bool {
                return this->put(k, e, t);
            });
    }

    /**
     * @brief Remove a single cache entry asynchronously.
     *
     * @param key Key of the entry to remove.
     * @return A future that resolves when the entry has been removed (or was
     *         already absent).
     */
    virtual std::future<void> invalidateAsync(std::string_view key) {
        return std::async(std::launch::async,
            [this, k = std::string(key)] {
                this->invalidate(k);
            });
    }
};

// ---------------------------------------------------------------------------
// NoOpAsyncCache
// ---------------------------------------------------------------------------

/**
 * @brief No-op async cache implementation.
 *
 * All sync cache operations are silent no-ops with sane return values.
 * All async methods return already-satisfied futures (via
 * `std::launch::deferred`) so tests can call them without spawning threads.
 *
 * Example (testing):
 * @code
 *   NoOpAsyncCache cache;
 *   auto f = cache.getAsync("user:42");
 *   EXPECT_FALSE(f.get().has_value()); // always a miss
 * @endcode
 */
class NoOpAsyncCache : public IAsyncCache {
public:
    // ICache sync methods (all no-ops with safe defaults)
    std::optional<CacheEntry> get(std::string_view) const override {
        return std::nullopt;
    }
    bool put(std::string_view, const CacheEntry&, uint64_t) override {
        return true;
    }
    void invalidate(std::string_view) override {}
    void clear() override {}
    void invalidatePattern(std::string_view) override {}
    size_t size() const override { return 0; }
    uint64_t hitCount() const override { return 0; }
    uint64_t missCount() const override { return 0; }
    double hitRate() const override { return 0.0; }
    void setMaxSize(size_t) override {}
    void setDefaultTTL(uint64_t) override {}
    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    // IAsyncCache overrides — deferred (no real thread spawned)
    std::future<std::optional<CacheEntry>> getAsync(std::string_view) override {
        return std::async(std::launch::deferred,
            []() -> std::optional<CacheEntry> { return std::nullopt; });
    }
    std::future<bool> putAsync(std::string_view, const CacheEntry&, uint64_t) override {
        return std::async(std::launch::deferred, [] { return true; });
    }
    std::future<void> invalidateAsync(std::string_view) override {
        return std::async(std::launch::deferred, [] {});
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
