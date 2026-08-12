/**
 * @file paxos_consensus.h
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

#include "sharding/consensus_module.h"
#include "sharding/wal_manager.h"
#include <map>
#include <set>
#include <atomic>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>

namespace themis::sharding {
class PaxosWAL;
class PaxosSnapshotManager;
}

namespace themisdb {
namespace sharding {

// Forward declarations (Phase 2.1)
using LSN = themis::sharding::LSN;
using PaxosWAL = themis::sharding::PaxosWAL;
using PaxosSnapshotManager = themis::sharding::PaxosSnapshotManager;

/**
 * @brief Paxos proposal number (ballot number)
 */
struct ProposalNumber {
    uint64_t round;        // Proposal round number
    std::string node_id;   // Proposer node ID
    
    bool operator<(const ProposalNumber& other) const {
        if (round != other.round) return round < other.round;
        return node_id < other.node_id;
    }
    
    bool operator==(const ProposalNumber& other) const {
        return round == other.round && node_id == other.node_id;
    }
    
    bool operator>(const ProposalNumber& other) const {
        return other < *this;
    }
};

/**
 * @brief Paxos instance for a single log slot
 */
struct PaxosInstance {
    uint64_t slot;                           // Log slot number
    ProposalNumber promised_proposal;        // Highest proposal promised
    ProposalNumber accepted_proposal;        // Accepted proposal number
    ConsensusLogEntry accepted_value;        // Accepted value
    bool is_committed;                       // Whether value is committed
    
    // Prepare phase
    std::set<std::string> prepare_promises;  // Nodes that promised
    
    // Accept phase
    std::set<std::string> accept_acks;       // Nodes that accepted
    
    PaxosInstance() : slot(0), is_committed(false) {
        promised_proposal.round = 0;
        accepted_proposal.round = 0;
    }
};

/**
 * @brief Paxos Consensus Implementation
 * 
 * Implements Multi-Paxos consensus algorithm for distributed agreement.
 * Features:
 * - Multi-Paxos optimization (stable leader)
 * - Fast path optimization (skip prepare when leader is stable)
 * - Quorum-based agreement
 * - Persistent state management
 * 
 * Sources:
 * - Lamport, L. (1998). "The Part-Time Parliament"
 * - Lamport, L. (2001). "Paxos Made Simple"
 * - van Renesse, R. & Altinbuken, D. (2015). "Paxos Made Moderately Complex"
 */
class PaxosConsensus : public ConsensusModule {
public:
    explicit PaxosConsensus(const ConsensusConfig& config);
    ~PaxosConsensus() override;
    
    // ConsensusModule interface
    ConsensusType getType() const override { return ConsensusType::PAXOS; }
    
    bool initialize(
        const std::string& node_id,
        const std::vector<std::string>& cluster_nodes
    ) override;
    
    bool start() override;
    void stop() override;
    
    bool isLeader() const override;
    std::string getLeaderId() const override;
    ConsensusState getState() const override;
    
    std::optional<uint64_t> propose(
        const std::string& operation,
        const nlohmann::json& data
    ) override;
    
    bool waitForCommit(
        uint64_t log_index,
        std::chrono::milliseconds timeout
    ) override;
    
    std::vector<ConsensusLogEntry> readLog(
        uint64_t start_index,
        std::optional<uint64_t> end_index = std::nullopt
    ) override;
    
    uint64_t getCommitIndex() const override;
    uint64_t getLastLogIndex() const override;
    
    bool addNode(
        const std::string& node_id,
        const std::string& endpoint
    ) override;
    
    bool removeNode(const std::string& node_id) override;
    
    bool transferLeadership(const std::string& target_node_id) override;
    
    bool takeSnapshot(const nlohmann::json& snapshot_data) override;
    bool restoreSnapshot(const nlohmann::json& snapshot_data) override;
    
    ConsensusStats getStats() const override;
    nlohmann::json getStatus() const override;
    
    void onCommit(
        std::function<void(const ConsensusLogEntry&)> callback
    ) override;
    
