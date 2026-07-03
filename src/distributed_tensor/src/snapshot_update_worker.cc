/// @file snapshot_update_worker.cc
/// @brief Implementation of snapshot-based update worker for tensor artifacts
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "src/distributed_tensor/include/snapshot_update_worker.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// SnapshotBasedUpdateWorker Methods
// ============================================================================

SnapshotBasedUpdateWorker::SnapshotBasedUpdateWorker()
    : state_(UpdateWorkerState::IDLE),
      patch_threshold_pct_(10.0),
      refit_threshold_pct_(50.0),
      residual_max_increase_allowed_(0.05) {}

bool SnapshotBasedUpdateWorker::start() {
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

  state_ = UpdateWorkerState::PROCESSING;
  auto analysis_start = std::chrono::high_resolution_clock::now();

  // Decide strategy
  UpdateDecision decision =
      decideUpdateStrategy(task.delta_window, task.artifact_size_bytes, task.current_manifest.residual);

  auto analysis_end = std::chrono::high_resolution_clock::now();
  metrics.analysis_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(analysis_end - analysis_start).count();

  // Execute decision
  auto exec_start = std::chrono::high_resolution_clock::now();
  ArtifactManifest updated_manifest = task.current_manifest;
  bool success = false;

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

  auto exec_end = std::chrono::high_resolution_clock::now();
  metrics.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(exec_end - exec_start).count();

  if (success) {
    // Publish updated manifest
    uint64_t current_version = 1;  // Placeholder; should get from ManifestStore
    success = publishManifest(task.artifact_id, updated_manifest, current_version,
                             decision == UpdateDecision::PATCH       ? "patched"
                             : decision == UpdateDecision::PARTIAL_REFIT ? "refit"
                                                                      : "rebuilt");
  }

  metrics.decision = decision;
  metrics.success = success;
  metrics.resulting_residual = updated_manifest.residual;
  metrics.resulting_rank_status = updated_manifest.rank_status;

  // Update worker stats
  stats_.total_tasks_processed++;
  if (decision == UpdateDecision::PATCH)
    stats_.total_patches_applied++;
  else if (decision == UpdateDecision::PARTIAL_REFIT)
    stats_.total_partial_refits++;
  else if (decision == UpdateDecision::REBUILD)
    stats_.total_rebuilds++;
  if (!success) stats_.total_failed_updates++;

  state_ = UpdateWorkerState::READY;
  return decision;
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
  if (delta_window.entries.empty()) {
    return UpdateDecision::NO_UPDATE;
  }

  // Estimate change fraction
  double change_fraction = delta_window.estimateChangeFraction(artifact_size_bytes);

  // Decision logic based on change fraction
  if (change_fraction < patch_threshold_pct_ / 100.0) {
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
  // Placeholder patch implementation
  // Real implementation would apply delta patches to artifact
  current_manifest.rebuild_state = RebuildState::PATCHED;
  current_manifest.update_mode = UpdateMode::PATCH;
  current_manifest.source_seq_end = delta_window.sequence_end;
  current_manifest.delta_lag = 0;
  return true;
}

bool SnapshotBasedUpdateWorker::executePartialRefit(const std::string& artifact_id,
                                                    const DeltaWindow& delta_window,
                                                    ArtifactManifest& current_manifest) {
  // Check rank cap breach
  if (wouldBreachRankCap(current_manifest, delta_window)) {
    // Fallback to rebuild
    return executeRebuild(artifact_id, delta_window, current_manifest);
  }

  // Placeholder partial refit implementation
  // Real implementation would selectively retrain tensor components
  current_manifest.rebuild_state = RebuildState::PARTIAL_REFITTED;
  current_manifest.update_mode = UpdateMode::PARTIAL_REFIT;
  current_manifest.source_seq_end = delta_window.sequence_end;
  current_manifest.delta_lag = 0;
  current_manifest.residual *= 1.02;  // Simulate slight quality loss
  return true;
}

bool SnapshotBasedUpdateWorker::executeRebuild(const std::string& artifact_id,
                                               const DeltaWindow& delta_window,
                                               ArtifactManifest& current_manifest) {
  // Placeholder rebuild implementation
  // Real implementation would completely rebuild artifact from source lineage
  current_manifest.rebuild_state = RebuildState::REBUILT;
  current_manifest.update_mode = UpdateMode::REBUILD;
  current_manifest.source_seq_end = delta_window.sequence_end;
  current_manifest.delta_lag = 0;
  current_manifest.residual = 0.0;  // Fresh rebuild has no residual
  current_manifest.last_rebuild_at_unix_sec =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  return true;
}

bool SnapshotBasedUpdateWorker::publishManifest(const std::string& artifact_id,
                                                const ArtifactManifest& new_manifest,
                                                uint64_t old_version,
                                                const std::string& reason) {
  // Placeholder manifest publish logic
  // Real implementation would use ManifestStore.write() with CAS semantics
  return true;
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
}

bool SnapshotBasedUpdateWorker::wouldBreachRankCap(const ArtifactManifest& manifest,
                                                   const DeltaWindow& delta_window) {
  // Estimate if rank would exceed cap
  // Placeholder: assume deltas don't cause rank breach unless very large
  double change_fraction = delta_window.estimateChangeFraction(manifest.rank_cap * 10000);
  return change_fraction > 0.9;  // Only breach if > 90% of data changed
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
