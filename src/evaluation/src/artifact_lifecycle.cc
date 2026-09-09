/**
 * @file artifact_lifecycle.cc
 * @brief Implementation of EPIC 2.6 Artifact Lifecycle management.
 */

#include "../include/artifact_lifecycle.h"

#include <algorithm>
#include <cassert>
#include <chrono>

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// ArtifactLifecycleManager Implementation
// ---------------------------------------------------------------------------

LifecycleState ArtifactLifecycleManager::computeState(
    const LifecycleMetadata& metadata,
    const StalenessPolicy& policy
) const noexcept {
    // If already in a terminal or intermediate state, preserve it unless
    // we have evidence of rebuild completion.
    const auto current_state = metadata.state;
    if (current_state == LifecycleState::INVALIDATED ||
        current_state == LifecycleState::REBUILDING ||
        current_state == LifecycleState::FAILED) {
        return current_state;
    }

    // If state is PRISTINE or READY, check staleness thresholds.
    if (current_state == LifecycleState::PRISTINE ||
        current_state == LifecycleState::READY) {

        // Check age threshold
        if (auto age_threshold = policy.ageThresholdMs()) {
            if (metadata.artifact_age_ms >= *age_threshold) {
                return LifecycleState::STALE;
            }
        }

        // Check delta lag threshold
        if (auto delta_threshold = policy.deltaLagThreshold()) {
            if (metadata.delta_lag >= *delta_threshold) {
                return LifecycleState::STALE;
            }
        }

        // Check residual threshold
        if (auto residual_threshold = policy.residualThreshold()) {
            if (metadata.approximation_residual >= *residual_threshold) {
                return LifecycleState::STALE;
            }
        }

        // Check rank cap threshold
        if (auto rank_threshold = policy.rankCapThreshold()) {
            if (metadata.max_permissible_rank < *rank_threshold) {
                return LifecycleState::STALE;
            }
        }

        // Check residual variance threshold
        if (auto variance_threshold = policy.residualVarianceThreshold()) {
            if (metadata.residual_variance >= *variance_threshold) {
                return LifecycleState::STALE;
            }
        }

        // No staleness threshold exceeded; remain in current state
        return current_state == LifecycleState::PRISTINE
            ? LifecycleState::READY
            : LifecycleState::READY;
    }

    // STALE state: preserve it (artifacts don't un-stale without explicit action)
    return LifecycleState::STALE;
}

bool ArtifactLifecycleManager::isUsableForPlanning(LifecycleState state) noexcept {
    return state == LifecycleState::READY || state == LifecycleState::STALE;
}

bool ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState state) noexcept {
    return state == LifecycleState::INVALIDATED || state == LifecycleState::FAILED;
}

LifecycleMetadata ArtifactLifecycleManager::invalidate(
    LifecycleMetadata metadata,
    InvalidationReason reason
) noexcept {
    metadata.state = LifecycleState::INVALIDATED;
    metadata.invalidation_reason = reason;
    metadata.state_change_timestamp_ms = std::chrono::system_clock::now()
                                             .time_since_epoch()
                                             .count() /
                                         1'000'000;  // Convert nanoseconds to ms
    return metadata;
}

LifecycleMetadata ArtifactLifecycleManager::beginRebuild(
    LifecycleMetadata metadata
) noexcept {
    metadata.state = LifecycleState::REBUILDING;
    metadata.rebuild_attempt_count++;
    metadata.state_change_timestamp_ms = std::chrono::system_clock::now()
                                             .time_since_epoch()
                                             .count() /
                                         1'000'000;  // Convert nanoseconds to ms
    return metadata;
}

LifecycleMetadata ArtifactLifecycleManager::completeRebuildSuccess(
    LifecycleMetadata metadata,
    std::uint32_t new_age_ms,
    std::uint64_t new_delta_lag,
    double new_residual
) noexcept {
    metadata.state = LifecycleState::READY;
    metadata.artifact_age_ms = new_age_ms;
    metadata.delta_lag = new_delta_lag;
    metadata.approximation_residual = new_residual;
    metadata.last_successful_rebuild_ms = std::chrono::system_clock::now()
                                               .time_since_epoch()
                                               .count() /
                                           1'000'000;  // Convert nanoseconds to ms
    metadata.state_change_timestamp_ms = *metadata.last_successful_rebuild_ms;
    return metadata;
}

