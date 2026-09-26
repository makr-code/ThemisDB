/**
 * @file deployment_gate_controller.h
 * @brief Quality-based deployment gate enforcement for RAG Phase 13
 *
 * Blocks/warns on quality regression with detailed rejection rationale.
 *
 * @version 0.1.0
 * @note Phase: 13 (Quality Gate Operationalization)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::quality {

/**
 * @brief Gate decision result
 */
struct GateDecision {
  enum Decision { kAllow, kWarn, kDeny };
  
  Decision decision = kAllow;
  std::string reason;                  ///< Human-readable rejection reason
  double regression_pct = 0.0;         ///< Max regression found (%)
  std::vector<std::string> failed_checks;  ///< Which metrics failed thresholds
  
  // Additional context
  uint32_t candidate_model_version = 0;
  uint32_t baseline_model_version = 0;
  double estimated_recovery_hours = 0.0;  ///< Time to recover regression
};

/**
 * @brief Deployment Gate Controller — Quality-gated promotion
 *
 * Evaluates whether a candidate model meets quality requirements before
 * deployment. Checks for regressions in recall, NDCG, MRR, faithfulness.
 * Integrates with Phase 11 ModelPromoter to enforce gates.
 *
 * Thread-safe for concurrent gate checks.
 */
class DeploymentGateController {
 public:
  /**
   * @brief Constructor
   *
   * @param hard_regression_threshold Hard gate: deny if regression > this % (default 5%)
   * @param soft_regression_threshold Soft gate: warn if regression > this % (default 2%)
   */
  DeploymentGateController(double hard_regression_threshold = 5.0,
                          double soft_regression_threshold = 2.0);
  ~DeploymentGateController();

  /**
   * @brief Evaluate whether candidate model meets quality gates
   *
   * Checks candidate against baseline across all quality metrics.
   * Returns allow/warn/deny decision with detailed rationale.
   *
   * @param candidate_model Model version attempting deployment
   * @param baseline_model Currently deployed model (comparison target)
   * @param candidate_metrics Metrics from candidate model evaluation
   * @param baseline_metrics Metrics from baseline model
   * @return Gate decision with reasoning
   */
  GateDecision EvaluateCandidate(uint32_t candidate_model, uint32_t baseline_model,
                                const struct AggregatedMetrics& candidate_metrics,
                                const struct AggregatedMetrics& baseline_metrics);

  /**
   * @brief Set per-metric thresholds
   *
   * Allow customization of which metrics trigger gates and at what thresholds.
   *
   * @param metric_name (e.g., "recall_10", "ndcg_10", "mrr")
   * @param hard_threshold Regression % that triggers kDeny
   * @param soft_threshold Regression % that triggers kWarn
   */
  void SetMetricThreshold(const std::string& metric_name, double hard_threshold,
                         double soft_threshold);

  /**
   * @brief Check if metric is enabled for gating
   *
   * Some metrics may be monitored but not gate deployment.
   *
   * @param metric_name
   * @param enabled true to gate on this metric, false to monitor only
   */
  void SetMetricEnabled(const std::string& metric_name, bool enabled);

  /**
   * @brief Get current gate configuration
   *
   * @return Summary of all thresholds and enabled metrics
   */
  struct GateConfig {
    double hard_regression_threshold = 0.0;
    double soft_regression_threshold = 0.0;
    std::vector<std::string> enabled_metrics;
    std::vector<std::pair<std::string, std::pair<double, double>>> metric_thresholds;
  };

  /**
   * @brief Get the current gate configuration.
   * @return GateConfig struct with thresholds, enabled metrics, and metric ranges.
   */
  GateConfig GetGateConfig() const;

  /**
   * @brief Simulate gate decision
   *
   * Dry-run evaluation without recording. Useful for dashboards showing
   * "would this model be allowed?" analysis.
   *
   * @param candidate_model
   * @param baseline_model
   * @param candidate_metrics
   * @param baseline_metrics
   * @return What would be decided (for analysis only)
   */
  GateDecision SimulateDecision(uint32_t candidate_model, uint32_t baseline_model,
                               const struct AggregatedMetrics& candidate_metrics,
                               const struct AggregatedMetrics& baseline_metrics);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::quality
