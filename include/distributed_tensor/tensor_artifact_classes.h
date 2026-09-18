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

/// @defgroup tensor_artifacts Distributed Tensor Artifacts
/// @brief Distributed tensor artifact classes, lifecycle stages, and metadata.
/// @{

/// Artifact class enumeration.
/// 
/// Defines the classification of tensor artifacts based on durability,
/// volatility, and rebuild semantics.
enum class ArtifactClass {
  /// Primary tensor artifacts are durable and semantically significant.
  /// Examples: LoRA weights, adapter tensors, factor matrices.
  /// Characteristics: versioned, integrity-critical, provenance-bound,
  /// often rebuildable, may require exact reconstruction.
  PRIMARY,

  /// Derived tensor artifacts are computed from primary artifacts or queries.
  /// Examples: shard-level summaries, tensor fingerprints, local caches.
  /// Characteristics: reconstructible, lower durability requirements,
  /// may be invalidated by upstream changes.
  DERIVED,

  /// Ephemeral tensor artifacts are temporary and session-scoped.
  /// Examples: intermediate computation results, temporary caches.
  /// Characteristics: volatile, not persisted, no provenance requirements.
  EPHEMERAL,
};

/// Artifact lifecycle stage enumeration.
///
/// Defines the temporal state of a tensor artifact through its lifecycle.
enum class ArtifactLifecycleStage {
  /// Artifact is being created or staged for distribution.
  STAGING,

  /// Artifact is actively available and queryable.
  ACTIVE,

  /// Artifact is marked for potential rebuild or refresh.
  STALE,

  /// Artifact is pending recovery or reconstruction.
  RECOVERING,

  /// Artifact is marked for deletion.
  DEPRECATED,

  /// Artifact has been deleted.
  DELETED,
};

/// Artifact durability requirement.
///
/// Specifies the durability guarantee expected for an artifact.
enum class DurabilityLevel {
  /// Ephemeral artifacts with no durability requirement.
  NONE,

  /// Single-copy artifacts; acceptable for derived/cached data.
  SINGLE_COPY,

  /// Replicated artifacts; acceptable for critical tensors.
  REPLICATED,

  /// Erasure-coded artifacts; high durability with reduced overhead.
  ERASURE_CODED,
};

/// Artifact metadata structure.
///
/// Captures identity, classification, and routing information for a tensor artifact.
/// This structure does not include payload content; it serves as a manifest header.
struct ArtifactMetadata {
  /// Globally unique artifact identifier.
  std::string artifact_id;

  /// Version identifier (e.g., semantic version or commit hash).
  std::string version;

  /// Artifact class (PRIMARY, DERIVED, or EPHEMERAL).
  ArtifactClass artifact_class;

  /// Current lifecycle stage.
  ArtifactLifecycleStage lifecycle_stage;

  /// Durability requirement for this artifact.
  DurabilityLevel durability_level;

  /// Size of the artifact in bytes (estimate or actual).
  uint64_t size_bytes = 0;

  /// Content hash (e.g., SHA-256) for integrity verification.
  /// Empty if not yet computed.
  std::string content_hash;

  /// Timestamp of artifact creation (ISO 8601).
  std::string created_at;

  /// Timestamp of last update (ISO 8601).
  std::string updated_at;

  /// Provenance origin (e.g., "training_job_id", "query_id").
  std::string provenance_origin;

  /// Package lineage identifier (links to training/adaptation runs).
  std::string package_lineage_id;

  /// Rebuildability flag: true if this artifact can be reconstructed
  /// from source data or parent artifacts.
  bool is_rebuildable = false;

  /// Rebuild instruction or plan if is_rebuildable is true.
  /// This may reference parent artifact IDs, computation graphs, or parameters.
  std::string rebuild_instruction;

  /// Custom metadata key-value pairs.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Tensor artifact class.
///
/// Represents a distributed tensor artifact with metadata and lifecycle tracking.
/// The artifact itself does not store the payload; instead, it manages metadata,
/// provenance, and integration with the manifest and shard placement systems.
class TensorArtifact {
 public:
  /// Construct a tensor artifact with basic metadata.
  ///
  /// @param artifact_id Unique identifier for the artifact.
  /// @param artifact_class Classification (PRIMARY, DERIVED, EPHEMERAL).
  /// @param durability_level Durability requirement.
  TensorArtifact(const std::string& artifact_id,
                 ArtifactClass artifact_class,
                 DurabilityLevel durability_level);

  /// Copy constructor deleted to enforce explicit lifetime management.
  TensorArtifact(const TensorArtifact&) = delete;

  /// Move constructor.
  TensorArtifact(TensorArtifact&&) noexcept = default;

  /// Assignment operator deleted.
  TensorArtifact& operator=(const TensorArtifact&) = delete;

  /// Move assignment operator.
  TensorArtifact& operator=(TensorArtifact&&) noexcept = default;

  /// Virtual destructor for polymorphic cleanup.
  virtual ~TensorArtifact() = default;

  /// Return the artifact metadata.
  const ArtifactMetadata& metadata() const noexcept { return metadata_; }

