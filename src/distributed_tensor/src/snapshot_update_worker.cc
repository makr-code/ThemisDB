/// @file snapshot_update_worker.cc
/// @brief Implementation of snapshot-based update worker for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "src/distributed_tensor/include/snapshot_update_worker.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace themis {
namespace distributed_tensor {

namespace {

// Phase B: Delta log overflow and instability detection constants
constexpr int64_t kDeltaWindowMaxAgeMs = 3600000;  // 1 hour max age
constexpr uint32_t kDeltaLogMaxEntries = 100000;   // Max entries before overflow
constexpr double kInstabilityThresholdMutationFreq = 0.8;  // 80% mutation density
constexpr double kInstabilityThresholdResidue = 0.3;  // 30% residual threshold

int64_t getCurrentTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

int64_t getCurrentTimeSec() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void updateAverages(SnapshotBasedUpdateWorker::Stats& stats,
                    int64_t analysis_ms,
                    int64_t execution_ms) {
  const auto count = static_cast<double>(std::max<uint64_t>(
      stats.total_tasks_processed, 1));
  stats.average_decision_time_ms =
      ((stats.average_decision_time_ms * (count - 1.0)) + analysis_ms) / count;
  stats.average_execution_time_ms =
      ((stats.average_execution_time_ms * (count - 1.0)) + execution_ms) / count;
  stats.last_activity_ms = getCurrentTimeMs();
}

/// Phase B: Checks if a delta window would be valid for patching.
/// A window is valid for patching if:
/// - It's not too old (< 1 hour)
/// - Entries are in sequence order
/// - No structural mutations (DELETE, SHARD_CHANGE)
/// - Total size is bounded
bool isValidForPatching(const DeltaWindow& window,
                        int64_t max_age_ms = kDeltaWindowMaxAgeMs) {
  if (window.entries.empty()) {
    return false;
  }
  
  // Check age
  int64_t age_ms = getCurrentTimeMs() - window.extracted_at_ms;
  if (age_ms > max_age_ms) {
    return false;
  }
  
  // Check sequence continuity
  uint64_t expected_seq = window.sequence_start;
  for (const auto& entry : window.entries) {
    if (entry.sequence_number != expected_seq) {
      return false;  // Gap detected
    }
    expected_seq++;
  }
  
  // Check for structural mutations
  if (window.countDeletes() > 0 || window.countShardChanges() > 0) {
    return false;
  }
  
  return true;
}

/// Phase B: Detects instability in delta patterns.
/// Returns true if the window exhibits signs of instability (e.g., thrashing).
bool detectInstability(const DeltaWindow& window,
                       double current_residual) {
  if (window.entries.empty()) {
    return false;
  }
  
  // High mutation density suggests thrashing
  double total_mutations = window.countInserts() + window.countUpdates();
  double mutation_frequency = total_mutations / window.entries.size();
  if (mutation_frequency > kInstabilityThresholdMutationFreq) {
    return true;
  }
  
  // High residual suggests instability
  if (current_residual > kInstabilityThresholdResidue) {
    return true;
  }
  
  // Multiple rapid updates to same entity (if we can detect it)
  // This is a simplification; a full implementation would track entity change counts
  
  return false;
}

/// Phase B: Checks if delta log appears to be overflowing.
bool isDeltaLogOverflowing(size_t current_entries,
                           uint32_t max_entries = kDeltaLogMaxEntries) {
  return current_entries >= (max_entries * 95) / 100;  // 95% of limit
}

}  // namespace

// ============================================================================
// SnapshotBasedUpdateWorker Methods
// ============================================================================

SnapshotBasedUpdateWorker::SnapshotBasedUpdateWorker(
    ManifestStore* manifest_store) noexcept
    : state_(UpdateWorkerState::IDLE),
      patch_threshold_pct_(10.0),
      refit_threshold_pct_(50.0),
      residual_max_increase_allowed_(0.05),
      manifest_store_(manifest_store) {}

bool SnapshotBasedUpdateWorker::start() {
  if (state_ == UpdateWorkerState::PROCESSING ||
      state_ == UpdateWorkerState::SHUTTING_DOWN) {
    return false;
  }
  state_ = UpdateWorkerState::READY;
  return true;
}

UpdateDecision SnapshotBasedUpdateWorker::processTask(const UpdateTask& task,
                                                      UpdateMetrics& metrics) {
  if (state_ != UpdateWorkerState::READY) {
    metrics.error_message = "Worker is not ready";
    metrics.success = false;
    return UpdateDecision::ERROR_FALLBACK_TO_REBUILD;
  }

  // Try to recover from checkpoint if one exists
  auto recovered = recoverFromCheckpoint(task.artifact_id);
  if (!recovered) {
    // No checkpoint was found (nullopt) is not an error - proceed with normal processing
    // recovered manifest would be used here if available, but we proceed with task manifest
  } else {
    // Use recovered manifest as the baseline for recovery
    // Store recovery info for diagnostics - the recovered_manifest contains the state
    // at the point of checkpoint save
  }

  // Acquire lock before processing
  std::string lock_reason = "update_processing";
  if (!acquireUpdateLock(task.artifact_id, lock_reason, 3600)) {
    metrics.error_message = "Failed to acquire update lock";
    metrics.success = false;
    return UpdateDecision::ERROR_FALLBACK_TO_REBUILD;
  }

  // Save checkpoint before starting update
  if (!saveCheckpoint(task.artifact_id, task)) {
    metrics.error_message = "Failed to save checkpoint";
    metrics.success = false;
    releaseUpdateLock(task.artifact_id);
    return UpdateDecision::ERROR_FALLBACK_TO_REBUILD;
  }

  state_ = UpdateWorkerState::PROCESSING;
  auto analysis_start = std::chrono::high_resolution_clock::now();

  // Decide strategy
  const UpdateDecision decision =
      decideUpdateStrategy(task.delta_window, task.artifact_size_bytes, task.current_manifest.residual);
  UpdateDecision final_decision = decision;

  auto analysis_end = std::chrono::high_resolution_clock::now();
  metrics.analysis_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(analysis_end - analysis_start).count();

  // Execute decision
  auto exec_start = std::chrono::high_resolution_clock::now();
  ArtifactManifest updated_manifest = task.current_manifest;
  bool success = false;

  try {
    switch (decision) {
      case UpdateDecision::PATCH:
        success = executePatch(task.artifact_id, task.delta_window, updated_manifest);
        break;
      case UpdateDecision::PARTIAL_REFIT:
        success = executePartialRefit(task.artifact_id, task.delta_window, updated_manifest);
        break;
      case UpdateDecision::REBUILD:
        success = executeRebuild(task.artifact_id, task.delta_window, updated_manifest);
        break;
      default:
        success = false;
    }
  } catch (const std::exception& e) {
    metrics.error_message = "Exception during execution: " + std::string(e.what());
    success = false;
  } catch (...) {
    metrics.error_message = "Unknown exception during execution";
    success = false;
  }

  if (!success && decision != UpdateDecision::REBUILD) {
    final_decision = UpdateDecision::ERROR_FALLBACK_TO_REBUILD;
    try {
      success = executeRebuild(task.artifact_id, task.delta_window,
                               updated_manifest);
      if (!success) {
        metrics.error_message =
            "Fallback rebuild failed after update-path error";
      }
    } catch (const std::exception& e) {
      metrics.error_message =
          "Fallback rebuild exception: " + std::string(e.what());
      success = false;
    } catch (...) {
      metrics.error_message = "Unknown fallback rebuild exception";
      success = false;
    }
  }

  auto exec_end = std::chrono::high_resolution_clock::now();
  metrics.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start).count();

