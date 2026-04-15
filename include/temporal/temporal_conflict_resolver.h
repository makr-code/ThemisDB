/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_conflict_resolver.h                       ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:09:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e1fff5135a  2026-03-12  fix(temporal): address PR review findings on TemporalConf... ║
    • 1b0158e116  2026-03-12  feat: implement TemporalConflictDetector for temporal con... ║
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
    REFERENTIAL_INTEGRITY,  ///< Both snapshots carry a "ref_entity_id" field but the values differ
    UNIQUENESS_VIOLATION    ///< Two different-origin snapshots carry divergent data for the same entity
};

/**
 * A detected temporal conflict between two snapshot versions.
 *
 * `table_name` and `entity_id` together identify what conflicted:
 *   - `table_name` is the table passed to detectConflicts().
 *   - `entity_id`  is the snapshot_id of the local snapshot (set by detectConflicts()).
 *
 * Version identity is preserved in `local_version.snapshot_id` and
 * `remote_version.snapshot_id`.
 */
struct Conflict {
    ConflictType type;
    std::string table_name;  ///< Table in which the conflict was detected
    std::string entity_id;   ///< snapshot_id of the local snapshot (version identity)
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
 *   CONCURRENT_UPDATE     – Neither HLC happened-before the other (concurrent writes)
 *                           and the snapshot data differs.
 *   OVERLAPPING_PERIODS   – Both snapshots carry integer "valid_start"/"valid_end"
 *                           fields (half-open [start, end) intervals) that overlap
 *                           while the data diverges.  Fields with non-integer values
 *                           are treated as "no period information present".
 *   REFERENTIAL_INTEGRITY – Both snapshots carry a "ref_entity_id" field but the
 *                           values differ.  If either snapshot is missing the field,
 *                           no conflict is raised.
 *   UNIQUENESS_VIOLATION  – Snapshots from different source nodes carry different
 *                           data.  affected_columns contains the symmetric difference
 *                           of diverging keys (keys that differ OR are present in only
 *                           one snapshot).
 *
 * Auto-resolution delegates to a TemporalConflictResolver with the chosen policy.
 * Conflicts queued for manual resolution are held in a thread-safe in-memory queue
 * and can be retrieved via getQueuedConflicts().
 *
 * Thread-safety: all public methods are thread-safe.
 */
class TemporalConflictDetector {
public:
    TemporalConflictDetector() = default;

    /**
     * Detect all conflicts between @p local and @p remote for @p table_name.
     *
     * Each returned Conflict has its `table_name` set to @p table_name and its
     * `entity_id` set to `local.snapshot_id`.
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
     * @return The winning snapshot, or std::nullopt when @p policy is MANUAL
     *         (call queueForManualResolution instead).
     */
    std::optional<TemporalSnapshot> autoResolveConflict(
        const Conflict& conflict,
        ConflictPolicy policy
    );

    /**
     * Queue @p conflict for manual resolution within @p table_name.
     *
     * The stored entry has its `table_name` field overwritten with @p table_name
     * so the queued conflict is always self-consistent.  The dedup key is derived
     * from table_name + entity_id + conflict type + both snapshot IDs, so the
     * same logical conflict is never queued twice for the same table.
     *
     * @return true if the conflict was queued; false if an identical conflict
     *         entry is already in the queue.
     */
    bool queueForManualResolution(const std::string& table_name,
                                  const Conflict& conflict);

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
    /// Key: "table_name|entity_id|type|local_snapshot_id|remote_snapshot_id"
    std::map<std::string, Conflict> manual_queue_;

    /// Helper: generate a deterministic dedup key for a conflict
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
