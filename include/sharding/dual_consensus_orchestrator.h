// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file dual_consensus_orchestrator.h
 * @brief Dual-Consensus Orchestrator for Converged Storage-Inference
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY | Score: 95/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Enables coordinated consensus between KV-Cache (Raft) and Storage (Paxos)
 */

#pragma once

#include "sharding/consensus_module.h"
#include "sharding/raid_paxos_config.h"
#include <map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <functional>
#include <nlohmann/json.hpp>

namespace themisdb::sharding {

// Forward declarations
class RAIDPaxosConsensus;

/**
 * @brief Consensus layer type for Dual-Consensus
 */
enum class ConsensusLayer {
    STORAGE,    ///> Persistent storage layer (Paxos/RAID-Paxos)
    CACHE       ///> KV-Cache layer (Raft)
};

/**
 * @brief Cross-layer consistency state
 */
enum class CrossLayerConsistencyState {
    CONSISTENT,      ///> Both layers consistent
    STORAGE_AHEAD,   ///> Storage has newer data
    CACHE_AHEAD,     ///> Cache has newer data
    DIVERGED,       ///> Layers have diverged (conflict)
    RECOVERING       ///> Recovery in progress
};

/**
 * @brief Version token for tracking consistency across layers
 */
struct CrossLayerVersionToken {
    uint64_t storage_version = 0;    ///> Version from storage layer
    uint64_t cache_version = 0;      ///> Version from cache layer
    std::string transaction_id;      ///> Associated transaction ID
    std::chrono::system_clock::time_point timestamp;
    
    /**
     * @brief Check if this token is newer than another
     */
    bool isNewerThan(const CrossLayerVersionToken& other) const {
        // Storage version takes precedence for durability
        if (storage_version != other.storage_version) {
            return storage_version > other.storage_version;
        }
        // If storage versions match, cache version decides
        return cache_version > other.cache_version;
    }
    
    /**
     * @brief Check if both layers are consistent
     */
    bool isConsistent() const {
        return storage_version == cache_version;
    }
    
    /**
     * @brief Convert to JSON for serialization
     */
    nlohmann::json toJson() const {
        return {
            {"storage_version", storage_version},
            {"cache_version", cache_version},
            {"transaction_id", transaction_id},
            {"timestamp_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
                timestamp.time_since_epoch()).count()}
        };
    }
    
    /**
     * @brief Create from JSON
     */
    static CrossLayerVersionToken fromJson(const nlohmann::json& j) {
        CrossLayerVersionToken token;
        token.storage_version = j.value("storage_version", 0);
        token.cache_version = j.value("cache_version", 0);
        token.transaction_id = j.value("transaction_id", "");
        
        auto timestamp_ms = j.value("timestamp_ms", 0);
        token.timestamp = std::chrono::system_clock::time_point(
            std::chrono::milliseconds(timestamp_ms));
        
        return token;
    }
};

/**
 * @brief Dual-Consensus Orchestrator
 * 
 * Coordinates consensus between two layers:
 * - **Storage Layer**: Persistent storage using Paxos/RAID-Paxos (durability)
 * - **Cache Layer**: KV-Cache using Raft (performance)
 * 
 * This class is specifically designed for ThemisDB's **Converged Storage-Inference**
 * topology, where KV-Cache and Persistent-Storage share a common sharding topology.
 * 
 * Key features:
 * - Cross-layer version tracking
 * - Automatic synchronization between layers
 * - Conflict detection and resolution
 * - Non-blocking operations with background recovery
 * - Grounding auditability across shards
 */
class DualConsensusOrchestrator {
public:
    /**
     * @brief Callback for cache-storage synchronization
     */
    using SyncCallback = std::function<bool(const std::string& key,
                                           const nlohmann::json& value,
                                           const CrossLayerVersionToken& token)>;
    
    /**
     * @brief Callback for conflict resolution
     */
    using ConflictResolver = std::function<nlohmann::json(
        const std::string& key,
        const nlohmann::json& cache_value,
        const nlohmann::json& storage_value,
        const CrossLayerVersionToken& cache_token,
        const CrossLayerVersionToken& storage_token)>;
    
    /**
     * @brief Callback for consistency state changes
     */
    using ConsistencyCallback = std::function<void(
        const std::string& key,
        CrossLayerConsistencyState old_state,
        CrossLayerConsistencyState new_state)>;

