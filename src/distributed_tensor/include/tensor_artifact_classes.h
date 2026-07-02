/// @file tensor_artifact_classes.h
/// @brief Tensor artifact classification, lifecycle, and truth-bearing semantics for distributed sharding
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-01
///
/// This header defines the distributed tensor artifact system for the Themis sharding fabric.
/// It establishes:
/// - Artifact class taxonomy (Primary, Derived, Ephemeral, AdvisoryOnly)
/// - Lifecycle state machine and transitions
/// - Truth-bearing semantics (SourceOfTruth, TruthAdjacent, Advisory)
/// - Registry and classification interfaces
///
/// ## Design Philosophy
///
/// Tensor artifacts in ThemisDB must be treated as first-class distributed knowledge objects.
/// Different artifact classes have different durability, replication, rebuild, and query requirements.
/// This header provides the classification framework and lifecycle rules for these artifacts.
///
/// ## References
/// - DISTRIBUTED_TENSOR_SHARDING.md: Core architectural thesis
/// - docs/EPIC3_ARTIFACT_CLASSES.md: Artifact class planning
/// - docs/EPIC2_ARTIFACT_LIFECYCLE.md: Lifecycle governance

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

namespace themis {
namespace distributed_tensor {

/// @brief Enumeration of tensor artifact classes.
///
/// Each class has distinct durability, replication, rebuild, and query characteristics.
/// As defined in DISTRIBUTED_TENSOR_SHARDING.md § 4 Tensor Artifact Classes.
enum class ArtifactClass : uint8_t {
  /// Primary tensor artifacts: durable, versioned, integrity-critical.
  /// Examples: LoRA weights, adapter tensors, factor matrices, TT cores, HT subtree components.
  /// Characteristics:
  /// - durable (persisted with integrity guarantees)
  /// - versioned (tracked with explicit versions)
  /// - integrity-critical (correctness affects model behavior)
  /// - provenance-bound (source tracking mandatory)
  /// - often rebuildable from package lineage
  /// - may require exact reconstruction
  PRIMARY = 0,

  /// Derived tensor artifacts: generated from primary knowledge, optimized for acceleration.
  /// Examples: tensor summaries, routing tensors, shard summaries, fingerprints, 
  ///           compressed slices, approximation structures.
  /// Characteristics:
  /// - rebuildable from source (primary artifacts or exact graph truth)
  /// - cacheable (may be discarded and regenerated)
  /// - replaceable (substitute implementations may exist)
  /// - may be quantized or lossy-compressed
  /// - not always exact source-of-truth
  /// - often serve acceleration or routing roles
  DERIVED = 1,

  /// Ephemeral tensor artifacts: query-time or batch-time intermediates.
  /// Examples: temporary contractions, session-local summaries, query-local routing objects,
  ///           short-lived decomposition outputs.
  /// Characteristics:
  /// - transient (exist only during computation)
  /// - non-durable (not persisted long-term)
  /// - not intended for replication across shards
  /// - may still require traceability in critical workflows
  /// - typically scoped to a single query, session, or batch operation
  EPHEMERAL = 2,

  /// Advisory-only artifacts: provide optimization hints but carry no binding semantics.
  /// Examples: routing hints, cost estimates, summary previews, compression suggestions.
  /// Characteristics:
  /// - not truth-bearing (incorrect advice does not affect correctness)
  /// - optional to respect (implementations may ignore)
  /// - may be stale or approximate
  /// - do not require durability or verification
  /// - typically consumed for performance optimization
  ADVISORY_ONLY = 3,
};

/// @brief Enumeration of artifact lifecycle states.
///
/// Artifacts progress through states indicating their availability and freshness:
/// Created → Active → (Stale) → Invalidated/Rebuilt/Deleted
///
/// @note State transitions are constrained; not all transitions are valid.
enum class LifecycleState : uint8_t {
  /// Artifact has been created but not yet activated.
  /// Invariant: No consumer should access this artifact.
  CREATED = 0,

