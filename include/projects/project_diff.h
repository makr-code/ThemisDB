/**
 * @file project_diff.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "projects/project_metrics.h"
#include "projects/project_versioning.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace projects {

using json = nlohmann::json;

// ─── Delta types ─────────────────────────────────────────────────────────────

/**
 * @brief Kind of change recorded in a DeltaEntry.
 */
enum class DeltaType {
    ADDED,    ///< Field / document was not present in `from` but is in `to`
    REMOVED,  ///< Field / document was present in `from` but not in `to`
    MODIFIED, ///< Field / document exists in both but has a different value
};

/**
 * @brief A single field-level change between two project states.
 *
 * @c field_path follows JSON Pointer syntax (RFC 6901), e.g.
 * @c "/metadata/title".  For top-level document additions/removals the
 * path is the document key (e.g. @c "/doc:<uuid>").
 */
struct DeltaEntry {
    std::string field_path; ///< JSON Pointer path of the changed field
    DeltaType   type;       ///< Type of change
    json        old_value;  ///< Previous value (null for ADDED entries)
    json        new_value;  ///< New value      (null for REMOVED entries)

    json toJson() const;
    static DeltaEntry fromJson(const json& j);
};

/**
 * @brief Ordered collection of field-level changes between two project states.
 *
 * DeltaSet is the canonical result type of all diff operations and the
 * input to merge operations.  It is always structured (typed field-level
 * deltas), never a raw text diff.
 */
struct DeltaSet {
    std::vector<DeltaEntry> entries;

    /// Number of individual field changes.
    size_t totalChanges() const noexcept { return entries.size(); }

    /// True when no differences were found.
    bool empty() const noexcept { return entries.empty(); }

    json toJson() const;
    static DeltaSet fromJson(const json& j);
};

// ─── Merge result ─────────────────────────────────────────────────────────────

/**
 * @brief Result of a three-way project merge.
 */
struct MergeResult {
    bool     ok = false;       ///< True when merge completed without conflicts
    DeltaSet applied;          ///< Changes successfully applied
    DeltaSet conflicts;        ///< Deltas that could not be auto-resolved
    std::string message;       ///< Human-readable status / error description
};

// ─── ProjectDiff ─────────────────────────────────────────────────────────────

/**
 * @brief Computes structured, field-level diffs between project snapshots.
 *
 * All diff operations return a @c DeltaSet — never a raw text diff.  The
 * comparison is performed at document-metadata level: each
 * @c DocumentMeta field is compared independently, and missing documents
 * appear as ADDED / REMOVED top-level entries.
 *
 * @note @c ProjectDiff is stateless and all methods are thread-safe.
 */
class ProjectDiff {
public:
    explicit ProjectDiff(std::shared_ptr<RocksDBWrapper> storage);

    /**
     * @brief Compute the delta between two existing snapshots.
     *
     * @param from_snap  Baseline snapshot identifier.
     * @param to_snap    Target snapshot identifier.
     * @return Structured field-level DeltaSet.
     */
    DeltaSet diff(
        const SnapshotId& from_snap,
        const SnapshotId& to_snap
    ) const;

    /**
     * @brief Compute the delta between two arbitrary JSON documents.
     *
     * Useful for one-off comparisons without creating persistent snapshots.
     * Both @p from and @p to are treated as JSON objects; nested fields are
     * compared recursively.
     */
    DeltaSet diffDocuments(const json& from, const json& to) const;

    /**
     * @brief Inject a metrics sink.
     *
     * When set, every `diff()` call records its wall-clock latency via
     * `ProjectMetrics::recordDiff()`.  Pass `nullptr` to disable.
     * Thread-safe.
     */
    void setMetrics(std::shared_ptr<ProjectMetrics> metrics);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<ProjectMetrics> metrics_; // guarded by metrics_mutex_
    mutable std::mutex metrics_mutex_;

    /// Recursively compare two JSON values, accumulating DeltaEntry records.
    void diffRecursive(
        const std::string& path,
        const json& from,
        const json& to,
        std::vector<DeltaEntry>& out
    ) const;
};

// ─── ProjectMerge ─────────────────────────────────────────────────────────────

/**
 * @brief Three-way project merge using a common ancestor snapshot.
 *
 * The merge algorithm:
 *  1. Compute ours_delta   = diff(ancestor, ours)
 *  2. Compute theirs_delta = diff(ancestor, theirs)
 *  3. Apply non-conflicting deltas from theirs onto ours.
 *  4. Report as conflicts any delta that modifies a field already changed
 *     by ours (same field_path, different value).
 *
 * The merge never writes directly to any snapshot; callers are
 * responsible for committing the resulting state.
 */
class ProjectMerge {
public:
    explicit ProjectMerge(std::shared_ptr<RocksDBWrapper> storage);

    /**
     * @brief Perform a three-way merge.
     *
     * @param ancestor_snap  Common ancestor snapshot.
     * @param ours_snap      Our current state snapshot.
     * @param theirs_snap    Incoming state snapshot.
     * @return MergeResult with applied deltas and any unresolved conflicts.
     */
    MergeResult merge(
        const SnapshotId& ancestor_snap,
        const SnapshotId& ours_snap,
        const SnapshotId& theirs_snap
    ) const;

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    ProjectDiff                     diff_;
};

} // namespace projects
} // namespace themis

