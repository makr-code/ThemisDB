/**
 * @file model_evaluator.cpp
 * @brief Model Evaluator implementation
 *
 * Statistical validation of candidate models against baseline using
 * quality metrics, cost metrics, and significance testing.
 */

#include "rag/model_evaluator.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>

#include "rag/model_registry.h"

namespace themis::rag::lifecycle {

using json = nlohmann::json;

struct ModelEvaluator::Impl {
  ModelRegistry* model_registry = nullptr;
  double min_improvement_threshold = 0.02;
  bool enable_statistical_tests = true;
  double cost_weight = 0.3;
  mutable std::mutex mu;

  uint64_t GetCurrentTimestampUs() const {
    using namespace std::chrono;
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
  }

  // Simple t-test p-value approximation for small samples
  double ComputePValue(double baseline, double candidate, int sample_count = 30) {
    if (sample_count < 2) return 1.0;
    
    // Simplified t-test for single comparison
    double diff = std::abs(candidate - baseline);
    double t_stat = diff / (1.0 + 1.0 / sample_count);
    
    // Very rough p-value approximation (normally would use statistical library)
    // For t > 2.0, p ≈ 0.05; for t > 1.0, p ≈ 0.2; for t < 1.0, p ≈ 0.5+
    if (t_stat > 2.0) return 0.05;
    if (t_stat > 1.0) return 0.2;
    return 0.5;
  }
};

ModelEvaluator::ModelEvaluator(ModelRegistry& model_registry,
                               double min_improvement_threshold,
                               bool enable_statistical_tests, double cost_weight)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->model_registry = &model_registry;
  pimpl_->min_improvement_threshold = min_improvement_threshold;
  pimpl_->enable_statistical_tests = enable_statistical_tests;
  pimpl_->cost_weight = cost_weight;
}

ModelEvaluator::~ModelEvaluator() = default;

EvaluationDecision ModelEvaluator::Evaluate(uint32_t candidate_version,
                                           const std::string& baseline_metrics_json,
                                           const std::string& baseline_cost_json) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);

  EvaluationDecision decision;
  decision.evaluation_completed_at_us = pimpl_->GetCurrentTimestampUs();

  auto candidate_meta = pimpl_->model_registry->GetByVersion(candidate_version);
  if (!candidate_meta) {
    decision.approved = false;
    decision.decision_reason = "Candidate model version not found";
    return decision;
  }

  try {
    auto baseline_metrics = json::parse(baseline_metrics_json);
    auto baseline_cost = json::parse(baseline_cost_json);
    auto candidate_metrics = json::parse(candidate_meta->metrics_json);
    auto candidate_cost = json::parse(candidate_meta->cost_stats_json);

    // Compare quality metrics
    double quality_improvement_sum = 0.0;
    int quality_metric_count = 0;

    for (auto& [key, baseline_value] : baseline_metrics.items()) {
      if (!baseline_value.is_number()) continue;

      double bv = baseline_value.get<double>();
      if (candidate_metrics.contains(key) && candidate_metrics[key].is_number()) {
        double cv = candidate_metrics[key].get<double>();
        double improvement = (cv - bv) / std::max(std::abs(bv), 1e-6);

        MetricEvaluationResult result;
        result.metric_name = key;
        result.baseline_value = bv;
        result.candidate_value = cv;
        result.improvement_pct = improvement * 100.0;

        if (pimpl_->enable_statistical_tests) {
          result.p_value = pimpl_->ComputePValue(bv, cv);
          result.statistically_significant = result.p_value < 0.05;
          result.statistical_test = "t-test";
        }

        result.passes_threshold =
            improvement >= pimpl_->min_improvement_threshold;

        decision.metric_results.push_back(result);
        quality_improvement_sum += improvement;
        quality_metric_count++;
      }
    }

    // Compare cost metrics (latency, price per query, etc.)
    double cost_improvement_sum = 0.0;
    int cost_metric_count = 0;

    for (auto& [key, baseline_value] : baseline_cost.items()) {
      if (!baseline_value.is_number()) continue;

      double bv = baseline_value.get<double>();
      // For cost metrics, negative improvement (lower values) is better
      // So we negate to make it consistent with quality metrics
      double improvement = -(baseline_value.get<double>() -
                           candidate_cost[key].get<double>()) / std::max(bv, 1e-6);

      MetricEvaluationResult result;
      result.metric_name = key + "_cost";
      result.baseline_value = bv;
      result.candidate_value = candidate_cost[key].get<double>();
      result.improvement_pct = improvement * 100.0;

      if (pimpl_->enable_statistical_tests) {
        result.p_value = pimpl_->ComputePValue(bv, candidate_cost[key].get<double>());
        result.statistically_significant = result.p_value < 0.05;
        result.statistical_test = "t-test";
      }

      result.passes_threshold = improvement >= (pimpl_->min_improvement_threshold * 0.5);

      decision.metric_results.push_back(result);
      cost_improvement_sum += improvement;
      cost_metric_count++;
    }

    // Compute overall score (weighted combination of quality and cost)
    double quality_avg = quality_metric_count > 0 ? quality_improvement_sum / quality_metric_count : 0.0;
    double cost_avg = cost_metric_count > 0 ? cost_improvement_sum / cost_metric_count : 0.0;
    decision.overall_improvement_score =
        (1.0 - pimpl_->cost_weight) * quality_avg + pimpl_->cost_weight * cost_avg;

    // Decision logic:
    // - Approve if overall score >= threshold OR
    // - Any quality metric significantly improved (statistically significant at p<0.05)
    bool has_significant_improvement = false;
    for (const auto& result : decision.metric_results) {
      if (!result.metric_name.find("cost") &&  // not a cost metric
          result.statistically_significant && result.passes_threshold) {
        has_significant_improvement = true;
        break;
      }
    }

    decision.approved =
        decision.overall_improvement_score >= pimpl_->min_improvement_threshold ||
        has_significant_improvement;

    if (decision.approved) {
      decision.decision_reason =
          "Model passes evaluation: overall_score=" +
          std::to_string(decision.overall_improvement_score);
    } else {
      decision.decision_reason =
          "Model fails evaluation: overall_score=" +
          std::to_string(decision.overall_improvement_score) + " < " +
          std::to_string(pimpl_->min_improvement_threshold);
    }

  } catch (const std::exception& e) {
    decision.approved = false;
    decision.decision_reason = std::string("Evaluation failed: ") + e.what();
  }

  return decision;
}

EvaluationDecision ModelEvaluator::EvaluateVsDeployed(uint32_t candidate_version) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);

  EvaluationDecision decision;
  decision.evaluation_completed_at_us = pimpl_->GetCurrentTimestampUs();

  auto deployed = pimpl_->model_registry->GetDeployedModel();
  if (!deployed) {
    decision.approved = false;
    decision.decision_reason = "No deployed baseline model found";
    return decision;
  }

  lock.unlock();
  decision =
      Evaluate(candidate_version, deployed->metrics_json, deployed->cost_stats_json);
  lock.lock();

  return decision;
}

void ModelEvaluator::SetMinImprovementThreshold(double threshold) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  pimpl_->min_improvement_threshold = threshold;
}

double ModelEvaluator::GetMinImprovementThreshold() const {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  return pimpl_->min_improvement_threshold;
}

void ModelEvaluator::SetCostWeight(double weight) {
  std::lock_guard<std::mutex> lock(pimpl_->mu);
  pimpl_->cost_weight = std::max(0.0, std::min(1.0, weight));
}

}  // namespace themis::rag::lifecycle
