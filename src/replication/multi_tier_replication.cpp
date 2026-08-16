/**
 * @file multi_tier_replication.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Multi-Tier Replication Manager Implementation  (v1.8.0)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "replication/multi_tier_replication.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace themisdb {
namespace replication {

// ============================================================================
// Built-in tier defaults
// ============================================================================

namespace {

/**
 * Return the built-in TierConfig for the given tier.
 *
 * These represent the recommended settings for each tier:
 *
 *   TIER_1_CRITICAL  – 3 replicas, SYNC, 10 ms SLA, 99% availability
 *   TIER_2_STANDARD  – 2 replicas, SEMI_SYNC, 50 ms SLA, 99% availability
 *   TIER_3_ARCHIVAL  – 1 replica,  ASYNC,   no SLA,  90% availability
 */
TierConfig builtinTierConfig(ReplicationTier tier) {
    switch (tier) {
    case ReplicationTier::TIER_1_CRITICAL: {
        TierConfig cfg;
        cfg.tier                 = ReplicationTier::TIER_1_CRITICAL;
        cfg.replica_count        = 3;
        cfg.mode                 = ReplicationMode::SYNC;
        cfg.max_latency_ms       = 10;
        cfg.min_availability_pct = 99;
        return cfg;
    }
    case ReplicationTier::TIER_2_STANDARD: {
        TierConfig cfg;
        cfg.tier                 = ReplicationTier::TIER_2_STANDARD;
        cfg.replica_count        = 2;
        cfg.mode                 = ReplicationMode::SEMI_SYNC;
        cfg.max_latency_ms       = 50;
        cfg.min_availability_pct = 99;
        return cfg;
    }
    case ReplicationTier::TIER_3_ARCHIVAL: {
        TierConfig cfg;
        cfg.tier                 = ReplicationTier::TIER_3_ARCHIVAL;
        cfg.replica_count        = 1;
        cfg.mode                 = ReplicationMode::ASYNC;
        cfg.max_latency_ms       = 0; // no guarantee
        cfg.min_availability_pct = 90;
        return cfg;
    }
    }
    // Unreachable – satisfy compiler
    return TierConfig{};
}

} // anonymous namespace

// ============================================================================
// Constructor
// ============================================================================

MultiTierReplicationManager::MultiTierReplicationManager(
    const MultiTierConfig& config)
    : config_(config)
    , auto_tiering_(config.auto_tiering_enabled)
{}

// ============================================================================
// Tier assignment
// ============================================================================

void MultiTierReplicationManager::assignTier(const std::string& collection,
                                             ReplicationTier    tier)
{
    if (collection.empty()) {
        THEMIS_WARN("MultiTierReplicationManager::assignTier: empty collection name ignored");
        return;
    }

    {
        std::unique_lock<std::shared_mutex> lk(assignments_mutex_);
        tier_assignments_[collection] = tier;
    }

    // Ensure an access-stats entry exists for this collection so that
    // getCollectionStats() returns it even before any access is recorded.
    {
        std::unique_lock<std::shared_mutex> lk(stats_mutex_);
        auto& stats      = access_stats_[collection];
        stats.collection = collection;
        stats.current_tier = tier;
    }

    THEMIS_INFO("MultiTierReplicationManager: collection '{}' assigned to tier {}",
                collection, static_cast<int>(tier));
}

void MultiTierReplicationManager::removeTier(const std::string& collection)
{
    {
        std::unique_lock<std::shared_mutex> lk(assignments_mutex_);
        tier_assignments_.erase(collection);
    }

    {
        std::unique_lock<std::shared_mutex> lk(stats_mutex_);
        auto it = access_stats_.find(collection);
        if (it != access_stats_.end()) {
            it->second.current_tier = config_.default_tier;
        }
    }
}

ReplicationTier MultiTierReplicationManager::getTier(
    const std::string& collection) const
{
    std::shared_lock<std::shared_mutex> lk(assignments_mutex_);
    auto it = tier_assignments_.find(collection);
    if (it != tier_assignments_.end()) {
        return it->second;
    }
    return config_.default_tier;
}

TierConfig MultiTierReplicationManager::getTierConfig(
    const std::string& collection) const
{
    return getDefaultTierConfig(getTier(collection));
}

