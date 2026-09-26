// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/reranker_budget_gate.h"

namespace themis::rag {

class RerankerBudgetGateTest : public ::testing::Test {
 protected:
  RerankerBudgetGate gate_{0.1f, 100.0f};  // ROI threshold 0.1, 100ms max
};

TEST_F(RerankerBudgetGateTest, ROIEstimation) {
  RerankerBudgetGate::BudgetContext context;
  context.query_text = "What is the capital of France?";
  context.available_budget = 100.0f;
  context.hybrid_results = {};

  auto roi = gate_.EstimateROI(context);
  
  EXPECT_GE(roi.estimated_quality_gain, 0.0f);
  EXPECT_GE(roi.estimated_cost, 0.0f);
  EXPECT_LE(roi.estimated_cost, 100.0f);  // Must be under hard cap
}

TEST_F(RerankerBudgetGateTest, ShouldRerankedWhenROIFavorable) {
  RerankerBudgetGate::BudgetContext context;
  context.query_text = "What is machine learning in the context of "
                       "neural networks and deep learning algorithms?";  // Long query
  context.available_budget = 100.0f;
  context.hybrid_results = {};

  auto roi = gate_.EstimateROI(context);
  
  // Long query should have higher ROI
  if (roi.should_rerank) {
    EXPECT_GE(roi.roi_ratio, gate_.GetROIThreshold());
  }
}

TEST_F(RerankerBudgetGateTest, HardCapEnforcement) {
  RerankerBudgetGate gate{0.05f, 50.0f};  // Lower hard cap

  RerankerBudgetGate::BudgetContext context;
  context.query_text = "This is a very long query that might exceed the hard cap";
  context.available_budget = 200.0f;
  context.hybrid_results = {};

  auto roi = gate.EstimateROI(context);
  
  EXPECT_LE(roi.estimated_cost, 50.0f);
}

TEST_F(RerankerBudgetGateTest, InsufficientBudgetRejection) {
  RerankerBudgetGate::BudgetContext context;
  context.query_text = "What is AI?";
  context.available_budget = 1.0f;  // Very low budget
  context.hybrid_results = {};

  auto roi = gate_.EstimateROI(context);
  
  if (roi.estimated_cost > context.available_budget) {
    EXPECT_FALSE(roi.should_rerank);
  }
}

TEST_F(RerankerBudgetGateTest, GetTenantBudgetStatus) {
  gate_.SetTenantBudget("tenant_1", 1000.0f);
  
  auto status = gate_.GetTenantBudgetStatus("tenant_1");
  
  EXPECT_EQ(status.available, 1000.0f);
  EXPECT_NEAR(status.percentage_used, 0.0f, 0.1f);
  EXPECT_FALSE(status.at_risk);
}

TEST_F(RerankerBudgetGateTest, ConsumeBudgetAndTrack) {
  gate_.SetTenantBudget("tenant_2", 100.0f);
  
  bool consumed = gate_.ConsumeBudget("tenant_2", 30.0f);
  EXPECT_TRUE(consumed);

  auto status = gate_.GetTenantBudgetStatus("tenant_2");
  EXPECT_EQ(status.consumed, 30.0f);
  EXPECT_EQ(status.available, 70.0f);
}

TEST_F(RerankerBudgetGateTest, BudgetCeilingEnforcement) {
  gate_.SetTenantBudget("tenant_3", 100.0f);

  // Consume up to 85%
  gate_.ConsumeBudget("tenant_3", 85.0f);

  // Try to consume more (would exceed 90% ceiling)
  bool consumed = gate_.ConsumeBudget("tenant_3", 10.0f);  // Would be 95% total
  EXPECT_FALSE(consumed);

  // Status should still show 85% consumed
  auto status = gate_.GetTenantBudgetStatus("tenant_3");
  EXPECT_EQ(status.consumed, 85.0f);
}

TEST_F(RerankerBudgetGateTest, MultiTenantBudgetIsolation) {
  gate_.SetTenantBudget("tenant_a", 100.0f);
  gate_.SetTenantBudget("tenant_b", 200.0f);

  gate_.ConsumeBudget("tenant_a", 50.0f);
  gate_.ConsumeBudget("tenant_b", 100.0f);

  auto status_a = gate_.GetTenantBudgetStatus("tenant_a");
  auto status_b = gate_.GetTenantBudgetStatus("tenant_b");

  EXPECT_EQ(status_a.consumed, 50.0f);
  EXPECT_EQ(status_b.consumed, 100.0f);
  EXPECT_EQ(status_a.available, 50.0f);
  EXPECT_EQ(status_b.available, 100.0f);
}

TEST_F(RerankerBudgetGateTest, ResetBudget) {
  gate_.SetTenantBudget("tenant_4", 100.0f);
  gate_.ConsumeBudget("tenant_4", 50.0f);

  gate_.ResetTenantBudget("tenant_4");

  auto status = gate_.GetTenantBudgetStatus("tenant_4");
  EXPECT_EQ(status.consumed, 0.0f);
  EXPECT_EQ(status.available, 100.0f);
}

TEST_F(RerankerBudgetGateTest, SetROIThreshold) {
  gate_.SetROIThreshold(0.2f);
  
  RerankerBudgetGate::BudgetContext context;
  context.query_text = "test";
  context.available_budget = 100.0f;
  context.hybrid_results = {};

  auto roi = gate_.EstimateROI(context);
  
  if (roi.roi_ratio < 0.2f) {
    EXPECT_FALSE(roi.should_rerank);
  }
}

}  // namespace themis::rag
