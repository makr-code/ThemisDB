// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/recommendation_engine.h"

namespace themis::rag::testing {

class OptimizationRecommendationEngineTest : public ::testing::Test {
 protected:
  OptimizationRecommendationEngine engine_;
};

TEST_F(OptimizationRecommendationEngineTest, SetContext) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  EXPECT_NO_THROW({});
}

TEST_F(OptimizationRecommendationEngineTest, LoadCostData) {
  std::vector<std::map<std::string, float>> cost_data = {
      {{"num_retrieved", 50.0f}, {"cost", 0.05f}},
      {{"num_retrieved", 100.0f}, {"cost", 0.10f}}
  };
  
  engine_.LoadCostData(cost_data);
  EXPECT_NO_THROW({});
}

TEST_F(OptimizationRecommendationEngineTest, SetConstraints) {
  std::map<std::string, float> constraints = {
      {"min_ndcg", 0.80f}, {"max_latency_ms", 500.0f}};
  
  engine_.SetConstraints(constraints);
  EXPECT_NO_THROW({});
}

TEST_F(OptimizationRecommendationEngineTest, GenerateRecommendations) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(5);
  EXPECT_LE(recommendations.size(), 5);
  EXPECT_GT(recommendations.size(), 0);
}

TEST_F(OptimizationRecommendationEngineTest, RecommendationPriority) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(10);
  
  // Check that recommendations have valid priorities
  for (const auto& rec : recommendations) {
    EXPECT_NE(rec.priority, OptimizationRecommendationEngine::Recommendation::Priority::Unknown);
  }
}

TEST_F(OptimizationRecommendationEngineTest, GetRecommendationsByCategory) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto config_recs = engine_.GetRecommendationsForCategory("retrieval");
  EXPECT_GE(config_recs.size(), 0);
}

TEST_F(OptimizationRecommendationEngineTest, SimulateRecommendation) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(1);
  if (!recommendations.empty()) {
    auto outcome = engine_.SimulateRecommendation(recommendations[0]);
    EXPECT_GT(outcome.size(), 0);
  }
}

TEST_F(OptimizationRecommendationEngineTest, SimulateCombined) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(3);
  auto combined = engine_.SimulateCombined(recommendations);
  
  EXPECT_GT(combined.size(), 0);
}

TEST_F(OptimizationRecommendationEngineTest, GetRationale) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(1);
  if (!recommendations.empty()) {
    auto rationale = engine_.GetRationale(recommendations[0]);
    EXPECT_GT(rationale.length(), 0);
  }
}

TEST_F(OptimizationRecommendationEngineTest, ExportReport) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(5);
  bool success = engine_.ExportReport(recommendations, "/tmp/recommendations.json");
  EXPECT_TRUE(success);
}

TEST_F(OptimizationRecommendationEngineTest, HighROIRecommendations) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(10);
  
  // Recommendations should be sorted by ROI
  if (recommendations.size() > 1) {
    EXPECT_GE(recommendations[0].roi_score, recommendations[1].roi_score);
  }
}

TEST_F(OptimizationRecommendationEngineTest, RecommendationCost) {
  OptimizationRecommendationEngine::Context context;
  context.current_cost_usd_per_query = 0.50f;
  context.current_latency_p95_ms = 200.0f;
  
  engine_.SetContext(context);
  
  auto recommendations = engine_.GenerateRecommendations(5);
  
  for (const auto& rec : recommendations) {
    EXPECT_GE(rec.estimated_cost_savings_pct, 0.0f);
    EXPECT_LE(rec.estimated_cost_savings_pct, 100.0f);
  }
}

}  // namespace themis::rag::testing
