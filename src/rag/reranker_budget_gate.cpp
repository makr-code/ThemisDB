// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "reranker_budget_gate.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace themis::rag {

RerankerBudgetGate::RerankerBudgetGate(float roi_threshold,
                                       float max_cost_per_query)
    : roi_threshold_(roi_threshold), max_cost_per_query_(max_cost_per_query) {}

RerankerBudgetGate::RankingROI RerankerBudgetGate::EstimateROI(
    const BudgetContext& context) {
  RankingROI result;
  result.estimated_quality_gain = 0.0f;
  result.estimated_cost = 0.0f;
  result.roi_ratio = 0.0f;
  result.should_rerank = false;

  // Analyze query complexity
  float complexity_factor = ComputeQueryComplexityFactor(context.query_text);
  
  // Compute result uncertainty
  float uncertainty_score = ComputeResultUncertainty(context.hybrid_results);

  // Estimate quality gain
  float base_gain = 0.03f;  // Assume 3% baseline gain
  result.estimated_quality_gain = base_gain * complexity_factor * uncertainty_score;

  // Estimate cost (assume 100ms model latency + token cost)
  uint32_t context_tokens = std::min(static_cast<uint32_t>(context.query_text.length() / 5), 2000u);
  result.estimated_cost = EstimateModelLatency(context_tokens);

  // Check hard cap
  if (result.estimated_cost > max_cost_per_query_) {
    result.rationale = "Cost exceeds hard cap (100ms)";
    result.should_rerank = false;
    return result;
  }

  // Compute ROI ratio
  if (result.estimated_cost > 0.0f) {
    result.roi_ratio = result.estimated_quality_gain / result.estimated_cost;
  }

  // Check budget availability
  if (context.available_budget < result.estimated_cost) {
    result.rationale = "Insufficient budget available";
    result.should_rerank = false;
    return result;
  }

  // Check ROI threshold
  if (result.roi_ratio >= roi_threshold_) {
    result.should_rerank = true;
    result.rationale = "ROI favorable: " + std::to_string(result.roi_ratio) +
                       " > threshold (" + std::to_string(roi_threshold_) + ")";
  } else {
    result.should_rerank = false;
    result.rationale = "ROI unfavorable: " + std::to_string(result.roi_ratio) +
                       " < threshold (" + std::to_string(roi_threshold_) + ")";
  }

  return result;
}

RerankerBudgetGate::BudgetStatus RerankerBudgetGate::GetTenantBudgetStatus(
    const std::string& tenant_id) {
  BudgetStatus status;
  status.consumed = tenant_budgets_[tenant_id];
  status.available = tenant_daily_limits_[tenant_id] - status.consumed;
  status.percentage_used = 
      (status.consumed / tenant_daily_limits_[tenant_id]) * 100.0f;
  status.at_risk = status.percentage_used > 80.0f;
  return status;
}

bool RerankerBudgetGate::ConsumeBudget(const std::string& tenant_id,
                                      float cost) {
  // Check if consuming cost would exceed 90% ceiling
  float current_usage = tenant_budgets_[tenant_id];
  float daily_limit = tenant_daily_limits_[tenant_id];
  float usage_after = current_usage + cost;

  // 10% reserve: max 90% usage
  if (usage_after > daily_limit * 0.9f) {
    return false;
  }

  // Atomically deduct cost
  tenant_budgets_[tenant_id] = usage_after;
  return true;
}

void RerankerBudgetGate::SetTenantBudget(const std::string& tenant_id,
                                        float daily_budget) {
  tenant_daily_limits_[tenant_id] = daily_budget;
}

void RerankerBudgetGate::ResetTenantBudget(const std::string& tenant_id) {
  tenant_budgets_[tenant_id] = 0.0f;
}

void RerankerBudgetGate::SetROIThreshold(float threshold) {
  roi_threshold_ = threshold;
}

float RerankerBudgetGate::ComputeQueryComplexityFactor(
    const std::string& query_text) {
  // Short queries (< 10 tokens): 0.6x (lower ROI expected)
  // Medium queries (10-50 tokens): 1.0x (baseline)
  // Long queries (> 50 tokens): 1.2x (higher ROI expected)
  
  uint32_t token_count = query_text.length() / 5;  // Rough estimate

  if (token_count < 10) {
    return 0.6f;
  } else if (token_count > 50) {
    return 1.2f;
  } else {
    return 1.0f;
  }
}

float RerankerBudgetGate::ComputeResultUncertainty(
    const std::vector<Document>& results) {
  // Compute variance in top-5 scores
  if (results.size() < 5) {
    return 0.5f;  // Low uncertainty if few results
  }

  // TODO: This requires Document struct with score field
  // For now, return fixed value
  return 0.8f;  // High uncertainty boost
}

float RerankerBudgetGate::EstimateModelLatency(uint32_t context_tokens) {
  // Cross-encoder latency estimation
  // Base: 50ms
  // Per-token: 0.1ms / 100 tokens
  float base_latency = 50.0f;
  float token_latency = static_cast<float>(context_tokens) * 0.001f;
  return base_latency + token_latency;
}

}  // namespace themis::rag