TierConfig MultiTierReplicationManager::getDefaultTierConfig(
    ReplicationTier tier) const
{
    const std::optional<TierConfig>* override_ptr = nullptr;
    switch (tier) {
    case ReplicationTier::TIER_1_CRITICAL: override_ptr = &config_.tier1_config; break;
    case ReplicationTier::TIER_2_STANDARD: override_ptr = &config_.tier2_config; break;
    case ReplicationTier::TIER_3_ARCHIVAL: override_ptr = &config_.tier3_config; break;
    }

    if (override_ptr && override_ptr->has_value()) {
        return override_ptr->value();
    }
    return builtinTierConfig(tier);
}

// ============================================================================
// Auto-tiering
// ============================================================================

void MultiTierReplicationManager::enableAutoTiering(bool enabled)
{
    // Only update the atomic flag; config_ fields are immutable after construction
    // to avoid data races with concurrent readers.
    auto_tiering_.store(enabled);
    THEMIS_INFO("MultiTierReplicationManager: auto-tiering {}",
                enabled ? "enabled" : "disabled");
}

bool MultiTierReplicationManager::isAutoTieringEnabled() const
{
    return auto_tiering_.load();
}

void MultiTierReplicationManager::recordAccess(const std::string& collection)
{
    if (!auto_tiering_.load()) {
        return;
    }
    if (collection.empty()) {
        return;
    }

    // SCOPE FIX (BATCH 4 - Agent 3): Move getTier call outside stats_mutex_ scope
    // to avoid lock-order inversion: getTier() acquires assignments_mutex_,
    // so we must NOT hold stats_mutex_ while calling it.
    // BEFORE: Direct call inside lock created potential deadlock.
    // AFTER: Call getTier first, then hold stats_mutex_ for single atomic update.
    ReplicationTier tier = getTier(collection);  // Acquire only assignments_mutex_
    const auto now = std::chrono::system_clock::now();

    // SCOPE FIX: Now acquire stats_mutex_ for the actual update operation.
    // This ensures consistent variable lifetime and prevents deadlock.
    {
        std::unique_lock<std::shared_mutex> lk(stats_mutex_);
        auto& stats = access_stats_[collection];
        if (stats.collection.empty()) {
            // SCOPE FIX: Initialize all fields in the same scope where lock is held
            stats.collection   = collection;
            stats.current_tier = tier;
        }
        stats.total_accesses++;
        stats.access_timestamps.push_back(now);
    }
    // stats_mutex_ released here; no dangling references to stats
}

ReplicationTier MultiTierReplicationManager::evaluateTierPromotion(
    const std::string& collection)
{
    if (collection.empty()) {
        return config_.default_tier;
    }

    if (!auto_tiering_.load()) {
        return getTier(collection);
    }

    // SCOPE FIX (BATCH 4): Move getTier() and stats fetch outside separate scopes.
    // Prevents repeated lock acquisitions and ensures variable lifetime is correct.
    ReplicationTier current = getTier(collection);
    double rate = 0.0;

    // SCOPE FIX: Single stats_mutex_ scope to compute rate
    {
        std::unique_lock<std::shared_mutex> lk(stats_mutex_);
        auto& stats = access_stats_[collection];
        if (stats.collection.empty()) {
            stats.collection   = collection;
            stats.current_tier = current;
        }
        refreshAccessRate(stats);  // Compute rate while holding lock
        rate = stats.access_rate_per_min;  // Extract rate inside scope
    }
    // stats reference is invalid after lock release; rate is copied out

    ReplicationTier new_tier = current;

    if (rate >= static_cast<double>(config_.hot_access_threshold)) {
        // Hot data → promote to Tier 1
        new_tier = ReplicationTier::TIER_1_CRITICAL;
    } else if (rate < static_cast<double>(config_.cold_access_threshold)) {
        // Cold data → demote to Tier 3
        new_tier = ReplicationTier::TIER_3_ARCHIVAL;
    } else {
        // Moderate access → normalise to Tier 2
        new_tier = ReplicationTier::TIER_2_STANDARD;
    }

    if (new_tier != current) {
        applyTierChange(collection, current, new_tier);
    }

    return new_tier;
}

// ============================================================================
// Statistics
// ============================================================================

