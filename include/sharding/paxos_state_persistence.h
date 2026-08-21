/**
 * @file paxos_state_persistence.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "sharding/paxos_snapshot.h"
#include "sharding/paxos_wal.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace themisdb {
namespace sharding {

using LSN = themis::sharding::LSN;
using PaxosWAL = themis::sharding::PaxosWAL;
using PaxosSnapshotManager = themis::sharding::PaxosSnapshotManager;

// ─────────────────────────────────────────────────────────────────────────────
// DurableAcceptorState – what must survive a crash
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief The minimal acceptor state that must be persisted per Paxos slot.
 *
 * Paxos safety requires that once a node has promised a ballot number or
 * accepted a value, those facts must be recoverable after a crash.
 */
struct DurableAcceptorState {
    uint64_t slot            = 0;   ///< Paxos log slot
    uint64_t promised_round  = 0;   ///< Highest ballot number promised
    std::string promised_node;      ///< Node ID of the proposer
    uint64_t accepted_round  = 0;   ///< Ballot of the last accepted value
    std::string accepted_value;     ///< Serialised accepted value (empty = none)
    bool     is_committed    = false; ///< Whether the slot is already committed

    bool operator==(const DurableAcceptorState& o) const noexcept;
};

/**
 * @brief Process-scoped durable state (persisted across restarts).
 */
struct DurableNodeState {
    std::string node_id;             ///< This node's identifier
    uint64_t    current_round  = 0;  ///< Global proposal round counter
    uint64_t    last_committed = 0;  ///< Highest committed slot seen
    LSN         last_lsn       {};  ///< WAL LSN of the last persisted entry
};

// ─────────────────────────────────────────────────────────────────────────────
// PaxosStatePersistence
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Persistence façade for Paxos acceptor state.
 *
 * Usage:
 * ```cpp
 * PaxosStatePersistence persist(wal, snapshot_mgr, config);
 * persist.open();                                // Load on startup
 *
 * // On every PROMISE:
 * persist.persistPromise(slot, ballot);
 *
 * // On every ACCEPT:
 * persist.persistAccept(slot, ballot, value);
 *
 * // After commit:
 * persist.persistCommit(slot);
 *
 * // Periodic compaction (call from a background thread):
 * persist.maybeCompact();
 * ```
 */
class PaxosStatePersistence {
public:
    /**
     * @brief Configuration for the persistence layer.
     */
    struct Config {
        std::string state_dir      = "paxos_state"; ///< Directory for durable files
        size_t  compact_interval   = 512;   ///< Compact after this many committed slots
        bool    sync_on_write      = true;  ///< fsync after every write (crash safety)
        size_t  max_wal_entries    = 10000; ///< Trigger snapshot after N WAL entries
       std::chrono::milliseconds init_timeout{5000}; ///< FIXED: Timeout for state lock acquisition during initialization
    };

    PaxosStatePersistence(PaxosWAL*             wal,
                          PaxosSnapshotManager* snapshot_mgr);

    PaxosStatePersistence(PaxosWAL*             wal,
                          PaxosSnapshotManager* snapshot_mgr,
                          const Config&         config);
    ~PaxosStatePersistence() = default;

    // Non-copyable
    PaxosStatePersistence(const PaxosStatePersistence&)            = delete;
    PaxosStatePersistence& operator=(const PaxosStatePersistence&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Load durable state from disk on process startup.
     *
     * Reads the latest snapshot and replays the WAL tail.
     * Must be called before any other method.
     * @return true on success (empty state on first startup is also success).
     */
    bool open(const std::string& node_id);

    /**
     * @brief Flush and close all open handles.  Safe to call multiple times.
     */
    void close();

    // ── Persistence operations ────────────────────────────────────────────────

    /**
     * @brief Persist a Phase-1 promise (ballot upgrade) for @p slot.
     *
     * Must be called — and must complete — before sending a PROMISE response.
     */
    bool persistPromise(uint64_t slot, uint64_t ballot_round,
                        const std::string& proposer_node_id);

    /**
     * @brief Persist a Phase-2 accept for @p slot.
     *
     * Must be called — and must complete — before sending an ACCEPTED response.
     */
    bool persistAccept(uint64_t slot, uint64_t ballot_round,
                       const std::string& value);

    /**
     * @brief Record that @p slot has been committed.
     *
     * Triggers compaction if Config::compact_interval has been reached.
     */
    bool persistCommit(uint64_t slot);

    // ── Recovery ─────────────────────────────────────────────────────────────

    /**
     * @brief Retrieve the durable acceptor state for a slot after a restart.
     *
     * Returns std::nullopt when the slot is unknown (not yet seen or already
     * compacted into a snapshot).
     */
    std::optional<DurableAcceptorState> getAcceptorState(uint64_t slot) const;

    /**
     * @brief Return the recovered node-level state (round counter, commit index).
     */
    const DurableNodeState& nodeState() const noexcept { return node_state_; }

    // ── Compaction ────────────────────────────────────────────────────────────

    /**
     * @brief Take a snapshot and truncate the WAL if the compact threshold is met.
     *
     * Safe to call from a background thread.
     */
    void maybeCompact();

    /**
     * @brief Force a snapshot regardless of the compaction threshold.
     */
    bool forceCompact();

    bool isOpen() const noexcept { return is_open_.load(std::memory_order_relaxed); }

private:
    PaxosWAL*             wal_;
    PaxosSnapshotManager* snapshot_mgr_;
    Config                config_;

    mutable std::timed_mutex mutex_;
    std::atomic<bool>     is_open_{false};
    DurableNodeState      node_state_;
    uint64_t              commits_since_compact_ = 0;

    // In-memory acceptor state cache (slot → state)
    std::unordered_map<uint64_t, DurableAcceptorState> slot_cache_;

    void replayWal(LSN from_lsn);
};

} // namespace sharding
} // namespace themisdb
