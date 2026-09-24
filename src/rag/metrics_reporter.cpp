/**
 * @file metrics_reporter.cpp
 * @brief Dashboard and trend analysis implementation
 */

#include "rag/metrics_reporter.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <numeric>
#include <sstream>

namespace themis::rag::quality {

struct MetricsReporter::Impl {
  std::mutex reporter_mutex;
  std::map<std::string, std::deque<TimeSeriesPoint>> time_series_data;
  uint32_t historical_window_days = 30;
  
  // Linear regression helper
  static std::pair<double, double> LinearRegression(const std::vector<std::pair<double, double>>& points) {
    if (points.size() < 2) return {0.0, 1.0};
    
    double n = static_cast<double>(points.size());
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
    
    for (const auto& p : points) {
      sum_x += p.first;
      sum_y += p.second;
      sum_xy += p.first * p.second;
      sum_x2 += p.first * p.first;
    }
    
    double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
    double intercept = (sum_y - slope * sum_x) / n;
    
    return {slope, intercept};
  }
  
  // Compute R-squared
  static double ComputeRSquared(const std::vector<std::pair<double, double>>& points,
                               double slope, double intercept) {
    if (points.empty()) return 0.0;
    
    double mean_y = 0.0;
    for (const auto& p : points) {
      mean_y += p.second;
    }
    mean_y /= points.size();
    
    double ss_tot = 0.0, ss_res = 0.0;
    for (const auto& p : points) {
      double predicted = slope * p.first + intercept;
      ss_res += (p.second - predicted) * (p.second - predicted);
      ss_tot += (p.second - mean_y) * (p.second - mean_y);
    }
    
    if (ss_tot == 0.0) return 0.0;
    return 1.0 - (ss_res / ss_tot);
  }
};

MetricsReporter::MetricsReporter(uint32_t historical_window_days)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->historical_window_days = historical_window_days;
}

MetricsReporter::~MetricsReporter() = default;

std::vector<TimeSeriesPoint> MetricsReporter::ExportTimeSeries(
    const std::string& metric_name, std::chrono::system_clock::time_point start,
    std::chrono::system_clock::time_point end) {
  std::lock_guard<std::mutex> lock(pimpl_->reporter_mutex);
  
  std::vector<TimeSeriesPoint> result;
  
  if (!pimpl_->time_series_data.count(metric_name)) {
    return result;
  }
  
  const auto& data = pimpl_->time_series_data[metric_name];
  for (const auto& point : data) {
    if (point.timestamp >= start && point.timestamp <= end) {
      result.push_back(point);
    }
  }
  
  return result;
}

std::string MetricsReporter::ExportToJSON(const std::string& metric_name,
                                         std::chrono::system_clock::time_point start,
                                         std::chrono::system_clock::time_point end) {
  auto points = ExportTimeSeries(metric_name, start, end);
  
  std::ostringstream oss;
  oss << "{\"metric\":\"" << metric_name << "\",\"data\":[";
  
  for (size_t i = 0; i < points.size(); ++i) {
    if (i > 0) oss << ",";
    oss << "{\"timestamp\":" << std::chrono::system_clock::to_time_t(points[i].timestamp)
       << ",\"value\":" << points[i].value << "}";
  }
  
  oss << "]}";
  return oss.str();
}

std::string MetricsReporter::ExportToCSV(const std::vector<std::string>& metric_names,
                                        std::chrono::system_clock::time_point start,
                                        std::chrono::system_clock::time_point end) {
  std::ostringstream oss;
  
  // Header
  oss << "timestamp";
  for (const auto& name : metric_names) {
    oss << "," << name;
  }
  oss << "\n";
  
  // Collect all timestamps
  std::set<std::chrono::system_clock::time_point> all_timestamps;
  std::map<std::string, std::vector<TimeSeriesPoint>> data_by_metric;
  
  for (const auto& name : metric_names) {
    data_by_metric[name] = ExportTimeSeries(name, start, end);
    for (const auto& point : data_by_metric[name]) {
      all_timestamps.insert(point.timestamp);
    }
  }
  
  // Output rows
  for (const auto& ts : all_timestamps) {
    oss << std::chrono::system_clock::to_time_t(ts);
    for (const auto& name : metric_names) {
      oss << ",";
      bool found = false;
      for (const auto& point : data_by_metric[name]) {
        if (point.timestamp == ts) {
          oss << point.value;
          found = true;
          break;
        }
      }
      if (!found) {
        oss << "";  // Empty cell if metric not available at this timestamp
      }
    }
    oss << "\n";
  }
  
  return oss.str();
}

