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

class ArtifactInvalidationManager {
 public:
  ArtifactInvalidationManager() = default;
  /**
   * @brief Artifact Invalidation Manager.
   * @return Return value.
   */
  virtual ~ArtifactInvalidationManager() = default;

  // Prevent copy/move
  ArtifactInvalidationManager(const ArtifactInvalidationManager&) = delete;
  ArtifactInvalidationManager& operator=(const ArtifactInvalidationManager&) = delete;
  ArtifactInvalidationManager(ArtifactInvalidationManager&&) = delete;
  ArtifactInvalidationManager& operator=(ArtifactInvalidationManager&&) = delete;

  /**
   * @brief Should Invalidate For Staleness.
   * @param[in] manifest Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldInvalidateForStaleness(const ArtifactManifest& manifest, int64_t now_unix_sec) const;

  /**
   * @brief Should Invalidate For Corruption.
   * @param[in] manifest Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldInvalidateForCorruption(const ArtifactManifest& manifest) const;

  /**
   * @brief Should Invalidate For Rank Breach.
   * @param[in] manifest Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldInvalidateForRankBreach(const ArtifactManifest& manifest) const;

  /**
   * @brief Should Invalidate For Residual.
   * @param[in] manifest Input parameter.
   * @param[in] residual_threshold Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldInvalidateForResidual(const ArtifactManifest& manifest,
                                           double residual_threshold) const;

  /**
   * @brief Mark Stale.
   * @param[in] manifest Input parameter.
   * @return Return value.
   */
  virtual ArtifactManifest markStale(const ArtifactManifest& manifest) const;

  /**
   * @brief Invalidate.
   * @param[in] manifest Input parameter.
   * @param[in] reason Input parameter.
   * @return Return value.
   */
  virtual ArtifactManifest invalidate(const ArtifactManifest& manifest, InvalidationReason reason) const;

  /**
   * @brief Transition To Rebuilding.
   * @param[in] manifest Input parameter.
   * @param[in] mode Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @return Return value.
   */
  virtual ArtifactManifest transitionToRebuilding(const ArtifactManifest& manifest,
                                                  UpdateMode mode,
                                                  int64_t now_unix_sec) const;

  /**
   * @brief Transition To Ready After Rebuild.
   * @param[in] manifest Input parameter.
   * @param[in] rebuild_state Input parameter.
   * @param[in] source_seq_start Input parameter.
   * @param[in] source_seq_end Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @return Return value.
   */
  virtual ArtifactManifest transitionToReadyAfterRebuild(const ArtifactManifest& manifest,
                                                         RebuildState rebuild_state,
                                                         uint64_t source_seq_start,
                                                         uint64_t source_seq_end,
                                                         int64_t now_unix_sec) const;

  /**
   * @brief Transition To Failed.
   * @param[in] manifest Input parameter.
   * @param[in] reason Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @return Return value.
   */
  virtual ArtifactManifest transitionToFailed(const ArtifactManifest& manifest,
                                              InvalidationReason reason,
                                              int64_t now_unix_sec) const;

  /**
   * @brief Should Reject For Planner.
   * @param[in] manifest Input parameter.
   * @param[in] now_unix_sec Input parameter.
   * @param[in] max_delta_lag Input parameter.
   * @param[in] max_residual Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool shouldRejectForPlanner(const ArtifactManifest& manifest,
                                      int64_t now_unix_sec,
                                      uint64_t max_delta_lag,
                                      double max_residual) const;

  /**
   * @brief Get Cascade Invalidation Targets.
   * @param[in] source_artifact_id Identifier of the source artifact.
   * @return Return value.
   */
  virtual std::vector<std::string> getCascadeInvalidationTargets(const std::string& source_artifact_id) const;
};

}  // namespace distributed_tensor
}  // namespace themis
