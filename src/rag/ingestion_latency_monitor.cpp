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

  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  int64_t delta_us = now_us - ingestion_time_us;
  uint64_t staleness_us = delta_us > 0 ? static_cast<uint64_t>(delta_us) : 0;
  uint64_t staleness_ms = staleness_us / 1000;

  latency_samples_.push_back(staleness_ms);

  const int64_t hour_bucket = now_us / (60LL * 60LL * 1000000LL);
  const auto percentiles = GetPercentiles();
  historical_aggregates_[hour_bucket] = percentiles;
  last_ingestion_times_[shard_id] = ingestion_time_us;
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

    const std::string lower = shard_id;
    const auto has_primary = lower.find("primary") != std::string::npos;
    const auto has_secondary = lower.find("secondary") != std::string::npos;
    const auto has_replica = lower.find("replica") != std::string::npos;
    status.is_primary = has_primary || (!has_secondary && !has_replica);
    if (has_secondary || has_replica) {
      status.is_primary = false;
    }

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
  auto percentiles = GetPercentiles();
  historical_aggregates_[hour_bucket] = percentiles;
  if (historical_aggregates_.size() > 168) {
    std::map<int64_t, LatencyPercentiles> pruned;
    for (const auto& [bucket, aggregate] : historical_aggregates_) {
      if (bucket >= hour_bucket - 167) {
        pruned[bucket] = aggregate;
      }
    }
    historical_aggregates_.swap(pruned);
  }
}

std::vector<IngestionLatencyMonitor::LatencyPercentiles>
IngestionLatencyMonitor::GetTrendData(uint32_t hours) {
  if (hours == 0) {
    return {GetPercentiles()};
  }

  const auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
  const int64_t current_hour = now_us / (60LL * 60LL * 1000000LL);
  const int64_t window_start = current_hour - static_cast<int64_t>(hours) + 1;

  std::vector<LatencyPercentiles> trend;
  for (const auto& [hour_bucket, aggregate] : historical_aggregates_) {
    if (hour_bucket >= window_start && hour_bucket <= current_hour) {
      trend.push_back(aggregate);
    }
  }

  if (trend.empty()) {
    const auto latest = GetPercentiles();
    trend.push_back(latest);
  } else {
    std::sort(trend.begin(), trend.end(),
              [](const LatencyPercentiles& lhs, const LatencyPercentiles& rhs) {
                return lhs.computed_at < rhs.computed_at;
              });
  }
  return trend;
}

void IngestionLatencyMonitor::Reset() {
  last_ingestion_times_.clear();
  latency_samples_.clear();
  historical_aggregates_.clear();
}

}  // namespace themis::rag