    /**
     * @brief Construct Dual-Consensus Orchestrator
     * 
     * @param storage_consensus Consensus module for storage layer (Paxos/RAID-Paxos)
     * @param cache_consensus Consensus module for cache layer (Raft)
     */
    DualConsensusOrchestrator(
        std::unique_ptr<ConsensusModule> storage_consensus,
        std::unique_ptr<ConsensusModule> cache_consensus
    );
    
    /**
     * @brief Destructor
     */
    ~DualConsensusOrchestrator();
    
    // Delete copy constructors and assignment operators
    DualConsensusOrchestrator(const DualConsensusOrchestrator&) = delete;
    DualConsensusOrchestrator& operator=(const DualConsensusOrchestrator&) = delete;
    
    // ========================================================================
    // Initialization and Configuration
    // ========================================================================
    
    /**
     * @brief Initialize both consensus layers
     * 
     * @param node_id This node's identifier
     * @param cluster_nodes List of all cluster node identifiers
     * @return true if both layers initialized successfully
     */
    bool initialize(
        const std::string& node_id,
        const std::vector<std::string>& cluster_nodes
    );
    
    /**
     * @brief Start both consensus layers
     */
    void start();
    
    /**
     * @brief Stop both consensus layers
     */
    void stop();
    
    // ========================================================================
    // Storage Layer Operations
    // ========================================================================
    
    /**
     * @brief Propose an operation to the storage layer
     * 
     * @param operation Operation type
     * @param data Operation data
     * @return Log index if successful, nullopt otherwise
     */
    std::optional<uint64_t> proposeToStorage(
        const std::string& operation,
        const nlohmann::json& data
    );
    