  /// Artifact is active and available for consumption.
  /// Invariant: Artifact meets freshness and integrity requirements.
  ACTIVE = 1,

  /// Artifact is stale (still usable but marked for refresh).
  /// Invariant: Artifact may be used with awareness of staleness.
  /// Action: Background rebuild or invalidation should be scheduled.
  STALE = 2,

  /// Artifact has been invalidated (no longer usable).
  /// Invariant: Consumers must not use this artifact.
  /// Action: Rebuild required before reuse, or permanent deletion.
  INVALIDATED = 3,

  /// Artifact has been rebuilt (refreshed from source).
  /// Invariant: Transitions to ACTIVE after verification.
  /// Note: Transient state; artifact should quickly move to ACTIVE.
  REBUILT = 4,

  /// Artifact has been deleted (permanently removed).
  /// Invariant: No recovery possible; permanent end state.
  DELETED = 5,
};

/// @brief Enumeration of truth-bearing semantics for artifacts.
///
/// This semantic determines whether an artifact can serve as source-of-truth for queries.
enum class TruthSemantic : uint8_t {
  /// This artifact is exact source-of-truth.
  /// Invariant: No approximation; value is authoritative.
  /// Implication: Can be used directly in graph validation and query results.
  /// Examples: Exact tensor values before quantization, uncompressed factor matrices.
  SOURCE_OF_TRUTH = 0,

  /// This artifact is truth-adjacent (derived but without formal approximation bounds).
  /// Invariant: Derived from truth but may not be exact.
  /// Implication: Cannot replace exact graph truth; suitable for optimization/routing.
  /// Examples: Tensor summaries, routing hints derived from exact data.
  TRUTH_ADJACENT = 1,

  /// This artifact is advisory (provides hints but carries no truth semantics).
  /// Invariant: Incorrect or missing advisory data does not affect correctness.
  /// Implication: Purely optional; can be ignored without functional change.
  /// Examples: Cost estimates, routing suggestions, compression heuristics.
  ADVISORY = 2,
};

/// @brief Artifact metadata and classification record.
///
/// This struct captures essential metadata for tensor artifacts, enabling
/// classification, lifecycle tracking, and integrity verification.
struct ArtifactMetadata {
  /// Unique identifier for this artifact (e.g., "LoRA:v2.1:adapter:model-x").
  std::string artifact_id;

  /// Artifact class (Primary, Derived, Ephemeral, AdvisoryOnly).
  ArtifactClass artifact_class;

  /// Current lifecycle state (Created, Active, Stale, Invalidated, Rebuilt, Deleted).
  LifecycleState lifecycle_state;

  /// Truth-bearing semantic (SourceOfTruth, TruthAdjacent, Advisory).
  TruthSemantic truth_semantic;

  /// Version number or hash for this artifact.
  std::string version;

  /// Content hash (e.g., SHA-256) for integrity verification.
  std::string content_hash;

  /// Timestamp when artifact was created (seconds since epoch).
  int64_t created_at_unix_sec = 0;

  /// Timestamp when artifact was last updated (seconds since epoch).
  int64_t updated_at_unix_sec = 0;

  /// Timestamp of last freshness check or validation.
  int64_t last_verified_unix_sec = 0;

  /// If non-zero, artifact is considered stale after this duration (seconds).
  int64_t staleness_threshold_sec = 0;

  /// Optional reference to the source/parent artifact this was derived from.
  /// Used for rebuild and provenance tracking.
  std::string source_artifact_id;

  /// Indicates whether this artifact must be replicated across all shards (true)
  /// or can be placed selectively (false).
  bool requires_full_replication = false;

  /// Indicates whether this artifact can be safely discarded and rebuilt.
  /// Derived and Ephemeral artifacts typically have this set to true.
  bool is_rebuildable = false;

