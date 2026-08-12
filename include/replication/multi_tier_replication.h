/**
 * @file multi_tier_replication.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Multi-Tier Replication Manager  (v1.8.0)
 *
 * Hierarchical replication with different consistency and durability tiers:
 *
 *   TIER_1_CRITICAL  – Strong consistency, high durability (3+ replicas, sync, <10ms)
 *   TIER_2_STANDARD  – Eventual consistency, moderate durability (2 replicas, semi-sync, <50ms)
 *   TIER_3_ARCHIVAL  – Best-effort, low durability (1 replica, async, no latency guarantee)
 *
 * Per-collection tier assignment is supported.  When auto-tiering is enabled,
 * the manager monitors per-collection access rates and automatically promotes
 * hot collections to TIER_1_CRITICAL or demotes cold collections to
 * TIER_3_ARCHIVAL.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <map>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <optional>
#include <cstdint>

#include "replication/replication_manager.h"  // ReplicationMode

namespace themisdb {
namespace replication {

// ============================================================================
// ReplicationTier
// ============================================================================

/**
 * Replication tier classification.
 *
 *   TIER_1_CRITICAL  – 3+ synchronous replicas; strong consistency; < 10 ms commit latency.
 *                      Suitable for financial transactions, user auth, order processing.
 *   TIER_2_STANDARD  – 2 semi-synchronous replicas; eventual consistency; < 50 ms.
 *                      Suitable for user profiles, product catalogs, session data.
 *   TIER_3_ARCHIVAL  – 1 asynchronous replica; best-effort durability; no latency guarantee.
 *                      Suitable for audit logs, analytics events, metrics.
 */
enum class ReplicationTier {
    TIER_1_CRITICAL = 1, ///< Strong consistency, high durability
    TIER_2_STANDARD = 2, ///< Eventual consistency, moderate durability
    TIER_3_ARCHIVAL = 3  ///< Best-effort, low durability
};

// ============================================================================
// TierConfig
// ============================================================================

/**
 * Per-tier replication configuration.
 */
struct TierConfig {
    ReplicationTier tier                  = ReplicationTier::TIER_2_STANDARD;
    uint32_t        replica_count         = 2;    ///< Number of replicas required
    ReplicationMode mode                  = ReplicationMode::SEMI_SYNC;
    uint32_t        max_latency_ms        = 50;   ///< SLA: maximum acceptable commit latency (ms)
    uint32_t        min_availability_pct  = 99;   ///< Minimum replica availability (%)
};

// ============================================================================
// MultiTierConfig
// ============================================================================

/**
 * Global configuration for MultiTierReplicationManager.
 */
struct MultiTierConfig {
    // Auto-tiering thresholds
    bool     auto_tiering_enabled         = false;
    uint32_t hot_access_threshold         = 100; ///< Accesses/min to trigger Tier 1 promotion
    uint32_t cold_access_threshold        = 5;   ///< Accesses/min below which Tier 3 demotion occurs
    uint32_t auto_tier_window_seconds     = 60;  ///< Rolling window for access rate computation

    // Default tier for collections not explicitly assigned
    ReplicationTier default_tier          = ReplicationTier::TIER_2_STANDARD;

    // Per-tier overrides (empty → built-in defaults used)
    std::optional<TierConfig> tier1_config;
    std::optional<TierConfig> tier2_config;
    std::optional<TierConfig> tier3_config;
};

// ============================================================================
// CollectionAccessStats
// ============================================================================

/**
 * Access statistics tracked per collection for auto-tiering decisions.
 */
struct CollectionAccessStats {
    std::string collection;
    uint64_t    total_accesses    = 0; ///< Lifetime access count
    uint64_t    recent_accesses   = 0; ///< Accesses within the last window (derived from access_timestamps)
    double      access_rate_per_min = 0.0; ///< Computed access rate (accesses/min over the rolling window)
    ReplicationTier current_tier  = ReplicationTier::TIER_2_STANDARD;
    std::chrono::system_clock::time_point last_promotion;
    std::chrono::system_clock::time_point last_demotion;
    /// Timestamps of individual accesses kept for rolling-window rate computation.
    /// Old entries (older than auto_tier_window_seconds) are expired on each evaluation.
    std::deque<std::chrono::system_clock::time_point> access_timestamps;
};

// ============================================================================
// MultiTierStats
// ============================================================================

/**
 * Aggregate statistics for MultiTierReplicationManager.
 */
struct MultiTierStats {
    uint32_t collections_tier1 = 0;  ///< Collections currently in Tier 1
    uint32_t collections_tier2 = 0;  ///< Collections currently in Tier 2
    uint32_t collections_tier3 = 0;  ///< Collections currently in Tier 3
    uint64_t total_promotions  = 0;  ///< Lifetime auto-tier promotion count
    uint64_t total_demotions   = 0;  ///< Lifetime auto-tier demotion count
    bool     auto_tiering_active = false;
};

// ============================================================================
// MultiTierReplicationManager
// ============================================================================

