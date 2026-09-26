/**
 * @file deployment_gate_controller.cpp
 * @brief Quality-based deployment gate implementation
 */

#include "rag/deployment_gate_controller.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <sstream>

namespace themis::rag::quality {

struct DeploymentGateController::Impl {
  std::mutex gate_mutex;
  double hard_regression_threshold = 5.0;
  double soft_regression_threshold = 2.0;
  
  // Per-metric thresholds
  std::map<std::string, std::pair<double, double>> metric_thresholds;  // hard, soft
  std::map<std::string, bool> metric_enabled;
  
  // Default metrics to gate on
  std::vector<std::string> default_metrics = {
      "recall_10", "ndcg_10", "mrr", "faithfulness", "relevance"
  };
  
  Impl() {
    // Initialize default metric settings
    for (const auto& metric : default_metrics) {
      metric_thresholds[metric] = {hard_regression_threshold, soft_regression_threshold};
      metric_enabled[metric] = true;
    }
  }
};

DeploymentGateController::DeploymentGateController(double hard_threshold, double soft_threshold)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->hard_regression_threshold = hard_threshold;
  pimpl_->soft_regression_threshold = soft_threshold;
  
  // Re-initialize with provided thresholds
  for (auto& pair : pimpl_->metric_thresholds) {
    pair.second = {hard_threshold, soft_threshold};
  }
}

DeploymentGateController::~DeploymentGateController() = default;

GateDecision DeploymentGateController::EvaluateCandidate(
    uint32_t candidate_model, uint32_t baseline_model, const AggregatedMetrics& candidate_metrics,
    const AggregatedMetrics& baseline_metrics) {
  std::lock_guard<std::mutex> lock(pimpl_->gate_mutex);
  
  GateDecision decision;
  decision.candidate_model_version = candidate_model;
  decision.baseline_model_version = baseline_model;
  decision.decision = GateDecision::kAllow;
  
  double worst_regression = 0.0;
  
  // Check recall@10
  if (pimpl_->metric_enabled["recall_10"]) {
    double change_pct =
        ((candidate_metrics.mean_recall_10 - baseline_metrics.mean_recall_10) /
         baseline_metrics.mean_recall_10) *
        100.0;
    if (change_pct < 0.0) {
      double abs_change = std::abs(change_pct);
      worst_regression = std::max(worst_regression, abs_change);
      
      if (abs_change > pimpl_->metric_thresholds["recall_10"].first) {
        decision.failed_checks.push_back("recall@10: " + std::to_string(abs_change) + "% regression");
        decision.decision = GateDecision::kDeny;
      } else if (abs_change > pimpl_->metric_thresholds["recall_10"].second) {
        decision.failed_checks.push_back("recall@10: " + std::to_string(abs_change) + "% regression (warning)");
        if (decision.decision == GateDecision::kAllow) {
          decision.decision = GateDecision::kWarn;
        }
      }
    }
  }
  
  // Check NDCG@10
  if (pimpl_->metric_enabled["ndcg_10"]) {
    double change_pct = ((candidate_metrics.mean_ndcg_10 - baseline_metrics.mean_ndcg_10) /
                         baseline_metrics.mean_ndcg_10) *
                        100.0;
    if (change_pct < 0.0) {
      double abs_change = std::abs(change_pct);
      worst_regression = std::max(worst_regression, abs_change);
      
      if (abs_change > pimpl_->metric_thresholds["ndcg_10"].first) {
        decision.failed_checks.push_back("ndcg@10: " + std::to_string(abs_change) + "% regression");
        decision.decision = GateDecision::kDeny;
      } else if (abs_change > pimpl_->metric_thresholds["ndcg_10"].second) {
        decision.failed_checks.push_back("ndcg@10: " + std::to_string(abs_change) + "% regression (warning)");
        if (decision.decision == GateDecision::kAllow) {
          decision.decision = GateDecision::kWarn;
        }
      }
    }
  }
  
  // Check MRR
  if (pimpl_->metric_enabled["mrr"]) {
    double change_pct =
        ((candidate_metrics.mean_mrr - baseline_metrics.mean_mrr) / baseline_metrics.mean_mrr) *
        100.0;
    if (change_pct < 0.0) {
      double abs_change = std::abs(change_pct);
      worst_regression = std::max(worst_regression, abs_change);
      
      if (abs_change > pimpl_->metric_thresholds["mrr"].first) {
        decision.failed_checks.push_back("MRR: " + std::to_string(abs_change) + "% regression");
        decision.decision = GateDecision::kDeny;
      } else if (abs_change > pimpl_->metric_thresholds["mrr"].second) {
        decision.failed_checks.push_back("MRR: " + std::to_string(abs_change) + "% regression (warning)");
        if (decision.decision == GateDecision::kAllow) {
          decision.decision = GateDecision::kWarn;
        }
      }
    }
  }
  
  decision.regression_pct = worst_regression;
  
  // Build reason message
  if (decision.decision == GateDecision::kAllow) {
    decision.reason = "Model " + std::to_string(candidate_model) + " meets quality gates";
  } else if (decision.decision == GateDecision::kWarn) {
    decision.reason = "Model " + std::to_string(candidate_model) +
                     " has minor quality regressions (soft threshold exceeded)";
  } else {
    decision.reason = "Model " + std::to_string(candidate_model) +
                     " has significant quality regressions (hard threshold exceeded)";
    decision.estimated_recovery_hours = 4.0;  // Placeholder estimate
  }
  
  return decision;
}

void DeploymentGateController::SetMetricThreshold(const std::string& metric_name,
                                                 double hard_threshold,
                                                 double soft_threshold) {
  std::lock_guard<std::mutex> lock(pimpl_->gate_mutex);
  pimpl_->metric_thresholds[metric_name] = {hard_threshold, soft_threshold};
}

void DeploymentGateController::SetMetricEnabled(const std::string& metric_name, bool enabled) {
  std::lock_guard<std::mutex> lock(pimpl_->gate_mutex);
  pimpl_->metric_enabled[metric_name] = enabled;
}

DeploymentGateController::GateConfig DeploymentGateController::GetGateConfig() const {
  std::lock_guard<std::mutex> lock(pimpl_->gate_mutex);
  
  GateConfig config;
  config.hard_regression_threshold = pimpl_->hard_regression_threshold;
  config.soft_regression_threshold = pimpl_->soft_regression_threshold;
  
  for (const auto& pair : pimpl_->metric_enabled) {
    if (pair.second) {
      config.enabled_metrics.push_back(pair.first);
      if (pimpl_->metric_thresholds.count(pair.first)) {
        auto thresholds = pimpl_->metric_thresholds.at(pair.first);
        config.metric_thresholds.push_back(
            {pair.first, {thresholds.first, thresholds.second}});
      }
    }
  }
  
  return config;
}

GateDecision DeploymentGateController::SimulateDecision(
    uint32_t candidate_model, uint32_t baseline_model, const AggregatedMetrics& candidate_metrics,
    const AggregatedMetrics& baseline_metrics) {
  // Same as EvaluateCandidate but doesn't record anything
  return EvaluateCandidate(candidate_model, baseline_model, candidate_metrics, baseline_metrics);
}

}  // namespace themis::rag::quality
