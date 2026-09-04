/**
 * @file snapshot_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <unordered_map>
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
    std::string snapshot_id = {};
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
 * Result of comparing two snapshots for the same table.
 *
 * - `added`    — rows present in `handle_b` but absent in `handle_a`.
 * - `removed`  — rows present in `handle_a` but absent in `handle_b`.
 * - `modified` — rows present in both snapshots whose `data` has changed
 *                (contains the `handle_b` version).
 */
struct SnapshotDiff {
    std::vector<VersionedDocument> added;
    std::vector<VersionedDocument> removed;
    std::vector<VersionedDocument> modified;

    bool empty() const noexcept {
        return added.empty() && removed.empty() && modified.empty();
    }

    size_t totalChanges() const noexcept {
        return added.size() + removed.size() + modified.size();
    }

    nlohmann::json toJson() const {
        auto to_json_arr = [](const std::vector<VersionedDocument>& v) {
            nlohmann::json arr = nlohmann::json::array();
            for (const auto& doc : v) { arr.push_back(doc.toJson()); }
            return arr;
        };
        return {{"added",    to_json_arr(added)},
                {"removed",  to_json_arr(removed)},
                {"modified", to_json_arr(modified)}};
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

    // ── Snapshot diffing ─────────────────────────────────────────────────────

    /**
     * Result of comparing two snapshots.
     *
     * Describes the incremental difference between an older snapshot (base)
     * and a newer snapshot (other) on a per-table, per-key basis.
     */
    struct SnapshotDiff {
        /** Keys whose latest version has a different data map in @p other. */
        std::map<std::string /*table*/,
                 std::vector<std::string /*key*/>> modified;

        /** Keys present in @p other but absent from @p base. */
        std::map<std::string, std::vector<std::string>> added;

        /** Keys present in @p base but absent from @p other. */
        std::map<std::string, std::vector<std::string>> removed;

        /** Total tables examined (intersection of both snapshots). */
        size_t tables_examined{0};

        /** true when base and other are identical across all shared tables. */
        bool empty() const noexcept {
            return modified.empty() && added.empty() && removed.empty();
        }

        nlohmann::json toJson() const;
    };

    /**
     * Compute the incremental difference between two snapshots.
     *
     * Both handles must be valid (not released).  The function compares each
     * table that appears in both snapshots.  Tables that exist in only one
     * snapshot are treated as entirely added or entirely removed.
     *
     * Complexity: O(R log R) where R is the total number of rows across all
     * shared tables (sort + linear scan per table).
     *
     * @param base   Older snapshot handle.
     * @param other  Newer snapshot handle.
     * @return       SnapshotDiff; empty() == true when the snapshots are equal.
     * @throws       std::invalid_argument when either handle is invalid or
     *               refers to a released snapshot.
     */
    SnapshotDiff diff(const SnapshotHandle& base,
                      const SnapshotHandle& other) const;

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