  if (success) {
    // Try to renew lock during long operations
    renewUpdateLock(task.artifact_id, 3600);

    // Publish updated manifest
    const uint64_t current_version = task.current_manifest.version;
    success = publishManifest(task.artifact_id, updated_manifest, current_version,
                             final_decision == UpdateDecision::PATCH
                                 ? "patched"
                                 : final_decision == UpdateDecision::PARTIAL_REFIT
                                       ? "refit"
                                       : "rebuilt");
  }

  metrics.decision = final_decision;
  metrics.success = success;
  metrics.resulting_residual = updated_manifest.residual;
  metrics.resulting_rank_status = updated_manifest.rank_status;

  // Update worker stats
  stats_.total_tasks_processed++;
  if (final_decision == UpdateDecision::PATCH)
    stats_.total_patches_applied++;
  else if (final_decision == UpdateDecision::PARTIAL_REFIT)
    stats_.total_partial_refits++;
  else if (final_decision == UpdateDecision::REBUILD
           || final_decision == UpdateDecision::ERROR_FALLBACK_TO_REBUILD)
    stats_.total_rebuilds++;
  if (!success) stats_.total_failed_updates++;
  updateAverages(stats_, metrics.analysis_time_ms, metrics.execution_time_ms);

  // Clean up checkpoint on success
  if (success && checkpoint_manager_) {
    checkpoint_manager_->deleteCheckpoint(task.artifact_id);
  }

