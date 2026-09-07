/// @file error_recovery_handler.h
/// @brief Unified error handling and recovery for tensor updates
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03
///
/// This header defines unified error handling and recovery strategies
/// for various failure modes in tensor artifact updates.
///
/// ## Failure Modes Handled
///
/// - Failed partial refit → Fallback to rebuild
/// - Rank cap breach → Abort update and trigger rebuild
/// - Residual breach → Reject update, mark stale
/// - Lock timeout → Release and escalate to worker priority queue
/// - Checkpoint corruption → Delete and continue
/// - Update timeout → Mark artifact stale and continue

#pragma once

#include "artifact_manifest.h"
#include "artifact_invalidation.h"
#include <string>
#include <memory>
#include <cstdint>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @brief Error codes for update operations.
enum class UpdateErrorCode : uint16_t {
  /// No error.
  OK = 0,

  /// Partial refit failed (residual exceeded threshold).
  PARTIAL_REFIT_FAILED = 1,

  /// Rank cap would be breached.
  RANK_CAP_BREACH = 2,

  /// Residual quality exceeded acceptable threshold.
  RESIDUAL_BREACH = 3,

  /// Lock acquisition timed out.
  LOCK_TIMEOUT = 4,

  /// Checkpoint file corrupted.
  CHECKPOINT_CORRUPTED = 5,

  /// Update operation timed out.
  UPDATE_TIMEOUT = 6,

  /// Manifest publication failed.
  PUBLISH_FAILED = 7,

  /// Worker internal error.
  WORKER_ERROR = 8,

  /// Unknown error.
  UNKNOWN_ERROR = 9,
};

/// @brief Recovery action to take.
enum class RecoveryAction : uint8_t {
  /// No action needed.
  NONE = 0,

  /// Retry the operation.
  RETRY = 1,

  /// Fallback to rebuild.
  FALLBACK_TO_REBUILD = 2,

  /// Mark artifact stale.
  MARK_STALE = 3,

  /// Invalidate artifact.
  INVALIDATE = 4,

  /// Escalate to priority queue for immediate retry.
  ESCALATE_TO_PRIORITY = 5,

  /// Defer the update for later.
  DEFER_UPDATE = 6,
};

/// @brief Error recovery information.
struct ErrorRecoveryInfo {
  /// Error code that occurred.
  UpdateErrorCode error_code = UpdateErrorCode::OK;

  /// Human-readable error message.
  std::string error_message;

  /// Recommended recovery action.
  RecoveryAction recovery_action = RecoveryAction::NONE;

  /// Number of retry attempts allowed.
  uint32_t retry_attempts_allowed = 0;

  /// Whether the error is recoverable.
  bool is_recoverable = true;

  /// Whether cascade invalidation should be triggered.
  bool should_cascade_invalidate = false;

  /// Optional context for the error (e.g., which step failed).
  std::string context;
};

/// @brief Unified error recovery handler.
///
/// Analyzes update errors and recommends recovery actions,
/// including fallback strategies, retries, and invalidation cascades.
///
class ErrorRecoveryHandler {
 public:
  /// Creates an error recovery handler.
  ErrorRecoveryHandler();

  virtual ~ErrorRecoveryHandler() = default;

  // Prevent copy/move
  ErrorRecoveryHandler(const ErrorRecoveryHandler&) = delete;
  ErrorRecoveryHandler& operator=(const ErrorRecoveryHandler&) = delete;
  ErrorRecoveryHandler(ErrorRecoveryHandler&&) = delete;
  ErrorRecoveryHandler& operator=(const ErrorRecoveryHandler&&) = delete;

  /// Analyzes a partial refit failure and recommends action.
  /// @param artifact_id Artifact that failed
  /// @param failure_reason Reason for failure
  /// @param previous_residual Residual before update
  /// @param resulting_residual Residual after failed update
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzePartialRefitFailure(
      const std::string& artifact_id,
      const std::string& failure_reason,
      double previous_residual,
      double resulting_residual);

  /// Analyzes a rank cap breach and recommends action.
  /// @param artifact_id Artifact in question
  /// @param current_rank_status Current rank status
  /// @param rank_cap Rank cap limit
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzeRankCapBreach(const std::string& artifact_id,
                                                  uint32_t current_rank_status,
                                                  uint32_t rank_cap);

  /// Analyzes a residual breach and recommends action.
  /// @param artifact_id Artifact in question
  /// @param resulting_residual Residual after update
  /// @param residual_threshold Maximum acceptable residual
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzeResidualBreach(const std::string& artifact_id,
                                                   double resulting_residual,
                                                   double residual_threshold);

  /// Analyzes a lock timeout and recommends action.
  /// @param artifact_id Artifact that timed out
  /// @param timeout_ms Timeout duration in milliseconds
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzeLockTimeout(const std::string& artifact_id,
                                               int64_t timeout_ms);

  /// Analyzes a checkpoint corruption and recommends action.
  /// @param artifact_id Artifact with corrupted checkpoint
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzeCheckpointCorruption(const std::string& artifact_id);

  /// Analyzes an update timeout and recommends action.
  /// @param artifact_id Artifact that timed out
  /// @param timeout_ms Timeout duration in milliseconds
  /// @param delta_lag Current delta lag
  /// @return Recovery information
  virtual ErrorRecoveryInfo analyzeUpdateTimeout(const std::string& artifact_id,
                                                 int64_t timeout_ms,
                                                 uint64_t delta_lag);

  /// Gets recovery statistics.
  struct RecoveryStats {
    /// Total errors encountered.
    uint64_t total_errors = 0;

    /// Errors recovered successfully.
    uint64_t successful_recoveries = 0;

    /// Errors that required invalidation.
    uint64_t errors_requiring_invalidation = 0;

    /// Errors escalated to priority queue.
    uint64_t errors_escalated = 0;

    /// Errors marked for deferral.
    uint64_t errors_deferred = 0;

    /// Errors that failed recovery.
    uint64_t irrecoverable_errors = 0;
  };

  /// Gets recovery statistics.
  virtual RecoveryStats getStats() const;

  /// Resets statistics.
  virtual void resetStats();

  /// Sets thresholds for recovery decisions.
  /// @param residual_increase_threshold Maximum acceptable residual increase (0.0-1.0)
  /// @param retry_threshold Number of retries before giving up
  void setRecoveryThresholds(double residual_increase_threshold, uint32_t retry_threshold);

 protected:
  double residual_increase_threshold_ = 0.05;  // 5% increase allowed
  uint32_t retry_threshold_ = 3;
  RecoveryStats stats_;

  /// Internal helper to get current time in milliseconds since epoch.
  int64_t getCurrentTimeMs();
};

}  // namespace distributed_tensor
}  // namespace themis
