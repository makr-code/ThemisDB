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

/// @defgroup recovery Recovery and Rebuild Management
/// @brief Recovery scheduling, rebuild strategies, and failure handling.
/// @{

/// Recovery strategy enumeration.
///
/// Specifies the mechanism used to recover from shard failures.
enum class RecoveryStrategy {
  /// Replication: recover from intact replicas of the same shard.
  REPLICATION,

  /// Erasure coding: recover from data and parity blocks.
  ERASURE_CODING,

  /// Rebuilding from parent: reconstruct from parent artifact (for derived).
  REBUILD_FROM_PARENT,

  /// Full reconstruction: recompute from original computation graph.
  FULL_RECONSTRUCTION,

  /// None: artifact cannot be recovered (ephemeral).
  NONE,
};

/// Recovery failure mode enumeration.
///
/// Categorizes the type of failure encountered during recovery.
enum class RecoveryFailureMode {
  /// Shard is temporarily unavailable but will become available.
  TRANSIENT_UNAVAILABLE,

  /// Shard is permanently lost or corrupted.
  PERMANENT_LOSS,

  /// Insufficient redundancy to recover shard.
  INSUFFICIENT_REDUNDANCY,

  /// Recovery timeout occurred.
  RECOVERY_TIMEOUT,

  /// Recovery process encountered an error.
  RECOVERY_ERROR,
};

/// Recovery job status enumeration.
///
/// Tracks the state of a recovery job through its lifecycle.
enum class RecoveryJobStatus {
  /// Job is queued and waiting to start.
  QUEUED,

  /// Job is currently executing.
  RUNNING,

  /// Job has completed successfully.
  COMPLETED,

  /// Job failed with recoverable error.
  FAILED_RECOVERABLE,

  /// Job failed with unrecoverable error.
  FAILED_UNRECOVERABLE,

  /// Job was cancelled.
  CANCELLED,
};

/// Recovery job metadata.
///
/// Tracks a single recovery operation for a shard or artifact.
struct RecoveryJob {
  /// Unique job identifier.
  std::string job_id;

  /// Artifact ID being recovered.
  std::string artifact_id;

  /// Shard ID being recovered (empty if whole artifact).
  std::string shard_id;

  /// Strategy used for recovery.
  RecoveryStrategy recovery_strategy;

  /// Current job status.
  RecoveryJobStatus status;

  /// Failure mode (if status is FAILED_*).
  std::optional<RecoveryFailureMode> failure_mode;

  /// Estimated completion time in milliseconds.
  uint64_t estimated_completion_ms = 0;

  /// Number of retry attempts made.
  uint32_t retry_count = 0;

  /// Maximum allowed retry attempts.
  uint32_t max_retries = 3;

  /// Timestamp when job was created (ISO 8601).
  std::string created_at;

  /// Timestamp when job started (ISO 8601).
  std::string started_at;

  /// Timestamp when job completed (ISO 8601).
  std::string completed_at;

  /// Progress as percentage (0-100).
  uint32_t progress_percent = 0;

  /// Error message if recovery failed.
  std::string error_message;
};

/// Recovery plan for an artifact.
///
/// Specifies the recovery actions needed for an artifact with failures.
struct RecoveryPlan {
  /// Artifact ID this plan applies to.
  std::string artifact_id;

  /// Overall recovery strategy.
  RecoveryStrategy recovery_strategy;

  /// List of shards requiring recovery.
  std::vector<std::string> shards_to_recover;

  /// Estimated total recovery time in milliseconds.
  uint64_t estimated_total_recovery_time_ms = 0;

  /// Priority level (0=lowest, 100=highest).
  uint32_t priority_level = 50;

  /// If true, can proceed with degraded mode during recovery.
  bool allow_degraded_mode = true;

  /// If false, recovery must be blocked because no safe recovery path exists.
  bool is_recoverable = true;

  /// Failure classification explaining why recovery was blocked.
  std::optional<RecoveryFailureMode> blocking_failure_mode;

  /// Human-readable blocking reason for audit and diagnostics.
  std::string blocking_reason;

  /// Custom recovery parameters.
  std::unordered_map<std::string, std::string> custom_parameters;
};

/// Recovery manager interface.
///
/// Manages recovery and rebuild operations for distributed tensor artifacts.
class RecoveryManager {
 public:
  /// Construct a recovery manager.
  RecoveryManager() = default;

