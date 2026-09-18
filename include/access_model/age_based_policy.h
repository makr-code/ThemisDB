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

enum class DataHotnessLevel {
    HOT,   ///< Very recent, high-frequency access — keep in L1 cache
    WARM,  ///< Moderate age and frequency — keep in L2/L3 cache
    COOL,  ///< Older with low frequency — move to warm storage
    COLD,  ///< Rarely accessed, very old — archive to cold storage
};

// ============================================================================
// § 2  Age-Based Policy
// ============================================================================

struct AgeBasedPolicy {

    uint32_t l1_zero_access_days = 1;

    uint32_t l2_zero_access_days = 7;

    uint32_t l3_to_storage_days = 14;


    uint32_t hot_zero_access_days = 14;

    uint32_t hot_to_warm_days = 30;

    uint32_t warm_zero_access_days = 45;

    uint32_t warm_to_cold_days = 90;


    uint64_t l1_promotion_threshold = 10;

    uint64_t l2_promotion_threshold = 5;

    uint64_t l3_promotion_threshold = 2;

    uint64_t storage_promotion_threshold = 3;

    std::chrono::seconds storage_promotion_window{86400};  // 24 hours

    uint64_t cache_warmup_grace_period_ms = 0;

    // ── Inline Age-Check Helpers (fast path, called from hot loops) ──────────

    bool shouldDemoteL1ToL2(uint32_t seconds_since_access) const noexcept {
        if (l1_zero_access_days == 0) {
          return false;
        }
        return seconds_since_access > (l1_zero_access_days * 86400u);
    }

    bool shouldDemoteL2ToL3(uint32_t seconds_since_access) const noexcept {
        if (l2_zero_access_days == 0) {
          return false;
        }
        return seconds_since_access > (l2_zero_access_days * 86400u);
    }

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

    bool shouldPromoteStorageToCache(uint64_t access_count) const noexcept {
        return access_count >= storage_promotion_threshold;
    }

    /**
     * @brief ── Non-inline Methods (implemented in age_based_policy.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return True when the operation succeeds.
     * @details cpp) ────────────
     */

    bool shouldPromoteL1ToL2(uint64_t access_count,
                             const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Should Promote L2 To L3.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPromoteL2ToL3(uint64_t access_count,
                             const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Should Promote L3 To Storage.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPromoteL3ToStorage(uint64_t access_count,
                                  const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Should Promote Storage Warm To Cold.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPromoteStorageWarmToCold(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Should Demote Storage Cold.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldDemoteStorageCold(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Should Promote Storage Cold To Warm.
     * @param[in] access_count Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPromoteStorageColdToWarm(uint64_t access_count) const;

    /**
     * @brief Should Promote Storage Warm To L3 Cache.
     * @param[in] access_count Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldPromoteStorageWarmToL3Cache(uint64_t access_count) const;

    /**
     * @brief Classify Hotness.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return Return value.
     */
    DataHotnessLevel classifyHotness(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Recommend Tier For Data.
     * @param[in] access_count Input parameter.
     * @param[in] time_since_last_access Input parameter.
     * @return Return value.
     */
    TierLevel recommendTierForData(
        uint64_t access_count,
        const std::chrono::seconds& time_since_last_access) const;

    /**
     * @brief Is Valid.
     * @return True when the operation succeeds.
     */
    bool isValid() const;

    /**
     * @brief To Json.
     * @return Return value.
     */
    std::string toJson() const;

    /**
     * @brief Describe.
     * @return Return value.
     */
    std::string describe() const;
};

}  // namespace access_model
}  // namespace themis
