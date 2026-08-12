/**
 * @file multi_master_replication.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Multi-Master Replication
 * 
 * Multi-Master replication architecture enabling writes on any node
 * with automatic conflict detection and resolution.
 * 
 * Features:
 * - Write anywhere: Any node can accept writes
 * - Conflict detection: Vector clocks + hybrid logical clocks
 * - Conflict resolution: Pluggable strategies (LWW, CRDT, custom)
 * - Causality tracking: Ensures causal consistency
 * - Anti-entropy: Background synchronization
 * - Cross-datacenter: Geo-distributed deployment support
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <chrono>
#include <functional>
#include <atomic>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <queue>

namespace themisdb {
namespace replication {

// Forward declarations
class VectorClock;
class HybridLogicalClock;
class CRDTMerger;
class ConflictResolver;

/**
 * Multi-Master Node State
 */
enum class MMNodeState {
    ACTIVE,             // Normal operation
    SYNCING,            // Catching up with peers
    PARTITIONED,        // Network partition detected
    RECOVERING,         // Recovery after failure
    OFFLINE             // Node is offline
};

/**
 * Conflict Type
 */
enum class ConflictType {
    CONCURRENT_UPDATE,      // Same document updated on different nodes
    DELETE_UPDATE,          // One node deleted, another updated
    SCHEMA_CONFLICT,        // Incompatible schema changes
    CONSTRAINT_VIOLATION,   // Unique constraint violated
    CAUSALITY_VIOLATION     // Causal order violated
};

/**
 * Vector Clock for causality tracking
 * Maps node_id -> logical timestamp
 */
class VectorClock {
public:
    VectorClock() = default;
    explicit VectorClock(const std::string& node_id);
    
    // Copy / move constructors (mutex is not copied – new instance gets its own mutex)
    VectorClock(const VectorClock& other);
    VectorClock(VectorClock&& other) noexcept;
    VectorClock& operator=(const VectorClock& other);
    VectorClock& operator=(VectorClock&& other) noexcept;
    
    // Increment this node's clock
    void increment(const std::string& node_id);
    
    // Merge with another vector clock (take max of each component)
    void merge(const VectorClock& other);
    
    // Get timestamp for a specific node
    uint64_t get(const std::string& node_id) const;
    
    // Compare two vector clocks
    // Returns: -1 if this < other, 0 if concurrent, 1 if this > other
    int compare(const VectorClock& other) const;
    
    // Check if this happened-before other
    bool happensBefore(const VectorClock& other) const;
    
    // Check if clocks are concurrent (neither happened-before the other)
    bool isConcurrent(const VectorClock& other) const;
    
    // Serialize to JSON
    std::string toJson() const;
    
    // Deserialize from JSON
    static VectorClock fromJson(const std::string& json);
    
private:
    std::map<std::string, uint64_t> clocks_;
    mutable std::shared_mutex mutex_;
};

/**
 * Hybrid Logical Clock (HLC)
 * Combines physical time with logical counters
 * Paper: "Logical Physical Clocks and Consistent Snapshots in Globally Distributed Databases"
 */
class HybridLogicalClock {
public:
    struct Timestamp {
        uint64_t physical;  // Physical time (milliseconds since epoch)
        uint32_t logical;   // Logical counter
        std::string node_id;

        bool operator<(const Timestamp& other) const {
            if (physical != other.physical) return physical < other.physical;
            if (logical  != other.logical)  return logical  < other.logical;
            return node_id < other.node_id;
        }
        bool operator==(const Timestamp& other) const {
            return physical == other.physical &&
                   logical  == other.logical  &&
                   node_id  == other.node_id;
        }
        std::string toString() const {
            std::ostringstream oss;
            oss << "HLC(" << physical << "," << logical << "," << node_id << ")";
            return oss.str();
        }
    };
    
    explicit HybridLogicalClock(const std::string& node_id);
    
    // Generate a new timestamp for a local event
    Timestamp now();
    
    // Update clock based on received message timestamp
    Timestamp receive(const Timestamp& received);
    
