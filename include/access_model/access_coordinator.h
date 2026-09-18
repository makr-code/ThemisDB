/**
 * @file access_coordinator.h
 * @brief Broker for cache↔storage promotion/demotion coordination.
 *
 * ThemisDB | File: access_coordinator.h | Version: 2.0.0
 * Maturity: 🟡 ALPHA (Phase 2 Implementation) | Status: Active development
 * Author: Copilot | Date: 2026-08-03
 *
 * The `AccessCoordinator` is the central broker that manages tier transitions
 * between cache and storage tiers. It receives eviction events from cache,
 * access patterns from storage, and applies unified aging policies to make
 * promotion/demotion decisions.
 *
 * **Listener vs. Coordinator design:**
 * - `EvictionListener` / `PromotionListener` are thin interfaces used by cache
 *   and storage modules to emit raw events.
 * - `AccessCoordinator` is the full coordinator that receives structured events
 *   (`EvictionEvent`, `AccessEvent`) and drives tier transitions.
 *
 * @see include/access_model/access_tier_interface.h
 * @see include/access_model/promotion_demotion.h
 * @see include/access_model/access_metrics.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 */

#pragma once

#include "access_metrics.h"
#include "access_tier_interface.h"
#include "age_based_policy.h"
#include "promotion_demotion.h"

#include <atomic>
#include <chrono>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Thin Event-Listener Interfaces (used by cache & storage modules)
// ============================================================================

/**
 * @brief Callback interface for cache eviction events.
 *
 * Cache implementations (e.g., AdaptiveQueryCache) hold a raw pointer to an
 * object implementing this interface and call `onCacheEvicted()` synchronously
 * when an entry is evicted.
 *
 * **Implementation contract for listeners:**
 * - Keep execution time < 1 ms (queue async work; do not block for I/O)
 * - Do not throw exceptions
 * - Thread-safe: may be called from concurrent cache operations
 */
struct EvictionListener {
    virtual ~EvictionListener() = default;

    /**
     * @brief Called when cache evicts a key.
     *
     * @param key Evicted key
     * @param from_tier Cache tier that evicted the key
     * @param size_bytes Size of evicted value in bytes
     * @param access_count Number of accesses to this key before eviction
     * @param last_access_age_secs Age since last access
     * @param eviction_reason Reason for eviction (e.g., "lru", "lfu", "ttl")
     */
    virtual void onCacheEvicted(std::string_view key, TierLevel from_tier,
                                std::size_t size_bytes, uint64_t access_count,
                                std::chrono::seconds last_access_age_secs,
                                std::string_view eviction_reason) = 0;
};

/**
 * @brief Callback interface for storage access patterns.
 *
 * Storage implementations hold a raw pointer to an object implementing this
 * interface and call `onStorageAccess()` when hot access patterns are detected.
 */
struct PromotionListener {
    virtual ~PromotionListener() = default;

    /**
     * @brief Called when storage detects a hot-access pattern.
     *
     * @param key Accessed key
     * @param from_tier Storage tier that detected the access
     * @param access_count Total accesses within the measurement window
     * @param access_window Time window for access counting
     */
    virtual void onStorageAccess(std::string_view key, TierLevel from_tier,
                                 uint64_t access_count,
                                 std::chrono::seconds access_window) = 0;
};

// ============================================================================
// § 2  Structured Event Types (used by AccessCoordinator internal interface)
// ============================================================================

/**
 * @brief Structured eviction event passed to AccessCoordinator.
 *
 * Higher-level than the raw `EvictionListener::onCacheEvicted()` parameters;
 * groups all eviction context for coordinator policy decisions.
 */
struct EvictionEvent {
    /// Evicted key
    std::string key;

    /// Cache tier that performed the eviction
    TierLevel tier = TierLevel::UNKNOWN;

    /// Reason string (e.g., "lru_eviction", "ttl_expired", "capacity_pressure")
    std::string reason;

    /// Size of evicted value in bytes
    std::size_t evicted_size_bytes = 0;

    /// Number of accesses before eviction
    uint64_t access_count = 0;

    /// Age since last access at eviction time
    std::chrono::seconds last_access_age_secs{0};

    /// Optional correlation ID for tracing
    std::string correlation_id;
};

/**
 * @brief Structured access event passed to AccessCoordinator.
 *
 * Describes a hot-access detection from a storage tier.
 */
struct AccessEvent {
    /// Accessed key
    std::string key;

    /// Storage tier that observed the hot pattern
    TierLevel current_tier = TierLevel::UNKNOWN;