    void onStateChange(
        std::function<void(ConsensusState, ConsensusState)> callback
    ) override;
    
    void onLeaderChange(
        std::function<void(const std::string&, const std::string&)> callback
    ) override;

    // -----------------------------------------------------------------
    // RPC peer callbacks for multi-node operation
    //
    // These callbacks are invoked by the Paxos engine when it needs to
    // send Prepare or Accept RPCs to remote nodes.  In single-node mode
    // (or in tests) they may be left unset; the engine will then only
    // count the local self-promise/self-accept and will therefore be
    // unable to reach quorum in a cluster with N > 1.
    //
    // Signature for prepare callback:
    //   bool prepare_fn(const std::string& peer_node_id,
    //                   uint64_t slot,
    //                   uint64_t round,
    //                   const std::string& proposer_id)
    //   Returns true if the peer promised (granted the prepare).
    //
    // Signature for accept callback:
    //   bool accept_fn(const std::string& peer_node_id,
    //                  uint64_t slot,
    //                  uint64_t round,
    //                  const ConsensusLogEntry& value)
    //   Returns true if the peer acknowledged the accept.
    // -----------------------------------------------------------------
    using PaxosPrepareCallback =
        std::function<bool(const std::string& peer,
                           uint64_t slot,
                           uint64_t round,
                           const std::string& proposer_id)>;

    /// Extended prepare callback that, on success, returns the acceptor's
    /// highest-ballot accepted value (Paxos Phase-1b safe-value propagation).
    /// When the acceptor has never accepted a value, the optional is empty.
    /// If this callback is registered it takes priority over PaxosPrepareCallback.
    struct PreparePromiseResult {
        bool promised = false;                           ///< true = acceptor promised
        uint64_t accepted_round = 0;                     ///< ballot of the last accepted proposal
        std::optional<ConsensusLogEntry> accepted_value; ///< value accepted at that ballot
    };
    using PaxosPrepareFullCallback =
        std::function<PreparePromiseResult(const std::string& peer,
                                           uint64_t slot,
                                           uint64_t round,
                                           const std::string& proposer_id)>;

    using PaxosAcceptCallback =
        std::function<bool(const std::string& peer,
                           uint64_t slot,
                           uint64_t round,
                           const ConsensusLogEntry& value)>;

    /**
     * @brief Inject the RPC callback used to send Phase-1 Prepare messages.
     *
     * Must be called before start() in multi-node deployments.
     * Not required for single-node operation.
     */
    void setPrepareRPCCallback(PaxosPrepareCallback cb);

    /**
     * @brief Inject the extended Phase-1 Prepare RPC callback.
     *
     * When registered this callback supersedes the basic PaxosPrepareCallback and
     * enables correct highest-accepted-value propagation (Paxos Phase-1b safety).
     * Prefer this over setPrepareRPCCallback in multi-node deployments.
     */
    void setPrepareFullRPCCallback(PaxosPrepareFullCallback cb);

    /**
     * @brief Inject the RPC callback used to send Phase-2 Accept messages.
     *
     * Must be called before start() in multi-node deployments.
     * Not required for single-node operation.
     */
    void setAcceptRPCCallback(PaxosAcceptCallback cb);

    /**
     * @brief Handle prepare request from proposer
     */
    bool handlePrepare(uint64_t slot, const ProposalNumber& proposal);
    
    /**
     * @brief Handle accept request from proposer
     */
    bool handleAccept(
        uint64_t slot,
        const ProposalNumber& proposal,
        const ConsensusLogEntry& value
    );
    
    /**
     * @brief Handle commit notification
     */
    void handleCommit(uint64_t slot, const ConsensusLogEntry& value);
    
private:
    /**
     * @brief Run Paxos proposer role
     */
    void runProposer();
    
    /**
     * @brief Run Paxos acceptor role
     */
    void runAcceptor();
    
    /**
     * @brief Run Paxos learner role
     */
    void runLearner();
    