    // Get current timestamp without incrementing
    Timestamp current() const;
    
private:
    std::string node_id_;
    std::atomic<uint64_t> last_physical_;
    std::atomic<uint32_t> logical_counter_;
    mutable std::mutex mutex_;
};

/**
 * Multi-Master Write Entry
 * Contains all metadata needed for conflict detection and resolution
 */
struct MMWriteEntry {
    std::string write_id;           // Unique write identifier
    std::string origin_node;        // Node that originated the write
    std::string collection;
    std::string document_id;
    std::string operation;          // INSERT, UPDATE, DELETE, MERGE
    std::string data;               // JSON payload
    VectorClock vector_clock;       // Causality tracking
    HybridLogicalClock::Timestamp hlc;  // Hybrid logical timestamp
    std::string checksum;           // Content checksum
    std::vector<std::string> dependencies;  // Causal dependencies
    
    // Serialize/deserialize
    std::vector<uint8_t> serialize() const;
    static std::optional<MMWriteEntry> deserialize(const std::vector<uint8_t>& data);
};

/**
 * Conflict Record
 * Represents a detected conflict between writes
 */
struct ConflictRecord {
    std::string conflict_id;
    ConflictType type;
    std::string document_id;
    std::string collection;
    std::vector<MMWriteEntry> conflicting_writes;
    std::chrono::system_clock::time_point detected_at;
    bool resolved;
    std::string resolution_strategy;
    std::string winning_write_id;
};

/**
 * Multi-Master Peer Information
 */
struct MMPeerInfo {
    std::string node_id;
    std::string endpoint;
    std::string datacenter;
    std::string region;
    MMNodeState state;
    VectorClock last_known_clock;
    HybridLogicalClock::Timestamp last_heartbeat_hlc;
    uint64_t replication_lag_ms;    // Estimated lag
    uint32_t priority;              // For conflict resolution
    bool is_local_datacenter;
};

/**
 * Conflict Resolution Strategy Interface
 */
class ConflictResolver {
public:
    virtual ~ConflictResolver() = default;
    
    // Resolve a conflict, returns the winning write
    [[nodiscard]] virtual MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes
    ) = 0;
    
    // Get strategy name
    [[nodiscard]] virtual std::string strategyName() const = 0;
};

/**
 * Last-Write-Wins Resolver
 * Uses HLC timestamps to determine winner
 */
class LastWriteWinsResolver : public ConflictResolver {
public:
    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes
    ) override;
    
    std::string strategyName() const override { return "LAST_WRITE_WINS"; }
};

/**
 * CRDT-based Merge Resolver
 * Automatically merges concurrent writes using CRDT semantics
 */
class CRDTMergeResolver : public ConflictResolver {
public:
    enum class CRDTType {
        LWW_REGISTER,       // Last-Write-Wins Register
        MV_REGISTER,        // Multi-Value Register
        G_COUNTER,          // Grow-only Counter
        PN_COUNTER,         // Positive-Negative Counter
        G_SET,              // Grow-only Set
        OR_SET,             // Observed-Remove Set
        LWW_MAP,            // Last-Write-Wins Map
        TWO_P_SET,          // Two-Phase Set (supports removal via tombstones)
        RGA,                // Replicated Growable Array (ordered sequences)
        FLAG_EW,            // Enable-Wins Flag (concurrent enable+disable → enabled)
        FLAG_DW             // Disable-Wins Flag (concurrent enable+disable → disabled)
    };
    
    explicit CRDTMergeResolver(CRDTType type);
    
    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes
    ) override;
    
    std::string strategyName() const override;
    
