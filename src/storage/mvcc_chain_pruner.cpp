/**
 * @file mvcc_chain_pruner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//

#include "storage/mvcc_chain_pruner.h"
#include "storage/mvcc_snapshot_pins.h"
#include "temporal/temporal_types.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <shared_mutex>
#include <sstream>
#include <vector>

namespace themis {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

MVCCChainPruner::MVCCChainPruner(
    std::shared_ptr<MVCCStore>                               mvcc,
    std::shared_ptr<themisdb::temporal::TemporalTierManager> tier_manager
)
    : mvcc_(std::move(mvcc))
    , tier_manager_(std::move(tier_manager))
{}

// ─────────────────────────────────────────────────────────────────────────────
// Per-key pruning
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Prune Key.
 * @param[in] key Input parameter.
 * @param[in] gc_horizon Input parameter.
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: scanVersions(), push_back(), size(), std::min(), key_str(), valueToDocument(), toTemporalTs(), insert().
 */
MVCCChainPruner::PruneStats MVCCChainPruner::pruneKey(
    std::string_view key,
    HLCTimestamp     gc_horizon,
    Config           config
) {
    PruneStats stats;
    stats.keys_scanned = 1;

    // Snapshot-pin interlock: hold the pin registry's GC guard (shared) for
    // the whole select → migrate → delete sequence of this key so that a
    // concurrently acquired snapshot pin cannot be bypassed.
    auto pins = mvcc_->snapshotPinRegistry();
    std::shared_lock<std::shared_mutex> pin_guard;
    std::vector<HLCTimestamp> pinned;
    if (pins) {
        pin_guard = pins->gcGuard();
        pinned = pins->pinnedTimestamps();
    }

    // 1. Collect all MVCC versions for this key in ascending timestamp order.
    struct RawVersion {
        HLCTimestamp         ts;
        std::vector<uint8_t> value;
    };
    std::vector<RawVersion> all_versions;
    // copy_overhead scanner alert: all_versions is populated inside a
    // scanVersions callback; the total version count is not known before the
    // scan completes, so reserve() is not applicable here.
    mvcc_->scanVersions(key, [&](const MVCCStore::VersionEntry& e) -> bool {
        all_versions.push_back({e.timestamp, e.value});
        return true;  // ascending order guaranteed by MVCCStore
    });

    const auto total = all_versions.size();
    if (total == 0) {
        return stats;
    }

    // 2. Determine which versions may be pruned: ts < gc_horizon, not among
    //    the newest min_versions_to_keep, and not visible at any pinned
    //    snapshot timestamp (shared rule: MVCCStore::selectGcVictims).
    const uint32_t min_keep =
        (config.min_versions_to_keep > 0) ? config.min_versions_to_keep : 1;
    std::vector<HLCTimestamp> chain_ts;
    chain_ts.reserve(total);
    for (const auto& v : all_versions) {
        chain_ts.push_back(v.ts);
    }
    const auto victims = MVCCStore::selectGcVictims(chain_ts, gc_horizon, min_keep, pinned);
    if (victims.empty()) {
        return stats;
    }

    // 3. Migrate exactly the victim versions into the TemporalTierManager,
    //    then delete exactly those from the MVCC store.
    //
    //    sys_time.start = this version's timestamp
    //    sys_time.end   = next version's timestamp (the moment this version
    //                     was superseded).  Victims never include the last
    //                     version (min_keep ≥ 1), so a successor always exists.

    std::string key_str(key);
    std::vector<HLCTimestamp> victim_ts;
    victim_ts.reserve(victims.size());

    for (size_t i : victims) {
        const auto& cur  = all_versions[i];
        const auto& next = all_versions[i + 1];  // safe: i < total - min_keep

        using namespace themisdb::temporal;

        VersionedDocument doc;
        doc.key         = key_str;
        doc.data        = valueToDocument(cur.value);
        doc.sys_time    = TimeRange{toTemporalTs(cur.ts), toTemporalTs(next.ts)};
        doc.valid_time  = doc.sys_time;  // MVCC has no application-time
        doc.modified_by = config.modified_by;

        tier_manager_->insert(config.table_name, doc);
        ++stats.versions_migrated;
        victim_ts.push_back(cur.ts);
    }

    // 4. Delete the migrated versions (and only those) from the MVCC store.
    const uint64_t deleted = mvcc_->deleteVersions(key, victim_ts);
    stats.versions_deleted = deleted;

    if (deleted > 0) {
        ++stats.keys_pruned;
    }

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Full-store pruning
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Prune All.
 * @param[in] gc_horizon Input parameter.
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: scanBaseKeys(), pruneKey().
 */
MVCCChainPruner::PruneStats MVCCChainPruner::pruneAll(
    HLCTimestamp  gc_horizon,
    Config        config
) {
    PruneStats total;

    mvcc_->scanBaseKeys([&](std::string_view base_key) -> bool {
        total += pruneKey(base_key, gc_horizon, config);
        return true;
    });

    return total;
}

// ─────────────────────────────────────────────────────────────────────────────
// Safe-horizon management
// ─────────────────────────────────────────────────────────────────────────────

HLCTimestamp MVCCChainPruner::safeHorizon() const noexcept {
    /**
     * @brief Lk.
     * @param[in] horizon_mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(horizon_mu_);
    return safe_horizon_;
}

void MVCCChainPruner::setSafeHorizon(HLCTimestamp horizon) noexcept {
    /**
     * @brief Lk.
     * @param[in] horizon_mu_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lk(horizon_mu_);
    if (horizon > safe_horizon_) {
        safe_horizon_ = horizon;
    }
}

/**
 * @brief Prune All Safe.
 * @param[in] config Input parameter.
 * @return Return value.
 * @details Calls: safeHorizon(), pruneAll().
 */
MVCCChainPruner::PruneStats MVCCChainPruner::pruneAllSafe(Config config) {
    const HLCTimestamp h = safeHorizon();
    if (h.value == 0) {
        return {};
    }
    return pruneAll(h, config);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Value To Document.
 * @param[in] raw Input parameter.
 * @return Return value.
 * @details Calls: empty(), data(), nlohmann::json::parse(), size(), std::setfill(), std::setw(), str().
 */
themisdb::temporal::Document MVCCChainPruner::valueToDocument(
    const std::vector<uint8_t>& raw
) {
    if (!raw.empty()) {
        const auto* cbegin = reinterpret_cast<const char*>(raw.data());
        try {
            return nlohmann::json::parse(cbegin, cbegin + raw.size() );
        } catch (const nlohmann::json::exception&) {
            // Fall through: encode as hex string.
        }
    }

    // Hex-encode non-JSON bytes for lossless round-trip.
    std::ostringstream hex = {};
    hex << std::hex << std::setfill('0');
    for (const auto b : raw) {
        hex << std::setw(2) << static_cast<unsigned>(b);
    }
    return nlohmann::json{{"_raw", hex.str()}};
}

themisdb::temporal::Timestamp MVCCChainPruner::toTemporalTs(
    HLCTimestamp ts
) noexcept {
    // HLCTimestamp::value is uint64_t; cast to int64_t.
    // HLC timestamps are monotonically increasing and the encoded value
    // stays below INT64_MAX for physical clocks within the year 2262, so
    // the cast is safe for any realistic deployment lifetime.
    return static_cast<themisdb::temporal::Timestamp>(ts.value);
}

} // namespace themis
