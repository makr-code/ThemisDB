// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/staleness_aware_router.h"

#include <gtest/gtest.h>

#include <memory>

#include "rag/ingestion_latency_monitor.h"

namespace themis::rag {

class StalenessAwareRouterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    monitor_ = std::make_shared<IngestionLatencyMonitor>(5.0);
    router_ = std::make_unique<StalenessAwareRouter>(monitor_);
  }

  std::shared_ptr<IngestionLatencyMonitor> monitor_;
  std::unique_ptr<StalenessAwareRouter> router_;
};

TEST_F(StalenessAwareRouterTest, HealthyPrimaryRouting) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // 10 sec old

  auto decision = router_->Route("test query", {"shard_1", "shard_2"});

  EXPECT_FALSE(decision.use_fallback);
  EXPECT_EQ(decision.primary_shards.size(), 2);
  EXPECT_EQ(decision.fallback_shards.size(), 0);
  EXPECT_FLOAT_EQ(decision.freshness_confidence, 1.0f);
}

TEST_F(StalenessAwareRouterTest, DegradedPrimaryRouting) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 350000 * 1000);  // 350 sec old

  auto decision = router_->Route("test query", {"shard_1"});

  EXPECT_FALSE(decision.use_fallback);
  EXPECT_LT(decision.freshness_confidence, 1.0f);
  EXPECT_GT(decision.freshness_confidence, 0.5f);
}

TEST_F(StalenessAwareRouterTest, CriticalPrimaryFallback) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 700000 * 1000);  // 700 sec old

  auto decision = router_->Route("test query", {"shard_1", "shard_2"});

  EXPECT_TRUE(decision.use_fallback);
  EXPECT_EQ(decision.primary_shards.size(), 0);
  EXPECT_EQ(decision.fallback_shards.size(), 2);
}

TEST_F(StalenessAwareRouterTest, RouteWithExplicitFallback) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 700000 * 1000);  // critical

  auto decision = router_->RouteWithFallback(
      "test query",
      {"primary_1", "primary_2"},
      {"replica_1", "replica_2"});

  EXPECT_TRUE(decision.use_fallback);
  EXPECT_EQ(decision.fallback_shards[0], "replica_1");
}

TEST_F(StalenessAwareRouterTest, GetStalenessMetadata) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // healthy

  auto metadata = router_->GetStalenessMetadata();

  EXPECT_FALSE(metadata.is_degraded);
  EXPECT_EQ(metadata.freshness_note, "Index is fresh");
}

TEST_F(StalenessAwareRouterTest, HealthCheckMethods) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // healthy

  EXPECT_TRUE(router_->IsPrimaryHealthy());
  EXPECT_FALSE(router_->IsPrimaryDegraded());
  EXPECT_FALSE(router_->IsPrimaryCritical());
}

}  // namespace themis::rag