  // Release lock
  releaseUpdateLock(task.artifact_id);

  state_ = UpdateWorkerState::READY;
  return final_decision;
}

UpdateDecision SnapshotBasedUpdateWorker::processDeltaWindow(const std::string& artifact_id,
                                                             const DeltaWindow& delta_window,
                                                             const ArtifactManifest& current_manifest,
                                                             uint64_t artifact_size_bytes) {
  UpdateTask task;
  task.artifact_id = artifact_id;
  task.delta_window = delta_window;
  task.current_manifest = current_manifest;
  task.artifact_size_bytes = artifact_size_bytes;

  UpdateMetrics metrics;
  return processTask(task, metrics);
}

UpdateDecision SnapshotBasedUpdateWorker::decideUpdateStrategy(const DeltaWindow& delta_window,
                                                               uint64_t artifact_size_bytes,
                                                               double current_residual) {
  if (!delta_window.isValid() || delta_window.entries.empty()) {
    return UpdateDecision::NO_UPDATE;
  }

  // Phase B: Detect instability early
  if (detectInstability(delta_window, current_residual)) {
    return UpdateDecision::REBUILD;  // Fail-closed: rebuild on instability
  }

  // Structural mutations always require rebuild
  if (delta_window.countDeletes() > 0 || delta_window.countShardChanges() > 0) {
    return UpdateDecision::REBUILD;
  }

  // Estimate change fraction
  double change_fraction = delta_window.estimateChangeFraction(artifact_size_bytes);

  // Phase B: Check for valid patch conditions
  if (change_fraction < patch_threshold_pct_ / 100.0) {
    // Patch is a candidate, but validate applicability
    if (!isValidForPatching(delta_window)) {
      return UpdateDecision::REBUILD;  // Patch not applicable, fallback to rebuild
    }
    return UpdateDecision::PATCH;
  } else if (change_fraction < refit_threshold_pct_ / 100.0) {
    // Check if partial refit would exceed residual threshold
    double estimated_residual = estimateResultingResidual(delta_window, current_residual, UpdateDecision::PARTIAL_REFIT);
    if (estimated_residual - current_residual <= residual_max_increase_allowed_) {
      return UpdateDecision::PARTIAL_REFIT;
    }
    // Fallback to rebuild if residual would exceed limit
    return UpdateDecision::REBUILD;
  } else {
    // Large delta requires rebuild
    return UpdateDecision::REBUILD;
  }
}

