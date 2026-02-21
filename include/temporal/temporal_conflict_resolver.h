/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            temporal_conflict_resolver.h                       ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

} // namespace temporal
} // namespace themisdb
