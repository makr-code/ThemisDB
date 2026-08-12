/**
 * @file replication_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=3, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Manager
 * 
 * Leader-Follower Replication Architecture
 * 
 * Features:
 * - Asynchronous Replication with configurable lag
 * - Write-Ahead Log (WAL) based replication
 * - Automatic failover with leader election
 * - Read replicas for horizontal read scaling
 * - Conflict resolution for eventual consistency
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <queue>
#include <deque>
#include <map>
#include <set>

namespace themisdb {
namespace replication {

// Forward declarations
struct WALEntry;
class ReplicationStream;
class LeaderElection;
class CompressedReplicationStream;

/**
 * Replication Role
 */
enum class ReplicationRole {
    LEADER,         // Primary node accepting writes
    FOLLOWER,       // Read replica receiving updates
    CANDIDATE,      // Participating in leader election
    OBSERVER,       // Non-voting member (async replica)
    WITNESS         // Vote-only member: participates in quorum but stores no data
};

/**
 * Replication Mode
 */
enum class ReplicationMode {
    SYNC,           // Synchronous: wait for all replicas
    SEMI_SYNC,      // Semi-synchronous: wait for quorum
    ASYNC           // Asynchronous: don't wait
};

/**
 * Conflict Resolution Strategy
 */
enum class ConflictResolution {
    LAST_WRITE_WINS,    // Timestamp-based
    FIRST_WRITE_WINS,   // First value preserved
    VECTOR_CLOCK,       // Causal ordering
    CUSTOM              // User-defined resolver
};

/**
 * Replica Health Status
 */
enum class HealthStatus {
    HEALTHY,        // Replica is responding and up-to-date
    DEGRADED,       // Replica is lagging but responsive
    FAILED,         // Replica is not responding
    UNKNOWN         // Health status not yet determined
};

/**
 * Read Preference for query routing
 */
enum class ReadPreference {
    PRIMARY,        // Read from primary only
    SECONDARY,      // Read from secondary replicas only
    PRIMARY_PREFERRED,  // Prefer primary, fallback to secondary
    SECONDARY_PREFERRED, // Prefer secondary, fallback to primary
    NEAREST         // Read from replica with lowest latency
};

/**
 * Write-Ahead Log Entry
 */
struct WALEntry {
    uint64_t sequence_number;       // Monotonic sequence
    uint64_t term;                  // Leader term (Raft-like)
    std::chrono::system_clock::time_point timestamp;
    std::string operation;          // INSERT, UPDATE, DELETE
    std::string collection;
    std::string document_id;
    std::string data;               // JSON payload
    std::string checksum;           // SHA-256 integrity check
    
    // Serialize to binary format
    std::vector<uint8_t> serialize() const;
    
    // Deserialize from binary format
    static std::optional<WALEntry> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Replica Node Information
 */
struct ReplicaInfo {
    std::string node_id;
    std::string endpoint;           // hostname:port
    ReplicationRole role;
    uint64_t last_applied_sequence;
    uint64_t last_applied_term;
    std::chrono::system_clock::time_point last_heartbeat;
    bool is_voting_member;
    std::string datacenter;
    int32_t priority;               // For leader election preference
    HealthStatus health_status = HealthStatus::UNKNOWN;
    uint32_t consecutive_failures = 0;
    std::chrono::system_clock::time_point last_failure_time;
    
    bool isHealthy() const {
        return health_status == HealthStatus::HEALTHY;
    }
    
    bool isHealthyWithTimeout(uint32_t timeout_ms) const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_heartbeat
        ).count();
        return elapsed < static_cast<int64_t>(timeout_ms);
    }
    
    int64_t replicationLagMs() const;
    
    void updateHealthStatus(uint32_t heartbeat_timeout_ms, uint32_t degraded_lag_threshold_ms);
};

/**
 * Replication Configuration
 */
struct ReplicationConfig {
    // Basic settings
    bool enabled = false;
    ReplicationMode mode = ReplicationMode::ASYNC;
    ConflictResolution conflict_strategy = ConflictResolution::LAST_WRITE_WINS;
    
    // Timing
    uint32_t heartbeat_interval_ms = 1000;
    uint32_t election_timeout_min_ms = 3000;
    uint32_t election_timeout_max_ms = 5000;
    uint32_t replication_timeout_ms = 10000;
    
    // Batching
    uint32_t batch_size = 100;
    uint32_t batch_timeout_ms = 50;
    
    // WAL settings
    std::string wal_directory = "/var/lib/themisdb/wal";
    uint64_t wal_segment_size_bytes = 64 * 1024 * 1024;  // 64MB
    uint32_t wal_retention_segments = 100;
    bool wal_sync_on_commit = true;
    
    // Quorum settings
    uint32_t min_sync_replicas = 1;         // For semi-sync mode
    bool allow_stale_reads = false;
    uint32_t max_replication_lag_ms = 10000;
    
    // HA settings
    bool enable_auto_failover = true;
    uint32_t failure_detection_timeout_ms = 5000;
    uint32_t min_quorum_for_failover = 2;  // Minimum replicas needed for quorum
    uint32_t max_consecutive_failures = 3;  // Failures before marking as FAILED
    uint32_t degraded_lag_threshold_ms = 5000;  // Lag threshold for DEGRADED status
    ReadPreference default_read_preference = ReadPreference::PRIMARY_PREFERRED;

    // Lease read settings (Raft leader lease reads)
    bool enable_leader_lease = true;
    // Lease duration must be strictly less than election_timeout_min_ms to guarantee
    // that no follower can start a new election while the leader's lease is valid.
    uint32_t leader_lease_duration_ms = 2500;
    
    // WAL compression (Zstd) for bandwidth reduction
    bool enable_wal_compression = false;
    std::string wal_compression_algorithm = "zstd";  // "zstd", "lz4", "snappy", "auto", "none"
    int wal_compression_level = 3;                   // 1–9 (validated when enabled)
    uint64_t wal_compression_min_batch_bytes = 1024; // Skip compression for batches < this size

    // TLS/Security
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    bool require_mtls = true;
    
    // Initial cluster members
    std::vector<std::string> seed_nodes;
};

// ============================================================================
// Lag-Based Read Traffic Shifter (v1.7.0)
// ============================================================================

/**
 * LagBasedReadRouter
 *
 * Automatically shifts read traffic away from replicas whose replication lag
 * exceeds a configurable threshold, routing those reads to healthier replicas
 * or falling back to the primary.
 *
 * Algorithm:
 *  - Each replica is eligible for reads only when its lag <= lag_threshold_ms.
 *  - Among eligible replicas, the one with the lowest current lag is preferred
 *    (ties broken by replica declaration order).
 *  - If NO secondary is eligible, reads are redirected to the primary node.
 *  - ReadPreference semantics are preserved: PRIMARY always uses primary;
 *    SECONDARY returns empty when no eligible secondary exists.
 *
 * Usage:
 *   LagBasedReadRouter::RouterConfig cfg;
 *   cfg.lag_threshold_ms = 5000;
 *   LagBasedReadRouter router(cfg);
 *   auto dec = router.selectReplica(ReadPreference::SECONDARY_PREFERRED,
 *                                   replicas, primary_node_id);
 */
class LagBasedReadRouter {
public:
    struct RouterConfig {
        /// Replicas with lag > lag_threshold_ms are excluded from read routing.
        int64_t  lag_threshold_ms  = 10000;
        /// When true, a replica that just recovered below the threshold is
        /// re-admitted immediately rather than requiring a grace period.
        bool     immediate_reentry = true;
    };

    /// Result of a routing decision.
    struct RoutingDecision {
        std::string node_id;                  ///< Selected node (empty = no eligible node)
        bool        is_primary     = false;   ///< true when the primary was selected
        int64_t     replica_lag_ms = -1;      ///< Lag of the selected replica (-1 for primary)
        std::string reason;                   ///< Human-readable explanation
    };

    LagBasedReadRouter();  ///< Uses default RouterConfig (lag_threshold_ms = 10000)
    explicit LagBasedReadRouter(const RouterConfig& config);

    /**
     * Select the best node to serve a read request.
     *
     * @param preference      Caller's read preference.
     * @param replicas        Current snapshot of all replica infos.
     * @param primary_node_id Node ID of the current primary.
     * @return RoutingDecision describing which node to use and why.
     */
    RoutingDecision selectReplica(
        ReadPreference preference,
        const std::vector<ReplicaInfo>& replicas,
        const std::string& primary_node_id) const;

    /// Returns the number of replicas currently below the lag threshold.
    size_t eligibleReplicaCount(const std::vector<ReplicaInfo>& replicas) const;

    /// Export current routing state as Prometheus metrics.
    std::string exportPrometheusMetrics(
        const std::vector<ReplicaInfo>& replicas) const;

    void setConfig(const RouterConfig& config);
    const RouterConfig& getConfig() const { return config_; }

private:
    RouterConfig config_;
};

/**
 * Replication Statistics
 */
struct ReplicationStats {
    std::atomic<uint64_t> entries_replicated{0};
    std::atomic<uint64_t> bytes_replicated{0};
    std::atomic<uint64_t> replication_errors{0};
    std::atomic<uint64_t> leader_elections{0};
    std::atomic<uint64_t> conflicts_resolved{0};
    std::atomic<int64_t> max_replication_lag_ms{0};
    std::atomic<int64_t> avg_replication_lag_ms{0};
    std::atomic<uint64_t> automatic_failovers{0};
    std::atomic<uint64_t> manual_failovers{0};
    mutable std::atomic<uint64_t> replica_failures_detected{0};
    std::atomic<uint64_t> network_partitions_detected{0};
    // Leader lease read counters
    mutable std::atomic<uint64_t> lease_reads_served{0};
    mutable std::atomic<uint64_t> lease_reads_rejected{0};
    
    // Prometheus metrics export
    std::string toPrometheusFormat() const;
};

/**
 * Custom Conflict Resolver Interface
 */
class IConflictResolver {
public:
    virtual ~IConflictResolver() = default;
    
    /**
     * Resolve conflict between two versions of a document
     * @param local Local version
     * @param remote Remote version
     * @return Resolved document (merged or selected)
     */
    [[nodiscard]] virtual std::string resolve(
        const std::string& local,
        const std::string& remote,
        const std::string& collection,
        const std::string& document_id
    ) = 0;
};

/**
 * Last-Write-Wins conflict resolver based on embedded timestamp field.
 * Selects the document with the higher "updated_at" (milliseconds epoch)
 * in its JSON payload. Falls back to remote on parse errors.
 */
class LWWConflictResolver : public IConflictResolver {
public:
    std::string resolve(
        const std::string& local,
        const std::string& remote,
        const std::string& collection,
        const std::string& document_id
    ) override;

private:
    // Extract "updated_at" timestamp from a minimal JSON string.
    // Returns -1 if the field is absent or unparsable.
    static int64_t extractTimestamp(const std::string& json_doc);
};

