/// @file error_recovery_handler.cc
/// @brief Implementation of error recovery handler
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-03

#include "../include/error_recovery_handler.h"
#include <chrono>
#include <cmath>

namespace themis {
namespace distributed_tensor {

ErrorRecoveryHandler::ErrorRecoveryHandler() {}

int64_t ErrorRecoveryHandler::getCurrentTimeMs() {
  auto now = std::chrono::high_resolution_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzePartialRefitFailure(
    const std::string& artifact_id,
    const std::string& failure_reason,
    double previous_residual,
    double resulting_residual) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::PARTIAL_REFIT_FAILED;
  info.error_message = "Partial refit failed: " + failure_reason;
  info.context = "artifact_id=" + artifact_id;

  // Check if residual increased too much
  double residual_increase = resulting_residual - previous_residual;
  if (residual_increase > residual_increase_threshold_) {
    // Residual breach, need to rebuild
    info.recovery_action = RecoveryAction::FALLBACK_TO_REBUILD;
    info.retry_attempts_allowed = 0;
    info.is_recoverable = true;
  } else {
    // Might be recoverable with retry
    info.recovery_action = RecoveryAction::RETRY;
    info.retry_attempts_allowed = retry_threshold_;
    info.is_recoverable = true;
  }

  stats_.total_errors++;
  if (info.is_recoverable) {
    stats_.successful_recoveries++;
  } else {
    stats_.irrecoverable_errors++;
  }

  return info;
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzeRankCapBreach(const std::string& artifact_id,
                                                              uint32_t current_rank_status,
                                                              uint32_t rank_cap) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::RANK_CAP_BREACH;
  info.error_message = "Rank cap breach: " + std::to_string(current_rank_status) + " > " + std::to_string(rank_cap);
  info.context = "artifact_id=" + artifact_id;

  // Rank cap breach requires rebuild to reset rank status
  info.recovery_action = RecoveryAction::FALLBACK_TO_REBUILD;
  info.retry_attempts_allowed = 0;
  info.is_recoverable = true;
  info.should_cascade_invalidate = true;

  stats_.total_errors++;
  stats_.successful_recoveries++;
  stats_.errors_requiring_invalidation++;

  return info;
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzeResidualBreach(const std::string& artifact_id,
                                                               double resulting_residual,
                                                               double residual_threshold) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::RESIDUAL_BREACH;
  info.error_message = "Residual breach: " + std::to_string(resulting_residual) + " > " + std::to_string(residual_threshold);
  info.context = "artifact_id=" + artifact_id;

  // Residual breach requires invalidation and fallback
  info.recovery_action = RecoveryAction::INVALIDATE;
  info.retry_attempts_allowed = 0;
  info.is_recoverable = false;
  info.should_cascade_invalidate = true;

  stats_.total_errors++;
  stats_.errors_requiring_invalidation++;
  stats_.irrecoverable_errors++;

  return info;
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzeLockTimeout(const std::string& artifact_id,
                                                           int64_t timeout_ms) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::LOCK_TIMEOUT;
  info.error_message = "Lock acquisition timeout after " + std::to_string(timeout_ms) + "ms";
  info.context = "artifact_id=" + artifact_id;

  // Lock timeout → escalate to priority queue for immediate retry
  info.recovery_action = RecoveryAction::ESCALATE_TO_PRIORITY;
  info.retry_attempts_allowed = 1;
  info.is_recoverable = true;

  stats_.total_errors++;
  stats_.successful_recoveries++;
  stats_.errors_escalated++;

  return info;
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzeCheckpointCorruption(const std::string& artifact_id) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::CHECKPOINT_CORRUPTED;
  info.error_message = "Checkpoint file corrupted";
  info.context = "artifact_id=" + artifact_id;

  // Checkpoint corruption → delete it and continue normally
  info.recovery_action = RecoveryAction::NONE;
  info.retry_attempts_allowed = 1;
  info.is_recoverable = true;

  stats_.total_errors++;
  stats_.successful_recoveries++;

  return info;
}

ErrorRecoveryInfo ErrorRecoveryHandler::analyzeUpdateTimeout(const std::string& artifact_id,
                                                             int64_t timeout_ms,
                                                             uint64_t delta_lag) {
  ErrorRecoveryInfo info;
  info.error_code = UpdateErrorCode::UPDATE_TIMEOUT;
  info.error_message = "Update timeout after " + std::to_string(timeout_ms) + "ms, delta_lag=" + std::to_string(delta_lag);
  info.context = "artifact_id=" + artifact_id;

  // Update timeout → mark stale and defer for later processing
  if (delta_lag > 10000) {
    // Large delta lag → defer to let worker catch up
    info.recovery_action = RecoveryAction::DEFER_UPDATE;
    info.retry_attempts_allowed = 0;
    info.is_recoverable = true;
  } else {
    // Small lag → mark stale and retry
    info.recovery_action = RecoveryAction::MARK_STALE;
    info.retry_attempts_allowed = 1;
    info.is_recoverable = true;
  }

  stats_.total_errors++;
  stats_.successful_recoveries++;
  stats_.errors_deferred++;

  return info;
}

ErrorRecoveryHandler::RecoveryStats ErrorRecoveryHandler::getStats() const {
  return stats_;
}

void ErrorRecoveryHandler::resetStats() {
  stats_ = RecoveryStats();
}

void ErrorRecoveryHandler::setRecoveryThresholds(double residual_increase_threshold,
                                                 uint32_t retry_threshold) {
  residual_increase_threshold_ = residual_increase_threshold;
  retry_threshold_ = retry_threshold;
}

}  // namespace distributed_tensor
}  // namespace themis
