/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            consensus_module.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:13:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     366                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

/**
 * @brief Consensus state of a node
 */
enum class ConsensusState {
    FOLLOWER,     // Node is following a leader
    CANDIDATE,    // Node is running for leadership
    LEADER,       // Node is the current leader
    OBSERVER      // Node observes but doesn't participate in voting
};

/**
 * @brief Consensus module type
 */
enum class ConsensusType {
    RAFT,         // Raft consensus (leader-based, strongly consistent)
    GOSSIP,       // Gossip protocol (leaderless, eventually consistent)
    PAXOS,        // Paxos consensus (quorum-based, strongly consistent)
    MULTI_PAXOS   // Multi-Paxos (optimized Paxos for multiple rounds)
};

/**
 * @brief Log entry for consensus replication
 */
struct ConsensusLogEntry {
    uint64_t index;              // Log entry index
    uint64_t term;               // Term/epoch number
    std::string operation;       // Operation type (PUT, DELETE, etc.)
    nlohmann::json data;         // Operation data
    std::chrono::system_clock::time_point timestamp;
    
    nlohmann::json toJson() const {
        return {
            {"index", index},
            {"term", term},
            {"operation", operation},
            {"data", data},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()).count()}
        };
    }
};

/**
 * @brief Vote request for leader election
 */
struct VoteRequest {
    std::string candidate_id;    // Candidate requesting vote
    uint64_t term;               // Candidate's term
    uint64_t last_log_index;     // Index of candidate's last log entry
    uint64_t last_log_term;      // Term of candidate's last log entry
};

/**
 * @brief Vote response
 */
struct VoteResponse {
    std::string voter_id;        // ID of the voter
    uint64_t term;               // Current term of voter
    bool vote_granted;           // Whether vote was granted
    std::string reason;          // Reason for denial (if applicable)
};

/**
 * @brief Replication request
 */
struct ReplicationRequest {
    std::string leader_id;       // Leader sending entries
    uint64_t term;               // Leader's term
    uint64_t prev_log_index;     // Index of log entry before new entries
    uint64_t prev_log_term;      // Term of prev_log_index entry
    std::vector<ConsensusLogEntry> entries;  // Log entries to replicate
    uint64_t leader_commit;      // Leader's commit index
};

/**
 * @brief Replication response
 */
struct ReplicationResponse {
    std::string follower_id;     // ID of the follower
    uint64_t term;               // Current term of follower
    bool success;                // Whether replication succeeded
    uint64_t match_index;        // Highest index replicated (for leader)
    std::string error_message;   // Error message (if failed)
};

/**
 * @brief Consensus statistics
 */
struct ConsensusStats {
    uint64_t current_term;
    uint64_t commit_index;
    uint64_t last_applied;
    ConsensusState state;
    std::string current_leader;
    size_t cluster_size;
    size_t reachable_nodes;
    std::chrono::milliseconds average_replication_latency;
    uint64_t total_operations;
    uint64_t failed_operations;
};

/**
 * @brief Abstract interface for consensus modules
 * 
 * Provides a pluggable interface for different consensus algorithms
 * (Raft, Gossip, Paxos) to be used interchangeably in ThemisDB's
 * distributed sharding architecture.
 */
class ConsensusModule {
public:
    virtual ~ConsensusModule() = default;
    
    /**
     * @brief Get the consensus module type
     */
    virtual ConsensusType getType() const = 0;
    
    /**
     * @brief Initialize the consensus module
     * @param node_id This node's unique identifier
     * @param cluster_nodes List of all nodes in the cluster
     * @return true if initialization succeeded
     */
    virtual bool initialize(
        const std::string& node_id,
        const std::vector<std::string>& cluster_nodes
    ) = 0;
    
    /**
     * @brief Start the consensus protocol
     * @return true if started successfully
     */
    virtual bool start() = 0;
    
    /**
     * @brief Stop the consensus protocol gracefully
     */
    virtual void stop() = 0;
    
    /**
     * @brief Check if this node is the leader
     */
    virtual bool isLeader() const = 0;
    
    /**
     * @brief Get the current leader's ID
     * @return Leader ID or empty if no leader
     */
    virtual std::string getLeaderId() const = 0;
    
    /**
     * @brief Get current consensus state
     */
    virtual ConsensusState getState() const = 0;
    
    /**
     * @brief Propose a new operation to be replicated
     * @param operation Operation type
     * @param data Operation data
     * @return Index of the log entry, or nullopt if failed
     */
    virtual std::optional<uint64_t> propose(
        const std::string& operation,
        const nlohmann::json& data
    ) = 0;
    