LifecycleMetadata ArtifactLifecycleManager::completeRebuildFailure(
    LifecycleMetadata metadata
) noexcept {
    metadata.state = LifecycleState::FAILED;
    metadata.last_failed_rebuild_ms = std::chrono::system_clock::now()
                                          .time_since_epoch()
                                          .count() /
                                      1'000'000;  // Convert nanoseconds to ms
    metadata.state_change_timestamp_ms = *metadata.last_failed_rebuild_ms;
    return metadata;
}

LifecycleMetadata ArtifactLifecycleManager::markReady(
    LifecycleMetadata metadata,
    std::uint32_t age_ms,
    std::uint64_t delta_lag,
    double residual
) noexcept {
    metadata.state = LifecycleState::READY;
    metadata.artifact_age_ms = age_ms;
    metadata.delta_lag = delta_lag;
    metadata.approximation_residual = residual;
    metadata.state_change_timestamp_ms = std::chrono::system_clock::now()
                                             .time_since_epoch()
                                             .count() /
                                         1'000'000;  // Convert nanoseconds to ms
    return metadata;
}

std::optional<std::string> ArtifactLifecycleManager::diagnoseStalenessCause(
    const LifecycleMetadata& metadata,
    const StalenessPolicy& policy
) const noexcept {
    // Check age threshold
    if (auto age_threshold = policy.ageThresholdMs()) {
        if (metadata.artifact_age_ms >= *age_threshold) {
            return "Age threshold exceeded: " + std::to_string(metadata.artifact_age_ms) +
                   "ms >= " + std::to_string(*age_threshold) + "ms";
        }
    }

    // Check delta lag threshold
    if (auto delta_threshold = policy.deltaLagThreshold()) {
        if (metadata.delta_lag >= *delta_threshold) {
            return "Delta lag threshold exceeded: " + std::to_string(metadata.delta_lag) +
                   " >= " + std::to_string(*delta_threshold);
        }
    }

    // Check residual threshold
    if (auto residual_threshold = policy.residualThreshold()) {
        if (metadata.approximation_residual >= *residual_threshold) {
            return "Residual threshold exceeded: " +
                   std::to_string(metadata.approximation_residual) + " >= " +
                   std::to_string(*residual_threshold);
        }
    }

    // Check rank cap threshold
    if (auto rank_threshold = policy.rankCapThreshold()) {
        if (metadata.max_permissible_rank < *rank_threshold) {
            return "Rank cap threshold breached: " +
                   std::to_string(metadata.max_permissible_rank) + " < " +
                   std::to_string(*rank_threshold);
        }
    }

    // Check residual variance threshold
    if (auto variance_threshold = policy.residualVarianceThreshold()) {
        if (metadata.residual_variance >= *variance_threshold) {
            return "Residual variance threshold exceeded: " +
                   std::to_string(metadata.residual_variance) + " >= " +
                   std::to_string(*variance_threshold);
        }
    }

    return std::nullopt;
}

std::vector<LifecycleState> ArtifactLifecycleManager::computeStatesBatch(
    const std::vector<LifecycleMetadata>& metadata_batch,
    const StalenessPolicy& policy
) const noexcept {
    std::vector<LifecycleState> states = {};

    states.reserve(metadata_batch.size());

    for (const auto& metadata : metadata_batch) {
        states.push_back(computeState(metadata, policy));
    }

    return states;
}

std::vector<LifecycleMetadata> ArtifactLifecycleManager::filterUsableArtifacts(
    const std::vector<LifecycleMetadata>& metadata_batch,
    const StalenessPolicy& policy
) const noexcept {
    std::vector<LifecycleMetadata> usable;

    for (const auto& metadata : metadata_batch) {
        auto state = computeState(metadata, policy);
        if (isUsableForPlanning(state)) {
            usable.push_back(metadata);
        }
    }

    return usable;
}

