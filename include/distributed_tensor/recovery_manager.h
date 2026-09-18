/**
 * @file recovery_manager.h
 * @brief Recovery manager for failed or incomplete distributed tensor operations.
 *
 * Coordinates checkpoint replication, partial-failure detection, and
 * re-execution of failed tensor shards within the distributed training pipeline.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace distributed_tensor {


enum class RecoveryStrategy {
  REPLICATION,

  ERASURE_CODING,

  REBUILD_FROM_PARENT,

  FULL_RECONSTRUCTION,

  NONE,
};

enum class RecoveryFailureMode {
  TRANSIENT_UNAVAILABLE,

  PERMANENT_LOSS,

  INSUFFICIENT_REDUNDANCY,

  RECOVERY_TIMEOUT,

  RECOVERY_ERROR,
};

enum class RecoveryJobStatus {
  QUEUED,

  RUNNING,

  COMPLETED,

  FAILED_RECOVERABLE,

  FAILED_UNRECOVERABLE,

  CANCELLED,
};

struct RecoveryJob {
  std::string job_id;

  std::string artifact_id;

  std::string shard_id;

  RecoveryStrategy recovery_strategy;

  RecoveryJobStatus status;

  std::optional<RecoveryFailureMode> failure_mode;

  uint64_t estimated_completion_ms = 0;

  uint32_t retry_count = 0;

  uint32_t max_retries = 3;

  std::string created_at;

  std::string started_at;

  std::string completed_at;

  uint32_t progress_percent = 0;

  std::string error_message;
};

struct RecoveryPlan {
  std::string artifact_id;

  RecoveryStrategy recovery_strategy;

  std::vector<std::string> shards_to_recover;

  uint64_t estimated_total_recovery_time_ms = 0;

  uint32_t priority_level = 50;

  bool allow_degraded_mode = true;

  bool is_recoverable = true;

  std::optional<RecoveryFailureMode> blocking_failure_mode;

  std::string blocking_reason;

  std::unordered_map<std::string, std::string> custom_parameters;
};

class RecoveryManager {
 public:
  RecoveryManager() = default;

  RecoveryManager(const RecoveryManager&) = delete;

  RecoveryManager(RecoveryManager&&) noexcept = default;

  RecoveryManager& operator=(const RecoveryManager&) = delete;

  RecoveryManager& operator=(RecoveryManager&&) noexcept = default;

  /**
   * @brief Recovery Manager.
   * @return Return value.
   */
  virtual ~RecoveryManager() = default;

  virtual RecoveryPlan create_recovery_plan(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids,
      std::optional<RecoveryStrategy> preferred_strategy = std::nullopt)
      const noexcept = 0;

  virtual std::string submit_recovery_job(
      const std::string& artifact_id,
      const std::string& shard_id,
      RecoveryStrategy recovery_strategy,
      uint32_t priority = 50) noexcept = 0;

  /**
   * @brief Get recovery job.
   * @param[in] job_id Identifier of the job.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::optional<RecoveryJob> get_recovery_job(
      const std::string& job_id) const noexcept = 0;

  /**
   * @brief Cancel recovery job.
   * @param[in] job_id Identifier of the job.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool cancel_recovery_job(const std::string& job_id) noexcept = 0;

  /**
   * @brief List active recovery jobs.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual std::vector<RecoveryJob> list_active_recovery_jobs() const
      noexcept = 0;

  /**
   * @brief Retry recovery job.
   * @param[in] job_id Identifier of the job.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool retry_recovery_job(const std::string& job_id) noexcept = 0;
};

class DefaultRecoveryManager : public RecoveryManager {
 public:
  DefaultRecoveryManager() = default;

  DefaultRecoveryManager(DefaultRecoveryManager&&) noexcept = default;

  DefaultRecoveryManager& operator=(DefaultRecoveryManager&&) noexcept =
      default;

  ~DefaultRecoveryManager() override = default;

  RecoveryPlan create_recovery_plan(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids,
      std::optional<RecoveryStrategy> preferred_strategy = std::nullopt)
      const noexcept override;

  std::string submit_recovery_job(
      const std::string& artifact_id,
      const std::string& shard_id,
      RecoveryStrategy recovery_strategy,
      uint32_t priority = 50) noexcept override;

  std::optional<RecoveryJob> get_recovery_job(
      const std::string& job_id) const noexcept override;

  bool cancel_recovery_job(const std::string& job_id) noexcept override;

  std::vector<RecoveryJob> list_active_recovery_jobs() const noexcept override;

  bool retry_recovery_job(const std::string& job_id) noexcept override;

 private:
  std::unordered_map<std::string, RecoveryJob> recovery_jobs_;

  /**
   * @brief Select recovery strategy.
   * @param[in] manifest Input parameter.
   * @param[in] failed_shard_ids Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  RecoveryStrategy select_recovery_strategy(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids) const noexcept;
};


}  // namespace distributed_tensor
}  // namespace themis
