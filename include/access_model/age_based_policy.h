/**
 * @file age_based_policy.h
 * @brief Unified age-based migration policy for cache & storage tiers.
 *
 * ThemisDB | File: age_based_policy.h | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 1 API Definition) | Status: Frozen for v1.x
 * Author: Copilot | Date: 2026-08-03
 *
 * The `AgeBasedPolicy` is the single source of truth for age-based tier
 * migration decisions. Both cache and storage tiers use it to determine
 * when data should be demoted to lower tiers.
 *
 * @see include/access_model/access_coordinator.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 */

#pragma once

#include <chrono>
#include <cstdint>

namespace themis {
namespace access_model {

/**
 * @brief Unified age-based migration policy.
 *
 * Defines thresholds for demotion based on data age and access recency.
 * Applied uniformly across cache and storage tiers via `AccessCoordinator`.
 */
struct AgeBasedPolicy {
    /// ────────────────────────────────────────────────────────────────────
    /// Cache Tier Thresholds
    /// ────────────────────────────────────────────────────────────────────

    /// Days since last access before L1→L2 demotion (0 = disabled)
    uint32_t l1_zero_access_days = 1;

    /// Days since last access before L2→L3 demotion (0 = disabled)
    uint32_t l2_zero_access_days = 7;

    /// Days since write before L3→storage demotion (0 = disabled)
    uint32_t l3_to_storage_days = 14;

    /// ────────────────────────────────────────────────────────────────────
    /// Storage Tier Thresholds
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
    /// Promotion Thresholds (Inverse of Demotion)
    /// ────────────────────────────────────────────────────────────────────

    /// Number of accesses to keep data in L1 (before demotion consideration)
    uint64_t l1_promotion_threshold = 10;

    /// Number of accesses to keep data in L2
    uint64_t l2_promotion_threshold = 5;

    /// Number of accesses in window to promote storage→cache
    uint64_t storage_promotion_threshold = 3;

    /// Time window for measuring storage access frequency
    std::chrono::seconds storage_promotion_window{86400};  // 24 hours

    /// ────────────────────────────────────────────────────────────────────
    /// Utility Methods
    /// ────────────────────────────────────────────────────────────────────

    /**
     * @brief Check if data in L1 should be demoted based on age.
     *
     * @param seconds_since_access Time since last access
     * @return True if should be demoted to L2
     */
    bool shouldDemoteL1ToL2(uint32_t seconds_since_access) const {
        if (l1_zero_access_days == 0) return false;
        return seconds_since_access > (l1_zero_access_days * 86400);
    }

    /**
     * @brief Check if data in L2 should be demoted based on age.
     */
    bool shouldDemoteL2ToL3(uint32_t seconds_since_access) const {
        if (l2_zero_access_days == 0) return false;
        return seconds_since_access > (l2_zero_access_days * 86400);
    }

    /**
     * @brief Check if data in hot storage should be demoted based on age.
     */
    bool shouldDemoteHotToWarm(uint32_t seconds_since_access,
                               uint32_t seconds_since_write) const {
        if (hot_zero_access_days > 0 &&
            seconds_since_access > (hot_zero_access_days * 86400)) {
            return true;
        }
        if (hot_to_warm_days > 0 && seconds_since_write > (hot_to_warm_days * 86400)) {
            return true;
        }
        return false;
    }

    /**
     * @brief Check if data in warm storage should be demoted to cold.
     */
    bool shouldDemoteWarmToCold(uint32_t seconds_since_access,
                               uint32_t seconds_since_write) const {
        if (warm_zero_access_days > 0 &&
            seconds_since_access > (warm_zero_access_days * 86400)) {
            return true;
        }
        if (warm_to_cold_days > 0 &&
            seconds_since_write > (warm_to_cold_days * 86400)) {
            return true;
        }
        return false;
    }

    /**
     * @brief Check if data should be promoted from storage to cache.
     *
     * @param access_count Number of accesses in the window
     * @return True if promotion candidate
     */
    bool shouldPromoteStorageToCache(uint64_t access_count) const {
        return access_count >= storage_promotion_threshold;
    }

    /**
     * @brief Get human-readable description of policy.
     */
    std::string describe() const;
};

}  // namespace access_model
}  // namespace themis

#endif  // THEMISDB_INCLUDE_ACCESS_MODEL_AGE_BASED_POLICY_H
