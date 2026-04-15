/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_shard_manager.h                               ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     205                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/raft_consensus.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <optional>

namespace themisdb {
namespace sharding {

/**
 * @brief Raft instance information per shard
 */
struct ShardRaftInfo {
    std::string shard_id;
    RaftNodeState role;         // FOLLOWER, CANDIDATE, or LEADER
    uint64_t term;              // Current Raft term
    uint64_t commit_index;      // Last committed log index
    uint64_t last_applied;      // Last applied log index
    std::string leader_id;      // Current leader shard ID (empty if unknown)
    bool has_quorum;            // Does shard have quorum?
    bool is_healthy;            // Overall health status
    std::chrono::steady_clock::time_point last_heartbeat;
    std::vector<std::string> replica_ids;  // Replica shard IDs in Raft group
};

/**
 * @brief Manages Raft consensus instances per shard
 * 
 * This class creates and manages one RaftConsensus instance for each shard,
 * enabling per-shard leader election, write enforcement, and failover.
 * 
 * Key responsibilities:
 * - Create/destroy Raft instances as shards are added/removed
 * - Track leader status per shard
 * - Route writes to shard leaders
 * - Handle leader elections and failover
 * - Provide Raft health/metrics per shard
 */
class RaftShardManager {
public:
    /**
     * @brief Configuration for Raft per shard
     */
    struct Config {
        RaftConsensus::Config raft_config;
        size_t replication_factor{3};  // Number of replicas per shard
        bool enable_auto_start{true};  // Auto-start Raft on shard addition
        std::chrono::milliseconds leader_check_interval{1000};
    };
    
    explicit RaftShardManager(const Config& config);
    ~RaftShardManager();
    
    // Prevent copying
    RaftShardManager(const RaftShardManager&) = delete;
    RaftShardManager& operator=(const RaftShardManager&) = delete;
    
    /**
     * @brief Initialize Raft for a shard
     * Creates a new Raft consensus instance for the shard
     * @param shard_id Shard identifier
     * @param replica_ids List of replica shard IDs in Raft group
     * @return true if successful
     */
    bool initializeShard(const std::string& shard_id,
                        const std::vector<std::string>& replica_ids);
    
    /**
     * @brief Remove Raft instance for a shard
     * @param shard_id Shard identifier
     */
    void removeShard(const std::string& shard_id);
    
    /**
     * @brief Start Raft consensus for a shard
     * @param shard_id Shard identifier
     * @return true if successful
     */
    bool startShard(const std::string& shard_id);
    
    /**
     * @brief Stop Raft consensus for a shard
     * @param shard_id Shard identifier
     */
    void stopShard(const std::string& shard_id);
    
    /**
     * @brief Check if a shard is the leader
     * @param shard_id Shard identifier
     * @return true if this node is leader for the shard
     */
    bool isShardLeader(const std::string& shard_id) const;
    
    /**
     * @brief Get the leader shard ID for a given shard
     * @param shard_id Shard identifier
     * @return Leader shard ID, or empty if unknown
     */
    std::string getShardLeader(const std::string& shard_id) const;
    
    /**
     * @brief Propose a write to a shard (must be leader)
     * @param shard_id Shard identifier
     * @param command Command to replicate via Raft log
     * @return Future that resolves when committed or fails
     */
    std::future<bool> proposeWrite(const std::string& shard_id,
                                   const std::string& command);
    
    /**
     * @brief Get Raft info for a shard
     * @param shard_id Shard identifier
     * @return Raft info if shard exists
     */
    std::optional<ShardRaftInfo> getShardRaftInfo(const std::string& shard_id) const;
    
    /**
     * @brief Get Raft info for all shards
     * @return Map of shard_id to Raft info
     */
    std::map<std::string, ShardRaftInfo> getAllShardRaftInfo() const;
    
    /**
     * @brief Check if shard has quorum
     * @param shard_id Shard identifier
     * @return true if shard has quorum
     */
    bool hasQuorum(const std::string& shard_id) const;
    
    /**
     * @brief Get the Raft consensus instance for a shard
     * For advanced operations (testing, debugging)
     * @param shard_id Shard identifier
     * @return Raft consensus instance or nullptr
     */
    std::shared_ptr<RaftConsensus> getRaftInstance(const std::string& shard_id) const;
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;
    
    // Map of shard_id to Raft consensus instance
    std::map<std::string, std::shared_ptr<RaftConsensus>> raft_instances_;
    mutable std::mutex mutex_;
    
    /**
     * @brief Create Raft configuration for a shard
     */
    RaftConsensus::Config createRaftConfig(const std::string& shard_id,
                                          const std::vector<std::string>& replica_ids);
};

}  // namespace sharding
}  // namespace themisdb

// Helper function to convert RaftNodeState to string
namespace themisdb {
namespace sharding {

inline std::string raftNodeStateToString(RaftNodeState state) {
    switch (state) {
        case RaftNodeState::LEADER: return "LEADER";
        case RaftNodeState::FOLLOWER: return "FOLLOWER";
        case RaftNodeState::CANDIDATE: return "CANDIDATE";
        default: return "UNKNOWN";
    }
}

}  // namespace sharding
}  // namespace themisdb
