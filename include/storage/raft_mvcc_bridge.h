/**
 * @file raft_mvcc_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "storage/mvcc_store.h"
#include "storage/hlc.h"
#include "sharding/distributed_time_coordinator.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {

/**
 * @brief Bridge between the Raft consensus layer and the per-shard MVCC store.
 *
 * In a distributed ThemisDB cluster each shard runs its own `MVCCStore` whose
 * writes are stamped with `HybridLogicalClock` (HLC) timestamps.  The cluster
 * also runs a Raft consensus group managed through `DistributedTimeCoordinator`
 * which assigns causally-ordered _log-index_ timestamps to committed entries.
 *
 * This class bridges the two timestamp spaces to enable:
 *
 * ### Linearizable reads (across the Raft group)
 * A linearizable read is guaranteed to see all writes that completed before the
 * read was issued.  It must be served by the Raft **leader** (or by a follower
 * that has confirmed the leader's commit index via a lease read).
 *
 * ```cpp
 * auto [value, ok] = bridge.linearizableRead("user:42");
 * // ok == false if this node is not the leader → caller must retry on leader
 * ```
 *
 * ### Snapshot reads at a Raft-consistent timestamp
 * A snapshot read returns a consistent view of the data as of the moment the
 * Raft commit index was last advanced.  The bridge converts the Raft commit
 * index into an HLC timestamp that is guaranteed to be ≥ any HLC timestamp of
 * a write that was reflected in that commit index.
 *
 * ```cpp
 * HLCTimestamp snap_ts = bridge.snapshotTimestamp();
 * auto val = bridge.snapshotRead("user:42", snap_ts);
 * ```
 *
 * ### Raft-aware writes
 * After proposing a new value to Raft, the bridge calls `putWithTimestamp` on
 * the MVCCStore using the HLC timestamp at the moment of the Raft proposal.
 * This keeps the HLC and the Raft log causally consistent.
 *
 * ```cpp
 * HLCTimestamp commit_ts = bridge.raftAwareWrite("user:42", new_value);
 * ```
 *
 * ## Timestamp conversion
 *
 * `DistributedTimeCoordinator::now()` returns a `TimeInterval` whose fields are:
 *   - `logical_timestamp` : current Raft last-log-index
 *   - `system_time_ns`    : wall-clock nanoseconds since epoch
 *   - `uncertainty_ns`    : upper bound on clock skew
 *
 * This bridge converts that to an `HLCTimestamp` as follows:
 *   - `physical_ms = system_time_ns / 1e6`  (wall-clock ms)
 *   - `logical     = logical_timestamp & 0xFFFFF`  (lower 20 bits of log index)
 *
 * The resulting HLC timestamp is then passed through
 * `HybridLogicalClock::update(synthetic)` so the local clock advances
 * monotonically.
 *
 * @note The bridge does **not** own either the MVCCStore or the coordinator.
 *       Both must outlive this object.
 */
class RaftMvccBridge {
public:
    /**
     * @brief Result of a linearizable read.
     *
     * If `is_leader` is false the read cannot be served here and must be
     * redirected to the current Raft leader.
     */
    struct LinearizableResult {
        bool                              is_leader = false;
        std::optional<std::vector<uint8_t>> value;
    };

    /**
     * @brief Construct a RaftMvccBridge.
     *
     * @param mvcc_store  Shared MVCC store for this shard.
     * @param coordinator Shared DistributedTimeCoordinator backed by the Raft
     *                    consensus module for this shard.
     */
    RaftMvccBridge(
        std::shared_ptr<MVCCStore>                          mvcc_store,
        std::shared_ptr<themisdb::sharding::DistributedTimeCoordinator> coordinator
    );

    // ─── Timestamp conversion ─────────────────────────────────────────────────

    /**
     * @brief Produce an HLC snapshot timestamp consistent with the Raft commit
     *        index.
     *
     * Advances the MVCCStore's HLC to be ≥ the synthetic HLC derived from the
     * coordinator's current view (system wall-clock + Raft log index).  The
     * returned timestamp can be used directly in `snapshotRead()` or
     * `MVCCStore::getAtTimestamp()`.
     *
     * @return HLC timestamp consistent with the current Raft commit point.
     */
    HLCTimestamp snapshotTimestamp();

    /**
     * @brief Convert a Raft `TimeInterval` to an HLC timestamp.
     *
     * This is a pure static conversion — it does **not** advance any clock.
     * Use `snapshotTimestamp()` if you also need to advance the local clock.
     *
     * @param interval  A `TimeInterval` returned by `DistributedTimeCoordinator::now()`.
     * @return Equivalent `HLCTimestamp` with physical=wall_ms, logical=log_idx&0xFFFFF.
     */
    static HLCTimestamp toHlcTimestamp(
        const themisdb::sharding::DistributedTimeCoordinator::TimeInterval& interval
    );

    // ─── Reads ────────────────────────────────────────────────────────────────

    /**
     * @brief Linearizable read: only succeeds on the Raft leader.
     *
     * Returns the latest committed version of @p key.  If this node is not the
     * Raft leader, `is_leader` will be `false` and `value` will be empty —
     * the caller must retry on the leader returned by
     * `DistributedTimeCoordinator` / its underlying `ConsensusModule`.
     *
     * @param key  The logical record key.
     * @return `LinearizableResult` with `is_leader` and optionally `value`.
     */
    LinearizableResult linearizableRead(std::string_view key);

    /**
     * @brief Snapshot read at a specific HLC timestamp.
     *
     * Reads the most-recent version of @p key committed at or before @p ts.
     * Safe to call on any replica (leader or follower) — data visibility is
     * determined entirely by @p ts.
     *
     * @param key  The logical record key.
     * @param ts   The snapshot point (from `snapshotTimestamp()`).
     * @return The value bytes, or `std::nullopt` if no version exists ≤ ts.
     */
    std::optional<std::vector<uint8_t>> snapshotRead(
        std::string_view key,
        HLCTimestamp     ts
    );

    // ─── Writes ───────────────────────────────────────────────────────────────

    /**
     * @brief Raft-aware write: stamps the value with a Raft-consistent HLC.
     *
     * Obtains the current HLC advanced to the Raft coordinator's view of time
     * and calls `MVCCStore::putWithTimestamp` so the write is visible to
     * snapshot reads that use `snapshotTimestamp()` *after* this call returns.
     * This call is leader-only and throws if invoked on a follower.
     *
     * @param key    The logical record key.
     * @param value  Value bytes to store.
     * @return The HLC timestamp assigned to this write.
     * @throws std::runtime_error if this node is not the current Raft leader.
     */
    HLCTimestamp raftAwareWrite(
        std::string_view             key,
        const std::vector<uint8_t>&  value
    );

    // ─── Accessors ────────────────────────────────────────────────────────────

    /** @return Whether this node is the current Raft leader. */
    bool isLeader() const;

    /** @return The underlying MVCC store. */
    std::shared_ptr<MVCCStore> mvccStore() const { return mvcc_store_; }

    /** @return The underlying DistributedTimeCoordinator. */
    std::shared_ptr<themisdb::sharding::DistributedTimeCoordinator>
    coordinator() const { return coordinator_; }

private:
    std::shared_ptr<MVCCStore>                                         mvcc_store_;
    std::shared_ptr<themisdb::sharding::DistributedTimeCoordinator>   coordinator_;
};

} // namespace themis
