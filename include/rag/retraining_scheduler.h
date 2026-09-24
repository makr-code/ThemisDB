/**
 * @file retraining_scheduler.h
 * @brief Autonomous retraining triggering for RAG Phase 11
 *
 * Coordinates when retraining should occur based on multiple signals:
 * time-based schedules, cost model drift, and production quality metrics.
 *
 * @version 0.1.0
 * @note Phase: 11 (Retraining Automation & Orchestration)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace themis::rag::lifecycle {

// Forward declaration
class ModelRegistry;

/**
 * @brief Reasons why retraining should be triggered
 */
enum class RetariningTrigger {
  kScheduledTime,     ///< Time-based schedule (hourly/daily/weekly)
  kDriftDetected,     ///< Cost model drift exceeded threshold
  kQualityRegression, ///< Production quality metrics fell below baseline
  kManualRequest,     ///< Explicit human request
  kNone               ///< No trigger active
};

/**
 * @brief Convert RetariningTrigger to string
 */
std::string TriggerToString(RetariningTrigger trigger);

/**
 * @brief Callback fired when retraining should be triggered
 *
 * Called with trigger reason and recommended action.
 * Handler should initiate retraining via ContinuousLearningOrchestrator.
 */
using RetariningCallback = std::function<void(RetariningTrigger trigger,
                                              const std::string& reason)>;

/**
 * @brief Retraining Scheduler — Autonomous retraining orchestration
 *
 * Monitors multiple signals and triggers retraining when conditions are met:
 * - Time-based schedules (configurable intervals)
 * - Cost model drift (when RMSE increases beyond threshold)
 * - Quality regression (when production metrics degrade)
 *
 * Prevents concurrent retraining via a simple queuing mechanism.
 * Thread-safe for concurrent monitoring and callback dispatch.
 */
class RetariningScheduler {
 public:
  /**
   * @brief Constructor
   *
   * @param model_registry Reference to model lifecycle manager
   * @param retraining_interval_hours Base schedule interval (1-168 hours recommended)
   * @param drift_threshold_rmse Trigger retraining if cost model RMSE increases by this % (e.g., 0.10 for 10%)
   * @param quality_regression_threshold_pct Trigger if quality metrics degrade by % (e.g., 0.05 for 5%)
   */
  RetariningScheduler(ModelRegistry& model_registry,
                      uint32_t retraining_interval_hours = 24,
                      double drift_threshold_rmse = 0.15,
                      double quality_regression_threshold_pct = 0.05);
  ~RetariningScheduler();

  /**
   * @brief Register callback to be invoked when retraining should occur
   *
   * Only one callback can be registered at a time. Registering a new callback
   * replaces any existing callback.
   *
   * @param callback Function to invoke on trigger (must be thread-safe)
   */
  void SetRetariningCallback(const RetariningCallback& callback);

  /**
   * @brief Start the background monitoring loop
   *
   * Launches a background thread that periodically checks all trigger conditions
   * and invokes the registered callback if conditions are met.
   *
   * Should be called once at startup. Multiple calls are safe but have no effect.
   */
  void Start();

  /**
   * @brief Stop the background monitoring loop
   *
   * Stops monitoring and waits for any in-flight retraining to complete.
   * Safe to call multiple times.
   */
  void Stop();

  /**
   * @brief Check if retraining is currently in progress
   *
   * @return true if a retraining task has been triggered and not yet completed
   */
  bool IsRetariningInProgress() const;

  /**
   * @brief Manually request retraining
   *
   * Triggers retraining via kManualRequest trigger regardless of other conditions.
   * If retraining is already in progress, request is queued.
   *
   * @return true if request was queued, false if a limit was hit
   */
  bool RequestRetraining();

  /**
   * @brief Report cost model drift detection
   *
   * Called by CostModelBuilder when it detects drift.
   * Scheduler may trigger retraining if drift exceeds configured threshold.
   *
   * @param current_rmse Current model RMSE
   * @param baseline_rmse Previous model RMSE (for comparison)
   */
  void ReportCostModelDrift(double current_rmse, double baseline_rmse);

  /**
   * @brief Report quality metric regression
   *
   * Called by observability stack when production metrics degrade.
   * Scheduler may trigger retraining if regression exceeds configured threshold.
   *
   * @param metric_name Name of degraded metric (e.g., "ndcg@10", "recall@10")
   * @param current_value Current metric value
   * @param baseline_value Baseline value (for comparison)
   */
  void ReportQualityRegression(const std::string& metric_name, double current_value,
                               double baseline_value);

  /**
   * @brief Get last retraining trigger reason
   *
   * @return Tuple of (trigger_type, reason_string)
   */
  std::pair<RetariningTrigger, std::string> GetLastTriggerReason() const;

  /**
   * @brief Get timestamp of last retraining trigger (microseconds since epoch)
   *
   * @return 0 if no retraining has been triggered yet
   */
  uint64_t GetLastRetariningRequestTimeUs() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::lifecycle
