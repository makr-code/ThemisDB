/**
 * @file raft_consensus.h
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

#include "sharding/raft_state.h"
#include "sharding/raft_log.h"
#include <functional>
#include <future>
#include <atomic>
#include <condition_variable>

namespace themisdb {
namespace sharding {

/**
 * @brief Heartbeat message for partition detection
 */
struct Heartbeat {
    std::string leader_id;                             ///< Current leader node ID.
    uint64_t term;                                    ///< Leader term associated with heartbeat.
    uint64_t commit_index;                            ///< Leader commit index at send time.
    std::chrono::steady_clock::time_point timestamp;  ///< Local send timestamp.
    std::vector<std::string> reachable_nodes;         ///< Peers currently reachable by the leader.
};

/**
 * @brief Health status of a replica
 */
enum class ReplicaHealth {
    HEALTHY,        ///< Responding normally.
    DEGRADED,       ///< Responding but showing failures or latency.
    UNREACHABLE,    ///< Currently unreachable.
    PARTITIONED     ///< Known to be in a different network partition.
};

/**
 * @brief Replica state tracking
 */
struct ReplicaState {
    std::string node_id;          ///< Replica node identifier.
    std::string endpoint;           ///< Network endpoint ("host:port" or URL); updated by updatePeerAddress()
    ReplicaHealth health;        ///< Current health classification.
    uint64_t next_index;         ///< Next log index to send to replica.
    uint64_t match_index;        ///< Highest log index known replicated on replica.
    std::chrono::steady_clock::time_point last_contact; ///< Last successful contact timestamp.
    uint32_t consecutive_failures; ///< Consecutive failed contact attempts.
};

/**
 * @brief Partition detection result
 */
struct PartitionStatus {
    bool is_partitioned;                                ///< True when partition symptoms are detected.
    std::vector<std::string> reachable_nodes;           ///< Peers reachable from this node.
    std::vector<std::string> unreachable_nodes;         ///< Peers currently unreachable.
    bool has_quorum;                                    ///< True when reachable set still forms quorum.
    std::string partition_id;                           ///< Deterministic identifier for current reachable set.
    std::chrono::steady_clock::time_point detected_at;  ///< Time partition status was computed.
};

/**
 * @brief Enhanced Raft Consensus with Partition Detection
 * 
 * Extends basic Raft implementation with:
 * - Network partition detection
 * - Split-brain prevention
 * - Quorum-enforced operations
 * - Automatic partition healing
 */
class RaftConsensus {
public:
    using ReplicationCallback = std::function<bool(const std::string& node_id, 
                                                   const LogEntry& entry)>;
    using HeartbeatCallback = std::function<bool(const std::string& node_id,
                                                 const Heartbeat& heartbeat)>;
    
    /**
     * @brief Consensus configuration
     */
    struct Config {
        RaftConfig raft_config;                                        ///< Core Raft timing and membership config.
        std::chrono::milliseconds heartbeat_timeout{500};              ///< Timeout used to judge heartbeat staleness.
        std::chrono::milliseconds partition_detection_interval{1000};  ///< Poll interval for partition detection.
        uint32_t max_consecutive_failures{3};                          ///< Failure threshold before replica becomes unreachable.
        bool enable_partition_detection{true};                         ///< Enables background partition detector thread.
        bool enable_split_brain_prevention{true};                      ///< Enables partition-aware safety measures.
        bool read_only_on_partition{true};                             ///< Enter read-only mode in minority partition.
    };

    /**
     * @brief Construct enhanced Raft consensus engine.
     * @param config Consensus configuration and timing thresholds.
     */
    explicit RaftConsensus(const Config& config);

    /** @brief Stop background threads and release owned resources. */
    ~RaftConsensus();
    
    // Prevent copying
    RaftConsensus(const RaftConsensus&) = delete;
    RaftConsensus& operator=(const RaftConsensus&) = delete;
    
    /**
     * @brief Start consensus protocol
     */
    void start();
    
    /**
     * @brief Stop consensus protocol
     */
    void stop();
    
    /**
     * @brief Propose a new log entry (leader only)
     * @param command Command to replicate
     * @return Future that resolves when entry is committed or fails
     */
    std::future<bool> propose(const std::string& command);
    
    /**
     * @brief Check if this node is the leader
        * @return True when underlying Raft state is leader.
     */
    bool isLeader() const;
    
    /**
     * @brief Check if cluster has quorum
        * @return True when enough healthy replicas are reachable.
     */
    bool hasQuorum() const;
    
    /**
     * @brief Get current partition status
        * @return Snapshot of latest partition detection result.
     */
    PartitionStatus getPartitionStatus() const;
    
    /**
     * @brief Check if node is in read-only mode (minority partition)
        * @return True when write operations should be refused.
     */
    bool isReadOnly() const;
    
    /**
     * @brief Get current leader ID
        * @return Current known leader identifier, possibly empty.
     */
    std::string getLeaderId() const;
    
    /**
     * @brief Get current term
        * @return Current Raft term.
     */
    uint64_t getCurrentTerm() const;
    
    /**
     * @brief Get replica states
        * @return Copy of tracked follower replication/health states.
     */
    std::vector<ReplicaState> getReplicaStates() const;
    
