/**
 * @file artifact_manifest.h
 * @brief Manifest schema and coordination for distributed tensor artifacts.
 *
 * The manifest is the single source of truth describing which stripes of a
 * sharded artifact are stored on which shards, their checksums, and any
 * pending reconciliation work.
 *
 * Planned in: docs/EPIC3_MANIFEST_SCHEMA.md
 * Sub-issue:   #5430
 */

#pragma once

#include "tensor_artifact_classes.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::distributed_tensor {

/// Status of a manifest entry.
enum class ManifestEntryStatus {
    Committed,    ///< All stripes confirmed written
    Pending,      ///< Write in progress
    PartialLoss,  ///< One or more stripes missing
    Corrupt,      ///< Checksum mismatch detected
    Rebuilding,   ///< Recovery in progress
};

/// A single manifest entry for one artifact.
struct ManifestEntry {
    std::string          artifact_id;
    ManifestEntryStatus  status = ManifestEntryStatus::Pending;
    std::vector<ShardStripe> stripes;
    std::string          global_checksum; ///< XOR of stripe checksums
    std::uint64_t        schema_version   = 1;
    std::chrono::system_clock::time_point committed_at;
    std::string          coordinator_node; ///< Node that committed this entry
};

/// A full manifest snapshot (used for persistence and replication).
struct ManifestSnapshot {
    std::string  snapshot_id;
    std::uint64_t epoch       = 0;
    std::vector<ManifestEntry> entries;
    std::chrono::system_clock::time_point taken_at;
};

/**
 * @brief Manifest store interface.
 *
 * Manages the lifecycle of manifest entries across distributed nodes.
 */
class IManifestStore {
public:
    virtual ~IManifestStore() = default;

    /// Commit a new or updated manifest entry.
    virtual bool commit(ManifestEntry entry) = 0;

    /// Look up the manifest entry for an artifact.
    virtual std::optional<ManifestEntry> lookup(
        const std::string& artifact_id) const = 0;

    /// Mark an entry as having partial loss (triggers recovery).
    virtual bool reportPartialLoss(const std::string& artifact_id,
                                    const std::vector<std::string>& missing_shard_keys) = 0;

    /// Update the status of an existing entry.
    virtual bool updateStatus(const std::string& artifact_id,
                               ManifestEntryStatus status) = 0;

    /// Take a consistent snapshot of the full manifest.
    virtual ManifestSnapshot snapshot() const = 0;

    /// Restore the manifest from a snapshot.
    virtual bool restore(const ManifestSnapshot& snap) = 0;

    /// List all artifact IDs with a given status.
    virtual std::vector<std::string> listByStatus(
        ManifestEntryStatus status) const = 0;
};

/// Factory: create an in-memory manifest store.
std::unique_ptr<IManifestStore> makeManifestStore();

} // namespace themis::distributed_tensor
