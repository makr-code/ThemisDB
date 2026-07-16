/// @file manifest_store.h
/// @brief Manifest persistence and atomic update store for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines the manifest store for persistent, atomic management of
/// artifact manifests. It provides compare-and-swap semantics for atomic updates,
/// manifest versioning for history tracking, and integration with RocksDB.
///
/// ## Design Philosophy
///
/// The manifest store is:
/// - **Atomic**: Manifest updates are ACID-compliant via CAS operations
/// - **Durable**: All manifests are persisted to RocksDB by default
/// - **Versioned**: Full history of manifest changes is maintained
/// - **Transactional**: Supports batch updates across multiple manifests
/// - **Observable**: Manifest history is queryable for audit and debugging
///
/// ## Thread Safety
///
/// ManifestStore is thread-safe for concurrent reads and writes:
/// - Reads can proceed concurrently with other reads
/// - Writes use internal synchronization (lock-free where possible)
/// - CAS operations are atomic and return success/failure
///

#pragma once

#include "src/distributed_tensor/include/artifact_manifest.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <map>
#include <cstdint>

namespace themis {
namespace distributed_tensor {

/// @brief Result of a manifest store operation.
enum class ManifestStoreStatus : uint8_t {
  /// Operation succeeded.
  OK = 0,

  /// Manifest not found in store.
  NOT_FOUND = 1,

  /// Compare-and-swap failed (manifest was modified).
  CAS_FAILED = 2,

  /// Invalid manifest (failed validation).
  INVALID_MANIFEST = 3,

  /// Storage error (RocksDB failure).
  STORAGE_ERROR = 4,

  /// Version mismatch (requested version does not exist).
  VERSION_MISMATCH = 5,

  /// Artifact is locked for exclusive update.
  ARTIFACT_LOCKED = 6,

  /// Manifest versioning exceeded maximum history.
  VERSION_LIMIT_EXCEEDED = 7,

  /// Unknown error.
  UNKNOWN_ERROR = 8,
};

/// @brief Version metadata for a single manifest snapshot.
struct ManifestVersion {
  /// Unique version ID (monotonically increasing per artifact).
  uint64_t version_id = 0;

  /// Timestamp when this version was created (seconds since epoch).
  int64_t created_at_unix_sec = 0;

  /// The manifest state at this version.
  ArtifactManifest manifest;

  /// Optional reason for the version change.
  std::string change_reason;

  /// Optional operator/system that triggered this version.
  std::string changed_by;
};

/// @brief ManifestStore: Atomic, durable storage and coordination for artifact manifests.
///
/// Provides:
/// - Atomic read/write operations with CAS semantics
/// - Manifest versioning for full audit trails
/// - RocksDB persistence for durability
/// - Batch operations for coordinated multi-artifact updates
/// - Exclusive locking for long-running updates
///
class ManifestStore {
 public:
  ManifestStore() = default;
  virtual ~ManifestStore() = default;

  // Prevent copy/move to maintain invariants
  ManifestStore(const ManifestStore&) = delete;
  ManifestStore& operator=(const ManifestStore&) = delete;
  ManifestStore(ManifestStore&&) = delete;
  ManifestStore& operator=(ManifestStore&&) = delete;

  /// Opens/initializes the manifest store with RocksDB backend.
  /// @param db_path Path to RocksDB database directory
  /// @return OK on success, STORAGE_ERROR on failure
  virtual ManifestStoreStatus open(const std::string& db_path);

  /// Closes the manifest store and flushes all data.
  /// @return OK on success, STORAGE_ERROR on failure
  virtual ManifestStoreStatus close();

  /// Reads a manifest from the store (latest version).
  /// @param artifact_id Artifact identifier
  /// @param[out] manifest The manifest to populate
  /// @return OK on success, NOT_FOUND if artifact doesn't exist
  virtual ManifestStoreStatus read(const std::string& artifact_id, ArtifactManifest& manifest);

  /// Reads a specific version of a manifest from history.
  /// @param artifact_id Artifact identifier
  /// @param version_id Version to retrieve (0 = latest)
  /// @param[out] manifest The manifest to populate
  /// @return OK on success, NOT_FOUND if version doesn't exist
  virtual ManifestStoreStatus readVersion(const std::string& artifact_id,
                                          uint64_t version_id,
                                          ArtifactManifest& manifest);

