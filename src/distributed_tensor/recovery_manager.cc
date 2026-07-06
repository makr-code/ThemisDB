// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/recovery_manager.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <uuid/uuid.h>

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
  plan.shards_to_recover = failed_shard_ids;

  // Select recovery strategy.
  if (preferred_strategy) {
    plan.recovery_strategy = *preferred_strategy;
  } else {
    plan.recovery_strategy = select_recovery_strategy(manifest, failed_shard_ids);
  }

  // Estimate recovery time (1 shard = ~100ms in this simplified model).
  plan.estimated_total_recovery_time_ms =
      failed_shard_ids.size() * 100;

  // Set default priority and degraded mode allowance.
  plan.priority_level = 50;
  plan.allow_degraded_mode = true;

  return plan;
}

std::string DefaultRecoveryManager::submit_recovery_job(
    const std::string& artifact_id,
    const std::string& shard_id,
    RecoveryStrategy recovery_strategy,
    uint32_t priority) noexcept {
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
  std::vector<RecoveryJob> active_jobs;
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

  return true;
}

RecoveryStrategy DefaultRecoveryManager::select_recovery_strategy(
    const ArtifactManifest& manifest,
    const std::vector<std::string>& failed_shard_ids) const noexcept {
  // Strategy selection heuristic:
  // 1. For primary artifacts with replication: use REPLICATION.
  // 2. For derived artifacts: use REBUILD_FROM_PARENT if parent available.
  // 3. For erasure-coded artifacts: use ERASURE_CODING.
  // 4. Default: REPLICATION.

  const auto& recovery_strategy = manifest.recovery_strategy();

  if (recovery_strategy == "erasure_coding") {
    return RecoveryStrategy::ERASURE_CODING;
  } else if (recovery_strategy == "replication") {
    return RecoveryStrategy::REPLICATION;
  } else if (manifest.artifact_class() == ArtifactClass::DERIVED) {
    return RecoveryStrategy::REBUILD_FROM_PARENT;
  }

  return RecoveryStrategy::REPLICATION;
}

}  // namespace distributed_tensor
}  // namespace themis
