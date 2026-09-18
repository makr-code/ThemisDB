/**
 * @file replica_consistency.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    /** @brief Construct empty vector clock. */
    VectorClock() = default;
    /** @brief Construct vector clock from node->counter map. */
    explicit VectorClock(const std::map<std::string, uint64_t>& timestamps);
    
     * @param[in] node_id Input parameter.
    /** @brief Increment logical counter for node_id. */
    void increment(const std::string& node_id);
    
     * @param[in] other Input parameter.
    /** @brief Merge with another clock using element-wise maximum. */
    void update(const VectorClock& other);
    
     * @param[in] node_id Input parameter.
     * @return Return value.
    /** @brief Get node counter value (0 when node is absent). */
    uint64_t get(const std::string& node_id) const;
    
     * @param[in] other Input parameter.
     * @return True on success.
    /** @brief Return true when this clock causally precedes other clock. */
    bool happensBefore(const VectorClock& other) const;
    
     * @param[in] other Input parameter.
     * @return True on success.
    /** @brief Return true when this clock causally succeeds other clock. */
    bool happensAfter(const VectorClock& other) const;
    
     * @param[in] other Input parameter.
     * @return True on success.
    /** @brief Return true when clocks are concurrent (no causal ordering). */
    bool isConcurrent(const VectorClock& other) const;
    
     * @return Return value.
    /** @brief Serialize to stable comma-separated node:counter string. */
    std::string serialize() const;
    
     * @param[in] data Input parameter.
     * @return Return value.
    /** @brief Parse vector clock from serialized node:counter representation. */
    static std::optional<VectorClock> deserialize(const std::string& data);
    
    /** @brief Return full node->counter timestamp map. */
    const std::map<std::string, uint64_t>& getTimestamps() const { 
        return timestamps_; 
    }

private:
    std::map<std::string, uint64_t> timestamps_;
};

/** @brief Versioned value annotated with vector clock and origin metadata. */
struct VersionedEntry {
    /** @brief Value payload for the key. */
    std::string data;
    /** @brief Vector-clock version associated with the value. */
    VectorClock version;
    /** @brief Origin node that created this version. */
    std::string node_id;  // Node that created this version
    /** @brief Wall-clock timestamp used by LWW fallback policies. */
    std::chrono::system_clock::time_point timestamp;
    
     * @return Return value.
    /** @brief Serialize versioned entry for transport/storage. */
    std::string serialize() const;
     * @param[in] data Input parameter.
     * @return Return value.
    /** @brief Parse versioned entry from serialized representation. */
    static std::optional<VersionedEntry> deserialize(const std::string& data);
};

/** @brief Policy used to resolve divergent concurrent versions. */
enum class ConflictResolutionStrategy {
    LAST_WRITE_WINS,        // Use timestamp
    VECTOR_CLOCK_ORDERING,  // Use causality
    HIGHEST_NODE_ID,        // Deterministic by node ID
    MANUAL                  // Requires manual resolution
};

/** @brief Conflict payload containing competing versions and resolution metadata. */
struct VersionConflict {
    /** @brief Key for which conflicting versions were observed. */
    std::string key;
    /** @brief Concurrent versions requiring reconciliation. */
    std::vector<VersionedEntry> conflicting_versions;
    /** @brief Strategy selected for automatic/manual resolution. */
    ConflictResolutionStrategy resolution_strategy;
    /** @brief Resolved winner when already decided. */
    std::optional<VersionedEntry> resolved_version;
    /** @brief True when manual operator resolution is required. */
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
    /** @brief Callback type used for custom/manual conflict resolution. */
    using ConflictCallback = std::function<VersionedEntry(const VersionConflict&)>;
    
    /** @brief Runtime consistency/conflict policy configuration. */
    struct Config {
        /** @brief Default conflict resolution strategy. */
        ConflictResolutionStrategy default_strategy{ConflictResolutionStrategy::LAST_WRITE_WINS};
        /** @brief Enable automatic conflict resolution when possible. */
        bool auto_resolve_conflicts{true};
        /** @brief Track and merge vector-clock causality metadata. */
        bool track_causality{true};
        /** @brief Maximum retained version-history entries per key. */
        uint32_t max_version_history{100};
    };
    
     * @param[in] config Input parameter.
     * @return Return value.
    /** @brief Construct replica consistency manager with runtime policy config. */
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
    
     * @param[in] conflict Input parameter.
     * @param[in] resolved_entry Input parameter.
    /** @brief Resolve previously detected conflict using caller-selected winner. */
    void resolveConflict(const VersionConflict& conflict,
                        const VersionedEntry& resolved_entry);
    
     * @param[in] node_id Input parameter.
     * @return Return value.
    /** @brief Return current vector clock snapshot for node_id. */
    VectorClock getVectorClock(const std::string& node_id) const;
    
     * @param[in] node_id Input parameter.
     * @param[in] clock Input parameter.
    /** @brief Merge node's vector clock with received remote clock. */
    void updateVectorClock(const std::string& node_id, const VectorClock& clock);
    
     * @param[in] callback Input parameter.
    /** @brief Set custom callback used for resolving conflicts. */
    void setConflictCallback(ConflictCallback callback);
    
     * @param[in] key Input parameter.
     * @return Return value.
    /** @brief Return retained version history for key (possibly empty). */
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
    
    /** @brief Consistency/conflict activity counters. */
    struct Statistics {
        /** @brief Number of recorded write operations. */
        uint64_t total_writes{0};
        /** @brief Number of detected version conflicts. */
        uint64_t conflicts_detected{0};
        /** @brief Number of automatically resolved conflicts. */
        uint64_t conflicts_resolved{0};
        /** @brief Number of manually resolved conflicts. */
        uint64_t manual_resolutions{0};
        /** @brief Number of merge operations performed. */
        uint64_t merges_performed{0};
    };
    
    /** @brief Return statistics counters snapshot. */
    const Statistics& getStatistics() const { return stats_; }

private:
    // -------------------------------------------------------------------------
    // Lock hierarchy for ReplicaConsistencyManager:
    //   mutex_ (1) — single mutex covers all shared state; no nested acquisition.
    //   conflict_callback_ is protected by mutex_ on both read and write paths.
    // -------------------------------------------------------------------------

    Config config_;
    
    mutable std::mutex mutex_;
    std::map<std::string, VectorClock> node_clocks_;
    std::map<std::string, std::vector<VersionedEntry>> version_history_;
    
    Statistics stats_;
    ConflictCallback conflict_callback_;
    
     * @param[in] key Input parameter.
     * @param[in] entries Input parameter.
     * @return Return value.
    /** @brief Detect whether input entries contain concurrent conflicting versions. */
    std::optional<VersionConflict> detectConflict(
        const std::string& key,
        const std::vector<VersionedEntry>& entries);
    
     * @param[in] conflict Input parameter.
     * @return Return value.
    /** @brief Resolve conflict using callback or configured strategy. */
    VersionedEntry autoResolveConflict(const VersionConflict& conflict);
    
     * @param[in] entries Input parameter.
     * @param[in] strategy Input parameter.
     * @return Return value.
    /** @brief Select winner from entries according to conflict strategy. */
    VersionedEntry selectWinningVersion(
        const std::vector<VersionedEntry>& entries,
        ConflictResolutionStrategy strategy);
};

}  // namespace sharding
}  // namespace themisdb