    /**
     * @brief Wait for an operation to be committed
     * @param log_index Index of the log entry
     * @param timeout Maximum time to wait
     * @return true if committed, false if timeout
     */
    virtual bool waitForCommit(
        uint64_t log_index,
        std::chrono::milliseconds timeout
    ) = 0;
    
    /**
     * @brief Read committed entries from the log
     * @param start_index Starting index (inclusive)
     * @param end_index Ending index (inclusive), or nullopt for all
     * @return Vector of committed log entries
     */
    virtual std::vector<ConsensusLogEntry> readLog(
        uint64_t start_index,
        std::optional<uint64_t> end_index = std::nullopt
    ) = 0;
    
    /**
     * @brief Get the current commit index
     */
    virtual uint64_t getCommitIndex() const = 0;
    
    /**
     * @brief Get the last log index (highest index in the log)
     * @return The index of the last entry in the log
     */
    virtual uint64_t getLastLogIndex() const = 0;
    
    /**
     * @brief Add a node to the cluster
     * @param node_id ID of the new node
     * @param endpoint Network endpoint of the new node
     * @return true if added successfully
     */
    virtual bool addNode(
        const std::string& node_id,
        const std::string& endpoint
    ) = 0;
    
    /**
     * @brief Remove a node from the cluster
     * @param node_id ID of the node to remove
     * @return true if removed successfully
     */
    virtual bool removeNode(const std::string& node_id) = 0;
    
    /**
     * @brief Transfer leadership to another node
     * @param target_node_id ID of the target node
     * @return true if transfer initiated successfully
     */
    virtual bool transferLeadership(const std::string& target_node_id) = 0;
    
    /**
     * @brief Take a snapshot of the current state
     * @param snapshot_data Snapshot data to persist
     * @return true if snapshot succeeded
     */
    virtual bool takeSnapshot(const nlohmann::json& snapshot_data) = 0;
    
    /**
     * @brief Restore from a snapshot
     * @param snapshot_data Snapshot data to restore
     * @return true if restore succeeded
     */
    virtual bool restoreSnapshot(const nlohmann::json& snapshot_data) = 0;
    
    /**
     * @brief Get consensus statistics
     */
    virtual ConsensusStats getStats() const = 0;
    
    /**
     * @brief Get detailed status as JSON
     */
    virtual nlohmann::json getStatus() const = 0;
    
    /**
     * @brief Register callback for committed entries
     * @param callback Function to call when entries are committed
     */
    virtual void onCommit(
        std::function<void(const ConsensusLogEntry&)> callback
    ) = 0;
    
    /**
     * @brief Register callback for state changes
     * @param callback Function to call when consensus state changes
     */
    virtual void onStateChange(
        std::function<void(ConsensusState, ConsensusState)> callback
    ) = 0;
    
    /**
     * @brief Register callback for leader changes
     * @param callback Function to call when leader changes
     */
    virtual void onLeaderChange(
        std::function<void(const std::string&, const std::string&)> callback
    ) = 0;
};

/**
 * @brief Configuration for consensus modules
 */
struct ConsensusConfig {
    ConsensusType type = ConsensusType::RAFT;
    
    // Common settings
    std::string node_id;
    std::vector<std::string> cluster_nodes;
    std::chrono::milliseconds heartbeat_interval{500};
    std::chrono::milliseconds election_timeout_min{1000};
    std::chrono::milliseconds election_timeout_max{2000};
    
    // Raft-specific settings
    size_t raft_log_max_entries = 10000;
    bool raft_enable_pipelining = true;
    
    // Gossip-specific settings
    size_t gossip_fanout = 3;
    std::chrono::milliseconds gossip_interval{1000};
    
    // Paxos-specific settings
    size_t paxos_quorum_size = 0;  // 0 = auto-calculate (n/2 + 1)
    bool paxos_enable_fast_path = true;
    std::chrono::milliseconds paxos_prepare_timeout{1000};
    std::chrono::milliseconds paxos_accept_timeout{500};
    
    // Persistence settings
    std::string data_dir = "./consensus_data";
    bool enable_persistence = true;
    size_t snapshot_interval = 1000;  // Snapshot every N entries
    
    // Network settings
    size_t max_batch_size = 100;
    std::chrono::milliseconds rpc_timeout{5000};
    bool enable_compression = false;
    
    nlohmann::json toJson() const {
        return {
            {"type", static_cast<int>(type)},
            {"node_id", node_id},
            {"cluster_nodes", cluster_nodes},
            {"heartbeat_interval_ms", heartbeat_interval.count()},
            {"election_timeout_min_ms", election_timeout_min.count()},
            {"election_timeout_max_ms", election_timeout_max.count()},
            {"data_dir", data_dir},
            {"enable_persistence", enable_persistence}
        };
    }
};

} // namespace sharding
} // namespace themisdb