  /// Shard(s) where this artifact is currently placed.
  /// Empty for ephemeral artifacts that exist only in memory.
  std::vector<std::string> shard_placements;
};

/// @brief Artifact lifecycle transition policy and validation.
///
/// Enforces constraints on state transitions and provides lifecycle management utilities.
class ArtifactLifecyclePolicy {
 public:
  /// Checks whether a transition from source_state to target_state is valid.
  /// @param source_state Current lifecycle state.
  /// @param target_state Desired next lifecycle state.
  /// @return true if transition is allowed; false otherwise.
  static bool isValidTransition(LifecycleState source_state, LifecycleState target_state);

  /// Gets a human-readable description of a lifecycle state.
  /// @param state The lifecycle state.
  /// @return String description of the state.
  static std::string stateToString(LifecycleState state);

  /// Parses a string into a lifecycle state.
  /// @param state_str String representation of a state.
  /// @return LifecycleState if valid; std::nullopt otherwise.
  static std::optional<LifecycleState> stringToState(const std::string& state_str);

  /// Determines whether an artifact in the given state can be used by consumers.
  /// @param state The current lifecycle state.
  /// @return true if state is ACTIVE; false otherwise.
  /// @note STALE artifacts are usable but should trigger background refresh.
  static bool isUsable(LifecycleState state);

  /// Determines whether an artifact is in a terminal state (DELETED).
  /// @param state The current lifecycle state.
  /// @return true if state is DELETED; false otherwise.
  static bool isTerminal(LifecycleState state);

  /// Determines whether an artifact requires verification before use.
  /// @param state The current lifecycle state.
  /// @return true if state is CREATED or REBUILT (not yet verified).
  static bool requiresVerification(LifecycleState state);
};

/// @brief Artifact classification and truthfulness validator.
///
/// Provides utilities for validating artifact class combinations and truth semantics.
class ArtifactClassifier {
 public:
  /// Gets a human-readable description of an artifact class.
  /// @param artifact_class The artifact class.
  /// @return String description of the class.
  static std::string classToString(ArtifactClass artifact_class);

  /// Parses a string into an artifact class.
  /// @param class_str String representation of a class.
  /// @return ArtifactClass if valid; std::nullopt otherwise.
  static std::optional<ArtifactClass> stringToClass(const std::string& class_str);

  /// Gets a human-readable description of a truth semantic.
  /// @param semantic The truth semantic.
  /// @return String description of the semantic.
  static std::string semanticToString(TruthSemantic semantic);

  /// Parses a string into a truth semantic.
  /// @param semantic_str String representation of a semantic.
  /// @return TruthSemantic if valid; std::nullopt otherwise.
  static std::optional<TruthSemantic> stringToSemantic(const std::string& semantic_str);

  /// Determines whether a given artifact class and truth semantic combination is valid.
  /// @param artifact_class The artifact class.
  /// @param truth_semantic The truth semantic.
  /// @return true if combination is valid; false otherwise.
  ///
  /// Validity rules:
  /// - PRIMARY artifacts must be SOURCE_OF_TRUTH or TRUTH_ADJACENT
  /// - DERIVED artifacts must be TRUTH_ADJACENT or ADVISORY
  /// - EPHEMERAL artifacts can be any semantic (depend on role)
  /// - ADVISORY_ONLY artifacts must be ADVISORY
  static bool isValidCombination(ArtifactClass artifact_class, TruthSemantic truth_semantic);

  /// Determines whether an artifact is advisory-only (provides hints, not truth).
  /// @param artifact_class The artifact class.
  /// @param truth_semantic The truth semantic.
  /// @return true if artifact is purely advisory; false otherwise.
  ///
  /// An artifact is advisory-only if:
  /// - artifact_class == ADVISORY_ONLY, or
  /// - artifact_class == EPHEMERAL and truth_semantic == ADVISORY, or
  /// - (other combinations where advice is not binding)
  static bool isAdvisoryOnly(ArtifactClass artifact_class, TruthSemantic truth_semantic);

