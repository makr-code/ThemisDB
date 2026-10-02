// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/recommendation_engine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace themis::rag {
namespace {

std::string ToPriorityString(
    OptimizationRecommendationEngine::Recommendation::Priority priority) {
  switch (priority) {
    case OptimizationRecommendationEngine::Recommendation::Priority::Low:
      return "low";
    case OptimizationRecommendationEngine::Recommendation::Priority::Medium:
      return "medium";
    case OptimizationRecommendationEngine::Recommendation::Priority::High:
      return "high";
    case OptimizationRecommendationEngine::Recommendation::Priority::Critical:
      return "critical";
  }
  return "low";
}

std::string ToTypeString(OptimizationRecommendationEngine::Recommendation::Type type) {
  switch (type) {
    case OptimizationRecommendationEngine::Recommendation::Type::ConfigChange:
      return "config_change";
    case OptimizationRecommendationEngine::Recommendation::Type::RefreshStrategy:
      return "refresh_strategy";
    case OptimizationRecommendationEngine::Recommendation::Type::TenantRouting:
      return "tenant_routing";
    case OptimizationRecommendationEngine::Recommendation::Type::BudgetReallocation:
      return "budget_reallocation";
    case OptimizationRecommendationEngine::Recommendation::Type::ResourceScaling:
      return "resource_scaling";
  }
  return "config_change";
}

std::string EscapeJson(const std::string& value) {
  std::string result;
  result.reserve(value.size());
  for (char ch : value) {
    switch (ch) {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result += ch;
        break;
    }
  }
  return result;
}

}  // namespace

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

  auto config_recs = GenerateConfigRecommendations();
  auto refresh_recs = GenerateRefreshRecommendations();
  auto routing_recs = GenerateRoutingRecommendations();
  auto budget_recs = GenerateBudgetRecommendations();

  recommendations.insert(recommendations.end(), config_recs.begin(), config_recs.end());
  recommendations.insert(recommendations.end(), refresh_recs.begin(), refresh_recs.end());
  recommendations.insert(recommendations.end(), routing_recs.begin(), routing_recs.end());
  recommendations.insert(recommendations.end(), budget_recs.begin(), budget_recs.end());

  std::sort(recommendations.begin(), recommendations.end(),
            [](const Recommendation& a, const Recommendation& b) {
              return a.roi_score > b.roi_score;
            });

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
    const auto outcome = SimulateRecommendation(rec);
    total_cost_reduction += outcome.at("estimated_cost_reduction_usd");
    total_latency_change += outcome.at("estimated_latency_change_ms");
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
  if (output_path.empty()) {
    return false;
  }

  const std::filesystem::path path(output_path);
  if (!path.parent_path().empty()) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
      return false;
    }
  }

  std::ofstream output(output_path);
  if (!output.is_open()) {
    return false;
  }

  output << "{\n";
  output << "  \"tenant_id\": \"" << EscapeJson(context_.tenant_id) << "\",\n";
  output << "  \"dataset_name\": \"" << EscapeJson(context_.dataset_name) << "\",\n";
  output << "  \"recommendations\": [\n";

  for (std::size_t i = 0; i < recommendations.size(); ++i) {
    const auto& rec = recommendations[i];
    output << "    {\n";
    output << "      \"type\": \"" << EscapeJson(ToTypeString(rec.type)) << "\",\n";
    output << "      \"priority\": \"" << EscapeJson(ToPriorityString(rec.priority)) << "\",\n";
    output << "      \"category\": \"" << EscapeJson(rec.category) << "\",\n";
    output << "      \"description\": \"" << EscapeJson(rec.description) << "\",\n";
    output << "      \"estimated_cost_savings_usd\": " << rec.estimated_cost_savings_usd << ",\n";
    output << "      \"estimated_cost_savings_pct\": " << rec.estimated_cost_savings_pct << ",\n";
    output << "      \"estimated_latency_impact_pct\": " << rec.estimated_latency_impact_pct << ",\n";
    output << "      \"confidence_pct\": " << rec.confidence_pct << ",\n";
    output << "      \"roi_score\": " << rec.roi_score << "\n";
    output << "    }";
    if (i + 1 != recommendations.size()) {
      output << ",";
    }
    output << "\n";
  }

  output << "  ]\n";
  output << "}\n";
  return true;
}

std::vector<OptimizationRecommendationEngine::Recommendation>
OptimizationRecommendationEngine::GenerateConfigRecommendations() {
  std::vector<Recommendation> recommendations;

  {
    Recommendation rec;
    rec.type = Recommendation::Type::ConfigChange;
    rec.priority = Recommendation::Priority::High;
    rec.category = "retrieval";
    rec.description = "Reduce retrieval count from 100 to 50 documents";
    rec.changes["num_retrieved"] = 50.0f;
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.20f;
    rec.estimated_cost_savings_pct = 20.0f;
    rec.estimated_latency_impact_pct = -10.0f;
    rec.confidence_pct = 85.0f;
    rec.effort_score = 2;
    rec.risk_score = 2;
    rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
    rec.implementation_guide = "1. Update retrieval_count param\n2. Run benchmark to verify impact\n3. A/B test with subset of traffic";
    rec.requires_testing = true;
    rec.test_strategy = "Offline benchmark + 1% canary";
    recommendations.push_back(rec);
  }

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
    rec.risk_score = 3;
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

  {
    Recommendation rec;
    rec.type = Recommendation::Type::TenantRouting;
    rec.priority = Recommendation::Priority::Low;
    rec.category = "routing";
    rec.description = "Route low-priority tenants to cheaper retriever";
    rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.05f;
    rec.estimated_cost_savings_pct = 5.0f;
    rec.estimated_latency_impact_pct = 10.0f;
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

  Recommendation rec;
  rec.type = Recommendation::Type::BudgetReallocation;
  rec.priority = Recommendation::Priority::Medium;
  rec.category = "budget";
  rec.description = "Reallocate 10% of retrieval budget to rerank optimization";
  rec.changes["budget_pct_rerank"] = 10.0f;
  rec.estimated_cost_savings_usd = context_.current_cost_usd_per_query * 0.08f;
  rec.estimated_cost_savings_pct = 8.0f;
  rec.estimated_latency_impact_pct = -2.0f;
  rec.confidence_pct = 65.0f;
  rec.effort_score = 2;
  rec.risk_score = 3;
  rec.roi_score = (rec.estimated_cost_savings_pct * rec.confidence_pct) / (rec.effort_score * rec.risk_score);
  rec.implementation_guide = "1. Move reserved budget\n2. Validate resource utilization\n3. Review tenant-specific SLOs";
  rec.requires_testing = true;
  rec.test_strategy = "Budget replay + SLO validation";
  recommendations.push_back(rec);

  return recommendations;
}

}  // namespace themis::rag
