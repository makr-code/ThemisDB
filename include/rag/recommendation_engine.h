// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

// Forward declarations (not currently used)
// class CostModelBuilder;
// class GradientDescentOptimizer;

/// @brief Optimization recommendations for cost reduction.
///
/// Analyzes current system settings and cost data to recommend optimization
/// strategies. Uses cost models and optimization algorithms to identify
/// Pareto-optimal configurations that reduce cost while maintaining quality.
///
/// @details
/// Recommendation types:
/// - Config: Adjust settings (retrieval_count, rerank_threshold, etc)
/// - Refresh: Change freshness refresh interval
/// - Routing: Tenant-specific routing strategies
/// - Budget: Reallocate budget across tenants
/// - Scaling: Add/remove resources
///
/// Generation workflow:
/// 1. Train cost models from historical data
/// 2. Run optimization with current quality constraints
/// 3. Identify cost savings vs current config
/// 4. Estimate implementation effort and risk
/// 5. Rank by impact/effort ratio
/// 6. Present top recommendations with confidence intervals
///
/// Recommendation scoring:
/// - Impact: Estimated % cost reduction
/// - Confidence: Reliability of forecast (0-100%)
/// - Effort: Implementation complexity (1-5)
/// - Risk: Potential quality regression (1-5)
/// - ROI: (Impact * Confidence) / (Effort * Risk)
///
/// @code
/// auto engine = std::make_unique<OptimizationRecommendationEngine>();
/// engine->LoadCostData(historical_costs);
/// engine->SetConstraints({{"ndcg_10_min": 0.50}, {"latency_p95_ms_max": 500}});
/// auto recommendations = engine->GenerateRecommendations();
/// for (auto& rec : recommendations) {
///   std::cout << rec.description << " -> " << rec.estimated_cost_savings_pct
///             << "% savings (confidence: " << rec.confidence_pct << "%)" << std::endl;
/// }
/// @endcode
class OptimizationRecommendationEngine {
 public:
  /// @brief Single recommendation.
  struct Recommendation {
    enum class Type {
      ConfigChange,     ///< Change settings
      RefreshStrategy,  ///< Adjust refresh interval
      TenantRouting,    ///< Tenant-specific routing
      BudgetReallocation,
      ResourceScaling
    };

    enum class Priority {
      Low,
      Medium,
      High,
      Critical
    };

    Type type;
    Priority priority;
    std::string category;              ///< "retrieval", "reranking", "freshness", etc
    std::string description;           ///< Human-readable description
    std::map<std::string, float> changes;  ///< Config changes: variable_name -> new_value
    float estimated_cost_savings_usd;
    float estimated_cost_savings_pct;
    float estimated_latency_impact_pct;  ///< Negative = slower
    float confidence_pct;                ///< 0-100 confidence in estimate
    int effort_score;                    ///< 1-5 implementation difficulty
    int risk_score;                      ///< 1-5 quality regression risk
    float roi_score;                     ///< Impact / (Effort * Risk)
    std::string implementation_guide;
    std::vector<std::string> dependencies;  ///< Prerequisite changes
    bool requires_testing;
    std::string test_strategy;
  };

  /// @brief Recommendation context.
  struct Context {
    std::string tenant_id;
    std::string dataset_name;
    float current_cost_usd_per_query;
    float current_latency_p95_ms;
    float current_rerank_rate;
    float current_ndcg_10;
    std::map<std::string, float> current_config;
  };

  /// @brief Constructor.
  OptimizationRecommendationEngine();

  /// @brief Set optimization context.
  /// @param context Current system context.
  void SetContext(const Context& context);

  /// @brief Load historical cost data.
  /// @param cost_data Cost records for model training.
  void LoadCostData(const std::vector<std::map<std::string, float>>& cost_data);

  /// @brief Set quality constraints.
  /// @param constraints Map of constraint_name -> threshold value.
  void SetConstraints(const std::map<std::string, float>& constraints);

  /// @brief Generate recommendations.
  /// @param num_recommendations Max recommendations to return.
  /// @return Ranked recommendations (highest ROI first).
  std::vector<Recommendation> GenerateRecommendations(uint32_t num_recommendations = 10);

  /// @brief Get recommendation for specific category.
  /// @param category Category filter (e.g., "retrieval", "reranking").
  /// @return Top recommendations in category.
  std::vector<Recommendation> GetRecommendationsForCategory(const std::string& category);

  /// @brief Simulate recommendation impact.
  /// @param recommendation Recommendation to simulate.
  /// @return Simulated outcome (cost, latency, quality).
  std::map<std::string, float> SimulateRecommendation(const Recommendation& recommendation);

  /// @brief Estimate combined impact of multiple recommendations.
  /// @param recommendations Recommendations to simulate together.
  /// @return Combined outcome.
  std::map<std::string, float> SimulateCombined(
      const std::vector<Recommendation>& recommendations);

  /// @brief Get recommendation rationale.
  /// @param recommendation Recommendation to explain.
  /// @return Detailed explanation for recommendation.
  std::string GetRationale(const Recommendation& recommendation);

  /// @brief Export recommendations to report.
  /// @param recommendations Recommendations to export.
  /// @param output_path Output file path (JSON or markdown).
  /// @return true if exported successfully.
  bool ExportReport(
      const std::vector<Recommendation>& recommendations,
      const std::string& output_path);

 private:
  Context context_;
  std::vector<std::map<std::string, float>> cost_data_;
  std::map<std::string, float> constraints_;

  std::vector<Recommendation> GenerateConfigRecommendations();
  std::vector<Recommendation> GenerateRefreshRecommendations();
  std::vector<Recommendation> GenerateRoutingRecommendations();
  std::vector<Recommendation> GenerateBudgetRecommendations();
};

}  // namespace themis::rag
