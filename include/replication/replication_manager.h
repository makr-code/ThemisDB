/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            replication_manager.h                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1272                                           ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f34d9abde  2026-02-22  fix(replication): audit fixes – config validation + Prome... ║
    • 573513108  2026-02-22  feat(replication): implement Raft leader lease reads for ... ║
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

/**
 * Replication Role
 */
enum class ReplicationRole {
    LEADER,         // Primary node accepting writes
    FOLLOWER,       // Read replica receiving updates
    CANDIDATE,      // Participating in leader election
    OBSERVER        // Non-voting member (async replica)
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
    
    // TLS/Security
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
    bool require_mtls = true;
    
    // Initial cluster members
    std::vector<std::string> seed_nodes;
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
    std::atomic<uint64_t> replica_failures_detected{0};
    std::atomic<uint64_t> network_partitions_detected{0};
    // Leader lease read counters
    std::atomic<uint64_t> lease_reads_served{0};
    std::atomic<uint64_t> lease_reads_rejected{0};
    
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

} // namespace replication
} // namespace themisdb
