// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/ingestion_latency_monitor.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace themis::rag {

IngestionLatencyMonitor::IngestionLatencyMonitor(double target_p95_min)
    : target_p95_min_(target_p95_min) {}

void IngestionLatencyMonitor::RecordIngestionTime(
    const std::string& shard_id,
    int64_t ingestion_time_us) {
  last_ingestion_times_[shard_id] = ingestion_time_us;

  // Compute staleness
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  int64_t delta_us = now_us - ingestion_time_us;
  uint64_t staleness_us = delta_us > 0 ? static_cast<uint64_t>(delta_us) : 0;
  uint64_t staleness_ms = staleness_us / 1000;

  latency_samples_.push_back(staleness_ms);
}

IngestionLatencyMonitor::LatencyPercentiles
IngestionLatencyMonitor::GetPercentiles() {
  LatencyPercentiles result;
  result.sample_count = last_ingestion_times_.size();

  if (latency_samples_.empty()) {
    result.p50_latency_ms = 0;
    result.p75_latency_ms = 0;
    result.p95_latency_ms = 0;
    result.p99_latency_ms = 0;
    result.max_latency_ms = 0;
    return result;
  }

  // Compute percentiles (simple quantile method)
  std::vector<uint64_t> sorted_samples = latency_samples_;
  std::sort(sorted_samples.begin(), sorted_samples.end());

  size_t p50_idx = (sorted_samples.size() * 50) / 100;
  size_t p75_idx = (sorted_samples.size() * 75) / 100;
  size_t p95_idx = (sorted_samples.size() * 95) / 100;
  size_t p99_idx = (sorted_samples.size() * 99) / 100;

  result.p50_latency_ms = sorted_samples[std::min(p50_idx, sorted_samples.size() - 1)];
  result.p75_latency_ms = sorted_samples[std::min(p75_idx, sorted_samples.size() - 1)];
  result.p95_latency_ms = sorted_samples[std::min(p95_idx, sorted_samples.size() - 1)];
  result.p99_latency_ms = sorted_samples[std::min(p99_idx, sorted_samples.size() - 1)];
  result.max_latency_ms = sorted_samples.back();

  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  char buf[256];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t_now));
  result.computed_at = std::string(buf);

  return result;
}

std::map<std::string, IngestionLatencyMonitor::ShardStatus>
IngestionLatencyMonitor::GetShardStatuses() {
  std::map<std::string, ShardStatus> result;
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  uint64_t target_us = static_cast<uint64_t>(target_p95_min_ * 60 * 1000000);

  for (const auto& [shard_id, ingestion_us] : last_ingestion_times_) {
    ShardStatus status;
    status.shard_id = shard_id;
    status.last_ingestion_us = ingestion_us;
    int64_t delta_us = now_us - ingestion_us;
    status.current_staleness_ms =
        (delta_us > 0 ? static_cast<uint64_t>(delta_us) : 0) / 1000;
    status.is_primary = true;  // TODO: Track primary vs secondary

    if (status.current_staleness_ms < target_us / 1000) {
      status.status = "healthy";
    } else if (status.current_staleness_ms < 2 * target_us / 1000) {
      status.status = "stale";
    } else {
      status.status = "critical";
    }

    result[shard_id] = status;
  }

  return result;
}

bool IngestionLatencyMonitor::IsCompliant() {
  auto percentiles = GetPercentiles();
  uint64_t target_ms = static_cast<uint64_t>(target_p95_min_ * 60 * 1000);
  return percentiles.p95_latency_ms <= target_ms;
}

std::vector<IngestionLatencyMonitor::ShardStatus>
IngestionLatencyMonitor::GetCriticalShards() {
  std::vector<ShardStatus> critical;
  auto statuses = GetShardStatuses();
  uint64_t critical_threshold_ms = static_cast<uint64_t>(target_p95_min_ * 60 * 1000 * 2);

  for (const auto& [shard_id, status] : statuses) {
    if (status.current_staleness_ms > critical_threshold_ms) {
      critical.push_back(status);
    }
  }

  return critical;
}

void IngestionLatencyMonitor::RotateHourlyAggregate(int64_t hour_bucket) {
  // Archive current percentiles for this hour
  auto percentiles = GetPercentiles();
  historical_aggregates_[hour_bucket] = percentiles;

  // TODO: Implement persistence to RocksDB
}

std::vector<IngestionLatencyMonitor::LatencyPercentiles>
IngestionLatencyMonitor::GetTrendData(uint32_t hours) {
  std::vector<LatencyPercentiles> trend;

  // TODO: Retrieve historical aggregates for last N hours
  // For now, return current percentile only
  trend.push_back(GetPercentiles());

  return trend;
}

void IngestionLatencyMonitor::Reset() {
  last_ingestion_times_.clear();
  latency_samples_.clear();
  historical_aggregates_.clear();
}

}  // namespace themis::rag
