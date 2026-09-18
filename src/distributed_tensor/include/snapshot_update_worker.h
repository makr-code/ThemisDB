/// @file snapshot_update_worker.h
/// @brief Snapshot-based update worker for dynamic tensor artifact refreshing
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines the snapshot-based update worker that consumes delta windows
/// and decides whether to patch, partially refit, or fully rebuild tensor artifacts.
///
/// ## Design Philosophy
///
/// The update worker is:
/// - **Asynchronous**: Operates independently from the query path
/// - **Intelligent**: Decides update strategy based on delta analysis
/// - **Safe**: Never makes tensor artifacts truth-bearing; always advisory-only
/// - **Observable**: Publishes metrics on update decisions and quality
/// - **Recoverable**: Crash-safe with checkpointing
///
/// ## Workflow
///
/// 1. Extract delta window from TensorDeltaLog
/// 2. Analyze delta characteristics (size, mutation distribution, etc.)
/// 3. Decide update strategy: PATCH, PARTIAL_REFIT, or REBUILD
/// 4. Execute the strategy asynchronously
/// 5. Create new artifact version with updated manifest
/// 6. Atomically publish new manifest to ManifestStore
/// 7. Collect metrics for observability
///
/// ## Update Decision Logic
///
/// - **PATCH**: When delta_size < 10% of artifact_size
///   - Best for: Sporadic, small updates (e.g., fixing outliers)
///   - Cost: O(delta_size)
///
/// - **PARTIAL_REFIT**: When 10% <= delta_size <= 50% of artifact_size
///   - Best for: Moderate updates (e.g., domain adaptation, LoRA tuning)
///   - Cost: O(k) where k is the subset being retrained
///
/// - **REBUILD**: When delta_size > 50% of artifact_size
///   - Best for: Major changes (e.g., new training data, model swap)
///   - Cost: O(n) where n is full artifact size
///   - Fallback when partial refit fails or exceeds residual threshold
///

#pragma once

#include "artifact_manifest.h"
#include "tensor_delta_log.h"
#include "manifest_store.h"
#include "crash_recovery_checkpoint.h"
#include "distributed_lock_manager.h"
#include "stale_artifact_detector.h"
#include "error_recovery_handler.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <cstdint>
#include <atomic>

namespace themis {
namespace distributed_tensor {

enum class UpdateWorkerState : uint8_t {
  IDLE = 0,

  READY = 1,

  PROCESSING = 2,

  ERROR = 3,

  SHUTTING_DOWN = 4,
};

enum class UpdateDecision : uint8_t {
  NO_UPDATE = 0,

  PATCH = 1,

  PARTIAL_REFIT = 2,

  REBUILD = 3,

  ERROR_FALLBACK_TO_REBUILD = 4,
};

struct UpdateTask {
  std::string artifact_id;

  DeltaWindow delta_window;

  ArtifactManifest current_manifest;

  uint64_t artifact_size_bytes = 0;
};

struct UpdateMetrics {
  UpdateDecision decision = UpdateDecision::NO_UPDATE;

  int64_t analysis_time_ms = 0;

  int64_t execution_time_ms = 0;

  double resulting_residual = 0.0;

  uint32_t resulting_rank_status = 0;

  bool success = false;

  std::string error_message;

  double throughput_deltas_per_sec = 0.0;
};

class SnapshotBasedUpdateWorker {
 public:
  explicit SnapshotBasedUpdateWorker(ManifestStore* manifest_store = nullptr) noexcept;

  /**
   * @brief Snapshot Based Update Worker.
   * @return Return value.
   */
  virtual ~SnapshotBasedUpdateWorker() = default;

  // Prevent copy/move to maintain worker state invariants
  SnapshotBasedUpdateWorker(const SnapshotBasedUpdateWorker&) = delete;
  SnapshotBasedUpdateWorker& operator=(const SnapshotBasedUpdateWorker&) = delete;
  SnapshotBasedUpdateWorker(SnapshotBasedUpdateWorker&&) = delete;
  SnapshotBasedUpdateWorker& operator=(SnapshotBasedUpdateWorker&&) = delete;

