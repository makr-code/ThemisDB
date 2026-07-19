/// @file artifact_manifest.h
/// @brief Distributed tensor artifact manifest schema for coordination, reconstruction, and freshness
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02
///
/// This header defines the comprehensive manifest schema for distributed tensor artifacts.
/// It captures all metadata required for:
/// - Artifact placement and reconstruction across shards
/// - Provenance tracking and rebuild history
/// - Freshness and validity assessment for query planning
/// - Integrity verification and corruption detection
/// - Dynamic update coordination (patch, partial_refit, rebuild)
///
/// ## Design Philosophy
///
/// The manifest is the durable, authoritative coordination object for tensor artifacts.
/// It serves as the single source of truth for:
/// - What the artifact is (class, semantic, version)
/// - Where it is (placement, replication)
/// - How fresh it is (age, staleness, last_verified)
/// - How to reconstruct it (source, update_mode, rebuild history)
/// - Whether it can be trusted (integrity, validity)
///
/// The tensor payload itself is managed relative to the manifest.
///
/// ## Manifest Lifecycle
///
/// 1. CREATED: Manifest instantiated, not yet activated
/// 2. ACTIVE: Manifest is current and usable
/// 3. STALE: Manifest still valid but marked for refresh
/// 4. INVALIDATED: Manifest no longer valid; rebuild required
/// 5. REBUILT: Manifest has been refreshed; transitioning to ACTIVE
/// 6. DELETED: Manifest permanently removed
///
/// ## Planner Integration
///
/// Fields marked with [PLANNER] are specifically designed for query planner consumption.
/// These fields enable the planner to determine:
/// - Whether an artifact is suitable for the current query
/// - Whether freshness requirements are met
/// - Whether summary-first or exact loading is required
/// - Whether rebuild is necessary before use
///
/// ## References
/// - DISTRIBUTED_TENSOR_SHARDING.md: Core architectural thesis
/// - docs/EPIC3_MANIFEST_SCHEMA.md: Manifest planning and roadmap
/// - src/distributed_tensor/include/tensor_artifact_classes.h: Artifact classification

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>

// Include tensor artifact classes for type definitions
#include "tensor_artifact_classes.h"

namespace themis {
namespace distributed_tensor {

// Backwards-compatibility: older code/tests refer to `ArtifactLifecycleState`.
using ArtifactLifecycleState = LifecycleState;

// Forward declarations (if needed)
struct ArtifactMetadata;

/// @brief Enumeration of rebuild states for artifacts.
///
/// Tracks how an artifact has been modified since its original creation.
/// Used to determine whether exact reconstruction or incremental updates are possible.
enum class RebuildState : uint8_t {
  /// Artifact is in its original, unmodified state as created/deployed.
  /// Invariant: Exactly matches the creation source.
  PRISTINE = 0,

  /// Artifact has been modified via patch operations (small, localized changes).
  /// Invariant: Original source + patches = current artifact.
  /// Characteristic: Can be reverse-patched or re-derived from source.
  PATCHED = 1,

  /// Artifact has been modified via partial refit (selective tensor retraining/update).
  /// Invariant: Some tensor components are updated; others unchanged.
  /// Characteristic: Requires lineage tracking for components that changed.
  PARTIAL_REFITTED = 2,

  /// Artifact has been completely rebuilt from scratch or from source lineage.
  /// Invariant: Artifact matches its rebuild source exactly.
  /// Characteristic: Full rebuild provenance available; ready for deployment.
  REBUILT = 3,
};

/// @brief Enumeration of update modes for dynamic tensor artifact changes.
///
/// Describes the method used to modify or refresh an artifact.
enum class UpdateMode : uint8_t {
  /// Patch updates: small, localized changes (delta-based).
  /// Example: Correcting outlier values, adjusting weights for specific samples.
  /// Cost: O(1) per change relative to full tensor size.
  PATCH = 0,

  /// Partial refit: selective tensor retraining/update for subset of components.
  /// Example: LoRA adapter retraining on new domain data; factor matrix update.
  /// Cost: O(k) where k is the subset size; less expensive than full rebuild.
  PARTIAL_REFIT = 1,

  /// Full rebuild: complete recomputation or re-derivation from source.
  /// Example: Reconstructing artifact from package lineage after corruption.
  /// Cost: O(n) where n is full artifact size; most expensive.
  REBUILD = 2,
};

/// @brief Enumeration of invalidation reasons.
///
/// Explains WHY an artifact was invalidated (transitioned to INVALIDATED state).
/// Used for diagnostics, recovery planning, and audit trails.
enum class InvalidationReason : uint8_t {
  /// Artifact was invalidated for unspecified or unknown reason.
  UNKNOWN = 0,

