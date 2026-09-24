/**
 * @file model_promoter.h
 * @brief Canary deployment and traffic-shifting for RAG Phase 11
 *
 * Manages progressive deployment of new models with traffic shifting,
 * quality monitoring, and automatic rollback on regression.
 *
 * @version 0.1.0
 * @note Phase: 11 (Retraining Automation & Orchestration)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::lifecycle {

// Forward declarations
class ModelRegistry;
class ModelEvaluator;

/**
 * @brief Canary deployment phases
 */
enum class CanaryPhase {
  kNone,      ///< No canary deployment active
  kShadow,    ///< Shadow mode: run in parallel, 0% traffic
  kCanary5,   ///< 5% of traffic
  kCanary10,  ///< 10% of traffic
  kCanary25,  ///< 25% of traffic
  kCanary50,  ///< 50% of traffic
  kDeployed   ///< 100% of traffic (fully deployed)
};

/**
 * @brief Convert CanaryPhase to string
 */
std::string PhaseToString(CanaryPhase phase);

/**
 * @brief Traffic split instruction for request routing
 *
 * Returned by ModelPromoter to tell request handlers which model to use.
 * Includes canary model version and traffic percentage.
 */
struct TrafficSplitDecision {
  uint32_t baseline_version = 0; ///< Current deployed model version
  uint32_t canary_version = 0;   ///< Candidate model version
  CanaryPhase phase = CanaryPhase::kNone;
  double canary_traffic_pct = 0.0; ///< Percentage of traffic for canary (e.g., 5.0 for 5%)
};

/**
 * @brief Callback fired when quality regression detected during canary
 *
 * Parameters: (metric_name, baseline_value, current_value, threshold)
 */
using QualityRegressionCallback =
    std::function<void(const std::string& metric, double baseline, double current,
                      double threshold)>;

/**
 * @brief Model Promoter — Canary deployment and progressive traffic shifting
 *
 * Manages safe deployment of new models with:
 * - Shadow mode: run new model in parallel (0% traffic) for validation
 * - Canary phases: progressively increase traffic (5%→10%→25%→50%→100%)
 * - Quality monitoring: track metrics during each phase
 * - Automatic rollback: revert to baseline if regression detected
 *
 * Thread-safe for concurrent traffic routing decisions and phase transitions.
 */
class ModelPromoter {
 public:
  /**
   * @brief Constructor
   *
   * @param model_registry Reference to model lifecycle manager
   * @param model_evaluator Reference to evaluation service
   * @param quality_regression_threshold_pct Rollback if metric degrades by % (default 5%)
   * @param canary_phase_duration_hours Duration to run each phase (1-24 recommended)
   */
  ModelPromoter(ModelRegistry& model_registry, ModelEvaluator& model_evaluator,
                double quality_regression_threshold_pct = 0.05,
                uint32_t canary_phase_duration_hours = 1);
  ~ModelPromoter();

  /**
   * @brief Start canary deployment for a candidate model
   *
   * Begins shadow mode (0% traffic) and schedules progression through
   * canary phases based on configured duration and quality metrics.
   *
   * @param candidate_version Model version to deploy
   * @return true if canary started, false if already running or invalid version
   */
  bool StartCanary(uint32_t candidate_version);

  /**
   * @brief Get current traffic split decision
   *
   * Returns routing instruction for current request. Tells caller which model
   * to use (baseline or canary) and what percentage of traffic should use canary.
   *
   * @return TrafficSplitDecision with current phase and traffic split
   */
  TrafficSplitDecision GetTrafficSplit() const;

  /**
   * @brief Advance to next canary phase
   *
   * Manually triggers progression to next phase (e.g., shadow→5%→10%).
   * Called automatically on schedule, but can be forced for manual control.
   *
   * @return true if advanced to next phase, false if already fully deployed
   */
  bool AdvancePhase();

  /**
   * @brief Rollback to baseline model
   *
   * Immediately stops canary and reverts to previously deployed model.
   * Triggered automatically on quality regression if enabled.
   *
   * @param reason Human-readable reason for rollback
   * @return true if rollback succeeded, false if no canary in progress
   */
  bool Rollback(const std::string& reason = "");

  /**
   * @brief Report quality metric observation during canary
   *
   * Called by observability stack to report metrics observed during canary.
   * Triggers automatic rollback if metric falls below threshold.
   *
   * @param metric_name Metric identifier (e.g., "ndcg@10")
   * @param current_value Current observed value
   * @param baseline_value Baseline value from deployed model
   * @return true if metric passes threshold, false if below threshold (may trigger rollback)
   */
  bool ReportMetric(const std::string& metric_name, double current_value,
                    double baseline_value);

  /**
   * @brief Register callback for quality regression events
   *
   * @param callback Function to invoke when regression detected
   */
  void SetQualityRegressionCallback(const QualityRegressionCallback& callback);

  /**
   * @brief Get current canary phase
   */
  CanaryPhase GetCurrentPhase() const;

  /**
   * @brief Check if canary deployment is active
   */
  bool IsCanaryActive() const;

  /**
   * @brief Get canary deployment start time (microseconds since epoch)
   *
   * @return 0 if no canary active
   */
  uint64_t GetCanaryStartTimeUs() const;

  /**
   * @brief Finalize canary and promote to deployed
   *
   * Called after successful progression through all canary phases.
   * Updates model registry to mark new model as deployed and archives old one.
   *
   * @return true if promotion succeeded, false if validation failed
   */
  bool FinalizeDeployment();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::lifecycle
