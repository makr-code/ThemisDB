/**
 * @file model_evaluator.h
 * @brief Pre-deployment model evaluation and validation for RAG Phase 11
 *
 * Validates trained models against baselines using statistical tests
 * and quality metrics. Integrates with Phase 9 (MetricComputation) and
 * Phase 10 (CostModelBuilder).
 *
 * @version 0.1.0
 * @note Phase: 11 (Retraining Automation & Orchestration)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace themis::rag::lifecycle {

// Forward declarations
class ModelRegistry;

/**
 * @brief Evaluation result for a single metric
 */
struct MetricEvaluationResult {
  std::string metric_name;      ///< Name (e.g., "ndcg@10", "recall@10", "latency_ms")
  double baseline_value = 0.0;  ///< Baseline metric value
  double candidate_value = 0.0; ///< New model metric value
  double improvement_pct = 0.0; ///< Percentage improvement ((candidate - baseline) / baseline * 100)
  bool passes_threshold = false; ///< true if improvement >= min_improvement_threshold
  std::string statistical_test; ///< Test applied (e.g., "t-test", "ks-test", "none")
  double p_value = 1.0;         ///< p-value from statistical test (0.0-1.0), lower is more significant
  bool statistically_significant = false; ///< true if p_value < 0.05
};

/**
 * @brief Overall evaluation decision
 */
struct EvaluationDecision {
  bool approved = false;                ///< Should model be promoted to candidate status?
  std::string decision_reason;          ///< Human-readable explanation
  std::vector<MetricEvaluationResult> metric_results; ///< Per-metric details
  double overall_improvement_score = 0.0; ///< Weighted score of all improvements
  uint64_t evaluation_completed_at_us = 0; ///< Completion timestamp
};

/**
 * @brief Model Evaluator — Pre-deployment validation
 *
 * Compares a candidate model against the currently deployed baseline model
 * using statistical tests and configurable thresholds. Integrates with
 * Phase 9 metrics and Phase 10 cost modeling.
 *
 * Thread-safe for concurrent evaluations.
 */
class ModelEvaluator {
 public:
  /**
   * @brief Constructor
   *
   * @param model_registry Reference to model lifecycle manager
   * @param min_improvement_threshold Minimum improvement % required (e.g., 0.02 for 2%)
   * @param enable_statistical_tests true to run t-test/KS-test on metrics
   * @param cost_weight Weight of cost metrics vs quality metrics (0.0-1.0)
   */
  ModelEvaluator(ModelRegistry& model_registry, double min_improvement_threshold = 0.02,
                 bool enable_statistical_tests = true, double cost_weight = 0.3);
  ~ModelEvaluator();

  /**
   * @brief Evaluate a candidate model against baseline
   *
   * Compares the candidate model (identified by version) against the
   * currently deployed baseline model using metrics from Phase 9 and
   * cost stats from Phase 10.
   *
   * @param candidate_version Version of model to evaluate
   * @param baseline_metrics_json Quality metrics JSON for baseline (e.g., {"ndcg@10": 0.75, ...})
   * @param baseline_cost_json Cost metrics JSON for baseline (e.g., {"latency_ms": 42.5, ...})
   * @return Evaluation decision with per-metric results
   */
  EvaluationDecision Evaluate(uint32_t candidate_version,
                             const std::string& baseline_metrics_json,
                             const std::string& baseline_cost_json);

  /**
   * @brief Evaluate using currently deployed model as baseline
   *
   * Convenience method that retrieves deployed model from registry
   * and uses its metrics as baseline for comparison.
   *
   * @param candidate_version Version of model to evaluate
   * @return Evaluation decision, or decision.approved=false if no deployed baseline exists
   */
  EvaluationDecision EvaluateVsDeployed(uint32_t candidate_version);

  /**
   * @brief Update minimum improvement threshold
   *
   * Changes the threshold for what counts as a successful improvement.
   * Applied to new evaluations only; existing evaluations unaffected.
   *
   * @param threshold New minimum improvement as fraction (e.g., 0.05 for 5%)
   */
  void SetMinImprovementThreshold(double threshold);

  /**
   * @brief Get minimum improvement threshold
   */
  double GetMinImprovementThreshold() const;

  /**
   * @brief Set cost weight in overall score calculation
   *
   * Controls balance between cost optimization and quality optimization.
   * Higher weight (closer to 1.0) prioritizes cost improvements.
   *
   * @param weight Cost weight 0.0-1.0 (default 0.3, quality-focused)
   */
  void SetCostWeight(double weight);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::lifecycle
