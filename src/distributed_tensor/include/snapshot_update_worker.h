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
#include "src/distributed_tensor/include/tensor_delta_log.h"
#include "src/distributed_tensor/include/manifest_store.h"
#include "src/distributed_tensor/include/crash_recovery_checkpoint.h"
#include "src/distributed_tensor/include/distributed_lock_manager.h"
#include "src/distributed_tensor/include/stale_artifact_detector.h"
#include "src/distributed_tensor/include/error_recovery_handler.h"
#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <cstdint>
#include <atomic>

namespace themis {
namespace distributed_tensor {

/// @brief Update worker state machine for artifact refresh lifecycle.
enum class UpdateWorkerState : uint8_t {
  /// Worker is not running.
  IDLE = 0,

  /// Worker is initialized and ready to process deltas.
  READY = 1,

  /// Worker is currently processing a delta window.
  PROCESSING = 2,

  /// Worker has encountered an error and is paused.
  ERROR = 3,

  /// Worker is shutting down gracefully.
  SHUTTING_DOWN = 4,
};

/// @brief Result of an update decision.
enum class UpdateDecision : uint8_t {
  /// No update needed (artifact is fresh enough).
  NO_UPDATE = 0,

  /// Apply patch updates (small delta).
  PATCH = 1,

  /// Apply partial refit (medium delta, selective retraining).
  PARTIAL_REFIT = 2,

  /// Full rebuild required (large delta or quality issues).
  REBUILD = 3,

  /// Update failed or inconclusive (fallback to rebuild).
  ERROR_FALLBACK_TO_REBUILD = 4,
};

/// @brief Update task for the worker to process.
struct UpdateTask {
  /// Artifact to update.
  std::string artifact_id;

  /// Delta window to process.
  DeltaWindow delta_window;

  /// Current manifest of the artifact.
  ArtifactManifest current_manifest;

  /// Estimated artifact size in bytes (for decision logic).
  uint64_t artifact_size_bytes = 0;
};

/// @brief Update metrics collected during processing.
struct UpdateMetrics {
  /// Decision that was made (PATCH, PARTIAL_REFIT, REBUILD).
  UpdateDecision decision = UpdateDecision::NO_UPDATE;

  /// Time spent analyzing deltas (milliseconds).
  int64_t analysis_time_ms = 0;

  /// Time spent executing update (milliseconds).
  int64_t execution_time_ms = 0;

  /// Quality residual of resulting artifact ([0.0, 1.0]).
  double resulting_residual = 0.0;

  /// Quality rank status after update.
  uint32_t resulting_rank_status = 0;

  /// Whether the update succeeded.
  bool success = false;

  /// Optional error message if update failed.
  std::string error_message;

  /// Estimated throughput: deltas processed per second.
  double throughput_deltas_per_sec = 0.0;
};

/// @brief SnapshotBasedUpdateWorker: Asynchronous tensor artifact refresher.
///
/// Consumes delta windows from the TensorDeltaLog and decides the optimal
/// update strategy (patch, partial_refit, rebuild) for each artifact.
/// Publishes updated manifests atomically to the ManifestStore.
///
class SnapshotBasedUpdateWorker {
 public:
  /// Creates a new update worker.
  explicit SnapshotBasedUpdateWorker(ManifestStore* manifest_store = nullptr) noexcept;

  virtual ~SnapshotBasedUpdateWorker() = default;

  // Prevent copy/move to maintain worker state invariants
  SnapshotBasedUpdateWorker(const SnapshotBasedUpdateWorker&) = delete;
  SnapshotBasedUpdateWorker& operator=(const SnapshotBasedUpdateWorker&) = delete;
  SnapshotBasedUpdateWorker(SnapshotBasedUpdateWorker&&) = delete;
  SnapshotBasedUpdateWorker& operator=(SnapshotBasedUpdateWorker&&) = delete;

  /// Starts the worker (initializes resources).
  /// @return true on success, false on error
  virtual bool start();

