// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/tensor_artifact_classes.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace distributed_tensor {

// Helper: Format current timestamp as ISO 8601 string.
static std::string get_iso8601_timestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss = {};
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// TensorArtifact implementation.

TensorArtifact::TensorArtifact(const std::string& artifact_id,
                               ArtifactClass artifact_class,
                               DurabilityLevel durability_level)
    : metadata_{
          .artifact_id = artifact_id,
          .version = "",
          .artifact_class = artifact_class,
          .lifecycle_stage = ArtifactLifecycleStage::STAGING,
          .durability_level = durability_level,
          .size_bytes = 0,
          .content_hash = "",
          .created_at = get_iso8601_timestamp(),
          .updated_at = get_iso8601_timestamp(),
          .provenance_origin = "",
          .package_lineage_id = "",
          .is_rebuildable = false,
          .rebuild_instruction = "",
          .custom_metadata = {},
      } {}

bool TensorArtifact::transition_lifecycle_stage(
    ArtifactLifecycleStage new_stage) noexcept {
  if (!is_valid_transition(metadata_.lifecycle_stage, new_stage)) {
    return false;
  }
  metadata_.lifecycle_stage = new_stage;
  metadata_.updated_at = get_iso8601_timestamp();
  return true;
}

bool TensorArtifact::is_valid_transition(ArtifactLifecycleStage current_stage,
                                          ArtifactLifecycleStage target_stage) noexcept {
  // STAGING can transition to ACTIVE, DEPRECATED, or DELETED.
  if (current_stage == ArtifactLifecycleStage::STAGING) {
    return target_stage == ArtifactLifecycleStage::ACTIVE ||
           target_stage == ArtifactLifecycleStage::DEPRECATED ||
           target_stage == ArtifactLifecycleStage::DELETED;
  }

  // ACTIVE can transition to STALE, RECOVERING, DEPRECATED, or DELETED.
  if (current_stage == ArtifactLifecycleStage::ACTIVE) {
    return target_stage == ArtifactLifecycleStage::STALE ||
           target_stage == ArtifactLifecycleStage::RECOVERING ||
           target_stage == ArtifactLifecycleStage::DEPRECATED ||
           target_stage == ArtifactLifecycleStage::DELETED;
  }

  // STALE can transition to ACTIVE, RECOVERING, DEPRECATED, or DELETED.
  if (current_stage == ArtifactLifecycleStage::STALE) {
    return target_stage == ArtifactLifecycleStage::ACTIVE ||
           target_stage == ArtifactLifecycleStage::RECOVERING ||
           target_stage == ArtifactLifecycleStage::DEPRECATED ||
           target_stage == ArtifactLifecycleStage::DELETED;
  }

  // RECOVERING can transition to ACTIVE, STALE, DEPRECATED, or DELETED.
  if (current_stage == ArtifactLifecycleStage::RECOVERING) {
    return target_stage == ArtifactLifecycleStage::ACTIVE ||
           target_stage == ArtifactLifecycleStage::STALE ||
           target_stage == ArtifactLifecycleStage::DEPRECATED ||
           target_stage == ArtifactLifecycleStage::DELETED;
  }

  // DEPRECATED can transition to DELETED only.
  if (current_stage == ArtifactLifecycleStage::DEPRECATED) {
    return target_stage == ArtifactLifecycleStage::DELETED;
  }

  // DELETED is a terminal state; no transitions allowed.
  return false;
}

// PrimaryTensorArtifact implementation.

PrimaryTensorArtifact::PrimaryTensorArtifact(const std::string& artifact_id,
                                             const std::string& version,
                                             DurabilityLevel durability_level)
    : TensorArtifact(artifact_id, ArtifactClass::PRIMARY, durability_level) {
  metadata_.version = version;
}

// DerivedTensorArtifact implementation.

DerivedTensorArtifact::DerivedTensorArtifact(const std::string& artifact_id,
                                             const std::string& parent_artifact_id,
                                             DurabilityLevel durability_level)
    : TensorArtifact(artifact_id, ArtifactClass::DERIVED, durability_level),
      parent_artifact_id_(parent_artifact_id) {
  // Derived artifacts are inherently rebuildable from their parents.
  metadata_.is_rebuildable = true;
  metadata_.rebuild_instruction = "reconstruct_from_parent:" + parent_artifact_id;
}

// EphemeralTensorArtifact implementation.

EphemeralTensorArtifact::EphemeralTensorArtifact(const std::string& artifact_id,
                                                 const std::string& session_id)
    : TensorArtifact(artifact_id, ArtifactClass::EPHEMERAL,
                     DurabilityLevel::NONE),
      session_id_(session_id) {
  // Ephemeral artifacts are not rebuildable by definition.
  metadata_.is_rebuildable = false;
}

}  // namespace distributed_tensor
}  // namespace themis
