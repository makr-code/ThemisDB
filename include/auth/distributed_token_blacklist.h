/*
 * ThemisDB | File: distributed_token_blacklist.h | Version: 0.1.0
 * Author: Copilot | Maturity: 🟡 BETA | Status: Production Implementation (v1.3.0)
 * Purpose: Distributed token blacklist with full TCP cluster synchronization (TBLK/v1 RPC)
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
 * @brief Distributed token blacklist with full cluster synchronization (v1.3.0)
 *
 * Extends the RocksDB-backed token blacklist with production-grade cluster sync:
 *
 * - Local: Persists revoked JTIs to RocksDB for durability across restarts
 * - Distributed: Periodically syncs local blacklist state with peer nodes via TCP
 * - Atomic: Revocation checks remain deterministic and constant-time during sync
 * - Resilient: Continues operation if peers are temporarily unavailable
 *
 * ### Architecture
 *   - Leader election: node with the lowest `node_id` string is the replication leader
 *   - Leader pushes all non-expired revocations to each follower via TCP PUSH messages
 *   - Follower pulls revocations from the leader via TCP PULL_REQ / PULL_RESP exchange
 *   - Every node runs a TCP server listener on `local_node.rpc_port` to receive inbound
 *     connections from peers; bind failure is non-fatal (node still initiates outbound)
 *   - Conflict resolution: Last-Write-Wins on expiry timestamp; older entries are overwritten
 *
 * ### Wire Protocol (TBLK/v1)
 *   - 10-byte header: magic[4] = "TBLK", version[1] = 0x01, type[1], count[4] (big-endian)
 *   - Message types: PUSH(0x01), PULL_REQ(0x02), PULL_RESP(0x03), ACK(0x04)
 *   - Entry encoding: jti_len[2] + jti[jti_len] + expiry[8] (unix seconds, big-endian int64)
 *   - Timeouts: `peer_rpc_timeout_ms` controls connect/send/recv deadlines
 *
 * ### Thread-safety
 *   All public methods are thread-safe. Replication and purge happen in background
 *   threads without blocking revocation checks (`isRevoked()` is one RocksDB read).
 *
 * ### Performance targets (v1.3.0)
 *   - `isRevoked()`: O(1) RocksDB point-read, < 1 µs on warm cache
 *   - `add()`: < 1 ms (single RocksDB Put)
 *   - Cluster sync every 30 seconds (configurable); does not block validation hot path
 *   - Leader election converges in < 1 second (local node-ID comparison)
 *
 * ### Failure / degradation contract (auth_principal_contract.h §5)
 *   - If the RocksDB backend is unavailable at construction, the constructor throws
 *     std::runtime_error; no partial-open state is exposed to callers.
 *   - If the backend becomes unavailable after construction, isRevoked() MUST return
 *     true (deny) for any JTI that cannot be positively confirmed from the local cache.
 *     Implementations that expose this class SHOULD emit AuthErrorCode::REVOCATION_BACKEND_UNAVAILABLE.
 *   - Cluster sync failures are non-fatal to the local node: revocations continue to be
 *     accepted and persisted locally. Failures are reported via ReplicationStats::failed_syncs
 *     and logged at WARNING level. Callers MAY surface AuthErrorCode::REVOCATION_CLUSTER_SYNC_FAILED
 *     to operators via metrics/alerting.
 *   - JTI length exceeding kMaxJtiBytes (1024) causes add() to throw AuthException with
 *     AuthErrorCode::REVOCATION_ENTRY_INVALID; isRevoked() for such a JTI returns false
 *     (unknown, not revoked) — callers SHOULD validate JTI size at the inbound boundary.
 *   - Peer bind failure (TCP server listener) is non-fatal; outbound replication still works.
 *
 * @see include/auth/auth_principal_contract.h — §4 Fail-closed contract, §5 Revocation contract
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
    
    /// Server socket file descriptor that accepts inbound connections from cluster peers.
    /// Stored as std::uintptr_t to accommodate both POSIX (int) and Windows (SOCKET =
    /// ULONG_PTR) handle types without truncation. Sentinel value is static_cast<std::uintptr_t>(-1)
    /// (equivalent to INVALID_SOCKET on Windows and maps to UINTPTR_MAX on POSIX),
    /// meaning the listener could not be bound (non-fatal).
    std::uintptr_t server_fd_{static_cast<std::uintptr_t>(-1)};
    
    /// Background thread that runs the TCP accept loop for incoming peer connections.
    std::thread listener_thread_;
    
    // Background loops
    void purgeLoop();
    void replicationLoop();
    
    // TCP server listener — accepts PUSH and PULL_REQ connections from cluster peers
    void serveIncomingConnections();
    
    /**
     * @brief Handle a single inbound peer connection.
     *
     * Dispatches on the TBLK/v1 message type:
     *   - PUSH: reads entries sent by a leader and writes them to local RocksDB (LWW)
     *   - PULL_REQ: reads all non-expired local entries and sends them as PULL_RESP
     *
     * @param client_fd Accepted socket handle stored as std::uintptr_t to avoid truncating
     *                  the Windows SOCKET (ULONG_PTR) type to int.
     *                  Ownership is retained by the caller; this method closes nothing.
     */
    void handlePeerConnection(std::uintptr_t client_fd);
    
    /**
     * @brief Return all non-expired entries from RocksDB.
     *
     * Used by `handlePeerConnection` (PULL_REQ) and `pushRevisionsToFollower`.
     * Expired entries are omitted; the result is used as the push/pull payload.
     */
    std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>
        getAllEntries() const;
    
    /**
     * @brief Apply a batch of (jti, expiry_unix_seconds) pairs to local RocksDB.
     *
     * Entries with past expiry are silently dropped. Existing entries are overwritten
     * (Last-Write-Wins semantics). Writes are batched for efficiency.
     *
     * @param entries Vector of (jti, unix_seconds_since_epoch) pairs received from a peer.
     * @throws std::runtime_error if the RocksDB batch write fails.
     */
    void applyEntries(const std::vector<std::pair<std::string, int64_t>>& entries);
    
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
