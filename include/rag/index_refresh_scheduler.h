// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Index refresh scheduling for background and emergency updates.
///
/// Manages background refresh cycles (hourly, daily) and triggers emergency
/// refreshes when staleness exceeds critical threshold. Supports multi-shard
/// orchestration with rate limiting and retry logic.
///
/// @details
/// Scheduling modes:
/// - Background: Periodic refresh at fixed intervals (hourly default)
/// - Emergency: Triggered when p95 > 2x target (adaptive delay)
/// - Canary: Refresh single shard for testing before full rollout
///
/// Rate limiting:
/// - Max 1 refresh per shard per 5 minutes
/// - Global max 10 concurrent refreshes
/// - Backoff on failure (exponential, max 30 min)
///
/// Refresh workflow:
/// 1. Trigger refresh (background or emergency)
/// 2. Acquire shard lock (fail if already refreshing)
/// 3. Query data store for updates
/// 4. Build index incrementally
/// 5. Activate index atomically
/// 6. Verify (spot check, then report)
///
/// @code
/// auto scheduler = std::make_unique<IndexRefreshScheduler>(data_store);
/// scheduler->SetBackgroundInterval(3600);  // 1 hour
/// scheduler->ScheduleRefresh("shard_1");   // Trigger immediately
/// auto status = scheduler->GetRefreshStatus("shard_1");
/// @endcode
class IndexRefreshScheduler {
 public:
  /// @brief Refresh request status.
  enum class RefreshStatus {
    Pending,      ///< Waiting to be processed
    InProgress,   ///< Currently refreshing
    Completed,    ///< Successfully completed
    Failed,       ///< Refresh failed (will retry)
    Cancelled     ///< User cancelled
  };

  /// @brief Refresh job result.
  struct RefreshResult {
    std::string shard_id;
    RefreshStatus status;
    uint64_t documents_added;      ///< New docs indexed
    uint64_t documents_updated;    ///< Existing docs updated
    uint64_t refresh_time_ms;      ///< Elapsed time (ms)
    std::string error_message;     ///< If status == Failed
    int64_t completed_at_us;       ///< UTC microseconds
    bool was_emergency;            ///< Emergency vs background
  };

  /// @brief Constructor.
  /// @param background_interval_sec Interval between background refreshes.
  explicit IndexRefreshScheduler(uint64_t background_interval_sec = 3600);

  /// @brief Trigger refresh immediately (background).
  /// @param shard_id Shard to refresh.
  /// @return true if scheduled, false if rate-limited.
  bool ScheduleRefresh(const std::string& shard_id);

  /// @brief Trigger emergency refresh (higher priority).
  /// @param shard_id Shard to refresh.
  /// @param priority_level 1-10 (10 = highest priority).
  /// @return true if emergency refresh triggered.
  bool ScheduleEmergencyRefresh(const std::string& shard_id, int priority_level);

  /// @brief Get refresh status for shard.
  /// @param shard_id Shard identifier.
  /// @return Current status.
  RefreshStatus GetRefreshStatus(const std::string& shard_id);

  /// @brief Get latest refresh result.
  /// @param shard_id Shard identifier.
  /// @return Result or std::nullopt if never refreshed.
  std::optional<RefreshResult> GetLastRefreshResult(const std::string& shard_id);

  /// @brief Get pending refresh queue.
  /// @return List of pending refresh jobs.
  std::vector<RefreshResult> GetPendingRefreshes();

  /// @brief Get in-progress refreshes.
  /// @return List of currently refreshing shards.
  std::vector<RefreshResult> GetInProgressRefreshes();

  /// @brief Cancel pending refresh.
  /// @param shard_id Shard to cancel.
  /// @return true if cancelled, false if not pending.
  bool CancelRefresh(const std::string& shard_id);

  /// @brief Set background refresh interval.
  /// @param interval_sec Seconds between refreshes.
  void SetBackgroundInterval(uint64_t interval_sec);

  /// @brief Set emergency refresh delay.
  /// @param delay_sec Delay before emergency refresh (adaptive).
  void SetEmergencyRefreshDelay(uint64_t delay_sec);

  /// @brief Set max concurrent refreshes.
  /// @param max_concurrent Maximum number of parallel refreshes.
  void SetMaxConcurrentRefreshes(uint32_t max_concurrent);

  /// @brief Process scheduled refreshes (called by background task).
  /// @return Number of refreshes started.
  uint32_t ProcessScheduledRefreshes();

  /// @brief Get scheduler health/stats.
  /// @return Map of stat_name → value.
  std::map<std::string, int64_t> GetStats();

 private:
  uint64_t background_interval_sec_;
  uint64_t emergency_refresh_delay_sec_;
  uint32_t max_concurrent_refreshes_;
  std::map<std::string, RefreshStatus> refresh_status_;
  std::map<std::string, RefreshResult> last_results_;
};

}  // namespace themis::rag
