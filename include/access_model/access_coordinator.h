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

/** @brief Callback contract for cache adapters reporting completed evictions. */
struct EvictionListener {
    /**
     * @brief Eviction Listener.
     * @return Return value.
     */
    virtual ~EvictionListener() = default;

    /**
     * @brief On Cache Evicted.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] size_bytes Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] last_access_age_secs Input parameter.
     * @param[in] eviction_reason Input parameter.
     */
    virtual void onCacheEvicted(std::string_view key, TierLevel from_tier,
                                std::size_t size_bytes, uint64_t access_count,
                                std::chrono::seconds last_access_age_secs,
                                std::string_view eviction_reason) = 0;
};

/** @brief Callback contract for storage adapters reporting hot-key accesses. */
struct PromotionListener {
    /**
     * @brief Promotion Listener.
     * @return Return value.
     */
    virtual ~PromotionListener() = default;

    /**
     * @brief On Storage Access.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] access_window Input parameter.
     */
    virtual void onStorageAccess(std::string_view key, TierLevel from_tier,
                                 uint64_t access_count,
                                 std::chrono::seconds access_window) = 0;
};

// ============================================================================
// § 2  Structured Event Types (used by AccessCoordinator internal interface)
// ============================================================================

/** @brief Normalized cache eviction metadata consumed by the coordinator. */
struct EvictionEvent {
    std::string key;

    TierLevel tier = TierLevel::UNKNOWN;

    std::string reason;

    std::size_t evicted_size_bytes = 0;

    uint64_t access_count = 0;

    std::chrono::seconds last_access_age_secs{0};

    std::string correlation_id;
};

/** @brief Normalized access metadata consumed by the promotion policy. */
struct AccessEvent {
    std::string key;

    TierLevel current_tier = TierLevel::UNKNOWN;

    uint64_t access_count = 0;

    std::chrono::seconds access_window{86400};  // Default: 24 hours

    std::string correlation_id;
};

/** @brief Status and timing for an asynchronous source-to-destination copy. */
struct PromotionResult {
    bool success = false;

    std::string error_message;

    std::size_t size_bytes = 0;

    TierLevel from_tier = TierLevel::UNKNOWN;

    TierLevel to_tier = TierLevel::UNKNOWN;

    std::chrono::milliseconds total_latency_ms{0};

    std::string correlation_id;

    std::chrono::system_clock::time_point completed_at = std::chrono::system_clock::now();
};

/** @brief Auditable in-memory record of an attempted tier transition. */
struct AccessTransitionEvent {
    std::string key;

    TierLevel from_tier = TierLevel::UNKNOWN;

    TierLevel to_tier = TierLevel::UNKNOWN;

    std::string reason;

    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    std::chrono::milliseconds latency_ms{0};

    std::string correlation_id;

    bool success = true;

    std::string error_message;
};

// ============================================================================
// § 3  AccessCoordinator Interface
// ============================================================================

/**
 * @brief Coordinates bounded background tier promotions and planned demotions.
 * @note Adapter backends are independent; transfer operations are not atomic.
 */
class AccessCoordinator {
public:
    /**
     * @brief Destroy the coordinator and release its implementation resources.
     * @note Call shutdown() and unregister external listeners before destruction.
     */
    virtual ~AccessCoordinator() = default;


    /**
     * @brief Validate and register the tier adapters used for transitions.
     * @param[in] all_tiers Map from TierLevel to the corresponding AccessTier instance.
     *            Must contain at least one cache tier and one storage tier. Every
     *            entry must be non-null, healthy, and report the mapped tier level.
     * @return true on success; false for an empty, inconsistent, unhealthy, or
     *         cache-only/storage-only registry.
     * @note Tier adapters are owned by the caller and must remain alive through
     *       shutdown().
     */
    virtual bool initialize(
        const std::map<TierLevel, std::shared_ptr<AccessTier>>& all_tiers) = 0;

    /**
     * @brief Start worker threads after successful initialization.
     * @note Calling start() more than once while running has no effect.
     */
    virtual void start() = 0;

    /**
     * @brief Stop accepting work, drain queued tasks, and join all workers.
     * @note A task already executing is allowed to finish; queued futures are
     *       completed before this method returns.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    virtual bool isRunning() const = 0;


    /**
     * @brief On Eviction.
     * @param[in] event Input parameter.
     */
    virtual void onEviction(const EvictionEvent& event) = 0;

    /**
     * @brief On Hot Access.
     * @param[in] event Input parameter.
     */
    virtual void onHotAccess(const AccessEvent& event) = 0;

