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

/**
 * @brief Enum for cache tier level (L1, L2, L3).
 * 
 * Maps to storage tiers for promotion/demotion coordination.
 * L1 = hot cache (fast, small)
 * L2 = warm cache (compressed, medium)
 * L3 = cold cache (persistent, large)
 */
enum class TierLevel : uint8_t {
    L1 = 2,  ///< Hot cache tier (in-memory, microsecond latency)
    L2 = 1,  ///< Warm cache tier (compressed, millisecond latency)
    L3 = 0   ///< Cold cache tier (persistent, multi-millisecond latency)
};

/**
 * @brief Reason why an eviction occurred.
 * 
 * Used to inform coordinator of the eviction context for better
 * promotion/demotion decisions.
 */
enum class EvictionReason : uint8_t {
    Capacity,        ///< Cache tier reached capacity; LRU/LFU victim selected
    TTL_Expired,     ///< Entry TTL elapsed
    AccessCount,     ///< Access count fell below retention threshold
    Manual,          ///< Caller explicitly requested eviction
    Pattern,         ///< Glob/regex pattern eviction
    TenantEviction,  ///< All entries for a tenant evicted
    Flush            ///< Entire cache or partition flushed
};

/**
 * @brief Event data emitted when cache evicts an entry.
 * 
 * Consumed by AccessCoordinator to decide whether to:
 * - Promote high-access entries to warm storage
 * - Demote low-access entries to cold storage
 * - Track promotion path history
 */
struct CacheEvictionEvent {
    std::string key;                                    ///< Evicted cache key
    TierLevel from_tier = TierLevel::L1;               ///< Cache tier that evicted
    std::size_t size_bytes = 0;                        ///< Size of evicted value
    uint64_t access_count = 0;                         ///< Number of accesses to this key
    std::chrono::seconds last_access_age;              ///< Age since last access
    EvictionReason reason = EvictionReason::Capacity;  ///< Why eviction occurred
    std::string correlation_id;                        ///< Trace correlation ID
};

/**
 * @brief Listener interface for cache eviction events.
 * 
 * Cache implementations register listeners at startup to emit eviction signals.
 * Listeners are called synchronously when eviction occurs.
 * 
 * **Implementation notes for listeners:**
 * - Keep implementations fast (< 1ms total)
 * - Do not block for I/O operations (queue async work instead)
 * - Do not throw exceptions (log errors and continue)
 * - Thread-safe: may be called from concurrent cache operations
 */
class IEvictionListener {
public:
    virtual ~IEvictionListener() = default;

    /**
     * @brief Called when cache evicts an entry.
     * 
     * @param event Event data describing the eviction
     * 
     * **Callback Semantics:**
     * - Called immediately after cache removes the entry
     * - Runs on caller's thread (typically a cache worker or user thread)
     * - Should complete quickly; async operations should be queued
     * - Exceptions must not be thrown (log errors internally)
     * 
     * **Coordinator Usage (typical):**
     * - High access_count → consider promoting to warm storage
     * - Low access_count → may be demotion candidate
     * - from_tier = L1/L2 → consider L3 fallback before evicting
     * - reason = Capacity → high-pressure signal
     */
    virtual void onCacheEvicted(const CacheEvictionEvent& event) = 0;

    /**
     * @brief Optional: called when capacity pressure is detected.
     * 
     * @param from_tier Tier experiencing pressure
     * @param current_capacity_percent Current usage (0-100)
     * @param recommended_eviction_count Suggested entries to evict
     * 
     * Allows coordinators to prepare promotion/demotion decisions
     * before evictions occur. Default implementation is no-op.
     */
    virtual void onCapacityPressure(TierLevel from_tier,
                                    uint32_t current_capacity_percent,
                                    std::size_t recommended_eviction_count) {}
};

/**
 * @brief Manager for eviction listeners.
 * 
 * Cache implementations use this to:
 * 1. Register listeners at startup
 * 2. Emit eviction events to all listeners
 * 3. Unregister listeners at shutdown
 */
class EvictionListenerManager {
public:
    virtual ~EvictionListenerManager() = default;

    /**
     * @brief Register a listener to receive eviction events.
     * 
     * @param listener Listener callback
     * @return Handle for later unregistration
     * 
     * Multiple listeners can be registered. Each receives all events.
     */
    virtual uint64_t registerListener(std::shared_ptr<IEvictionListener> listener) = 0;

    /**
     * @brief Unregister a listener.
     * 
     * @param handle Handle returned by registerListener()
     * 
     * After unregistration, listener receives no more events.
     */
    virtual void unregisterListener(uint64_t handle) = 0;

    /**
     * @brief Emit an eviction event to all registered listeners.
     * 
     * @param event Event data
     * 
     * Calls onCacheEvicted() on all registered listeners synchronously.
     * If any listener throws, exception is logged and other listeners
     * still receive the event.
     */
    virtual void emitEvictionEvent(const CacheEvictionEvent& event) = 0;

    /**
     * @brief Emit a capacity pressure event to all registered listeners.
     * 
     * @param from_tier Tier experiencing pressure
     * @param current_capacity_percent Current usage (0-100)
     * @param recommended_eviction_count Suggested entries to evict
     */
    virtual void emitCapacityPressure(TierLevel from_tier,
                                      uint32_t current_capacity_percent,
                                      std::size_t recommended_eviction_count) = 0;

    /**
     * @brief Get count of registered listeners.
     */
    virtual std::size_t getListenerCount() const noexcept = 0;
};

/**
 * @brief Create a new EvictionListenerManager instance.
 * 
 * @return Unique pointer to manager
 * 
 * Cache implementations should create one manager per cache instance
 * and use it to manage listener registration/emission.
 */
std::unique_ptr<EvictionListenerManager> createEvictionListenerManager();

}  // namespace cache
}  // namespace themis

