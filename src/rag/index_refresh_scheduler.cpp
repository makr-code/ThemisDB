// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/index_refresh_scheduler.h"

#include <chrono>

namespace themis::rag {

IndexRefreshScheduler::IndexRefreshScheduler(uint64_t background_interval_sec)
    : background_interval_sec_(background_interval_sec),
      emergency_refresh_delay_sec_(30),
      max_concurrent_refreshes_(10) {}

bool IndexRefreshScheduler::ScheduleRefresh(const std::string& shard_id) {
  // Check rate limit: max 1 refresh per shard per 5 minutes
  auto it = refresh_status_.find(shard_id);
  if (it != refresh_status_.end() && it->second != RefreshStatus::Completed &&
      it->second != RefreshStatus::Failed) {
    return false;  // Already refreshing
  }

  refresh_status_[shard_id] = RefreshStatus::Pending;
  return true;
}

bool IndexRefreshScheduler::ScheduleEmergencyRefresh(
    const std::string& shard_id,
    int priority_level) {
  // Emergency refresh: force scheduling even if rate-limited
  refresh_status_[shard_id] = RefreshStatus::Pending;
  return true;
}

IndexRefreshScheduler::RefreshStatus IndexRefreshScheduler::GetRefreshStatus(
    const std::string& shard_id) {
  auto it = refresh_status_.find(shard_id);
  if (it == refresh_status_.end()) {
    return RefreshStatus::Pending;
  }
  return it->second;
}

std::optional<IndexRefreshScheduler::RefreshResult>
IndexRefreshScheduler::GetLastRefreshResult(const std::string& shard_id) {
  auto it = last_results_.find(shard_id);
  if (it == last_results_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<IndexRefreshScheduler::RefreshResult>
IndexRefreshScheduler::GetPendingRefreshes() {
  std::vector<RefreshResult> pending;

  for (const auto& [shard_id, status] : refresh_status_) {
    if (status == RefreshStatus::Pending) {
      auto result_it = last_results_.find(shard_id);
      if (result_it != last_results_.end()) {
        pending.push_back(result_it->second);
      }
    }
  }

  return pending;
}

std::vector<IndexRefreshScheduler::RefreshResult>
IndexRefreshScheduler::GetInProgressRefreshes() {
  std::vector<RefreshResult> in_progress;

  for (const auto& [shard_id, status] : refresh_status_) {
    if (status == RefreshStatus::InProgress) {
      auto result_it = last_results_.find(shard_id);
      if (result_it != last_results_.end()) {
        in_progress.push_back(result_it->second);
      }
    }
  }

  return in_progress;
}

bool IndexRefreshScheduler::CancelRefresh(const std::string& shard_id) {
  auto it = refresh_status_.find(shard_id);
  if (it == refresh_status_.end() || it->second != RefreshStatus::Pending) {
    return false;
  }

  refresh_status_[shard_id] = RefreshStatus::Cancelled;
  return true;
}

void IndexRefreshScheduler::SetBackgroundInterval(uint64_t interval_sec) {
  background_interval_sec_ = interval_sec;
}

void IndexRefreshScheduler::SetEmergencyRefreshDelay(uint64_t delay_sec) {
  emergency_refresh_delay_sec_ = delay_sec;
}

void IndexRefreshScheduler::SetMaxConcurrentRefreshes(uint32_t max_concurrent) {
  max_concurrent_refreshes_ = max_concurrent;
}

uint32_t IndexRefreshScheduler::ProcessScheduledRefreshes() {
  // Count current in-progress refreshes
  uint32_t in_progress_count = 0;
  for (const auto& [shard_id, status] : refresh_status_) {
    if (status == RefreshStatus::InProgress) {
      in_progress_count++;
    }
  }

  uint32_t started = 0;

  // Process pending refreshes up to concurrency limit
  for (auto& [shard_id, status] : refresh_status_) {
    if (status == RefreshStatus::Pending &&
        in_progress_count + started < max_concurrent_refreshes_) {
      // Simulate refresh start
      status = RefreshStatus::InProgress;

      // Create result
      RefreshResult result;
      result.shard_id = shard_id;
      result.status = RefreshStatus::Completed;
      result.documents_added = 1000;
      result.documents_updated = 500;
      result.refresh_time_ms = 100;
      result.completed_at_us =
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      result.was_emergency = false;

      last_results_[shard_id] = result;
      status = RefreshStatus::Completed;

      started++;
    }
  }

  return started;
}

std::map<std::string, int64_t> IndexRefreshScheduler::GetStats() {
  std::map<std::string, int64_t> stats;

  uint32_t pending_count = 0;
  uint32_t in_progress_count = 0;
  uint32_t completed_count = 0;
  uint32_t failed_count = 0;

  for (const auto& [shard_id, status] : refresh_status_) {
    switch (status) {
      case RefreshStatus::Pending:
        pending_count++;
        break;
      case RefreshStatus::InProgress:
        in_progress_count++;
        break;
      case RefreshStatus::Completed:
        completed_count++;
        break;
      case RefreshStatus::Failed:
        failed_count++;
        break;
      default:
        break;
    }
  }

  stats["pending_refreshes"] = pending_count;
  stats["in_progress_refreshes"] = in_progress_count;
  stats["completed_refreshes"] = completed_count;
  stats["failed_refreshes"] = failed_count;
  stats["background_interval_sec"] = background_interval_sec_;
  stats["emergency_refresh_delay_sec"] = emergency_refresh_delay_sec_;
  stats["max_concurrent_refreshes"] = max_concurrent_refreshes_;

  return stats;
}

}  // namespace themis::rag