  /// Artifact failed integrity verification (content hash mismatch).
  /// Action: Rebuild from source or replace with backup.
  INTEGRITY_CHECK_FAILED = 1,

  /// Artifact exceeds staleness threshold; refresh required.
  /// Action: Rebuild or revalidate from source.
  STALENESS_EXCEEDED = 2,

  /// Artifact's source or dependency has been invalidated.
  /// Implication: Artifact is no longer buildable or trustworthy.
  /// Action: Rebuild from higher-order source or mark for deletion.
  SOURCE_INVALIDATED = 3,

  /// Artifact's source lineage is incomplete or corrupted.
  /// Action: Attempt recovery from backup lineage or mark for deletion.
  SOURCE_LINEAGE_CORRUPTED = 4,

  /// Artifact conflicts with updated classification or policies.
  /// Example: Semantic or class reassignment; policy change renders artifact invalid.
  /// Action: Reclassify or rebuild to conform to new policies.
  POLICY_VIOLATION = 5,

  /// Artifact was explicitly marked for invalidation by administrator or system.
  /// Action: Rebuild or delete per administrative directive.
  ADMIN_REQUESTED = 6,

  /// Artifact's shard placement is no longer valid (shard unavailable/failed).
  /// Action: Migrate to healthy shard or rebuild.
  SHARD_UNAVAILABLE = 7,
};

/// @brief Rank status for tensor components.
/// Backwards-compatibility: older tests expect `RankStatus::ACTIVE` symbolic names.
enum RankStatus : uint32_t {
  RANK_ACTIVE = 0,
  RANK_PRUNED = 1,
  RANK_INVALIDATED = 2,
  // Provide short names commonly used in tests
  ACTIVE = RANK_ACTIVE,
  PRUNED = RANK_PRUNED,
  INVALIDATED = RANK_INVALIDATED
};

/// @brief Core manifest for distributed tensor artifacts.
///
/// Provides durable, authoritative metadata for artifact coordination, placement,
/// reconstruction, and freshness assessment across shards.
///
/// ## Thread Safety
/// This struct is a plain data type; callers are responsible for synchronization.
struct ArtifactManifest {
  // ========================================================================
  // Identity & Classification (from ArtifactMetadata)
  // ========================================================================

  /// Unique identifier for this artifact (e.g., "LoRA:v2.1:adapter:model-x").
  std::string artifact_id;

  /// Artifact class (Primary, Derived, Ephemeral, AdvisoryOnly).
  /// [PLANNER] Used to determine rebuild eligibility and caching behavior.
  ArtifactClass artifact_class = ArtifactClass::PRIMARY;

  /// Truth-bearing semantic (SourceOfTruth, TruthAdjacent, Advisory).
  /// [PLANNER] Used to determine query correctness guarantees.
  TruthSemantic truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;

  /// Current lifecycle state.
  /// [PLANNER] Determines if artifact is usable; ACTIVE/STALE are usable.
  /// Backwards-compatibility: older code/tests reference `current_state` and the
  /// alias type `ArtifactLifecycleState`. Provide both names via a small union
  /// so callers can use either identifier interchangeably.
  using ArtifactLifecycleState = LifecycleState;
  union {
    LifecycleState lifecycle_state = LifecycleState::CREATED;
    LifecycleState current_state;
  };

  // ========================================================================
  // Versioning & Integrity
  // ========================================================================

  /// Version number or hash for this artifact (semantic version or content hash).
  std::string version;

  /// Content hash (e.g., SHA-256) for integrity verification and deduplication.
  std::string content_hash;

  /// Manifest hash: hash of this manifest itself for change detection.
  /// Used to identify when manifest metadata has changed.
  std::string manifest_hash;

  // ========================================================================
  // Temporal Metadata
  // ========================================================================

  /// Timestamp when artifact was created (seconds since epoch).
  int64_t created_at_unix_sec = 0;

  /// Timestamp when artifact was last updated or refreshed (seconds since epoch).
  int64_t updated_at_unix_sec = 0;

  /// Timestamp of last freshness check or validation (seconds since epoch).
  /// [PLANNER] Used to calculate artifact age relative to now.
  int64_t last_verified_unix_sec = 0;

  /// Timestamp of last rebuild operation (seconds since epoch).
  /// Used to track rebuild frequency and plan next refresh.
  int64_t last_rebuild_at_unix_sec = 0;

  /// If non-zero, artifact is considered stale after this duration (seconds).
  /// [PLANNER] Determines if artifact should trigger background refresh.
  int64_t staleness_threshold_sec = 0;