  /**
   * @brief Start.
   * @return True when the operation succeeds.
   */
  virtual bool start();

  /**
   * @brief Process Task.
   * @param[in] task Input parameter.
   * @param[in,out] metrics Input/output parameter.
   * @return Return value.
   */
  virtual UpdateDecision processTask(const UpdateTask& task, UpdateMetrics& metrics);

  /**
   * @brief Process Delta Window.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] delta_window Input parameter.
   * @param[in] current_manifest Input parameter.
   * @param[in] artifact_size_bytes Input parameter.
   * @return Return value.
   */
  virtual UpdateDecision processDeltaWindow(const std::string& artifact_id,
                                            const DeltaWindow& delta_window,
                                            const ArtifactManifest& current_manifest,
                                            uint64_t artifact_size_bytes);

  /**
   * @brief Decide Update Strategy.
   * @param[in] delta_window Input parameter.
   * @param[in] artifact_size_bytes Input parameter.
   * @param[in] current_residual Input parameter.
   * @return Return value.
   */
  virtual UpdateDecision decideUpdateStrategy(const DeltaWindow& delta_window,
                                              uint64_t artifact_size_bytes,
                                              double current_residual);

  /**
   * @brief Execute Patch.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] delta_window Input parameter.
   * @param[in,out] current_manifest Input/output parameter.
   * @return True when the operation succeeds.
   */
  virtual bool executePatch(const std::string& artifact_id,
                            const DeltaWindow& delta_window,
                            ArtifactManifest& current_manifest);

  /**
   * @brief Execute Partial Refit.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] delta_window Input parameter.
   * @param[in,out] current_manifest Input/output parameter.
   * @return True when the operation succeeds.
   */
  virtual bool executePartialRefit(const std::string& artifact_id,
                                   const DeltaWindow& delta_window,
                                   ArtifactManifest& current_manifest);

  /**
   * @brief Execute Rebuild.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] delta_window Input parameter.
   * @param[in,out] current_manifest Input/output parameter.
   * @return True when the operation succeeds.
   */
  virtual bool executeRebuild(const std::string& artifact_id,
                              const DeltaWindow& delta_window,
                              ArtifactManifest& current_manifest);

  /**
   * @brief Publish Manifest.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] new_manifest Input parameter.
   * @param[in] old_version Input parameter.
   * @param[in] reason Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool publishManifest(const std::string& artifact_id,
                               const ArtifactManifest& new_manifest,
                               uint64_t old_version,
                               const std::string& reason);

  /**
   * @brief Shutdown.
   * @return True when the operation succeeds.
   */
  virtual bool shutdown();

  /**
   * @brief Get State.
   * @return Return value.
   */
  UpdateWorkerState getState() const;

  struct Stats {
    uint64_t total_tasks_processed = 0;
    uint64_t total_patches_applied = 0;
    uint64_t total_partial_refits = 0;
    uint64_t total_rebuilds = 0;
    uint64_t total_failed_updates = 0;
    double average_decision_time_ms = 0.0;
    double average_execution_time_ms = 0.0;
    int64_t last_activity_ms = 0;
  };

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  virtual Stats getStats() const;

  /**
   * @brief Set Decision Thresholds.
   * @param[in] patch_threshold_pct Input parameter.
   * @param[in] refit_threshold_pct Input parameter.
   * @param[in] residual_max_increase_allowed Input parameter.
   */
  void setDecisionThresholds(double patch_threshold_pct,
                             double refit_threshold_pct,
                             double residual_max_increase_allowed);

  /**
   * @brief Set Checkpoint Path.
   * @param[in] checkpoint_path Path to the checkpoint.
   */
  void setCheckpointPath(const std::string& checkpoint_path);