  /// Copy constructor deleted.
  RecoveryManager(const RecoveryManager&) = delete;

  /// Move constructor.
  RecoveryManager(RecoveryManager&&) noexcept = default;

  /// Assignment operator deleted.
  RecoveryManager& operator=(const RecoveryManager&) = delete;

  /// Move assignment operator.
  RecoveryManager& operator=(RecoveryManager&&) noexcept = default;

  /// Virtual destructor.
  virtual ~RecoveryManager() = default;

  /// Create a recovery plan for an artifact with failures.
  ///
  /// @param manifest Artifact manifest.
  /// @param failed_shard_ids Identifiers of failed shards.
  /// @param preferred_strategy Preferred recovery strategy.
  /// @return Recovery plan for the artifact.
  virtual RecoveryPlan create_recovery_plan(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids,
      std::optional<RecoveryStrategy> preferred_strategy = std::nullopt)
      const noexcept = 0;

  /// Submit a recovery job for execution.
  ///
  /// @param artifact_id Artifact to recover.
  /// @param shard_id Shard to recover (empty = whole artifact).
  /// @param recovery_strategy Recovery strategy to use.
  /// @param priority Job priority (0-100).
  /// @return Job ID for tracking.
  virtual std::string submit_recovery_job(
      const std::string& artifact_id,
      const std::string& shard_id,
      RecoveryStrategy recovery_strategy,
      uint32_t priority = 50) noexcept = 0;

  /// Get the status of a recovery job.
  ///
  /// @param job_id Job identifier.
  /// @return Recovery job metadata if found.
  virtual std::optional<RecoveryJob> get_recovery_job(
      const std::string& job_id) const noexcept = 0;

  /// Cancel a running recovery job.
  ///
  /// @param job_id Job identifier.
  /// @return true if job was cancelled, false if not found or already done.
  virtual bool cancel_recovery_job(const std::string& job_id) noexcept = 0;

  /// List all active recovery jobs.
  ///
  /// @return Vector of active recovery jobs.
  virtual std::vector<RecoveryJob> list_active_recovery_jobs() const
      noexcept = 0;

  /// Retry a failed recovery job.
  ///
  /// @param job_id Job identifier.
  /// @return true if retry was scheduled, false if job not found or max retries exceeded.
  virtual bool retry_recovery_job(const std::string& job_id) noexcept = 0;
};

/// Default recovery manager implementation.
///
/// Handles recovery scheduling and state transitions with support for
/// replication, erasure coding, and reconstruction strategies.
class DefaultRecoveryManager : public RecoveryManager {
 public:
  /// Construct the default recovery manager.
  DefaultRecoveryManager() = default;

  /// Move constructor.
  DefaultRecoveryManager(DefaultRecoveryManager&&) noexcept = default;

  /// Move assignment operator.
  DefaultRecoveryManager& operator=(DefaultRecoveryManager&&) noexcept =
      default;

  /// Destructor.
  ~DefaultRecoveryManager() override = default;

  /// Create recovery plan.
  RecoveryPlan create_recovery_plan(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids,
      std::optional<RecoveryStrategy> preferred_strategy = std::nullopt)
      const noexcept override;

  /// Submit recovery job.
  std::string submit_recovery_job(
      const std::string& artifact_id,
      const std::string& shard_id,
      RecoveryStrategy recovery_strategy,
      uint32_t priority = 50) noexcept override;

  /// Get recovery job status.
  std::optional<RecoveryJob> get_recovery_job(
      const std::string& job_id) const noexcept override;

  /// Cancel recovery job.
  bool cancel_recovery_job(const std::string& job_id) noexcept override;

  /// List active recovery jobs.
  std::vector<RecoveryJob> list_active_recovery_jobs() const noexcept override;

  /// Retry recovery job.
  bool retry_recovery_job(const std::string& job_id) noexcept override;

 private:
  /// In-memory job registry (in production, would use persistent storage).
  std::unordered_map<std::string, RecoveryJob> recovery_jobs_;

  /// Select best recovery strategy based on artifact and available data.
  RecoveryStrategy select_recovery_strategy(
      const ArtifactManifest& manifest,
      const std::vector<std::string>& failed_shard_ids) const noexcept;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
