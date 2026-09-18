/**
 * @file tensor_artifact_classes.h
 * @brief Core data classes for distributed tensor artifacts.
 *
 * Central type definitions shared by the artifact manifest, integrity
 * verification, and recovery subsystems of the distributed tensor module.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace themis {
namespace distributed_tensor {


enum class ArtifactClass {
  PRIMARY,

  DERIVED,

  EPHEMERAL,
};

enum class ArtifactLifecycleStage {
  STAGING,

  ACTIVE,

  STALE,

  RECOVERING,

  DEPRECATED,

  DELETED,
};

enum class DurabilityLevel {
  NONE,

  SINGLE_COPY,

  REPLICATED,

  ERASURE_CODED,
};

struct ArtifactMetadata {
  std::string artifact_id;

  std::string version;

  ArtifactClass artifact_class;

  ArtifactLifecycleStage lifecycle_stage;

  DurabilityLevel durability_level;

  uint64_t size_bytes = 0;

  std::string content_hash;

  std::string created_at;

  std::string updated_at;

  std::string provenance_origin;

  std::string package_lineage_id;

  bool is_rebuildable = false;

  std::string rebuild_instruction;

  std::unordered_map<std::string, std::string> custom_metadata;
};

class TensorArtifact {
 public:
  TensorArtifact(const std::string& artifact_id,
                 ArtifactClass artifact_class,
                 DurabilityLevel durability_level);

  TensorArtifact(const TensorArtifact&) = delete;

  TensorArtifact(TensorArtifact&&) noexcept = default;

  TensorArtifact& operator=(const TensorArtifact&) = delete;

  TensorArtifact& operator=(TensorArtifact&&) noexcept = default;

  /**
   * @brief Tensor Artifact.
   * @return Return value.
   */
  virtual ~TensorArtifact() = default;

  const ArtifactMetadata& metadata() const noexcept { return metadata_; }

  ArtifactMetadata& mutable_metadata() noexcept { return metadata_; }

  /**
   * @brief Transition lifecycle stage.
   * @param[in] new_stage Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  bool transition_lifecycle_stage(ArtifactLifecycleStage new_stage) noexcept;

  void mark_stale() noexcept {
    transition_lifecycle_stage(ArtifactLifecycleStage::STALE);
  }

  void mark_deprecated() noexcept {
    transition_lifecycle_stage(ArtifactLifecycleStage::DEPRECATED);
  }

  bool is_queryable() const noexcept {
    return metadata_.lifecycle_stage == ArtifactLifecycleStage::ACTIVE;
  }

  bool is_rebuildable() const noexcept { return metadata_.is_rebuildable; }

  void set_rebuild_instruction(const std::string& rebuild_instruction) noexcept {
    metadata_.is_rebuildable = true;
    metadata_.rebuild_instruction = rebuild_instruction;
  }

 protected:
  ArtifactMetadata metadata_;

  /**
   * @brief Is valid transition.
   * @param[in] current_stage Input parameter.
   * @param[in] target_stage Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  static bool is_valid_transition(ArtifactLifecycleStage current_stage,
                                   ArtifactLifecycleStage target_stage) noexcept;
};

class PrimaryTensorArtifact : public TensorArtifact {
 public:
  PrimaryTensorArtifact(const std::string& artifact_id,
                        const std::string& version,
                        DurabilityLevel durability_level);

  PrimaryTensorArtifact(PrimaryTensorArtifact&&) noexcept = default;

  PrimaryTensorArtifact& operator=(PrimaryTensorArtifact&&) noexcept = default;

  ~PrimaryTensorArtifact() override = default;

  void set_provenance_origin(const std::string& origin) noexcept {
    metadata_.provenance_origin = origin;
  }

  void set_package_lineage_id(const std::string& lineage_id) noexcept {
    metadata_.package_lineage_id = lineage_id;
  }

  void set_content_hash(const std::string& content_hash) noexcept {
    metadata_.content_hash = content_hash;
  }
};

class DerivedTensorArtifact : public TensorArtifact {
 public:
  DerivedTensorArtifact(const std::string& artifact_id,
                        const std::string& parent_artifact_id,
                        DurabilityLevel durability_level);

  DerivedTensorArtifact(DerivedTensorArtifact&&) noexcept = default;

  DerivedTensorArtifact& operator=(DerivedTensorArtifact&&) noexcept = default;

  ~DerivedTensorArtifact() override = default;

  const std::string& parent_artifact_id() const noexcept {
    return parent_artifact_id_;
  }

 private:
  std::string parent_artifact_id_;
};

class EphemeralTensorArtifact : public TensorArtifact {
 public:
  EphemeralTensorArtifact(const std::string& artifact_id,
                          const std::string& session_id);

  EphemeralTensorArtifact(EphemeralTensorArtifact&&) noexcept = default;

  EphemeralTensorArtifact& operator=(EphemeralTensorArtifact&&) noexcept = default;

  ~EphemeralTensorArtifact() override = default;

  const std::string& session_id() const noexcept { return session_id_; }

 private:
  std::string session_id_;
};


}  // namespace distributed_tensor
}  // namespace themis
