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

struct EvictionEvent {
    std::string key;

    TierLevel tier = TierLevel::UNKNOWN;

    std::string reason;

    std::size_t evicted_size_bytes = 0;

    uint64_t access_count = 0;

    std::chrono::seconds last_access_age_secs{0};

    std::string correlation_id;
};

struct AccessEvent {
    std::string key;

    TierLevel current_tier = TierLevel::UNKNOWN;

    uint64_t access_count = 0;

    std::chrono::seconds access_window{86400};  // Default: 24 hours

    std::string correlation_id;
};

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

class AccessCoordinator {
public:
    /**
     * @brief Access Coordinator.
     * @return Return value.
     */
    virtual ~AccessCoordinator() = default;


    /**
     * @brief Initialize the coordinator with the full tier registry.
     * @param[in] all_tiers Map from TierLevel to the corresponding AccessTier instance.
     *            Must contain at least one cache tier and one storage tier.
     * @return true on success; false if any tier reference is null or configuration is invalid.
     */
    virtual bool initialize(
        const std::map<TierLevel, std::shared_ptr<AccessTier>>& all_tiers) = 0;

    /**
     * @brief Start.
     */
    virtual void start() = 0;

    /**
     * @brief Shutdown.
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
     * @brief Set Age Policy.
     * @param[in] policy Input parameter.
     */
    virtual void setAgePolicy(const AgeBasedPolicy& policy) = 0;

    /**
     * @brief Set Promotion Thresholds.
     * @param[in] cache_threshold Input parameter.
     * @param[in] storage_threshold Input parameter.
     */
    virtual void setPromotionThresholds(uint64_t cache_threshold,
                                        uint64_t storage_threshold) = 0;


    /**
     * @brief Promote Async.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] to_tier Input parameter.
     * @param[in] size_bytes Input parameter.
     * @return Return value.
     */
    virtual std::future<PromotionResult> promoteAsync(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t size_bytes) = 0;

    /**
     * @brief Plan Demotion.
     * @param[in] key Input parameter.
     * @param[in] from_tier Input parameter.
     * @param[in] to_tier Input parameter.
     * @param[in] data_size_bytes Input parameter.
     * @return Return value.
     */
    virtual std::optional<DemotionPlan> planDemotion(const std::string& key,
                                                     TierLevel from_tier,
                                                     TierLevel to_tier,
                                                     uint64_t data_size_bytes) = 0;

    /**
     * @brief Execute Demotion.
     * @param[in] plan_id Identifier of the plan.
     * @return Return value.
     */
    virtual std::optional<DemotionResult> executeDemotion(
        const std::string& plan_id) = 0;


    /**
     * @brief Get Key Metrics.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    virtual AccessMetrics getKeyMetrics(const std::string& key) = 0;

    /**
     * @brief Get Tier Metrics.
     * @param[in] tier_level Input parameter.
     * @return Return value.
     */
    virtual AccessMetrics getTierMetrics(TierLevel tier_level) = 0;

    /**
     * @brief Get Access Model Metrics.
     * @return Return value.
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

}  // namespace access_model
}  // namespace themis
