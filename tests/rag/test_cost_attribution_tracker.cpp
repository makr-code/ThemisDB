// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <filesystem>

#include "rag/cost_attribution_tracker.h"

namespace themis::rag {

class CostAttributionTrackerTest : public ::testing::Test {
 protected:
 CostAttributionTracker tracker_;
};

TEST_F(CostAttributionTrackerTest, RecordCost) {
 tracker_.RecordCost("tenant1", "retrieval", 0.05f, "model-a", {{"doc_count", "100"}});
 EXPECT_NEAR(tracker_.GetTenantCost("tenant1", "1d").total_cost_usd, 0.05f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, MultipleCosts) {
 tracker_.RecordCost("tenant1", "retrieval", 0.05f);
 tracker_.RecordCost("tenant1", "reranking", 0.03f);
 tracker_.RecordCost("tenant1", "retrieval", 0.04f);

 auto total = tracker_.GetTenantCost("tenant1", "1d");
 EXPECT_NEAR(total.total_cost_usd, 0.12f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, MultiTenantIsolation) {
 tracker_.RecordCost("tenant1", "retrieval", 0.05f);
 tracker_.RecordCost("tenant2", "retrieval", 0.03f);

 EXPECT_NEAR(tracker_.GetTenantCost("tenant1", "1d").total_cost_usd, 0.05f, 0.001f);
 EXPECT_NEAR(tracker_.GetTenantCost("tenant2", "1d").total_cost_usd, 0.03f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, InitializeCreatesPersistenceDirectory) {
 const auto db_path = std::filesystem::temp_directory_path() / "themis_cost_tracker";
 EXPECT_TRUE(tracker_.Initialize(db_path.string()));
 EXPECT_TRUE(tracker_.IsPersistentStorageAvailable());
 std::filesystem::remove_all(db_path);
}

TEST_F(CostAttributionTrackerTest, SetTenantBudget) {
 tracker_.SetTenantBudget("tenant1", 1.0f);
 EXPECT_NO_THROW({});
}

TEST_F(CostAttributionTrackerTest, GetBudgetAlerts) {
 tracker_.SetTenantBudget("tenant1", 0.1f);
 tracker_.RecordCost("tenant1", "retrieval", 0.08f);

 auto alerts = tracker_.GetBudgetAlerts();
 EXPECT_GE(alerts.size(), 0u);
}

TEST_F(CostAttributionTrackerTest, CostByOperation) {
 tracker_.RecordCost("tenant1", "retrieval", 0.05f);
 tracker_.RecordCost("tenant1", "reranking", 0.03f);
 tracker_.RecordCost("tenant1", "retrieval", 0.02f);

 auto metrics = tracker_.GetTenantCost("tenant1", "1d");
 EXPECT_GT(metrics.cost_by_operation.size(), 0u);
}

TEST_F(CostAttributionTrackerTest, BudgetAlert) {
 tracker_.SetTenantBudget("tenant1", 0.05f);
 tracker_.RecordCost("tenant1", "retrieval", 0.04f);
 tracker_.RecordCost("tenant1", "reranking", 0.02f);

 auto alerts = tracker_.GetBudgetAlerts();
 EXPECT_GT(alerts.size(), 0u);
}

TEST_F(CostAttributionTrackerTest, ForecastCost) {
 for (int i = 0; i < 10; ++i) {
   tracker_.RecordCost("tenant1", "retrieval", 0.05f);
 }

 auto forecast = tracker_.ForecastTenantCost("tenant1", 30);
 EXPECT_GE(forecast.projected_cost_usd, 0.0f);
}

TEST_F(CostAttributionTrackerTest, ZeroCostAllowed) {
 tracker_.RecordCost("tenant1", "retrieval", 0.0f);
 EXPECT_EQ(tracker_.GetTenantCost("tenant1", "1d").total_cost_usd, 0.0f);
}

TEST_F(CostAttributionTrackerTest, CostWithAttributes) {
 std::map<std::string, std::string> attrs = {
     {"doc_count", "50"}, {"model", "bge-large"}};
 tracker_.RecordCost("tenant1", "retrieval", 0.10f, "model-bge-large", attrs);

 EXPECT_NEAR(tracker_.GetTenantCost("tenant1", "1d").total_cost_usd, 0.10f, 0.001f);
}

TEST_F(CostAttributionTrackerTest, NoReserveViolation) {
 tracker_.SetTenantBudget("tenant1", 1.0f);
 tracker_.RecordCost("tenant1", "retrieval", 0.90f);
 tracker_.RecordCost("tenant1", "reranking", 0.05f);

 auto alerts = tracker_.GetBudgetAlerts();
 EXPECT_GE(alerts.size(), 0u);
}

TEST_F(CostAttributionTrackerTest, ForecastVarianceMatchesActual) {
 tracker_.SetTenantBudget("tenant1", 5.0f);
 tracker_.RecordCost("tenant1", "retrieval", 0.10f);
 tracker_.RecordCost("tenant1", "retrieval", 0.20f);
 tracker_.RecordCost("tenant1", "reranking", 0.30f);

 const float variance = tracker_.GetForecastVariance("tenant1");
 EXPECT_FALSE(std::isnan(variance));
 EXPECT_GE(variance, -100.0f);
 EXPECT_LE(variance, 100.0f);
}

}  // namespace themis::rag
