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

enum class LifecycleState : uint8_t {
    PRISTINE = 0,

    READY = 1,

    STALE = 2,

    INVALIDATED = 3,

    REBUILDING = 4,

    FAILED = 5,
};

// ---------------------------------------------------------------------------
// InvalidationReason — why an artifact was invalidated
// ---------------------------------------------------------------------------

enum class InvalidationReason : uint8_t {
    UNKNOWN = 0,

    INTEGRITY_CHECK_FAILED = 1,

    STALENESS_EXCEEDED = 2,

    SOURCE_INVALIDATED = 3,

    SOURCE_LINEAGE_CORRUPTED = 4,

    POLICY_VIOLATION = 5,

    ADMIN_REQUESTED = 6,

    SHARD_UNAVAILABLE = 7,

    SCHEMA_INCOMPATIBLE = 8,
};

// ---------------------------------------------------------------------------
// StalenessPolicy — configurable staleness thresholds
// ---------------------------------------------------------------------------

class StalenessPolicy {
 public:
    StalenessPolicy() = default;

    /**
     * @brief With Age Threshold Ms.
     * @param[in] threshold_ms Input parameter.
     * @return Return value.
     * @details Implements withAgeThresholdMs without additional internal calls.
     */
    StalenessPolicy& withAgeThresholdMs(std::uint32_t threshold_ms) {
        age_threshold_ms_ = threshold_ms;
        return *this;
    }

    /**
     * @brief With Delta Lag Threshold.
     * @param[in] threshold Input parameter.
     * @return Return value.
     * @details Implements withDeltaLagThreshold without additional internal calls.
     */
    StalenessPolicy& withDeltaLagThreshold(std::uint64_t threshold) {
        delta_lag_threshold_ = threshold;
        return *this;
    }

    /**
     * @brief With Residual Threshold.
     * @param[in] threshold Input parameter.
     * @return Return value.
     * @details Implements withResidualThreshold without additional internal calls.
     */
    StalenessPolicy& withResidualThreshold(double threshold) {
        residual_threshold_ = threshold;
        return *this;
    }

    /**
     * @brief With Rank Cap Threshold.
     * @param[in] threshold Input parameter.
     * @return Return value.
     * @details Implements withRankCapThreshold without additional internal calls.
     */
    StalenessPolicy& withRankCapThreshold(std::uint32_t threshold) {
        rank_cap_threshold_ = threshold;
        return *this;
    }

    /**
     * @brief With Residual Variance Threshold.
     * @param[in] threshold Input parameter.
     * @return Return value.
     * @details Implements withResidualVarianceThreshold without additional internal calls.
     */
    StalenessPolicy& withResidualVarianceThreshold(double threshold) {
        residual_variance_threshold_ = threshold;
        return *this;
    }

    [[nodiscard]] std::optional<std::uint32_t> ageThresholdMs() const {
        return age_threshold_ms_;
    }

    [[nodiscard]] std::optional<std::uint64_t> deltaLagThreshold() const {
        return delta_lag_threshold_;
    }

    [[nodiscard]] std::optional<double> residualThreshold() const {
        return residual_threshold_;
    }

    [[nodiscard]] std::optional<std::uint32_t> rankCapThreshold() const {
        return rank_cap_threshold_;
    }

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

struct LifecycleMetadata {
    std::string artifact_id;

    LifecycleState state = LifecycleState::PRISTINE;

    InvalidationReason invalidation_reason = InvalidationReason::UNKNOWN;

    std::uint64_t source_seq_start = 0;

    std::uint64_t source_seq_end = 0;

    std::uint64_t delta_lag = 0;

    std::uint32_t artifact_age_ms = 0;

    double approximation_residual = 0.0;

    double residual_variance = 0.0;

    std::uint32_t max_permissible_rank = 0;

    std::uint64_t state_change_timestamp_ms = 0;

    std::uint32_t rebuild_attempt_count = 0;

    std::optional<std::uint64_t> last_successful_rebuild_ms;

    std::optional<std::uint64_t> last_failed_rebuild_ms;
};

// ---------------------------------------------------------------------------
// ArtifactLifecycleManager — core lifecycle and staleness API
// ---------------------------------------------------------------------------

class ArtifactLifecycleManager {
 public:
    ArtifactLifecycleManager() = default;

    ArtifactLifecycleManager(const ArtifactLifecycleManager&) = delete;
    ArtifactLifecycleManager& operator=(const ArtifactLifecycleManager&) = delete;

    ArtifactLifecycleManager(ArtifactLifecycleManager&&) = default;
    ArtifactLifecycleManager& operator=(ArtifactLifecycleManager&&) = default;

    // -----------------------------------------------------------------------
    // Lifecycle State Computation
    // -----------------------------------------------------------------------

    [[nodiscard]] LifecycleState computeState(
        const LifecycleMetadata& metadata,
        const StalenessPolicy& policy
    ) const noexcept;

    [[nodiscard]] static bool isUsableForPlanning(LifecycleState state) noexcept;

    [[nodiscard]] static bool requiresImmediateRebuild(LifecycleState state) noexcept;

    // -----------------------------------------------------------------------
    // State Transitions
    // -----------------------------------------------------------------------

    [[nodiscard]] static LifecycleMetadata invalidate(
        LifecycleMetadata metadata,
        InvalidationReason reason
    ) noexcept;

    [[nodiscard]] static LifecycleMetadata beginRebuild(
        LifecycleMetadata metadata
    ) noexcept;

    [[nodiscard]] static LifecycleMetadata completeRebuildSuccess(
        LifecycleMetadata metadata,
        std::uint32_t new_age_ms,
        std::uint64_t new_delta_lag,
        double new_residual
    ) noexcept;

    [[nodiscard]] static LifecycleMetadata completeRebuildFailure(
        LifecycleMetadata metadata
    ) noexcept;

    [[nodiscard]] static LifecycleMetadata markReady(
        LifecycleMetadata metadata,
        std::uint32_t age_ms,
        std::uint64_t delta_lag,
        double residual
    ) noexcept;

    // -----------------------------------------------------------------------
    // Staleness Detection
    // -----------------------------------------------------------------------

    [[nodiscard]] std::optional<std::string> diagnoseStalenessCause(
        const LifecycleMetadata& metadata,
        const StalenessPolicy& policy
    ) const noexcept;

    // -----------------------------------------------------------------------
    // Batch Operations
    // -----------------------------------------------------------------------

    [[nodiscard]] std::vector<LifecycleState> computeStatesBatch(
        const std::vector<LifecycleMetadata>& metadata_batch,
        const StalenessPolicy& policy
    ) const noexcept;

    [[nodiscard]] std::vector<LifecycleMetadata> filterUsableArtifacts(
        const std::vector<LifecycleMetadata>& metadata_batch,
        const StalenessPolicy& policy
    ) const noexcept;

    [[nodiscard]] std::vector<LifecycleMetadata> identifyRebuildCandidates(
        const std::vector<LifecycleMetadata>& metadata_batch
    ) const noexcept;
};

// ---------------------------------------------------------------------------
// Utility Functions — string conversion
// ---------------------------------------------------------------------------

[[nodiscard]] std::string lifecycleStateToString(LifecycleState state) noexcept;

[[nodiscard]] std::optional<LifecycleState> stringToLifecycleState(
    const std::string& state_str
) noexcept;

[[nodiscard]] std::string invalidationReasonToString(InvalidationReason reason) noexcept;

[[nodiscard]] std::optional<InvalidationReason> stringToInvalidationReason(
    const std::string& reason_str
) noexcept;

}  // namespace evaluation
}  // namespace themis
