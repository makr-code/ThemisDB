// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "rag/common_types.h"

namespace themis::rag {

/// @brief Re-ranking ROI (Return on Investment) gate for cost-effective re-ranking.
///
/// Makes per-query decisions on whether to apply cross-encoder re-ranking based on
/// predicted quality gain vs. cost. Prevents wasteful re-ranking on queries where
/// improvement is unlikely or not worth the cost.
///
/// @details
/// ROI estimation formula:
///   estimated_quality_gain = base_gain * query_complexity_factor * result_uncertainty
///   estimated_cost = cross_encoder_latency + (context_tokens * token_cost)
///   roi_ratio = estimated_quality_gain / estimated_cost
///   decision = (roi_ratio > threshold) AND (budget_available)
///
/// Thresholds:
/// - ROI threshold: 0.1 (1% quality gain / 10ms cost → rerank)
/// - Budget reserve: 10% (never exceed 90% of tenant budget)
/// - Max per-query cost: 100ms (hard cap)
///
/// @note Latency target: <5ms p99 (decision computation only)
class RerankerBudgetGate {
 public:
  /// @brief Budget context for ROI estimation.
  struct BudgetContext {
    std::string tenant_id;
    std::string query_id;
    const std::string& query_text;
    const std::vector<Document>& hybrid_results;  ///< k=50 results
    float available_budget;                       ///< ms or credits
  };

  /// @brief ROI decision result.
  struct RankingROI {
    bool should_rerank;                ///< True if ROI favorable
    float estimated_quality_gain;      ///< Predicted nDCG@10 delta
    float estimated_cost;              ///< ms or credits
    float roi_ratio;                   ///< gain / cost
    std::string rationale;             ///< Explanation for human review
  };

  /// @brief Tenant budget status.
  struct BudgetStatus {
    float consumed;                    ///< Credits already used
    float available;                   ///< Credits remaining
    float percentage_used;             ///< (consumed / total) * 100
    bool at_risk;                      ///< True if >80% used
  };

  /// @brief Constructor.
  /// 
  /// @param roi_threshold ROI threshold for reranking decision (default 0.1).
  /// @param max_cost_per_query Hard ceiling on single-query cost (default 100ms).
  RerankerBudgetGate(float roi_threshold = 0.1f,
                     float max_cost_per_query = 100.0f);

  /// @brief Destructor.
  ~RerankerBudgetGate() = default;

  /// @brief Estimate ROI and decide whether to rerank.
  ///
  /// @param context Budget context with query and results.
  /// @return RankingROI with decision and rationale.
  ///
  /// @details
  /// - Analyzes query length and complexity
  /// - Computes result uncertainty (variance in top-5 scores)
  /// - Estimates quality gain based on uncertainty
  /// - Estimates cost (model latency + token cost)
  /// - Computes ROI = gain / cost
  /// - Checks budget availability
  /// - Respects hard caps (100ms, 10% reserve)
  RankingROI EstimateROI(const BudgetContext& context);

  /// @brief Get tenant budget status.
  ///
  /// @param tenant_id Tenant identifier.
  /// @return Budget usage information.
  BudgetStatus GetTenantBudgetStatus(const std::string& tenant_id);

  /// @brief Enforce budget ceiling: consume credits if reranking proceeds.
  ///
  /// @param tenant_id Tenant identifier.
  /// @param cost Cost to charge (ms or credits).
  /// @return true if cost applied, false if budget exceeded.
  ///
  /// @details
  /// - Checks if cost would exceed 90% ceiling
  /// - Atomically deducts cost if under limit
  /// - Returns false if deduction would violate ceiling
  bool ConsumeBudget(const std::string& tenant_id, float cost);

  /// @brief Set per-tenant budget.
  ///
  /// @param tenant_id Tenant identifier.
  /// @param daily_budget Daily budget in credits or ms.
  void SetTenantBudget(const std::string& tenant_id, float daily_budget);

  /// @brief Reset tenant daily budget (typically called at 00:00 UTC).
  ///
  /// @param tenant_id Tenant identifier.
  void ResetTenantBudget(const std::string& tenant_id);

  /// @brief Set ROI threshold.
  ///
  /// @param threshold New threshold (higher = more selective).
  void SetROIThreshold(float threshold);

 private:
  float roi_threshold_;
  float max_cost_per_query_;
  
  // Budget tracking (tenant_id → current consumption)
  std::map<std::string, float> tenant_budgets_;
  std::map<std::string, float> tenant_daily_limits_;

  // Query analysis helpers
  float ComputeQueryComplexityFactor(const std::string& query_text);
  float ComputeResultUncertainty(const std::vector<Document>& results);
  float EstimateModelLatency(uint32_t context_tokens);
};

}  // namespace themis::rag
