/**
 * @file access_coordinator.h
 * @brief Broker for cache↔storage promotion/demotion coordination.
 *
 * ThemisDB | File: access_coordinator.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 1 API Definition) | Status: Frozen for v1.x
 * Author: Copilot | Date: 2026-08-03
 *
 * The `AccessCoordinator` is the central broker that manages tier transitions
 * between cache and storage tiers. It listens to eviction events from cache,
 * access patterns from storage, and applies unified aging policies to make
 * promotion/demotion decisions.
 *
 * @see include/access_model/access_tier_interface.h
 * @see include/access_model/promotion_demotion.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 */

#pragma once

#include "access_tier_interface.h"
#include "promotion_demotion.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace access_model {

// Forward declarations
class AgeBasedPolicy;
class AccessMetrics;

// ============================================================================
// § 1  Coordinator Event Listeners
// ============================================================================

/**
 * @brief Callback interface for cache eviction events.
 *
 * Cache implementations call this to notify the coordinator when evicting
 * entries due to capacity constraints.
 */
struct EvictionListener {
    virtual ~EvictionListener() = default;

    /**
     * @brief Called when cache evicts a key.
     *
     * @param key Evicted key
     * @param from_tier Cache tier that evicted the key
     * @param size_bytes Size of evicted value
     * @param access_count Number of accesses to this key
     * @param last_access_age_secs Age since last access
     * @param eviction_reason Reason for eviction (e.g., "lru", "lfu", "ttl")
     *
     * Coordinator should consider:
     * - If access_count is high: promote to warm storage
     * - If access_count is low: demotion candidate
     * - If from_tier is L1/L2: consider fallback to L3 before evicting
     */
    virtual void onCacheEvicted(std::string_view key, TierLevel from_tier,
                               std::size_t size_bytes, uint64_t access_count,
                               std::chrono::seconds last_access_age_secs,
                               std::string_view eviction_reason) = 0;
};

/**
 * @brief Callback interface for storage access patterns.
 *
 * Storage implementations call this to notify the coordinator when detecting
 * hot access patterns (candidates for promotion to cache).
 */
struct PromotionListener {
    virtual ~PromotionListener() = default;

    /**
     * @brief Called when storage detects hot access.
     *
     * @param key Accessed key
     * @param from_tier Storage tier that detected the access
     * @param access_count Total accesses within window
     * @param access_window Time window for access counting
     *
     * Coordinator should consider:
     * - If access_count / access_window > threshold: candidate for promotion
     * - Promote cold→warm→L3 asynchronously
     * - Warm→L3 if access_count high; cold→warm always
     */
    virtual void onStorageAccess(std::string_view key, TierLevel from_tier,
                                uint64_t access_count,
                                std::chrono::seconds access_window) = 0;
};

// ============================================================================
// § 2  AccessCoordinator Interface
// ============================================================================

/**
 * @brief Central broker for cache↔storage tier coordination.
 *
 * The coordinator:
 * 1. Listens to cache eviction events
 * 2. Listens to storage access patterns
 * 3. Applies unified age-based policies
 * 4. Orchestrates promotion/demotion workers
 * 5. Tracks metrics and correlation IDs
 * 6. Exposes observability surfaces
 *
 * **Thread Safety:** Yes, all public methods are thread-safe
 *
 * **Opt-In Design:** Applications must explicitly register the coordinator
 * at startup. Existing code continues to work without it.
 */
class AccessCoordinator : public EvictionListener, public PromotionListener {
public:
    virtual ~AccessCoordinator() = default;

    /// ────────────────────────────────────────────────────────────────────
    /// Initialization & Lifecycle
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Initialize the coordinator with tier registry.
     *
     * @param cache_tiers Map of cache tiers by level (L1, L2, L3)
     * @param storage_tiers Map of storage tiers by level (hot, warm, cold)
     * @return True if initialization succeeded
     *
     * Must be called before registering listeners or initiating operations.
     */
    virtual bool initialize(const std::map<TierLevel, std::shared_ptr<AccessTier>>& all_tiers) = 0;

    /**
     * @brief Shutdown the coordinator gracefully.
     *
     * Stops background workers, flushes pending operations.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if coordinator is running.
     */
    virtual bool isRunning() const = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Policy Configuration
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Set unified age-based migration policy.
     *
     * @param policy Age policy applied to both cache and storage tiers
     *
     * Ensures cache and storage use consistent "hotness" definitions.
     */
    virtual void setAgePolicy(const AgeBasedPolicy& policy) = 0;

    /**
     * @brief Get current age-based policy.
     */
    virtual AgeBasedPolicy getAgePolicy() const = 0;