/**
 * Simple CRDT-style merge resolver.
 * For numeric fields it keeps the larger value (grow-only counter
 * semantics); for all other fields it falls back to Last-Write-Wins.
 */
class CRDTConflictResolver : public IConflictResolver {
public:
    std::string resolve(
        const std::string& local,
        const std::string& remote,
        const std::string& collection,
        const std::string& document_id
    ) override;
};

/**
 * Replication Event Listener
 */
class IReplicationListener {
public:
    virtual ~IReplicationListener() = default;
    
    virtual void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) = 0;
    virtual void onLeaderElected(const std::string& leader_id) = 0;
    virtual void onReplicaAdded(const ReplicaInfo& replica) = 0;
    virtual void onReplicaRemoved(const std::string& node_id) = 0;
    virtual void onConflictDetected(const std::string& document_id) = 0;
    virtual void onReplicationLagWarning(int64_t lag_ms) = 0;
    virtual void onReplicaHealthChanged(const std::string& node_id, HealthStatus old_status, HealthStatus new_status) = 0;
    virtual void onFailoverStarted(const std::string& failed_leader_id, const std::string& new_leader_id) = 0;
    virtual void onFailoverCompleted(const std::string& new_leader_id, bool success) = 0;
    virtual void onNetworkPartitionDetected(const std::vector<std::string>& unreachable_nodes) = 0;

    // Called each time a WAL entry is successfully replicated (used by CDC)
    virtual void onWALEntryApplied(const WALEntry& /*entry*/) {}
};

/**
 * Write-Ahead Log Manager
 */
class WALManager {
public:
    explicit WALManager(const ReplicationConfig& config);
    ~WALManager();
    
    // Append entry to WAL
    uint64_t append(const WALEntry& entry);
    
    // Read entries starting from sequence number
    std::vector<WALEntry> readFrom(uint64_t start_sequence, uint32_t limit = 1000);
    
    // Get current sequence number
    uint64_t getCurrentSequence() const { return current_sequence_.load(); }
    
    // Get current term
    uint64_t getCurrentTerm() const { return current_term_.load(); }
    
    // Increment term (for leader election)
    uint64_t incrementTerm();
    
    // Truncate WAL up to sequence (for compaction)
    void truncateBefore(uint64_t sequence);
    
    // Sync WAL to disk
    void sync();
    
    // Get WAL size in bytes
    uint64_t getSize() const;
    
    // Get the internal WAL mutex for external synchronization
    // (used by ReplicationStream for atomic read operations)
    std::mutex& getMutex() { return wal_mutex_; }

private:
    ReplicationConfig config_;
    std::atomic<uint64_t> current_sequence_{0};
    std::atomic<uint64_t> current_term_{0};
    std::mutex wal_mutex_;
    
    // Internal file handling
    void rotateSegment();
    void loadFromDisk();
};

/**
 * Leader Election using Raft-like consensus
 */
class LeaderElection {
public:
    explicit LeaderElection(
        const std::string& node_id,
        const ReplicationConfig& config,
        std::shared_ptr<WALManager> wal
    );
    ~LeaderElection();
    
    // Start election process
    void startElection();
    
    // Request vote from this node
    bool requestVote(
        uint64_t term,
        const std::string& candidate_id,
        uint64_t last_log_sequence,
        uint64_t last_log_term
    );
    
    // Receive heartbeat from leader
    void receiveHeartbeat(
        uint64_t term,
        const std::string& leader_id,
        uint64_t leader_commit
    );
    
    // Get current role
    ReplicationRole getRole() const { return role_.load(); }
    
    // Get current leader ID
    std::string getLeaderId() const;
    
    // Is this node the leader?
    bool isLeader() const { return role_.load() == ReplicationRole::LEADER; }
    
    // Inform the election module of the current cluster size (for quorum calculation)
    void setClusterSize(uint32_t size) { cluster_size_.store(size); }
    
    // Record an incoming vote grant for the current term (called by ReplicationManager
    // when a peer replies positively to our RequestVote RPC simulation)
    void grantVote(uint64_t term);
    
    // Start the background election-timeout loop
    void start();
    
    // Get current term
    uint64_t getCurrentTerm() const { return current_term_.load(); }

    /**
     * Returns the highest WAL sequence that this follower knows to be
     * committed by the leader quorum.  Updated by receiveHeartbeat().
     * Leaders always have commit_index == their own getCurrentSequence.
     */
    uint64_t getCommitIndex() const { return commit_index_.load(); }

    // Raft leader lease management -----------------------------------------

    /**
     * Renew the leader lease for `duration_ms` milliseconds from now.
     * Must only be called by the leader after successfully broadcasting a
     * heartbeat to the quorum.
     */
    void renewLease(uint32_t duration_ms);

    /**
     * Returns true if this node holds a valid (non-expired) leader lease.
     * A valid lease guarantees that no other node can have been elected
     * leader since the lease was last renewed.
     */
    bool hasValidLease() const;

    /**
     * Returns the absolute time at which the current lease expires.
     * Returns a past time-point when no lease is held.
     */
    std::chrono::steady_clock::time_point leaseExpiresAt() const;

private:
    std::string node_id_;
    ReplicationConfig config_;
    std::shared_ptr<WALManager> wal_;
    
    std::atomic<ReplicationRole> role_{ReplicationRole::FOLLOWER};
    std::atomic<uint64_t> current_term_{0};
    std::string voted_for_;
    std::string current_leader_;
    std::atomic<uint32_t> votes_received_{0};    // Votes gathered in current election
    std::atomic<uint32_t> cluster_size_{1};      // Total known cluster size (set externally)
    std::atomic<uint64_t> commit_index_{0};      // Highest WAL sequence known committed by leader quorum
    std::chrono::steady_clock::time_point last_heartbeat_time_;

    // Leader lease expiry time; epoch when no lease is held.
    mutable std::shared_mutex lease_mutex_;
    std::chrono::steady_clock::time_point lease_expires_at_;
    
    std::mutex election_mutex_;
    std::condition_variable election_cv_;
    std::thread election_thread_;
    std::atomic<bool> running_{false};
    
    void electionLoop();
    void becomeLeader();
    void becomeFollower(uint64_t term, const std::string& leader_id);
};

/**
 * Replication Stream for streaming WAL entries to followers
 */
class ReplicationStream {
public:
    ReplicationStream(
        const std::string& follower_endpoint,
        std::shared_ptr<WALManager> wal,
        const ReplicationConfig& config
    );
    ~ReplicationStream();
    
    // Start streaming
    void start();
    
    // Stop streaming
    void stop();
    
    // Get follower info
    const ReplicaInfo& getFollowerInfo() const { return follower_info_; }
    
    // Get last acknowledged sequence
    uint64_t getLastAckedSequence() const { return last_acked_sequence_.load(); }
    
    // Check if stream is healthy
    bool isHealthy() const;
    
    // Get current consecutive send failure count
    uint32_t getConsecutiveFailures() const { return consecutive_failures_.load(); }

private:
    std::string follower_endpoint_;
    std::shared_ptr<WALManager> wal_;
    ReplicationConfig config_;
    ReplicaInfo follower_info_;
    
    std::atomic<uint64_t> last_acked_sequence_{0};
    std::atomic<bool> running_{false};
    std::thread stream_thread_;
    mutable std::mutex wait_mutex_;
    std::condition_variable wait_cv_;
    
    // Retry / backoff state
    std::atomic<uint32_t> consecutive_failures_{0};
    static constexpr uint32_t kMaxRetries = 3;
    static constexpr uint32_t kBaseBackoffMs = 100;
    static constexpr uint32_t kMaxBackoffMs = 5000;

    // Compressed WAL transport (Zstd/LZ4/Snappy, configured via ReplicationConfig)
    std::unique_ptr<CompressedReplicationStream> compressed_stream_;

    void streamLoop();
    bool sendBatch(const std::vector<WALEntry>& entries);
    uint32_t computeBackoffMs() const;
};

/**
 * Main Replication Manager
 */
class ReplicationManager {
public:
    /**
     * Construct a ReplicationManager with the given configuration.
     * 
     * @param config Configuration parameters for replication behavior
     *        (mode, failover settings, WAL, conflict resolution, etc.)
     * 
     * @post Object is not yet active; call initialize() to start replication threads.
     */
    explicit ReplicationManager(const ReplicationConfig& config);
    
    ~ReplicationManager();
    
    /**
     * Initialize replication subsystem and start background threads.
     *
     * Initializes WAL, replicas, leader election state, and background replication
     * threads. If already initialized, returns true immediately (idempotent).
     *
     * @return true on success or if already initialized; false if configuration
     *         validation (validateConfig()) fails.
     *
     * @post On success, background replication threads are running and this node
     *       has joined the configured replica group.
     */
    bool initialize();
    
    /**
     * Shutdown replication subsystem and wait for background threads to exit.
     *
     * Gracefully stops replication threads, flushes any pending WAL entries,
     * and releases all replica connections. Safe to call from any thread.
     *
     * @note Blocking; may take up to heartbeat_interval_ms + election_timeout_max_ms
     *       to complete in worst case (if leader election is in progress).
     */
    void shutdown();
    
    /**
     * Replicate a write operation to all configured replicas.
     *
     * Appends the WAL entry to this node's Write-Ahead Log and schedules
     * replication to followers according to the configured replication mode
     * (SYNC, SEMI_SYNC, or ASYNC).
     *
     * @param entry WAL entry containing operation, document, data, and checksum.
     * @return true if entry was appended to WAL; false if this node is not
     *         the leader, WAL append failed, or replication is not initialized.
     *
     * @note In ASYNC mode, returns true immediately without waiting for replicas.
     *       In SEMI_SYNC mode, waits for min_sync_replicas to acknowledge.
     *       In SYNC mode, waits for all voting replicas.
     */
    bool replicate(const WALEntry& entry);
    
    /**
     * Wait for a specific WAL entry to be replicated to a sufficient number of replicas.
     *
     * Blocks until either:
     *   - The entry (identified by sequence number) has been acknowledged by
     *     enough replicas to satisfy the configured replication mode, or
     *   - timeout_ms milliseconds have elapsed (0 = no timeout).
     *
     * @param sequence WAL sequence number to wait for.
     * @param timeout_ms Maximum time to wait in milliseconds; 0 means use
     *        the configured replication_timeout_ms from ReplicationConfig.
     * @return true if replication completed within timeout; false on timeout or error.
     *
     * @note Unused in ASYNC mode (returns immediately).
     */
    bool waitForReplication(uint64_t sequence, uint32_t timeout_ms = 0);
    
    /**
     * Get the current replication role of this node.
     *
     * @return ReplicationRole::LEADER if this node is the leader; otherwise
     *         FOLLOWER, CANDIDATE, OBSERVER, or WITNESS.
     */
    ReplicationRole getRole() const;
    
    /**
     * Get the network endpoint of the current leader.
     *
     * @return Empty string if this node is the leader; otherwise the
     *         "hostname:port" endpoint of the current leader.
     */
    std::string getLeaderEndpoint() const;
    