  /**
   * @brief Set Lock Manager.
   * @param[in] lock_manager Input parameter.
   */
  void setLockManager(std::shared_ptr<DistributedLockManager> lock_manager);

  /**
   * @brief Set Stale Artifact Detector.
   * @param[in] detector Input parameter.
   */
  void setStaleArtifactDetector(std::shared_ptr<StaleArtifactDetector> detector);

  /**
   * @brief Set Error Recovery Handler.
   * @param[in] handler Input parameter.
   */
  void setErrorRecoveryHandler(std::shared_ptr<ErrorRecoveryHandler> handler);

  /**
   * @brief Set Manifest Store.
   * @param[in,out] manifest_store Input/output parameter.
   * @note Exception safety: noexcept.
   */
  void setManifestStore(ManifestStore* manifest_store) noexcept;

  /**
   * @brief Recover From Checkpoint.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  virtual std::optional<ArtifactManifest> recoverFromCheckpoint(const std::string& artifact_id);

  /**
   * @brief Save Checkpoint.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] task Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool saveCheckpoint(const std::string& artifact_id, const UpdateTask& task);

  virtual bool acquireUpdateLock(const std::string& artifact_id,
                                 const std::string& lock_reason = "",
                                 int64_t ttl_seconds = 3600);

  /**
   * @brief Release Update Lock.
   * @param[in] artifact_id Identifier of the artifact.
   * @return True when the operation succeeds.
   */
  virtual bool releaseUpdateLock(const std::string& artifact_id);

  virtual bool renewUpdateLock(const std::string& artifact_id, int64_t ttl_seconds = 3600);

  /**
   * @brief Detect Staleness.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] current_manifest Input parameter.
   * @param[in] current_source_seq Input parameter.
   * @return Return value.
   */
  virtual StaleArtifactMetrics detectStaleness(const std::string& artifact_id,
                                               const ArtifactManifest& current_manifest,
                                               uint64_t current_source_seq);

  virtual bool isValidForPatchingPublic(const DeltaWindow& delta_window,
                                        int64_t max_age_ms = 3600000) const;

  /**
   * @brief Detect Instability Public.
   * @param[in] delta_window Input parameter.
   * @param[in] current_residual Input parameter.
   * @return True when the operation succeeds.
   */
  virtual bool detectInstabilityPublic(const DeltaWindow& delta_window,
                                       double current_residual) const;

  virtual bool isDeltaLogOverflowingPublic(size_t current_entries,
                                           uint32_t max_entries = 100000) const;

 protected:
  UpdateWorkerState state_ = UpdateWorkerState::IDLE;
  Stats stats_;
  double patch_threshold_pct_ = 10.0;
  double refit_threshold_pct_ = 50.0;
  double residual_max_increase_allowed_ = 0.05;
  std::string checkpoint_path_;
  std::unique_ptr<CrashRecoveryCheckpoint> checkpoint_manager_;
  std::shared_ptr<DistributedLockManager> lock_manager_;
  std::shared_ptr<StaleArtifactDetector> stale_detector_;
  std::shared_ptr<ErrorRecoveryHandler> error_handler_;
  ManifestStore* manifest_store_ = nullptr;
  std::string worker_id_;  // Unique worker identifier for locking

  /**
   * @brief Would Breach Rank Cap.
   * @param[in] manifest Input parameter.
   * @param[in] delta_window Input parameter.
   * @return True when the operation succeeds.
   */
  bool wouldBreachRankCap(const ArtifactManifest& manifest, const DeltaWindow& delta_window);

  /**
   * @brief Estimate Resulting Residual.
   * @param[in] delta_window Input parameter.
   * @param[in] current_residual Input parameter.
   * @param[in] decision Input parameter.
   * @return Return value.
   */
  double estimateResultingResidual(const DeltaWindow& delta_window,
                                   double current_residual,
                                   UpdateDecision decision);
};

}  // namespace distributed_tensor
}  // namespace themis