    /**
     * @brief Wait for storage commit
     */
    bool waitForStorageCommit(
        uint64_t log_index,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)
    );
    
    /**
     * @brief Get latest storage version
     */
    uint64_t getStorageVersion() const;
    
    // ========================================================================
    // Cache Layer Operations
    // ========================================================================
    
    /**
     * @brief Propose an operation to the cache layer
     */
    std::optional<uint64_t> proposeToCache(
        const std::string& operation,
        const nlohmann::json& data
    );
    
    /**
     * @brief Wait for cache commit
     */
    bool waitForCacheCommit(
        uint64_t log_index,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)
    );
    
    /**
     * @brief Get latest cache version
     */
    uint64_t getCacheVersion() const;
    
    // ========================================================================
    // Cross-Layer Operations
    // ========================================================================
    
    /**
     * @brief Synchronously update both layers (blocking)
     * 
     * This is the traditional approach where both layers must commit
     * before the operation is considered successful.
     * 
     * @param key Data key
     * @param value Data value
     * @param operation Operation type
     * @return true if both layers committed successfully
     */
    bool syncUpdateBothLayers(
        const std::string& key,
        const nlohmann::json& value,
        const std::string& operation = "update"
    );
    
    /**
     * @brief Update cache first, then storage (non-blocking cache)
     * 
     * Optimized for read-heavy workloads where cache performance is critical.
     * Cache update returns immediately, storage update happens in background.
     * 
     * @param key Data key
     * @param value Data value
     * @param operation Operation type
     * @return Cache log index (storage update in background)
     */
    std::optional<uint64_t> updateCacheFirst(
        const std::string& key,
        const nlohmann::json& value,
        const std::string& operation = "update"
    );
    
    /**
     * @brief Update storage first, then cache (non-blocking cache)
     * 
     * Optimized for write-heavy workloads where durability is critical.
     * Storage update must succeed, cache update happens in background.
     * 
     * @param key Data key
     * @param value Data value
     * @param operation Operation type
     * @return Storage log index (cache update in background)
     */
    std::optional<uint64_t> updateStorageFirst(
        const std::string& key,
        const nlohmann::json& value,
        const std::string& operation = "update"
    );
    
    /**
     * @brief Update both layers with version tracking (Converged-optimized)
     * 
     * This is the primary method for Converged Storage-Inference.
     * It ensures both layers stay consistent while allowing non-blocking
     * operations when one layer is temporarily unavailable.
     * 
     * @param key Data key
     * @param value Data value
     * @param operation Operation type
     * @return Cross-layer version token
     */
    CrossLayerVersionToken updateWithVersionTracking(
        const std::string& key,
        const nlohmann::json& value,
        const std::string& operation = "update"
    );
    
    // ========================================================================
    // Version and Consistency Management
    // ========================================================================
    
    /**
     * @brief Get current cross-layer version token for a key
     * 
     * @param key Data key
     * @return Version token or nullopt if not found
     */
    std::optional<CrossLayerVersionToken> getVersionToken(
        const std::string& key
    ) const;
    
    /**
     * @brief Check cross-layer consistency for a key
     * 
     * @param key Data key
     * @return Consistency state
     */
    CrossLayerConsistencyState checkConsistency(
        const std::string& key
    ) const;
    
    /**
     * @brief Check if a key is consistent across layers
     * 
     * @param key Data key
     * @return true if consistent
     */
    bool isConsistent(const std::string& key) const;
    
    /**
     * @brief Get all inconsistent keys
     */
    std::vector<std::string> getInconsistentKeys() const;
    
    // ========================================================================
    // Recovery and Synchronization
    // ========================================================================
    
    /**
     * @brief Synchronize cache from storage for a specific key
     * 
     * Used when cache is stale or after recovery.
     * 
     * @param key Data key
     * @return true if synchronization was successful
     */
    bool syncCacheFromStorage(const std::string& key);
    
    /**
     * @brief Synchronize storage from cache for a specific key
     * 
     * Used when cache has newer data (rare, but possible in edge cases).
     * 
     * @param key Data key
     * @return true if synchronization was successful
     */
    bool syncStorageFromCache(const std::string& key);
    
    /**
     * @brief Resolve consistency conflict between layers
     * 
     * Uses the configured conflict resolver or default logic.
     * Default: Storage wins (durability > performance)
     * 
     * @param key Data key
     * @return true if conflict was resolved
     */
    bool resolveConflict(const std::string& key);
    
    /**
     * @brief Trigger full synchronization between layers
     * 
     * @return Number of keys synchronized
     */
    size_t triggerFullSync();
    
    // ========================================================================
    // Configuration and Callbacks
    // ========================================================================
    
    /**
     * @brief Set synchronization callback
     */
    void setSyncCallback(SyncCallback callback);
    
    /**
     * @brief Set conflict resolver
     */
    void setConflictResolver(ConflictResolver resolver);
    
    /**
     * @brief Set consistency state change callback
     */
    void setConsistencyCallback(ConsistencyCallback callback);
    
    /**
     * @brief Set background synchronization interval
     * 
     * @param interval Synchronization interval in milliseconds
     */
    void setBackgroundSyncInterval(std::chrono::milliseconds interval);
    
    // ========================================================================
    // Grounding and Auditability
    // ========================================================================
    
    /**
     * @brief Log a grounding operation with cross-layer version tracking
     * 
     * @param request_id Request identifier
     * @param shard_responses Responses from all shards
     * @param final_answer Final answer from the system
     * @param source_references Source references used for grounding
     */
    void logGroundingOperation(
        const std::string& request_id,
        const std::map<std::string, nlohmann::json>& shard_responses,
        const std::string& final_answer,
        const std::vector<std::string>& source_references
    );
    
    /**
     * @brief Get grounding audit log for a request
     * 
     * @param request_id Request identifier
     * @return Audit log entry or nullopt if not found
     */
    std::optional<nlohmann::json> getGroundingAuditLog(
        const std::string& request_id
    ) const;
    
    /**
     * @brief Get all grounding audit logs within a time range
     * 
     * @param start_time Start time
     * @param end_time End time
     * @return Vector of audit log entries
     */
    std::vector<nlohmann::json> getGroundingAuditLogs(
        std::chrono::system_clock::time_point start_time,
        std::chrono::system_clock::time_point end_time
    ) const;
    
    // ========================================================================
    // Metrics and Monitoring
    // ========================================================================
    
    /**
     * @brief Get dual-consensus metrics
     */
    nlohmann::json getMetrics() const;
    
    /**
     * @brief Reset metrics
     */
    void resetMetrics();
    
    // ========================================================================
    // Accessors
    // ========================================================================
    
    /**
     * @brief Get storage consensus module
     */
    ConsensusModule* getStorageConsensus() const { return storage_consensus_.get(); }
    
    /**
     * @brief Get cache consensus module
     */
    ConsensusModule* getCacheConsensus() const { return cache_consensus_.get(); }
    
    /**
     * @brief Check if both layers are operational
     */
    bool isFullyOperational() const;
    
    /**
     * @brief Check if in degraded mode (one layer down)
     */
    bool isDegraded() const;