    /**
     * Get information about all configured replicas.
     *
     * @return Vector of ReplicaInfo structs for all known replicas,
     *         including role, health status, and replication lag.
     */
    std::vector<ReplicaInfo> getReplicas() const;
    
    /**
     * Get replication runtime statistics.
     *
     * @return Const reference to the current ReplicationStats (entries replicated,
     *         failures, latencies, etc.).
     */
    const ReplicationStats& getStats() const { return stats_; }
    
    /**
     * Add a replica to the replication group.
     *
     * The new replica will receive a full snapshot followed by incremental WAL
     * entries. Adding a replica during active write traffic may incur lag
     * until the replica catches up.
     *
     * @param replica Replica information (node_id and endpoint must be non-empty).
     * @throws std::invalid_argument if node_id or endpoint is empty.
     * @note Rejects empty node_id or endpoint fail-closed to prevent silent
     *       replica registration failures.
     */
    void addReplica(const ReplicaInfo& replica);
    
    /**
     * Remove a replica from the replication group.
     *
     * Existing WAL entries are not deleted; the replica simply stops
     * receiving new replication updates.
     *
     * @param node_id Unique identifier of the replica to remove.
     * @note If the removed replica is the current leader, failover is triggered.
     */
    void removeReplica(const std::string& node_id);

    /**
     * Add a witness node to the cluster.
     *
     * A witness node participates in leader-election voting (and therefore
     * contributes to quorum) but does NOT receive WAL data.  This allows a
     * 2-node data cluster to maintain quorum without requiring a third full
     * data replica.
     *
     * @param node_id   Unique identifier for the witness node.
     * @param endpoint  Network address (hostname:port) of the witness node.
     */
    void addWitnessNode(const std::string& node_id, const std::string& endpoint);
    
    /**
     * Set a custom conflict resolver for multi-master replication.
     *
     * The resolver will be called whenever two or more writes conflict on the
     * same document. The resolver must be thread-safe and idempotent.
     *
     * @param resolver Implementation of IConflictResolver; if nullptr, the
     *                 default Last-Write-Wins strategy is used.
     *
     * @note This is only meaningful in multi-master (CRDT) replication mode.
     *       In leader-follower mode, write ordering prevents most conflicts.
     */
    void setConflictResolver(std::shared_ptr<IConflictResolver> resolver);
    
    /**
     * Register a listener for replication lifecycle events.
     *
     * The listener will be called on the replication background thread for
     * each event (apply, failover, conflict, lag, health change, etc.).
     *
     * @param listener Shared pointer to an IReplicationListener implementation.
     * @note Listeners must not block for more than 1 ms or spawn I/O.
     * @note Multiple listeners can be registered; all are called for each event.
     */
    void addListener(std::shared_ptr<IReplicationListener> listener);
    
    /**
     * Initiate a manual failover to a specific target replica.
     *
     * Attempts to promote the target replica to leader and demote this node
     * (or the current leader) to follower. Useful for maintenance or
     * deliberate topology changes.
     *
     * @param target_node_id Node ID of the replica to promote.
     * @return true on success; false if target replica is unreachable or
     *         unable to assume leadership.
     *
     * @note Blocking; may take up to election_timeout_max_ms to complete.
     * @note If this node is not the current leader, the call is relayed to
     *       the leader for execution.
     */
    bool triggerFailover(const std::string& target_node_id);
    
    /**
     * Promote this follower node to leader (for planned maintenance or
     * deliberate topology change).
     *
     * @return true on success; false if this node is already the leader,
     *         unable to communicate with other replicas, or the quorum
     *         is not achieved.
     * @note Blocking; may take up to election_timeout_max_ms to complete.
     */
    bool promoteToLeader();
    
    /**
     * Demote this leader node to follower (used during planned maintenance).
     *
     * Stops accepting new writes and voluntarily steps down from leadership.
     * Another replica will be elected as the new leader.
     *
     * @return true on success; false if this node is not the leader or
     *         if the new leader cannot be elected.
     */
    bool demoteToFollower();
    
    /**
     * Enable multi-region replication
     * @param region_id: Identifier for this region
     * @param peer_regions: List of peer region endpoints
     * @return true on success
     */
    bool enableMultiRegion(const std::string& region_id,
                          const std::vector<std::string>& peer_regions);
    
    /**
     * Promote a read replica to primary
     * @param replica_id: Node ID of replica to promote
     * @return true on success
     */
    bool promoteReplica(const std::string& replica_id);
    
    /**
     * Setup cascading replication (replica replicating to other replicas)
     * @param source_replica: Source replica node ID
     * @param target_replicas: Target replica node IDs
     * @return true on success
     */
    bool setupCascadingReplication(const std::string& source_replica,
                                   const std::vector<std::string>& target_replicas);
    
    /**
     * Get replication lag for specific replica
     * @param replica_id: Node ID of replica
     * @return Lag in milliseconds
     */
    int64_t getReplicationLag(const std::string& replica_id) const;
    
    /**
     * Check cluster health
     * @return Health status map (node_id -> is_healthy)
     */
    std::map<std::string, bool> getClusterHealth() const;
    
    /**
     * Export metrics in Prometheus format
     * @return Prometheus-formatted metrics string
     */
    std::string exportPrometheusMetrics() const;
    /**
     * Get health status of all replicas.
     *
     * @return Vector of (node_id, HealthStatus) pairs for all replicas,
     *         indicating whether each is HEALTHY, DEGRADED, FAILED, or UNKNOWN.
     * @see HealthStatus enum for interpretation of each status.
     */
    std::vector<std::pair<std::string, HealthStatus>> getReplicaHealthStatus() const;
    
    /**
     * Check if the cluster currently has quorum for write operations.
     *
     * Quorum is defined as: for a cluster with N voting members, at least
     * ceil(N/2)+1 members must be responsive (HEALTHY or DEGRADED status).
     *
     * @return true if quorum is achieved; false if partition or insufficient
     *         healthy replicas.
     * @note This check is fast (O(1)) as quorum state is maintained continuously.
     */
    bool hasQuorum() const;
    
    /**
     * Trigger a health check on all configured replicas.
     *
     * Sends heartbeat/ping messages to all replicas and updates their health
     * status based on responses. Called automatically at heartbeat_interval_ms
     * but can also be called explicitly to force immediate health assessment.
     *
     * @note Non-blocking; results are available via getReplicaHealthStatus()
     *       after heartbeat_interval_ms or on next check call.
     */
    void performHealthCheck();
    
    /**
     * Detect if there is a network partition in the cluster.
     *
     * A partition is detected when:
     *   - This node and some replicas are unable to communicate, and
     *   - Neither partition can achieve quorum, or
     *   - Quorum has been lost.
     *
     * @return true if a partition is detected; false if cluster is connected
     *         or this node is isolated but retains quorum.
     *
     * @note In leader mode: leader remains available for writes if it has quorum.
     * @note In follower mode: no writes allowed; read traffic routed to healthy
     *       replicas or primary.
     */
    bool detectNetworkPartition() const;
    
    /**
     * Get the read preference configuration for query routing.
     *
     * @return Configured ReadPreference (PRIMARY, SECONDARY, PRIMARY_PREFERRED, etc.).
     * @see setReadPreference() to change the default.
     */
    ReadPreference getReadPreference() const { return config_.default_read_preference; }
    
    /**
     * Set the read preference for query routing.
     *
     * @param preference New read preference strategy.
     *
     * @note Changes take effect immediately for new read requests.
     *       In-flight reads are not affected.
     */
    void setReadPreference(ReadPreference preference);

    /**
     * Select the best node for a read request using automated lag-based
     * traffic shifting.
     *
     * Replicas whose replication lag exceeds
     * `config_.max_replication_lag_ms` are excluded from read routing;
     * if none are eligible the primary node is returned.
     *
     * @param preference Override read preference (uses configured default
     *                   when not provided).
     * @return RoutingDecision describing which node was chosen and why.
     */
    LagBasedReadRouter::RoutingDecision selectReadReplica(
        std::optional<ReadPreference> preference = std::nullopt) const;

    // -----------------------------------------------------------------------
    // Raft leader lease reads for linearizable read-scale-out
    // -----------------------------------------------------------------------

    /**
     * Result returned by leaseRead().
     */
    struct LeaseReadResult {
        bool  success = false;         ///< true when the read could be served
        bool  served_under_lease = false; ///< true when linearizable via lease
        std::string node_id;           ///< node that served the read
        uint64_t    commit_index = 0;  ///< WAL sequence visible at read time
    };

    /**
     * Attempt a linearizable read via the leader lease mechanism.
     *
     * If this node is the current Raft leader *and* its lease is still valid,
     * the read is served locally with linearizability guarantees (no quorum
     * round-trip required).  Otherwise the call returns `success=false` and
     * the caller should redirect to the primary or use a quorum read.
     *
     * @param collection  Collection name (informational; not used for storage
     *                    lookup within this module).
     * @param document_id Document identifier (informational).
     * @return LeaseReadResult describing whether the read was served and how.
     */
    LeaseReadResult leaseRead(const std::string& collection,
                              const std::string& document_id) const;

    /**
     * Returns true when this node is the leader AND its leader lease is
     * currently valid.  Can be used by routing layers to decide whether to
     * serve a read locally.
     */
    bool hasLeaderLease() const;

private:
    ReplicationConfig config_;
    std::string node_id_;
    
    std::shared_ptr<WALManager> wal_;
    std::unique_ptr<LeaderElection> election_;
    
    std::vector<std::unique_ptr<ReplicationStream>> streams_;
    mutable std::shared_mutex replicas_mutex_;  // Protects replicas_ and streams_
    std::vector<ReplicaInfo> replicas_;
    
    std::shared_ptr<IConflictResolver> conflict_resolver_;
    std::vector<std::shared_ptr<IReplicationListener>> listeners_;
    
    ReplicationStats stats_;
    
    std::mutex manager_mutex_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    
    // Background threads
    std::thread heartbeat_thread_;
    std::thread compaction_thread_;
    std::thread health_monitor_thread_;
    
    bool validateConfig();
    void heartbeatLoop();
    void compactionLoop();
    void healthMonitorLoop();
    void notifyListeners(std::function<void(IReplicationListener&)> callback);
    void attemptAutomaticFailover(const std::string& failed_node_id);
    bool electNewLeader();
    void updateReplicaHealth(ReplicaInfo& replica);
};

// ============================================================================
// Parallel Replication Worker (v1.6.0)
// ============================================================================

/**
 * ParallelReplicationWorker
 *
 * Applies WAL entries on a follower using multiple worker threads while
 * maintaining causal ordering via a dependency graph keyed on document_id.
 * Independent writes (different document_ids) execute concurrently;
 * writes to the same document are serialized.
 */