    /**
     * @brief On Cache Evicted.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] size_bytes Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] last_access_age_secs Input parameter.
     * @param[in] eviction_reason Input parameter.
     * @details Calls: std::string(), onEviction().
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
     * @brief On Storage Access.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] access_count Input parameter.
     * @param[in] access_window Input parameter.
     * @details Calls: std::string(), onHotAccess().
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


    /**
     * @brief Set the validated age and access thresholds used by event handlers.
     * @param[in] policy Policy whose threshold ordering must satisfy isValid().
     * @throws std::invalid_argument if the policy is invalid; the current policy
     *         remains unchanged.
     */
    virtual void setAgePolicy(const AgeBasedPolicy& policy) = 0;

    /**
     * @brief Replace the cache and storage access-count thresholds.
     * @param[in] cache_threshold Non-zero L1 promotion threshold.
     * @param[in] storage_threshold Non-zero storage promotion threshold.
     * @throws std::invalid_argument if the resulting policy is invalid; no update
     *         is applied in that case.
     */
    virtual void setPromotionThresholds(uint64_t cache_threshold,
                                        uint64_t storage_threshold) = 0;


    /**
     * @brief Copy a key from a source tier into a destination tier asynchronously.
     * @param[in] key Non-empty key present in from_tier.
     * @param[in] from_tier Registered source tier; it is retained on success.
     * @param[in] to_tier Registered destination tier, distinct from from_tier.
     * @param[in] size_bytes Expected size used when the source reports no size.
     * @return A future completed with transfer status, including queue-full,
     *         missing-key, adapter, and shutdown failures.
     * @note The coordinator retains the promise until completion. A valid future
     *       is always completed, including when shutdown drains queued work.
     */
    virtual std::future<PromotionResult> promoteAsync(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t size_bytes) = 0;

    /**
     * @brief Create a one-shot plan to move a key to a lower tier.
     * @param[in] key Non-empty key expected in from_tier.
     * @param[in] from_tier Registered source tier.
     * @param[in] to_tier Registered destination tier, distinct from from_tier.
     * @param[in] data_size_bytes Expected size used if the source omits its size.
     * @return A plan on valid input; otherwise std::nullopt.
     */
    virtual std::optional<DemotionPlan> planDemotion(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t data_size_bytes) = 0;

    /**
     * @brief Move the planned key, invalidating the source only after destination
     *        placement succeeds.
     * @param[in] plan_id Identifier returned by planDemotion().
     * @return A result for a known plan, including adapter failures; std::nullopt
     *         if the identifier is unknown or has already been consumed.
     */
    virtual std::optional<DemotionResult> executeDemotion(
        const std::string& plan_id) = 0;


    /**
     * @brief Return retained access and transition counters for a key.
     * @param[in] key Exact key to query.
     * @return A metrics snapshot, or default metrics if the key is not retained.
     */
    virtual AccessMetrics getKeyMetrics(const std::string& key) = 0;

    /**
     * @brief Return access and transition counters for a registered tier.
     * @param[in] tier_level Tier to query.
     * @return A metrics snapshot, or default metrics if the tier is unknown.
     */
    virtual AccessMetrics getTierMetrics(TierLevel tier_level) = 0;

    /**
     * @brief Return coordinator counters and latency histograms.
     * @return A consistent snapshot including current and maximum pending queue
     *         depth.
     */
    virtual AccessModelMetrics getAccessModelMetrics() = 0;

    /**
     * @brief Return the most recent tier transition events.
     * @param[in] limit Maximum number of events to return (default: 100).
     * @return Vector of AccessTransitionEvent records, ordered newest-first.
     */
    virtual std::vector<AccessTransitionEvent> getRecentTransitions(
        std::size_t limit = 100) = 0;
};

// ============================================================================
// § 4  Factory Function
// ============================================================================

/**
 * @brief Create a new AccessCoordinator instance backed by a thread pool.
 * @param[in] thread_pool_size Number of background worker threads for async
 *            promotion/demotion processing (default: 4).
 * @return Shared pointer to the created AccessCoordinator; never null.
 */
std::shared_ptr<AccessCoordinator> createAccessCoordinator(
    std::size_t thread_pool_size = 4);

/**
 * @brief Create an access coordinator with explicit worker and queue limits.
 * @param[in] thread_pool_size Number of worker threads; must be greater than zero.
 * @param[in] max_queue_depth Maximum number of waiting tasks; must be greater than zero.
 * @return Shared pointer to a coordinator with bounded task admission.
 * @throws std::invalid_argument if either limit is zero.
 */
std::shared_ptr<AccessCoordinator> createAccessCoordinator(
    std::size_t thread_pool_size, std::size_t max_queue_depth);

}  // namespace access_model
}  // namespace themis