  /// Processes a single update task and returns decision/metrics.
  /// @param task Update task to process
  /// @param[out] metrics Metrics collected during processing
  /// @return Update decision (PATCH, PARTIAL_REFIT, REBUILD, or ERROR_FALLBACK)
  virtual UpdateDecision processTask(const UpdateTask& task, UpdateMetrics& metrics);

  /// Processes a delta window for the specified artifact.
  /// @param artifact_id Artifact identifier
  /// @param delta_window Delta window to process
  /// @param current_manifest Current manifest of the artifact
  /// @param artifact_size_bytes Size of the artifact for decision logic
  /// @return Update decision
  virtual UpdateDecision processDeltaWindow(const std::string& artifact_id,
                                            const DeltaWindow& delta_window,
                                            const ArtifactManifest& current_manifest,
                                            uint64_t artifact_size_bytes);

  /// Decides update strategy based on delta characteristics.
  /// @param delta_window Delta window to analyze
  /// @param artifact_size_bytes Total artifact size
  /// @param current_residual Current quality residual
  /// @return Decision: PATCH, PARTIAL_REFIT, or REBUILD
  virtual UpdateDecision decideUpdateStrategy(const DeltaWindow& delta_window,
                                              uint64_t artifact_size_bytes,
                                              double current_residual);

  /// Applies patch updates to an artifact.
  /// @param artifact_id Artifact identifier
  /// @param delta_window Patch deltas to apply
  /// @param current_manifest Current manifest
  /// @return true on success, false on error
  virtual bool executePatch(const std::string& artifact_id,
                            const DeltaWindow& delta_window,
                            ArtifactManifest& current_manifest);

  /// Applies partial refit to an artifact.
  /// @param artifact_id Artifact identifier
  /// @param delta_window Deltas triggering refit
  /// @param current_manifest Current manifest
  /// @return true on success, false on error
  virtual bool executePartialRefit(const std::string& artifact_id,
                                   const DeltaWindow& delta_window,
                                   ArtifactManifest& current_manifest);

  /// Triggers full rebuild of an artifact from source.
  /// @param artifact_id Artifact identifier
  /// @param delta_window Deltas triggering rebuild
  /// @param current_manifest Current manifest
  /// @return true on success, false on error
  virtual bool executeRebuild(const std::string& artifact_id,
                              const DeltaWindow& delta_window,
                              ArtifactManifest& current_manifest);

  /// Publishes updated manifest to ManifestStore atomically.
  /// @param artifact_id Artifact identifier
  /// @param new_manifest Updated manifest
  /// @param old_version Version of manifest being replaced
  /// @param reason Reason for publish (e.g., "patched", "rebuilt")
  /// @return true on success, false on CAS failure
  virtual bool publishManifest(const std::string& artifact_id,
                               const ArtifactManifest& new_manifest,
                               uint64_t old_version,
                               const std::string& reason);

  /// Shuts down the worker gracefully.
  /// @return true on success, false on error
  virtual bool shutdown();

  /// Returns the current worker state.
  UpdateWorkerState getState() const;

  /// Returns statistics about the worker's activity.
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

  /// Returns current worker statistics.
  virtual Stats getStats() const;

  /// Sets the decision thresholds for update strategy selection.
  /// @param patch_threshold_pct Percentage threshold for patch vs refit (default: 10%)
  /// @param refit_threshold_pct Percentage threshold for refit vs rebuild (default: 50%)
  /// @param residual_max_increase_allowed Maximum residual increase from refit (default: 0.05)
  void setDecisionThresholds(double patch_threshold_pct,
                             double refit_threshold_pct,
                             double residual_max_increase_allowed);

  /// Sets checkpoint path for crash recovery.
  /// @param checkpoint_path Path to store checkpoint files
  void setCheckpointPath(const std::string& checkpoint_path);

  /// Sets the lock manager for distributed locking.
  /// @param lock_manager Shared lock manager instance
  void setLockManager(std::shared_ptr<DistributedLockManager> lock_manager);

