/**
 * @file raft_shard_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/raft_consensus.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_topology.h"
#include "sharding/consistent_hash.h"
#include <memory>
#include <map>
#include <string>
#include <mutex>
#include <optional>
#include <functional>
#include <vector>

namespace themis::sharding {
class RaftTwoPhaseCommitDecisionStore;
}

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
    std::vector<std::string> replica_ids;               ///< Remote member node IDs in this shard's Raft group.
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
        RaftConsensus::Config raft_config;                       ///< Base settings; nested RaftConfig::node_id identifies this process's replica.
        size_t replication_factor{3};                            ///< Required replica count per shard.
        bool enable_auto_start{true};                            ///< Start Raft immediately after initialization.
        std::chrono::milliseconds leader_check_interval{1000};   ///< Polling interval for leader checks/monitoring.
        /**
         * @brief Resolve a verified mTLS client configuration for one remote member.
         * @param peer_node_id Configured member whose endpoint and client identity are needed.
         * @return Complete client configuration, or `std::nullopt` to fail initialization.
         *         Resolver exceptions and invalid configurations also make
         *         `initializeShard()` return false.
         * @note Each result must identify this local node and the requested remote
         *       node explicitly. An absent resolver leaves multi-node RPC callbacks
         *       unset, so elections and proposals remain fail-closed.
         */
        std::function<std::optional<themis::sharding::ShardRPCClient::Config>(
            const std::string& peer_node_id)> raft_peer_config_resolver;

        /**
         * @brief Resolve the local durable snapshot source used for follower catch-up.
         * @param shard_id Logical Raft group whose local snapshot should be sent.
         * @return Shared snapshot manager, or null when the group has no available snapshot.
         * @note Called concurrently by Raft heartbeat workers. The resolver must be
         *       thread-safe. Without it, lagging followers behind compaction remain
         *       unable to catch up and the transport fails closed.
         */
        std::function<std::shared_ptr<RaftSnapshotManager>(
            const std::string& shard_id)> raft_snapshot_manager_resolver;

        /// Deadline used when streaming a snapshot to a lagging peer; must be 1–300000 ms.
        uint32_t raft_snapshot_timeout_ms{300000};
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
     * @param shard_id Logical Raft group/shard identifier used as the manager key.
     * @param replica_ids Unique member node IDs, including the local ID configured in
     *        node ID in the manager's nested Raft configuration.
     * @return true if successful
     * @note Empty IDs, duplicate members, or a membership that omits the configured
     *       local node are rejected; the group ID is not treated as a node ID.
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
     * @brief Obtain shared access to a shard's local Raft consensus instance.
     * @param shard_id Logical Raft group identifier.
     * @return Shared local consensus instance, or nullptr if the group is absent.
     * @note Typed adapters such as `RaftTwoPhaseCommitDecisionStore` use this
     *       handle to attach group-specific state. It does not route requests to
     *       a remote leader; each operation must still validate local authority.
     * @note Shared ownership keeps the object alive, but does not keep a removed
     *       group registered or running.
     */
    [[nodiscard]] std::shared_ptr<RaftConsensus> getRaftInstance(
        const std::string& shard_id) const;

    /**
     * @brief Create the shared 2PC decision adapter for a managed shard.
     * @param shard_id Logical Raft group ID; also used as the decision-store group ID.
     * @param snapshot_manager Snapshot storage used to restore any installed decision state.
     * @return A decision store retaining the shard's Raft instance.
     * @throws std::invalid_argument if the shard has no managed Raft instance.
     * @throws std::runtime_error if its installed snapshot cannot be restored.
     * @note Callers should share the returned store among coordinators using this
     *       group so each observes the same restored in-memory decision state.
     */
    [[nodiscard]] std::shared_ptr<
        themis::sharding::RaftTwoPhaseCommitDecisionStore>
    createTwoPhaseCommitDecisionStore(
        const std::string& shard_id,
        const RaftSnapshotManager& snapshot_manager) const;

    /**
     * @brief Refresh every live 2PC decision store after snapshot journal installation.
     * @param shard_id Logical Raft group whose snapshot was installed.
     * @param snapshot Verified snapshot whose boundary must equal the durable Raft anchor.
     * @return Number of live decision-store instances refreshed; zero when none exist.
     * @throws std::invalid_argument if the group is not managed.
     * @throws std::runtime_error if the installed boundary differs or a live store
     *         cannot restore the snapshot's decision extension.
     * @note Call only after application restoration and
     *       `RaftState::installSnapshotFromLeader()` succeed. The method serializes
     *       with store creation so no active adapter misses the refresh.
     */
    [[nodiscard]] size_t restoreActiveTwoPhaseCommitDecisionStores(
        const std::string& shard_id,
        const RaftSnapshot& snapshot) const;
    
    /**
     * @brief Get configuration
        * @return Immutable manager configuration snapshot.
     */
    const Config& getConfig() const { return config_; }
    
private:
    Config config_;  ///< Immutable manager configuration.
    
    // Map of shard_id to Raft consensus instance
    std::map<std::string, std::shared_ptr<RaftConsensus>> raft_instances_; ///< Managed Raft instances per shard.
    mutable std::map<std::string, std::vector<std::weak_ptr<
        themis::sharding::RaftTwoPhaseCommitDecisionStore>>> active_decision_stores_;
    mutable std::mutex mutex_;                                              ///< Protects instance map and lifecycle operations.
    
    /**
     * @brief Create Raft configuration for a shard
        * @param shard_id Logical Raft group identifier.
        * @param replica_ids Unique member node IDs including the configured local node.
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
