/**
 * @file i_async_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_cache.h"
#include <future>
#include <optional>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

class IAsyncCache : public ICache {
public:
    // -----------------------------------------------------------------------
    // Async cache operations
    // -----------------------------------------------------------------------

    /**
     * @brief Get Async.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: std::async(), std::string(), get().
     */
    virtual std::future<std::optional<CacheEntry>> getAsync(std::string_view key) {
        return std::async(std::launch::async,
            [this, k = std::string(key)]() -> std::optional<CacheEntry> {
                return this->get(k);
            });
    }

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
     * @brief Invalidate Async.
     * @param[in] key Input parameter.
     * @return Return value.
     * @details Calls: std::async(), std::string(), invalidate().
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
