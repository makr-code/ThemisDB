/**
 * @file artifact_manifest.h
 * @brief Tensor artifact manifest schema for ThemisDB distributed tensor layer.
 *
 * ## Advisory-Only Artifact Policy (Phase A invariant)
 *
 * **Tensor artifacts described by this manifest are advisory only.**
 * They capture summaries, candidates, and freshness metadata produced by the
 * ANN and tensor mid-layers.  They MUST NOT be used as the authoritative
 * source of truth for any query result.  The Graph Truth Layer (CPU-first,
 * exact traversal) is the sole authoritative source.
 *
 * @invariant ArtifactKind::ADVISORY_SUMMARY artifacts are inputs to the
 *            query planner, never outputs returned to callers.
 * @invariant No manifest entry overrides a graph-verified result.
 * @invariant Stale manifests (freshness_age_s > threshold) must cause the
 *            planner to fall back to exact graph retrieval.
 *
 * @see ai_working/HYBRID_RETRIEVAL_ROLLOUT_PLAN.md §4 Phase A
 * @see ManifestStore for the storage and freshness API
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ArtifactKind
// ---------------------------------------------------------------------------

/**
 * @brief Classification of a tensor artifact.
 *
 * Only ADVISORY_SUMMARY is allowed in Phase A.  DELTA_LOG and SHARD_SUMMARY
 * entries are reserved for Phase B and C respectively.
 */
enum class ArtifactKind : uint8_t {
    /// Summarized representation of a tensor; advisory, never authoritative.
    ADVISORY_SUMMARY = 0,
    /// Incremental delta record; Phase B entry gate.
    DELTA_LOG        = 1,
    /// Cross-shard summary for routing; Phase C entry gate.
    SHARD_SUMMARY    = 2,
};

// ---------------------------------------------------------------------------
// RebuildState
// ---------------------------------------------------------------------------

/**
 * @brief Describes how the artifact was last updated.
 *
 * Tracks the lifecycle state of a tensor artifact after the snapshot
 * update worker processes a delta window.  Used by the planner to
 * determine confidence in the artifact before use.
 */
enum class RebuildState : uint8_t {
    /// Artifact has never been updated (pristine/initial state).
    PRISTINE = 0,
    /// Artifact was patched (small delta, O(delta_size) cost).
    PATCHED = 1,
    /// Artifact underwent selective partial refit.
    PARTIAL_REFITTED = 2,
    /// Artifact was fully rebuilt from source graph state.
    REBUILT = 3,
    /// Artifact is in an invalid/unknown state; treat as stale.
    INVALID = 4,
};

// ---------------------------------------------------------------------------
// UpdateMode
// ---------------------------------------------------------------------------

/**
 * @brief Describes the update strategy applied by the snapshot update worker.
 *
 * Mirrors @c UpdateDecision but persisted in the manifest so that consumers
 * can reason about the cost and quality of the last update cycle.
 */
enum class UpdateMode : uint8_t {
    /// No update has been applied yet.
    NONE = 0,
    /// Patch was applied (< 10 % delta fraction).
    PATCH = 1,
    /// Partial refit was applied (10–50 % delta fraction).
    PARTIAL_REFIT = 2,
    /// Full rebuild was performed (> 50 % delta fraction).
    REBUILD = 3,
};

// ---------------------------------------------------------------------------
// LifecycleState
// ---------------------------------------------------------------------------

/**
 * @brief Lifecycle state of a tensor artifact.
 *
 * Used by the planner and invalidation manager to determine whether an
 * artifact is safe to use for routing decisions.
 */
enum class LifecycleState : uint8_t {
    /// Artifact is current and safe for advisory use.
    ACTIVE = 0,
    /// Artifact is outdated; should be refreshed before use.
    STALE = 1,
    /// Artifact has been invalidated and must not be used.
    INVALIDATED = 2,
};

// ---------------------------------------------------------------------------
// InvalidationReason
// ---------------------------------------------------------------------------

/**
 * @brief Reason why an artifact was invalidated.
 *
 * Persisted in the manifest so that consumers can distinguish transient
 * staleness from hard integrity failures.
 */
enum class InvalidationReason : uint8_t {
    /// Reason is unknown or not set.
    UNKNOWN = 0,
    /// CRC or payload integrity check failed.
    INTEGRITY_CHECK_FAILED = 1,
    /// Staleness threshold exceeded.
    STALENESS_EXCEEDED = 2,
    /// Source artifact was invalidated (cascade).
    SOURCE_INVALIDATED = 3,
    /// Source lineage is corrupted.
    SOURCE_LINEAGE_CORRUPTED = 4,
    /// Policy rule violation.
    POLICY_VIOLATION = 5,
    /// Explicit admin/operator request.
    ADMIN_REQUESTED = 6,
    /// Owning shard is unavailable.
    SHARD_UNAVAILABLE = 7,
};