Trend MetricsReporter::AnalyzeTrend(const std::string& metric_name,
                                   std::chrono::system_clock::duration window) {
  auto points = ExportTimeSeries(metric_name, std::chrono::system_clock::now() - window,
                                std::chrono::system_clock::now());
  
  Trend trend;
  trend.data_points = points.size();
  
  if (points.size() < 2) {
    return trend;  // Not enough data
  }
  
  // Prepare data for regression (x = time in hours, y = metric value)
  std::vector<std::pair<double, double>> regression_points;
  double min_time = std::numeric_limits<double>::max();
  
  for (const auto& point : points) {
    double time_val = std::chrono::system_clock::to_time_t(point.timestamp);
    if (time_val < min_time) min_time = time_val;
  }
  
  for (const auto& point : points) {
    double time_hours = (std::chrono::system_clock::to_time_t(point.timestamp) - min_time) / 3600.0;
    regression_points.push_back({time_hours, point.value});
  }
  
  auto [slope, intercept] = Impl::LinearRegression(regression_points);
  trend.slope = slope;
  trend.r_squared = Impl::ComputeRSquared(regression_points, slope, intercept);
  trend.velocity = slope;  // Change per hour
  
  // Simplified acceleration (change of velocity)
  if (regression_points.size() > 3) {
    auto first_half = std::vector<std::pair<double, double>>(
        regression_points.begin(), regression_points.begin() + regression_points.size() / 2);
    auto second_half = std::vector<std::pair<double, double>>(
        regression_points.begin() + regression_points.size() / 2, regression_points.end());
    
    auto [slope1, _] = Impl::LinearRegression(first_half);
    auto [slope2, __] = Impl::LinearRegression(second_half);
    trend.acceleration = slope2 - slope1;
  }
  
  // Determine direction
  if (slope > 0.01) {
    trend.direction = "improving";
  } else if (slope < -0.01) {
    trend.direction = "degrading";
  } else {
    trend.direction = "stable";
  }
  
  return trend;
}

ComparativeReport MetricsReporter::CompareModels(const std::string& metric_name,
                                                uint32_t model_a_version,
                                                uint32_t model_b_version,
                                                std::chrono::system_clock::duration period) {
  // TODO: Implement with access to model-specific metrics
  ComparativeReport report;
  report.metric_name = metric_name;
  report.entity_a_name = "Model " + std::to_string(model_a_version);
  report.entity_b_name = "Model " + std::to_string(model_b_version);
  return report;
}

ComparativeReport MetricsReporter::ComparePeriods(
    const std::string& metric_name, std::chrono::system_clock::time_point period_a_start,
    std::chrono::system_clock::time_point period_a_end,
    std::chrono::system_clock::time_point period_b_start,
    std::chrono::system_clock::time_point period_b_end) {
  std::lock_guard<std::mutex> lock(pimpl_->reporter_mutex);
  
  ComparativeReport report;
  report.metric_name = metric_name;
  report.entity_a_name = "Period A";
  report.entity_b_name = "Period B";
  
  auto points_a = ExportTimeSeries(metric_name, period_a_start, period_a_end);
  auto points_b = ExportTimeSeries(metric_name, period_b_start, period_b_end);
  
  if (points_a.empty() || points_b.empty()) {
    return report;
  }
  
  // Compute means
  double sum_a = 0.0, sum_b = 0.0;
  for (const auto& p : points_a) sum_a += p.value;
  for (const auto& p : points_b) sum_b += p.value;
  
  report.entity_a_mean = sum_a / points_a.size();
  report.entity_b_mean = sum_b / points_b.size();
  
  // Compute p95
  std::vector<double> values_a, values_b;
  for (const auto& p : points_a) values_a.push_back(p.value);
  for (const auto& p : points_b) values_b.push_back(p.value);
  
  std::sort(values_a.begin(), values_a.end());
  std::sort(values_b.begin(), values_b.end());
  
  report.entity_a_p95 = values_a[static_cast<size_t>(values_a.size() * 0.95)];
  report.entity_b_p95 = values_b[static_cast<size_t>(values_b.size() * 0.95)];
  
  report.difference_pct = ((report.entity_b_mean - report.entity_a_mean) / report.entity_a_mean) * 100.0;
  report.winner = (report.entity_b_mean > report.entity_a_mean) ? "Period B" : "Period A";
  
  return report;
}

