// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <chrono>

#include "rag/realtime_slo_tracker.h"

namespace themis::rag {

class RealtimeSLOTrackerTest : public ::testing::Test {
 protected:
  RealtimeSLOTracker tracker_;
};

TEST_F(RealtimeSLOTrackerTest, RecordQuerySuccess) {
  auto initial_metrics = tracker_.GetCurrentMetrics();
  EXPECT_EQ(initial_metrics.size(), 2u);
  EXPECT_EQ(initial_metrics["total_queries"], 0u);
  EXPECT_EQ(initial_metrics["rerank_successes"], 0u);

  tracker_.RecordQuery(150, true, 0.82f, 0.03f);
  auto metrics = tracker_.GetCurrentMetrics();
  EXPECT_EQ(metrics.size(), 6u);
  EXPECT_EQ(metrics["total_queries"], 1u);
  EXPECT_EQ(metrics["rerank_successes"], 1u);
  EXPECT_EQ(metrics["avg_cost_usd_millicents"], 3000u);
}

TEST_F(RealtimeSLOTrackerTest, IsCompliantWithinBudget) {
  tracker_.SetTarget("p95_latency_ms", 200);
  tracker_.RecordQuery(150, true, 0.82f, 0.03f);
  tracker_.RecordQuery(160, true, 0.83f, 0.03f);
  tracker_.RecordQuery(170, true, 0.84f, 0.03f);

  bool compliant = tracker_.UpdateCompliance();
  EXPECT_FALSE(compliant);
  EXPECT_TRUE(tracker_.IsCompliant());
}

TEST_F(RealtimeSLOTrackerTest, IsNotCompliantBeyondBudget) {
  tracker_.SetTarget("p95_latency_ms", 200);
  tracker_.RecordQuery(500, true, 0.82f, 0.03f);
  tracker_.RecordQuery(550, true, 0.83f, 0.03f);
  
  EXPECT_TRUE(tracker_.UpdateCompliance());
  EXPECT_FALSE(tracker_.IsCompliant());
}

TEST_F(RealtimeSLOTrackerTest, ComplianceSnapshot) {
  tracker_.SetTarget("p95_latency_ms", 250);
  tracker_.RecordQuery(100, true, 0.82f, 0.03f);
  tracker_.RecordQuery(200, true, 0.83f, 0.03f);
  tracker_.RecordQuery(50, true, 0.81f, 0.03f);
  
  auto snapshot = tracker_.GetComplianceSnapshot();
  EXPECT_GT(snapshot.window_start_us, 0u);
  EXPECT_GT(snapshot.window_end_us, snapshot.window_start_us);
  EXPECT_GE(snapshot.metric_compliance.size(), 1u);
}

TEST_F(RealtimeSLOTrackerTest, FailureTracking) {
  tracker_.RecordQuery(100, true, 0.82f, 0.03f);
  tracker_.RecordQuery(150, false, 0.50f, 0.05f);
  tracker_.RecordQuery(200, true, 0.83f, 0.03f);
  
  auto metrics = tracker_.GetCurrentMetrics();
  EXPECT_EQ(metrics.at("total_queries"), 3u);
  EXPECT_EQ(metrics.at("rerank_successes"), 2u);
}

TEST_F(RealtimeSLOTrackerTest, MultipleMetrics) {
  tracker_.RecordQuery(100, true, 0.82f, 0.03f);
  tracker_.RecordQuery(50, true, 0.85f, 0.02f);
  tracker_.RecordQuery(30, true, 0.88f, 0.01f);
  
  auto metrics = tracker_.GetCurrentMetrics();
  EXPECT_GE(metrics.size(), 3u);
}

TEST_F(RealtimeSLOTrackerTest, HealthScore) {
  tracker_.RecordQuery(100, true, 0.82f, 0.03f);
  tracker_.RecordQuery(110, true, 0.84f, 0.03f);
  tracker_.RecordQuery(120, true, 0.86f, 0.03f);
  
  float health = tracker_.GetHealthScore();
  EXPECT_GE(health, 0.0f);
  EXPECT_LE(health, 100.0f);
}

TEST_F(RealtimeSLOTrackerTest, ComplianceWindow5Min) {
  tracker_.RecordQuery(150, true, 0.82f, 0.03f);
  auto snapshot = tracker_.GetComplianceSnapshot();
  
  EXPECT_GT(snapshot.window_start_us, 0u);
}

TEST_F(RealtimeSLOTrackerTest, UpdateComplianceThresholds) {
  tracker_.SetTarget("p95_latency_ms", 200);
  
  // After threshold update
  tracker_.RecordQuery(180, true, 0.82f, 0.03f);
  bool compliant = tracker_.UpdateCompliance();
  EXPECT_FALSE(compliant);
}

TEST_F(RealtimeSLOTrackerTest, EmptyTrackerCompliant) {
  bool compliant = tracker_.IsCompliant();
  EXPECT_TRUE(compliant);
}

}  // namespace themis::rag
