/**
 * @file mvcc_chain_pruner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/mvcc_store.h"
#include "temporal/temporal_tier_manager.h"
#include "temporal/temporal_types.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace themis {

/**
 * @brief Migrates committed MVCC version chains into the TemporalTierManager
 *        and removes the migrated versions from the MVCC store.
 *
 * @see MVCCStore, themisdb::temporal::TemporalTierManager
 */
class MVCCChainPruner {
public:
    // ─── Configuration ────────────────────────────────────────────────────────

    /**
     * @brief Tuning parameters for a prune operation.
     */
    struct Config {
        /**
         * Number of most-recent MVCC versions to retain after pruning.
         * These versions are **not** migrated and remain in the MVCC store to
         * serve in-progress snapshot reads.  Must be ≥ 1.
         */
        uint32_t min_versions_to_keep;

        /**
         * Table name used when inserting migrated documents into the
         * TemporalTierManager.  All keys from a single prune run are stored
         * under the same table.
         */
        std::string table_name = {};

        /**
         * Value of `VersionedDocument::modified_by` on migrated documents.
         */
        std::string modified_by = {};

        Config()
            : min_versions_to_keep(1)
            , table_name("mvcc")
            , modified_by("mvcc_chain_pruner")
        {}
    };

    // ─── Results ──────────────────────────────────────────────────────────────

    /**
     * @brief Statistics returned by each prune call.
     */
    struct PruneStats {
        uint64_t keys_scanned      = 0;  ///< Distinct base keys examined
        uint64_t keys_pruned       = 0;  ///< Keys from which ≥1 version was pruned
        uint64_t versions_migrated = 0;  ///< Versions inserted into tier manager
        uint64_t versions_deleted  = 0;  ///< Versions removed from MVCC store

        PruneStats& operator+=(const PruneStats& o) noexcept {
            keys_scanned      += o.keys_scanned;
            keys_pruned       += o.keys_pruned;
            versions_migrated += o.versions_migrated;
            versions_deleted  += o.versions_deleted;
            return *this;
        }
    };

    // ─── Construction ─────────────────────────────────────────────────────────

    /**
     * @brief Construct a pruner backed by @p mvcc and @p tier_manager.
     *
     * Both references are retained as shared_ptr for the lifetime of the
     * pruner; the caller must ensure they remain valid.
     *
     * @param mvcc         The MVCC store to prune versions from.
     * @param tier_manager The temporal tier manager to migrate versions into.
     */
    MVCCChainPruner(
        std::shared_ptr<MVCCStore>                              mvcc,
        std::shared_ptr<themisdb::temporal::TemporalTierManager> tier_manager
    );

    // ─── Per-key pruning ──────────────────────────────────────────────────────

    /**
     * @brief Prune a single key.
     *
     * All MVCC versions of @p key with timestamp strictly less than
     * @p gc_horizon are migrated to the TemporalTierManager and then deleted
     * from the MVCC store.  The @p config.min_versions_to_keep most-recent
     * versions are protected from migration/deletion regardless of timestamp.
     *
     * @param key         The logical record key (base key, without timestamp).
     * @param gc_horizon  All versions with ts < gc_horizon are candidates.
     * @param config      Optional tuning parameters.
     * @return Statistics for this key.
     */
    PruneStats pruneKey(
        std::string_view key,
        HLCTimestamp     gc_horizon,
        Config           config = Config{}
    );

    // ─── Full-store pruning ───────────────────────────────────────────────────

    /**
     * @brief Prune every key in the MVCC store whose versions fall below
     *        @p gc_horizon.
     *
     * Internally enumerates all distinct base keys via
     * `MVCCStore::scanBaseKeys()`, then calls `pruneKey()` for each.  This is
     * an O(N) full-store scan; prefer per-key invocations in hot paths.
     *
     * @param gc_horizon  Versions older than this timestamp are migrated.
     * @param config      Optional tuning parameters.
     * @return Aggregate statistics across all pruned keys.
     */
    PruneStats pruneAll(HLCTimestamp gc_horizon, Config config = Config{});

    // ─── Safe-horizon management ──────────────────────────────────────────────

    /**
     * @brief Return the current safe GC horizon.
     *
     * Only versions strictly older than this timestamp can be pruned without
     * risk of breaking an active snapshot read.
     *
     * Returns a zero-valued HLCTimestamp by default, meaning "nothing is safe
     * to prune" until `setSafeHorizon()` has been called (e.g. by the
     * transaction manager after all active transactions are accounted for).
     */
    HLCTimestamp safeHorizon() const noexcept;

    /**
     * @brief Set the safe GC horizon.
     *
     * Typically called by the transaction manager after computing the oldest
     * active read timestamp across all open transactions.  The new horizon
     * must be monotonically non-decreasing; a smaller value is silently
     * ignored.
     *
     * @param horizon  New safe horizon.  All MVCC versions with timestamp
     *                 strictly less than this value are eligible for pruning.
     */
    void setSafeHorizon(HLCTimestamp horizon) noexcept;

    /**
     * @brief Prune all MVCC versions that are below the current safe horizon.
     *
     * Convenience wrapper: equivalent to `pruneAll(safeHorizon(), config)`.
     * Returns an empty `PruneStats` if `safeHorizon()` is zero.
     */
    PruneStats pruneAllSafe(Config config = Config{});

private:
    // ─── Helpers ──────────────────────────────────────────────────────────────

    /**
     * @brief Convert a raw MVCC VersionEntry value to a `nlohmann::json`
     *        Document for storage in TemporalTierManager.
     *
     * If @p raw contains valid UTF-8 JSON, it is parsed directly.
     * Otherwise the bytes are hex-encoded and wrapped as `{"_raw": "<hex>"}`.
     */
    static themisdb::temporal::Document valueToDocument(
        const std::vector<uint8_t>& raw
    );

    /**
     * @brief Convert an `HLCTimestamp` to the `temporal::Timestamp` (int64_t)
     *        used by TemporalTierManager.
     *
     * The raw uint64_t value is reinterpreted as int64_t.  HLC timestamps are
     * monotonically increasing and the physical component (physical_ms << 20)
     * stays well below INT64_MAX for any plausible wall-clock date.
     */
    static themisdb::temporal::Timestamp toTemporalTs(HLCTimestamp ts) noexcept;

    std::shared_ptr<MVCCStore>                               mvcc_;
    std::shared_ptr<themisdb::temporal::TemporalTierManager> tier_manager_;

    mutable std::mutex    horizon_mu_;
    HLCTimestamp          safe_horizon_{0};
};

} // namespace themis