class ParallelReplicationWorker {
public:
    struct ParallelConfig {
        uint32_t worker_threads      = 4;
        uint32_t queue_size          = 10000;
        bool use_dependency_tracking = true;
        bool group_transactions      = true;  // Drain multiple entries per worker iteration
    };

    struct Stats {
        uint64_t entries_applied;
        uint64_t dependencies_detected;
        uint64_t average_latency_us;  // Average submit-to-apply latency in microseconds
        uint64_t parallel_batches;
        double   parallelism_factor;  // average concurrent entries per batch
    };

    explicit ParallelReplicationWorker(const ParallelConfig& config);
    ~ParallelReplicationWorker();

    // Submit a WAL entry for parallel application (non-blocking)
    void submit(const WALEntry& entry);

    // Block until all previously submitted entries have been applied
    void sync();

    Stats getStats() const;

private:
    ParallelConfig config_;

    // Per-document serialization: tracks the done-flag of the most recently
    // submitted write per document_id so the next write can depend on it.
    std::map<std::string, std::shared_ptr<std::atomic<bool>>> last_done_per_doc_;
    mutable std::mutex dep_mutex_;

    // Work queue
    struct WorkItem {
        WALEntry entry;
        std::shared_ptr<std::atomic<bool>> ready;  // Set to true when this entry is done
        std::vector<std::shared_ptr<std::atomic<bool>>> deps;
        std::chrono::steady_clock::time_point submit_time;
    };

    std::queue<WorkItem> work_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    // Workers
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};

    // In-flight counter: incremented in submit(), decremented in workerLoop()
    std::atomic<uint64_t> in_flight_count_{0};

    // Stats counters
    std::atomic<uint64_t> stats_entries_applied_{0};
    std::atomic<uint64_t> stats_deps_detected_{0};
    std::atomic<uint64_t> stats_batches_{0};
    std::atomic<uint64_t> stats_total_latency_us_{0};  // Sum of per-entry latencies

    void workerLoop();
};

// ============================================================================
// Quorum Read Manager (v1.6.0)
// ============================================================================

/**
 * QuorumReadManager
 *
 * Issues a read to multiple replicas concurrently, waits for at least
 * `read_quorum` responses, reconciles any divergence using the latest
 * sequence number, and optionally triggers read-repair.
 */
class QuorumReadManager {
public:
    struct QuorumReadConfig {
        uint32_t read_quorum          = 2;
        uint32_t read_timeout_ms      = 1000;
        bool     repair_on_read       = true;
        uint32_t session_token_ttl_ms = 30000;  ///< TTL for session tokens (ms)
    };

    struct QuorumReadResult {
        bool        success;
        std::string data;
        uint64_t    version;
        bool        had_conflicts;
        std::vector<std::string> sources;  // replica endpoints that responded
        std::string session_token;         ///< Opaque token for session consistency
    };

    explicit QuorumReadManager(
        const QuorumReadConfig& config,
        const std::vector<ReplicaInfo>& replicas
    );

    QuorumReadResult read(
        const std::string& collection,
        const std::string& document_id,
        uint32_t quorum = 0,                    // 0 = use config default
        const std::string& session_token = ""   // opaque token for session consistency
    );

    // Update the replica list (called when topology changes)
    void setReplicas(const std::vector<ReplicaInfo>& replicas);

    /// Callback type for fetching a document from a specific replica.
    /// Arguments: endpoint, collection, document_id.  Returns the serialised
    /// document data, or an empty string when the replica does not hold the
    /// document or the fetch fails.
    using DocumentFetchFn = std::function<
        std::string(const std::string& /*endpoint*/,
                    const std::string& /*collection*/,
                    const std::string& /*document_id*/)>;

    /// Inject a data-fetch function so that queryReplica() can return real
    /// document content.  The storage / RPC layer sets this at startup; tests
    /// inject a local-memory lookup.  Without a callback the data field of
    /// every ReplicaResponse remains empty (original behaviour).
    void setDocumentFetchCallback(DocumentFetchFn fn);

    /// Callback type for fetching a document from the local storage engine
    /// when no replicas are configured (single-node deployment).
    /// Arguments: collection, document_id.
    /// Returns: {serialised document data, version}.  An empty data string is
    /// valid when the document does not exist; version 0 means unknown.
    ///
    /// Injection API for the single-node local read path.
    /// Provides concrete document content/version when no replica topology is present.
    using LocalDocumentFetchFn = std::function<
        std::pair<std::string, uint64_t>(const std::string& /*collection*/,
                                         const std::string& /*document_id*/)>;

    /// Inject a local-storage read function used by read() when the replica
    /// list is empty (single-node deployments).  Without a callback the
    /// data field remains empty and version=0 (original behaviour).
    void setLocalDocumentFetchFn(LocalDocumentFetchFn fn);

private:
    QuorumReadConfig config_;
    std::vector<ReplicaInfo> replicas_;
    mutable std::shared_mutex replicas_mutex_;
    DocumentFetchFn      doc_fetch_fn_;         ///< Optional RPC / storage data fetcher
    LocalDocumentFetchFn local_doc_fetch_fn_;   ///< Optional local-storage read for single-node mode

    // Per-replica read response
    struct ReplicaResponse {
        bool        ok;
        std::string data;
        uint64_t    version;
        std::string endpoint;
    };

    ReplicaResponse queryReplica(
        const ReplicaInfo& replica,
        const std::string& collection,
        const std::string& document_id
    ) const;

    /// Generate an opaque session token encoding @p version and an expiry timestamp.
    std::string generateSessionToken(uint64_t version) const;

    /// Parse @p token and return the embedded version (0 on error or expiry).
    uint64_t parseSessionToken(const std::string& token) const;
};

// ============================================================================
// Persistent Replication State (v1.6.0)
// ============================================================================

/**
 * PersistentReplicationState
 *
 * Persists the follower's last-applied sequence number (and other state)
 * to a local file so that on restart the follower can resume replication
 * from the correct position instead of replaying the entire WAL.
 */
class PersistentReplicationState {
public:
    struct State {
        uint64_t    last_applied_sequence = 0;
        uint64_t    current_term          = 0;
        std::string voted_for;
        std::string leader_id;
        std::chrono::system_clock::time_point persisted_at;
    };

    explicit PersistentReplicationState(const std::string& state_file_path);

    // Persist current state to disk (fsync)
    bool persist(const State& state);

    // Load state from disk; returns default-constructed State on first run
    State load() const;

    // Check whether a persisted state file exists
    bool exists() const;

    // Delete the state file (e.g., on clean shutdown or reset)
    void remove();

private:
    std::string path_;
    mutable std::mutex file_mutex_;
};

// ============================================================================
// Compressed Replication Stream (v1.6.0)
// ============================================================================

/**
 * CompressedReplicationStream
 *
 * Wraps a replication batch before sending and decompresses on receive.
 * Supported algorithms: NONE, LZ4 (fast), ZSTD (best ratio), SNAPPY (fastest).
 * AUTO selects ZSTD for batches >= min_batch_size, otherwise NONE.
 */
class CompressedReplicationStream {
public:
    enum class CompressionAlgorithm {
        NONE,
        LZ4,     // Fast, moderate compression
        ZSTD,    // Best compression ratio
        SNAPPY,  // Very fast, low compression
        AUTO     // Select based on batch size & data
    };

    struct CompressionConfig {
        CompressionAlgorithm algorithm    = CompressionAlgorithm::AUTO;
        int      compression_level        = 3;   // 1-9 (Zstd); ignored by LZ4/Snappy
        bool     adaptive                 = true;
        uint32_t min_batch_size           = 1024; // bytes; skip compression below this
    };

    struct CompressionStats {
        uint64_t    bytes_uncompressed = 0;
        uint64_t    bytes_compressed   = 0;
        double      compression_ratio  = 1.0;
        std::string algorithm_used;
    };

    CompressedReplicationStream(
        const std::string& endpoint,
        const CompressionConfig& config
    );

    // Construct with default compression config
    explicit CompressedReplicationStream(const std::string& endpoint);
    // Compress entries and (in production) send over network.
    // Returns true on success; false on compression error.
    bool sendBatch(const std::vector<WALEntry>& entries);

    // Decompress a byte buffer received from the network.
    // Returns the decompressed bytes on success or an empty vector on error.
    std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed,
                                    CompressionAlgorithm algo) const;

    CompressionStats getStats() const;

    // Reset accumulated statistics
    void resetStats();

private:
    std::string       endpoint_;
    CompressionConfig config_;

    mutable std::mutex stats_mutex_;
    CompressionStats   stats_;

    // Serialize WAL entries to a flat byte buffer
    std::vector<uint8_t> serializeEntries(const std::vector<WALEntry>& entries) const;

    // Compress a byte buffer using the configured algorithm
    std::vector<uint8_t> compress(const std::vector<uint8_t>& data,
                                   CompressionAlgorithm algo) const;

    // Select algorithm for a given payload size (used in AUTO mode)
    CompressionAlgorithm selectAlgorithm(size_t payload_bytes) const;

    static std::string algorithmName(CompressionAlgorithm algo);
};

// ============================================================================
// Batched Acknowledgment Tracker (v1.6.0)
// ============================================================================

/**
 * BatchedAckTracker
 *
 * Accumulates acknowledgment sequence numbers from followers and flushes
 * them in a single batch message to reduce network round-trips.
 *
 * Usage (on the follower side):
 *   tracker.recordApplied(entry.sequence_number);
 *   // Background flush thread calls flush() periodically or on batch fill.
 *
 * The leader side dequeues batched ACK payloads via dequeuePendingAcks().
 */
class BatchedAckTracker {
public:
    struct AckBatchConfig {
        uint32_t max_batch_size    = 100;     // Flush when >= this many ACKs queued
        uint32_t flush_interval_ms = 50;      // Flush at least every N ms
    };

    struct AckBatch {
        std::vector<uint64_t> sequences;      // Sequence numbers being ACK'd
        std::chrono::system_clock::time_point created_at;
    };

    explicit BatchedAckTracker();
    explicit BatchedAckTracker(const AckBatchConfig& config);
    ~BatchedAckTracker();

    // Called by the follower when a WAL entry has been applied
    void recordApplied(uint64_t sequence_number);

    // Dequeue the next pending ACK batch (called by the network sender)
    // Returns nullopt when no batch is ready
    std::optional<AckBatch> dequeuePendingAcks();

    // Force an immediate flush of whatever is buffered (called on shutdown)
    void forceFlush();

    // Get the highest sequence number ACK'd so far
    uint64_t getHighestAcked() const { return highest_acked_.load(); }

    struct Stats {
        uint64_t total_acks_sent;
        uint64_t total_batches_sent;
        double   avg_batch_size;
    };
    Stats getStats() const;

private:
    AckBatchConfig config_;

    std::vector<uint64_t> pending_;        // ACKs not yet flushed
    mutable std::mutex    pending_mutex_;
    std::condition_variable flush_cv_;

    std::queue<AckBatch>  ready_batches_;  // Batches ready for network send
    mutable std::mutex    ready_mutex_;