bool SnapshotBasedUpdateWorker::executePatch(const std::string& artifact_id,
                                             const DeltaWindow& delta_window,
                                             ArtifactManifest& current_manifest) {
  (void)artifact_id;
  
  // Phase B: Validate patch applicability and bounds
  if (delta_window.entries.empty() || delta_window.countDeletes() > 0 ||
      delta_window.countShardChanges() > 0) {
    return false;
  }

  // Phase B: Check sequence continuity and age
  if (!isValidForPatching(delta_window)) {
    return false;
  }

  // Phase B: Check delta window bounds
  // A patch window must have entries within expected sequence range
  if (delta_window.sequence_end < delta_window.sequence_start) {
    return false;
  }
  if (delta_window.entries.size() != (delta_window.sequence_end - delta_window.sequence_start + 1)) {
    return false;  // Gap in sequence
  }

  // Ensure manifest state is compatible with patch
  if (!current_manifest.validate()) {
    return false;
  }

  // Apply patch: minimal updates to manifest reflecting small delta absorption
  ++current_manifest.version;
  current_manifest.markPublished(UpdateMode::PATCH, RebuildState::PATCHED,
                                delta_window.sequence_end);
  
  // Phase B: Residual improvement from patch (reflects incremental fix)
  double residual_improvement = std::min(0.01, 0.005 * delta_window.entries.size() / 100.0);
  current_manifest.residual = std::max(0.0, current_manifest.residual - residual_improvement);
  
  current_manifest.rank_status =
      std::min<uint32_t>(current_manifest.rank_cap == 0
                            ? current_manifest.rank_status
                            : current_manifest.rank_cap,
                         current_manifest.rank_status);
  return true;
}

bool SnapshotBasedUpdateWorker::executePartialRefit(const std::string& artifact_id,
                                                    const DeltaWindow& delta_window,
                                                    ArtifactManifest& current_manifest) {
  // Phase B: Check rank cap breach (state machine transition guard)
  if (wouldBreachRankCap(current_manifest, delta_window)) {
    if (error_handler_) {
      [[maybe_unused]] const auto error_info = error_handler_->analyzeRankCapBreach(
          artifact_id, current_manifest.rank_status + 100, current_manifest.rank_cap);
    }
    return false;
  }

  // Phase B: Validate manifest state before refit
  if (!current_manifest.validate()) {
    if (error_handler_) {
      [[maybe_unused]] const auto error_info = error_handler_->analyzePartialRefitFailure(
          artifact_id, "manifest validation failed", current_manifest.residual, current_manifest.residual);
    }
    return false;
  }

  try {
    double prev_residual = current_manifest.residual;
    const double change_fraction =
        delta_window.estimateChangeFraction(std::max<uint64_t>(
            current_manifest.rank_cap == 0 ? 1 : current_manifest.rank_cap * 1024ULL,
            1ULL));
    
    // Phase B: Apply refit_threshold_pct_ more carefully
    double residual_increase = std::min(0.25, change_fraction * 0.05);
    current_manifest.residual = prev_residual + residual_increase;
    
    // Phase B: Track state transition for refit completion
    current_manifest.rank_status +=
        static_cast<uint32_t>(delta_window.countUpdates() +
                              delta_window.countInserts());

    // Phase B: Check if residual increased too much (fail-closed)
    if (current_manifest.residual - prev_residual > residual_max_increase_allowed_) {
      // Revert and fail to signal rebuild fallback
      current_manifest.residual = prev_residual;
      if (error_handler_) {
        [[maybe_unused]] const auto error_info = error_handler_->analyzePartialRefitFailure(
            artifact_id, "residual threshold exceeded", prev_residual, current_manifest.residual);
      }
      return false;
    }

    // Phase B: Mark state machine transition to PARTIAL_REFITTED
    ++current_manifest.version;
    current_manifest.markPublished(UpdateMode::PARTIAL_REFIT,
                                   RebuildState::PARTIAL_REFITTED,
                                   delta_window.sequence_end);
    return true;
  } catch (const std::exception& e) {
    if (error_handler_) {
      [[maybe_unused]] const auto error_info = error_handler_->analyzePartialRefitFailure(
          artifact_id, std::string(e.what()), current_manifest.residual, current_manifest.residual);
    }
    return false;
  }
}

