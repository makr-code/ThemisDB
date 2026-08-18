/**
 * @file artifact_lifecycle.h
 * @brief Derived artifact lifecycle, staleness policy, and invalidation management.
 *
 * This module defines the EPIC 2.6 Artifact Lifecycle contract for evaluation and
 * planner consumption. It provides the freshness semantics, rebuild governance, and
 * staleness detection hooks needed to coordinate retrieval quality with storage and
 * compute constraints across the layered retrieval architecture.
 *
 * ## Lifecycle States
 *
 * Each artifact transitions through a finite state machine:
 *
 * ```
 *   PRISTINE (new artifact)
 *       ↓
 *   READY (in use)
 *       ↓
 *   STALE (exceeded freshness threshold)
 *       ↓
 *   INVALIDATED (marked for rebuild)
 *       ↓
 *   REBUILDING (background rebuild in progress)
 *       ↙↘
 *   READY ← ↓ → FAILED (rebuild unsuccessful)
 * ```
 *
 * ## Staleness Policy
 *
 * A staleness policy defines thresholds for when an artifact transitions from
 * READY to STALE. Multiple overlapping policies may apply (age, delta lag,
 * residual error, rank cap). The artifact becomes STALE if any policy fires.
 *
 * ## Integration Points
 *
 * - **Query Planner**: Consumes artifact lifecycle state to make routing decisions
 * - **Recovery Manager**: Uses lifecycle state to drive rebuild priorities
 * - **Artifact Manifest**: Stores lifecycle metadata alongside artifact content
 * - **Background Workers**: Drive INVALIDATED → REBUILDING → READY transitions
 *
 * ## Usage Example
 *
 * ```cpp
 * themis::evaluation::ArtifactLifecycleManager lifecycle_mgr;
 *
 * // Create a staleness policy: artifact becomes STALE after 5 seconds or
 * // if delta lag exceeds 1000 operations
 * auto policy = themis::evaluation::StalenessPolicy()
 *     .withAgeThresholdMs(5000)
 *     .withDeltaLagThreshold(1000);
 *
 * // Check if an artifact is usable for planning
 * auto state = lifecycle_mgr.computeState(artifact_metadata, policy);
 * if (lifecycle_mgr.isUsableForPlanning(state)) {
 *     // Use the artifact as a candidate source
 * } else {
 *     // Fall back to exact graph traversal
 * }
 *
 * // Mark an artifact for rebuild (e.g., after integrity check failure)
 * lifecycle_mgr.invalidate(manifest, InvalidationReason::INTEGRITY_CHECK_FAILED);
 * ```
 *
 * @see docs/EPIC2_ARTIFACT_LIFECYCLE.md
 * @see src/distributed_tensor/include/artifact_manifest.h
 * @see src/evaluation/include/query_planner.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// LifecycleState — artifact FSM states
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of possible artifact lifecycle states.
 *
 * Artifacts progress through these states based on freshness policies,
 * invalidation triggers, and rebuild outcomes.
 */
enum class LifecycleState : uint8_t {
    /// Artifact has just been created; not yet in the active pool.
    PRISTINE = 0,

    /// Artifact is current and usable for planning decisions.
    READY = 1,

    /// Artifact exceeds a freshness/lag threshold but remains usable for advisory decisions.
    STALE = 2,

    /// Artifact has been explicitly invalidated; must not be used.
    INVALIDATED = 3,

    /// A rebuild or re-materialization is actively in progress.
    REBUILDING = 4,

    /// Rebuild attempt failed; artifact cannot be used until the next rebuild.
    FAILED = 5,
};

// ---------------------------------------------------------------------------
// InvalidationReason — why an artifact was invalidated
// ---------------------------------------------------------------------------

/**
 * @brief Enumeration of reasons why an artifact was invalidated.
 *
 * Used for diagnostics, audit trails, and rebuild prioritization.
 */
enum class InvalidationReason : uint8_t {
    /// Reason not recorded or unknown.
    UNKNOWN = 0,

    /// Artifact failed CRC-32 or cryptographic integrity check.
    INTEGRITY_CHECK_FAILED = 1,

    /// Artifact age or delta lag exceeded configured thresholds.
    STALENESS_EXCEEDED = 2,

    /// Parent artifact or source-of-truth artifact was invalidated (cascade).
    SOURCE_INVALIDATED = 3,

    /// Provenance chain is broken or evidence of tampering detected.
    SOURCE_LINEAGE_CORRUPTED = 4,

