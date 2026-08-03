/**
 * @file age_based_policy.h
 * @brief Unified age-based migration policy for cache & storage tiers.
 *
 * ThemisDB | File: age_based_policy.h | Version: 2.0.0
 * Maturity: 🟡 ALPHA (Phase 2 Implementation) | Status: Active development
 * Author: Copilot | Date: 2026-08-03
 *
 * The `AgeBasedPolicy` is the single source of truth for age-based tier
 * migration decisions. Both cache and storage tiers use it to determine
 * when data should be promoted to or demoted from a tier.
 *
 * @see include/access_model/access_coordinator.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 */

#pragma once

#include "access_tier_interface.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Data Hotness Classification
// ============================================================================

/**
 * @brief Classification of data hotness based on access frequency and age.
 *
 * Used by `AgeBasedPolicy::classifyHotness()` to drive tier placement decisions.
 */
enum class DataHotnessLevel {
    HOT,   ///< Very recent, high-frequency access — keep in L1 cache
    WARM,  ///< Moderate age and frequency — keep in L2/L3 cache
    COOL,  ///< Older with low frequency — move to warm storage
    COLD,  ///< Rarely accessed, very old — archive to cold storage
};

// ============================================================================
// § 2  Age-Based Policy
// ============================================================================

/**
 * @brief Unified age-based migration policy.
 *
 * Defines thresholds for demotion and promotion based on data age and
 * access recency. Applied uniformly across cache and storage tiers via
 * `AccessCoordinator`.
 */
struct AgeBasedPolicy {
    /// ────────────────────────────────────────────────────────────────────
    /// Cache Tier Age Thresholds (demotion triggers)
    /// ────────────────────────────────────────────────────────────────────

    /// Days since last access before L1→L2 demotion (0 = disabled)
    uint32_t l1_zero_access_days = 1;

    /// Days since last access before L2→L3 demotion (0 = disabled)
    uint32_t l2_zero_access_days = 7;

    /// Days since write before L3→storage demotion (0 = disabled)
    uint32_t l3_to_storage_days = 14;

    /// ────────────────────────────────────────────────────────────────────
    /// Storage Tier Age Thresholds (demotion triggers)
    /// ────────────────────────────────────────────────────────────────────

    /// Days since last access before hot→warm demotion (0 = disabled)
    uint32_t hot_zero_access_days = 14;

    /// Days since write before hot→warm demotion (0 = disabled)
    uint32_t hot_to_warm_days = 30;

    /// Days since last access before warm→cold demotion (0 = disabled)
    uint32_t warm_zero_access_days = 45;

    /// Days since write before warm→cold demotion (0 = disabled)
    uint32_t warm_to_cold_days = 90;

    /// ────────────────────────────────────────────────────────────────────
    /// Access-Count Promotion Thresholds
    /// ────────────────────────────────────────────────────────────────────

    /// Minimum access count to retain data in L1 (below this → demotion candidate)
    uint64_t l1_promotion_threshold = 10;

    /// Minimum access count to retain data in L2
    uint64_t l2_promotion_threshold = 5;

    /// Minimum access count to retain data in L3 cache
    uint64_t l3_promotion_threshold = 2;

    /// Minimum access count in window to promote storage→cache
    uint64_t storage_promotion_threshold = 3;

    /// Time window for measuring storage access frequency
    std::chrono::seconds storage_promotion_window{86400};  // 24 hours

    /// Grace period after cache warmup during which demotion is suppressed (ms)
    uint64_t cache_warmup_grace_period_ms = 0;

    // ── Inline Age-Check Helpers (fast path, called from hot loops) ──────────

    /**
     * @brief Check if data in L1 should be demoted to L2 based on age.
     * @param seconds_since_access Time in seconds since last access
     */
    bool shouldDemoteL1ToL2(uint32_t seconds_since_access) const noexcept {
        if (l1_zero_access_days == 0) return false;
        return seconds_since_access > (l1_zero_access_days * 86400u);
    }

