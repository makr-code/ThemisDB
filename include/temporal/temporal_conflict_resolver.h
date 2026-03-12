/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_conflict_resolver.h                       ║
  Version:         0.0.34                                             ║
  Last Modified:   2026-03-09 03:55:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Conflict Resolver
 * 
 * Resolves conflicts between temporal snapshots using HLC timestamps
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#include "replication/multi_master_replication.h"

namespace themisdb {
namespace temporal {

/**
 * Conflict Resolution Policy
 */
enum class ConflictPolicy {
    LAST_WRITE_WINS,      // Default: Highest HLC wins
    FIRST_WRITE_WINS,     // Lowest HLC wins
    NODE_PRIORITY,        // Configured Node-Priority (e.g. Node ID Tiebreaker)
    MANUAL,               // Conflict in Queue for manual resolution
    CRDT_MERGE            // Automatic Merge via CRDT (if supported)
};

/**
 * Temporal Snapshot with HLC versioning
 */
struct TemporalSnapshot {
    std::string snapshot_id;
    replication::HybridLogicalClock::Timestamp hlc;
    std::string source_node_id;
    nlohmann::json data;
    std::string checksum;  // SHA-256
    
    nlohmann::json toJson() const;
    static std::optional<TemporalSnapshot> fromJson(const nlohmann::json& j);
};

/**
 * Conflict Record for Logging/Monitoring
 */
struct ConflictRecord {
    std::string conflict_id;
    std::string entity_id;
    TemporalSnapshot local_version;
    TemporalSnapshot remote_version;
    ConflictPolicy resolution_policy;
    std::string winner;  // "local" | "remote" | "merged"
    std::chrono::system_clock::time_point detected_at;
    bool resolved;
};

/**
 * Temporal Conflict Resolver
 * 
 * Resolves conflicts for temporal snapshots based on HLC timestamps.
 */
class TemporalConflictResolver {
public:
    explicit TemporalConflictResolver(ConflictPolicy default_policy = ConflictPolicy::LAST_WRITE_WINS);
    
    /**
     * Resolve conflict between local and remote snapshot
     * 
     * @param local Local snapshot version
     * @param remote Remote snapshot version
     * @param policy Override default policy (optional)
     * @return Winning snapshot (merged or selected)
     */
    TemporalSnapshot resolve(
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote,
        std::optional<ConflictPolicy> policy = std::nullopt
    );
    
    /**
     * Get all unresolved conflicts (for MANUAL policy)
     */
    std::vector<ConflictRecord> getUnresolvedConflicts() const;
    
    /**
     * Manually resolve a conflict
     */
    void resolveManually(const std::string& conflict_id, const std::string& winner);
    
    /**
     * Get the complete conflict history (resolved + unresolved).
     * Useful for audit, compliance and replay.
     */
    std::vector<ConflictRecord> getConflictHistory() const;

    /**
     * Export the complete conflict history as a JSON array.
     * Each entry contains: conflict_id, entity_id, winner, policy, resolved,
     * detected_at_ms.
     */
    nlohmann::json exportAuditLog() const;

    /**
     * Get conflict statistics
     */
    nlohmann::json getStatistics() const;
    
private:
    ConflictPolicy default_policy_;
    std::vector<ConflictRecord> conflict_history_;
    std::map<std::string, ConflictRecord> unresolved_conflicts_;
    mutable std::mutex mutex_;
    
    // Statistics
    std::atomic<uint64_t> total_conflicts_{0};
    std::atomic<uint64_t> lww_resolutions_{0};
    std::atomic<uint64_t> fww_resolutions_{0};
    std::atomic<uint64_t> manual_resolutions_{0};
    std::atomic<uint64_t> crdt_merges_{0};
    
    std::string generateConflictId() const;
    
