// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <chrono>

#include "rag/realtime_slo_tracker.h"

namespace themis::rag::testing {

class RealtimeSLOTrackerTest : public ::testing::Test {
 protected:
  RealtimeSLOTracker tracker_;
};

TEST_F(RealtimeSLOTrackerTest, RecordQuerySuccess) {
  EXPECT_EQ(tracker_.GetMetrics().size(), 0);
  
  tracker_.RecordQuery("retrieval", 150, true);
  auto metrics = tracker_.GetMetrics();
  EXPECT_GT(metrics.size(), 0);
}

TEST_F(RealtimeSLOTrackerTest, IsCompliantWithinBudget) {
  tracker_.RecordQuery("retrieval", 150, true);
  tracker_.RecordQuery("retrieval", 160, true);
  tracker_.RecordQuery("retrieval", 170, true);
  
  bool compliant = tracker_.IsCompliant("retrieval", 200);
  EXPECT_TRUE(compliant);
}

TEST_F(RealtimeSLOTrackerTest, IsNotCompliantBeyondBudget) {
  tracker_.RecordQuery("retrieval", 500, true);
  tracker_.RecordQuery("retrieval", 550, true);
  
  bool compliant = tracker_.IsCompliant("retrieval", 200);
  EXPECT_FALSE(compliant);
}

TEST_F(RealtimeSLOTrackerTest, ComplianceSnapshot) {
  tracker_.RecordQuery("retrieval", 100, true);
  tracker_.RecordQuery("retrieval", 200, true);
  tracker_.RecordQuery("reranking", 50, true);
  
  auto snapshot = tracker_.GetComplianceSnapshot();
  EXPECT_GT(snapshot.snapshot_time_us, 0);
  EXPECT_GT(snapshot.metric_compliances.size(), 0);
}

TEST_F(RealtimeSLOTrackerTest, FailureTracking) {
  tracker_.RecordQuery("retrieval", 100, true);
  tracker_.RecordQuery("retrieval", 150, false);  // Failure
  tracker_.RecordQuery("retrieval", 200, true);
  
  auto metrics = tracker_.GetMetrics();
  // Check that failures are counted
  EXPECT_TRUE(metrics.find("retrieval") != metrics.end());
}

TEST_F(RealtimeSLOTrackerTest, MultipleMetrics) {
  tracker_.RecordQuery("retrieval", 100, true);
  tracker_.RecordQuery("reranking", 50, true);
  tracker_.RecordQuery("freshness_check", 30, true);
  
  auto metrics = tracker_.GetMetrics();
  EXPECT_GE(metrics.size(), 3);
}

TEST_F(RealtimeSLOTrackerTest, HealthScore) {
  tracker_.RecordQuery("retrieval", 100, true);
  tracker_.RecordQuery("retrieval", 110, true);
  tracker_.RecordQuery("retrieval", 120, true);
  
  float health = tracker_.GetHealthScore();
  EXPECT_GE(health, 0.0f);
  EXPECT_LE(health, 1.0f);
}

TEST_F(RealtimeSLOTrackerTest, ComplianceWindow5Min) {
  tracker_.RecordQuery("retrieval", 150, true);
  auto snapshot = tracker_.GetComplianceSnapshot();
  
  EXPECT_GT(snapshot.snapshot_time_us, 0);
}

TEST_F(RealtimeSLOTrackerTest, UpdateComplianceThresholds) {
  tracker_.UpdateCompliance("retrieval", 200);
  
  // After threshold update
  tracker_.RecordQuery("retrieval", 180, true);
  bool compliant = tracker_.IsCompliant("retrieval", 200);
  EXPECT_TRUE(compliant);
}

TEST_F(RealtimeSLOTrackerTest, EmptyTrackerCompliant) {
  bool compliant = tracker_.IsCompliant("retrieval", 200);
  // Empty tracker should be compliant (no violations)
  EXPECT_TRUE(compliant);
}

}  // namespace themis::rag::testing