    /// Rank cap or residual approximation error threshold was breached.
    POLICY_VIOLATION = 5,

    /// Operator or admin explicitly requested invalidation.
    ADMIN_REQUESTED = 6,

    /// One or more shards are unavailable; artifact cannot be reliably accessed.
    SHARD_UNAVAILABLE = 7,

    /// Artifact is incompatible with current schema or codec version.
    SCHEMA_INCOMPATIBLE = 8,
};

// ---------------------------------------------------------------------------
// StalenessPolicy — configurable staleness thresholds
// ---------------------------------------------------------------------------

/**
 * @brief Configurable freshness thresholds for artifact staleness detection.
 *
 * An artifact becomes STALE if any threshold is exceeded. All thresholds are
 * optional; omitted thresholds are ignored.
 *
 * @invariant If age_threshold_ms is set, artifact_age_ms >= age_threshold_ms
 *            causes staleness.
 * @invariant If delta_lag_threshold is set, delta_lag >= delta_lag_threshold
 *            causes staleness.
 * @invariant If residual_threshold is set, approximation_residual >= residual_threshold
 *            causes staleness.
 * @invariant If rank_cap_threshold is set, max_permissible_rank < rank_cap_threshold
 *            causes staleness.
 */
class StalenessPolicy {
 public:
    StalenessPolicy() = default;

    /// @brief Set the artifact age threshold in milliseconds.
    StalenessPolicy& withAgeThresholdMs(std::uint32_t threshold_ms) {
        age_threshold_ms_ = threshold_ms;
        return *this;
    }

    /// @brief Set the delta lag threshold (gap between artifact end and exact-graph head).
    StalenessPolicy& withDeltaLagThreshold(std::uint64_t threshold) {
        delta_lag_threshold_ = threshold;
        return *this;
    }

    /// @brief Set the approximation residual threshold (quality metric).
    StalenessPolicy& withResidualThreshold(double threshold) {
        residual_threshold_ = threshold;
        return *this;
    }

    /// @brief Set the rank cap threshold for partial-refit paths.
    StalenessPolicy& withRankCapThreshold(std::uint32_t threshold) {
        rank_cap_threshold_ = threshold;
        return *this;
    }

    /// @brief Set the maximum acceptable residual variance.
    StalenessPolicy& withResidualVarianceThreshold(double threshold) {
        residual_variance_threshold_ = threshold;
        return *this;
    }

    /// @brief Retrieve the age threshold (if set).
    [[nodiscard]] std::optional<std::uint32_t> ageThresholdMs() const {
        return age_threshold_ms_;
    }

    /// @brief Retrieve the delta lag threshold (if set).
    [[nodiscard]] std::optional<std::uint64_t> deltaLagThreshold() const {
        return delta_lag_threshold_;
    }

    /// @brief Retrieve the residual threshold (if set).
    [[nodiscard]] std::optional<double> residualThreshold() const {
        return residual_threshold_;
    }

    /// @brief Retrieve the rank cap threshold (if set).
    [[nodiscard]] std::optional<std::uint32_t> rankCapThreshold() const {
        return rank_cap_threshold_;
    }

    /// @brief Retrieve the residual variance threshold (if set).
    [[nodiscard]] std::optional<double> residualVarianceThreshold() const {
        return residual_variance_threshold_;
    }

 private:
    std::optional<std::uint32_t> age_threshold_ms_;
    std::optional<std::uint64_t> delta_lag_threshold_;
    std::optional<double> residual_threshold_;
    std::optional<std::uint32_t> rank_cap_threshold_;
    std::optional<double> residual_variance_threshold_;
};

// ---------------------------------------------------------------------------
// LifecycleMetadata — artifact freshness tracking
// ---------------------------------------------------------------------------

/**
 * @brief Captures lifecycle and freshness metadata for a single artifact.
 *
 * This structure holds all information needed to compute lifecycle state
 * transitions and staleness decisions.
 */
struct LifecycleMetadata {
    /// Artifact ID (unique within deployment).
    std::string artifact_id;

    /// Current lifecycle state.
    LifecycleState state = LifecycleState::PRISTINE;

    /// Reason for last invalidation (if applicable).
    InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

    /// Sequence window start (inclusive).
    std::uint64_t source_seq_start = 0;

    /// Sequence window end (inclusive).
    std::uint64_t source_seq_end = 0;

    /// Gap between source_seq_end and current exact-graph head.
    std::uint64_t delta_lag = 0;

    /// Age of artifact in milliseconds at last update.
    std::uint32_t artifact_age_ms = 0;

