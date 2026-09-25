/**
 * @file metrics_reporter.h
 * @brief Dashboard and trend analysis for RAG Phase 13
 *
 * Time-series export, trend analysis, comparative reports, and anomaly
 * summarization for operator dashboards.
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
#include <chrono>

namespace themis::rag::quality {

/**
 * @brief Time-series data point
 */
struct TimeSeriesPoint {
  std::chrono::system_clock::time_point timestamp;
  double value = 0.0;
  double std_deviation = 0.0;  ///< Uncertainty/confidence
};

/**
 * @brief Trend analysis result
 */
struct Trend {
  double slope = 0.0;                  ///< Linear regression slope (change per hour)
  double r_squared = 0.0;              ///< Goodness of fit
  std::string direction;               ///< "improving", "stable", "degrading"
  double velocity = 0.0;               ///< Rate of change per hour
  double acceleration = 0.0;           ///< Rate of change of rate
  uint32_t data_points = 0;
};

/**
 * @brief Comparative report (model vs model, period vs period)
 */
struct ComparativeReport {
  std::string metric_name;
  
  // Comparison entities
  std::string entity_a_name;           ///< Model A, Period A, etc.
  double entity_a_mean = 0.0;
  double entity_a_p95 = 0.0;
  
  std::string entity_b_name;           ///< Model B, Period B, etc.
  double entity_b_mean = 0.0;
  double entity_b_p95 = 0.0;
  
  // Analysis
  double difference_pct = 0.0;         ///< (B - A) / A * 100
  bool is_statistically_significant = false;
  std::string winner;                  ///< Which entity is better
};

/**
 * @brief Anomaly in metrics
 */
struct MetricAnomaly {
  std::string metric_name;
  std::chrono::system_clock::time_point timestamp;
  double value = 0.0;
  double expected_value = 0.0;
  double z_score = 0.0;                ///< Deviation in standard deviations
  std::string anomaly_type;            ///< "spike", "drop", "plateau"
  bool is_severe = false;              ///< z_score > 3.0
};

/**
 * @brief Metrics Reporter — Dashboard data generation
 *
 * Exports time-series data, computes trends, generates comparative reports,
 * and identifies anomalies for operator dashboards and analysis tools.
 *
 * Thread-safe for concurrent reporting.
 */
class MetricsReporter {
 public:
  /**
   * @brief Constructor
   *
   * @param historical_window_days Keep this much historical data (default 30 days)
   */
  explicit MetricsReporter(uint32_t historical_window_days = 30);
  ~MetricsReporter();

  /**
   * @brief Export time-series data for a metric
   *
   * Returns raw data points suitable for graphing (JSON, CSV, etc.).
   *
   * @param metric_name (e.g., "recall_10", "ndcg_10", "mrr")
   * @param start Start of time range
   * @param end End of time range
   * @return Time-series data points
   */
  std::vector<TimeSeriesPoint> ExportTimeSeries(
      const std::string& metric_name, std::chrono::system_clock::time_point start,
      std::chrono::system_clock::time_point end);

  /**
   * @brief Export to JSON format (for dashboards)
   *
   * @param metric_name
   * @param start
   * @param end
   * @return JSON string with time-series data
   */
  std::string ExportToJSON(const std::string& metric_name,
                          std::chrono::system_clock::time_point start,
                          std::chrono::system_clock::time_point end);

  /**
   * @brief Export to CSV format (for analysis)
   *
   * @param metric_names Multiple metrics to export
   * @param start
   * @param end
   * @return CSV string
   */
  std::string ExportToCSV(const std::vector<std::string>& metric_names,
                         std::chrono::system_clock::time_point start,
                         std::chrono::system_clock::time_point end);

  /**
   * @brief Analyze trend for a metric
   *
   * Uses linear regression to fit trend; computes velocity and acceleration.
   *
   * @param metric_name
   * @param window Time window to analyze
   * @return Trend analysis
   */
  Trend AnalyzeTrend(const std::string& metric_name,
                    std::chrono::system_clock::duration window);

  /**
   * @brief Compare two models
   *
   * Computes mean/p95 for each metric, performs t-test to check significance.
   *
   * @param metric_name
   * @param model_a_version
   * @param model_b_version
   * @param period Time period to analyze
   * @return Comparative report
   */
  ComparativeReport CompareModels(const std::string& metric_name, uint32_t model_a_version,
                                 uint32_t model_b_version,
                                 std::chrono::system_clock::duration period);

  /**
   * @brief Compare two time periods
   *
   * E.g., "this week vs last week", "this month vs baseline month".
   *
   * @param metric_name
   * @param period_a_start Period A start time
   * @param period_a_end Period A end time
   * @param period_b_start Period B start time
   * @param period_b_end Period B end time
   * @return Comparative report
   */
  ComparativeReport ComparePeriods(const std::string& metric_name,
                                  std::chrono::system_clock::time_point period_a_start,
                                  std::chrono::system_clock::time_point period_a_end,
                                  std::chrono::system_clock::time_point period_b_start,
                                  std::chrono::system_clock::time_point period_b_end);

  /**
   * @brief Detect anomalies in recent metric data
   *
   * Uses Z-score test to identify unusual values.
   *
   * @param metric_name
   * @param z_score_threshold Anomaly threshold (default 3.0 = 99.7% CI)
   * @return List of detected anomalies
   */
  std::vector<MetricAnomaly> DetectAnomalies(const std::string& metric_name,
                                            double z_score_threshold = 3.0);

  /**
   * @brief Get summary report for operator dashboard
   *
   * Returns high-level summary of key metrics: current values, trends,
   * anomalies, alerts.
   *
   * @return JSON summary suitable for dashboard display
   */
  std::string GenerateDashboardSummary();

  /**
   * @brief Record metric data point
   *
   * Called to add data for export/analysis.
   *
   * @param metric_name
   * @param value
   * @param timestamp
   * @param model_version Optional model version
   */
  void RecordMetricPoint(const std::string& metric_name, double value,
                        std::chrono::system_clock::time_point timestamp,
                        uint32_t model_version = 0);

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::quality
