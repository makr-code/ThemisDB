/**
 * @file quality_metrics_collector.h
 * @brief Quality metrics aggregation for RAG Phase 13
 *
 * Collects and aggregates IR metrics (recall, NDCG, MRR) at scale with
 * percentile computation, time-windowing, and baseline comparison.
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
 * @brief Quality metrics for a query/evaluation
 */
struct QualityMetrics {
  double recall_at_1 = 0.0;         ///< Recall@1 (0-1)
  double recall_at_5 = 0.0;         ///< Recall@5
  double recall_at_10 = 0.0;        ///< Recall@10
  
  double ndcg_at_1 = 0.0;           ///< NDCG@1 (normalized DCG)
  double ndcg_at_5 = 0.0;           ///< NDCG@5
  double ndcg_at_10 = 0.0;          ///< NDCG@10
  
  double mrr = 0.0;                 ///< Mean Reciprocal Rank
  double map = 0.0;                 ///< Mean Average Precision
  double precision_at_5 = 0.0;      ///< Precision@5
  
  double faithfulness_score = 0.0;  ///< LLM judge: faithfulness (0-1)
  double relevance_score = 0.0;     ///< LLM judge: relevance (0-1)
  
  uint32_t model_version = 0;       ///< Model version from Phase 11
  std::string dataset_name;         ///< Benchmark dataset
};

/**
 * @brief Aggregated metrics with percentiles
 */
struct AggregatedMetrics {
  double mean_recall_10 = 0.0;
  double p50_recall_10 = 0.0;       ///< 50th percentile
  double p75_recall_10 = 0.0;       ///< 75th percentile
  double p95_recall_10 = 0.0;       ///< 95th percentile
  
  double mean_ndcg_10 = 0.0;
  double p50_ndcg_10 = 0.0;
  double p95_ndcg_10 = 0.0;
  
  double mean_mrr = 0.0;
  double p95_mrr = 0.0;
  
  uint32_t sample_count = 0;        ///< Number of queries evaluated
  std::chrono::system_clock::time_point window_start;
  std::chrono::system_clock::time_point window_end;
};

/**
 * @brief Quality Metrics Collector — Scale metric aggregation
 *
 * Collects individual query evaluation metrics and aggregates them into
 * windowed summaries with percentile statistics. Supports comparison against
 * baseline (previous model or SLA target).
 *
 * Thread-safe for concurrent metric reporting.
 */
class QualityMetricsCollector {
 public:
  /**
   * @brief Constructor
   *
   * @param max_samples_per_window Maximum metrics to buffer before aggregation (default 10000)
   */
  explicit QualityMetricsCollector(uint32_t max_samples_per_window = 10000);
  ~QualityMetricsCollector();

  /**
   * @brief Report query evaluation metrics
   *
   * Called after evaluation to record metrics for this query.
   *
   * @param metrics Query evaluation results
   */
  void ReportMetrics(const QualityMetrics& metrics);

  /**
   * @brief Get aggregated metrics for current time window
   *
   * Computes percentiles (p50, p75, p95) and mean for all collected metrics.
   * Window is typically 1-hour or 1-day based on configuration.
   *
   * @param window_duration Duration to aggregate (e.g., 1 hour)
   * @return Aggregated metrics with percentiles
   */
  AggregatedMetrics GetAggregatedMetrics(
      std::chrono::system_clock::duration window_duration = std::chrono::hours(1));

  /**
   * @brief Get metrics for specific time range
   *
   * @param start Start of time range
   * @param end End of time range
   * @return Aggregated metrics for that period
   */
  AggregatedMetrics GetMetricsForRange(std::chrono::system_clock::time_point start,
                                      std::chrono::system_clock::time_point end);

  /**
   * @brief Compare current metrics against baseline
   *
   * Computes % change from baseline. Positive = improvement, negative = regression.
   *
   * @param current Current aggregated metrics
   * @param baseline Previous/expected metrics
   * @return Map of metric_name → percent_change (e.g., "recall@10" → -5.2 means 5.2% regression)
   */
  struct RegressionAnalysis {
    double recall_10_change_pct = 0.0;
    double ndcg_10_change_pct = 0.0;
    double mrr_change_pct = 0.0;
    bool is_regression = false;  ///< true if any metric degraded
    double worst_regression_pct = 0.0;
  };
  RegressionAnalysis CompareToBaseline(const AggregatedMetrics& current,
                                       const AggregatedMetrics& baseline);

  /**
   * @brief Get metrics for specific model version
   *
   * @param model_version Model version from Phase 11
   * @param window_duration Time window to aggregate
   * @return Model-specific aggregated metrics
   */
  AggregatedMetrics GetMetricsForModel(uint32_t model_version,
                                      std::chrono::system_clock::duration window_duration);

  /**
   * @brief Clear historical data
   *
   * Useful for testing or restarting collection.
   */
  void Reset();

  /**
   * @brief Get total number of metrics collected
   *
   * @return Cumulative count of reported metrics
   */
  uint64_t GetMetricsCount() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::quality
