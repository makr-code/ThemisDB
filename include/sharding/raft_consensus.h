/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_consensus.h                                   ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:20:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     296                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 429d2af3ca  2026-02-25  fix(audit): close all gaps in joint consensus implementation ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_RAFT_CONSENSUS_H
#define THEMISDB_SHARDING_RAFT_CONSENSUS_H

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
    std::string leader_id;
    uint64_t term;
    uint64_t commit_index;
    std::chrono::steady_clock::time_point timestamp;
    std::vector<std::string> reachable_nodes;  // Nodes leader can reach
};

/**
 * @brief Health status of a replica
 */
enum class ReplicaHealth {
    HEALTHY,        // Responding normally
    DEGRADED,       // Slow responses
    UNREACHABLE,    // Cannot connect
    PARTITIONED     // In different network partition
};

/**
 * @brief Replica state tracking
 */
struct ReplicaState {
    std::string node_id;
    ReplicaHealth health;
    uint64_t next_index;        // Next log index to send
    uint64_t match_index;       // Highest log index replicated
    std::chrono::steady_clock::time_point last_contact;
    uint32_t consecutive_failures;
};

/**
 * @brief Partition detection result
 */
struct PartitionStatus {
    bool is_partitioned;
    std::vector<std::string> reachable_nodes;
    std::vector<std::string> unreachable_nodes;
    bool has_quorum;
    std::string partition_id;  // Identifier for this partition group
    std::chrono::steady_clock::time_point detected_at;
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
        RaftConfig raft_config;
        std::chrono::milliseconds heartbeat_timeout{500};
        std::chrono::milliseconds partition_detection_interval{1000};
        uint32_t max_consecutive_failures{3};
        bool enable_partition_detection{true};
        bool enable_split_brain_prevention{true};
        bool read_only_on_partition{true};  // Become read-only if minority partition
    };
    
    explicit RaftConsensus(const Config& config);
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
     */
    bool isLeader() const;
    
    /**
     * @brief Check if cluster has quorum
     */
    bool hasQuorum() const;
    
    /**
     * @brief Get current partition status
     */
    PartitionStatus getPartitionStatus() const;
    
    /**
     * @brief Check if node is in read-only mode (minority partition)
     */
    bool isReadOnly() const;
    
    /**
     * @brief Get current leader ID
     */
    std::string getLeaderId() const;
    
    /**
     * @brief Get current term
     */
    uint64_t getCurrentTerm() const;
    
    /**
     * @brief Get replica states
     */
    std::vector<ReplicaState> getReplicaStates() const;
    
    /**
     * @brief Set replication callback
     */
    void setReplicationCallback(ReplicationCallback callback);
    
    /**
     * @brief Set heartbeat callback
     */
    void setHeartbeatCallback(HeartbeatCallback callback);
    
    /**
     * @brief Handle incoming heartbeat from leader
     */
    void receiveHeartbeat(const Heartbeat& heartbeat);
    
    /**
     * @brief Handle append entries response from follower
     */
    void receiveAppendEntriesResponse(const std::string& node_id,
                                     const AppendEntriesResponse& response);
    
    /**
     * @brief Get reference to underlying Raft state
     */
    RaftState& getRaftState() { return raft_state_; }
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

private:
    Config config_;
    RaftState raft_state_;
    
    // Callbacks
    ReplicationCallback replication_callback_;
    HeartbeatCallback heartbeat_callback_;
    
    // Replica tracking
    mutable std::mutex replica_mutex_;
    std::map<std::string, ReplicaState> replica_states_;
    
    // Partition detection
    mutable std::mutex partition_mutex_;
    PartitionStatus partition_status_;
    std::atomic<bool> read_only_mode_{false};
    
    // Background threads
    std::atomic<bool> running_{false};
    std::thread heartbeat_thread_;
    std::thread election_thread_;
    std::thread partition_detector_thread_;
    
    // Thread synchronization
    std::condition_variable cv_;
    std::mutex cv_mutex_;
    
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
     */
    bool replicateToFollower(const std::string& node_id, const LogEntry& entry);
    
    /**
     * @brief Detect network partition
     */
    PartitionStatus detectPartition();
    
    /**
     * @brief Update replica health status
     */
    void updateReplicaHealth(const std::string& node_id, bool success);
    
    /**
     * @brief Check if have quorum of healthy replicas
     */
    bool checkQuorum() const;
    
    /**
     * @brief Initialize replica states
     */
    void initializeReplicaStates();
};

}  // namespace sharding
}  // namespace themisdb

#endif  // THEMISDB_SHARDING_RAFT_CONSENSUS_H