  /// Artifact age in milliseconds (calculated relative to now).
  /// Convenience field; artifact_age_ms = (now - created_at) * 1000.
  /// [PLANNER] Used for freshness-aware query planning.
  uint64_t artifact_age_ms = 0;

  // ========================================================================
  // Sequence & Lag Tracking (for dynamic updates)
  // ========================================================================

  /// Sequence number when this artifact's source data began (inclusive).
  /// Used to track which records from the source are included in this artifact.
  /// Example: If source sequences are 0..10000, artifact might cover 100..5000.
  union {
    uint64_t source_seq_start = 0;
    /// Backwards-compatibility name expected by older tests
    uint64_t source_sequence_begin;
  };

  /// Sequence number when this artifact's source data ended (inclusive).
  union {
    uint64_t source_seq_end = 0;
    /// Backwards-compatibility name expected by older tests
    uint64_t source_sequence_current;
  };

  /// Delta lag: how far behind the source data is this artifact, measured in sequence units.
  /// [PLANNER] Indicates freshness relative to latest source.
  /// If source is at seq 10000 and artifact covers up to seq 9500, delta_lag = 500.
  uint64_t delta_lag = 0;

  // ========================================================================
  // Approximation & Quality Metrics
  // ========================================================================

  /// Residual error for approximate/compressed artifacts.
  /// Measures deviation from ground truth for lossy-compressed or quantized tensors.
  /// Range: [0.0, 1.0] or [0, 100] depending on metric.
  /// [PLANNER] Determines whether artifact is suitable for accuracy-critical queries.
  double residual = 0.0;

  /// Maximum tensor rank captured in this artifact.
  /// For Tucker decomposition, TT decomposition, or factorized tensors.
  /// [PLANNER] Used to determine decomposition quality and rebuild necessity.
  uint32_t rank_cap = 0;

  /// Current tensor rank status (subset of tensors that are active/valid).
  /// May be less than rank_cap if some factors have been pruned or invalidated.
  uint32_t rank_status = 0;

  // ========================================================================
  // Rebuild & Update Tracking
  // ========================================================================

  /// Current rebuild state of this artifact.
  /// Tracks whether artifact is pristine, patched, partially refitted, or fully rebuilt.
  /// [PLANNER] Determines whether incremental updates are possible.
  RebuildState rebuild_state = RebuildState::PRISTINE;

  /// Most recent update mode applied to this artifact.
  /// Indicates whether last modification was a patch, partial_refit, or rebuild.
  /// [PLANNER] Helps determine cost and feasibility of future updates.
  UpdateMode update_mode = UpdateMode::PATCH;

  /// Reason why artifact was invalidated (if lifecycle_state == INVALIDATED).
  /// Provides diagnostics and recovery guidance.
  InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

  // ========================================================================
  // Provenance & Reconstruction
  // ========================================================================

  /// Optional reference to the source/parent artifact this was derived from.
  /// Used for rebuild and provenance tracking.
  /// Example: A DERIVED artifact might reference a PRIMARY artifact as source_artifact_id.
  std::string source_artifact_id;

  /// Provenance chain: list of artifact IDs that contributed to this artifact.
  /// Enables full lineage reconstruction and audit trails.
  /// Example: [base_model, lora_adapter_1, lora_adapter_2, partial_refit_3].
  std::vector<std::string> provenance_chain;

  /// Reconstruction instructions: how to rebuild this artifact from source.
  /// Example: "apply patches [p1, p2, p3]" or "refit with config v2.1" or "regenerate from lineage".
  /// [PLANNER] May contain information about rebuild strategy.
  std::string reconstruction_instructions;

  // ========================================================================
  // Placement & Distribution
  // ========================================================================

  /// Shard(s) where this artifact is currently placed.
  /// Empty for ephemeral artifacts that exist only in memory.
  /// [PLANNER] Determines where to fetch the artifact for local aggregation.
  std::vector<std::string> shard_placements;

  /// Indicates whether this artifact must be replicated across all shards (true)
  /// or can be placed selectively (false).
  /// [PLANNER] Affects query routing and retrieval strategy.
  bool requires_full_replication = false;

  /// Indicates whether this artifact can be safely discarded and rebuilt.
  /// Derived and Ephemeral artifacts typically have this set to true.
  /// [PLANNER] Determines whether cached artifacts can be evicted.
  bool is_rebuildable = false;

  // Backwards-compatibility fields expected by older tests
  // Some legacy tests directly read/write `size_bytes` and `semantic_hint`.
  uint64_t size_bytes = 0;
  std::string semantic_hint;

  // ========================================================================
  // Replication & Redundancy Strategy
  // ========================================================================