    std::atomic<uint64_t> highest_acked_{0};
    std::atomic<bool>     running_{false};
    std::thread           flush_thread_;

    // Stats
    std::atomic<uint64_t> stats_total_acks_{0};
    std::atomic<uint64_t> stats_total_batches_{0};

    void flushLoop();
    void flushPending();  // Called with pending_mutex_ held
};

// ============================================================================
// Replication Analytics (v1.6.0)
// ============================================================================

/**
 * ReplicationAnalytics
 *
 * Tracks per-replica replication lag over time and surfaces insights
 * (LAG_SPIKE, SLOW_REPLICA, NETWORK_ISSUE) with actionable recommendations.
 * Also performs simple bottleneck classification.
 */
class ReplicationAnalytics {
public:
    struct Insight {
        std::string type;            // "LAG_SPIKE" | "SLOW_REPLICA" | "NETWORK_ISSUE"
        std::string description;
        std::string recommendation;
        std::chrono::system_clock::time_point detected_at;
        std::map<std::string, std::string> metadata;
    };

    struct LagDataPoint {
        std::chrono::system_clock::time_point timestamp;
        int64_t lag_ms;
    };

    struct LagHistory {
        std::vector<LagDataPoint> data_points;
        int64_t avg_lag_ms  = 0;
        int64_t p95_lag_ms  = 0;
        int64_t p99_lag_ms  = 0;
        int64_t max_lag_ms  = 0;
    };

    struct Bottleneck {
        std::string replica_id;
        std::string bottleneck_type;  // "NETWORK" | "DISK_IO" | "CPU"
        double      severity;         // 0.0 – 1.0
        std::string details;
    };

    ReplicationAnalytics();

    // Record a lag observation for a replica (call periodically)
    void recordLag(const std::string& replica_id, int64_t lag_ms);

    // Get current insights (refreshed on each call)
    std::vector<Insight> getInsights() const;

    // Get lag history for a replica over the last `duration`
    LagHistory getLagHistory(const std::string& replica_id,
                              std::chrono::hours duration) const;

    // Detect bottlenecks across all replicas
    std::vector<Bottleneck> detectBottlenecks() const;

    // Export summary to Prometheus text format
    std::string exportPrometheusMetrics() const;

    // Configuration
    struct AnalyticsConfig {
        int64_t  lag_spike_threshold_ms  = 5000;  // lag > this triggers LAG_SPIKE
        int64_t  slow_replica_avg_ms     = 2000;  // avg lag > this = SLOW_REPLICA
        size_t   max_history_per_replica = 10000; // rolling window
    };
    void setConfig(const AnalyticsConfig& config);

private:
    AnalyticsConfig config_;
    mutable std::shared_mutex data_mutex_;

    // Per-replica rolling lag history
    std::map<std::string, std::deque<LagDataPoint>> lag_history_;

    // Compute percentile from a sorted vector
    static int64_t percentile(const std::vector<int64_t>& sorted, double p);
};

// ============================================================================
// Replication Benchmark (v1.6.0)
// ============================================================================

/**
 * ReplicationBenchmark
 *
 * Measures replication throughput and latency by submitting synthetic
 * WAL entries through the given WALManager and recording timings.
 * Reports writes/sec and p50/p95/p99 latency percentiles.
 */
class ReplicationBenchmark {
public:
    struct BenchmarkConfig {
        uint32_t num_entries         = 10000;
        uint32_t entry_size_bytes    = 256;
        uint32_t warmup_entries      = 500;
        std::string collection       = "benchmark";
    };

    struct BenchmarkResult {
        uint32_t  total_entries;
        double    duration_seconds;
        double    writes_per_second;
        int64_t   latency_p50_us;
        int64_t   latency_p95_us;
        int64_t   latency_p99_us;
        int64_t   latency_max_us;
        uint64_t  bytes_written;
    };

    explicit ReplicationBenchmark(std::shared_ptr<WALManager> wal,
                                   const BenchmarkConfig& config);
    explicit ReplicationBenchmark(std::shared_ptr<WALManager> wal);

    // Run the benchmark; blocks until complete
    BenchmarkResult run();

    // Format result as human-readable string
    static std::string format(const BenchmarkResult& result);

private:
    std::shared_ptr<WALManager> wal_;
    BenchmarkConfig config_;
};

// ============================================================================
// Change Data Capture (CDC) – v1.6.0
// ============================================================================

/**
 * CDCManager
 *
 * Captures every WAL entry that passes through the replication pipeline and
 * delivers it to registered consumer callbacks.  Consumers can filter by
 * collection name or receive every change.
 *
 * Usage:
 *   auto cdc = std::make_shared<CDCManager>();
 *   cdc->subscribe("users", [](const WALEntry& e){ ... });
 *   repl_mgr.addListener(cdc);
 */
class CDCManager : public IReplicationListener {
public:
    // Callback signature: (WALEntry) -> void
    using CDCCallback = std::function<void(const WALEntry&)>;

    CDCManager() = default;

    // Subscribe to all collections ("" = wildcard for every collection)
    uint64_t subscribe(const std::string& collection, CDCCallback callback);

    // Unsubscribe a previously registered handler
    void unsubscribe(uint64_t subscription_id);

    // Number of active subscriptions
    size_t subscriptionCount() const;

    // -------------------------------------------------------------------
    // IReplicationListener overrides (no-ops except onWALEntryApplied)
    // -------------------------------------------------------------------
    void onRoleChange(ReplicationRole, ReplicationRole) override {}
    void onLeaderElected(const std::string&) override {}
    void onReplicaAdded(const ReplicaInfo&) override {}
    void onReplicaRemoved(const std::string&) override {}
    void onConflictDetected(const std::string&) override {}
    void onReplicationLagWarning(int64_t) override {}
    void onReplicaHealthChanged(const std::string&, HealthStatus, HealthStatus) override {}
    void onFailoverStarted(const std::string&, const std::string&) override {}
    void onFailoverCompleted(const std::string&, bool) override {}
    void onNetworkPartitionDetected(const std::vector<std::string>&) override {}

    // Dispatches the entry to all matching subscribers
    void onWALEntryApplied(const WALEntry& entry) override;

private:
    struct Subscription {
        uint64_t    id;
        std::string collection;   // empty = match everything
        CDCCallback callback;
    };

    mutable std::shared_mutex subs_mutex_;
    std::vector<Subscription> subscriptions_;
    std::atomic<uint64_t>     next_id_{1};
};

// ============================================================================
// Cross-Cluster Publish/Subscribe Replication – v1.7.0
// ============================================================================

/**
 * PublicationFilter
 *
 * Specifies which WAL entries are forwarded to remote cluster subscribers.
 * An empty filter (default) matches every entry.
 */
struct PublicationFilter {
    std::vector<std::string> include_collections;  // empty = all collections
    std::vector<std::string> include_operations;   // empty = all operations

    // Returns true when `entry` satisfies all active filter criteria.
    bool matches(const WALEntry& entry) const;
};

/**
 * CrossClusterPublication
 *
 * Publishes WAL entries to remote cluster subscriptions.
 * Implements IReplicationListener so it can be registered directly with
 * ReplicationManager::addListener().  Every WAL entry that passes the
 * configured filter is forwarded to all registered remote subscribers.
 *
 * Usage:
 *   auto pub = std::make_shared<CrossClusterPublication>("orders_pub");
 *   pub->setFilter(filter);
 *   repl_mgr.addListener(pub);
 *   pub->addRemoteSubscriber([](const WALEntry& e){ remote.apply(e); });
 */
class CrossClusterPublication : public IReplicationListener {
public:
    using RemoteSubscriberCallback = std::function<void(const WALEntry&)>;

    explicit CrossClusterPublication(const std::string& name);

    // Publication name
    const std::string& name() const;

    // Set / get the publication filter (thread-safe)
    void setFilter(const PublicationFilter& filter);
    PublicationFilter getFilter() const;

    // Add a remote subscriber; returns an opaque subscriber ID
    uint64_t addRemoteSubscriber(RemoteSubscriberCallback callback);

    // Remove a remote subscriber by the ID returned from addRemoteSubscriber()
    void removeRemoteSubscriber(uint64_t subscriber_id);

    // Number of currently active remote subscribers
    size_t subscriberCount() const;

    // Total WAL entries that passed the filter and were delivered
    uint64_t publishedCount() const;

    // Apply filter and deliver `entry` to all remote subscribers
    void publish(const WALEntry& entry);

    // Export Prometheus text-format metrics
    std::string exportPrometheusMetrics() const;

    // -----------------------------------------------------------------------
    // IReplicationListener overrides
    // -----------------------------------------------------------------------
    void onWALEntryApplied(const WALEntry& entry) override;
    void onRoleChange(ReplicationRole, ReplicationRole) override {}
    void onLeaderElected(const std::string&) override {}
    void onReplicaAdded(const ReplicaInfo&) override {}
    void onReplicaRemoved(const std::string&) override {}
    void onConflictDetected(const std::string&) override {}
    void onReplicationLagWarning(int64_t) override {}
    void onReplicaHealthChanged(const std::string&, HealthStatus, HealthStatus) override {}
    void onFailoverStarted(const std::string&, const std::string&) override {}
    void onFailoverCompleted(const std::string&, bool) override {}
    void onNetworkPartitionDetected(const std::vector<std::string>&) override {}

private:
    struct RemoteSubscriber {
        uint64_t id;
        RemoteSubscriberCallback callback;
    };

    std::string name_;

    mutable std::shared_mutex filter_mutex_;
    PublicationFilter filter_;

    mutable std::shared_mutex subs_mutex_;
    std::vector<RemoteSubscriber> subscribers_;
    std::atomic<uint64_t> next_id_{1};
    std::atomic<uint64_t> published_count_{0};
};

/**
 * CrossClusterSubscription
 *
 * Subscribes to a CrossClusterPublication and delivers received WAL entries
 * to a local apply callback.  Tracks applied/error counts and the last
 * applied sequence number for monitoring.
 *
 * Usage:
 *   CrossClusterSubscription sub("orders_sub", pub,
 *       [](const WALEntry& e){ local_store.apply(e); });
 *   sub.enable();
 */
class CrossClusterSubscription {
public:
    using ApplyCallback = std::function<void(const WALEntry&)>;

    CrossClusterSubscription(const std::string& name,
                              std::shared_ptr<CrossClusterPublication> publication,
                              ApplyCallback on_apply);

    // Automatically unregisters from the publication on destruction
    ~CrossClusterSubscription();

    // Subscription name
    const std::string& name() const;

    // Register with the publication (idempotent)
    void enable();

    // Unregister from the publication (idempotent)
    void disable();

    // Whether the subscription is currently active
    bool isEnabled() const;

    // Count of entries successfully applied (no exception thrown)
    uint64_t appliedCount() const;

    // Highest sequence number successfully applied
    uint64_t lastAppliedSequence() const;

    // Count of apply-callback exceptions caught
    uint64_t errorCount() const;

