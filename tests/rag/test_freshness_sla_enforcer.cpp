// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/freshness_sla_enforcer.h"

#include <gtest/gtest.h>

#include <memory>

#include "rag/ingestion_latency_monitor.h"

namespace themis::rag {

class FreshnessSLAEnforcerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    monitor_ = std::make_shared<IngestionLatencyMonitor>(5.0);
    enforcer_ = std::make_unique<FreshnessSLAEnforcer>(monitor_, "test_sla", 300000);
  }

  std::shared_ptr<IngestionLatencyMonitor> monitor_;
  std::unique_ptr<FreshnessSLAEnforcer> enforcer_;
};

TEST_F(FreshnessSLAEnforcerTest, ComplianceWhenHealthy) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // 10 sec old

  EXPECT_TRUE(enforcer_->IsCompliant());
}

TEST_F(FreshnessSLAEnforcerTest, BreachWhenStale) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 400000 * 1000);  // 400 sec old

  EXPECT_FALSE(enforcer_->IsCompliant());
}

TEST_F(FreshnessSLAEnforcerTest, GetComplianceStatus) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);

  auto status = enforcer_->GetComplianceStatus();

  EXPECT_TRUE(status.is_compliant);
  EXPECT_EQ(status.sla_name, "test_sla");
  EXPECT_LE(status.p95_latency_ms, status.sla_target_ms);
}

TEST_F(FreshnessSLAEnforcerTest, TriggerEmergencyRefresh) {
  bool triggered = enforcer_->TriggerEmergencyRefresh();
  EXPECT_TRUE(triggered);

  auto events = enforcer_->GetRecentEvents(1);
  EXPECT_GT(events.size(), 0);
  EXPECT_EQ(events.back().type, FreshnessSLAEnforcer::SLAEvent::Type::EmergencyTriggered);
}

TEST_F(FreshnessSLAEnforcerTest, FallbackShards) {
  enforcer_->RegisterFallbackShard("replica_1");
  enforcer_->RegisterFallbackShard("replica_2");

  auto shards = enforcer_->GetFallbackShards();
  EXPECT_EQ(shards.size(), 2);
  EXPECT_EQ(shards[0], "replica_1");
}

TEST_F(FreshnessSLAEnforcerTest, UnregisterFallbackShard) {
  enforcer_->RegisterFallbackShard("replica_1");
  enforcer_->RegisterFallbackShard("replica_2");

  enforcer_->UnregisterFallbackShard("replica_1");

  auto shards = enforcer_->GetFallbackShards();
  EXPECT_EQ(shards.size(), 1);
  EXPECT_EQ(shards[0], "replica_2");
}

TEST_F(FreshnessSLAEnforcerTest, SetTarget) {
  enforcer_->SetTarget(600000);  // Change to 10 min

  auto status = enforcer_->GetComplianceStatus();
  EXPECT_EQ(status.sla_target_ms, 600000);
}

TEST_F(FreshnessSLAEnforcerTest, GetStalenessNotice) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);  // healthy
  enforcer_->UpdateCompliance();

  std::string notice = enforcer_->GetStalenessNotice();
  EXPECT_TRUE(notice.empty() || notice.find("fresh") != std::string::npos);
}

TEST_F(FreshnessSLAEnforcerTest, UpdateComplianceDetectsBreach) {
  auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

  // Start healthy
  monitor_->RecordIngestionTime("shard_1", now_us - 10000 * 1000);
  enforcer_->UpdateCompliance();

  // Simulate staleness increase
  monitor_->RecordIngestionTime("shard_1", now_us - 700000 * 1000);  // critical
  bool changed = enforcer_->UpdateCompliance();

  EXPECT_TRUE(changed);  // State should have changed
}

}  // namespace themis::rag