  /// Sets the stale artifact detector.
  /// @param detector Shared detector instance
  void setStaleArtifactDetector(std::shared_ptr<StaleArtifactDetector> detector);

  /// Sets the error recovery handler.
  /// @param handler Shared error recovery handler instance
  void setErrorRecoveryHandler(std::shared_ptr<ErrorRecoveryHandler> handler);

  /// Sets the advisory manifest store used for publish operations.
  /// @param manifest_store Store to receive updated manifests; may be nullptr
  void setManifestStore(ManifestStore* manifest_store) noexcept;

  /// Recovers from a crash by loading checkpoint.
  /// @param artifact_id Artifact to recover
  /// @return Recovered manifest if recovery successful, std::nullopt if no checkpoint or error
  /// Note: Returns nullopt in these cases:
  ///   - No checkpoint found (not an error)
  ///   - Checkpoint corrupted and deleted
  ///   - Validation failed (invalid manifest, corrupted data, etc.)
  virtual std::optional<ArtifactManifest> recoverFromCheckpoint(const std::string& artifact_id);

  /// Saves a checkpoint before starting long-running operation.
  /// @param artifact_id Artifact being processed
  /// @param task Update task being executed
  /// @return true on success, false on error
  virtual bool saveCheckpoint(const std::string& artifact_id, const UpdateTask& task);

  /// Acquires exclusive lock for artifact update.
  /// @param artifact_id Artifact to lock
  /// @param lock_reason Reason for the lock (e.g., "partial_refit", "rebuild")
  /// @param ttl_seconds Time-to-live for lock in seconds
  /// @return true if lock acquired, false otherwise
  virtual bool acquireUpdateLock(const std::string& artifact_id,
                                 const std::string& lock_reason = "",
                                 int64_t ttl_seconds = 3600);

  /// Releases a lock for an artifact.
  /// @param artifact_id Artifact to unlock
  /// @return true on success, false otherwise
  virtual bool releaseUpdateLock(const std::string& artifact_id);

  /// Renews a lock during long-running updates.
  /// @param artifact_id Artifact whose lock to renew
  /// @param ttl_seconds New time-to-live
  /// @return true on success, false otherwise
  virtual bool renewUpdateLock(const std::string& artifact_id, int64_t ttl_seconds = 3600);

  /// Detects and handles stale artifacts.
  /// @param artifact_id Artifact to check
  /// @param current_manifest Current manifest state
  /// @param current_source_seq Current sequence number in exact graph
  /// @return Staleness metrics
  virtual StaleArtifactMetrics detectStaleness(const std::string& artifact_id,
                                               const ArtifactManifest& current_manifest,
                                               uint64_t current_source_seq);

  /// Phase B: Checks if a delta window is valid for patching operations.
  /// @param delta_window Window to validate
  /// @param max_age_ms Maximum allowed age of window (default 1 hour)
  /// @return true if window is valid for patching, false otherwise
  virtual bool isValidForPatchingPublic(const DeltaWindow& delta_window,
                                        int64_t max_age_ms = 3600000) const;

  /// Phase B: Detects instability in delta patterns (e.g., thrashing).
  /// @param delta_window Window to analyze
  /// @param current_residual Current residual of the artifact
  /// @return true if instability detected, false otherwise
  virtual bool detectInstabilityPublic(const DeltaWindow& delta_window,
                                       double current_residual) const;

  /// Phase B: Checks if delta log appears to be overflowing.
  /// @param current_entries Current number of entries in delta log
  /// @param max_entries Maximum allowed entries (default 100000)
  /// @return true if overflow imminent (>95% of limit), false otherwise
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

  /// Internal helper to check if rank cap would be breached.
  bool wouldBreachRankCap(const ArtifactManifest& manifest, const DeltaWindow& delta_window);

  /// Internal helper to estimate resulting residual after update.
  double estimateResultingResidual(const DeltaWindow& delta_window,
                                   double current_residual,
                                   UpdateDecision decision);
};

}  // namespace distributed_tensor
}  // namespace themis