    /// Approximation quality metric (higher = more error).
    double approximation_residual = 0.0;

    /// Variance in approximation residual (statistical uncertainty).
    double residual_variance = 0.0;

    /// Maximum permissible rank for partial-refit paths.
    std::uint32_t max_permissible_rank = 0;

    /// Timestamp when state last changed (milliseconds since epoch).
    std::uint64_t state_change_timestamp_ms = 0;

    /// Number of rebuild attempts for this artifact.
    std::uint32_t rebuild_attempt_count = 0;

    /// Timestamp of the last successful rebuild (if applicable).
    std::optional<std::uint64_t> last_successful_rebuild_ms;

    /// Timestamp of the last failed rebuild (if applicable).
    std::optional<std::uint64_t> last_failed_rebuild_ms;
};

// ---------------------------------------------------------------------------
// ArtifactLifecycleManager — core lifecycle and staleness API
// ---------------------------------------------------------------------------

/**
 * @brief Manages artifact lifecycle state transitions and staleness detection.
 *
 * This class provides the core decision logic for determining when artifacts
 * should transition from READY to STALE, when they should be invalidated, and
 * when rebuilds should be prioritized.
 */
class ArtifactLifecycleManager {
 public:
    ArtifactLifecycleManager() = default;

    /// Delete copy semantics; lifecycle managers are typically stateful.
    ArtifactLifecycleManager(const ArtifactLifecycleManager&) = delete;
    ArtifactLifecycleManager& operator=(const ArtifactLifecycleManager&) = delete;

    /// Allow move semantics.
    ArtifactLifecycleManager(ArtifactLifecycleManager&&) = default;
    ArtifactLifecycleManager& operator=(ArtifactLifecycleManager&&) = default;

    // -----------------------------------------------------------------------
    // Lifecycle State Computation
    // -----------------------------------------------------------------------

    /**
     * @brief Compute the lifecycle state of an artifact given current metadata
     *        and staleness policy.
     *
     * This function implements the state machine logic:
     * - If state is already INVALIDATED, REBUILDING, or FAILED, return as-is
     *   unless rebuild is complete.
     * - If state is READY and any staleness threshold is exceeded, return STALE.
     * - Otherwise, return current state.
     *
     * @param metadata Current lifecycle metadata.
     * @param policy Staleness policy to apply.
     * @return The computed lifecycle state.
     */
    [[nodiscard]] LifecycleState computeState(
        const LifecycleMetadata& metadata,
        const StalenessPolicy& policy
    ) const noexcept;

    /**
     * @brief Determine if an artifact is usable for planning decisions.
     *
     * @invariant Only READY and STALE artifacts are usable for planning.
     * @invariant INVALIDATED, REBUILDING, and FAILED artifacts force exact
     *            graph fallback.
     *
     * @param state Lifecycle state to evaluate.
     * @return true if the artifact can be used as a candidate source, false otherwise.
     */
    [[nodiscard]] static bool isUsableForPlanning(LifecycleState state) noexcept;

    /**
     * @brief Determine if an artifact requires immediate rebuild.
     *
     * @param state Lifecycle state to evaluate.
     * @return true if the artifact is INVALIDATED or FAILED and needs rebuild.
     */
    [[nodiscard]] static bool requiresImmediateRebuild(LifecycleState state) noexcept;

    // -----------------------------------------------------------------------
    // State Transitions
    // -----------------------------------------------------------------------

    /**
     * @brief Transition an artifact to INVALIDATED state.
     *
     * @param metadata Metadata to update (state and invalidation_reason modified).
     * @param reason Why the artifact was invalidated.
     * @return Updated metadata with state = INVALIDATED.
     */
    [[nodiscard]] static LifecycleMetadata invalidate(
        LifecycleMetadata metadata,
        InvalidationReason reason
    ) noexcept;

    /**
     * @brief Transition an artifact to REBUILDING state.
     *
     * @param metadata Metadata to update.
     * @return Updated metadata with state = REBUILDING and rebuild_attempt_count incremented.
     */
    [[nodiscard]] static LifecycleMetadata beginRebuild(
        LifecycleMetadata metadata
    ) noexcept;

    /**
     * @brief Transition an artifact to READY state after successful rebuild.
     *
     * @param metadata Metadata to update.
     * @param new_age_ms Age of the rebuilt artifact in milliseconds.
     * @param new_delta_lag Delta lag of the rebuilt artifact.
     * @param new_residual Approximation residual of the rebuilt artifact.
     * @return Updated metadata with state = READY and refresh metadata.
     */
    [[nodiscard]] static LifecycleMetadata completeRebuildSuccess(
        LifecycleMetadata metadata,
        std::uint32_t new_age_ms,
        std::uint64_t new_delta_lag,
        double new_residual
    ) noexcept;

