/**
 * @file cost_forecaster.h
 * @brief Time-series forecasting and anomaly detection for RAG Phase 12
 *
 * Predicts cost trends and detects sudden spikes for proactive budgeting.
 *
 * @version 0.1.0
 * @note Phase: 12 (Advanced Cost Optimization)
 * @note Status: IMPLEMENTATION
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::optimization {

/**
 * @brief Forecast time point
 */
struct ForecastPoint {
  std::chrono::system_clock::time_point timestamp;
  double predicted_cost = 0.0;       ///< USD
  double predicted_query_volume = 0; ///< Queries per hour
  double confidence_interval = 0.0;  ///< 95% CI width
};

/**
 * @brief Anomaly detection result
 */
struct AnomalyDetectionResult {
  bool is_anomaly = false;
  double z_score = 0.0;              ///< Standard deviations from mean
  double expected_value = 0.0;       ///< Expected based on forecast
  double actual_value = 0.0;         ///< Observed value
  std::string reason;                ///< Why detected as anomaly
};

/**
 * @brief Alert trigger configuration
 */
struct AlertThreshold {
  double cost_increase_pct = 50.0;   ///< Alert if cost jumps 50% above forecast
  double query_volume_increase_pct = 100.0;  ///< Alert if volume doubles
  double z_score_threshold = 3.0;    ///< Alert if Z-score > 3 (99.7% CI)
};

/**
 * @brief Cost Forecaster — Predictive budgeting and anomaly detection
 *
 * Tracks daily and weekly cost patterns using exponential smoothing and
 * moving averages. Forecasts future costs and detects sudden spikes.
 * Integrates with Phase 10 cost model to refine predictions.
 *
 * Thread-safe for concurrent metric collection.
 */
class CostForecastor {
 public:
  /**
   * @brief Constructor
   *
   * @param lookback_days Historical window for pattern detection (default 14)
   * @param alpha Exponential smoothing factor (0-1, default 0.3)
   */
  CostForecastor(uint32_t lookback_days = 14, double alpha = 0.3);
  ~CostForecastor();

  /**
   * @brief Report hourly cost observed
   *
   * Called at end of each hour to record actual cost and query volume.
   *
   * @param hourly_cost USD spent in this hour
   * @param query_volume Queries processed
   * @param tenant_id Optional: report per-tenant, or "" for system-wide
   */
  void ReportHourlyCost(double hourly_cost, uint32_t query_volume,
                       const std::string& tenant_id = "");

  /**
   * @brief Forecast next 24 hours of cost
   *
   * Returns hourly cost predictions for next 24 hours.
   * Forecast incorporates:
   * - Time-of-day pattern (peak hours, off-peak)
   * - Day-of-week pattern (weekday vs weekend)
   * - Trend (increasing/decreasing load)
   *
   * @return List of 24 hourly forecast points
   */
  std::vector<ForecastPoint> ForecastNext24Hours() const;

  /**
   * @brief Forecast weekly cost
   *
   * @return Estimated total cost for next 7 days
   */
  double ForecastWeeklyCost() const;

  /**
   * @brief Detect anomaly in current metrics
   *
   * Checks if current cost/volume is anomalously high or low.
   * Uses Z-score test against historical baseline.
   *
   * @param current_cost Current hourly cost
   * @param current_volume Current query volume
   * @return Anomaly detection result with reason
   */
  AnomalyDetectionResult DetectAnomaly(double current_cost, uint32_t current_volume) const;

  /**
   * @brief Check alert conditions
   *
   * Returns true if any alert threshold is breached.
   *
   * @param current_cost Current cost observation
   * @param current_volume Current volume
   * @param threshold Alert configuration
   * @return true if alert should be triggered
   */
  bool ShouldAlert(double current_cost, uint32_t current_volume,
                  const AlertThreshold& threshold) const;

  /**
   * @brief Get historical statistics
   *
   * @return Mean, std dev, min, max of recent costs
   */
  struct Statistics {
    double mean = 0.0;
    double stddev = 0.0;
    double min_value = 0.0;
    double max_value = 0.0;
    uint32_t sample_count = 0;
  };

  /**
   * @brief Get historical latency statistics for this forecaster.
   * @return Statistics struct with mean, stddev, min, max, and sample count.
   */
  Statistics GetHistoricalStats() const;

  /**
   * @brief Set alert thresholds
   *
   * @param threshold Alert configuration
   */
  void SetAlertThreshold(const AlertThreshold& threshold);

  /**
   * @brief Reset forecaster state
   *
   * Clears all historical data (e.g., for testing or after deployment).
   */
  void Reset();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::optimization