    /**
     * @brief Set replication callback
        * @param callback Callback used to send replicated log entries to peers.
     */
    void setReplicationCallback(ReplicationCallback callback);
    
    /**
     * @brief Set heartbeat callback
        * @param callback Callback used to send leader heartbeats to peers.
     */
    void setHeartbeatCallback(HeartbeatCallback callback);
    
    /**
     * @brief Handle incoming heartbeat from leader
        * @param heartbeat Incoming heartbeat payload.
     */
    void receiveHeartbeat(const Heartbeat& heartbeat);
    
    /**
     * @brief Handle append entries response from follower
        * @param node_id Follower identifier.
        * @param response AppendEntries RPC response from follower.
     */
    void receiveAppendEntriesResponse(const std::string& node_id,
                                     const AppendEntriesResponse& response);
    
    /**
     * @brief Get reference to underlying Raft state
        * @return Mutable reference to underlying Raft state machine.
     */
    RaftState& getRaftState() { return raft_state_; }
        /** @brief Return const reference to underlying Raft state machine. */
    const RaftState& getRaftState() const { return raft_state_; }

    /**
     * @brief Register a new peer node for replication and health tracking
     *
     * Called after a membership change is committed so that the new node
     * appears in replica_states_ and will receive heartbeats/AppendEntries.
     *
     * @param node_id The new peer's node identifier
     */
    void addReplicaNode(const std::string& node_id);

    /**
     * @brief Remove a peer node from replication and health tracking
     *
     * Called after a membership change is committed so the removed node is
     * no longer contacted for heartbeats/AppendEntries or counted in quorum.
     *
     * @param node_id The removed peer's node identifier
     */
    void removeReplicaNode(const std::string& node_id);

    /**
     * @brief Update the network endpoint (address) of a known peer.
     *
     * Called during hardware migration (Phase 5) to reflect a shard's new
     * physical host:port after the operator runs
     * `POST /api/v1/shards/{id}/migrate-hardware`.
     *
     * Thread-safe.  Logs a warning (via spdlog) if node_id is not present in
     * replica_states_; the call is a no-op in that case.
     *
     * @param node_id      The peer whose endpoint is changing (must match an
     *                     existing replica state entry).
     * @param new_endpoint New "host:port" or URL string.
     */
    void updatePeerAddress(const std::string& node_id,
                           const std::string& new_endpoint);

private:
    Config config_;
    RaftState raft_state_;
    
    // Callbacks
    ReplicationCallback replication_callback_;   ///< Log replication transport callback.
    HeartbeatCallback heartbeat_callback_;       ///< Heartbeat transport callback.
    
    // Replica tracking
    mutable std::mutex replica_mutex_;
    std::map<std::string, ReplicaState> replica_states_; ///< Per-replica replication and health state.
    
    // Partition detection
    mutable std::mutex partition_mutex_;
    PartitionStatus partition_status_;               ///< Latest detected partition status.
    std::atomic<bool> read_only_mode_{false};        ///< True when writes must be rejected.
    
    // Background threads
    std::atomic<bool> running_{false};               ///< Lifecycle flag shared by background threads.
    std::thread heartbeat_thread_;                   ///< Leader heartbeat worker thread.
    std::thread election_thread_;                    ///< Election timeout worker thread.
    std::thread partition_detector_thread_;          ///< Partition detector worker thread.
    
    // Thread synchronization
    std::condition_variable cv_;
    std::mutex cv_mutex_;                            ///< Wait mutex for worker thread sleep/wake.
    
    /**
     * @brief Heartbeat loop (for leader)
     */
    void heartbeatLoop();
    
    /**
     * @brief Election timeout loop (for followers)
     */
    void electionLoop();
    
    /**
     * @brief Partition detection loop
     */
    void partitionDetectionLoop();
    
    /**
     * @brief Send heartbeats to all followers
     */
    void sendHeartbeats();
    
    /**
     * @brief Replicate log entry to follower
        * @param node_id Follower identifier.
        * @param entry Log entry to replicate.
        * @param callback Transport callback to invoke.
        * @return True when replication callback reports success.
     */
    bool replicateToFollower(const std::string& node_id,
                             const LogEntry& entry,
                             const ReplicationCallback& callback);
    
    /**
     * @brief Detect network partition
        * @return Newly computed partition status snapshot.
     */
    PartitionStatus detectPartition();
    
    /**
     * @brief Update replica health status
        * @param node_id Replica identifier.
        * @param success True when the most recent contact succeeded.
     */
    void updateReplicaHealth(const std::string& node_id, bool success);

    /**
     * @brief Update a single replica state's health with replica_mutex_ already held
        * @param state Mutable replica state to update.
        * @param success True when the most recent contact succeeded.
        * @param now Timestamp used for staleness calculations.
     */
    void updateReplicaHealthLocked(ReplicaState& state,
                                   bool success,
                                   std::chrono::steady_clock::time_point now);
    
    /**
     * @brief Check if have quorum of healthy replicas
        * @return True when reachable healthy/degraded nodes satisfy quorum.
     */
    bool checkQuorum() const;
    
    /**
     * @brief Initialize replica states
     */
    void initializeReplicaStates();
};

}  // namespace sharding
}  // namespace themisdb