private:
    // -------------------------------------------------------------------------
    // Lock hierarchy (always acquire in this order to prevent deadlocks):
    //   state_mutex_ (1) < audit_mutex_ (2) < metrics_mutex_ (3)
    // When holding two mutexes simultaneously use std::scoped_lock to acquire
    // both atomically.  Never acquire a lower-numbered mutex while holding a
    // higher-numbered one.
    // -------------------------------------------------------------------------

    // Consensus layers
    std::unique_ptr<ConsensusModule> storage_consensus_;  ///> Storage layer (Paxos/RAID-Paxos)
    std::unique_ptr<ConsensusModule> cache_consensus_;    ///> Cache layer (Raft)
    
    // State tracking  (lock level 1)
    mutable std::mutex state_mutex_;
    std::map<std::string, CrossLayerVersionToken> version_tokens_;  ///> Key -> Version token
    std::map<std::string, CrossLayerConsistencyState> consistency_states_;
    
    // Callbacks (protected by state_mutex_)
    SyncCallback sync_callback_;
    ConflictResolver conflict_resolver_;
    ConsistencyCallback consistency_callback_;
    
    // Background synchronization
    std::atomic<bool> running_;
    std::thread background_sync_thread_;
    /// Background sync interval stored as milliseconds count so it can be
    /// read/written atomically without requiring a mutex.
    std::atomic<uint64_t> background_sync_interval_ms_{5000};
    
    // Grounding audit logs  (lock level 2)
    mutable std::mutex audit_mutex_;
    std::map<std::string, nlohmann::json> grounding_audit_logs_;
    
    // Metrics  (lock level 3 — retained for future non-atomic metric fields)
    mutable std::mutex metrics_mutex_;
    std::atomic<uint64_t> total_operations_{0};
    std::atomic<uint64_t> successful_operations_{0};
    std::atomic<uint64_t> failed_operations_{0};
    std::atomic<uint64_t> consistency_conflicts_{0};
    std::atomic<uint64_t> sync_operations_{0};
    std::atomic<uint64_t> conflict_resolutions_{0};
    
    // ========================================================================
    // Private helper methods
    // ========================================================================
    
    /**
     * @brief Background synchronization thread
     */
    void backgroundSyncThread();
    
    /**
     * @brief Check and update consistency state for a key.
     * @note Caller must NOT hold state_mutex_ — acquires it internally.
     */
    void updateConsistencyState(const std::string& key);

    /**
     * @brief Check and update consistency state for a key (lock-free variant).
     * @note Caller MUST already hold state_mutex_.
     */
    void updateConsistencyStateLocked(const std::string& key);
    
    /**
     * @brief Default conflict resolver (storage wins)
     */
    nlohmann::json defaultConflictResolver(
        const std::string& key,
        const nlohmann::json& cache_value,
        const nlohmann::json& storage_value,
        const CrossLayerVersionToken& cache_token,
        const CrossLayerVersionToken& storage_token
    );
    
    /**
     * @brief Notify consistency callback if state changed
     */
    void notifyConsistencyChange(
        const std::string& key,
        CrossLayerConsistencyState old_state,
        CrossLayerConsistencyState new_state
    );
};

/**
 * @brief Factory function to create Dual-Consensus Orchestrator
 * 
 * @param raid_mode RAID mode for storage layer
 * @param base_config Base consensus configuration
 * @return Unique pointer to Dual-Consensus Orchestrator
 */
std::unique_ptr<DualConsensusOrchestrator> createDualConsensusOrchestrator(
    RAIDMode raid_mode = RAIDMode::MIRROR,
    const ConsensusConfig& base_config = ConsensusConfig{}
);

/**
 * @brief Factory function to create Dual-Consensus Orchestrator with RAID-Paxos
 * 
 * @param raid_config RAID-specific configuration
 * @param raft_config Raft configuration for cache layer
 * @return Unique pointer to Dual-Consensus Orchestrator
 */
std::unique_ptr<DualConsensusOrchestrator> createDualConsensusOrchestrator(
    const RAIDPaxosConfig& raid_config,
    const ConsensusConfig& raft_config = ConsensusConfig{}
);

} // namespace themisdb::sharding
