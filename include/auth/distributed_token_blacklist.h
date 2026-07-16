/*
 * ThemisDB | File: distributed_token_blacklist.h | Version: 0.0.1
 * Author: Copilot | Maturity: 🟡 BETA | Status: New Implementation
 * Purpose: Distributed token blacklist with cluster synchronization
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

namespace themis {
namespace auth {

/**
 * @brief Cluster node information for blacklist replication
 */
struct ClusterNode {
    std::string node_id;        ///< Unique node identifier
    std::string rpc_address;    ///< Network address for RPC (e.g., "192.168.1.1:9090")
    int rpc_port{9090};         ///< RPC port for replication
    bool is_leader{false};      ///< Whether this node is the leader
};

/**
 * @brief Configuration for distributed token blacklist
 */
struct DistributedBlacklistConfig {
    /// Path to the persistent storage (RocksDB)
    std::string db_path;
    
    /// Name of the column family for the blacklist
    std::string column_family = "distributed_blacklist";
    
    /// How often to purge expired entries (seconds)
    uint32_t purge_interval_seconds = 300;
    
    /// How often to sync with peer nodes (seconds)
    uint32_t sync_interval_seconds = 30;
    
    /// Maximum time to wait for a peer RPC response (milliseconds)
    int peer_rpc_timeout_ms = 5000;
    
    /// Local cluster node configuration
    ClusterNode local_node;
    
    /// List of peer nodes in the cluster
    std::vector<ClusterNode> peer_nodes;
    
    /// Enable cluster synchronization (disable for single-node deployments)
    bool enable_cluster_sync = true;
};

/**
 * @brief Distributed token blacklist implementation
 *
 * Extends the RocksDB-backed token blacklist with cluster synchronization:
 *
 * - Local: Persists revoked JTIs to RocksDB for durability across restarts
 * - Distributed: Periodically syncs the local blacklist state with peer nodes
 * - Atomic: Revocation checks remain deterministic during sync operations
 * - Resilient: Continues operation if peers are temporarily unavailable
 *
 * Architecture:
 *   - Leader election: One node acts as the replication leader
 *   - Pull-based sync: Followers periodically pull updates from the leader
 *   - Conflict resolution: Latest timestamp wins (Last-Write-Wins)
 *   - Fault tolerance: If leader fails, another node is elected
 *
 * Thread-safety: All public methods are thread-safe. Replication happens
 * in background threads without blocking revocation checks.
 *
 * Performance targets (auth roadmap v1.3.0):
 *   - isRevoked() remains constant-time lookup (1 RocksDB read)
 *   - add() to RocksDB + local cache (negligible overhead)
 *   - Cluster sync happens every 30 seconds without blocking callers
 */
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
     * @brief Open the RocksDB database and start background threads
     *
     * When cluster_sync is enabled, connects to peer nodes and begins
     * periodic synchronization.
     *
     * @param config Configuration for the distributed blacklist
     * @throws std::runtime_error if the database cannot be opened
     */
    explicit DistributedTokenBlacklist(const DistributedBlacklistConfig& config);
    
    /**
     * @brief Stop background threads and close the database
     */
    ~DistributedTokenBlacklist() override;
    
    DistributedTokenBlacklist(const DistributedTokenBlacklist&) = delete;
    DistributedTokenBlacklist& operator=(const DistributedTokenBlacklist&) = delete;
    
    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------
    
    /**
     * @brief Add a revoked JTI to the blacklist
     *
     * Stores locally and marks for replication to peer nodes.
     */
    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;
    
    /**
     * @brief Check if a JTI is revoked
     *
     * Fast O(1) lookup in local RocksDB. Does not wait for cluster sync.
     *
     * @return true if revoked and not expired, false otherwise
     */
    bool isRevoked(const std::string& jti) const override;
    
    /**
     * @brief Delete expired entries
     *
     * Called by the background purge thread periodically. Safe to call
     * manually for maintenance.
     */
    void purgeExpired() override;
    
    // -----------------------------------------------------------------------
    // Distributed-specific methods
    // -----------------------------------------------------------------------
    
    /**
     * @brief Manually trigger cluster synchronization
     *
     * Useful for testing or immediate consistency requirements.
     * Non-blocking: returns a future that completes when sync finishes.
     *
     * @return std::future<bool> — true if sync succeeded, false if failed
     */
    std::future<bool> syncWithCluster();
    
    /**
     * @brief Get replication statistics
     *
     * Returns counters for monitoring cluster health.
     */
    ReplicationStats getReplicationStats() const;
    
    /**
     * @brief Return the current configuration
     */
    const DistributedBlacklistConfig& config() const { return config_; }
    
    /**
     * @brief Check if this node is the replication leader
     */
    bool isLeader() const { return is_leader_.load(); }
    
    /**
     * @brief Wait for at least one successful sync with the cluster
     *
     * Useful at startup to ensure the node has converged with peers.
     *
     * @param timeout Maximum time to wait (zero = no limit)
     * @return true if converged, false if timeout expired
     */
    bool waitForClusterConvergence(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    
private:
    DistributedBlacklistConfig config_;
    
    // RocksDB state (same as RocksDBTokenBlacklist)
    void* db_{nullptr};  // rocksdb::DB*
    void* cf_{nullptr};  // rocksdb::ColumnFamilyHandle*
    std::vector<void*> other_cf_handles_;
    
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
    
    // Background loops
    void purgeLoop();
    void replicationLoop();
    
    // RPC handlers (for peer-to-peer communication)
    bool performClusterSync();
    bool performLeaderElection();
    bool pushRevisionsToFollower(const std::string& peer_address);
    bool pullRevisionsFromLeader(const std::string& leader_address);
    
    // Encoding/decoding helpers
    static std::string encodeExpiry(std::chrono::system_clock::time_point tp);
    static std::chrono::system_clock::time_point decodeExpiry(const std::string& val);
};

} // namespace auth
} // namespace themis
