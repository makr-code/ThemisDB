/**
 * @file i_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 89/100
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

struct CacheEntry {
    std::string payload;      // Serialized data
    uint64_t version{0};      // Version number for cache invalidation
    uint64_t timestamp_ms{0}; // Creation/update timestamp

    CacheEntry() = default;
    CacheEntry(std::string data, uint64_t ver = 0, uint64_t ts = 0)
        : payload(std::move(data)), version(ver), timestamp_ms(ts) {}
};

class ICache {
public:
    /**
     * @brief ICache.
     * @return Return value.
     */
    virtual ~ICache() = default;

    // -----------------------------------------------------------------------
    // Core cache operations
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual std::optional<CacheEntry> get(std::string_view key) const = 0;

    [[nodiscard]] virtual bool put(std::string_view key, const CacheEntry& entry, uint64_t ttl_ms = 0) = 0;

    /**
     * @brief Invalidate.
     * @param[in] key Input parameter.
     */
    virtual void invalidate(std::string_view key) = 0;

    /**
     * @brief Clear.
     */
    virtual void clear() = 0;

    // -----------------------------------------------------------------------
    // Batch operations
    // -----------------------------------------------------------------------

    /**
     * @brief Invalidate Pattern.
     * @param[in] pattern Input parameter.
     */
    virtual void invalidatePattern(std::string_view pattern) = 0;

    // -----------------------------------------------------------------------
    // Cache statistics
    // -----------------------------------------------------------------------

    [[nodiscard]] virtual size_t size() const = 0;

    [[nodiscard]] virtual uint64_t hitCount() const = 0;

    [[nodiscard]] virtual uint64_t missCount() const = 0;

    [[nodiscard]] virtual double hitRate() const = 0;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set Max Size.
     * @param[in] maxSize Input parameter.
     */
    virtual void setMaxSize(size_t maxSize) = 0;

    /**
     * @brief Set Default TTL.
     * @param[in] ttl_ms Input parameter.
     */
    virtual void setDefaultTTL(uint64_t ttl_ms) = 0;

    // -----------------------------------------------------------------------
    // Optional extension points
    // -----------------------------------------------------------------------

    /**
     * @brief Get Eviction Strategy.
     * @return Pointer to the result.
     * @details Implements getEvictionStrategy without additional internal calls.
     */
    virtual IEvictionStrategy* getEvictionStrategy() { return nullptr; }

    virtual const IEvictionStrategy* getEvictionStrategy() const { return nullptr; }

    virtual const CacheMetrics* getMetrics() const { return nullptr; }

    // Lifecycle hooks
    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }
};

} // namespace concerns
} // namespace core
} // namespace themis
