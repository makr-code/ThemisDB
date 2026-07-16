/// @file artifact_invalidation.cc
/// @brief Implementation of artifact invalidation manager
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "src/distributed_tensor/include/artifact_invalidation.h"

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

std::vector<std::string> ArtifactInvalidationManager::getCascadeInvalidationTargets(
    const std::string& source_artifact_id) const {
  // Placeholder for cascade logic
  // Real implementation would query artifact dependency graph
  std::vector<std::string> targets;
  return targets;
}

}  // namespace distributed_tensor
}  // namespace themis
