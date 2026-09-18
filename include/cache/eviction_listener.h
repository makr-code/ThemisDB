/**
 * @file eviction_listener.h
 * @brief Cache eviction listener interface for Phase 3 cache→coordinator→storage integration.
 * @version 1.0.0
 * @note Maturity: 🟡 BETA (Phase 3 Integration)
 * 
 * Provides callback interface for cache implementations to emit eviction signals to
 * external consumers (e.g., AccessCoordinator). This enables:
 * - Unified cache-storage tier management
 * - Demotion feedback hooks
 * - Storage promotion/demotion decision support
 * 
 * @see include/access_model/access_coordinator.h (EvictionListener base)
 * @see src/cache/ROADMAP.md Phase 3
 * @see docs/architecture/CACHE_STORAGE_INTEGRATION.md
 */

#pragma once

#include <chrono>
#include <memory>
#include <string_view>
#include <vector>

namespace themis {
namespace cache {

// Forward declaration
class CacheEvictionPolicy;

enum class TierLevel : uint8_t {
    L1 = 2,  ///< Hot cache tier (in-memory, microsecond latency)
    L2 = 1,  ///< Warm cache tier (compressed, millisecond latency)
    L3 = 0   ///< Cold cache tier (persistent, multi-millisecond latency)
};

enum class EvictionReason : uint8_t {
    Capacity,        ///< Cache tier reached capacity; LRU/LFU victim selected
    TTL_Expired,     ///< Entry TTL elapsed
    AccessCount,     ///< Access count fell below retention threshold
    Manual,          ///< Caller explicitly requested eviction
    Pattern,         ///< Glob/regex pattern eviction
    TenantEviction,  ///< All entries for a tenant evicted
    Flush            ///< Entire cache or partition flushed
};

struct CacheEvictionEvent {
    std::string key;                                    ///< Evicted cache key
    TierLevel from_tier = TierLevel::L1;               ///< Cache tier that evicted
    std::size_t size_bytes = 0;                        ///< Size of evicted value
    uint64_t access_count = 0;                         ///< Number of accesses to this key
    std::chrono::seconds last_access_age;              ///< Age since last access
    EvictionReason reason = EvictionReason::Capacity;  ///< Why eviction occurred
    std::string correlation_id;                        ///< Trace correlation ID
};

class IEvictionListener {
public:
    /**
     * @brief IEviction Listener.
     * @return Return value.
     */
    virtual ~IEvictionListener() = default;

    /**
     * @brief On Cache Evicted.
     * @param[in] event Input parameter.
     */
    virtual void onCacheEvicted(const CacheEvictionEvent& event) = 0;

    /**
     * @brief On Capacity Pressure.
     * @param[in] from_tier Input parameter.
     * @param[in] current_capacity_percent Input parameter.
     * @param[in] recommended_eviction_count Input parameter.
     * @details Implements onCapacityPressure without additional internal calls.
     */
    virtual void onCapacityPressure(TierLevel from_tier,
                                    uint32_t current_capacity_percent,
                                    std::size_t recommended_eviction_count) {}
};

class EvictionListenerManager {
public:
    /**
     * @brief Eviction Listener Manager.
     * @return Return value.
     */
    virtual ~EvictionListenerManager() = default;

    /**
     * @brief Register Listener.
     * @param[in] listener Input parameter.
     * @return Return value.
     */
    virtual uint64_t registerListener(std::shared_ptr<IEvictionListener> listener) = 0;

    /**
     * @brief Unregister Listener.
     * @param[in] handle Input parameter.
     */
    virtual void unregisterListener(uint64_t handle) = 0;

    /**
     * @brief Emit Eviction Event.
     * @param[in] event Input parameter.
     */
    virtual void emitEvictionEvent(const CacheEvictionEvent& event) = 0;

    /**
     * @brief Emit Capacity Pressure.
     * @param[in] from_tier Input parameter.
     * @param[in] current_capacity_percent Input parameter.
     * @param[in] recommended_eviction_count Input parameter.
     */
    virtual void emitCapacityPressure(TierLevel from_tier,
                                      uint32_t current_capacity_percent,
                                      std::size_t recommended_eviction_count) = 0;

    /**
     * @brief Get Listener Count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    virtual std::size_t getListenerCount() const noexcept = 0;
};

/**
 * @brief Create Eviction Listener Manager.
 * @return Return value.
 */
std::unique_ptr<EvictionListenerManager> createEvictionListenerManager();

}  // namespace cache
}  // namespace themis

