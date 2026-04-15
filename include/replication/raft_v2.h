/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_v2.h                                          ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 07:08:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     335                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 8bc8c37687  2026-03-10  feat(replication): implement Phase 4 – Raft v2, CRDT expa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file raft_v2.h
 * @brief Full Raft v2 implementation: joint consensus for safe cluster
 *        membership changes.
 *
 * Implements the joint-consensus protocol from the Raft PhD dissertation
 * (Ongaro 2014, §4.3) for the ThemisDB replication module.
 *
 * ## Joint Consensus for Membership Changes
 *
 * Safe membership changes require a two-phase transition:
 *  1. **C_old,new** – both the old and new configuration must agree
 *     (joint consensus).  A log entry carrying C_old,new is committed when
 *     a majority of *both* C_old and C_new acknowledge it.
 *  2. **C_new** – once C_old,new is committed the new configuration is
 *     activated and the old is discarded.  A log entry carrying C_new is
 *     committed by a majority of C_new alone.
 *
 * This ensures that at no point can two disjoint majorities be elected as
 * leaders simultaneously.
 *
 * ## Key Classes
 *
 * | Class | Responsibility |
 * |---|---|
 * | `RaftV2ClusterConfig` | Holds old + new member sets; calculates joint quorum |
 * | `MembershipChangeEntry` | Log entry representing a configuration change |
 * | `MembershipChangeManager` | Orchestrates the two-phase membership transition |
 * | `RaftV2State` | Persistent per-node state (currentTerm, votedFor, log) |
 *
 * ## Compatibility
 *
 * All new fields are additive only; the existing `ReplicationConfig` struct
 * is untouched and continues to work for clusters that do not use dynamic
 * membership changes.
 *
 * Thread Safety: all public methods of all classes are thread-safe.
 */

#pragma once

#include "replication/replication_manager.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations
// ─────────────────────────────────────────────────────────────────────────────

class MembershipChangeManager;

// ─────────────────────────────────────────────────────────────────────────────
// RaftV2ClusterConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Represents the current cluster configuration in Raft v2.
 *
 * During a joint-consensus transition the object holds *both* the old and the
 * new member sets.  A majority is only granted when a majority in *each* set
 * independently has voted.
 */
class RaftV2ClusterConfig {
public:
    /**
     * @brief Create a stable (non-transitional) configuration.
     * @param members Initial set of voting member node IDs.
     */
    explicit RaftV2ClusterConfig(const std::set<std::string>& members = {});

    // ── Mutation ────────────────────────────────────────────────────────────

    /**
     * @brief Begin a joint-consensus transition to add @p node_id.
     * @throws std::runtime_error if a membership change is already in flight.
     */
    void beginAddMember(const std::string& node_id);

    /**
     * @brief Begin a joint-consensus transition to remove @p node_id.
     * @throws std::runtime_error if a membership change is already in flight
     *         or if the resulting cluster would have fewer than 1 member.
     */
    void beginRemoveMember(const std::string& node_id);

    /**
     * @brief Commit the pending transition, activating C_new.
     * @throws std::runtime_error if not in joint consensus.
     */
    void commitTransition();

    /**
     * @brief Roll back the pending transition, restoring C_old.
     * @throws std::runtime_error if not in joint consensus.
     */
    void rollbackTransition();

    // ── Queries ─────────────────────────────────────────────────────────────

    /** @brief True while a joint-consensus transition is in flight. */
    bool isInJointConsensus() const;

    /**
     * @brief Returns true if @p node_id is a voting member in either
     *        C_old or C_new (or both).
     */
    bool isMember(const std::string& node_id) const;

    /**
     * @brief Returns the union of old and new members during joint consensus,
     *        or the single active set otherwise.
     */
    std::set<std::string> getAllMembers() const;

    /** @brief Current (new/target) member set. */
    std::set<std::string> getNewMembers() const;

    /** @brief Previous member set (empty when not in transition). */
    std::set<std::string> getOldMembers() const;

    /**
     * @brief Returns true when @p votes satisfy quorum requirements.
     *
     * During joint consensus both C_old and C_new must separately have
     * majority votes.  Outside joint consensus only C_new is checked.
     */
    bool hasQuorum(const std::set<std::string>& votes) const;

    /**
     * @brief Minimum number of votes required for a simple-majority quorum
     *        in the current (non-transitional) configuration.
     */
    size_t quorumSize() const;

private:
    mutable std::mutex mutex_;
    std::set<std::string> old_members_;  // empty when not in transition
    std::set<std::string> new_members_;
    bool in_joint_consensus_{false};

    static size_t majority(size_t n) { return (n / 2) + 1; }
};

// ─────────────────────────────────────────────────────────────────────────────
// MembershipChangeEntry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A log entry that carries a configuration change.
 *
 * The entry is written to the leader's WAL and replicated before either the
 * joint configuration or the final new configuration is activated.
 */