std::vector<LifecycleMetadata> ArtifactLifecycleManager::identifyRebuildCandidates(
    const std::vector<LifecycleMetadata>& metadata_batch
) const noexcept {
    std::vector<LifecycleMetadata> candidates;

    for (const auto& metadata : metadata_batch) {
        if (requiresImmediateRebuild(metadata.state)) {
            candidates.push_back(metadata);
        }
    }

    return candidates;
}

// ---------------------------------------------------------------------------
// Utility Functions — String Conversion
// ---------------------------------------------------------------------------

std::string lifecycleStateToString(LifecycleState state) noexcept {
    switch (state) {
        case LifecycleState::PRISTINE:
            return "PRISTINE";
        case LifecycleState::READY:
            return "READY";
        case LifecycleState::STALE:
            return "STALE";
        case LifecycleState::INVALIDATED:
            return "INVALIDATED";
        case LifecycleState::REBUILDING:
            return "REBUILDING";
        case LifecycleState::FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
    }
}

std::optional<LifecycleState> stringToLifecycleState(
    const std::string& state_str
) noexcept {
    if (state_str == "PRISTINE") {
        return LifecycleState::PRISTINE;
    } else if (state_str == "READY") {
        return LifecycleState::READY;
    } else if (state_str == "STALE") {
        return LifecycleState::STALE;
    } else if (state_str == "INVALIDATED") {
        return LifecycleState::INVALIDATED;
    } else if (state_str == "REBUILDING") {
        return LifecycleState::REBUILDING;
    } else if (state_str == "FAILED") {
        return LifecycleState::FAILED;
    }
    return std::nullopt;
}

std::string invalidationReasonToString(InvalidationReason reason) noexcept {
    switch (reason) {
        case InvalidationReason::UNKNOWN:
            return "UNKNOWN";
        case InvalidationReason::INTEGRITY_CHECK_FAILED:
            return "INTEGRITY_CHECK_FAILED";
        case InvalidationReason::STALENESS_EXCEEDED:
            return "STALENESS_EXCEEDED";
        case InvalidationReason::SOURCE_INVALIDATED:
            return "SOURCE_INVALIDATED";
        case InvalidationReason::SOURCE_LINEAGE_CORRUPTED:
            return "SOURCE_LINEAGE_CORRUPTED";
        case InvalidationReason::POLICY_VIOLATION:
            return "POLICY_VIOLATION";
        case InvalidationReason::ADMIN_REQUESTED:
            return "ADMIN_REQUESTED";
        case InvalidationReason::SHARD_UNAVAILABLE:
            return "SHARD_UNAVAILABLE";
        case InvalidationReason::SCHEMA_INCOMPATIBLE:
            return "SCHEMA_INCOMPATIBLE";
        default:
            return "UNKNOWN";
    }
}

std::optional<InvalidationReason> stringToInvalidationReason(
    const std::string& reason_str
) noexcept {
    if (reason_str == "UNKNOWN") {
        return InvalidationReason::UNKNOWN;
    } else if (reason_str == "INTEGRITY_CHECK_FAILED") {
        return InvalidationReason::INTEGRITY_CHECK_FAILED;
    } else if (reason_str == "STALENESS_EXCEEDED") {
        return InvalidationReason::STALENESS_EXCEEDED;
    } else if (reason_str == "SOURCE_INVALIDATED") {
        return InvalidationReason::SOURCE_INVALIDATED;
    } else if (reason_str == "SOURCE_LINEAGE_CORRUPTED") {
        return InvalidationReason::SOURCE_LINEAGE_CORRUPTED;
    } else if (reason_str == "POLICY_VIOLATION") {
        return InvalidationReason::POLICY_VIOLATION;
    } else if (reason_str == "ADMIN_REQUESTED") {
        return InvalidationReason::ADMIN_REQUESTED;
    } else if (reason_str == "SHARD_UNAVAILABLE") {
        return InvalidationReason::SHARD_UNAVAILABLE;
    } else if (reason_str == "SCHEMA_INCOMPATIBLE") {
        return InvalidationReason::SCHEMA_INCOMPATIBLE;
    }
    return std::nullopt;
}

}  // namespace evaluation
}  // namespace themis
