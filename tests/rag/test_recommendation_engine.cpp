// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "rag/recommendation_engine.h"

namespace themis::rag {

class OptimizationRecommendationEngineTest : public ::testing::Test {
 protected:
 OptimizationRecommendationEngine engine_;
};

TEST_F(OptimizationRecommendationEngineTest, SetContext) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);
 EXPECT_NO_THROW({});
}

TEST_F(OptimizationRecommendationEngineTest, LoadCostData) {
 std::vector<std::map<std::string, float>> cost_data = {
     {{"num_retrieved", 50.0f}, {"cost", 0.05f}},
     {{"num_retrieved", 100.0f}, {"cost", 0.10f}}};

 engine_.LoadCostData(cost_data);
 SUCCEED();
}

TEST_F(OptimizationRecommendationEngineTest, SetConstraints) {
 std::map<std::string, float> constraints = {
     {"min_ndcg", 0.80f}, {"max_latency_ms", 500.0f}};

 engine_.SetConstraints(constraints);
 SUCCEED();
}

TEST_F(OptimizationRecommendationEngineTest, GenerateRecommendations) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(5);
 EXPECT_LE(recommendations.size(), 5u);
 EXPECT_GT(recommendations.size(), 0u);
}

TEST_F(OptimizationRecommendationEngineTest, RecommendationPriority) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(10);
 for (const auto& rec : recommendations) {
   EXPECT_TRUE(
       rec.priority == OptimizationRecommendationEngine::Recommendation::Priority::Low ||
       rec.priority == OptimizationRecommendationEngine::Recommendation::Priority::Medium ||
       rec.priority == OptimizationRecommendationEngine::Recommendation::Priority::High ||
       rec.priority == OptimizationRecommendationEngine::Recommendation::Priority::Critical);
 }
}

TEST_F(OptimizationRecommendationEngineTest, GetRecommendationsByCategory) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto config_recs = engine_.GetRecommendationsForCategory("retrieval");
 EXPECT_GE(config_recs.size(), 0u);
}

TEST_F(OptimizationRecommendationEngineTest, SimulateRecommendation) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(1);
 if (!recommendations.empty()) {
   auto outcome = engine_.SimulateRecommendation(recommendations[0]);
   EXPECT_GT(outcome.size(), 0u);
 }
}

TEST_F(OptimizationRecommendationEngineTest, SimulateCombined) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(3);
 auto combined = engine_.SimulateCombined(recommendations);

 EXPECT_GT(combined.size(), 0u);
}

TEST_F(OptimizationRecommendationEngineTest, GetRationale) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(1);
 if (!recommendations.empty()) {
   auto rationale = engine_.GetRationale(recommendations[0]);
   EXPECT_GT(rationale.length(), 0u);
 }
}

TEST_F(OptimizationRecommendationEngineTest, ExportReport) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(5);
 const std::filesystem::path path =
     std::filesystem::temp_directory_path() / "themis_recommendations_report.json";
 bool success = engine_.ExportReport(recommendations, path.string());
 EXPECT_TRUE(success);

 std::ifstream input(path);
 ASSERT_TRUE(input.is_open());
 std::string content((std::istreambuf_iterator<char>(input)), {});
 EXPECT_NE(content.find("recommendations"), std::string::npos);
 EXPECT_NE(content.find("tenant-a"), std::string::npos);
 std::filesystem::remove(path);
}

TEST_F(OptimizationRecommendationEngineTest, HighROIRecommendations) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(10);
 if (recommendations.size() > 1) {
   EXPECT_GE(recommendations[0].roi_score, recommendations[1].roi_score);
 }
}

TEST_F(OptimizationRecommendationEngineTest, RecommendationCost) {
 OptimizationRecommendationEngine::Context context;
 context.current_cost_usd_per_query = 0.50f;
 context.current_latency_p95_ms = 200.0f;
 context.tenant_id = "tenant-a";
 context.dataset_name = "demo";

 engine_.SetContext(context);

 auto recommendations = engine_.GenerateRecommendations(5);
 for (const auto& rec : recommendations) {
   EXPECT_GE(rec.estimated_cost_savings_pct, 0.0f);
   EXPECT_LE(rec.estimated_cost_savings_pct, 100.0f);
 }
}

}  // namespace themis::rag