/**
 * MultiTierReplicationManager  (v1.8.0)
 *
 * Manages hierarchical replication with three durability/consistency tiers.
 *
 * Usage:
 * @code
 *   MultiTierConfig cfg;
 *   cfg.auto_tiering_enabled = true;
 *   MultiTierReplicationManager mgr(cfg);
 *
 *   mgr.assignTier("financial_transactions", ReplicationTier::TIER_1_CRITICAL);
 *   mgr.assignTier("user_profiles",          ReplicationTier::TIER_2_STANDARD);
 *   mgr.assignTier("audit_logs",             ReplicationTier::TIER_3_ARCHIVAL);
 *
 *   // Record accesses for auto-tiering
 *   mgr.recordAccess("audit_logs");
 *
 *   // Evaluate and apply tier changes
 *   mgr.evaluateTierPromotion("audit_logs");
 * @endcode
 *
 * Thread safety: all public methods are thread-safe.
 */
class MultiTierReplicationManager {
public:
    // ── Construction ─────────────────────────────────────────────────────────

    explicit MultiTierReplicationManager(const MultiTierConfig& config = {});
    ~MultiTierReplicationManager() = default;

    // Non-copyable, non-movable
    MultiTierReplicationManager(const MultiTierReplicationManager&) = delete;
    MultiTierReplicationManager& operator=(const MultiTierReplicationManager&) = delete;

    // ── Tier assignment ───────────────────────────────────────────────────────

    /**
     * Assign a collection to the specified replication tier.
     *
     * If the collection is already assigned to a different tier the assignment
     * is updated in-place.
     */
    void assignTier(const std::string& collection, ReplicationTier tier);

    /**
     * Remove a collection's explicit tier assignment.
     *
     * After removal getTier() returns the default_tier from the config.
     */
    void removeTier(const std::string& collection);

    /**
     * Get the current replication tier for a collection.
     *
     * Returns the explicitly assigned tier if one exists, otherwise the
     * default_tier from the MultiTierConfig.
     */
    ReplicationTier getTier(const std::string& collection) const;

    /**
     * Get the full TierConfig for a collection's current tier.
     */
    TierConfig getTierConfig(const std::string& collection) const;

    /**
     * Return the built-in (or overridden) TierConfig for the given tier.
     */
    TierConfig getDefaultTierConfig(ReplicationTier tier) const;

    // ── Auto-tiering ──────────────────────────────────────────────────────────

    /**
     * Enable or disable automatic tier promotion/demotion based on access
     * patterns.
     *
     * When enabled, `recordAccess()` updates per-collection counters and
     * `evaluateTierPromotion()` applies the promotion/demotion rules.
     */
    void enableAutoTiering(bool enabled);

    /** Returns true when auto-tiering is currently enabled. */
    bool isAutoTieringEnabled() const;

    /**
     * Record a single access to the collection.
     *
     * Has no effect when auto-tiering is disabled.  Thread-safe.
     */
    void recordAccess(const std::string& collection);

    /**
     * Evaluate and (if necessary) change the tier for the given collection
     * based on accumulated access statistics.
     *
     * Promotion rule:  access_rate >= hot_access_threshold  → TIER_1_CRITICAL
     * Demotion rule:   access_rate <  cold_access_threshold → TIER_3_ARCHIVAL
     * Otherwise:       no change (or revert to TIER_2_STANDARD if currently 1 or 3)
     *
     * Returns the new tier (which may be the same as the current tier).
     * Returns the current tier unchanged when auto-tiering is disabled.
     */
    ReplicationTier evaluateTierPromotion(const std::string& collection);

    // ── Statistics & introspection ────────────────────────────────────────────

    /** Return aggregate tier statistics. */
    MultiTierStats getStats() const;

    /**
     * Return per-collection access statistics.
     *
     * Only collections that have had at least one access recorded or an
     * explicit tier assignment are included.
     */
    std::vector<CollectionAccessStats> getCollectionStats() const;

    /**
     * Return all collections currently assigned to the given tier.
     *
     * Collections relying on the default tier are NOT included unless they have
     * been explicitly assigned.
     */
    std::vector<std::string> getCollectionsForTier(ReplicationTier tier) const;

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /**
     * Expire old timestamps outside the rolling window and recompute
     * access_rate_per_min and recent_accesses.
     * Must be called while holding the stats write lock.
     */
    void refreshAccessRate(CollectionAccessStats& stats) const;

    /**
     * Apply a tier change and update promotion/demotion counters.
     * Acquires assignments_mutex_ and stats_mutex_ internally.
     * Must NOT be called while holding either lock.
     */
    void applyTierChange(const std::string& collection,
                         ReplicationTier    old_tier,
                         ReplicationTier    new_tier);

    // ── State ─────────────────────────────────────────────────────────────────

    MultiTierConfig config_;

    mutable std::shared_mutex assignments_mutex_;
    std::map<std::string, ReplicationTier> tier_assignments_; ///< collection → tier

    mutable std::shared_mutex stats_mutex_;
    std::map<std::string, CollectionAccessStats> access_stats_; ///< collection → stats

    std::atomic<bool>     auto_tiering_{false};
    std::atomic<uint64_t> total_promotions_{0};
    std::atomic<uint64_t> total_demotions_{0};
};

} // namespace replication
} // namespace themisdb
