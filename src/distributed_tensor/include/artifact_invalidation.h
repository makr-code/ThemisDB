/// @file artifact_invalidation.h
/// @brief Artifact invalidation policy and manager for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines artifact invalidation policies and the manager that
/// applies invalidation decisions based on artifact state transitions.
///

#pragma once

#include "src/distributed_tensor/include/artifact_manifest.h"
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace themis {
namespace distributed_tensor {

/// @brief Artifact invalidation manager for marking stale/invalid artifacts.
///
/// Handles invalidation triggers such as:
/// - Staleness threshold exceeded
/// - Integrity check failures
/// - Source artifact invalidation (cascade)
/// - Rank cap breach
/// - Residual threshold breach
///
class ArtifactInvalidationManager {
 public:
  ArtifactInvalidationManager() = default;
  virtual ~ArtifactInvalidationManager() = default;

  // Prevent copy/move
  ArtifactInvalidationManager(const ArtifactInvalidationManager&) = delete;
  ArtifactInvalidationManager& operator=(const ArtifactInvalidationManager&) = delete;
  ArtifactInvalidationManager(ArtifactInvalidationManager&&) = delete;
  ArtifactInvalidationManager& operator=(ArtifactInvalidationManager&&) = delete;

  /// Checks if artifact should be invalidated based on staleness.
  /// @param manifest Artifact manifest to check
  /// @param now_unix_sec Current time
  /// @return true if artifact exceeds staleness_threshold_sec
  virtual bool shouldInvalidateForStaleness(const ArtifactManifest& manifest, int64_t now_unix_sec) const;

  /// Checks if artifact integrity has been compromised.
  /// @param manifest Artifact manifest to check
  /// @return true if corruption detected
  virtual bool shouldInvalidateForCorruption(const ArtifactManifest& manifest) const;

  /// Checks if artifact should be invalidated due to rank cap breach.
  /// @param manifest Artifact manifest to check
  /// @return true if rank_status exceeds rank_cap
  virtual bool shouldInvalidateForRankBreach(const ArtifactManifest& manifest) const;

  /// Checks if artifact should be invalidated due to residual threshold.
  /// @param manifest Artifact manifest to check
  /// @param residual_threshold Maximum acceptable residual
  /// @return true if residual exceeds threshold
  virtual bool shouldInvalidateForResidual(const ArtifactManifest& manifest,
                                           double residual_threshold) const;

  /// Marks artifact as stale (still usable but should be refreshed).
  /// @param manifest Artifact manifest to mark stale
  /// @return Updated manifest
  virtual ArtifactManifest markStale(const ArtifactManifest& manifest) const;

  /// Marks artifact as invalidated (no longer usable).
  /// @param manifest Artifact manifest to invalidate
  /// @param reason Reason for invalidation
  /// @return Updated manifest
  virtual ArtifactManifest invalidate(const ArtifactManifest& manifest, InvalidationReason reason) const;

  /// Cascades invalidation to derived artifacts (for source invalidation).
  /// @param source_artifact_id ID of source artifact that was invalidated
  /// @return List of derived artifact IDs that should also be invalidated
  virtual std::vector<std::string> getCascadeInvalidationTargets(const std::string& source_artifact_id) const;
};

}  // namespace distributed_tensor
}  // namespace themis
