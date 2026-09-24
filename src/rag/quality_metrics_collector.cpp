/**
 * @file quality_metrics_collector.cpp
 * @brief Quality metrics aggregation implementation
 */

#include "rag/quality_metrics_collector.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <mutex>
#include <numeric>

namespace themis::rag::quality {

struct QualityMetricsCollector::Impl {
  std::mutex metrics_mutex;
  std::deque<QualityMetrics> metrics_buffer;
  uint32_t max_samples = 10000;
  uint64_t total_count = 0;
  
  // Percentile computation
  static double ComputePercentile(std::vector<double>& values, double p) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    size_t idx = static_cast<size_t>(
        std::ceil((p / 100.0) * static_cast<double>(values.size())) - 1);
    idx = std::min(idx, values.size() - 1);
    return values[idx];
  }
  
  // Regression analysis between two metric sets
  static double ComputeRegression(double current, double baseline) {
    if (baseline == 0.0) return 0.0;
    return ((current - baseline) / baseline) * 100.0;
  }
};

QualityMetricsCollector::QualityMetricsCollector(uint32_t max_samples_per_window)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->max_samples = max_samples_per_window;
}

QualityMetricsCollector::~QualityMetricsCollector() = default;

void QualityMetricsCollector::ReportMetrics(const QualityMetrics& metrics) {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
  pimpl_->metrics_buffer.push_back(metrics);
  pimpl_->total_count++;
  
  // Trim old entries if buffer exceeds max size
  while (pimpl_->metrics_buffer.size() > pimpl_->max_samples) {
    pimpl_->metrics_buffer.pop_front();
  }
}

AggregatedMetrics QualityMetricsCollector::GetAggregatedMetrics(
    std::chrono::system_clock::duration window_duration) {
  auto now = std::chrono::system_clock::now();
  auto window_start = now - window_duration;
  return GetMetricsForRange(window_start, now);
}

AggregatedMetrics QualityMetricsCollector::GetMetricsForRange(
    std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
  
  AggregatedMetrics result;
  result.window_start = start;
  result.window_end = end;
  
  std::vector<double> recall_10_values;
  std::vector<double> ndcg_10_values;
  std::vector<double> mrr_values;
  
  for (const auto& metric : pimpl_->metrics_buffer) {
    recall_10_values.push_back(metric.recall_at_10);
    ndcg_10_values.push_back(metric.ndcg_at_10);
    mrr_values.push_back(metric.mrr);
  }
  
  result.sample_count = recall_10_values.size();
  
  if (result.sample_count > 0) {
    // Recall@10
    double sum_recall = std::accumulate(recall_10_values.begin(), recall_10_values.end(), 0.0);
    result.mean_recall_10 = sum_recall / result.sample_count;
    result.p50_recall_10 = Impl::ComputePercentile(recall_10_values, 50);
    result.p75_recall_10 = Impl::ComputePercentile(recall_10_values, 75);
    result.p95_recall_10 = Impl::ComputePercentile(recall_10_values, 95);
    
    // NDCG@10
    double sum_ndcg = std::accumulate(ndcg_10_values.begin(), ndcg_10_values.end(), 0.0);
    result.mean_ndcg_10 = sum_ndcg / result.sample_count;
    result.p50_ndcg_10 = Impl::ComputePercentile(ndcg_10_values, 50);
    result.p95_ndcg_10 = Impl::ComputePercentile(ndcg_10_values, 95);
    
    // MRR
    double sum_mrr = std::accumulate(mrr_values.begin(), mrr_values.end(), 0.0);
    result.mean_mrr = sum_mrr / result.sample_count;
    result.p95_mrr = Impl::ComputePercentile(mrr_values, 95);
  }
  
  return result;
}

QualityMetricsCollector::RegressionAnalysis QualityMetricsCollector::CompareToBaseline(
    const AggregatedMetrics& current, const AggregatedMetrics& baseline) {
  RegressionAnalysis analysis;
  
  analysis.recall_10_change_pct = Impl::ComputeRegression(current.mean_recall_10, baseline.mean_recall_10);
  analysis.ndcg_10_change_pct = Impl::ComputeRegression(current.mean_ndcg_10, baseline.mean_ndcg_10);
  analysis.mrr_change_pct = Impl::ComputeRegression(current.mean_mrr, baseline.mean_mrr);
  
  double max_regression = std::min({analysis.recall_10_change_pct, analysis.ndcg_10_change_pct,
                                    analysis.mrr_change_pct});
  analysis.worst_regression_pct = std::abs(max_regression);
  
  // Regression if any metric degrades
  analysis.is_regression = (analysis.recall_10_change_pct < 0.0 ||
                           analysis.ndcg_10_change_pct < 0.0 ||
                           analysis.mrr_change_pct < 0.0);
  
  return analysis;
}

AggregatedMetrics QualityMetricsCollector::GetMetricsForModel(
    uint32_t model_version, std::chrono::system_clock::duration window_duration) {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
  
  auto now = std::chrono::system_clock::now();
  auto window_start = now - window_duration;
  
  AggregatedMetrics result;
  result.window_start = window_start;
  result.window_end = now;
  
  std::vector<double> recall_10_values;
  std::vector<double> ndcg_10_values;
  std::vector<double> mrr_values;
  
  // Filter for specific model version
  for (const auto& metric : pimpl_->metrics_buffer) {
    if (metric.model_version == model_version) {
      recall_10_values.push_back(metric.recall_at_10);
      ndcg_10_values.push_back(metric.ndcg_at_10);
      mrr_values.push_back(metric.mrr);
    }
  }
  
  result.sample_count = recall_10_values.size();
  
  if (result.sample_count > 0) {
    double sum_recall = std::accumulate(recall_10_values.begin(), recall_10_values.end(), 0.0);
    result.mean_recall_10 = sum_recall / result.sample_count;
    result.p50_recall_10 = Impl::ComputePercentile(recall_10_values, 50);
    result.p75_recall_10 = Impl::ComputePercentile(recall_10_values, 75);
    result.p95_recall_10 = Impl::ComputePercentile(recall_10_values, 95);
    
    double sum_ndcg = std::accumulate(ndcg_10_values.begin(), ndcg_10_values.end(), 0.0);
    result.mean_ndcg_10 = sum_ndcg / result.sample_count;
    result.p50_ndcg_10 = Impl::ComputePercentile(ndcg_10_values, 50);
    result.p95_ndcg_10 = Impl::ComputePercentile(ndcg_10_values, 95);
    
    double sum_mrr = std::accumulate(mrr_values.begin(), mrr_values.end(), 0.0);
    result.mean_mrr = sum_mrr / result.sample_count;
    result.p95_mrr = Impl::ComputePercentile(mrr_values, 95);
  }
  
  return result;
}

void QualityMetricsCollector::Reset() {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
  pimpl_->metrics_buffer.clear();
}

uint64_t QualityMetricsCollector::GetMetricsCount() const {
  std::lock_guard<std::mutex> lock(pimpl_->metrics_mutex);
  return pimpl_->total_count;
}

}  // namespace themis::rag::quality