    TemporalSnapshot resolveLastWriteWins(const TemporalSnapshot& local, const TemporalSnapshot& remote);
    TemporalSnapshot resolveFirstWriteWins(const TemporalSnapshot& local, const TemporalSnapshot& remote);
    TemporalSnapshot resolveNodePriority(const TemporalSnapshot& local, const TemporalSnapshot& remote);
    TemporalSnapshot resolveCRDT(const TemporalSnapshot& local, const TemporalSnapshot& remote);
};

/**
 * Conflict type classification for temporal conflict detection.
 */
enum class ConflictType {
    CONCURRENT_UPDATE,      ///< Two writes with concurrent (non-causal) HLC timestamps
    OVERLAPPING_PERIODS,    ///< Valid-time periods of two versions overlap for the same entity
    REFERENTIAL_INTEGRITY,  ///< A data reference in one version does not match the other
    UNIQUENESS_VIOLATION    ///< Same entity claimed by both versions with divergent data
};

/**
 * A detected temporal conflict between two snapshot versions.
 */
struct Conflict {
    ConflictType type;
    std::string entity_id;
    TemporalSnapshot local_version;
    TemporalSnapshot remote_version;
    std::vector<std::string> affected_columns; ///< Data fields involved in the conflict
};

/**
 * TemporalConflictDetector
 *
 * Detects conflicts between two temporal snapshots of the same entity.
 *
 * Detection logic per ConflictType:
 *   CONCURRENT_UPDATE     – HLC timestamps are concurrent (neither happened-before
 *                           the other) and the data differs.
 *   OVERLAPPING_PERIODS   – Both snapshots carry a valid_time range in their JSON
 *                           payload (keys "valid_start"/"valid_end") and those ranges
 *                           overlap while the data diverges.
 *   REFERENTIAL_INTEGRITY – A "ref_entity_id" field present in one snapshot's data
 *                           is absent or different in the other.
 *   UNIQUENESS_VIOLATION  – Both snapshots share the same entity_id but carry
 *                           different data values for any common key.
 *
 * Auto-resolution delegates to a TemporalConflictResolver with the chosen policy.
 * Conflicts queued for manual resolution are held in an in-memory queue and can
 * be retrieved via getQueuedConflicts().
 *
 * Thread-safety: all public methods are thread-safe.
 */
class TemporalConflictDetector {
public:
    TemporalConflictDetector() = default;

    /**
     * Detect all conflicts between @p local and @p remote for @p table_name.
     *
     * @return A (possibly empty) list of detected Conflict objects. An empty list
     *         means the two snapshots are compatible.
     */
    std::vector<Conflict> detectConflicts(
        const std::string& table_name,
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );

    /**
     * Automatically resolve @p conflict using @p policy.
     *
     * @return The winning snapshot, or std::nullopt if the policy is MANUAL
     *         (use queueForManualResolution instead).
     */
    std::optional<TemporalSnapshot> autoResolveConflict(
        const Conflict& conflict,
        ConflictPolicy policy
    );

    /**
     * Queue @p conflict for manual resolution.
     *
     * @return true if the conflict was queued; false if an identical conflict_id
     *         is already in the queue.
     */
    bool queueForManualResolution(const Conflict& conflict);

    /**
     * Return a snapshot of all currently queued conflicts.
     */
    std::vector<Conflict> getQueuedConflicts() const;

    /**
     * Remove all entries from the manual-resolution queue.
     */
    void clearQueue();

private:
    mutable std::mutex queue_mutex_;
    /// Key: generated conflict key ("table|entity_id|type"), Value: Conflict
    std::map<std::string, Conflict> manual_queue_;

    /// Helper: generate a deterministic queue key for a conflict
    static std::string makeQueueKey(const std::string& table_name,
                                    const Conflict& conflict);

    /// Sub-detectors — each returns an optional Conflict if detected
    static std::optional<Conflict> detectConcurrentUpdate(
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );

    static std::optional<Conflict> detectOverlappingPeriods(
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );

    static std::optional<Conflict> detectReferentialIntegrity(
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );

    static std::optional<Conflict> detectUniquenessViolation(
        const TemporalSnapshot& local,
        const TemporalSnapshot& remote
    );
};

} // namespace temporal
} // namespace themisdb