private:
    CRDTType crdt_type_;
    
    // CRDT merge implementations
    std::string mergeLWWRegister(const std::vector<MMWriteEntry>& writes);
    std::string mergeMVRegister(const std::vector<MMWriteEntry>& writes);
    std::string mergeGCounter(const std::vector<MMWriteEntry>& writes);
    std::string mergePNCounter(const std::vector<MMWriteEntry>& writes);
    std::string mergeGSet(const std::vector<MMWriteEntry>& writes);
    std::string mergeORSet(const std::vector<MMWriteEntry>& writes);
    std::string mergeLWWMap(const std::vector<MMWriteEntry>& writes);
    std::string mergeTwoPSet(const std::vector<MMWriteEntry>& writes);
    std::string mergeRGA(const std::vector<MMWriteEntry>& writes);
    std::string mergeFlagEW(const std::vector<MMWriteEntry>& writes);
    std::string mergeFlagDW(const std::vector<MMWriteEntry>& writes);
};

/**
 * Custom Application Resolver
 * Allows application-specific conflict resolution logic
 */
class CustomResolver : public ConflictResolver {
public:
    using ResolverFunc = std::function<MMWriteEntry(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& writes
    )>;
    
    explicit CustomResolver(ResolverFunc resolver);
    
    MMWriteEntry resolve(
        const std::string& document_id,
        const std::vector<MMWriteEntry>& conflicting_writes
    ) override;
    
    std::string strategyName() const override { return "CUSTOM"; }
    
private:
    ResolverFunc resolver_;
};

/**
 * Multi-Master Replication Configuration
 */
struct MMReplicationConfig {
    std::string node_id;
    std::vector<std::string> seed_peers;        // Initial peer endpoints
    std::string datacenter;
    std::string region;
    
    // Replication settings
    uint32_t replication_factor = 3;            // Number of copies
    uint32_t write_quorum = 2;                  // Writes to acknowledge
    uint32_t read_quorum = 1;                   // Reads to satisfy
    
    // Timing
    uint32_t heartbeat_interval_ms = 1000;
    uint32_t sync_interval_ms = 100;            // Anti-entropy interval
    uint32_t timeout_ms = 5000;
    
    // Conflict resolution
    std::string default_resolution_strategy = "LAST_WRITE_WINS";
    std::map<std::string, std::string> collection_strategies;  // Per-collection override
    
    // Performance
    uint32_t max_batch_size = 1000;
    uint32_t max_pending_writes = 10000;
    bool async_apply = true;
    
    // Network
    bool use_mtls = true;
    std::string cert_path;
    std::string key_path;
    std::string ca_path;
};

/**
 * Multi-Master Replication Manager
 * 
 * Core class managing multi-master replication across nodes
 */
class MultiMasterReplicationManager {
public:
    using WriteCallback = std::function<void(const MMWriteEntry&, bool success)>;
    using ConflictCallback = std::function<void(const ConflictRecord&)>;
    
    explicit MultiMasterReplicationManager(const MMReplicationConfig& config);
    ~MultiMasterReplicationManager();
    
    // Lifecycle
    bool start();
    void stop();
    bool isRunning() const;
    
    // Write Operations
    // Returns write_id for tracking
    std::string write(
        const std::string& collection,
        const std::string& document_id,
        const std::string& operation,
        const std::string& data,
        WriteCallback callback = nullptr
    );
    
