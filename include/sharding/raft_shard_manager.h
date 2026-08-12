/**
 * @file raft_shard_manager.h
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
    std::string shard_id;                                ///< Shard identifier.
    RaftNodeState role;                                  ///< Current Raft role.
    uint64_t term;                                       ///< Current Raft term.
    uint64_t commit_index;                               ///< Last committed log index.
    uint64_t last_applied;                               ///< Last applied log index (or proxy value).
    std::string leader_id;                               ///< Current leader ID, empty when unknown.
    bool has_quorum;                                     ///< True when shard currently has quorum.
    bool is_healthy;                                     ///< Aggregated shard health flag.
    std::chrono::steady_clock::time_point last_heartbeat; ///< Last observed heartbeat timestamp.
    std::vector<std::string> replica_ids;               ///< Replica IDs participating in shard Raft group.
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
        RaftConsensus::Config raft_config;                       ///< Base Raft settings inherited per shard.
        size_t replication_factor{3};                            ///< Required replica count per shard.
        bool enable_auto_start{true};                            ///< Start Raft immediately after initialization.
        std::chrono::milliseconds leader_check_interval{1000};   ///< Polling interval for leader checks/monitoring.
    };

    /**
     * @brief Construct manager for per-shard Raft instances.
     * @param config Base per-shard configuration.
     */
    explicit RaftShardManager(const Config& config);

    /** @brief Stop and destroy all managed Raft shard instances. */
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
        * @return Immutable manager configuration snapshot.
     */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;  ///< Immutable manager configuration.
    
    // Map of shard_id to Raft consensus instance
    std::map<std::string, std::shared_ptr<RaftConsensus>> raft_instances_; ///< Managed Raft instances per shard.
    mutable std::mutex mutex_;                                              ///< Protects instance map and lifecycle operations.
    
    /**
     * @brief Create Raft configuration for a shard
        * @param shard_id Shard identifier used as local node ID.
        * @param replica_ids Replica set for shard consensus group.
        * @return Fully populated RaftConsensus configuration for shard.
     */
    RaftConsensus::Config createRaftConfig(const std::string& shard_id,
                                          const std::vector<std::string>& replica_ids);
};

}  // namespace sharding
}  // namespace themisdb

// Helper function to convert RaftNodeState to string
namespace themisdb {
namespace sharding {

/**
 * @brief Convert Raft node state enum to readable string.
 * @param state Raft node state enum value.
 * @return "LEADER", "FOLLOWER", "CANDIDATE", or "UNKNOWN".
 */
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