    // Export Prometheus text-format metrics
    std::string exportPrometheusMetrics() const;

private:
    std::string name_;
    std::shared_ptr<CrossClusterPublication> publication_;
    ApplyCallback on_apply_;

    std::mutex enable_mutex_;
    std::atomic<bool> enabled_{false};
    uint64_t subscriber_id_{0};

    std::atomic<uint64_t> applied_count_{0};
    std::atomic<uint64_t> last_applied_seq_{0};
    std::atomic<uint64_t> error_count_{0};
};

// ============================================================================
// WAL Archival Manager – v1.6.0
// ============================================================================

/**
 * IArchivalBackend
 *
 * Pluggable backend interface for WAL segment storage.  The default
 * LocalArchivalBackend writes to the local filesystem; cloud backends
 * (S3, GCS, Azure Blob) implement this interface for object-storage targets.
 */
class IArchivalBackend {
public:
    virtual ~IArchivalBackend() = default;

    // Write a segment payload to the backend.  Returns true on success.
    [[nodiscard]] virtual bool putObject(const std::string& key,
                           const std::vector<uint8_t>& data) = 0;

    // Read a segment payload from the backend.  Returns nullopt on failure.
    [[nodiscard]] virtual std::optional<std::vector<uint8_t>> getObject(
        const std::string& key) const = 0;

    // Remove an object from the backend.
    [[nodiscard]] virtual bool deleteObject(const std::string& key) = 0;

    // Transition an object to a colder storage tier (e.g. "cold", "glacier").
    // A no-op on backends that do not support tiering.
    virtual void setStorageTier(const std::string& key,
                                const std::string& tier) = 0;
};

/**
 * WALArchivalManager
 *
 * Archives completed WAL segments to a local (or cloud-pluggable) destination
 * with optional compression and AES-256-GCM encryption at rest.
 * Provides retrieval for point-in-time recovery (PITR) and lifecycle
 * management that transitions segments across storage tiers (standard → cold →
 * glacier) based on configurable age thresholds.
 *
 * Cloud backends (S3, GCS, Azure) are pluggable via the `IArchivalBackend`
 * interface; the default backend writes to the local filesystem.
 */
class WALArchivalManager {
public:
    struct ArchivalConfig {
        // Source WAL directory
        std::string wal_directory;
        // Local archive destination (used by the default filesystem backend)
        std::string archive_directory;

        // Cloud object-storage target (ignored when storage_type == "local")
        std::string storage_type           = "local"; // "local", "s3", "gcs", "azure"
        std::string bucket_name;                      // Cloud bucket / container
        std::string prefix;                           // Object-key prefix (e.g. "cluster-1/wal/")

        // Archival policy
        uint32_t    archive_after_segments   = 100; // segments to accumulate before archiving
        uint32_t    local_retention_segments = 10;  // segments to keep locally after archive
        bool        compress_before_archive  = true;
        uint32_t    delete_after_days        = 365; // purge archived segments older than N days

        // Encryption at rest (AES-256-GCM)
        bool        encrypt_at_rest    = false;
        // 64-character hex string encoding a 32-byte AES-256 key.
        // Required when encrypt_at_rest == true.
        std::string encryption_key_hex;

        // Lifecycle management: transition segments to colder tiers.
        // 0 = lifecycle management disabled.
        uint32_t    transition_to_cold_after_days = 90;
    };

    struct ArchivedSegment {
        uint64_t    segment_id     = 0;
        uint64_t    start_sequence = 0;
        uint64_t    end_sequence   = 0;
        uint64_t    size_bytes     = 0;
        bool        compressed     = false;
        bool        encrypted      = false;
        std::chrono::system_clock::time_point archived_at;
        std::string archive_path;
        // Storage tier for lifecycle management: "standard", "cold", "glacier"
        std::string storage_tier   = "standard";
    };

    explicit WALArchivalManager(const ArchivalConfig& config,
                                std::shared_ptr<IArchivalBackend> backend = nullptr);

    // Archive the given WAL segment files (paths relative to wal_directory).
    // Returns number of segments successfully archived.
    uint32_t archiveSegments(const std::vector<std::string>& segment_paths);

    // Retrieve an archived segment by ID; returns the original raw bytes
    // (decrypted and decompressed as required).
    std::optional<std::vector<uint8_t>> retrieveSegment(uint64_t segment_id) const;

    // List all archived segments (sorted by segment_id ascending).
    std::vector<ArchivedSegment> listArchived() const;

    // Purge archived segments older than delete_after_days.
    uint32_t purgeExpired();

    // Apply lifecycle transitions: promote segments to colder storage tiers
    // based on their age relative to transition_to_cold_after_days.
    // Returns the number of segments whose tier was updated.
    uint32_t transitionStorageTiers();

    // Background archival: scan wal_directory, archive old segments, return count.
    uint32_t runArchivalCycle();

private:
    ArchivalConfig config_;
    std::shared_ptr<IArchivalBackend> backend_;  // nullptr = local filesystem
    mutable std::mutex archive_mutex_;
    std::vector<ArchivedSegment> index_;  // in-memory index; persisted via index.txt side-car

    // Returns the archive destination key/path for a segment_id.
    // When backend_ is set, returns the cloud object key (prefix + filename).
    // When backend_ is null, returns the local filesystem path.
    std::string archivePath(uint64_t segment_id) const;
    void saveIndex() const;
    void loadIndex();
    static std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    // AES-256-GCM encryption helpers. Format: IV(12) || Tag(16) || Ciphertext.
    static std::vector<uint8_t> encryptAesGcm(const std::vector<uint8_t>& data,
                                               const std::vector<uint8_t>& key);
    static std::optional<std::vector<uint8_t>> decryptAesGcm(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& key);
    static std::vector<uint8_t> hexToBytes(const std::string& hex);
};

// ============================================================================
// Multi-Region Active-Active with Bounded Staleness (Phase 4 – v1.8.0)
// ============================================================================

/**
 * ConsistencyLevel
 *
 * Defines the consistency guarantee for a read or write operation in a
 * multi-region active-active deployment.
 */
enum class ConsistencyLevel {
    STRONG,             ///< Linearizable: reads always reflect the latest committed write
    BOUNDED_STALENESS,  ///< Stale reads are permitted up to max_staleness_ms
    SESSION,            ///< Read-your-writes guarantee within the same session token
    EVENTUAL            ///< No consistency guarantee – maximum availability and lowest latency
};

/**
 * Per-region staleness tracking snapshot.
 */
struct RegionStalenessInfo {
    std::string region_id;
    int64_t     staleness_ms = 0;            ///< Estimated replication lag to this region (ms)
    uint64_t    last_applied_sequence = 0;   ///< Last WAL sequence known to be applied
    std::chrono::system_clock::time_point last_update;
    bool        is_healthy = true;
};

/**
 * Configuration for MultiRegionActiveActiveManager.
 *
 * ### Consistency Guarantees
 * - `STRONG` consistency is only guaranteed within a single region.
 * - Cross-region writes are always eventual unless `leader_region_id` is set
 *   and all STRONG writes are routed exclusively through the designated leader.
 * - Vector-clock-based conflict resolution (LAST_WRITE_WINS, MERGE) may produce
 *   different outcomes across regions for concurrent writes; this is expected
 *   in Active/Active deployments.
 *
 * @warning For SERIALIZABLE or STRONG guarantees on critical data, restrict
 *          writes to a single region.  Cross-region `STRONG` writes are
 *          rejected unless `local_region_id == leader_region_id`.
 */
struct MultiRegionActiveActiveConfig {
    std::string local_region_id;                         ///< Identifier for the local region
    std::vector<std::string> peer_region_ids;            ///< Peer region identifiers
    ConsistencyLevel default_consistency = ConsistencyLevel::BOUNDED_STALENESS;
    uint32_t max_staleness_ms = 5000;                    ///< Upper bound for BOUNDED_STALENESS reads (ms)
    uint32_t session_token_ttl_ms = 30000;               ///< Time-to-live for session tokens (ms)
    ConflictResolution conflict_strategy = ConflictResolution::LAST_WRITE_WINS;

    /**
     * Per-collection consistency overrides.
     *
     * When a collection is present in this map, the configured level takes
     * precedence over the caller-supplied level for both reads and writes.
     * Use this to enforce STRONG consistency for critical collections while
     * keeping the global default at BOUNDED_STALENESS or EVENTUAL.
     *
     * @note Configuring STRONG here for a non-leader region will cause all
     *       writes to that collection to be rejected unless the local region
     *       is the designated leader.
     */
    std::map<std::string, ConsistencyLevel> collection_consistency_overrides;

    /**
     * Designated leader region for STRONG cross-region writes.
     *
     * When non-empty, `write()` calls with effective consistency `STRONG` are
     * rejected if `local_region_id != leader_region_id`.  This prevents
     * split-brain lost-update scenarios for critical data by funnelling all
     * strongly-consistent writes through a single region.
     *
     * Leave empty to allow STRONG writes on any region (legacy behaviour;
     * provides no cross-region linearisability guarantee).
     */
    std::string leader_region_id;

    /**
     * Enable split-brain detection.
     *
     * When `true`, `isSplitBrain()` returns `true` if all peer regions report
     * unhealthy staleness, indicating a possible network partition.  Callers
     * may choose to reject STRONG writes or emit alerts in this state.
     */
    bool split_brain_detection_enabled = false;
};

/**
 * MultiRegionActiveActiveManager
 *
 * Coordinates multi-region active-active replication with bounded staleness
 * guarantees.  Every region can accept writes.  Reads are served according to
 * the requested ConsistencyLevel (or the collection-level override when
 * configured in `MultiRegionActiveActiveConfig::collection_consistency_overrides`):
 *
 *   STRONG            – Only served when the local replica is known to be
 *                       up-to-date (staleness_ms == 0).  Writes at STRONG
 *                       are rejected when a `leader_region_id` is configured
 *                       and the local region is not the leader.
 *   BOUNDED_STALENESS – Served when staleness_ms <= max_staleness_ms; otherwise
 *                       the read is rejected (caller should retry or fall back
 *                       to another region).
 *   SESSION           – Always served, but only if the region has applied at
 *                       least up to the sequence embedded in the session token
 *                       (read-your-writes guarantee).
 *   EVENTUAL          – Always served from the local region regardless of lag.
 *
 * ### Cross-Region Consistency Limitations
 * STRONG consistency is **only** guaranteed within a single region.  Cross-region
 * writes are always eventual in the absence of a leader-region fence.
 * Vector-clock-based conflict resolution may produce different outcomes on
 * concurrent writes; this is an inherent property of Active/Active deployments
 * (see CAP theorem and Google Spanner TrueTime for background).
 *
 * For critical data requiring SERIALIZABLE isolation, restrict all writes to a
 * single region or use `leader_region_id` to designate one region as the sole
 * acceptor of STRONG writes.
 *
 * Staleness information must be fed in from the underlying replication layer
 * via updateRegionStaleness().  In a real deployment this is called by the
 * ReplicationManager whenever a heartbeat or WAL acknowledgement arrives from
 * a remote region.
 */