    /**
     * @brief Check if data in L2 should be demoted to L3 based on age.
     */
    bool shouldDemoteL2ToL3(uint32_t seconds_since_access) const noexcept {
        if (l2_zero_access_days == 0) return false;
        return seconds_since_access > (l2_zero_access_days * 86400u);
    }

    /**
     * @brief Check if data in hot storage should be demoted to warm.
     * @param seconds_since_access Time since last access
     * @param seconds_since_write Time since initial write
     */
    bool shouldDemoteHotToWarm(uint32_t seconds_since_access,
                               uint32_t seconds_since_write) const noexcept {
        if (hot_zero_access_days > 0 &&
            seconds_since_access > (hot_zero_access_days * 86400u)) {
            return true;
        }
        if (hot_to_warm_days > 0 && seconds_since_write > (hot_to_warm_days * 86400u)) {
            return true;
        }
        return false;
    }

    /**
     * @brief Check if data in warm storage should be demoted to cold.
     */
    bool shouldDemoteWarmToCold(uint32_t seconds_since_access,
                                uint32_t seconds_since_write) const noexcept {
        if (warm_zero_access_days > 0 &&
            seconds_since_access > (warm_zero_access_days * 86400u)) {
            return true;
        }
        if (warm_to_cold_days > 0 &&
            seconds_since_write > (warm_to_cold_days * 86400u)) {
            return true;
        }
        return false;
    }

    /**
     * @brief Check if data should be promoted from storage to cache.
     * @param access_count Observed access count in the measurement window
     */
    bool shouldPromoteStorageToCache(uint64_t access_count) const noexcept {
        return access_count >= storage_promotion_threshold;
    }

    // ── Non-inline Methods (implemented in age_based_policy.cpp) ────────────

    /**
     * @brief Check if data in L1 should migrate to L2 based on access count
     *        and recency.
     * @param access_count Current access count
     * @param time_since_last_access Age of the last access
     */
    bool shouldPromoteL1ToL2(uint64_t access_count,
                             const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Check if data in L2 should migrate to L3 based on access count
     *        and recency.
     */
    bool shouldPromoteL2ToL3(uint64_t access_count,
                             const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Check if data in L3 cache should be evicted to storage.
     */
    bool shouldPromoteL3ToStorage(uint64_t access_count,
                                  const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Check if data in warm storage should be demoted to cold.
     * @param access_count Current access count in the warm tier
     * @param time_since_last_access Age of the last access
     */
    bool shouldPromoteStorageWarmToCold(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Check if cold storage data is a deletion/archival candidate.
     */
    bool shouldDemoteStorageCold(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Check if cold storage data should be promoted to warm.
     * @param access_count Observed access count in the measurement window
     */
    bool shouldPromoteStorageColdToWarm(uint64_t access_count) const;

    /**
     * @brief Check if warm storage data should be promoted to L3 cache.
     * @param access_count Observed access count in the measurement window
     */
    bool shouldPromoteStorageWarmToL3Cache(uint64_t access_count) const;

    /**
     * @brief Classify data hotness based on access frequency and age.
     *
     * @param access_count Current access count
     * @param time_since_last_access Age of the last access
     * @return Hotness classification (HOT / WARM / COOL / COLD)
     */
    DataHotnessLevel classifyHotness(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Recommend the optimal tier for data given its access pattern.
     *
     * @param access_count Current access count
     * @param time_since_last_access Age of the last access
     * @return Recommended `TierLevel`; returns `TierLevel::STORAGE_COLD` if no
     *         better tier can be determined
     */
    TierLevel recommendTierForData(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Validate that all threshold fields are internally consistent.
     *
     * Logs a warning and returns false if:
     *   - Any access threshold is zero
     *   - Access thresholds are not in decreasing order (L1 > L2 > L3)
     *   - Storage age thresholds are not in increasing order (hot < warm)
     */
    bool isValid() const;

    /**
     * @brief Serialize the policy to a compact JSON string.
     */
    std::string toJson() const;

    /**
     * @brief Return a human-readable description of the policy.
     */
    std::string describe() const;
};

}  // namespace access_model
}  // namespace themis
