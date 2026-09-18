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

enum class UpdateErrorCode : uint16_t {
  OK = 0,

  PARTIAL_REFIT_FAILED = 1,

  RANK_CAP_BREACH = 2,

  RESIDUAL_BREACH = 3,

  LOCK_TIMEOUT = 4,

  CHECKPOINT_CORRUPTED = 5,

  UPDATE_TIMEOUT = 6,

  PUBLISH_FAILED = 7,

  WORKER_ERROR = 8,

  UNKNOWN_ERROR = 9,
};

enum class RecoveryAction : uint8_t {
  NONE = 0,

  RETRY = 1,

  FALLBACK_TO_REBUILD = 2,

  MARK_STALE = 3,

  INVALIDATE = 4,

  ESCALATE_TO_PRIORITY = 5,

  DEFER_UPDATE = 6,
};

struct ErrorRecoveryInfo {
  UpdateErrorCode error_code = UpdateErrorCode::OK;

  std::string error_message;

  RecoveryAction recovery_action = RecoveryAction::NONE;

  uint32_t retry_attempts_allowed = 0;

  bool is_recoverable = true;

  bool should_cascade_invalidate = false;

  std::string context;
};

class ErrorRecoveryHandler {
 public:
  ErrorRecoveryHandler();

  /**
   * @brief Error Recovery Handler.
   * @return Return value.
   */
  virtual ~ErrorRecoveryHandler() = default;

  // Prevent copy/move
  ErrorRecoveryHandler(const ErrorRecoveryHandler&) = delete;
  ErrorRecoveryHandler& operator=(const ErrorRecoveryHandler&) = delete;
  ErrorRecoveryHandler(ErrorRecoveryHandler&&) = delete;
  ErrorRecoveryHandler& operator=(const ErrorRecoveryHandler&&) = delete;

  /**
   * @brief Analyze Partial Refit Failure.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] failure_reason Input parameter.
   * @param[in] previous_residual Input parameter.
   * @param[in] resulting_residual Input parameter.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzePartialRefitFailure(
      const std::string& artifact_id,
      const std::string& failure_reason,
      double previous_residual,
      double resulting_residual);

  /**
   * @brief Analyze Rank Cap Breach.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] current_rank_status Input parameter.
   * @param[in] rank_cap Input parameter.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzeRankCapBreach(const std::string& artifact_id,
                                                  uint32_t current_rank_status,
                                                  uint32_t rank_cap);

  /**
   * @brief Analyze Residual Breach.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] resulting_residual Input parameter.
   * @param[in] residual_threshold Input parameter.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzeResidualBreach(const std::string& artifact_id,
                                                   double resulting_residual,
                                                   double residual_threshold);

  /**
   * @brief Analyze Lock Timeout.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] timeout_ms Input parameter.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzeLockTimeout(const std::string& artifact_id,
                                               int64_t timeout_ms);

  /**
   * @brief Analyze Checkpoint Corruption.
   * @param[in] artifact_id Identifier of the artifact.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzeCheckpointCorruption(const std::string& artifact_id);

  /**
   * @brief Analyze Update Timeout.
   * @param[in] artifact_id Identifier of the artifact.
   * @param[in] timeout_ms Input parameter.
   * @param[in] delta_lag Input parameter.
   * @return Return value.
   */
  virtual ErrorRecoveryInfo analyzeUpdateTimeout(const std::string& artifact_id,
                                                 int64_t timeout_ms,
                                                 uint64_t delta_lag);

  struct RecoveryStats {
    uint64_t total_errors = 0;

    uint64_t successful_recoveries = 0;

    uint64_t errors_requiring_invalidation = 0;

    uint64_t errors_escalated = 0;

    uint64_t errors_deferred = 0;

    uint64_t irrecoverable_errors = 0;
  };

  /**
   * @brief Get Stats.
   * @return Return value.
   */
  virtual RecoveryStats getStats() const;

  /**
   * @brief Reset Stats.
   */
  virtual void resetStats();

  /**
   * @brief Set Recovery Thresholds.
   * @param[in] residual_increase_threshold Input parameter.
   * @param[in] retry_threshold Input parameter.
   */
  void setRecoveryThresholds(double residual_increase_threshold, uint32_t retry_threshold);

 protected:
  double residual_increase_threshold_ = 0.05;  // 5% increase allowed
  uint32_t retry_threshold_ = 3;
  RecoveryStats stats_;

  /**
   * @brief Get Current Time Ms.
   * @return Return value.
   */
  int64_t getCurrentTimeMs();
};

}  // namespace distributed_tensor
}  // namespace themis