  /// Replication factor: number of copies to maintain across shards.
  /// Default 1 = single copy; higher values = geographic/fault redundancy.
  uint32_t replication_factor = 1;

  /// Erasure coding parameters (if applicable).
  /// Example: "reed_solomon_4_2" = 4 data blocks + 2 parity blocks.
  std::string erasure_coding_scheme;

  /// Backup shard IDs (shards that hold backup/replica copies).
  std::vector<std::string> backup_shard_placements;

  // ========================================================================
  // Compatibility & Constraints
  // ========================================================================

  /// Compatibility metadata: key-value pairs for domain-specific compatibility checks.
  /// Example: {"model_version": "1.2.3", "gpu_arch": "RTX40xx", "cuda_version": "12.2"}
  /// [PLANNER] Used to determine whether artifact can be used in current environment.
  std::map<std::string, std::string> compatibility_metadata;

  /// Minimum planner version required to use this artifact.
  /// Example: "2.4.0" means planner >= 2.4.0 required.
  /// [PLANNER] Checked before accepting artifact for use.
  std::string min_planner_version;

  /// Advisory-only flag: if true, artifact provides hints but not binding truth.
  /// Derived from (artifact_class == ADVISORY_ONLY || truth_semantic == ADVISORY).
  /// [PLANNER] Determines whether artifact can be used for correctness guarantees.
  bool advisory_only = false;

  // ========================================================================
  // Metadata & Extensibility
  // ========================================================================

  /// Custom attributes for extensibility and domain-specific metadata.
  /// Example: {"model_name": "llama-7b", "training_date": "2026-06-15", "domain": "medical"}.
  std::map<std::string, std::string> custom_attributes;

  /// Human-readable description or notes about this artifact.
  std::string description;

  // ========================================================================
  // Methods
  // ========================================================================

  /// Validates manifest invariants and consistency.
  /// @return true if manifest is valid; false otherwise.
  bool validate() const;

  /// Checks if artifact is usable (in ACTIVE state and meets freshness requirements).
  /// @param now_unix_sec Current time in seconds since epoch.
  /// @return true if artifact can be used for queries.
  bool isUsable(int64_t now_unix_sec) const;

  /// Checks if artifact is stale (exceeds staleness threshold).
  /// @param now_unix_sec Current time in seconds since epoch.
  /// @return true if artifact should be refreshed.
  bool isStale(int64_t now_unix_sec) const;

  /// Calculates freshness score [0.0, 1.0].
  /// 1.0 = just verified; 0.0 = stale or unverified.
  /// [PLANNER] Used for freshness-aware query planning.
  /// @param now_unix_sec Current time in seconds since epoch.
  /// @return Freshness score in range [0.0, 1.0].
  double getFreshnessScore(int64_t now_unix_sec) const;

  /// Checks if manifest has been corrupted (based on manifest_hash).
  /// @return true if manifest hash does not match expected hash.
  bool isCorrupted() const;

  /// Serializes manifest to JSON string.
  /// @return JSON representation of manifest.
  std::string toJSON() const;

  /// Deserializes manifest from JSON string.
  /// @param json_str JSON string.
  /// @return ArtifactManifest if deserialization succeeds; std::nullopt otherwise.
  static std::optional<ArtifactManifest> fromJSON(const std::string& json_str);

  /// Serializes manifest to YAML string.
  /// @return YAML representation of manifest.
  std::string toYAML() const;

  /// Deserializes manifest from YAML string.
  /// @param yaml_str YAML string.
  /// @return ArtifactManifest if deserialization succeeds; std::nullopt otherwise.
  static std::optional<ArtifactManifest> fromYAML(const std::string& yaml_str);
};

/// @brief Conversion utilities for rebuild state.
class RebuildStateUtils {
 public:
  /// Gets a human-readable description of a rebuild state.
  static std::string stateToString(RebuildState state);

  /// Parses a string into a rebuild state.
  static std::optional<RebuildState> stringToState(const std::string& state_str);
};

/// @brief Conversion utilities for update mode.
class UpdateModeUtils {
 public:
  /// Gets a human-readable description of an update mode.
  static std::string modeToString(UpdateMode mode);

  /// Parses a string into an update mode.
  static std::optional<UpdateMode> stringToMode(const std::string& mode_str);
};

/// @brief Conversion utilities for invalidation reason.
class InvalidationReasonUtils {
 public:
  /// Gets a human-readable description of an invalidation reason.
  static std::string reasonToString(InvalidationReason reason);

  /// Parses a string into an invalidation reason.
  static std::optional<InvalidationReason> stringToReason(const std::string& reason_str);
};

}  // namespace distributed_tensor
}  // namespace themis