// ---------------------------------------------------------------------------
// InvalidationReasonUtils
// ---------------------------------------------------------------------------

/**
 * @brief Utility functions for @c InvalidationReason enum conversions.
 */
struct InvalidationReasonUtils {
    /// Convert an @c InvalidationReason to its canonical string representation.
    /// @param reason  Value to convert.
    /// @return        Non-empty string; "UNKNOWN" for unrecognised values.
    static std::string reasonToString(InvalidationReason reason);

    /// Parse a canonical string back to an @c InvalidationReason value.
    /// @param reason_str  String produced by reasonToString().
    /// @return            Parsed value, or std::nullopt on failure.
    static std::optional<InvalidationReason> stringToReason(const std::string& reason_str);
};

// ---------------------------------------------------------------------------
// ArtifactLifecyclePolicy
// ---------------------------------------------------------------------------

/**
 * @brief Utility functions for @c LifecycleState enum conversions.
 */
struct ArtifactLifecyclePolicy {
    /// Convert a @c LifecycleState value to its canonical string representation.
    /// @param state  Value to convert.
    /// @return       Non-empty string; "UNKNOWN" for unrecognised values.
    static std::string stateToString(LifecycleState state);

    /// Parse a canonical string back to a @c LifecycleState value.
    /// @param state_str  String produced by stateToString().
    /// @return           Parsed value, or std::nullopt on failure.
    static std::optional<LifecycleState> stringToState(const std::string& state_str);
};

// ---------------------------------------------------------------------------
// RebuildStateUtils
// ---------------------------------------------------------------------------

/**
 * @brief Utility functions for @c RebuildState enum conversions.
 */
struct RebuildStateUtils {
    /// Convert a @c RebuildState value to its canonical string representation.
    /// @param state  Value to convert.
    /// @return       Non-empty string; "UNKNOWN" for unrecognised values.
    static std::string stateToString(RebuildState state);

    /// Parse a canonical string back to a @c RebuildState value.
    /// @param state_str  String produced by stateToString().
    /// @return           Parsed value, or std::nullopt on failure.
    static std::optional<RebuildState> stringToState(const std::string& state_str);
};

// ---------------------------------------------------------------------------
// UpdateModeUtils
// ---------------------------------------------------------------------------

/**
 * @brief Utility functions for @c UpdateMode enum conversions.
 */
struct UpdateModeUtils {
    /// Convert an @c UpdateMode value to its canonical string representation.
    /// @param mode  Value to convert.
    /// @return      Non-empty string; "unknown" for unrecognised values.
    static std::string modeToString(UpdateMode mode);

    /// Parse a canonical string back to an @c UpdateMode value.
    /// @param mode_str  String produced by modeToString().
    /// @return          Parsed value, or std::nullopt on failure.
    static std::optional<UpdateMode> stringToMode(const std::string& mode_str);
};

// ---------------------------------------------------------------------------
// ArtifactIntegrity
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight integrity token attached to each manifest entry.
 *
 * The checksum is a CRC-32 over the serialized payload bytes.  It is
 * verified on read to detect in-flight or at-rest corruption before an
 * artifact is handed to the planner.
 */
struct ArtifactIntegrity {
    /// CRC-32 over the artifact payload bytes; 0 means "not computed".
    uint32_t crc32 = 0;

    /// Number of payload bytes covered by @p crc32.
    uint64_t payload_bytes = 0;

    /// True when a non-zero CRC token is present (payload match is not verified here).
    [[nodiscard]] bool isValid() const noexcept { return crc32 != 0; }
};

// ---------------------------------------------------------------------------
// ArtifactManifest
// ---------------------------------------------------------------------------

/**
 * @brief A single entry in the distributed tensor artifact manifest.
 *
 * Each entry describes one advisory artifact associated with a named tensor.
 * The freshness age is derived from @p created_at relative to the caller's
 * wall-clock time; the planner must reject entries whose age exceeds the
 * configured threshold before using them for routing decisions.
 *
 * ### Example (Phase A, advisory summary):
 * @code
 * ArtifactManifest m;
 * m.artifact_id  = "ann-summary-users-v1";
 * m.tensor_name  = "users/embedding";
 * m.kind         = ArtifactKind::ADVISORY_SUMMARY;
 * m.shard_id     = 0;
 * m.version      = 1;
 * m.created_at   = std::chrono::system_clock::now();
 * m.integrity    = computeIntegrity(payload_bytes, payload_size);
 * @endcode
 */
struct ArtifactManifest {
    /// Unique identifier for this artifact (UUID or content-addressed key).
    std::string artifact_id;

