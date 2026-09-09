/// @file artifact_invalidation.cc
/// @brief Implementation of artifact invalidation manager
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/artifact_invalidation.h"

namespace themis {
namespace distributed_tensor {

bool ArtifactInvalidationManager::shouldInvalidateForStaleness(const ArtifactManifest& manifest,
                                                               int64_t now_unix_sec) const {
  return manifest.isStale(now_unix_sec);
}

bool ArtifactInvalidationManager::shouldInvalidateForCorruption(const ArtifactManifest& manifest) const {
  return manifest.isCorrupted();
}

bool ArtifactInvalidationManager::shouldInvalidateForRankBreach(const ArtifactManifest& manifest) const {
  return manifest.rank_status > manifest.rank_cap && manifest.rank_cap > 0;
}

bool ArtifactInvalidationManager::shouldInvalidateForResidual(const ArtifactManifest& manifest,
                                                              double residual_threshold) const {
  return manifest.residual > residual_threshold;
}

ArtifactManifest ArtifactInvalidationManager::markStale(const ArtifactManifest& manifest) const {
  ArtifactManifest updated = manifest;
  updated.lifecycle_state = LifecycleState::STALE;
  return updated;
}

ArtifactManifest ArtifactInvalidationManager::invalidate(const ArtifactManifest& manifest,
                                                         InvalidationReason reason) const {
  ArtifactManifest updated = manifest;
  updated.lifecycle_state = LifecycleState::INVALIDATED;
  updated.invalidation_reason = reason;
  return updated;
}

ArtifactManifest ArtifactInvalidationManager::transitionToRebuilding(
    const ArtifactManifest& manifest,
    UpdateMode mode,
    int64_t now_unix_sec) const {
  ArtifactManifest updated = manifest;
  updated.lifecycle_state = LifecycleState::REBUILDING;
  updated.update_mode = mode;
  updated.updated_at_unix_sec = now_unix_sec;
  return updated;
}

ArtifactManifest ArtifactInvalidationManager::transitionToReadyAfterRebuild(
    const ArtifactManifest& manifest,
    RebuildState rebuild_state,
    uint64_t source_seq_start,
    uint64_t source_seq_end,
    int64_t now_unix_sec) const {
  ArtifactManifest updated = manifest;
  updated.lifecycle_state = LifecycleState::READY;
  updated.rebuild_state = rebuild_state;
  updated.source_seq_start = source_seq_start;
  updated.source_seq_end = source_seq_end;
  updated.delta_lag = 0;
  updated.artifact_age_ms = 0;
  updated.invalidation_reason = InvalidationReason::UNKNOWN;
  updated.last_rebuild_at_unix_sec = now_unix_sec;
  updated.last_verified_unix_sec = now_unix_sec;
  updated.updated_at_unix_sec = now_unix_sec;
  return updated;
}

ArtifactManifest ArtifactInvalidationManager::transitionToFailed(
    const ArtifactManifest& manifest,
    InvalidationReason reason,
    int64_t now_unix_sec) const {
  ArtifactManifest updated = manifest;
  updated.lifecycle_state = LifecycleState::FAILED;
  updated.invalidation_reason = reason;
  updated.updated_at_unix_sec = now_unix_sec;
  return updated;
}

bool ArtifactInvalidationManager::shouldRejectForPlanner(
    const ArtifactManifest& manifest,
    int64_t now_unix_sec,
    uint64_t max_delta_lag,
    double max_residual) const {
  if (!ArtifactLifecyclePolicy::isUsableForPlanning(manifest.lifecycle_state)) {
    return true;
  }
  if (manifest.isStale(now_unix_sec)) {
    return true;
  }
  if (max_delta_lag > 0 && manifest.delta_lag > max_delta_lag) {
    return true;
  }
  if (max_residual >= 0.0 && manifest.residual > max_residual) {
    return true;
  }
  return false;
}

std::vector<std::string> ArtifactInvalidationManager::getCascadeInvalidationTargets(
    const std::string& source_artifact_id) const {
  // Placeholder for cascade logic
  // Real implementation would query artifact dependency graph
  std::vector<std::string> targets;
  return targets;
}

}  // namespace distributed_tensor
}  // namespace themis
