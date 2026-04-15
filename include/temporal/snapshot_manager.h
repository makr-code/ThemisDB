/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            snapshot_manager.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:14:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     204                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 72f3ebe873  2026-03-12  refactor(temporal): address review feedback on snapshot G... ║
    • 8098dfcd90  2026-03-12  feat(temporal): implement snapshot isolation - versioning... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
#include <functional>
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
 *
 * version_number is a monotonically increasing counter assigned at creation
 * time and can be used to impose a total order on snapshots.
 */
struct SnapshotHandle {
    std::string snapshot_id;
    Timestamp creation_time{0};
    std::vector<std::string> included_tables;
    uint64_t version_number{0};

    bool operator<(const SnapshotHandle& other) const noexcept {
        return version_number < other.version_number;
    }

    bool isValid() const noexcept { return !snapshot_id.empty(); }

    nlohmann::json toJson() const {
        return {{"snapshot_id", snapshot_id},
                {"creation_time", creation_time},
                {"included_tables", included_tables},
                {"version_number", version_number}};
    }
};

/**
 * Metadata describing a live snapshot.
 */
struct SnapshotMetadata {
    SnapshotHandle handle;
    size_t total_tables{0};
    size_t total_rows{0};
    bool is_valid{false};

    nlohmann::json toJson() const {
        return {{"handle", handle.toJson()},
                {"total_tables", total_tables},
                {"total_rows", total_rows},
                {"is_valid", is_valid}};
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
    /// Clock function type: returns the current time as a millisecond Timestamp.
    using ClockFn = std::function<Timestamp()>;

    /**
     * Construct a manager with an optional custom clock.
     *
     * @param clock  Callable that returns the current time in milliseconds
     *               since epoch.  Defaults to the module-level now().
     *               Primarily used in tests to advance time deterministically.
     */
    explicit TemporalSnapshotManager(ClockFn clock = &now);

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

    /**
     * Return metadata for a live snapshot.
     *
     * @param handle  A valid snapshot handle.
     * @return        Metadata, or a default-constructed (is_valid=false) struct
     *                if the snapshot does not exist.
     */
    SnapshotMetadata getSnapshotMetadata(const SnapshotHandle& handle) const;

    /**
     * Garbage-collect snapshots whose creation time is older than
     * (clock() - max_age_ms).  Snapshots whose age exceeds the threshold are
     * released automatically.
     *
     * @param max_age_ms  Maximum allowed age in milliseconds.  Pass 0 to
     *                    skip TTL-based collection.
     * @return            Number of snapshots removed.
     */
    size_t garbageCollectByAge(Timestamp max_age_ms);

    /**
     * Garbage-collect snapshots exceeding a maximum count.  The oldest
     * snapshots (by version_number) are removed first until at most
     * max_snapshots remain.
     *
     * @param max_snapshots  Maximum number of snapshots to keep.
     * @return               Number of snapshots removed.
     */
    size_t garbageCollectByCount(size_t max_snapshots);

    nlohmann::json getStatistics() const;

private:
    struct SnapshotData {
        SnapshotHandle handle;
        // Frozen copy of each table's scan result at creation time
        std::map<std::string, std::vector<VersionedDocument>> tables;
    };

    ClockFn clock_;
    std::map<std::string, SnapshotData> snapshots_; // keyed by snapshot_id
    mutable std::mutex mutex_;
    uint64_t next_version_{1};      // monotonically increasing snapshot version
    size_t total_created_{0};
    size_t total_released_{0};
    size_t total_gc_collected_{0};

    static std::string generateSnapshotId();
};

} // namespace temporal
} // namespace themisdb