class MultiRegionActiveActiveManager {
public:
    struct WriteResult {
        bool        success = false;
        std::string write_id;           ///< Unique write identifier
        std::string region_id;          ///< Region that accepted the write
        uint64_t    sequence_number = 0;
        std::string session_token;      ///< Updated session token (for SESSION consistency)
        bool        is_leader_region = false; ///< True when the write was accepted by the designated leader region
    };

    struct ReadResult {
        bool        success = false;
        std::string data;
        uint64_t    sequence_number = 0;
        std::string region_id;          ///< Region that served the read
        int64_t     staleness_ms = 0;   ///< Observed staleness at read time
        ConsistencyLevel served_at = ConsistencyLevel::EVENTUAL;
    };

    explicit MultiRegionActiveActiveManager(const MultiRegionActiveActiveConfig& config);

    /**
     * Record a write locally and return a WriteResult that includes a
     * session token embedding the new sequence number.
     *
     * If a collection-level override exists in
     * `MultiRegionActiveActiveConfig::collection_consistency_overrides`, it
     * replaces the caller-supplied `consistency` parameter.
     *
     * When `MultiRegionActiveActiveConfig::leader_region_id` is non-empty and
     * the effective consistency is `STRONG`, the write is rejected
     * (`success=false`) if the local region is not the designated leader.
     * This prevents split-brain lost-update scenarios for critical writes.
     *
     * @param collection  Collection (table) name.
     * @param document_id Unique document identifier within the collection.
     * @param operation   Operation type string (e.g. "INSERT", "UPDATE", "DELETE").
     * @param data        Serialized document payload.
     * @param consistency Requested consistency level (may be overridden per collection).
     * @param session_token Optional caller session token (currently unused by write).
     * @return WriteResult with `success=true` on acceptance, or `success=false` when
     *         rejected due to leader-region fencing.
     */
    WriteResult write(
        const std::string& collection,
        const std::string& document_id,
        const std::string& operation,
        const std::string& data,
        ConsistencyLevel consistency = ConsistencyLevel::SESSION,
        const std::string& session_token = ""
    );

    /**
     * Attempt a read at the requested consistency level.
     *
     * If a collection-level override exists in
     * `MultiRegionActiveActiveConfig::collection_consistency_overrides`, it
     * replaces the caller-supplied `consistency` parameter.
     *
     * Returns `success=false` when:
     *   - `STRONG`: local staleness > 0 (replica is not fully caught up)
     *   - `BOUNDED_STALENESS`: local staleness > `max_staleness_ms`
     *   - `SESSION`: the local replica has not yet applied the sequence in the token
     *
     * @param collection   Collection (table) name.
     * @param document_id  Unique document identifier.
     * @param consistency  Requested consistency level (may be overridden per collection).
     * @param session_token Optional session token for SESSION consistency (read-your-writes).
     * @return ReadResult with `success=true` when the consistency requirement is met.
     */
    ReadResult read(
        const std::string& collection,
        const std::string& document_id,
        ConsistencyLevel consistency = ConsistencyLevel::BOUNDED_STALENESS,
        const std::string& session_token = ""
    );

    /**
     * Create a new session token embedding the current local sequence.
     * The token is an opaque string that encodes the sequence number and an
     * expiry timestamp; it is intentionally human-readable for debuggability.
     */
    std::string createSessionToken() const;

    /**
     * Validate a session token and check whether the local replica has applied
     * at least required_sequence.  Returns false for malformed or expired tokens.
     */
    bool validateSessionToken(const std::string& token,
                              uint64_t required_sequence) const;

    /**
     * Return the current estimated staleness for a given region.
     * Returns max duration when the region is unknown.
     */
    std::chrono::milliseconds getStaleness(const std::string& region_id) const;

    /**
     * Returns true when the local region's staleness is within max_staleness_ms.
     */
    bool isWithinStalenessBound(const std::string& region_id) const;

    /**
     * Snapshot of staleness for every tracked region.
     */
    std::vector<RegionStalenessInfo> getAllRegionStaleness() const;

    /**
     * Called by the replication layer whenever new WAL progress is learned
     * for a remote region (e.g. on heartbeat or WAL ACK).
     */
    void updateRegionStaleness(const std::string& region_id,
                               int64_t staleness_ms,
                               uint64_t last_applied_sequence);

    /** Prometheus-format metrics snapshot. */
    std::string exportPrometheusMetrics() const;

    /**
     * Return the effective consistency level for a collection.
     *
     * If the collection has an entry in
     * `MultiRegionActiveActiveConfig::collection_consistency_overrides`, that
     * level is returned.  Otherwise `config_.default_consistency` is used.
     *
     * @param collection Collection (table) name to look up.
     * @return The effective ConsistencyLevel for that collection.
     */
    [[nodiscard]] ConsistencyLevel getEffectiveConsistency(
        const std::string& collection) const;

    /**
     * Return true when a split-brain condition is suspected.
     *
     * A split-brain is indicated when `split_brain_detection_enabled` is set
     * in the configuration **and** every configured peer region reports an
     * unhealthy staleness (i.e. no peer has been heard from within the
     * `max_staleness_ms * 2` window).  Returns false when detection is
     * disabled or when at least one peer is healthy.
     *
     * @note This is a heuristic based on the last staleness feed; a false
     *       negative is possible if staleness updates are delayed.
     */
    [[nodiscard]] bool isSplitBrain() const;

private:
    MultiRegionActiveActiveConfig config_;

    // Per-region staleness tracking
    mutable std::shared_mutex staleness_mutex_;
    std::map<std::string, RegionStalenessInfo> region_staleness_;

    // Monotonic write sequence counter for this region
    std::atomic<uint64_t> local_sequence_{0};

    // Counters
    std::atomic<uint64_t> writes_total_{0};
    std::atomic<uint64_t> reads_total_{0};
    std::atomic<uint64_t> staleness_rejections_{0};  ///< Reads rejected due to excessive lag
    std::atomic<uint64_t> strong_reads_{0};
    std::atomic<uint64_t> bounded_staleness_reads_{0};
    std::atomic<uint64_t> session_reads_{0};
    std::atomic<uint64_t> eventual_reads_{0};
    std::atomic<uint64_t> leader_write_rejections_{0}; ///< STRONG writes rejected because local is not the leader

    std::string generateWriteId(uint64_t sequence) const;
    std::string generateSessionToken(uint64_t sequence) const;
    uint64_t    parseSessionToken(const std::string& token) const;   ///< Returns 0 on error
};

// ============================================================================
// BidirectionalReplicationManager  (v1.7.0)
// ============================================================================

/**
 * BidirectionalReplicationManager
 *
 * Enables true bidirectional (active-active) replication between exactly two
 * peer nodes.  Both nodes accept writes and push changes to each other.
 *
 * Key capabilities:
 *  - Symmetric replication: every committed write is forwarded to the peer.
 *  - Conflict detection using HLC timestamps and monotonic sequence numbers.
 *  - Configurable conflict resolution per collection (LWW, FIRST_WRITE_WINS,
 *    VECTOR_CLOCK, or CUSTOM).
 *  - Origin tracking: each change is tagged with the node that originally
 *    created it, preventing replication loops (a change that originated from
 *    the peer is not forwarded back to the peer).
 *  - DDL replication with conflict detection (schema changes are sequenced and
 *    the later-arriving DDL wins by default, or a CUSTOM resolver is invoked).
 *
 * Lifecycle:
 *   BidirectionalReplicationManager mgr(config);
 *   mgr.start();
 *   // … application runs …
 *   mgr.stop();
 *
 * Thread safety: all public methods are thread-safe.
 */
class BidirectionalReplicationManager {
public:
    // ── Configuration ────────────────────────────────────────────────────────
    struct BidiConfig {
        std::string local_node_id;   ///< Identifier for this node (e.g. "us-west-1")
        std::string remote_node_id;  ///< Identifier for the peer node (e.g. "us-east-1")
        std::string remote_endpoint; ///< Network address of the peer (e.g. "host:port")

        // Conflict resolution
        ConflictResolution default_strategy = ConflictResolution::LAST_WRITE_WINS;
        std::map<std::string, ConflictResolution> collection_strategies; ///< Per-collection overrides

        // Origin tracking
        bool track_origin             = true;  ///< Tag every write with its origin node
        bool replicate_foreign_changes = false; ///< If false (default), suppress re-forwarding
                                                ///< changes whose origin is the remote peer

        // Synchronisation
        uint32_t sync_interval_ms = 1000; ///< How often to push pending changes (ms)
        bool bidirectional_sync   = true; ///< Enable the reverse (peer→local) replication path

        // DDL
        bool replicate_ddl = true; ///< Forward schema changes to the peer
    };

    // ── Data structures ───────────────────────────────────────────────────────

    /**
     * A single write event that participates in bidirectional replication.
     */
    struct BidiWriteEntry {
        std::string document_id;
        std::string collection;
        std::string operation;   ///< "INSERT" | "UPDATE" | "DELETE"
        std::string data;        ///< Serialised document payload
        std::string origin_node; ///< Node that first created this change
        uint64_t    origin_seq  = 0; ///< Monotonic sequence on the origin node
        int64_t     timestamp_ms = 0; ///< Wall-clock milliseconds (for LWW)
        bool        is_ddl       = false; ///< True for DDL (schema change) entries
    };

    /**
     * Conflict record produced when two concurrent writes target the same
     * document in the same collection.
     */
    struct BidiConflictRecord {
        std::string conflict_id;
        std::string document_id;
        std::string collection;
        BidiWriteEntry local_write;
        BidiWriteEntry remote_write;
        BidiWriteEntry resolved_write; ///< Winner after applying the strategy
        ConflictResolution strategy_used;
        std::chrono::system_clock::time_point detected_at;
        bool is_ddl_conflict = false;
    };

    /**
     * Synchronisation status snapshot.
     */
    struct SyncStatus {
        uint64_t local_sequence     = 0; ///< Latest sequence committed on the local node
        uint64_t remote_sequence    = 0; ///< Latest sequence acknowledged from the peer
        int64_t  lag_ms             = 0; ///< Estimated replication lag (ms)
        uint64_t conflicts_detected = 0; ///< Lifetime conflict count
        uint64_t conflicts_resolved = 0; ///< Conflicts resolved (including manual overrides)
        uint64_t conflicts_last_hour = 0; ///< Conflicts detected in the last 60 minutes (rolling)
        bool     is_synchronized    = false; ///< True when lag < sync_interval_ms and both nodes healthy
        bool     is_running         = false; ///< True while start() is active
    };

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    explicit BidirectionalReplicationManager(const BidiConfig& config);
    ~BidirectionalReplicationManager();

    // Non-copyable, non-movable
    BidirectionalReplicationManager(const BidirectionalReplicationManager&) = delete;
    BidirectionalReplicationManager& operator=(const BidirectionalReplicationManager&) = delete;

