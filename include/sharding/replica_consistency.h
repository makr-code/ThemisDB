/**
 * @file replica_consistency.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: replica_consistency.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/raft_log.h"
#include <string>
#include <map>
#include <vector>
#include <optional>
#include <functional>
#include <memory>
#include <variant>
#include <chrono>

namespace themisdb {
namespace sharding {

/**
 * @brief Vector clock for causality tracking
 */
class VectorClock {
public:
    VectorClock() = default;
    explicit VectorClock(const std::map<std::string, uint64_t>& timestamps);
    
    /**
     * @brief Increment clock for a node
     */
    void increment(const std::string& node_id);
    
    /**
     * @brief Update clock with another clock (merge)
     */
    void update(const VectorClock& other);
    
    /**
     * @brief Get timestamp for a node
     */
    uint64_t get(const std::string& node_id) const;
    
    /**
     * @brief Check if this clock happens before another
     */
    bool happensBefore(const VectorClock& other) const;
    
    /**
     * @brief Check if this clock happens after another
     */
    bool happensAfter(const VectorClock& other) const;
    
    /**
     * @brief Check if clocks are concurrent (neither before nor after)
     */
    bool isConcurrent(const VectorClock& other) const;
    
    /**
     * @brief Serialize to string
     */
    std::string serialize() const;
    
    /**
     * @brief Deserialize from string
     */
    static std::optional<VectorClock> deserialize(const std::string& data);
    
    /**
     * @brief Get all timestamps
     */
    const std::map<std::string, uint64_t>& getTimestamps() const { 
        return timestamps_; 
    }

private:
    std::map<std::string, uint64_t> timestamps_;
};

/**
 * @brief Versioned data entry
 */
struct VersionedEntry {
    std::string data;
    VectorClock version;
    std::string node_id;  // Node that created this version
    std::chrono::system_clock::time_point timestamp;
    
    std::string serialize() const;
    static std::optional<VersionedEntry> deserialize(const std::string& data);
};

/**
 * @brief Conflict resolution strategy
 */
enum class ConflictResolutionStrategy {
    LAST_WRITE_WINS,        // Use timestamp
    VECTOR_CLOCK_ORDERING,  // Use causality
    HIGHEST_NODE_ID,        // Deterministic by node ID
    MANUAL                  // Requires manual resolution
};

/**
 * @brief Conflict between versions
 */
struct VersionConflict {
    std::string key;
    std::vector<VersionedEntry> conflicting_versions;
    ConflictResolutionStrategy resolution_strategy;
    std::optional<VersionedEntry> resolved_version;
    bool needs_manual_resolution;
};

/**
 * @brief Replica consistency manager
 * 
 * Manages consistency across replicas using vector clocks
 * and handles partition healing with conflict resolution.
 */
class ReplicaConsistencyManager {
public:
    using ConflictCallback = std::function<VersionedEntry(const VersionConflict&)>;
    
    struct Config {
        ConflictResolutionStrategy default_strategy{ConflictResolutionStrategy::LAST_WRITE_WINS};
        bool auto_resolve_conflicts{true};
        bool track_causality{true};
        uint32_t max_version_history{100};
    };
    
    explicit ReplicaConsistencyManager(const Config& config);
    ~ReplicaConsistencyManager() = default;
    
    /**
     * @brief Record a write operation
     * @param key Data key
     * @param data Data value
     * @param node_id Node performing the write
     * @return Versioned entry with vector clock
     */
    VersionedEntry recordWrite(const std::string& key,
                               const std::string& data,
                               const std::string& node_id);
    
    /**
     * @brief Merge entries from different replicas
     * @param key Data key
     * @param entries Entries from different replicas
     * @return Resolved entry or conflict
     */
    std::variant<VersionedEntry, VersionConflict> 
    mergeReplicas(const std::string& key,
                  const std::vector<VersionedEntry>& entries);
    
    /**
     * @brief Resolve conflict manually
     * @param conflict Conflict to resolve
     * @param resolved_entry Manually resolved entry
     */
    void resolveConflict(const VersionConflict& conflict,
                        const VersionedEntry& resolved_entry);
    
    /**
     * @brief Get current vector clock for a node
     */
    VectorClock getVectorClock(const std::string& node_id) const;
    
    /**
     * @brief Update vector clock from another node
     */
    void updateVectorClock(const std::string& node_id, const VectorClock& clock);
    
    /**
     * @brief Set conflict resolution callback
     */
    void setConflictCallback(ConflictCallback callback);
    
    /**
     * @brief Get version history for a key
     */
    std::vector<VersionedEntry> getVersionHistory(const std::string& key) const;
    
    /**
     * @brief Merge log entries from partitioned replicas
     * @param local_entries Local log entries
     * @param remote_entries Remote log entries
     * @return Merged log entries
     */
    std::vector<LogEntry> mergePartitionedLogs(
        const std::vector<LogEntry>& local_entries,
        const std::vector<LogEntry>& remote_entries);
    
    /**
     * @brief Statistics
     */
    struct Statistics {
        uint64_t total_writes{0};
        uint64_t conflicts_detected{0};
        uint64_t conflicts_resolved{0};
        uint64_t manual_resolutions{0};
        uint64_t merges_performed{0};
    };
    
    const Statistics& getStatistics() const { return stats_; }

private:
    Config config_;
    
    mutable std::mutex mutex_;
    std::map<std::string, VectorClock> node_clocks_;
    std::map<std::string, std::vector<VersionedEntry>> version_history_;
    
    Statistics stats_;
    ConflictCallback conflict_callback_;
    
    /**
     * @brief Detect conflicts between entries
     */
    std::optional<VersionConflict> detectConflict(
        const std::string& key,
        const std::vector<VersionedEntry>& entries);
    
    /**
     * @brief Auto-resolve conflict using strategy
     */
    VersionedEntry autoResolveConflict(const VersionConflict& conflict);
    
    /**
     * @brief Select winning version using strategy
     */
    VersionedEntry selectWinningVersion(
        const std::vector<VersionedEntry>& entries,
        ConflictResolutionStrategy strategy);
};

}  // namespace sharding
}  // namespace themisdb