MultiTierStats MultiTierReplicationManager::getStats() const
{
    MultiTierStats s;
    s.total_promotions   = total_promotions_.load();
    s.total_demotions    = total_demotions_.load();
    s.auto_tiering_active = auto_tiering_.load();

    std::shared_lock<std::shared_mutex> lk(assignments_mutex_);
    for (const auto& [col, tier] : tier_assignments_) {
        switch (tier) {
        case ReplicationTier::TIER_1_CRITICAL: s.collections_tier1++; break;
        case ReplicationTier::TIER_2_STANDARD: s.collections_tier2++; break;
        case ReplicationTier::TIER_3_ARCHIVAL: s.collections_tier3++; break;
        }
    }
    return s;
}

std::vector<CollectionAccessStats>
MultiTierReplicationManager::getCollectionStats() const
{
    std::shared_lock<std::shared_mutex> lk(stats_mutex_);
    std::vector<CollectionAccessStats> result;
    result.reserve(access_stats_.size());
    for (const auto& [col, stats] : access_stats_) {
        result.push_back(stats);
    }
    return result;
}

std::vector<std::string>
MultiTierReplicationManager::getCollectionsForTier(ReplicationTier tier) const
{
    std::shared_lock<std::shared_mutex> lk(assignments_mutex_);
    std::vector<std::string> result;
    for (const auto& [col, t] : tier_assignments_) {
        if (t == tier) {
            result.push_back(col);
        }
    }
    return result;
}

// ============================================================================
// Internal helpers
// ============================================================================

void MultiTierReplicationManager::refreshAccessRate(
    CollectionAccessStats& stats) const
{
    const uint32_t window_secs =
        config_.auto_tier_window_seconds > 0 ? config_.auto_tier_window_seconds : 60;
    const auto cutoff = std::chrono::system_clock::now()
                      - std::chrono::seconds(window_secs);

    // SCOPE FIX (BATCH 4): Move expiration logic to minimize scope of iterator operations.
    // The while loop modifies access_timestamps, so we explicitly scope it.
    // BEFORE: Implicit iterator validity across loose code.
    // AFTER: Clear scoping of iterator validity in the loop.
    {
        // Expire timestamps older than the rolling window.
        // Use explicit loop to ensure proper iterator management.
        while (!stats.access_timestamps.empty() &&
               stats.access_timestamps.front() < cutoff) {
            stats.access_timestamps.pop_front();  // pop_front is safe on deque
        }
    }

    // SCOPE FIX: Compute derived values after expiration is complete.
    // This ensures size() reflects the current (non-expired) state.
    stats.recent_accesses   = stats.access_timestamps.size();
    const double window_min = static_cast<double>(window_secs) / 60.0;
    stats.access_rate_per_min =
        window_min > 0.0 ? static_cast<double>(stats.recent_accesses) / window_min : 0.0;
}

void MultiTierReplicationManager::applyTierChange(const std::string& collection,
                                                   ReplicationTier    old_tier,
                                                   ReplicationTier    new_tier)
{
    const bool is_promotion = (static_cast<int>(new_tier) < static_cast<int>(old_tier));

    {
        std::unique_lock<std::shared_mutex> lk(assignments_mutex_);
        tier_assignments_[collection] = new_tier;
    }

    {
        std::unique_lock<std::shared_mutex> lk(stats_mutex_);
        auto& stats      = access_stats_[collection];
        stats.current_tier = new_tier;
        if (is_promotion) {
            stats.last_promotion = std::chrono::system_clock::now();
        } else {
            stats.last_demotion = std::chrono::system_clock::now();
        }
        // Clear the rolling-window timestamps after a tier change so the
        // next evaluation starts fresh.
        stats.access_timestamps.clear();
    }

    if (is_promotion) {
        total_promotions_.fetch_add(1);
        THEMIS_INFO("MultiTierReplicationManager: collection '{}' promoted {} → {}",
                    collection,
                    static_cast<int>(old_tier),
                    static_cast<int>(new_tier));
    } else {
        total_demotions_.fetch_add(1);
        THEMIS_INFO("MultiTierReplicationManager: collection '{}' demoted {} → {}",
                    collection,
                    static_cast<int>(old_tier),
                    static_cast<int>(new_tier));
    }
}

} // namespace replication
} // namespace themisdb