    /**
     * @brief Execute prepare phase for a slot
     */
    bool executePreparePhase(uint64_t slot, const ConsensusLogEntry& value);
    
    /**
     * @brief Execute accept phase for a slot
     */
    bool executeAcceptPhase(
        uint64_t slot,
        const ProposalNumber& proposal,
        const ConsensusLogEntry& value
    );
    
    /**
     * @brief Broadcast commit to all nodes
     * @return false if WAL COMMIT log fails (phase must be aborted to preserve durability)
     */
    bool broadcastCommit(uint64_t slot, const ConsensusLogEntry& value);
    
    /**
     * @brief Calculate quorum size
     */
    size_t getQuorumSize() const;
    
    /**
     * @brief Check if we have quorum
     */
    bool hasQuorum(size_t count) const;
    
    /**
     * @brief Generate next proposal number
     */
    ProposalNumber generateProposalNumber();
    
    /**
     * @brief Load persistent state from disk
     */
    bool loadPersistentState();
    
    /**
     * @brief Save persistent state to disk
     */
    bool savePersistentState();
    
    /**
     * @brief Background thread for leader election and heartbeat
     */
    void leaderElectionThread();
    
    /**
     * @brief Create periodic snapshot (Phase 2.1)
     */
    void createPeriodicSnapshot();
    
    /**
     * @brief Recover from WAL and snapshot (Phase 2.1)
     */
    bool recoverFromWAL();
    
    ConsensusConfig config_;
    std::string node_id_;
    std::vector<std::string> cluster_nodes_;
    
    // State
    mutable std::mutex state_mutex_;
    std::atomic<ConsensusState> state_;
    std::atomic<bool> running_;
    std::string current_leader_;
    // PAX-5: current_round_ is incremented by the proposer thread and read by
    // the learner, election, and snapshot threads — make it atomic.
    std::atomic<uint64_t> current_round_;
    
    // Paxos instances (one per log slot)
    std::map<uint64_t, PaxosInstance> instances_;
    std::atomic<uint64_t> next_slot_;
    std::atomic<uint64_t> commit_index_;
    
    // Proposal queue
    std::timed_mutex proposal_mutex_;
    std::condition_variable_any proposal_cv_;
    std::map<uint64_t, ConsensusLogEntry> pending_proposals_;
    
    // Committed log
    std::map<uint64_t, ConsensusLogEntry> committed_log_;
    
    // Callbacks
    mutable std::mutex callbacks_mutex_;
    std::function<void(const ConsensusLogEntry&)> on_commit_callback_;
    std::function<void(ConsensusState, ConsensusState)> on_state_change_callback_;
    std::function<void(const std::string&, const std::string&)> on_leader_change_callback_;

    // RPC peer callbacks (optional; nil → single-node / test mode)
    PaxosPrepareCallback     rpc_prepare_cb_;
    PaxosPrepareFullCallback rpc_prepare_full_cb_; ///< Extended callback with safe-value return
    PaxosAcceptCallback      rpc_accept_cb_;
    
    // Background threads
    std::thread proposer_thread_;
    std::thread acceptor_thread_;
    std::thread learner_thread_;
    std::thread election_thread_;
    
    // Statistics
    std::atomic<uint64_t> total_proposals_;
    std::atomic<uint64_t> failed_proposals_;
    std::atomic<uint64_t> total_prepares_;
    std::atomic<uint64_t> total_accepts_;

    // Snapshot storage (protected by state_mutex_)
    nlohmann::json snapshot_data_;
    uint64_t snapshot_index_{0};
    uint64_t snapshot_term_{0};
    
    // Phase 2.1: Persistent State (WAL + Snapshots)
    std::unique_ptr<PaxosWAL> wal_;
    std::unique_ptr<PaxosSnapshotManager> snapshot_manager_;
    std::atomic<uint64_t> operations_since_snapshot_{0};
    LSN last_applied_lsn_;
};

} // namespace sharding
} // namespace themisdb