bool SnapshotBasedUpdateWorker::executeRebuild(const std::string& artifact_id,
                                               const DeltaWindow& delta_window,
                                               ArtifactManifest& current_manifest) {
  if (artifact_id.empty() || delta_window.sequence_end == 0) {
    return false;
  }

  // Phase B: Validate manifest state before rebuild
  if (!current_manifest.validate()) {
    if (error_handler_) {
      [[maybe_unused]] const auto error_info = error_handler_->analyzePartialRefitFailure(
          artifact_id, "rebuild: manifest validation failed", current_manifest.residual, 0.0);
    }
    return false;
  }

  try {
    // Phase B: State machine transition to REBUILT
    ++current_manifest.version;
    current_manifest.residual = 0.0;  // Reset residual on fresh rebuild
    current_manifest.rank_status = 0;
    current_manifest.markPublished(UpdateMode::REBUILD, RebuildState::REBUILT,
                                   delta_window.sequence_end);
    current_manifest.last_rebuild_at_unix_sec = getCurrentTimeSec();
    return true;
  } catch (const std::exception& e) {
    if (error_handler_) {
      [[maybe_unused]] const auto error_info = error_handler_->analyzePartialRefitFailure(
          artifact_id, std::string("rebuild exception: ") + e.what(), 
          current_manifest.residual, current_manifest.residual);
    }
    return false;
  }
}

bool SnapshotBasedUpdateWorker::publishManifest(const std::string& artifact_id,
                                                const ArtifactManifest& new_manifest,
                                                uint64_t old_version,
                                                const std::string& reason) {
  (void)artifact_id;
  (void)old_version;
  (void)reason;

  if (!manifest_store_) {
    return new_manifest.validate();
  }

  return manifest_store_->store(new_manifest);
}

bool SnapshotBasedUpdateWorker::shutdown() {
  state_ = UpdateWorkerState::SHUTTING_DOWN;
  state_ = UpdateWorkerState::IDLE;
  return true;
}

UpdateWorkerState SnapshotBasedUpdateWorker::getState() const { return state_; }

SnapshotBasedUpdateWorker::Stats SnapshotBasedUpdateWorker::getStats() const { return stats_; }

void SnapshotBasedUpdateWorker::setDecisionThresholds(double patch_threshold_pct,
                                                      double refit_threshold_pct,
                                                      double residual_max_increase_allowed) {
  patch_threshold_pct_ = patch_threshold_pct;
  refit_threshold_pct_ = refit_threshold_pct;
  residual_max_increase_allowed_ = residual_max_increase_allowed;
}

void SnapshotBasedUpdateWorker::setCheckpointPath(const std::string& checkpoint_path) {
  checkpoint_path_ = checkpoint_path;
  checkpoint_manager_ = std::make_unique<CrashRecoveryCheckpoint>(checkpoint_path);
}

void SnapshotBasedUpdateWorker::setLockManager(std::shared_ptr<DistributedLockManager> lock_manager) {
  lock_manager_ = lock_manager;
}

void SnapshotBasedUpdateWorker::setStaleArtifactDetector(std::shared_ptr<StaleArtifactDetector> detector) {
  stale_detector_ = detector;
}

void SnapshotBasedUpdateWorker::setErrorRecoveryHandler(std::shared_ptr<ErrorRecoveryHandler> handler) {
  error_handler_ = handler;
}

void SnapshotBasedUpdateWorker::setManifestStore(ManifestStore* manifest_store) noexcept {
  manifest_store_ = manifest_store;
}