  /// Determines whether an artifact is truth-bearing (affects correctness).
  /// @param artifact_class The artifact class.
  /// @param truth_semantic The truth semantic.
  /// @return true if artifact is truth-bearing; false otherwise.
  ///
  /// An artifact is truth-bearing if:
  /// - truth_semantic == SOURCE_OF_TRUTH or TRUTH_ADJACENT, and
  /// - artifact_class != ADVISORY_ONLY
  static bool isTruthBearing(ArtifactClass artifact_class, TruthSemantic truth_semantic);
};

/// @brief Registry for managing artifact metadata and classification lookups.
///
/// Provides centralized storage and lookup for artifact metadata.
/// Ensures consistent classification and lifecycle management across shards.
class IArtifactRegistry {
 public:
  virtual ~IArtifactRegistry() = default;

  /// Registers an artifact with the given metadata.
  /// @param metadata The artifact metadata to register.
  /// @return true if registration succeeded; false if artifact_id already exists.
  virtual bool registerArtifact(const ArtifactMetadata& metadata) = 0;

  /// Looks up artifact metadata by ID.
  /// @param artifact_id The artifact identifier.
  /// @return ArtifactMetadata if found; std::nullopt otherwise.
  virtual std::optional<ArtifactMetadata> lookup(const std::string& artifact_id) const = 0;

  /// Transitions an artifact to a new lifecycle state.
  /// @param artifact_id The artifact identifier.
  /// @param new_state The desired new lifecycle state.
  /// @return true if transition succeeded; false if invalid transition or not found.
  virtual bool transitionState(const std::string& artifact_id, LifecycleState new_state) = 0;

  /// Lists all artifacts with a given artifact class.
  /// @param artifact_class The class to filter by.
  /// @return Vector of matching artifact IDs.
  virtual std::vector<std::string> listByClass(ArtifactClass artifact_class) const = 0;

  /// Lists all artifacts with a given truth semantic.
  /// @param semantic The truth semantic to filter by.
  /// @return Vector of matching artifact IDs.
  virtual std::vector<std::string> listBySemantic(TruthSemantic semantic) const = 0;

  /// Lists all artifacts in a given lifecycle state.
  /// @param state The lifecycle state to filter by.
  /// @return Vector of matching artifact IDs.
  virtual std::vector<std::string> listByState(LifecycleState state) const = 0;

  /// Gets the total count of registered artifacts.
  /// @return Number of artifacts in the registry.
  virtual size_t count() const = 0;

  /// Removes an artifact from the registry.
  /// @param artifact_id The artifact identifier.
  /// @return true if removed; false if not found.
  virtual bool remove(const std::string& artifact_id) = 0;

  /// Clears all artifacts from the registry.
  virtual void clear() = 0;
};

/// @brief In-memory implementation of the artifact registry.
///
/// Provides basic thread-unsafe registry functionality for initial implementation.
/// Production use should add synchronization or persistence.
class InMemoryArtifactRegistry : public IArtifactRegistry {
 public:
  InMemoryArtifactRegistry() = default;
  ~InMemoryArtifactRegistry() override = default;

  bool registerArtifact(const ArtifactMetadata& metadata) override;
  std::optional<ArtifactMetadata> lookup(const std::string& artifact_id) const override;
  bool transitionState(const std::string& artifact_id, LifecycleState new_state) override;
  std::vector<std::string> listByClass(ArtifactClass artifact_class) const override;
  std::vector<std::string> listBySemantic(TruthSemantic semantic) const override;
  std::vector<std::string> listByState(LifecycleState state) const override;
  size_t count() const override;
  bool remove(const std::string& artifact_id) override;
  void clear() override;

 private:
  /// Maps artifact_id to metadata.
  std::unordered_map<std::string, ArtifactMetadata> metadata_map_;
};

}  // namespace distributed_tensor
}  // namespace themis
