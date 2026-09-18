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


struct ShardPlacement {
  std::string shard_id;

  std::string node_id;

  std::string tier_level = "warm";

  uint64_t shard_size_bytes = 0;

  std::string shard_content_hash;

  uint32_t replication_factor = 1;

  std::string erasure_coding_scheme;

  std::unordered_map<std::string, std::string> custom_metadata;
};

struct ReconstructionInstruction {
  std::string reconstruction_type;

  std::vector<std::string> parent_artifact_ids;

  std::vector<std::string> required_shard_ids;

  std::string reconstruction_parameters;

  uint64_t estimated_reconstruction_time_ms = 0;
};

struct IntegrityReceipt {
  std::string merkle_root_hash;

  std::string verification_method;

  std::string verified_at;

  bool is_valid = false;

  std::unordered_map<std::string, std::string> shard_hashes;
};

class ArtifactManifest {
 public:
  /**
   * @brief Artifact Manifest.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] artifact_class Input parameter.
   * @return Return value.
   */
  explicit ArtifactManifest(const std::string& artifact_id,
                            ArtifactClass artifact_class);

  ArtifactManifest(const ArtifactManifest&) = delete;

  ArtifactManifest(ArtifactManifest&&) noexcept = default;

  ArtifactManifest& operator=(const ArtifactManifest&) = delete;

  ArtifactManifest& operator=(ArtifactManifest&&) noexcept = default;

  /**
   * @brief Artifact Manifest.
   * @return Return value.
   */
  virtual ~ArtifactManifest() = default;

  // Accessors for core metadata.

  const std::string& artifact_id() const noexcept { return artifact_id_; }

  ArtifactClass artifact_class() const noexcept { return artifact_class_; }

  const std::string& version() const noexcept { return version_; }

  void set_version(const std::string& version) noexcept { version_ = version; }

  const std::string& content_hash() const noexcept { return content_hash_; }

  void set_content_hash(const std::string& hash) noexcept {
    content_hash_ = hash;
  }

  const std::string& manifest_hash() const noexcept { return manifest_hash_; }

  /**
   * @brief Compute manifest hash.
   * @note Exception safety: noexcept.
   */
  void compute_manifest_hash() noexcept;

  uint64_t total_size_bytes() const noexcept { return total_size_bytes_; }

  void set_total_size_bytes(uint64_t size) noexcept { total_size_bytes_ = size; }

  ArtifactLifecycleStage lifecycle_stage() const noexcept {
    return lifecycle_stage_;
  }

  void set_lifecycle_stage(ArtifactLifecycleStage stage) noexcept {
    lifecycle_stage_ = stage;
  }

  /**
   * @brief Shard placement management.
   * @param[in] placement Input parameter.
   * @note Exception safety: noexcept.
   */

  void add_shard_placement(ShardPlacement placement) noexcept;

  const std::vector<ShardPlacement>& shard_placements() const noexcept {
    return shard_placements_;
  }

  /**
   * @brief Get shard placement.
   * @param[in] shard_id Identifier of the shard.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  std::optional<ShardPlacement> get_shard_placement(
      const std::string& shard_id) const noexcept;

  size_t num_shards() const noexcept { return shard_placements_.size(); }

  // Reconstruction and recovery management.

  void set_reconstruction_instruction(
      ReconstructionInstruction instruction) noexcept {
    reconstruction_instruction_ = std::move(instruction);
  }

  const std::optional<ReconstructionInstruction>&
  reconstruction_instruction() const noexcept {
    return reconstruction_instruction_;
  }

  const std::string& recovery_strategy() const noexcept {
    return recovery_strategy_;
  }

  void set_recovery_strategy(const std::string& strategy) noexcept {
    recovery_strategy_ = strategy;
  }

  // Integrity and freshness metadata.

  void set_integrity_receipt(IntegrityReceipt receipt) noexcept {
    integrity_receipt_ = std::move(receipt);
  }

  const std::optional<IntegrityReceipt>& integrity_receipt() const noexcept {
    return integrity_receipt_;
  }

  const std::string& freshness_timestamp() const noexcept {
    return freshness_timestamp_;
  }

  /**
   * @brief Update freshness.
   * @note Exception safety: noexcept.
   */
  void update_freshness() noexcept;

  // Provenance and lineage.

  const std::string& provenance_origin() const noexcept {
    return provenance_origin_;
  }

  void set_provenance_origin(const std::string& origin) noexcept {
    provenance_origin_ = origin;
  }

  const std::string& package_lineage_id() const noexcept {
    return package_lineage_id_;
  }

  void set_package_lineage_id(const std::string& lineage_id) noexcept {
    package_lineage_id_ = lineage_id;
  }

  const std::string& parent_artifact_id() const noexcept {
    return parent_artifact_id_;
  }

  void set_parent_artifact_id(const std::string& parent_id) noexcept {
    parent_artifact_id_ = parent_id;
  }

  // Custom metadata.

  void set_custom_metadata(const std::string& key,
                           const std::string& value) noexcept {
    custom_metadata_[key] = value;
  }

  const std::unordered_map<std::string, std::string>& custom_metadata()
      const noexcept {
    return custom_metadata_;
  }

  [[nodiscard]] bool is_complete() const noexcept;

  [[nodiscard]] bool validate() const noexcept;

 protected:
  std::string artifact_id_;

  ArtifactClass artifact_class_;

  std::string version_;

  std::string content_hash_;

  std::string manifest_hash_;

  uint64_t total_size_bytes_ = 0;

  ArtifactLifecycleStage lifecycle_stage_ = ArtifactLifecycleStage::STAGING;

  std::vector<ShardPlacement> shard_placements_;

  std::optional<ReconstructionInstruction> reconstruction_instruction_;

  std::string recovery_strategy_;

  std::optional<IntegrityReceipt> integrity_receipt_;

  std::string freshness_timestamp_;

  std::string provenance_origin_;

  std::string package_lineage_id_;

  std::string parent_artifact_id_;

  std::unordered_map<std::string, std::string> custom_metadata_;
};


}  // namespace distributed_tensor
}  // namespace themis