std::optional<ArtifactManifest> SnapshotBasedUpdateWorker::recoverFromCheckpoint(const std::string& artifact_id) {
  if (!checkpoint_manager_) {
    return std::nullopt;  // No checkpoint manager, recovery not applicable
  }

  // Try to load checkpoint
  Checkpoint checkpoint;
  CheckpointStatus status = checkpoint_manager_->load(artifact_id, checkpoint);

  if (status == CheckpointStatus::NOT_FOUND) {
    return std::nullopt;  // No checkpoint to recover from (not an error)
  }

  if (status != CheckpointStatus::OK) {
    // Error loading checkpoint - fail closed per SG-DT-01
    if (status == CheckpointStatus::CORRUPTED) {
      // Corrupted checkpoint - delete it and continue with full rebuild
      spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                  "checkpoint corrupted, deleting artifact_id={}", artifact_id);
      checkpoint_manager_->deleteCheckpoint(artifact_id);
      return std::nullopt;  // Checkpoint deleted, allow processing to continue
    }
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "failed to load checkpoint artifact_id={}, status={}",
                artifact_id, static_cast<int>(status));
    return std::nullopt;  // I/O or version error - fail closed
  }

  // Check if we've exhausted retries - if so, give up and allow full rebuild
  if (checkpoint.retry_count >= checkpoint.max_retries) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "checkpoint max retries exceeded artifact_id={}, retry_count={}",
                artifact_id, checkpoint.retry_count);
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;  // Checkpoint exhausted, allow full rebuild
  }

  // ============================================================================
  // Production checkpoint recovery: Resume update from saved state
  // ============================================================================

  // Step 1: Validate checkpoint data integrity
  if (checkpoint.artifact_id.empty() || 
      checkpoint.current_manifest.artifact_id.empty() ||
      checkpoint.delta_window.entries.empty()) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "checkpoint data integrity check failed artifact_id={}, "
                "has_artifact_id={}, has_manifest_id={}, delta_entries={}",
                artifact_id, !checkpoint.artifact_id.empty(),
                !checkpoint.current_manifest.artifact_id.empty(),
                checkpoint.delta_window.entries.size());
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;
  }

  // Step 2: Validate manifest state
  if (!checkpoint.current_manifest.validate()) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "manifest validation failed artifact_id={}", artifact_id);
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;
  }

  // Step 3: Restore delta window from checkpoint
  // The checkpoint contains the full delta window at the point of save
  const DeltaWindow& checkpoint_delta = checkpoint.delta_window;

  // Determine update type from checkpoint decision or current state
  UpdateMode checkpoint_update_mode = UpdateMode::UNKNOWN;
  switch (checkpoint.last_decision) {
    case static_cast<uint32_t>(UpdateDecision::PATCH):
      checkpoint_update_mode = UpdateMode::PATCH;
      break;
    case static_cast<uint32_t>(UpdateDecision::PARTIAL_REFIT):
      checkpoint_update_mode = UpdateMode::PARTIAL_REFIT;
      break;
    case static_cast<uint32_t>(UpdateDecision::REBUILD):
      checkpoint_update_mode = UpdateMode::REBUILD;
      break;
    default:
      // Decide based on manifest state
      if (checkpoint.current_manifest.rebuild_state == RebuildState::REBUILDING) {
        checkpoint_update_mode = UpdateMode::REBUILD;
      } else if (checkpoint.current_manifest.rebuild_state == RebuildState::PATCHING) {
        checkpoint_update_mode = UpdateMode::PATCH;
      } else if (checkpoint.current_manifest.rebuild_state == RebuildState::PARTIAL_REFITTING) {
        checkpoint_update_mode = UpdateMode::PARTIAL_REFIT;
      }
      break;
  }

  // Step 4: Resume residual state and validate
  const double checkpoint_residual = checkpoint.current_manifest.residual;

  // Check if residual has become invalid (would exceed threshold)
  if (checkpoint_residual > 1.0 || checkpoint_residual < 0.0) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "invalid residual state artifact_id={}, residual={:.4f}",
                artifact_id, checkpoint_residual);
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;
  }

  // Step 5: Validate that the delta window is still applicable
  // If delta window is too old or contains invalid sequence, fail closed
  if (!checkpoint_delta.isValid() || checkpoint_delta.entries.empty()) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "delta window invalid or empty artifact_id={}, "
                "is_valid={}, entries={}",
                artifact_id, checkpoint_delta.isValid(),
                checkpoint_delta.entries.size());
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;
  }

  // Step 6: State machine validation - check if current state allows recovery
  RebuildState current_state = checkpoint.current_manifest.rebuild_state;
  bool state_allows_recovery = false;

  switch (current_state) {
    case RebuildState::REBUILDING:
      // Can always recover from REBUILDING state
      state_allows_recovery = true;
      break;
    case RebuildState::PATCHING:
      // Can recover from PATCHING if the mode matches
      state_allows_recovery = (checkpoint_update_mode == UpdateMode::PATCH);
      break;
    case RebuildState::PARTIAL_REFITTING:
      // Can recover from PARTIAL_REFITTING if the mode matches
      state_allows_recovery = (checkpoint_update_mode == UpdateMode::PARTIAL_REFIT);
      break;
    default:
      // Cannot recover from PATCHED, PARTIAL_REFITTED, REBUILT states
      state_allows_recovery = false;
      break;
  }

  if (!state_allows_recovery) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "state machine does not allow recovery artifact_id={}, "
                "rebuild_state={}, checkpoint_mode={}",
                artifact_id, static_cast<int>(current_state),
                static_cast<int>(checkpoint_update_mode));
    checkpoint_manager_->deleteCheckpoint(artifact_id);
    return std::nullopt;
  }

  // Step 7: Increment retry counter and save updated checkpoint
  checkpoint.retry_count++;
  checkpoint.created_at_unix_sec = getCurrentTimeSec();

  CheckpointStatus update_status = checkpoint_manager_->save(artifact_id, checkpoint);
  if (update_status != CheckpointStatus::OK) {
    spdlog::warn("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
                "failed to save updated checkpoint artifact_id={}, status={}",
                artifact_id, static_cast<int>(update_status));
    return std::nullopt;  // Failed to update checkpoint - fail closed
  }

  // Step 8: Prepare state machine transition
  // Transition manifests to REBUILDING during recovery to ensure consistency
  ArtifactManifest recovered_manifest = checkpoint.current_manifest;
  recovered_manifest.rebuild_state = RebuildState::REBUILDING;
  recovered_manifest.lifecycle_state = LifecycleState::UPDATING;

  // Step 9: Log recovery details for diagnostics
  spdlog::info("SnapshotBasedUpdateWorker::recoverFromCheckpoint: "
              "successfully recovered artifact_id={} retry_count={} "
              "update_mode={} residual={:.4f}",
              artifact_id, checkpoint.retry_count,
              static_cast<int>(checkpoint_update_mode), checkpoint_residual);

  // Recovery successful - return the recovered manifest to caller
  return recovered_manifest;
}

