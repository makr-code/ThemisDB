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

/// @brief Ingestion latency monitor using T-Digest for percentile tracking.
///
/// Tracks index update latencies (staleness duration) across shards with
/// configurable percentile computation. Supports both real-time aggregation
/// and historical trend analysis for SLA compliance monitoring.
///
/// @details
/// Latency tracking:
/// - Per-shard ingestion time (UTC timestamp of last update)
/// - Computed staleness = now() - ingestion_time
/// - Percentile aggregation via T-Digest (space-efficient, bounded error)
/// - p50, p75, p95, p99 available
/// - Configurable time windows (hourly, daily, weekly)
///
/// SLA Model:
/// - Target p95 staleness: < 5 minutes (configurable)
/// - Alert threshold: p95 > 2x target
/// - Historical retention: 7 days of hourly aggregates
///
/// @code
/// IngestionLatencyMonitor monitor(5.0);  // 5-minute p95 target
/// monitor.RecordIngestionTime("shard_1", 1695600000000);  // timestamp_us
/// auto percentiles = monitor.GetPercentiles();
/// if (percentiles.p95_latency_ms > 300000) {  // 5 min in ms
///   // SLA breach detected
/// }
/// @endcode
class IngestionLatencyMonitor {
 public:
  /// @brief Latency percentile snapshot.
  struct LatencyPercentiles {
    uint64_t p50_latency_ms;  ///< 50th percentile staleness (ms)
    uint64_t p75_latency_ms;  ///< 75th percentile staleness (ms)
    uint64_t p95_latency_ms;  ///< 95th percentile staleness (ms)
    uint64_t p99_latency_ms;  ///< 99th percentile staleness (ms)
    uint64_t max_latency_ms;  ///< Maximum observed staleness (ms)
    uint32_t sample_count;    ///< Number of shards sampled
    std::string computed_at;  ///< ISO 8601 timestamp of computation
  };

  /// @brief Per-shard ingestion status.
  struct ShardStatus {
    std::string shard_id;
    int64_t last_ingestion_us;    ///< Latest ingestion timestamp (UTC us)
    uint64_t current_staleness_ms;///< Elapsed since ingestion (ms)
    std::string status;           ///< "healthy" | "stale" | "critical"
    bool is_primary;              ///< Primary replica vs secondary
  };

  /// @brief Constructor with target SLA.
  /// @param target_p95_min Target p95 staleness in minutes.
  explicit IngestionLatencyMonitor(double target_p95_min = 5.0);

  /// @brief Record ingestion completion for a shard.
  /// @param shard_id Shard identifier.
  /// @param ingestion_time_us Ingestion completion timestamp (UTC microseconds).
  void RecordIngestionTime(const std::string& shard_id, int64_t ingestion_time_us);

  /// @brief Get current percentile snapshot.
  /// @return Computed percentiles across all shards.
  LatencyPercentiles GetPercentiles();

  /// @brief Get per-shard status.
  /// @return Map of shard_id → ShardStatus.
  std::map<std::string, ShardStatus> GetShardStatuses();

  /// @brief Check SLA compliance.
  /// @return true if p95 ≤ target, false if breach.
  bool IsCompliant();

  /// @brief Get shards exceeding critical threshold (2x target).
  /// @return List of critical shards.
  std::vector<ShardStatus> GetCriticalShards();

  /// @brief Rotate hourly aggregate (called by background task).
  /// @param hour_bucket Hour identifier for archival.
  void RotateHourlyAggregate(int64_t hour_bucket);

  /// @brief Get historical trend (last N hours).
  /// @param hours Number of hours to retrieve.
  /// @return Vector of percentile snapshots (oldest first).
  std::vector<LatencyPercentiles> GetTrendData(uint32_t hours);

  /// @brief Reset monitoring state.
  void Reset();

 private:
  double target_p95_min_;
  std::map<std::string, int64_t> last_ingestion_times_;
  std::vector<uint64_t> latency_samples_;  // T-Digest samples
  std::map<int64_t, LatencyPercentiles> historical_aggregates_;
};

}  // namespace themis::rag
