/**
 * @file artifact_manifest.h
 * @brief Artifact manifest for distributed tensor checkpoints.
 *
 * Defines the ArtifactManifest data model: shard descriptors, content
 * hashes, placement hints, and format versioning for distributed tensor artifacts.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/tensor_artifact_classes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @defgroup artifact_manifest Artifact Manifest Schema
/// @brief Manifest schema for distributed tensor artifact coordination.
/// @{

/// Shard placement metadata.
///
/// Describes where shards of a tensor artifact are placed in the cluster.
struct ShardPlacement {
  /// Shard identifier (unique within the artifact).
  std::string shard_id;

  /// Node or replica identifier where this shard is placed.
  std::string node_id;

  /// Tier level (e.g., "hot", "warm", "cold") for caching/retrieval priority.
  std::string tier_level = "warm";

  /// Size of this shard in bytes.
  uint64_t shard_size_bytes = 0;

  /// Content hash of this shard for integrity verification.
  std::string shard_content_hash;

  /// Replication factor: number of replicas for this shard.
  uint32_t replication_factor = 1;

  /// Erasure coding parameters (e.g., "8+4" for 8 data + 4 parity blocks).
  std::string erasure_coding_scheme;

  /// Custom placement metadata (e.g., hardware affinity, policy tags).
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Reconstruction instructions.
///
/// Specifies how to reconstruct an artifact from its components or parent artifacts.
struct ReconstructionInstruction {
  /// Type of reconstruction (e.g., "from_parent", "from_shards", "from_erasure").
  std::string reconstruction_type;

  /// Parent artifact IDs (for derived artifacts).
  std::vector<std::string> parent_artifact_ids;

  /// Shard IDs required for reconstruction.
  std::vector<std::string> required_shard_ids;

  /// Computation graph or parameters for reconstruction.
  std::string reconstruction_parameters;

  /// Estimated reconstruction time in milliseconds.
  uint64_t estimated_reconstruction_time_ms = 0;
};

/// Integrity and receipt information.
///
/// Captures integrity verification data and freshness certificates.
struct IntegrityReceipt {
  /// Merkle root hash of the entire artifact.
  std::string merkle_root_hash;

  /// Method used for integrity verification (e.g., "SHA256", "BLAKE3").
  std::string verification_method;

  /// Timestamp of last verification (ISO 8601).
  std::string verified_at;

  /// Verification status: true if all shards passed integrity checks.
  bool is_valid = false;

  /// Shard-level integrity hashes (shard_id -> hash).
  std::unordered_map<std::string, std::string> shard_hashes;
};

/// Artifact manifest structure.
///
/// The manifest is the durable coordination object for distributed tensor artifacts.
/// It captures all metadata necessary for placement, retrieval, integrity, recovery,
/// and query planning without carrying the tensor payload itself.
class ArtifactManifest {
 public:
  /// Construct a manifest for a tensor artifact.
  ///
  /// @param artifact_id Unique artifact identifier.
  /// @param artifact_class Classification of the artifact.
  explicit ArtifactManifest(const std::string& artifact_id,
                            ArtifactClass artifact_class);

  /// Copy constructor deleted to enforce explicit lifetime management.
  ArtifactManifest(const ArtifactManifest&) = delete;

  /// Move constructor.
  ArtifactManifest(ArtifactManifest&&) noexcept = default;

  /// Assignment operator deleted.
  ArtifactManifest& operator=(const ArtifactManifest&) = delete;

  /// Move assignment operator.
  ArtifactManifest& operator=(ArtifactManifest&&) noexcept = default;

  /// Virtual destructor.
  virtual ~ArtifactManifest() = default;

  // Accessors for core metadata.

  /// Return the artifact ID.
  const std::string& artifact_id() const noexcept { return artifact_id_; }

  /// Return the artifact class.
  ArtifactClass artifact_class() const noexcept { return artifact_class_; }

  /// Return the version identifier.
  const std::string& version() const noexcept { return version_; }

  /// Set the version identifier.
  void set_version(const std::string& version) noexcept { version_ = version; }

  /// Return the content hash.
  const std::string& content_hash() const noexcept { return content_hash_; }

  /// Set the content hash.
  void set_content_hash(const std::string& hash) noexcept {
    content_hash_ = hash;
  }

  /// Return the manifest hash (hash of manifest metadata).
  const std::string& manifest_hash() const noexcept { return manifest_hash_; }

  /// Compute and set the manifest hash based on current metadata.
  void compute_manifest_hash() noexcept;

  /// Return the total artifact size in bytes.
  uint64_t total_size_bytes() const noexcept { return total_size_bytes_; }

  /// Set the total artifact size.
  void set_total_size_bytes(uint64_t size) noexcept { total_size_bytes_ = size; }

  /// Return the current lifecycle stage tracked by the manifest.
  ///
  /// The lifecycle stage is used by distributed retrieval and recovery
  /// components to decide whether reads may proceed normally, in degraded
  /// mode, or must be blocked.
  ArtifactLifecycleStage lifecycle_stage() const noexcept {
    return lifecycle_stage_;
  }

  /// Set the current lifecycle stage tracked by the manifest.
  ///
  /// @param stage Lifecycle stage to record for planner and recovery checks.
  void set_lifecycle_stage(ArtifactLifecycleStage stage) noexcept {
    lifecycle_stage_ = stage;
  }

  // Shard placement management.

  /// Add a shard placement.
  ///
  /// @param placement Shard placement metadata.
  void add_shard_placement(ShardPlacement placement) noexcept;

  /// Retrieve all shard placements.
  const std::vector<ShardPlacement>& shard_placements() const noexcept {
    return shard_placements_;
  }

  /// Retrieve a specific shard placement by shard ID.
  ///
  /// @param shard_id Shard identifier.
  /// @return Optional shard placement if found.
  std::optional<ShardPlacement> get_shard_placement(
      const std::string& shard_id) const noexcept;

  /// Query the total number of shards.
  size_t num_shards() const noexcept { return shard_placements_.size(); }

  // Reconstruction and recovery management.

  /// Set the reconstruction instruction.
  ///
  /// @param instruction Reconstruction metadata.
  void set_reconstruction_instruction(
      ReconstructionInstruction instruction) noexcept {
    reconstruction_instruction_ = std::move(instruction);
  }

  /// Retrieve the reconstruction instruction.
  const std::optional<ReconstructionInstruction>&
  reconstruction_instruction() const noexcept {
    return reconstruction_instruction_;
  }

  /// Return the recovery/rebuild strategy (e.g., "replication", "erasure_coding").
  const std::string& recovery_strategy() const noexcept {
    return recovery_strategy_;
  }

  /// Set the recovery strategy.
  void set_recovery_strategy(const std::string& strategy) noexcept {
    recovery_strategy_ = strategy;
  }

  // Integrity and freshness metadata.

  /// Set the integrity receipt.
  ///
  /// @param receipt Integrity and verification metadata.
  void set_integrity_receipt(IntegrityReceipt receipt) noexcept {
    integrity_receipt_ = std::move(receipt);
  }

  /// Retrieve the integrity receipt.
  const std::optional<IntegrityReceipt>& integrity_receipt() const noexcept {
    return integrity_receipt_;
  }

  /// Return the freshness timestamp (ISO 8601).
  const std::string& freshness_timestamp() const noexcept {
    return freshness_timestamp_;
  }

  /// Update the freshness timestamp.
  void update_freshness() noexcept;

  // Provenance and lineage.

  /// Return the provenance origin (e.g., training job ID).
  const std::string& provenance_origin() const noexcept {
    return provenance_origin_;
  }

  /// Set the provenance origin.
  void set_provenance_origin(const std::string& origin) noexcept {
    provenance_origin_ = origin;
  }

  /// Return the package lineage ID.
  const std::string& package_lineage_id() const noexcept {
    return package_lineage_id_;
  }

  /// Set the package lineage ID.
  void set_package_lineage_id(const std::string& lineage_id) noexcept {
    package_lineage_id_ = lineage_id;
  }

  /// Return the parent artifact ID (for derived artifacts).
  const std::string& parent_artifact_id() const noexcept {
    return parent_artifact_id_;
  }

  /// Set the parent artifact ID.
  void set_parent_artifact_id(const std::string& parent_id) noexcept {
    parent_artifact_id_ = parent_id;
  }

  // Custom metadata.

  /// Set custom metadata key-value pair.
  ///
  /// @param key Metadata key.
  /// @param value Metadata value.
  void set_custom_metadata(const std::string& key,
                           const std::string& value) noexcept {
    custom_metadata_[key] = value;
  }

  /// Retrieve custom metadata.
  const std::unordered_map<std::string, std::string>& custom_metadata()
      const noexcept {
    return custom_metadata_;
  }

  /// Check if manifest is complete and valid.
  ///
  /// @return true if manifest has all required metadata, false otherwise.
  [[nodiscard]] bool is_complete() const noexcept;

  /// Validate manifest invariants (SG-DT-01 Fail-Closed).
  ///
  /// Checks all manifest invariants:
  ///   - artifact_id is non-empty
  ///   - At least one shard placement exists with replication_factor >= 1
  ///   - version is set
  ///   - artifact_class constraints are satisfied
  ///
  /// Returns false on any invariant violation to enforce fail-closed semantics.
  ///
  /// @return true if all invariants hold; false otherwise.
  [[nodiscard]] bool validate() const noexcept;

 protected:
  /// Artifact identifier.
  std::string artifact_id_;

  /// Artifact class.
  ArtifactClass artifact_class_;

  /// Version identifier.
  std::string version_;

  /// Content hash (e.g., SHA-256).
  std::string content_hash_;

  /// Manifest hash (computed from metadata).
  std::string manifest_hash_;

  /// Total artifact size in bytes.
  uint64_t total_size_bytes_ = 0;

  /// Lifecycle stage tracked for retrieval/readiness decisions.
  ArtifactLifecycleStage lifecycle_stage_ = ArtifactLifecycleStage::STAGING;

  /// Shard placement metadata.
  std::vector<ShardPlacement> shard_placements_;

  /// Reconstruction instruction.
  std::optional<ReconstructionInstruction> reconstruction_instruction_;

  /// Recovery/rebuild strategy.
  std::string recovery_strategy_;

  /// Integrity receipt.
  std::optional<IntegrityReceipt> integrity_receipt_;

  /// Freshness timestamp.
  std::string freshness_timestamp_;

  /// Provenance origin.
  std::string provenance_origin_;

  /// Package lineage ID.
  std::string package_lineage_id_;

  /// Parent artifact ID (for derived artifacts).
  std::string parent_artifact_id_;

  /// Custom metadata.
  std::unordered_map<std::string, std::string> custom_metadata_;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