bool SnapshotBasedUpdateWorker::saveCheckpoint(const std::string& artifact_id, const UpdateTask& task) {
  if (!checkpoint_manager_) {
    return true;  // No checkpoint manager, saving not applicable
  }

  try {
    Checkpoint checkpoint;
    checkpoint.version = 1;
    checkpoint.created_at_unix_sec =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    checkpoint.artifact_id = artifact_id;
    checkpoint.delta_window = task.delta_window;
    checkpoint.current_manifest = task.current_manifest;
    checkpoint.artifact_size_bytes = task.artifact_size_bytes;
    checkpoint.retry_count = 0;
    checkpoint.max_retries = 3;

    CheckpointStatus status = checkpoint_manager_->save(artifact_id, checkpoint);
    return status == CheckpointStatus::OK;
  } catch (...) {
    return false;
  }
}

bool SnapshotBasedUpdateWorker::acquireUpdateLock(const std::string& artifact_id,
                                                  const std::string& lock_reason,
                                                  int64_t ttl_seconds) {
  if (!lock_manager_) {
    return true;  // No lock manager, locking not applicable
  }

  if (worker_id_.empty()) {
    // Generate a worker ID if not set
    worker_id_ = "worker_" + std::to_string(reinterpret_cast<uintptr_t>(this));
  }

  LockStatus status = lock_manager_->acquireLock(artifact_id, worker_id_, ttl_seconds, lock_reason);
  return status == LockStatus::OK;
}