    // Synchronous write with quorum
    bool writeSync(
        const std::string& collection,
        const std::string& document_id,
        const std::string& operation,
        const std::string& data,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)
    );
    
    // Read Operations
    struct ReadResult {
        bool success;
        std::string data;
        VectorClock version;
        std::string source_node;
    };
    
    ReadResult read(
        const std::string& collection,
        const std::string& document_id,
        uint32_t read_quorum = 0  // 0 = use config default
    );
    
    // Peer Management
    void addPeer(const MMPeerInfo& peer);
    void removePeer(const std::string& node_id);
    std::vector<MMPeerInfo> getPeers() const;
    MMPeerInfo getLocalInfo() const;
    
    // Conflict Management
    void registerConflictCallback(ConflictCallback callback);
    void setConflictResolver(
        const std::string& collection,
        std::shared_ptr<ConflictResolver> resolver
    );
    std::vector<ConflictRecord> getUnresolvedConflicts() const;
    bool resolveConflict(const std::string& conflict_id, const std::string& winning_write_id);
    
    // Synchronization
    void triggerSync();  // Force immediate sync with peers
    uint64_t getReplicationLag() const;  // Max lag across all peers
    
    // Statistics
    struct Stats {
        uint64_t writes_total;
        uint64_t writes_replicated;
        uint64_t writes_pending;
        uint64_t conflicts_detected;
        uint64_t conflicts_resolved;
        uint64_t sync_rounds;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        std::chrono::milliseconds avg_replication_latency;
    };
    Stats getStats() const;

    // Topology Snapshot for web UI visualization
    struct TopologyNode {
        std::string node_id;
        std::string endpoint;
        std::string datacenter;
        std::string region;
        std::string state;          // "ACTIVE", "SYNCING", "PARTITIONED", "RECOVERING", "OFFLINE"
        uint64_t replication_lag_ms;
        bool is_local;              // True for this node
    };

    struct TopologyEdge {
        std::string from;
        std::string to;
        std::string type;           // "PEER"
    };

    struct TopologySnapshot {
        std::string local_node_id;
        std::vector<TopologyNode> nodes;
        std::vector<TopologyEdge> edges;
        uint64_t max_lag_ms;
        std::string replication_mode; // "MULTI_MASTER"
    };

    /**
     * Build a topology snapshot for visualization (web UI / REST API).
     * Returns the local node and all known peers with their current state
     * and estimated replication lag.
     */
    TopologySnapshot getTopologySnapshot() const;

    // Prometheus Metrics Export
    std::string exportPrometheusMetrics() const;
    
private:
    MMReplicationConfig config_;
    std::atomic<bool> running_{false};
    
    // Clock management
    std::unique_ptr<VectorClock> vector_clock_;
    std::unique_ptr<HybridLogicalClock> hlc_;
    
    // Peer state
    std::map<std::string, MMPeerInfo> peers_;
    mutable std::shared_mutex peers_mutex_;
    
    // Write queue
    std::queue<MMWriteEntry> pending_writes_;
    std::map<std::string, WriteCallback> write_callbacks_;
    mutable std::mutex writes_mutex_;
    std::condition_variable writes_cv_;

    // Committed write log: entries appended after successful replication so
    // that getMissingWrites() can return the delta to lagging peers.
    // Capped at max_pending_writes * 2 to bound memory usage.
    std::deque<MMWriteEntry> committed_writes_log_;
    mutable std::mutex committed_log_mutex_;
    
    // Conflict management
    std::map<std::string, std::shared_ptr<ConflictResolver>> resolvers_;
    std::shared_ptr<ConflictResolver> default_resolver_;
    std::vector<ConflictRecord> conflicts_;
    std::vector<ConflictCallback> conflict_callbacks_;
    mutable std::mutex conflicts_mutex_;
    
    // Background threads
    std::thread replication_thread_;
    std::thread heartbeat_thread_;
    std::thread sync_thread_;
    
    // Statistics
    std::atomic<uint64_t> stats_writes_total_{0};
    std::atomic<uint64_t> stats_writes_replicated_{0};
    std::atomic<uint64_t> stats_conflicts_detected_{0};
    std::atomic<uint64_t> stats_conflicts_resolved_{0};
    std::atomic<uint64_t> stats_sync_rounds_{0};
    std::atomic<uint64_t> stats_bytes_sent_{0};
    std::atomic<uint64_t> stats_bytes_received_{0};
    
    // Internal methods
    void replicationLoop();
    void heartbeatLoop();
    void syncLoop();
    
    bool replicateWrite(const MMWriteEntry& entry);
    bool sendToPeer(const std::string& node_id, const MMWriteEntry& entry);
    void receiveFromPeer(const std::string& node_id, const MMWriteEntry& entry);
    
    bool detectConflict(const MMWriteEntry& incoming, const MMWriteEntry& existing);
    void handleConflict(const std::string& document_id, 
                        const std::vector<MMWriteEntry>& conflicting_writes);
    
    void antiEntropySync(const std::string& peer_id);
    std::vector<MMWriteEntry> getMissingWrites(const VectorClock& peer_clock);
};

} // namespace replication
} // namespace themisdb
