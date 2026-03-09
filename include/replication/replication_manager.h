/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            replication_manager.h                              ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:54:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1724                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • abe3d4c8a  2026-03-01  feat(replication): update file metadata headers for witne... ║
    • 072dbcc55  2026-02-27  feat(replication): add witness node support for quorum in... ║
    • 4fc982b0d  2026-02-25  feat(replication): implement compressed WAL shipping (Zst... ║
    • 76c3f5b7b  2026-02-25  fix(replication): audit fixes – remove dead session code,... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
class WALEntry;
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
    virtual std::string resolve(
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
    virtual void onWALEntryApplied(const WALEntry& entry) {}
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
    std::chrono::steady_clock::time_point last_heartbeat_time_;

    // Leader lease expiry time; epoch when no lease is held.
    mutable std::mutex lease_mutex_;
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
    explicit ReplicationManager(const ReplicationConfig& config);
    ~ReplicationManager();
    
    // Initialize replication
    bool initialize();
    
    // Shutdown replication
    void shutdown();
    
    // Replicate a write operation
    bool replicate(const WALEntry& entry);
    
    // Wait for replication to complete (sync mode)
    bool waitForReplication(uint64_t sequence, uint32_t timeout_ms = 0);
    
    // Get current role
    ReplicationRole getRole() const;
    
    // Get leader endpoint (empty if this node is leader)
    std::string getLeaderEndpoint() const;
    
    // Get all replicas
    std::vector<ReplicaInfo> getReplicas() const;
    
    // Get replication statistics
    const ReplicationStats& getStats() const { return stats_; }
    
    // Add/remove replicas
    void addReplica(const ReplicaInfo& replica);
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
    
    // Set custom conflict resolver
    void setConflictResolver(std::shared_ptr<IConflictResolver> resolver);
    
    // Add event listener
    void addListener(std::shared_ptr<IReplicationListener> listener);
    
    // Trigger manual failover to specific node
    bool triggerFailover(const std::string& target_node_id);
    
    // Promote this follower to leader (for maintenance)
    bool promoteToLeader();
    
    // Demote this leader to follower
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
    // Get health status of all replicas
    std::vector<std::pair<std::string, HealthStatus>> getReplicaHealthStatus() const;
    
    // Check if cluster has quorum
    bool hasQuorum() const;
    
    // Trigger health check on all replicas
    void performHealthCheck();
    
    // Detect network partition
    bool detectNetworkPartition() const;
    
    // Get read preference configuration
    ReadPreference getReadPreference() const { return config_.default_read_preference; }
    
    // Set read preference
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
        uint32_t worker_threads   = 4;
        uint32_t queue_size       = 10000;
        bool use_dependency_tracking = true;
    };

    struct Stats {
        uint64_t entries_applied;
        uint64_t dependencies_detected;
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
        uint32_t read_quorum      = 2;
        uint32_t read_timeout_ms  = 1000;
        bool     repair_on_read   = true;
    };

    struct QuorumReadResult {
        bool        success;
        std::string data;
        uint64_t    version;
        bool        had_conflicts;
        std::vector<std::string> sources;  // replica endpoints that responded
    };

    explicit QuorumReadManager(
        const QuorumReadConfig& config,
        const std::vector<ReplicaInfo>& replicas
    );

    QuorumReadResult read(
        const std::string& collection,
        const std::string& document_id,
        uint32_t quorum = 0  // 0 = use config default
    );

    // Update the replica list (called when topology changes)
    void setReplicas(const std::vector<ReplicaInfo>& replicas);

private:
    QuorumReadConfig config_;
    std::vector<ReplicaInfo> replicas_;
    mutable std::shared_mutex replicas_mutex_;

    // Per-replica read simulation (real impl would use RPC)
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
 * WALArchivalManager
 *
 * Archives completed WAL segments to a local (or cloud-pluggable) directory
 * with optional compression.  Provides retrieval for point-in-time recovery.
 *
 * Cloud backends (S3, GCS, Azure) are pluggable via the `IArchivalBackend`
 * interface; the default backend writes to the local filesystem.
 */
class WALArchivalManager {
public:
    struct ArchivalConfig {
        std::string wal_directory;          // Source WAL directory
        std::string archive_directory;      // Local archive destination
        uint32_t    archive_after_segments  = 100; // segments to accumulate before archiving
        uint32_t    local_retention_segments= 10;  // segments to keep locally after archive
        bool        compress_before_archive = true;
        uint32_t    delete_after_days       = 365; // purge archived segments older than N days
    };

    struct ArchivedSegment {
        uint64_t    segment_id;
        uint64_t    start_sequence;
        uint64_t    end_sequence;
        uint64_t    size_bytes;
        bool        compressed;
        std::chrono::system_clock::time_point archived_at;
        std::string archive_path;
    };

    explicit WALArchivalManager(const ArchivalConfig& config);

    // Archive the given WAL segment files (paths relative to wal_directory).
    // Returns number of segments successfully archived.
    uint32_t archiveSegments(const std::vector<std::string>& segment_paths);

    // Retrieve an archived segment by ID; returns raw bytes (possibly compressed).
    std::optional<std::vector<uint8_t>> retrieveSegment(uint64_t segment_id) const;

    // List all archived segments (sorted by segment_id ascending).
    std::vector<ArchivedSegment> listArchived() const;

    // Purge archived segments older than delete_after_days.
    uint32_t purgeExpired();

    // Background archival: scan wal_directory, archive old segments, return count.
    uint32_t runArchivalCycle();

private:
    ArchivalConfig config_;
    mutable std::mutex archive_mutex_;
    std::vector<ArchivedSegment> index_;  // in-memory index; persisted via text-format index.txt side-car

    std::string archivePath(uint64_t segment_id) const;
    void saveIndex() const;
    void loadIndex();
    static std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
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
 */
struct MultiRegionActiveActiveConfig {
    std::string local_region_id;                         ///< Identifier for the local region
    std::vector<std::string> peer_region_ids;            ///< Peer region identifiers
    ConsistencyLevel default_consistency = ConsistencyLevel::BOUNDED_STALENESS;
    uint32_t max_staleness_ms = 5000;                    ///< Upper bound for BOUNDED_STALENESS reads (ms)
    uint32_t session_token_ttl_ms = 30000;               ///< Time-to-live for session tokens (ms)
    ConflictResolution conflict_strategy = ConflictResolution::LAST_WRITE_WINS;
};

/**
 * MultiRegionActiveActiveManager
 *
 * Coordinates multi-region active-active replication with bounded staleness
 * guarantees.  Every region can accept writes.  Reads are served according to
 * the requested ConsistencyLevel:
 *
 *   STRONG            – Only served when the local replica is known to be
 *                       up-to-date (staleness_ms == 0 or within lease window).
 *   BOUNDED_STALENESS – Served when staleness_ms <= max_staleness_ms; otherwise
 *                       the read is rejected (caller should retry or fall back
 *                       to another region).
 *   SESSION           – Always served, but only if the region has applied at
 *                       least up to the sequence embedded in the session token
 *                       (read-your-writes guarantee).
 *   EVENTUAL          – Always served from the local region regardless of lag.
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
     * Returns success=false when:
     *   - STRONG: local staleness > 0 (i.e., the replica is not fully caught up)
     *   - BOUNDED_STALENESS: local staleness > max_staleness_ms
     *   - SESSION: the local replica has not yet applied the sequence in the token
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

    std::string generateWriteId(uint64_t sequence) const;
    std::string generateSessionToken(uint64_t sequence) const;
    uint64_t    parseSessionToken(const std::string& token) const;   ///< Returns 0 on error
};

} // namespace replication
} // namespace themisdb