std::vector<MetricAnomaly> MetricsReporter::DetectAnomalies(const std::string& metric_name,
                                                           double z_score_threshold) {
  std::lock_guard<std::mutex> lock(pimpl_->reporter_mutex);
  
  std::vector<MetricAnomaly> anomalies;
  
  if (!pimpl_->time_series_data.count(metric_name)) {
    return anomalies;
  }
  
  const auto& data = pimpl_->time_series_data[metric_name];
  if (data.size() < 3) return anomalies;
  
  // Compute mean and std dev
  double sum = 0.0;
  for (const auto& point : data) {
    sum += point.value;
  }
  double mean = sum / data.size();
  
  double sum_sq_diff = 0.0;
  for (const auto& point : data) {
    double diff = point.value - mean;
    sum_sq_diff += diff * diff;
  }
  double std_dev = std::sqrt(sum_sq_diff / data.size());
  
  // Detect anomalies
  for (const auto& point : data) {
    double z_score = (point.value - mean) / std_dev;
    if (std::abs(z_score) > z_score_threshold) {
      MetricAnomaly anomaly;
      anomaly.metric_name = metric_name;
      anomaly.timestamp = point.timestamp;
      anomaly.value = point.value;
      anomaly.expected_value = mean;
      anomaly.z_score = z_score;
      anomaly.is_severe = (std::abs(z_score) > 3.0);
      
      if (point.value > mean) {
        anomaly.anomaly_type = "spike";
      } else {
        anomaly.anomaly_type = "drop";
      }
      
      anomalies.push_back(anomaly);
    }
  }
  
  return anomalies;
}

std::string MetricsReporter::GenerateDashboardSummary() {
  std::lock_guard<std::mutex> lock(pimpl_->reporter_mutex);
  
  std::ostringstream oss;
  oss << "{\"metrics\":[";
  
  bool first = true;
  for (const auto& pair : pimpl_->time_series_data) {
    if (!first) oss << ",";
    first = false;
    
    const auto& name = pair.first;
    const auto& data = pair.second;
    
    if (!data.empty()) {
      double latest = data.back().value;
      oss << "{\"name\":\"" << name << "\",\"latest\":" << latest << "}";
    }
  }
  
  oss << "]}";
  return oss.str();
}

void MetricsReporter::RecordMetricPoint(const std::string& metric_name, double value,
                                       std::chrono::system_clock::time_point timestamp,
                                       uint32_t model_version) {
  std::lock_guard<std::mutex> lock(pimpl_->reporter_mutex);
  
  TimeSeriesPoint point;
  point.timestamp = timestamp;
  point.value = value;
  point.std_deviation = 0.0;  // Simplified
  
  pimpl_->time_series_data[metric_name].push_back(point);
  
  // Trim old data beyond historical window
  auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * pimpl_->historical_window_days);
  while (!pimpl_->time_series_data[metric_name].empty() &&
         pimpl_->time_series_data[metric_name].front().timestamp < cutoff) {
    pimpl_->time_series_data[metric_name].pop_front();
  }
}

}  // namespace themis::rag::quality
