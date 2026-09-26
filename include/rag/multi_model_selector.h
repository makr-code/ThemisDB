/**
 * @file multi_model_selector.h
 * @brief A/B testing and intelligent model selection for RAG Phase 12
 *
 * Tracks model performance metrics and selects best model using
 * statistical confidence and cost-quality Pareto optimization.
 *
 * @version 0.1.0
 * @note Phase: 12 (Advanced Cost Optimization)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::optimization {

/**
 * @brief Model performance statistics
 */
struct ModelStats {
  uint32_t model_version = 0;
  std::string model_id;
  
  uint64_t total_queries = 0;         ///< Number of queries routed to this model
  double total_cost = 0.0;            ///< Total cost incurred (tokens or USD)
  double mean_latency_ms = 0.0;       ///< Average latency
  double mean_quality_score = 0.0;    ///< NDCG, recall, or combined metric
  
  double p95_latency_ms = 0.0;        ///< 95th percentile latency
  double p99_latency_ms = 0.0;        ///< 99th percentile latency
  
  bool is_canary = false;             ///< Is this model in canary phase?
  double canary_traffic_pct = 0.0;    ///< Traffic allocated if canary
  
  double confidence = 0.0;            ///< Confidence in these statistics (0-1)
};

/**
 * @brief Pareto frontier point (cost-quality tradeoff)
 */
struct ParetoPoint {
  uint32_t model_version = 0;
  double cost = 0.0;
  double quality = 0.0;
  bool is_dominated = false;  ///< true if better alternative exists
};

/**
 * @brief Multi-Model Selector — A/B testing and model optimization
 *
 * Continuously tracks performance of multiple models (e.g., different LLMs,
 * retrievers, re-rankers) and selects the best based on:
 * - Statistical significance (p-value < 0.05)
 * - Pareto optimality (cost vs quality)
 * - Confidence intervals (wide intervals = need more data)
 *
 * Thread-safe for concurrent metric updates.
 */
class MultiModelSelector {
 public:
  /**
   * @brief Constructor
   *
   * @param num_models Expected number of models to track
   * @param min_samples_for_decision Minimum queries per model before declaring winner (default 100)
   */
  explicit MultiModelSelector(uint32_t num_models = 5, uint32_t min_samples_for_decision = 100);
  ~MultiModelSelector();

  /**
   * @brief Register a model for tracking
   *
   * @param model_version Version ID from Phase 11 ModelRegistry
   * @param model_id Human-readable name
   * @param is_baseline true if this is the currently deployed baseline
   * @return Model index for metric reporting
   */
  uint32_t RegisterModel(uint32_t model_version, const std::string& model_id,
                        bool is_baseline = false);

  /**
   * @brief Report query execution metrics
   *
   * Called after query completes to update model statistics.
   *
   * @param model_version Model that handled query
   * @param latency_ms Execution latency
   * @param quality_score Quality metric (NDCG, recall, or combined 0-1)
   * @param cost_tokens Tokens consumed (for cost tracking)
   */
  void ReportQueryMetrics(uint32_t model_version, double latency_ms, double quality_score,
                         uint32_t cost_tokens = 0);

  /**
   * @brief Get statistics for a model
   *
   * @param model_version Model to query
   * @return Current statistics, or empty stats if version not found
   */
  ModelStats GetModelStats(uint32_t model_version) const;

  /**
   * @brief Select best model based on current data
   *
   * Returns model_version with highest composite score.
   * Composite = (1 - cost_weight) * quality + cost_weight * (-cost)
   *
   * @param cost_weight Balance between quality (0.0) and cost (1.0), default 0.3
   * @return Best model version, or 0 if insufficient data
   */
  uint32_t SelectBestModel(double cost_weight = 0.3) const;

  /**
   * @brief Check if model is statistically significant winner
   *
   * Uses t-test to check if best model is significantly better than baseline.
   * Returns true only if p-value < 0.05 AND minimum samples collected.
   *
   * @param confidence_threshold Minimum confidence needed (default 0.95)
   * @return true if statistical winner found, false if more data needed
   */
  bool IsStatisticallySignificantWinner(double confidence_threshold = 0.95) const;

  /**
   * @brief Compute Pareto frontier
   *
   * Returns all models that are not dominated on cost-quality tradeoff.
   * A model is dominated if another model has both lower cost and higher quality.
   *
   * @return List of Pareto-optimal models
   */
  std::vector<ParetoPoint> ComputeParetoFrontier() const;

  /**
   * @brief Get suggested fallback chain
   *
   * Returns list of models ordered by reliability for graceful degradation.
   * Primary: current best model
   * Secondary: previous best model
   * Tertiary: baseline (most stable)
   *
   * @return Model versions in fallback order
   */
  std::vector<uint32_t> GetFallbackChain() const;

  /**
   * @brief Configure confidence interval parameters
   *
   * @param confidence_level (e.g., 0.95 for 95% CI)
   */
  void SetConfidenceLevel(double confidence_level);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::optimization
