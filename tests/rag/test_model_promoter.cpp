/**
 * @file test_model_promoter.cpp
 * @brief Production-backed coverage for model canary routing and rollback.
 */

#include <array>

#include <gtest/gtest.h>

#include "rag/model_evaluator.h"
#include "rag/model_promoter.h"
#include "rag/model_registry.h"

namespace themis::rag::lifecycle {
namespace {

uint32_t RegisterCandidate(ModelRegistry& registry, const std::string& model_id) {
  const auto version = registry.RegisterModel(
      model_id, "quality-dataset", R"({"ndcg@10":0.8})",
      R"({"latency_ms":40.0})", model_id + ".bin");
  EXPECT_TRUE(registry.UpdateModelStatus(version, ModelStatus::kValidated));
  EXPECT_TRUE(registry.UpdateModelStatus(version, ModelStatus::kCandidate));
  return version;
}

uint32_t RegisterDeployedBaseline(ModelRegistry& registry) {
  const auto version = RegisterCandidate(registry, "baseline");
  EXPECT_TRUE(registry.UpdateModelStatus(version, ModelStatus::kDeployed));
  return version;
}

TEST(ModelPromoterTest, AdvancesCanaryPhasesAndTrafficBeforePromotion) {
  ModelRegistry registry("");
  const auto baseline_version = RegisterDeployedBaseline(registry);
  const auto candidate_version = RegisterCandidate(registry, "candidate");
  ModelEvaluator evaluator(registry);
  ModelPromoter promoter(registry, evaluator);

  ASSERT_TRUE(promoter.StartCanary(candidate_version));
  auto decision = promoter.GetTrafficSplit();
  EXPECT_EQ(decision.baseline_version, baseline_version);
  EXPECT_EQ(decision.canary_version, candidate_version);
  EXPECT_EQ(decision.phase, CanaryPhase::kShadow);
  EXPECT_DOUBLE_EQ(decision.canary_traffic_pct, 0.0);

  constexpr std::array phases{
      CanaryPhase::kCanary5, CanaryPhase::kCanary10, CanaryPhase::kCanary25,
      CanaryPhase::kCanary50, CanaryPhase::kDeployed};
  constexpr std::array traffic_percentages{5.0, 10.0, 25.0, 50.0, 100.0};
  for (std::size_t i = 0; i < phases.size(); ++i) {
    ASSERT_TRUE(promoter.AdvancePhase());
    decision = promoter.GetTrafficSplit();
    EXPECT_EQ(decision.phase, phases[i]);
    EXPECT_DOUBLE_EQ(decision.canary_traffic_pct, traffic_percentages[i]);
    EXPECT_EQ(decision.baseline_version, baseline_version);
    EXPECT_EQ(decision.canary_version, candidate_version);
  }
  EXPECT_FALSE(promoter.AdvancePhase());
  ASSERT_TRUE(promoter.FinalizeDeployment());
  EXPECT_FALSE(promoter.IsCanaryActive());

  const auto baseline = registry.GetByVersion(baseline_version);
  const auto candidate = registry.GetByVersion(candidate_version);
  ASSERT_TRUE(baseline.has_value());
  ASSERT_TRUE(candidate.has_value());
  EXPECT_EQ(baseline->status, ModelStatus::kRetired);
  EXPECT_EQ(candidate->status, ModelStatus::kDeployed);
  ASSERT_TRUE(registry.GetDeployedModel().has_value());
  EXPECT_EQ(registry.GetDeployedModel()->version, candidate_version);
}

TEST(ModelPromoterTest, QualityRegressionRollsBackAndFailsCandidateInRegistry) {
  ModelRegistry registry("");
  const auto baseline_version = RegisterDeployedBaseline(registry);
  const auto candidate_version = RegisterCandidate(registry, "regressing-candidate");
  ModelEvaluator evaluator(registry);
  ModelPromoter promoter(registry, evaluator, 0.05);
  bool regression_callback_invoked = false;
  promoter.SetQualityRegressionCallback(
      [&promoter, &regression_callback_invoked](const std::string& metric, double baseline,
                                                double current, double threshold) {
        regression_callback_invoked = true;
        EXPECT_EQ(metric, "ndcg@10");
        EXPECT_DOUBLE_EQ(baseline, 0.80);
        EXPECT_DOUBLE_EQ(current, 0.70);
        EXPECT_DOUBLE_EQ(threshold, 0.05);
        EXPECT_TRUE(promoter.IsCanaryActive());
      });

  ASSERT_TRUE(promoter.StartCanary(candidate_version));
  ASSERT_TRUE(promoter.AdvancePhase());
  EXPECT_EQ(promoter.GetCurrentPhase(), CanaryPhase::kCanary5);

  EXPECT_FALSE(promoter.ReportMetric("ndcg@10", 0.70, 0.80));
  EXPECT_TRUE(regression_callback_invoked);
  EXPECT_FALSE(promoter.IsCanaryActive());
  EXPECT_EQ(promoter.GetCurrentPhase(), CanaryPhase::kNone);
  EXPECT_FALSE(promoter.Rollback("already rolled back"));

  const auto baseline = registry.GetByVersion(baseline_version);
  const auto candidate = registry.GetByVersion(candidate_version);
  ASSERT_TRUE(baseline.has_value());
  ASSERT_TRUE(candidate.has_value());
  EXPECT_EQ(baseline->status, ModelStatus::kDeployed);
  EXPECT_EQ(candidate->status, ModelStatus::kFailed);
  EXPECT_EQ(candidate->notes, "Canary rollback: Quality regression in ndcg@10");
}

}  // namespace
}  // namespace themis::rag::lifecycle
