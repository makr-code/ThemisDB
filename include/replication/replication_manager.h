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
#include <condition_variable>
#include <thread>
#include <optional>
#include <queue>

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
    
    bool isHealthy() const {
        auto now = std::chrono::system_clock::now();
        return (now - last_heartbeat) < std::chrono::seconds(30);
    }
    
    int64_t replicationLagMs() const;
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

private:
    std::string node_id_;
    ReplicationConfig config_;
    std::shared_ptr<WALManager> wal_;
    
    std::atomic<ReplicationRole> role_{ReplicationRole::FOLLOWER};
    std::atomic<uint64_t> current_term_{0};
    std::string voted_for_;
    std::string current_leader_;
    
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

private:
    std::string follower_endpoint_;
    std::shared_ptr<WALManager> wal_;
    ReplicationConfig config_;
    ReplicaInfo follower_info_;
    
    std::atomic<uint64_t> last_acked_sequence_{0};
    std::atomic<bool> running_{false};
    std::thread stream_thread_;
    
    void streamLoop();
    bool sendBatch(const std::vector<WALEntry>& entries);
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

private:
    ReplicationConfig config_;
    std::string node_id_;
    
    std::shared_ptr<WALManager> wal_;
    std::unique_ptr<LeaderElection> election_;
    
    std::vector<std::unique_ptr<ReplicationStream>> streams_;
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
    
    void heartbeatLoop();
    void compactionLoop();
    void notifyListeners(std::function<void(IReplicationListener&)> callback);
};

} // namespace replication
} // namespace themisdb
