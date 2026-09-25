// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/ingestion_latency_monitor.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace themis::rag {

class IngestionLatencyMonitorTest : public ::testing::Test {
 protected:
  void SetUp() override {
    monitor_ = std::make_unique<IngestionLatencyMonitor>(5.0);  // 5 min target
  }

  std::unique_ptr<IngestionLatencyMonitor> monitor_;
};

TEST_F(IngestionLatencyMonitorTest, RecordAndRetrievePercentiles) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  // Record 3 shards with different staleness
  monitor_->RecordIngestionTime("shard_1", now_us - 100000 * 1000);  // 100 sec old
  monitor_->RecordIngestionTime("shard_2", now_us - 200000 * 1000);  // 200 sec old
  monitor_->RecordIngestionTime("shard_3", now_us - 150000 * 1000);  // 150 sec old

  auto percentiles = monitor_->GetPercentiles();

  // Verify percentiles are computed
  EXPECT_GT(percentiles.p50_latency_ms, 0);
  EXPECT_GE(percentiles.p95_latency_ms, percentiles.p50_latency_ms);
  EXPECT_GE(percentiles.p99_latency_ms, percentiles.p95_latency_ms);
  EXPECT_EQ(percentiles.sample_count, 3);
}

TEST_F(IngestionLatencyMonitorTest, ComplianceCheck) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  // Record fresh data (well within 5 min target)
  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // 10 sec old

  EXPECT_TRUE(monitor_->IsCompliant());
}

TEST_F(IngestionLatencyMonitorTest, ComplianceBreach) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  // Record stale data (beyond 5 min target = 300 sec)
  monitor_->RecordIngestionTime("shard_1", now_us - 400000 * 1000);  // 400 sec old

  EXPECT_FALSE(monitor_->IsCompliant());
}

TEST_F(IngestionLatencyMonitorTest, GetShardStatuses) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);   // healthy
  monitor_->RecordIngestionTime("shard_2", now_us - 350000 * 1000);  // stale

  auto statuses = monitor_->GetShardStatuses();

  EXPECT_EQ(statuses.size(), 2);
  EXPECT_EQ(statuses["shard_1"].status, "healthy");
  EXPECT_EQ(statuses["shard_2"].status, "stale");
}

TEST_F(IngestionLatencyMonitorTest, GetCriticalShards) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);    // healthy
  monitor_->RecordIngestionTime("shard_2", now_us - 700000 * 1000);   // critical
  monitor_->RecordIngestionTime("shard_3", now_us - 1000000 * 1000);  // critical

  auto critical = monitor_->GetCriticalShards();

  EXPECT_EQ(critical.size(), 2);
  for (const auto& shard : critical) {
    EXPECT_GT(shard.current_staleness_ms, 600000);  // > 2x target
  }
}

}  // namespace themis::rag