  /// Return mutable metadata for updates.
  ArtifactMetadata& mutable_metadata() noexcept { return metadata_; }

  /// Transition artifact to a new lifecycle stage.
  ///
  /// @param new_stage Target lifecycle stage.
  /// @return true if transition is valid, false otherwise.
  bool transition_lifecycle_stage(ArtifactLifecycleStage new_stage) noexcept;

  /// Mark the artifact as stale (requires refresh).
  void mark_stale() noexcept {
    transition_lifecycle_stage(ArtifactLifecycleStage::STALE);
  }

  /// Mark the artifact for deprecation.
  void mark_deprecated() noexcept {
    transition_lifecycle_stage(ArtifactLifecycleStage::DEPRECATED);
  }

  /// Query if the artifact is in an active, queryable state.
  bool is_queryable() const noexcept {
    return metadata_.lifecycle_stage == ArtifactLifecycleStage::ACTIVE;
  }

  /// Query if the artifact is rebuildable.
  bool is_rebuildable() const noexcept { return metadata_.is_rebuildable; }

  /// Set rebuild information for this artifact.
  ///
  /// @param rebuild_instruction Instruction or plan for rebuilding.
  void set_rebuild_instruction(const std::string& rebuild_instruction) noexcept {
    metadata_.is_rebuildable = true;
    metadata_.rebuild_instruction = rebuild_instruction;
  }

 protected:
  /// Artifact metadata.
  ArtifactMetadata metadata_;

  /// Validate lifecycle stage transition.
  ///
  /// @param current_stage Current stage.
  /// @param target_stage Target stage.
  /// @return true if transition is allowed, false otherwise.
  static bool is_valid_transition(ArtifactLifecycleStage current_stage,
                                   ArtifactLifecycleStage target_stage) noexcept;
};

/// Primary tensor artifact class.
///
/// Represents durable, versioned tensor artifacts that are provenance-bound
/// and often rebuildable (e.g., LoRA weights, factor matrices).
class PrimaryTensorArtifact : public TensorArtifact {
 public:
  /// Construct a primary tensor artifact.
  ///
  /// @param artifact_id Unique identifier.
  /// @param version Version identifier.
  /// @param durability_level Durability requirement.
  PrimaryTensorArtifact(const std::string& artifact_id,
                        const std::string& version,
                        DurabilityLevel durability_level);

  /// Move constructor.
  PrimaryTensorArtifact(PrimaryTensorArtifact&&) noexcept = default;

  /// Move assignment operator.
  PrimaryTensorArtifact& operator=(PrimaryTensorArtifact&&) noexcept = default;

  /// Destructor.
  ~PrimaryTensorArtifact() override = default;

  /// Set provenance origin (e.g., training job ID).
  void set_provenance_origin(const std::string& origin) noexcept {
    metadata_.provenance_origin = origin;
  }

  /// Set package lineage ID.
  void set_package_lineage_id(const std::string& lineage_id) noexcept {
    metadata_.package_lineage_id = lineage_id;
  }

  /// Set content hash for integrity verification.
  void set_content_hash(const std::string& content_hash) noexcept {
    metadata_.content_hash = content_hash;
  }
};

/// Derived tensor artifact class.
///
/// Represents artifacts computed from primary artifacts or queries (e.g., summaries).
class DerivedTensorArtifact : public TensorArtifact {
 public:
  /// Construct a derived tensor artifact.
  ///
  /// @param artifact_id Unique identifier.
  /// @param parent_artifact_id ID of the parent artifact.
  /// @param durability_level Durability requirement.
  DerivedTensorArtifact(const std::string& artifact_id,
                        const std::string& parent_artifact_id,
                        DurabilityLevel durability_level);

  /// Move constructor.
  DerivedTensorArtifact(DerivedTensorArtifact&&) noexcept = default;

  /// Move assignment operator.
  DerivedTensorArtifact& operator=(DerivedTensorArtifact&&) noexcept = default;

  /// Destructor.
  ~DerivedTensorArtifact() override = default;

  /// Return the parent artifact ID.
  const std::string& parent_artifact_id() const noexcept {
    return parent_artifact_id_;
  }

 private:
  std::string parent_artifact_id_;
};

/// Ephemeral tensor artifact class.
///
/// Represents temporary, session-scoped artifacts (e.g., intermediate results).
class EphemeralTensorArtifact : public TensorArtifact {
 public:
  /// Construct an ephemeral tensor artifact.
  ///
  /// @param artifact_id Unique identifier.
  /// @param session_id Session or context identifier.
  EphemeralTensorArtifact(const std::string& artifact_id,
                          const std::string& session_id);

  /// Move constructor.
  EphemeralTensorArtifact(EphemeralTensorArtifact&&) noexcept = default;

  /// Move assignment operator.
  EphemeralTensorArtifact& operator=(EphemeralTensorArtifact&&) noexcept = default;

  /// Destructor.
  ~EphemeralTensorArtifact() override = default;

  /// Return the session or context identifier.
  const std::string& session_id() const noexcept { return session_id_; }

 private:
  std::string session_id_;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
