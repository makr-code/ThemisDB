/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            snapshot_manager.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     140                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Snapshot Manager
 *
 * Creates and manages consistent point-in-time snapshots of
 * SystemVersionedTable collections for snapshot-isolation reads.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "temporal/temporal_types.h"
#include "temporal/system_versioned_table.h"
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace temporal {

/**
 * An opaque handle that identifies a snapshot.
 */
struct SnapshotHandle {
    std::string snapshot_id;
    Timestamp creation_time{0};
    std::vector<std::string> included_tables;

    bool operator<(const SnapshotHandle& other) const noexcept {
        return creation_time < other.creation_time;
    }

    bool isValid() const noexcept { return !snapshot_id.empty(); }

    nlohmann::json toJson() const {
        return {{"snapshot_id", snapshot_id},
                {"creation_time", creation_time},
                {"included_tables", included_tables}};
    }
};

/**
 * TemporalSnapshotManager
 *
 * Captures the state of one or more SystemVersionedTable instances at the
 * moment createSnapshot() is called.  Subsequent reads via the handle are
 * isolated from concurrent writes; the snapshot is a deep copy.
 *
 * Snapshots must be explicitly released (releaseSnapshot) to free memory.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class TemporalSnapshotManager {
public:
    TemporalSnapshotManager() = default;

    /**
     * Create a snapshot of the given tables at the current time.
     *
     * @param tables  Map of table_name → reference to the live table.
     *                The snapshot captures the tables' state atomically
     *                under a common timestamp.
     * @return        Handle to the new snapshot.
     */
    SnapshotHandle createSnapshot(
        const std::map<std::string, const SystemVersionedTable*>& tables);

    /**
     * Query a snapshot for rows that were current at its creation time.
     *
     * @param handle      Snapshot handle (must be valid and not released).
     * @param table_name  Name of the table to query within the snapshot.
     * @param filters     Optional field-level filters applied to rows.
     * @return            Matching rows, or empty if the snapshot/table is
     *                    invalid.
     */
    std::vector<VersionedDocument> querySnapshot(
        const SnapshotHandle& handle,
        const std::string& table_name,
        const std::vector<std::pair<std::string, nlohmann::json>>& filters = {})
        const;

    /** Release a snapshot and free its resources. */
    bool releaseSnapshot(const SnapshotHandle& handle);

    /** Return true if the snapshot handle is still alive. */
    bool isAlive(const SnapshotHandle& handle) const;

    /** Number of live snapshots. */
    size_t snapshotCount() const;

    nlohmann::json getStatistics() const;

private:
    struct SnapshotData {
        SnapshotHandle handle;
        // Frozen copy of each table's scan result at creation time
        std::map<std::string, std::vector<VersionedDocument>> tables;
    };

    std::map<std::string, SnapshotData> snapshots_; // keyed by snapshot_id
    mutable std::mutex mutex_;
    size_t total_created_{0};
    size_t total_released_{0};

    static std::string generateSnapshotId();
};

} // namespace temporal
} // namespace themisdb