    /// Number of accesses within the measurement window
    uint64_t access_count = 0;

    /// Measurement window duration
    std::chrono::seconds access_window{86400};  // Default: 24 hours

    /// Optional correlation ID for tracing
    std::string correlation_id;
};

/**
 * @brief Result of an asynchronous promotion operation.
 *
 * Returned via `std::future<PromotionResult>` from `promoteAsync()`.
 */
struct PromotionResult {
    /// True if the promotion completed successfully
    bool success = false;

    /// Error message (populated when success=false)
    std::string error_message;

    /// Data size promoted in bytes
    std::size_t size_bytes = 0;

    /// Source tier (where data was promoted from)
    TierLevel from_tier = TierLevel::UNKNOWN;

    /// Destination tier (where data was promoted to)
    TierLevel to_tier = TierLevel::UNKNOWN;

    /// End-to-end promotion latency
    std::chrono::milliseconds total_latency_ms{0};

    /// Correlation ID for tracing
    std::string correlation_id;

    /// Timestamp when promotion completed
    std::chrono::system_clock::time_point completed_at = std::chrono::system_clock::now();
};

/**
 * @brief Record of a tier transition event (for observability).
 */
struct AccessTransitionEvent {
    /// Key that was transitioned
    std::string key;

    /// Source tier
    TierLevel from_tier = TierLevel::UNKNOWN;

    /// Destination tier (UNKNOWN if no transition was triggered)
    TierLevel to_tier = TierLevel::UNKNOWN;

    /// Human-readable reason for the transition
    std::string reason;

    /// Timestamp of the event
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    /// End-to-end processing latency
    std::chrono::milliseconds latency_ms{0};

    /// Correlation ID for tracing
    std::string correlation_id;

    /// Whether the transition succeeded
    bool success = true;

    /// Error message (if success=false)
    std::string error_message;
};

// ============================================================================
// § 3  AccessCoordinator Interface
// ============================================================================

/**
 * @brief Central broker for cache↔storage tier coordination.
 *
 * The coordinator:
 * 1. Receives cache eviction events via `onEviction()`
 * 2. Receives storage hot-access events via `onHotAccess()`
 * 3. Applies unified age-based policies to decide promotion/demotion
 * 4. Orchestrates promotion/demotion workers via `promoteAsync()`
 * 5. Tracks metrics and correlation IDs for observability
 *
 * **Thread Safety:** All public methods are thread-safe.
 *
 * **Opt-In Design:** Applications must explicitly register the coordinator
 * at startup. Existing code continues to work without it.
 */
class AccessCoordinator {
public:
    virtual ~AccessCoordinator() = default;

    /// ────────────────────────────────────────────────────────────────────
    /// Lifecycle
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Initialize the coordinator with a tier registry.
     *
     * @param all_tiers Map of tier levels to `AccessTier` implementations
     * @return True if initialization succeeded
     *
     * Must be called before `start()`. Safe to call before registering
     * listeners.
     */
    virtual bool initialize(
        const std::map<TierLevel, std::shared_ptr<AccessTier>>& all_tiers) = 0;

    /**
     * @brief Start background worker threads.
     *
     * After this call, promotion/demotion tasks are processed asynchronously.
     * Must be called after `initialize()`.
     */
    virtual void start() = 0;

    /**
     * @brief Shutdown the coordinator gracefully.
     *
     * Stops background workers and waits for in-flight tasks to complete.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check whether the coordinator is running.
     */
    virtual bool isRunning() const = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Event Ingestion
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Notify the coordinator of a cache eviction.
     *
     * Called (typically by an `EvictionListener` adapter) when a cache tier
     * evicts a key. The coordinator applies the current `AgeBasedPolicy` to
     * decide whether to demote to storage.
     *
     * @param event Structured eviction event
     *
     * **Thread Safety:** Yes.
     * **Blocking:** Returns quickly; actual demotion is queued asynchronously.
     */
    virtual void onEviction(const EvictionEvent& event) = 0;

    /**
     * @brief Notify the coordinator of a storage hot-access pattern.
     *
     * Called (typically by a `PromotionListener` adapter) when a storage tier
     * detects repeated accesses on a key. The coordinator applies the current
     * `AgeBasedPolicy` to decide whether to promote to cache.
     *
     * @param event Structured access event
     *
     * **Thread Safety:** Yes.
     * **Blocking:** Returns quickly; actual promotion is queued asynchronously.
     */
    virtual void onHotAccess(const AccessEvent& event) = 0;