    /// Logical tensor name in the format "<module>/<tensor>" (e.g. "users/embedding").
    std::string tensor_name;

    /// Classification of this artifact.
    ArtifactKind kind = ArtifactKind::ADVISORY_SUMMARY;

    /// Shard index this entry belongs to (0 = single-shard Phase A).
    uint32_t shard_id = 0;

    /// Monotonically increasing version; larger = newer.
    uint64_t version = 0;

    /// Wall-clock time when the artifact was created or last refreshed.
    std::chrono::system_clock::time_point created_at;

    /// Integrity token computed over the artifact payload.
    ArtifactIntegrity integrity;

    // -----------------------------------------------------------------------
    // Dynamic update lifecycle fields (Phase B+, populated by update worker)
    // -----------------------------------------------------------------------

    /// Reconstruction quality residual in [0.0, 1.0]; 0.0 = perfect fidelity.
    /// Values above the configured epsilon_threshold trigger advisory-only mode.
    double residual = 0.0;

    /// Current TT decomposition rank of the artifact.
    /// Compared against rank_cap to detect runaway rank growth.
    uint32_t rank_status = 0;

    /// Maximum allowed TT rank; exceeding this triggers a forced rebuild.
    uint32_t rank_cap = 256;

    /// State of the artifact after the last update cycle.
    RebuildState rebuild_state = RebuildState::PRISTINE;

    /// Strategy used by the update worker in the last update cycle.
    UpdateMode update_mode = UpdateMode::NONE;

    /// Sequence number of the first exact-graph commit covered by this artifact.
    uint64_t source_seq_start = 0;

    /// Sequence number of the last exact-graph commit reflected in this artifact.
    uint64_t source_seq_end = 0;

    /// Number of exact-graph commits not yet reflected in this artifact.
    /// High values trigger advisory-only mode in the query planner.
    uint64_t delta_lag = 0;

    /// Unix timestamp (seconds) when the artifact was last fully rebuilt.
    /// 0 means the artifact has never been rebuilt; used by staleness detection.
    int64_t last_rebuild_at_unix_sec = 0;

    /// Unix timestamp (seconds) when this manifest entry was last verified.
    /// 0 means never verified; used by @c isStale() to classify freshness.
    int64_t last_verified_unix_sec = 0;

    /// Maximum allowed age (seconds) before the artifact is considered stale.
    /// 0 disables threshold-based staleness (artifact is always fresh by age).
    int64_t staleness_threshold_sec = 0;

    // -----------------------------------------------------------------------
    // Lifecycle and invalidation fields (Phase B+)
    // -----------------------------------------------------------------------

    /// Current lifecycle state of the artifact.
    LifecycleState lifecycle_state = LifecycleState::ACTIVE;

    /// Reason the artifact was invalidated (only meaningful when
    /// @p lifecycle_state == LifecycleState::INVALIDATED).
    InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

    /**
     * @brief True when this entry has exceeded its staleness threshold.
     *
     * Uses @p last_verified_unix_sec and @p staleness_threshold_sec.
     * Returns false when @p staleness_threshold_sec is 0 (threshold disabled).
     * Returns true when @p last_verified_unix_sec is 0 (never verified).
     *
     * @param now_unix_sec  Current time as Unix timestamp (seconds).
     * @return              true if the entry is stale.
     */
    [[nodiscard]] bool isStale(int64_t now_unix_sec) const;

    /**
     * @brief True when integrity verification has detected corruption.
     *
     * Compares a freshly-computed hash of the manifest's canonical fields
     * (all fields except @p manifest_hash itself) against the stored
     * @p manifest_hash.  Returns false when @p manifest_hash is empty
     * (hash not yet computed).
     *
     * @return true if the computed hash diverges from @p manifest_hash.
     */
    [[nodiscard]] bool isCorrupted() const;

    /**
     * @brief Freshness age in seconds relative to @p now.
     *
     * @param now  Reference time point (default: wall clock).
     * @return     Age in seconds; negative values indicate clock skew and
     *             should be treated as stale.
     */
    [[nodiscard]] double freshnessAgeSeconds(
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        using namespace std::chrono;
        const auto delta = now - created_at;
        return duration_cast<duration<double>>(delta).count();
    }

    /**
     * @brief True when this entry is within the given staleness budget.
     *
     * @param max_age_s  Maximum allowed age in seconds.
     * @param now        Reference time point (default: wall clock).
     * @return           true if the entry is fresh enough for planner use.
     */
    [[nodiscard]] bool isFresh(
        double max_age_s,
        std::chrono::system_clock::time_point now =
            std::chrono::system_clock::now()) const noexcept {
        const double age = freshnessAgeSeconds(now);
        return age >= 0.0 && age <= max_age_s;
    }
};

} // namespace distributed_tensor
} // namespace themis