    /**
     * Activate bidirectional replication.
     * Returns true on success, false when the manager is already running or the
     * configuration is invalid (e.g. local_node_id == remote_node_id).
     */
    bool start();

    /**
     * Gracefully stop replication and release all resources.
     * Safe to call even if start() was never called.
     */
    void stop();

    // ── Write path ────────────────────────────────────────────────────────────

    /**
     * Submit a local write for bidirectional replication.
     *
     * If track_origin is enabled, the entry is tagged with local_node_id.
     * The write is enqueued for forwarding to the peer on the next sync cycle
     * (or immediately if sync_interval_ms == 0).
     *
     * Returns the assigned local sequence number.  Returns 0 when stop() has
     * been called.
     */
    uint64_t submitWrite(const std::string& document_id,
                         const std::string& collection,
                         const std::string& operation,
                         const std::string& data,
                         bool is_ddl = false);

    /**
     * Apply an incoming write that was received from the peer.
     *
     * Origin tracking: if the entry's origin_node equals remote_node_id and
     * replicate_foreign_changes is false (default), the change is applied
     * locally but NOT re-forwarded back to the peer, breaking the loop.
     *
     * Conflict detection: if a pending local write targets the same
     * (collection, document_id), handleConflict() is called to resolve it.
    *
    * Fail-closed invariants:
    * - When origin tracking is enabled, incoming writes must carry
    *   non-empty origin_node and origin_seq > 0.
    * - Stale/duplicate writes from the same origin (origin_seq <= last seen
    *   sequence for the same document) are rejected.
     *
     * Returns true when the entry was accepted and applied.
     */
    bool applyRemoteWrite(const BidiWriteEntry& entry);

    // ── Status & metrics ──────────────────────────────────────────────────────

    /** Current synchronisation status snapshot. */
    SyncStatus getSyncStatus() const;

    /**
     * Return all conflict records (both auto-resolved and pending manual
     * resolution).
     */
    std::vector<BidiConflictRecord> getConflictHistory() const;

    /**
     * Return only the conflict records that are awaiting manual resolution
     * (i.e. strategy == CUSTOM and no manual resolution has been applied yet).
     */
    std::vector<BidiConflictRecord> getPendingConflicts() const;

    // ── Conflict resolution ───────────────────────────────────────────────────

    /**
     * Manually resolve a conflict by nominating which node's write wins.
     *
     * Locates the conflict record by document_id (most recent conflict for that
     * document), marks it resolved, and updates resolved_write to the nominated
     * node's write.  winner_node must be either local_node_id or remote_node_id.
     *
     * Returns true when a matching unresolved conflict was found and resolved.
     */
    bool resolveConflict(const std::string& document_id,
                         const std::string& winner_node);

    // ── Configuration helpers ─────────────────────────────────────────────────

    /** Update the conflict resolution strategy for a specific collection. */
    void setCollectionStrategy(const std::string& collection,
                               ConflictResolution strategy);

    /** Read back the effective strategy for a collection. */
    ConflictResolution getEffectiveStrategy(const std::string& collection) const;

    // ── Simulation helpers (testing / integration) ────────────────────────────

    /**
     * Inject a remote sequence number directly (used by tests and integration
     * harnesses that do not run a real network layer).
     */
    void updateRemoteSequence(uint64_t remote_seq, int64_t lag_ms = 0);

    /**
     * Simulate an incoming DDL event from the peer.  Delegates to
     * applyRemoteWrite() with is_ddl=true.
     */
    bool applyRemoteDDL(const std::string& ddl_statement,
                        const std::string& schema_version,
                        uint64_t origin_seq);

private:
    // ── Origin tracking ───────────────────────────────────────────────────────
    struct OriginInfo {
        std::string origin_node;
        uint64_t    origin_sequence;
        std::chrono::system_clock::time_point origin_timestamp;
    };

    OriginInfo getOrigin(const std::string& document_id) const;
    bool       isLocalOrigin(const OriginInfo& origin) const;

    // ── Conflict helpers ──────────────────────────────────────────────────────
    /**
     * Apply the configured resolution strategy and return the winning entry.
     */
    BidiWriteEntry resolveWrite(const BidiWriteEntry& local,
                                const BidiWriteEntry& remote,
                                ConflictResolution strategy) const;

    /**
     * Detect whether two entries targeting the same (collection, document_id)
     * constitute a conflict.  Two writes conflict when both have been submitted
     * since the last known-good sync point (i.e. their sequence numbers are
     * both ahead of the last acknowledged remote sequence).
     */
    bool detectConflict(const BidiWriteEntry& incoming,
                        const BidiWriteEntry& existing) const;

    /**
     * Record a conflict and apply the configured strategy.
     */
    void handleConflict(const BidiWriteEntry& local_write,
                        const BidiWriteEntry& remote_write,
                        bool is_ddl);

    // ── State ─────────────────────────────────────────────────────────────────
    BidiConfig config_;

    std::atomic<bool>     running_{false};
    std::atomic<uint64_t> local_sequence_{0};
    std::atomic<uint64_t> remote_sequence_{0};
    std::atomic<int64_t>  replication_lag_ms_{0};
    std::atomic<uint64_t> conflicts_detected_{0};
    std::atomic<uint64_t> conflicts_resolved_{0};

    // Pending local writes keyed by (collection + "\0" + document_id)
    mutable std::mutex               pending_mutex_;
    std::map<std::string, BidiWriteEntry> pending_writes_;

    // Conflict history
    mutable std::mutex                  conflicts_mutex_;
    std::vector<BidiConflictRecord>     conflict_history_;
    std::deque<std::chrono::system_clock::time_point> conflict_timestamps_; ///< For conflicts_last_hour

    // Origin index: document key → last known origin info
    mutable std::mutex                  origin_mutex_;
    std::map<std::string, OriginInfo>   origin_map_;

    std::string makeDocKey(const std::string& collection,
                           const std::string& document_id) const;
};

// ============================================================================
// GeoReplicationManager  (v1.7.0)
// ============================================================================

/**
 * GeoReplicationManager
 *
 * Provides a simple key/value API for geo-distributed deployments with
 * per-request consistency level control.  Applications can choose between
 * four consistency levels depending on their trade-off between correctness
 * and availability:
 *
 *   STRONG            – Linearizable reads; only served from a region with
 *                       zero replication lag.
 *   BOUNDED_STALENESS – Reads permitted when lag <= max_staleness_ms; the
 *                       freshest eligible region is preferred.
 *   SESSION           – Read-your-writes within a session token; the local
 *                       region must have applied at least the sequence
 *                       embedded in the token.
 *   EVENTUAL          – Always served from the local region regardless of
 *                       lag; maximum throughput, no freshness guarantee.
 *
 * The manager tracks per-region staleness and selects the appropriate region
 * automatically (automatic routing).  In a real deployment, staleness is
 * updated via updateRegionStaleness() whenever a WAL acknowledgement or
 * heartbeat arrives from a remote region.
 *
 * Thread safety: all public methods are thread-safe.
 */
class GeoReplicationManager {
public:
    /**
     * Configuration for GeoReplicationManager.
     */
    struct GeoConfig {
        std::string              local_region;                   ///< ID of the local (primary) region
        std::vector<std::string> regions;                        ///< All region IDs (including local)
        uint32_t replication_factor  = 3;                        ///< Total number of replicas
        uint32_t local_replicas      = 2;                        ///< Replicas in the local region
        uint32_t global_replicas     = 1;                        ///< Replicas in remote regions
        ConsistencyLevel default_consistency = ConsistencyLevel::SESSION;
        uint32_t max_staleness_ms    = 5000;                     ///< Bound for BOUNDED_STALENESS (ms)
        uint32_t session_token_ttl_ms = 30000;                   ///< Session token TTL (ms)
    };

    explicit GeoReplicationManager(const GeoConfig& config);

    /**
     * Write a key/value pair with the specified consistency level.
     *
     * Returns false only when consistency == STRONG and the local replica is
     * not fully caught up (staleness > 0).  All other levels always succeed
     * locally.  The returned session_token encodes the new sequence number
     * for subsequent SESSION reads.
     */
    bool write(
        const std::string& key,
        const std::string& value,
        ConsistencyLevel   consistency = ConsistencyLevel::SESSION
    );

    /**
     * Read a value with the specified consistency level.
     *
     * Automatic routing rules:
     *   STRONG            – served only if local staleness == 0.
     *   BOUNDED_STALENESS – served only if local staleness <= max_staleness_ms.
     *   SESSION           – served only if local sequence >= token sequence.
     *   EVENTUAL          – always served.
     *
     * Returns std::nullopt when the consistency constraint cannot be satisfied
     * by the local region (caller should retry or relax the level).
     */
    std::optional<std::string> read(
        const std::string& key,
        ConsistencyLevel   consistency  = ConsistencyLevel::SESSION,
        const std::string& session_token = ""
    );

    /**
     * Return a fresh session token embedding the current local sequence.
     * Pass this token to subsequent read() calls to obtain read-your-writes
     * (SESSION consistency).
     */
    std::string getSessionToken() const;

    /**
     * Return the estimated replication lag for a given region.
     * Returns chrono::milliseconds::max() for unknown regions.
     */
    std::chrono::milliseconds getStaleness(const std::string& region) const;

    /**
     * Feed new staleness information from the replication layer.
     * Called on every WAL acknowledgement or heartbeat from a remote region.
     */
    void updateRegionStaleness(const std::string& region,
                               int64_t            staleness_ms,
                               uint64_t           last_applied_sequence = 0);

    /**
     * Select the best read region for the given consistency level and optional
     * session token.  Returns an empty string when no eligible region exists.
     */
    std::string selectReadRegion(
        ConsistencyLevel   consistency,
        const std::string& session_token = ""
    ) const;

    /**
     * Validate a session token and return the sequence it encodes.
     * Returns 0 for malformed or expired tokens.
     */
    uint64_t parseSessionToken(const std::string& token) const;

    /** Prometheus-format metrics snapshot. */
    std::string exportPrometheusMetrics() const;

private:
    GeoConfig config_;

    // Per-region staleness
    mutable std::shared_mutex             staleness_mutex_;
    std::map<std::string, RegionStalenessInfo> region_staleness_;

    // Monotonic write sequence for this region
    std::atomic<uint64_t> local_sequence_{0};

    // Metrics counters
    std::atomic<uint64_t> writes_total_{0};
    std::atomic<uint64_t> reads_total_{0};
    std::atomic<uint64_t> reads_rejected_{0};
    std::atomic<uint64_t> strong_reads_{0};
    std::atomic<uint64_t> bounded_staleness_reads_{0};
    std::atomic<uint64_t> session_reads_{0};
    std::atomic<uint64_t> eventual_reads_{0};

    std::string generateSessionToken(uint64_t sequence) const;
};

} // namespace replication
} // namespace themisdb