struct MembershipChangeEntry {
    enum class Phase {
        JOINT,   ///< Activate C_old,new (joint consensus)
        COMMIT,  ///< Activate C_new (transition complete)
    };

    Phase phase;
    std::set<std::string> old_members;  ///< C_old
    std::set<std::string> new_members;  ///< C_new
    uint64_t log_index{0};              ///< WAL index of this entry
    uint64_t term{0};                   ///< Raft term when written

    /** @brief Returns true when this is a JOINT-phase entry. */
    bool isJointPhase() const { return phase == Phase::JOINT; }
};

// ─────────────────────────────────────────────────────────────────────────────
// RaftV2State
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Persistent per-node Raft state (currentTerm, votedFor, commitIndex).
 *
 * All fields that must survive a restart are stored here so they can be
 * flushed to disk atomically.
 */
struct RaftV2State {
    uint64_t current_term{0};     ///< Latest term seen by this node
    std::string voted_for;        ///< Candidate voted for in current_term (empty = none)
    uint64_t commit_index{0};     ///< Highest log entry known to be committed
    uint64_t last_applied{0};     ///< Highest log entry applied to state machine

    /** @brief Returns true when this node has cast a vote in the current term. */
    bool hasVoted() const { return !voted_for.empty(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// MembershipChangeManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates the two-phase joint-consensus membership change protocol.
 *
 * Usage:
 * @code
 * MembershipChangeManager mgr(current_config, node_id, wal_manager);
 *
 * // Propose adding a new node
 * auto entry = mgr.proposeAdd("new-node-4");
 * // … replicate the entry …
 * mgr.onJointCommitted(entry.log_index);
 * // … replicate the commit entry …
 * mgr.onNewConfigCommitted();
 * @endcode
 */
class MembershipChangeManager {
public:
    /**
     * @brief Construct the manager with the current cluster configuration.
     *
     * @param config   Shared cluster configuration (modified in-place).
     * @param node_id  ID of the local node (must be the current leader).
     * @param wal      WAL manager used to persist configuration entries.
     */
    MembershipChangeManager(
        std::shared_ptr<RaftV2ClusterConfig> config,
        const std::string& node_id,
        std::shared_ptr<WALManager> wal);

    // ── Proposal API ────────────────────────────────────────────────────────

    /**
     * @brief Propose adding a new voting member.
     *
     * Writes a JOINT-phase configuration entry to the WAL and begins
     * joint consensus.  The caller must replicate the returned entry and
     * call onJointCommitted() once a quorum of both C_old and C_new has
     * acknowledged it.
     *
     * @throws std::runtime_error if a change is already in progress.
     */
    MembershipChangeEntry proposeAdd(const std::string& node_id);

    /**
     * @brief Propose removing an existing voting member.
     *
     * Same semantics as proposeAdd() but for removal.
     *
     * @throws std::runtime_error if a change is already in progress or the
     *         resulting cluster would be empty.
     */
    MembershipChangeEntry proposeRemove(const std::string& node_id);

    // ── State machine callbacks ──────────────────────────────────────────────

    /**
     * @brief Called by the leader once the JOINT entry at @p log_index is
     *        committed (majority of both C_old and C_new have acked it).
     *
     * Writes the COMMIT-phase entry to the WAL.
     */
    void onJointCommitted(uint64_t log_index);

    /**
     * @brief Called once the COMMIT entry has been replicated to C_new quorum.
     *
     * Finalises the transition: C_new becomes the active configuration and
     * C_old is discarded.  After this call isChangeInProgress() returns false.
     */
    void onNewConfigCommitted();

    /**
     * @brief Apply an incoming configuration entry from a leader (follower path).
     *
     * Followers must apply configuration entries as soon as they are
     * *written* to the local log (not only after commit), per Raft §4.1.
     */
    void applyEntry(const MembershipChangeEntry& entry);

    // ── Status ──────────────────────────────────────────────────────────────

    /** @brief True while a membership change is in flight. */
    bool isChangeInProgress() const;

    /**
     * @brief Returns the pending change entry, if any.
     */
    std::optional<MembershipChangeEntry> pendingEntry() const;

    /**
     * @brief Returns the current cluster configuration (read-only view).
     */
    std::shared_ptr<const RaftV2ClusterConfig> currentConfig() const;

private:
    mutable std::mutex mutex_;
    std::shared_ptr<RaftV2ClusterConfig> config_;
    std::string node_id_;
    std::shared_ptr<WALManager> wal_;
    std::optional<MembershipChangeEntry> pending_;

    MembershipChangeEntry writeEntry(
        MembershipChangeEntry::Phase phase,
        const std::set<std::string>& old_members,
        const std::set<std::string>& new_members);
};

}  // namespace replication
}  // namespace themisdb