    /**
     * @brief Transition an artifact to FAILED state after rebuild failure.
     *
     * @param metadata Metadata to update.
     * @return Updated metadata with state = FAILED.
     */
    [[nodiscard]] static LifecycleMetadata completeRebuildFailure(
        LifecycleMetadata metadata
    ) noexcept;

    /**
     * @brief Mark an artifact as READY (used for initial artifact creation).
     *
     * @param metadata Metadata to update.
     * @param age_ms Initial age in milliseconds.
     * @param delta_lag Initial delta lag.
     * @param residual Initial approximation residual.
     * @return Updated metadata with state = READY.
     */
    [[nodiscard]] static LifecycleMetadata markReady(
        LifecycleMetadata metadata,
        std::uint32_t age_ms,
        std::uint64_t delta_lag,
        double residual
    ) noexcept;

    // -----------------------------------------------------------------------
    // Staleness Detection
    // -----------------------------------------------------------------------

    /**
     * @brief Determine which staleness threshold (if any) was exceeded.
     *
     * @param metadata Artifact metadata to check.
     * @param policy Staleness policy to apply.
     * @return A string describing which threshold was exceeded (if any),
     *         or std::nullopt if no threshold was exceeded.
     */
    [[nodiscard]] std::optional<std::string> diagnoseStalenessCause(
        const LifecycleMetadata& metadata,
        const StalenessPolicy& policy
    ) const noexcept;

    // -----------------------------------------------------------------------
    // Batch Operations
    // -----------------------------------------------------------------------

    /**
     * @brief Compute lifecycle states for a batch of artifacts.
     *
     * @param metadata_batch Vector of artifact metadata.
     * @param policy Staleness policy to apply to all.
     * @return Vector of computed states, parallel to input.
     */
    [[nodiscard]] std::vector<LifecycleState> computeStatesBatch(
        const std::vector<LifecycleMetadata>& metadata_batch,
        const StalenessPolicy& policy
    ) const noexcept;

    /**
     * @brief Filter a batch of artifacts to only those usable for planning.
     *
     * @param metadata_batch Vector of artifact metadata.
     * @param policy Staleness policy to apply.
     * @return Filtered metadata vector containing only usable artifacts.
     */
    [[nodiscard]] std::vector<LifecycleMetadata> filterUsableArtifacts(
        const std::vector<LifecycleMetadata>& metadata_batch,
        const StalenessPolicy& policy
    ) const noexcept;

    /**
     * @brief Identify artifacts that require rebuild.
     *
     * @param metadata_batch Vector of artifact metadata.
     * @return Vector of metadata for artifacts in INVALIDATED or FAILED state.
     */
    [[nodiscard]] std::vector<LifecycleMetadata> identifyRebuildCandidates(
        const std::vector<LifecycleMetadata>& metadata_batch
    ) const noexcept;
};

// ---------------------------------------------------------------------------
// Utility Functions — string conversion
// ---------------------------------------------------------------------------

/**
 * @brief Convert LifecycleState enum to human-readable string.
 *
 * @param state The lifecycle state.
 * @return String representation (e.g., "READY", "STALE", "INVALIDATED").
 */
[[nodiscard]] std::string lifecycleStateToString(LifecycleState state) noexcept;

/**
 * @brief Parse a string to LifecycleState enum.
 *
 * @param state_str String representation.
 * @return Parsed enum, or std::nullopt if string is unrecognized.
 */
[[nodiscard]] std::optional<LifecycleState> stringToLifecycleState(
    const std::string& state_str
) noexcept;

/**
 * @brief Convert InvalidationReason enum to human-readable string.
 *
 * @param reason The invalidation reason.
 * @return String representation (e.g., "INTEGRITY_CHECK_FAILED", "STALENESS_EXCEEDED").
 */
[[nodiscard]] std::string invalidationReasonToString(InvalidationReason reason) noexcept;

/**
 * @brief Parse a string to InvalidationReason enum.
 *
 * @param reason_str String representation.
 * @return Parsed enum, or std::nullopt if string is unrecognized.
 */
[[nodiscard]] std::optional<InvalidationReason> stringToInvalidationReason(
    const std::string& reason_str
) noexcept;

}  // namespace evaluation
}  // namespace themis