    /**
     * @brief Convenience adapter for cache eviction events from legacy callers.
     *
     * This bridges the older listener-style API to the structured coordinator API.
     */
    virtual void onCacheEvicted(std::string_view key, TierLevel from_tier,
                                std::size_t size_bytes, uint64_t access_count,
                                std::chrono::seconds last_access_age_secs,
                                std::string_view eviction_reason) {
        EvictionEvent event;
        event.key = std::string(key);
        event.tier = from_tier;
        event.reason = std::string(eviction_reason);
        event.evicted_size_bytes = size_bytes;
        event.access_count = access_count;
        event.last_access_age_secs = last_access_age_secs;
        onEviction(event);
    }

    /**
     * @brief Convenience adapter for storage hot-access events from legacy callers.
     *
     * This bridges the older listener-style API to the structured coordinator API.
     */
    virtual void onStorageAccess(std::string_view key, TierLevel from_tier,
                                uint64_t access_count,
                                std::chrono::seconds access_window) {
        AccessEvent event;
        event.key = std::string(key);
        event.current_tier = from_tier;
        event.access_count = access_count;
        event.access_window = access_window;
        onHotAccess(event);
    }

    /// ────────────────────────────────────────────────────────────────────
    /// Policy Configuration
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Set the unified age-based migration policy.
     *
     * @param policy Policy applied uniformly to cache and storage tiers
     */
    virtual void setAgePolicy(const AgeBasedPolicy& policy) = 0;

    /**
     * @brief Set access-frequency thresholds for promotion decisions.
     *
     * @param cache_threshold Access count threshold for cache tier retention
     * @param storage_threshold Access count to trigger storage→cache promotion
     */
    virtual void setPromotionThresholds(uint64_t cache_threshold,
                                        uint64_t storage_threshold) = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Promotion / Demotion Operations
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Asynchronously promote data from a lower tier to a higher tier.
     *
     * @param key Data key to promote
     * @param from_tier Source tier (lower in the hierarchy)
     * @param to_tier Destination tier (higher in the hierarchy)
     * @param size_bytes Approximate data size (for scheduling decisions)
     * @return Future that resolves to `PromotionResult` when promotion completes
     *
     * **Blocking:** Returns immediately; promotion happens on worker thread.
     * **Error:** Result.success=false if coordinator is not running or tiers
     *            are not registered.
     */
    virtual std::future<PromotionResult> promoteAsync(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t size_bytes) = 0;

    /**
     * @brief Plan a demotion operation (for deferred execution).
     *
     * @param key Data key to demote
     * @param from_tier Source tier
     * @param to_tier Destination tier
     * @param data_size_bytes Approximate data size
     * @return Demotion plan with a stable `plan_id`, or `std::nullopt` on error
     *
     * The returned plan can be cancelled before `executeDemotion()` is called.
     */
    virtual std::optional<DemotionPlan> planDemotion(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t data_size_bytes) = 0;

    /**
     * @brief Execute a previously created demotion plan.
     *
     * @param plan_id Plan ID returned by `planDemotion()`
     * @return Demotion result, or `std::nullopt` if plan not found
     */
    virtual std::optional<DemotionResult> executeDemotion(
        const std::string& plan_id) = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Observability & Metrics
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Get aggregated per-key metrics.
     *
     * @param key Data key
     * @return Per-key access metrics (tier, access count, promotion path)
     */
    virtual AccessMetrics getKeyMetrics(const std::string& key) = 0;

    /**
     * @brief Get tier-level metrics.
     *
     * @param tier_level Tier to query
     * @return Aggregated metrics for that tier
     */
    virtual AccessMetrics getTierMetrics(TierLevel tier_level) = 0;

    /**
     * @brief Get aggregated model-level metrics (promotions, demotions, latency).
     *
     * @return Current snapshot of `AccessModelMetrics`
     */
    virtual AccessModelMetrics getAccessModelMetrics() = 0;

    /**
     * @brief Get recent tier transition events (for debugging and dashboards).
     *
     * @param limit Maximum number of events to return (default: 100)
     * @return Vector of recent transitions (most recent first)
     */
    virtual std::vector<AccessTransitionEvent> getRecentTransitions(
        std::size_t limit = 100) = 0;
};

// ============================================================================
// § 4  Factory Function
// ============================================================================

/**
 * @brief Create a new AccessCoordinator instance.
 *
 * @param thread_pool_size Number of background worker threads (default: 4)
 * @return Shared pointer to coordinator (caller shares ownership with listeners)
 */
std::shared_ptr<AccessCoordinator> createAccessCoordinator(
    std::size_t thread_pool_size = 4);

}  // namespace access_model
}  // namespace themis