    /**
     * @brief Set access-frequency thresholds for promotion decisions.
     *
     * @param l1_to_l2_threshold Access count to keep in L1 (default: 10)
     * @param l2_to_l3_threshold Access count to promote L2→L3 (default: 5)
     * @param storage_promotion_threshold Access count to promote storage→cache (default: 3)
     */
    virtual void setPromotionThresholds(uint64_t l1_to_l2_threshold,
                                       uint64_t l2_to_l3_threshold,
                                       uint64_t storage_promotion_threshold) = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Promotion/Demotion Operations
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Promote data from a lower tier to a higher tier.
     *
     * @param key Data key to promote
     * @param from_tier Source tier (must be lower than to_tier)
     * @param to_tier Destination tier (must be higher than from_tier)
     * @param options Promotion options (timeout, callback, etc.)
     * @return Promotion result (success, latency, path taken)
     *
     * **Blocking:** Returns immediately; actual promotion happens async
     * **Callbacks:** on_complete called when promotion finishes
     */
    virtual TierPromotionResult promoteAsync(std::string_view key, TierLevel from_tier,
                                            TierLevel to_tier,
                                            const TierAccessOptions& options) = 0;

    /**
     * @brief Plan a demotion operation (for review before execution).
     *
     * @param key Data key to demote
     * @param from_tier Source tier
     * @param to_tier Destination tier
     * @param reason Reason for demotion (e.g., "age", "cache_eviction")
     * @return Demotion plan (can be cancelled before execute())
     *
     * Does not execute; use execute() on the returned plan.
     */
    virtual DemotionPlan planDemotion(std::string_view key, TierLevel from_tier,
                                     TierLevel to_tier,
                                     std::string_view reason) = 0;

    /**
     * @brief Execute a demotion plan.
     *
     * @param plan Plan returned from planDemotion()
     * @param options Execution options (timeout, callback)
     * @return Result of demotion
     */
    virtual DemotionResult executeDemotion(const DemotionPlan& plan,
                                          const TierAccessOptions& options) = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Event Listeners (EvictionListener & PromotionListener Impl)
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Callback: cache notifies of eviction.
     *
     * Implements EvictionListener interface.
     */
    void onCacheEvicted(std::string_view key, TierLevel from_tier,
                       std::size_t size_bytes, uint64_t access_count,
                       std::chrono::seconds last_access_age_secs,
                       std::string_view eviction_reason) override = 0;

    /**
     * @brief Callback: storage notifies of hot access.
     *
     * Implements PromotionListener interface.
     */
    void onStorageAccess(std::string_view key, TierLevel from_tier,
                        uint64_t access_count,
                        std::chrono::seconds access_window) override = 0;

    /// ────────────────────────────────────────────────────────────────────
    /// Observability & Metrics
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Get aggregated metrics for a key across all tiers.
     *
     * @param key Data key
     * @return Aggregated metrics (current tier, access count, promotion path)
     */
    virtual AccessMetrics getKeyMetrics(std::string_view key) const = 0;

    /**
     * @brief Get tier-specific metrics.
     *
     * @param tier_level Tier to query
     * @return Metrics for that tier (hit rate, avg latency, capacity)
     */
    virtual TierMetrics getTierMetrics(TierLevel tier_level) const = 0;

    /**
     * @brief Get recent promotion/demotion operations (for debugging).
     *
     * @param limit Maximum number of recent operations to return
     * @return Vector of recent tier transition events
     */
    virtual std::vector<AccessTransitionEvent> getRecentTransitions(
        std::size_t limit = 100) const = 0;

    /**
     * @brief Get correlation ID for a recent operation.
     *
     * @param key Data key
     * @return Most recent correlation ID for this key, or empty string
     */
    virtual std::string getLastCorrelationId(std::string_view key) const = 0;
};

// ============================================================================
// § 3  Supporting Data Structures
// ============================================================================

/**
 * @brief Metrics for a single tier.
 */
struct TierMetrics {
    TierLevel tier;
    double hit_rate = 0.0;                          ///< 0.0 to 1.0
    std::chrono::microseconds avg_get_latency_us;
    std::chrono::microseconds avg_put_latency_us;
    std::size_t current_size_bytes = 0;
    std::size_t max_capacity_bytes = 0;
    std::size_t entry_count = 0;
    uint64_t total_accesses = 0;
    uint64_t total_hits = 0;
    uint64_t total_misses = 0;
};

/**
 * @brief Aggregated metrics for a key across all tiers.
 */
struct AccessMetrics {
    std::string key;
    TierLevel current_tier;                         ///< Where key currently resides
    uint64_t total_accesses = 0;
    std::chrono::seconds age_in_tier;               ///< Time in current tier
    std::vector<TierLevel> promotion_path;          ///< Tiers key has visited
    std::chrono::milliseconds total_promotion_latency_ms;
    std::string last_correlation_id;
    std::chrono::system_clock::time_point last_access_time;
};

/**
 * @brief Record of a tier transition event.
 */
struct AccessTransitionEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string correlation_id;
    std::string key;
    TierLevel from_tier;
    TierLevel to_tier;
    std::chrono::milliseconds latency_ms;
    std::string reason;                             ///< "eviction", "age", "access", etc.
    bool success = true;
    std::string error_message;
};

// ============================================================================
// § 4  Factory Function
// ============================================================================

/**
 * @brief Create a new AccessCoordinator instance.
 *
 * @param thread_pool_size Number of background worker threads
 * @return Newly allocated coordinator (caller owns)
 */
std::unique_ptr<AccessCoordinator> createAccessCoordinator(
    std::size_t thread_pool_size = 4);

}  // namespace access_model
}  // namespace themis

#endif  // THEMISDB_INCLUDE_ACCESS_MODEL_ACCESS_COORDINATOR_H