  /// Writes a manifest atomically using compare-and-swap semantics.
  ///
  /// This operation is atomic: either the manifest is updated and a new version
  /// created, or the operation fails if the manifest was modified. This prevents
  /// lost updates.
  ///
  /// @param artifact_id Artifact identifier
  /// @param manifest The manifest to write
  /// @param expected_version Expected current version (for CAS); 0 = any version OK
  /// @param change_reason Optional reason for this update (for audit trail)
  /// @param changed_by Optional identifier of who/what triggered this change
  /// @return OK on successful update, CAS_FAILED if version mismatch
  virtual ManifestStoreStatus write(const std::string& artifact_id,
                                    const ArtifactManifest& manifest,
                                    uint64_t expected_version = 0,
                                    const std::string& change_reason = "",
                                    const std::string& changed_by = "");

  /// Atomically swaps the manifest for an artifact (publish/swap pattern).
  ///
  /// Used by the update worker to atomically replace a stale artifact manifest
  /// with a newly rebuilt one.
  ///
  /// @param artifact_id Artifact identifier
  /// @param new_manifest The new manifest to publish
  /// @param current_manifest_version Expected current version (for safety check)
  /// @param reason Reason for the swap (e.g., "rebuilt", "patched")
  /// @return OK on success, CAS_FAILED if current version doesn't match
  virtual ManifestStoreStatus swapManifest(const std::string& artifact_id,
                                           const ArtifactManifest& new_manifest,
                                           uint64_t current_manifest_version,
                                           const std::string& reason = "");

  /// Invalidates a manifest (marks it stale and triggers rebuild).
  /// @param artifact_id Artifact identifier
  /// @param reason Why the artifact is being invalidated
  /// @return OK on success, NOT_FOUND if artifact doesn't exist
  virtual ManifestStoreStatus invalidate(const std::string& artifact_id,
                                         InvalidationReason reason);

  /// Acquires an exclusive lock on a manifest for long-running updates.
  ///
  /// This is used by the update worker to prevent concurrent modifications
  /// while performing a long rebuild operation.
  ///
  /// @param artifact_id Artifact identifier
  /// @param lock_holder Identifier of the lock holder (for diagnostics)
  /// @param timeout_ms Maximum time to hold the lock (0 = no timeout)
  /// @return OK on success, ARTIFACT_LOCKED if already locked
  virtual ManifestStoreStatus acquireLock(const std::string& artifact_id,
                                          const std::string& lock_holder,
                                          int64_t timeout_ms = 0);

  /// Releases an exclusive lock on a manifest.
  /// @param artifact_id Artifact identifier
  /// @param lock_holder Expected lock holder (for safety)
  /// @return OK on success, ARTIFACT_LOCKED if held by different owner
  virtual ManifestStoreStatus releaseLock(const std::string& artifact_id,
                                          const std::string& lock_holder);

  /// Checks if an artifact is currently locked.
  /// @param artifact_id Artifact identifier
  /// @return true if locked, false otherwise
  virtual bool isLocked(const std::string& artifact_id) const;

  /// Retrieves the current version number for an artifact.
  /// @param artifact_id Artifact identifier
  /// @return Version number on success, 0 if artifact doesn't exist
  virtual uint64_t getCurrentVersion(const std::string& artifact_id) const;

  /// Retrieves the full version history of a manifest.
  /// @param artifact_id Artifact identifier
  /// @return Vector of ManifestVersion entries (newest last)
  virtual std::vector<ManifestVersion> getVersionHistory(const std::string& artifact_id);

  /// Deletes a manifest and all its version history (permanent deletion).
  /// @param artifact_id Artifact identifier
  /// @return OK on success, NOT_FOUND if artifact doesn't exist
  virtual ManifestStoreStatus deleteManifest(const std::string& artifact_id);

  /// Returns manifest statistics for observability.
  struct Stats {
    uint64_t total_artifacts = 0;
    uint64_t total_versions = 0;
    uint64_t locked_artifacts = 0;
    uint64_t recent_writes = 0;
    int64_t storage_size_bytes = 0;
  };

  /// Returns current store statistics.
  virtual Stats getStats() const;

 protected:
  // Internal implementation details (may be overridden by subclasses for testing)

  /// Internal storage implementation (can be mocked for testing).
  virtual ManifestStoreStatus internalRead(const std::string& artifact_id,
                                           uint64_t version_id,
                                           ArtifactManifest& manifest);

  virtual ManifestStoreStatus internalWrite(const std::string& artifact_id,
                                            const ArtifactManifest& manifest,
                                            const ManifestVersion& version_info);

  // Protected members
  std::string db_path_;
  bool is_open_ = false;
};

}  // namespace distributed_tensor
}  // namespace themis
