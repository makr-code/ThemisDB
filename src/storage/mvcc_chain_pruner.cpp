/**
 * @file mvcc_chain_pruner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License
//

#include "storage/mvcc_chain_pruner.h"
#include "temporal/temporal_types.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
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

MVCCChainPruner::PruneStats MVCCChainPruner::pruneKey(
    std::string_view key,
    HLCTimestamp     gc_horizon,
    Config           config
) {
    PruneStats stats;
    stats.keys_scanned = 1;

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

    // 2. Determine how many versions may be pruned.
    //    Candidates: ts < gc_horizon AND not among the newest min_versions_to_keep.
    const uint32_t min_keep =
        (config.min_versions_to_keep > 0) ? config.min_versions_to_keep : 1u;
    const uint64_t max_deletable =
        (total > min_keep) ? static_cast<uint64_t>(total - min_keep) : 0ULL;

    // Count how many versions are strictly below the horizon.
    uint64_t eligible = 0;
    for (const auto& v : all_versions) {
        if (v.ts < gc_horizon) {
            ++eligible;
        }
    }

    // We can prune at most max_deletable, and at most eligible.
    const uint64_t num_to_prune = std::min(eligible, max_deletable);
    if (num_to_prune == 0) {
        return stats;
    }

    // 3. Migrate the pruneable versions into the TemporalTierManager, then
    //    delete them from the MVCC store.
    //
    //    sys_time.start = this version's timestamp
    //    sys_time.end   = next version's timestamp (open interval: the moment
    //                     this version was superseded)
    //                   = kMaxTimestamp for the very last version if it stays
    //                     in MVCC (but that case is excluded by num_to_prune ≤
    //                     max_deletable, so we always have a successor).

    std::string key_str(key);

    for (uint64_t i = 0; i < num_to_prune; ++i) {
        const auto& cur  = all_versions[i];
        const auto& next = all_versions[i + 1];  // always safe: i < num_to_prune ≤ max_deletable ≤ total-1

        using namespace themisdb::temporal;

        VersionedDocument doc;
        doc.key         = key_str;
        doc.data        = valueToDocument(cur.value);
        doc.sys_time    = TimeRange{toTemporalTs(cur.ts), toTemporalTs(next.ts)};
        doc.valid_time  = doc.sys_time;  // MVCC has no application-time
        doc.modified_by = config.modified_by;

        tier_manager_->insert(config.table_name, doc);
        ++stats.versions_migrated;
    }

    // 4. Batch-delete the migrated versions from MVCC store in a single pass.
    //    gcVersionsBefore honours min_versions_to_keep so at most num_to_prune
    //    entries are removed.
    MVCCStore::GCOptions gc_opts;
    gc_opts.min_versions_to_keep = min_keep;
    const uint64_t deleted = mvcc_->gcVersionsBefore(key, gc_horizon, gc_opts);
    stats.versions_deleted = deleted;

    if (deleted > 0) {
        ++stats.keys_pruned;
    }

    return stats;
}

// ─────────────────────────────────────────────────────────────────────────────
// Full-store pruning
// ─────────────────────────────────────────────────────────────────────────────

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
    std::lock_guard<std::mutex> lk(horizon_mu_);
    return safe_horizon_;
}

void MVCCChainPruner::setSafeHorizon(HLCTimestamp horizon) noexcept {
    std::lock_guard<std::mutex> lk(horizon_mu_);
    if (horizon > safe_horizon_) {
        safe_horizon_ = horizon;
    }
}

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

themisdb::temporal::Document MVCCChainPruner::valueToDocument(
    const std::vector<uint8_t>& raw
) {
    if (!raw.empty()) {
        const auto* cbegin = reinterpret_cast<const char*>(raw.data());
        try {
            return nlohmann::json::parse(cbegin, cbegin + raw.size());
        } catch (const nlohmann::json::exception&) {
            // Fall through: encode as hex string.
        }
    }

    // Hex-encode non-JSON bytes for lossless round-trip.
    std::ostringstream hex;
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
