// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/recommendation_engine.h"

#include <algorithm>
#include <cmath>

namespace themis::rag {

OptimizationRecommendationEngine::OptimizationRecommendationEngine() {}

void OptimizationRecommendationEngine::SetContext(const Context& context) {
  context_ = context;
}

void OptimizationRecommendationEngine::LoadCostData(
    const std::vector<std::map<std::string, float>>& cost_data) {
  cost_data_ = cost_data;
}

void OptimizationRecommendationEngine::SetConstraints(
    const std::map<std::string, float>& constraints) {
  constraints_ = constraints;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateRecommendations(uint32_t num_recommendations) {
  std::vector<Recommendation> recommendations;

  // Generate recommendations by category
  auto config_recs = GenerateConfigRecommendations();
  auto refresh_recs = GenerateRefreshRecommendations();
  auto routing_recs = GenerateRoutingRecommendations();
  auto budget_recs = GenerateBudgetRecommendations();

  // Combine all recommendations
  recommendations.insert(recommendations.end(), config_recs.begin(), config_recs.end());
  recommendations.insert(recommendations.end(), refresh_recs.begin(), refresh_recs.end());
  recommendations.insert(recommendations.end(), routing_recs.begin(), routing_recs.end());
  recommendations.insert(recommendations.end(), budget_recs.begin(), budget_recs.end());

  // Sort by ROI score (descending)
  std::sort(recommendations.begin(), recommendations.end(),
            [](const Recommendation& a, const Recommendation& b) { return a.roi_score > b.roi_score; });

  // Limit to num_recommendations
  if (recommendations.size() > num_recommendations) {
    recommendations.resize(num_recommendations);
  }

  return recommendations;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GetRecommendationsForCategory(const std::string& category) {
  auto all_recommendations = GenerateRecommendations();

  std::vector<Recommendation> filtered;
  for (const auto& rec : all_recommendations) {
    if (rec.category == category) {
      filtered.push_back(rec);
    }
  }

  return filtered;
}

std::map<std::string, float> OptimizationRecommendationEngine::SimulateRecommendation(
    const Recommendation& recommendation) {
  std::map<std::string, float> outcome;

  float simulated_cost_reduction = context_.current_cost_usd_per_query * recommendation.estimated_cost_savings_pct / 100.0f;

  outcome["estimated_cost_reduction_usd"] = simulated_cost_reduction;
  outcome["estimated_cost_after_optimization"] = context_.current_cost_usd_per_query - simulated_cost_reduction;
  outcome["estimated_latency_change_ms"] = context_.current_latency_p95_ms * recommendation.estimated_latency_impact_pct / 100.0f;
  outcome["confidence_pct"] = recommendation.confidence_pct;

  return outcome;
}

std::map<std::string, float> OptimizationRecommendationEngine::SimulateCombined(
    const std::vector<Recommendation>& recommendations) {
  std::map<std::string, float> combined_outcome;

  float total_cost_reduction = 0.0f;
  float total_latency_change = 0.0f;
  float min_confidence = 100.0f;

  for (const auto& rec : recommendations) {
    auto outcome = SimulateRecommendation(rec);
    total_cost_reduction += outcome["estimated_cost_reduction_usd"];
    total_latency_change += outcome["estimated_latency_change_ms"];
    min_confidence = std::min(min_confidence, rec.confidence_pct);
  }

  combined_outcome["total_cost_reduction_usd"] = total_cost_reduction;
  combined_outcome["final_cost_per_query_usd"] = context_.current_cost_usd_per_query - total_cost_reduction;
  combined_outcome["total_latency_change_ms"] = total_latency_change;
  combined_outcome["min_confidence_pct"] = min_confidence;

  return combined_outcome;
}

std::string OptimizationRecommendationEngine::GetRationale(
    const Recommendation& recommendation) {
  std::string rationale = "Recommendation: " + recommendation.description + "\n";
  rationale += "Category: " + recommendation.category + "\n";
  rationale += "Estimated savings: " + std::to_string(recommendation.estimated_cost_savings_pct) + "%\n";
  rationale += "Confidence: " + std::to_string(recommendation.confidence_pct) + "%\n";
  rationale += "Implementation difficulty: " + std::to_string(recommendation.effort_score) + "/5\n";
  rationale += "Quality regression risk: " + std::to_string(recommendation.risk_score) + "/5\n";
  rationale += "ROI score: " + std::to_string(recommendation.roi_score) + "\n";
  rationale += "\nImplementation guide:\n" + recommendation.implementation_guide + "\n";

  return rationale;
}

bool OptimizationRecommendationEngine::ExportReport(
    const std::vector<Recommendation>& recommendations,
    const std::string& output_path) {
  // TODO: Export recommendations to file (JSON or markdown)
  return true;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateConfigRecommendations() {
  std::vector<Recommendation> recommendations;

  // Recommendation 1: Reduce retrieval count
  {
    Recommendation rec;
    rec.type = Recommendation::Type::ConfigChange;
    rec.priority = Recommendation::Priority::High;
    rec.category = "retrieval";
    rec.description = "Reduce retrieval count from 100 to 50 documents";
    rec.changes["num_retrieved"] = 50.0f;
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.20f;
    rec.estimated_cost_savings_pct = 20.0f;
    rec.estimated_latency_impact_pct = -10.0f;  // 10% faster
    rec.confidence_pct = 85.0f;
    rec.effort_score = 2;
    rec.risk_score = 2;
    rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
    rec.implementation_guide = "1. Update retrieval_count param\n2. Run benchmark to verify impact\n3. A/B test with subset of traffic";
    rec.requires_testing = true;
    rec.test_strategy = "Offline benchmark + 1% canary";

    recommendations.push_back(rec);
  }

  // Recommendation 2: Increase rerank threshold
  {
    Recommendation rec;
    rec.type = Recommendation::Type::ConfigChange;
    rec.priority = Recommendation::Priority::Medium;
    rec.category = "reranking";
    rec.description = "Increase reranking confidence threshold to 0.8";
    rec.changes["rerank_threshold"] = 0.8f;
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.15f;
    rec.estimated_cost_savings_pct = 15.0f;
    rec.estimated_latency_impact_pct = -5.0f;
    rec.confidence_pct = 70.0f;
    rec.effort_score = 1;
    rec.risk_score = 3;  // Higher quality risk
    rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
    rec.implementation_guide = "1. Update rerank_threshold config\n2. Monitor NDCG metrics\n3. Alert on >5% NDCG regression";
    rec.requires_testing = true;

    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateRefreshRecommendations() {
  std::vector<Recommendation> recommendations;

  // Recommendation: Extend refresh interval
  {
    Recommendation rec;
    rec.type = Recommendation::Type::RefreshStrategy;
    rec.priority = Recommendation::Priority::Medium;
    rec.category = "freshness";
    rec.description = "Extend index refresh interval from hourly to 2 hours";
    rec.changes["refresh_interval_minutes"] = 120.0f;
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.10f;
    rec.estimated_cost_savings_pct = 10.0f;
    rec.estimated_latency_impact_pct = 0.0f;
    rec.confidence_pct = 60.0f;
    rec.effort_score = 2;
    rec.risk_score = 2;
    rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
    rec.implementation_guide = "1. Update refresh interval\n2. Monitor staleness percentiles\n3. Alert if p95 > 2h";
    rec.requires_testing = true;

    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateRoutingRecommendations() {
  std::vector<Recommendation> recommendations;

  // Recommendation: Tenant-specific routing
  {
    Recommendation rec;
    rec.type = Recommendation::Type::TenantRouting;
    rec.priority = Recommendation::Priority::Low;
    rec.category = "routing";
    rec.description = "Route low-priority tenants to cheaper retriever";
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.05f;
    rec.estimated_cost_savings_pct = 5.0f;
    rec.estimated_latency_impact_pct = 10.0f;  // Slightly slower
    rec.confidence_pct = 50.0f;
    rec.effort_score = 3;
    rec.risk_score = 2;
    rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
    rec.implementation_guide = "1. Define tenant tier mapping\n2. Update routing policy\n3. Monitor per-tenant metrics";

    recommendations.push_back(rec);
  }

  return recommendations;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateBudgetRecommendations() {
  std::vector<Recommendation> recommendations;

  // Placeholder: generate budget recommendations
  return recommendations;
}

}  // namespace themis::rag
