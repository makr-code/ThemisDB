/**
 * @file raft_two_phase_commit_decision_store.h
 * @brief Quorum-backed 2PC decision callbacks over a durable Raft group.
 */
#pragma once

#include "sharding/two_phase_commit_coordinator.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace themisdb::sharding {
class RaftConsensus;
class RaftSnapshotManager;
}

namespace themis::sharding {

/**
 * @brief Provides 2PC decision writer and lookup callbacks backed by Raft.
 *
 * The supplied consensus instance must be the current leader of the decision
 * group and have a healthy durable journal and quorum. Calls fail closed when
 * those conditions are absent. One store instance may be shared by coordinators
 * using the same Raft instance; writes through the instance are serialized.
 *
 * The store exposes capture/restore methods for the named Raft snapshot
 * extension. Callers must capture that extension before compaction and restore
 * it from the installed snapshot before enabling callbacks; otherwise reads
 * and writes fail closed after the log is compacted.
 */
class RaftTwoPhaseCommitDecisionStore final {
public:
    static constexpr std::string_view kSnapshotExtensionName =
        "themisdb.2pc.decision-state.v1";

    /**
     * @brief Create a decision store for one Raft group.
     * @param raft Shared Raft instance for the decision group.
     * @param raft_group_id Non-empty logical group identity, at most 1024 bytes,
     *        included in decision records.
     * @throws std::invalid_argument if the instance is null or the group identity
     *         is empty or exceeds the supported size.
     * @note The store retains shared ownership of the Raft instance.
     */
    RaftTwoPhaseCommitDecisionStore(
        std::shared_ptr<themisdb::sharding::RaftConsensus> raft,
        std::string raft_group_id);

    /**
     * @brief Create a decision store and restore its installed Raft snapshot.
     * @param raft Shared Raft instance for the decision group.
     * @param raft_group_id Non-empty logical group identity, at most 1024 bytes.
     * @param snapshot_manager Manager holding the snapshot at the Raft journal's
     *        installed snapshot boundary.
     * @throws std::invalid_argument if the instance or group identity is invalid.
     * @throws std::runtime_error if a compacted boundary has no matching readable
     *         snapshot or the snapshot lacks a valid decision-state extension.
     * @note With no installed snapshot boundary, this overload behaves like the
     *       regular constructor. Construct the coordinator only after this returns.
     */
    RaftTwoPhaseCommitDecisionStore(
        std::shared_ptr<themisdb::sharding::RaftConsensus> raft,
        std::string raft_group_id,
        const themisdb::sharding::RaftSnapshotManager& snapshot_manager);

    /**
     * @brief Return a writer that commits immutable decisions through Raft.
     * @return Callback returning true only after a quorum-committed matching decision.
     * @throws std::invalid_argument from the callback for empty or oversized IDs,
     *         or an invalid COMMIT timestamp.
     * @throws std::runtime_error from the callback when Raft history is unavailable,
     *         corrupt, or outside the supported snapshot boundary.
     * @note The callback retains the store state and serializes operations sharing
     *       the same Raft instance within this process.
     */
    [[nodiscard]] TwoPhaseCommitDecisionWriter writer() const;

    /**
     * @brief Return a lookup that reads only committed decisions.
     * @return Callback returning nullopt only when the current leader's committed
     *         history contains no decision for the coordinator/transaction pair.
     * @throws std::invalid_argument from the callback for empty or oversized IDs.
     * @throws std::runtime_error from the callback when leadership, quorum,
     *         durable history, or log integrity cannot be established.
     * @note The callback retains the store state and serializes operations sharing
     *       the same Raft instance within this process.
     */
    [[nodiscard]] TwoPhaseCommitDecisionLookup lookup() const;

    /**
     * @brief Capture committed decisions through a Raft snapshot boundary.
     * @param snapshot_index Last committed entry represented by the snapshot.
     * @param snapshot_term Term of the entry at @p snapshot_index.
     * @return Versioned decision-state bytes for a named Raft snapshot extension.
     * @throws std::runtime_error if authority, history, boundary, or size validation
     *         fails, including when a prior compacted decision snapshot was not restored
     *         or the encoded state exceeds 256 MiB.
     * @note Callers must store these bytes under @ref kSnapshotExtensionName in
     *       RaftSnapshotExtensions before compacting the log.
     */
    [[nodiscard]] std::vector<uint8_t> captureSnapshotState(
        uint64_t snapshot_index,
        uint64_t snapshot_term) const;

    /**
     * @brief Persist application and 2PC decision state before compacting Raft.
     * @param snapshot_manager Snapshot manager that owns the durable snapshot directory.
     * @param snapshot_index Last committed entry represented by the snapshot.
     * @param snapshot_term Term of the entry at @p snapshot_index.
     * @param application_state Complete application state at the same Raft boundary.
     * @return True only when the snapshot is installed, Raft is compacted, and the
     *         in-memory decision snapshot is advanced; false when snapshot persistence
     *         or compaction fails.
     * @throws std::runtime_error if leader/quorum/journal authority, the boundary,
     *         decision history, or post-install state validation cannot be established.
     * @note This operation writes the decision bytes under @ref kSnapshotExtensionName.
     *       It serializes through the store's shared operation lock, but does not
     *       transfer or install snapshots on followers.
     */
    [[nodiscard]] bool createAndInstallSnapshot(
        themisdb::sharding::RaftSnapshotManager& snapshot_manager,
        uint64_t snapshot_index,
        uint64_t snapshot_term,
        const std::vector<uint8_t>& application_state) const;

    /**
     * @brief Restore decision state from a verified persisted Raft snapshot extension.
     * @param snapshot_index Snapshot boundary index.
     * @param snapshot_term Snapshot boundary term.
     * @param snapshot_data Bytes stored under @ref kSnapshotExtensionName.
     * @throws std::runtime_error if the payload is corrupt, belongs to another group,
     *         conflicts with retained Raft history, or does not match the installed
     *         Raft log anchor.
     * @note Restore is safe on followers; lookup and write still require current
     *       leader, quorum, and a healthy durable journal.
     */
    void restoreSnapshotState(
        uint64_t snapshot_index,
        uint64_t snapshot_term,
        const std::vector<uint8_t>& snapshot_data);

private:
    struct State;
    std::shared_ptr<State> state_;
};

} // namespace themis::sharding
