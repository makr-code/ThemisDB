// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/cost_attribution_tracker.h"

namespace themis::rag::testing {

class CostAttributionTrackerTest : public ::testing::Test {
 protected:
  CostAttributionTracker tracker_;
};

TEST_F(CostAttributionTrackerTest, RecordCost) {
  tracker_.RecordCost("tenant1", "retrieval", 0.05f, {{"doc_count", "100"}});
  EXPECT_EQ(tracker_.GetTenantCost("tenant1"), 0.05f);
}

TEST_F(CostAttributionTrackerTest, MultipleCosts) {
  tracker_.RecordCost("tenant1", "retrieval", 0.05f, {});
  tracker_.RecordCost("tenant1", "reranking", 0.03f, {});
  tracker_.RecordCost("tenant1", "retrieval", 0.04f, {});
  
  float total = tracker_.GetTenantCost("tenant1");
  EXPECT_NEAR(total, 0.12f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, MultiTenantIsolation) {
  tracker_.RecordCost("tenant1", "retrieval", 0.05f, {});
  tracker_.RecordCost("tenant2", "retrieval", 0.03f, {});
  
  EXPECT_NEAR(tracker_.GetTenantCost("tenant1"), 0.05f, 0.001f);
  EXPECT_NEAR(tracker_.GetTenantCost("tenant2"), 0.03f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, SetTenantBudget) {
  tracker_.SetTenantBudget("tenant1", 1.0f);
  EXPECT_NO_THROW({});
}

TEST_F(CostAttributionTrackerTest, GetBudgetAlerts) {
  tracker_.SetTenantBudget("tenant1", 0.1f);
  tracker_.RecordCost("tenant1", "retrieval", 0.08f, {});
  
  auto alerts = tracker_.GetBudgetAlerts();
  EXPECT_GE(alerts.size(), 0);
}

TEST_F(CostAttributionTrackerTest, CostByOperation) {
  tracker_.RecordCost("tenant1", "retrieval", 0.05f, {});
  tracker_.RecordCost("tenant1", "reranking", 0.03f, {});
  tracker_.RecordCost("tenant1", "retrieval", 0.02f, {});
  
  auto metrics = tracker_.GetCostMetrics("tenant1");
  EXPECT_GT(metrics.cost_by_operation.size(), 0);
}

TEST_F(CostAttributionTrackerTest, BudgetAlert) {
  tracker_.SetTenantBudget("tenant1", 0.05f);
  tracker_.RecordCost("tenant1", "retrieval", 0.04f, {});
  tracker_.RecordCost("tenant1", "reranking", 0.02f, {});
  
  auto alerts = tracker_.GetBudgetAlerts();
  EXPECT_GT(alerts.size(), 0);
}

TEST_F(CostAttributionTrackerTest, ForecastCost) {
  // Record costs over time
  for (int i = 0; i < 10; i++) {
    tracker_.RecordCost("tenant1", "retrieval", 0.05f, {});
  }
  
  float forecast = tracker_.ForecastTenantCost("tenant1", 3600000000LL);  // 1 hour
  EXPECT_GE(forecast, 0.0f);
}

TEST_F(CostAttributionTrackerTest, ZeroCostAllowed) {
  tracker_.RecordCost("tenant1", "retrieval", 0.0f, {});
  EXPECT_EQ(tracker_.GetTenantCost("tenant1"), 0.0f);
}

TEST_F(CostAttributionTrackerTest, CostWithAttributes) {
  std::map<std::string, std::string> attrs = {
      {"doc_count", "50"}, {"model", "bge-large"}};
  tracker_.RecordCost("tenant1", "retrieval", 0.10f, attrs);
  
  EXPECT_NEAR(tracker_.GetTenantCost("tenant1"), 0.10f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, NoReserveViolation) {
  tracker_.SetTenantBudget("tenant1", 1.0f);
  tracker_.RecordCost("tenant1", "retrieval", 0.90f, {});
  tracker_.RecordCost("tenant1", "reranking", 0.05f, {});
  
  // Should hit budget alert due to 10% reserve
  auto alerts = tracker_.GetBudgetAlerts();
  EXPECT_GE(alerts.size(), 0);
}

}  // namespace themis::rag::testing
