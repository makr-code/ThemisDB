/**
 * @file raft_two_phase_commit_decision_store.h
 * @brief Quorum-backed 2PC decision callbacks over a durable Raft group.
 */
#pragma once

#include "sharding/two_phase_commit_coordinator.h"

#include <memory>
#include <string>

namespace themisdb::sharding {
class RaftConsensus;
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
 * The current Raft snapshot format does not preserve this decision state
 * machine. Reads and writes therefore fail closed once the Raft log has been
 * compacted, until a snapshot containing the decision state is installed.
 */
class RaftTwoPhaseCommitDecisionStore final {
public:
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

private:
    struct State;
    std::shared_ptr<State> state_;
};

} // namespace themis::sharding
