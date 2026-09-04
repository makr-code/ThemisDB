// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/recovery_manager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <set>
#include <sstream>

namespace themis {
namespace distributed_tensor {

// Helper: Format current timestamp as ISO 8601 string.
static std::string get_iso8601_timestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// Helper: Generate unique job ID.
static std::string generate_job_id() noexcept {
  // Use timestamp-based ID generation (UUID would be more robust in production).
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto micros =
      std::chrono::duration_cast<std::chrono::microseconds>(now).count();

  std::ostringstream oss;
  oss << "recovery_job_" << std::hex << micros;
  return oss.str();
}

// DefaultRecoveryManager implementation.

RecoveryPlan DefaultRecoveryManager::create_recovery_plan(
    const ArtifactManifest& manifest,
    const std::vector<std::string>& failed_shard_ids,
    std::optional<RecoveryStrategy> preferred_strategy) const noexcept {
  RecoveryPlan plan;
  plan.artifact_id = manifest.artifact_id();
  std::set<std::string> unique_failed_shards(failed_shard_ids.begin(),
                                             failed_shard_ids.end());
  plan.shards_to_recover.assign(unique_failed_shards.begin(),
                                unique_failed_shards.end());

  if (!manifest.is_complete()) {
    plan.is_recoverable = false;
    plan.allow_degraded_mode = false;
    plan.blocking_failure_mode = RecoveryFailureMode::RECOVERY_ERROR;
    plan.blocking_reason =
        "Manifest is incomplete; recovery cannot be planned safely.";
    plan.recovery_strategy = RecoveryStrategy::NONE;
    return plan;
  }

  if (plan.shards_to_recover.empty()) {
    plan.is_recoverable = false;
    plan.allow_degraded_mode = false;
    plan.blocking_failure_mode = RecoveryFailureMode::RECOVERY_ERROR;
    plan.blocking_reason = "No failed shard identifiers were provided.";
    plan.recovery_strategy = RecoveryStrategy::NONE;
    return plan;
  }

  for (const auto& shard_id : plan.shards_to_recover) {
    if (!manifest.get_shard_placement(shard_id)) {
      plan.is_recoverable = false;
      plan.allow_degraded_mode = false;
      plan.blocking_failure_mode = RecoveryFailureMode::RECOVERY_ERROR;
      plan.blocking_reason =
          "Recovery plan references at least one shard that is absent from the manifest.";
      plan.recovery_strategy = RecoveryStrategy::NONE;
      return plan;
    }
  }

  const auto total_shards = manifest.num_shards();
  const auto failed_shard_count = plan.shards_to_recover.size();
  const auto healthy_shard_count =
      total_shards > failed_shard_count ? total_shards - failed_shard_count : 0U;

  // Select recovery strategy.
  if (preferred_strategy) {
    plan.recovery_strategy = *preferred_strategy;
  } else {
    plan.recovery_strategy =
        select_recovery_strategy(manifest, plan.shards_to_recover);
  }

  // Estimate recovery time (1 shard = ~100ms in this simplified model).
  plan.estimated_total_recovery_time_ms = failed_shard_count * 100;

  // Set default priority and degraded mode allowance.
  plan.priority_level = manifest.artifact_class() == ArtifactClass::PRIMARY ? 90 : 50;
  plan.allow_degraded_mode =
      manifest.artifact_class() != ArtifactClass::PRIMARY &&
      healthy_shard_count > 0;
  plan.custom_parameters["healthy_shard_count"] =
      std::to_string(healthy_shard_count);
  plan.custom_parameters["failed_shard_count"] =
      std::to_string(failed_shard_count);
  plan.custom_parameters["lifecycle_stage"] =
      std::to_string(static_cast<int>(manifest.lifecycle_stage()));

  if (manifest.artifact_class() == ArtifactClass::EPHEMERAL) {
    plan.is_recoverable = false;
    plan.allow_degraded_mode = false;
    plan.recovery_strategy = RecoveryStrategy::NONE;
    plan.blocking_failure_mode = RecoveryFailureMode::PERMANENT_LOSS;
    plan.blocking_reason =
        "Ephemeral artifacts are not recoverable once their shards are lost.";
    return plan;
  }

  if (healthy_shard_count == 0 &&
      plan.recovery_strategy == RecoveryStrategy::REPLICATION) {
    plan.is_recoverable = false;
    plan.allow_degraded_mode = false;
    plan.blocking_failure_mode = RecoveryFailureMode::INSUFFICIENT_REDUNDANCY;
    plan.blocking_reason =
        "Replication recovery requires at least one healthy shard copy.";
    return plan;
  }

  if (manifest.artifact_class() == ArtifactClass::DERIVED &&
      plan.recovery_strategy == RecoveryStrategy::REBUILD_FROM_PARENT &&
      manifest.parent_artifact_id().empty()) {
    plan.is_recoverable = false;
    plan.allow_degraded_mode = false;
    plan.blocking_failure_mode = RecoveryFailureMode::RECOVERY_ERROR;
    plan.blocking_reason =
        "Derived artifact recovery requires a parent artifact identifier.";
    return plan;
  }

  if (manifest.lifecycle_stage() == ArtifactLifecycleStage::DEPRECATED ||
      manifest.lifecycle_stage() == ArtifactLifecycleStage::DELETED) {
    plan.allow_degraded_mode = false;
  }

  return plan;
}

std::string DefaultRecoveryManager::submit_recovery_job(
    const std::string& artifact_id,
    const std::string& shard_id,
    RecoveryStrategy recovery_strategy,
    uint32_t priority) noexcept {
  (void)priority; // suppress unused-parameter warning when not used
  std::string job_id = generate_job_id();

  RecoveryJob job;
  job.job_id = job_id;
  job.artifact_id = artifact_id;
  job.shard_id = shard_id;
  job.recovery_strategy = recovery_strategy;
  job.status = RecoveryJobStatus::QUEUED;
  job.estimated_completion_ms = 100; // Default: 100ms.
  job.max_retries = 3;
  job.created_at = get_iso8601_timestamp();
  job.progress_percent = 0;

  recovery_jobs_[job_id] = job;
  return job_id;
}

std::optional<RecoveryJob> DefaultRecoveryManager::get_recovery_job(
    const std::string& job_id) const noexcept {
  auto it = recovery_jobs_.find(job_id);
  if (it != recovery_jobs_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool DefaultRecoveryManager::cancel_recovery_job(
    const std::string& job_id) noexcept {
  auto it = recovery_jobs_.find(job_id);
  if (it == recovery_jobs_.end()) {
    return false;
  }

  // Can only cancel queued or running jobs.
  if (it->second.status != RecoveryJobStatus::QUEUED &&
      it->second.status != RecoveryJobStatus::RUNNING) {
    return false;
  }

  it->second.status = RecoveryJobStatus::CANCELLED;
  it->second.completed_at = get_iso8601_timestamp();
  return true;
}

std::vector<RecoveryJob> DefaultRecoveryManager::list_active_recovery_jobs()
    const noexcept {
  std::vector<RecoveryJob> active_jobs = {};

  for (const auto& [job_id, job] : recovery_jobs_) {
    if (job.status == RecoveryJobStatus::QUEUED ||
        job.status == RecoveryJobStatus::RUNNING) {
      active_jobs.push_back(job);
    }
  }
  return active_jobs;
}

bool DefaultRecoveryManager::retry_recovery_job(
    const std::string& job_id) noexcept {
  auto it = recovery_jobs_.find(job_id);
  if (it == recovery_jobs_.end()) {
    return false;
  }

  // Can only retry failed jobs.
  if (it->second.status != RecoveryJobStatus::FAILED_RECOVERABLE) {
    return false;
  }

  // Check max retries.
  if (it->second.retry_count >= it->second.max_retries) {
    return false;
  }

  // Transition to queued for retry.
  it->second.status = RecoveryJobStatus::QUEUED;
  it->second.retry_count++;
  it->second.started_at = "";
  it->second.completed_at = "";
  it->second.error_message.clear();
  it->second.failure_mode.reset();
  it->second.progress_percent = 0;

  return true;
}

// NOTE: no global suppressions here.

RecoveryStrategy DefaultRecoveryManager::select_recovery_strategy(
    const ArtifactManifest& manifest,
    const std::vector<std::string>& failed_shard_ids) const noexcept {
  // Strategy selection heuristic:
  // 1. For primary artifacts with replication: use REPLICATION.
  // 2. For derived artifacts: use REBUILD_FROM_PARENT if parent available.
  // 3. For erasure-coded artifacts: use ERASURE_CODING.
  // 4. Default: REPLICATION.

  if (failed_shard_ids.empty()) {
    return RecoveryStrategy::NONE;
  }

  const auto& recovery_strategy = manifest.recovery_strategy();

  if (recovery_strategy == "erasure_coding") {
    return RecoveryStrategy::ERASURE_CODING;
  } else if (recovery_strategy == "replication") {
    return RecoveryStrategy::REPLICATION;
  } else if (manifest.artifact_class() == ArtifactClass::DERIVED) {
    return RecoveryStrategy::REBUILD_FROM_PARENT;
  } else if (manifest.artifact_class() == ArtifactClass::EPHEMERAL) {
    return RecoveryStrategy::NONE;
  }

  return RecoveryStrategy::REPLICATION;
}

}  // namespace distributed_tensor
}  // namespace themis