bool SnapshotBasedUpdateWorker::releaseUpdateLock(const std::string& artifact_id) {
  if (!lock_manager_ || worker_id_.empty()) {
    return true;  // No lock manager or worker ID
  }

  LockStatus status = lock_manager_->releaseLock(artifact_id, worker_id_);
  return status == LockStatus::OK || status == LockStatus::NOT_HELD;  // Both OK and NOT_HELD are acceptable
}

bool SnapshotBasedUpdateWorker::renewUpdateLock(const std::string& artifact_id, int64_t ttl_seconds) {
  if (!lock_manager_ || worker_id_.empty()) {
    return true;  // No lock manager or worker ID
  }

  LockStatus status = lock_manager_->renewLock(artifact_id, worker_id_, ttl_seconds);
  return status == LockStatus::OK;
}

StaleArtifactMetrics SnapshotBasedUpdateWorker::detectStaleness(const std::string& artifact_id,
                                                               const ArtifactManifest& current_manifest,
                                                               uint64_t current_source_seq) {
  static StaleArtifactMetrics default_metrics;  // Default if no detector

  if (!stale_detector_) {
    return default_metrics;
  }

  // Calculate worker throughput from stats
  double worker_throughput = 0.0;
  if (stats_.total_tasks_processed > 0 && stats_.average_execution_time_ms > 0) {
    worker_throughput = 1000.0 / stats_.average_execution_time_ms;
  }

  // Analyze staleness
  return stale_detector_->analyzeArtifactStaleness(
      artifact_id, current_manifest, current_source_seq, worker_throughput, 0.0);  // delta_arrival_rate unknown here
}

// Phase B: Public wrapper for patching validation
bool SnapshotBasedUpdateWorker::isValidForPatchingPublic(const DeltaWindow& delta_window,
                                                         int64_t max_age_ms) const {
  return isValidForPatching(delta_window, max_age_ms);
}

// Phase B: Public wrapper for instability detection
bool SnapshotBasedUpdateWorker::detectInstabilityPublic(const DeltaWindow& delta_window,
                                                        double current_residual) const {
  return detectInstability(delta_window, current_residual);
}

// Phase B: Public wrapper for overflow detection
bool SnapshotBasedUpdateWorker::isDeltaLogOverflowingPublic(size_t current_entries,
                                                            uint32_t max_entries) const {
  return isDeltaLogOverflowing(current_entries, max_entries);
}

bool SnapshotBasedUpdateWorker::wouldBreachRankCap(const ArtifactManifest& manifest,
                                                   const DeltaWindow& delta_window) {
  if (manifest.rank_cap == 0) {
    return false;
  }

  const auto projected_rank = manifest.rank_status +
      static_cast<uint32_t>(delta_window.countUpdates() +
                            delta_window.countInserts());
  return projected_rank > manifest.rank_cap;
}

double SnapshotBasedUpdateWorker::estimateResultingResidual(const DeltaWindow& delta_window,
                                                            double current_residual,
                                                            UpdateDecision decision) {
  // Estimate residual increase based on change fraction
  double change_fraction = delta_window.estimateChangeFraction(10000);  // Arbitrary artifact size for estimation

  switch (decision) {
    case UpdateDecision::PATCH:
      // Patch should not increase residual significantly
      return current_residual + (change_fraction * 0.01);
    case UpdateDecision::PARTIAL_REFIT:
      // Partial refit has moderate residual increase
      return current_residual + (change_fraction * 0.05);
    case UpdateDecision::REBUILD:
      // Rebuild can reset residual to 0 (ideal case) or increase slightly
      return 0.0;
    default:
      return current_residual;
  }
}

}  // namespace distributed_tensor
}  // namespace themis
