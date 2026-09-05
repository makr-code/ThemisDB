/// @file artifact_invalidation.h
/// @brief Artifact invalidation policy and manager for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines artifact invalidation policies and the manager that
/// applies invalidation decisions based on artifact state transitions.
///

#pragma once

#include "artifact_manifest.h"
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

  /// Transitions an artifact into REBUILDING state for a selected update path.
  /// @param manifest Artifact manifest to transition
  /// @param mode Requested update mode (patch / partial refit / rebuild)
  /// @param now_unix_sec Transition timestamp
  /// @return Updated manifest in REBUILDING state
  virtual ArtifactManifest transitionToRebuilding(const ArtifactManifest& manifest,
                                                  UpdateMode mode,
                                                  int64_t now_unix_sec) const;

  /// Finalizes a successful rebuild and rematerializes the artifact as READY.
  /// @param manifest Artifact manifest currently in rebuild flow
  /// @param rebuild_state Rebuild provenance to record
  /// @param source_seq_start New source window start (inclusive)
  /// @param source_seq_end New source window end (inclusive)
  /// @param now_unix_sec Completion timestamp
  /// @return Updated manifest in READY state with reset lag/age fields
  virtual ArtifactManifest transitionToReadyAfterRebuild(const ArtifactManifest& manifest,
                                                         RebuildState rebuild_state,
                                                         uint64_t source_seq_start,
                                                         uint64_t source_seq_end,
                                                         int64_t now_unix_sec) const;

  /// Marks an artifact as FAILED when rebuild/rematerialization fails.
  /// @param manifest Artifact manifest to mark failed
  /// @param reason Failure reason to persist for observability
  /// @param now_unix_sec Transition timestamp
  /// @return Updated manifest in FAILED state
  virtual ArtifactManifest transitionToFailed(const ArtifactManifest& manifest,
                                              InvalidationReason reason,
                                              int64_t now_unix_sec) const;

  /// Planner freshness gate: determines whether planner must reject this artifact.
  /// @param manifest Artifact manifest to evaluate
  /// @param now_unix_sec Current timestamp used for age-based staleness checks
  /// @param max_delta_lag Maximum acceptable lag; 0 disables lag gate
  /// @param max_residual Maximum acceptable residual; negative disables residual gate
  /// @return true if planner should reject and fall back to exact graph
  virtual bool shouldRejectForPlanner(const ArtifactManifest& manifest,
                                      int64_t now_unix_sec,
                                      uint64_t max_delta_lag,
                                      double max_residual) const;

  /// Cascades invalidation to derived artifacts (for source invalidation).
  /// @param source_artifact_id ID of source artifact that was invalidated
  /// @return List of derived artifact IDs that should also be invalidated
  virtual std::vector<std::string> getCascadeInvalidationTargets(const std::string& source_artifact_id) const;
};

}  // namespace distributed_tensor
}  // namespace themis
