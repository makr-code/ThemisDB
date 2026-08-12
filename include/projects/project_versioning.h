/**
 * @file project_versioning.h
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
#include <optional>
#include <variant>
#include <shared_mutex>
#include <nlohmann/json.hpp>
#include "projects/DocumentManager/document_manager.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace projects {

using json = nlohmann::json;

/// Opaque snapshot identifier (prefixed with "snap:")
using SnapshotId = std::string;

/// SHA-256 hex digest string (64 hex chars)
using Sha256Digest = std::string;

/**
 * @brief Immutable metadata for a project snapshot.
 *
 * Snapshots are content-addressed: the @p checksum field is the SHA-256
 * digest of the serialised snapshot content.  It is computed at creation
 * time and verified on every restore call.  Snapshots are never mutated
 * after creation.
 */
struct SnapshotMeta {
    SnapshotId  id;              ///< "snap:<uuid>"
    std::string project_id;      ///< Owning project UUID
    std::string description;     ///< Human-readable label
    int64_t     created_at = 0;  ///< Unix timestamp (seconds)
    Sha256Digest checksum;       ///< SHA-256 of snapshot content
    size_t      document_count = 0; ///< Number of documents captured
    json        metadata;        ///< Arbitrary user metadata

    json toJson() const;
    static SnapshotMeta fromJson(const json& j);
};

/**
 * @brief Manages point-in-time, immutable project snapshots.
 *
 * Snapshots are append-only: once created they are never modified.
 * Integrity is guaranteed by a SHA-256 content checksum that is verified
 * automatically on @c restoreSnapshot().
 *
 * All methods are thread-safe (protected by an internal shared_mutex).
 *
 * Key RocksDB layout
 * ──────────────────
 *  snap:<uuid>              → SnapshotMeta JSON
 *  snap_idx:<project_id>:<uuid> → "" (index for listSnapshots)
 *  snap_data:<uuid>:<doc_id> → DocumentMeta JSON (snapshot content)
 */
class ProjectVersioning {
public:
    explicit ProjectVersioning(std::shared_ptr<RocksDBWrapper> storage);
    ~ProjectVersioning() = default;

    /**
     * @brief Create an immutable snapshot of the given project.
     *
     * Captures the current list of document metadata records stored under
     * the project and computes a SHA-256 content checksum over the
     * serialised payload.
     *
     * @param project_id   Project UUID whose state is to be captured.
     * @param description  Optional human-readable label for the snapshot.
     * @param metadata     Optional arbitrary metadata stored with the snapshot.
     * @return New SnapshotId on success, or a Status{false, …} on failure.
     */
    std::variant<SnapshotId, Status> createSnapshot(
        const std::string& project_id,
        const std::string& description = {},
        const json& metadata = json::object()
    );

    /**
     * @brief Retrieve snapshot metadata by ID.
     * @return SnapshotMeta if found, std::nullopt otherwise.
     */
    std::optional<SnapshotMeta> getSnapshot(const SnapshotId& snap_id) const;

    /**
     * @brief List all snapshots belonging to a project, newest first.
     */
    std::vector<SnapshotMeta> listSnapshots(const std::string& project_id) const;

    /**
     * @brief Delete a snapshot and all associated data.
     *
     * This is an irreversible operation.  Active snapshots referenced by
     * an in-progress restore cannot be deleted and will return an error.
     */
    Status deleteSnapshot(const SnapshotId& snap_id);

    /**
     * @brief Restore a project to the state captured in a snapshot.
     *
     * The content checksum is verified before any data is written to
     * @p target_project_id.  On checksum mismatch the restore is aborted
     * and an error Status is returned.
     *
     * @param snap_id           Snapshot to restore from.
     * @param target_project_id Destination project UUID (may be same as source).
     */
    Status restoreSnapshot(
        const SnapshotId& snap_id,
        const std::string& target_project_id
    );

    /**
     * @brief Verify snapshot integrity (checksum re-computation).
     * @return true if the stored checksum matches the current content.
     */
    bool verifySnapshot(const SnapshotId& snap_id) const;

private:
    std::shared_ptr<RocksDBWrapper>   storage_;
    mutable std::shared_mutex         mutex_;

    std::string generateUuid() const;
    Sha256Digest computeChecksum(const std::string& data) const;

    /// Collect all doc keys for a project from storage
    std::vector<std::string> collectProjectDocKeys(
        const std::string& project_id) const;
};

} // namespace projects
} // namespace themis

