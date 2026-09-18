/**
 * @file distributed_token_blacklist.h
 * @brief Distributed token blacklist with TCP cluster synchronization.
 *
 * Provides DistributedTokenBlacklist implementing ITokenBlacklist via a
 * peer-to-peer TBLK/v1 RPC protocol.  Nodes gossip revocations so that
 * every replica reaches eventual consistency within the configured TTL.
 */

#pragma once

#include "auth/token_blacklist.h"

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <chrono>
#include <functional>
#include <future>
#include <rocksdb/db.h>

namespace themis {
namespace auth {

struct ClusterNode {
    std::string node_id;        ///< Unique node identifier
    std::string rpc_address;    ///< Network address for RPC (e.g., "192.168.1.1:9090")
    int rpc_port{9090};         ///< RPC port for replication
    bool is_leader{false};      ///< Whether this node is the leader
};

struct DistributedBlacklistConfig {
    std::string db_path;
    
    std::string column_family = "distributed_blacklist";
    
    uint32_t purge_interval_seconds = 300;
    
    uint32_t sync_interval_seconds = 30;
    
    int peer_rpc_timeout_ms = 5000;
    
    ClusterNode local_node;
    
    std::vector<ClusterNode> peer_nodes;
    
    bool enable_cluster_sync = true;
};

class DistributedTokenBlacklist final : public ITokenBlacklist {
public:
    struct ReplicationStats {
        uint64_t total_syncs{0};
        uint64_t successful_syncs{0};
        uint64_t failed_syncs{0};
        uint64_t entries_pushed{0};
        uint64_t entries_pulled{0};
        std::chrono::system_clock::time_point last_sync_time;
    };
    
    /**
     * @brief Distributed Token Blacklist.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit DistributedTokenBlacklist(const DistributedBlacklistConfig& config);
    
    ~DistributedTokenBlacklist() override;
    
    DistributedTokenBlacklist(const DistributedTokenBlacklist&) = delete;
    DistributedTokenBlacklist& operator=(const DistributedTokenBlacklist&) = delete;
    
    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------
    
    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;
    
    bool isRevoked(const std::string& jti) const override;
    
    void purgeExpired() override;
    
    // -----------------------------------------------------------------------
    // Distributed-specific methods
    // -----------------------------------------------------------------------
    
    /**
     * @brief Sync With Cluster.
     * @return Return value.
     */
    std::future<bool> syncWithCluster();
    
    /**
     * @brief Get Replication Stats.
     * @return Return value.
     */
    ReplicationStats getReplicationStats() const;
    
    const DistributedBlacklistConfig& config() const { return config_; }
    
    bool isLeader() const { return is_leader_.load(); }
    
    bool waitForClusterConvergence(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    
private:
    DistributedBlacklistConfig config_;
    
    // RocksDB state (raw pointer; destruction handled manually)
    rocksdb::DB* db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};
    std::vector<rocksdb::ColumnFamilyHandle*> other_cf_handles_;
    
    // Background threads
    std::thread purge_thread_;
    std::thread replication_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> is_leader_{false};
    
    // Synchronization
    mutable std::mutex cv_mutex_;
    std::condition_variable cv_;
    
    // Replication state
    mutable std::mutex stats_mutex_;
    ReplicationStats stats_;
    
    std::atomic<std::chrono::system_clock::time_point> last_successful_sync_;
    
    std::uintptr_t server_fd_{static_cast<std::uintptr_t>(-1)};
    
    std::thread listener_thread_;
    
    // Background loops
    /**
     * @brief Purge Loop.
     */
    void purgeLoop();
    /**
     * @brief Replication Loop.
     */
    void replicationLoop();
    
    /**
     * @brief TCP server listener — accepts PUSH and PULL_REQ connections from cluster peers
     */
    void serveIncomingConnections();
    
    /**
     * @brief Handle Peer Connection.
     * @param[in] client_fd Input parameter.
     */
    void handlePeerConnection(std::uintptr_t client_fd);
    
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>
        getAllEntries() const;
    
    void applyEntries(const std::vector<std::pair<std::string, int64_t>>& entries);
    
    /**
     * @brief RPC handlers (for peer-to-peer communication)
     * @return True when the operation succeeds.
     */
    bool performClusterSync();
    /**
     * @brief Perform Leader Election.
     * @return True when the operation succeeds.
     */
    bool performLeaderElection();
    /**
     * @brief Push Revisions To Follower.
     * @param[in] peer_address Input parameter.
     * @return True when the operation succeeds.
     */
    bool pushRevisionsToFollower(const std::string& peer_address);
    /**
     * @brief Pull Revisions From Leader.
     * @param[in] leader_address Input parameter.
     * @return True when the operation succeeds.
     */
    bool pullRevisionsFromLeader(const std::string& leader_address);
    
    // Encoding/decoding helpers
    /**
     * @brief Encode Expiry.
     * @param[in] tp Input parameter.
     * @return Return value.
     */
    static std::string encodeExpiry(std::chrono::system_clock::time_point tp);
    /**
     * @brief Decode Expiry.
     * @param[in] val Input parameter.
     * @return Return value.
     */
    static std::chrono::system_clock::time_point decodeExpiry(const std::string& val);
};

} // namespace auth
} // namespace themis
