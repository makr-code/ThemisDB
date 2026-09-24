/**
 * @file cost_forecaster.cpp
 * @brief Implementation of CostForecastor for Phase 12
 */

#include "rag/cost_forecaster.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <mutex>
#include <numeric>
#include <unordered_map>

namespace themis::rag::optimization {

struct CostForecastor::Impl {
  std::mutex mu;
  std::deque<double> hourly_costs;
  std::deque<uint32_t> hourly_volumes;
  uint32_t lookback_days = 14;
  double alpha = 0.3;  // Exponential smoothing factor
  
  // System-wide time-of-day patterns
  std::array<double, 24> hourly_pattern = {};  // Average cost per hour of day
  std::array<uint32_t, 7> daily_pattern = {};  // Query volume by day of week
  
  AlertThreshold alert_config;
  
  // Per-tenant tracking
  std::unordered_map<std::string, std::deque<double>> tenant_costs;
};

CostForecastor::CostForecastor(uint32_t lookback_days, double alpha)
    : pimpl_(std::make_unique<Impl>()) {
  pimpl_->lookback_days = lookback_days;
  pimpl_->alpha = alpha;
  pimpl_->alert_config.cost_increase_pct = 50.0;
  pimpl_->alert_config.query_volume_increase_pct = 100.0;
  pimpl_->alert_config.z_score_threshold = 3.0;
}

CostForecastor::~CostForecastor() = default;

void CostForecastor::ReportHourlyCost(double hourly_cost, uint32_t query_volume,
                                     const std::string& tenant_id) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  struct tm* tm_info = std::localtime(&time_t);
  
  // Record system-wide metrics
  pimpl_->hourly_costs.push_back(hourly_cost);
  pimpl_->hourly_volumes.push_back(query_volume);
  
  // Update hourly pattern (hour of day)
  int hour = tm_info->tm_hour;
  pimpl_->hourly_pattern[hour] =
      pimpl_->alpha * hourly_cost + (1.0 - pimpl_->alpha) * pimpl_->hourly_pattern[hour];
  
  // Update daily pattern (day of week)
  int day = tm_info->tm_wday;
  pimpl_->daily_pattern[day] = (pimpl_->daily_pattern[day] * 6 + query_volume) / 7;
  
  // Keep only lookback window
  uint32_t max_samples = pimpl_->lookback_days * 24;
  while (pimpl_->hourly_costs.size() > max_samples) {
    pimpl_->hourly_costs.pop_front();
    pimpl_->hourly_volumes.pop_front();
  }
  
  // Per-tenant tracking
  if (!tenant_id.empty()) {
    pimpl_->tenant_costs[tenant_id].push_back(hourly_cost);
    if (pimpl_->tenant_costs[tenant_id].size() > 1000) {
      pimpl_->tenant_costs[tenant_id].pop_front();
    }
  }
}

std::vector<ForecastPoint> CostForecastor::ForecastNext24Hours() const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  std::vector<ForecastPoint> forecast;
  
  if (pimpl_->hourly_costs.empty()) {
    // Default forecast if no data
    for (int i = 0; i < 24; ++i) {
      ForecastPoint p;
      p.timestamp = std::chrono::system_clock::now() + std::chrono::hours(i);
      p.predicted_cost = 10.0;
      p.predicted_query_volume = 100;
      p.confidence_interval = 5.0;
      forecast.push_back(p);
    }
    return forecast;
  }
  
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  struct tm* tm_info = std::localtime(&time_t);
  int current_hour = tm_info->tm_hour;
  int current_day = tm_info->tm_wday;
  
  // Compute historical mean and standard deviation
  double mean_cost = 0.0;
  if (!pimpl_->hourly_costs.empty()) {
    mean_cost = std::accumulate(pimpl_->hourly_costs.begin(), pimpl_->hourly_costs.end(), 0.0) /
               pimpl_->hourly_costs.size();
  }
  
  double variance = 0.0;
  for (double cost : pimpl_->hourly_costs) {
    variance += (cost - mean_cost) * (cost - mean_cost);
  }
  if (pimpl_->hourly_costs.size() > 1) {
    variance /= pimpl_->hourly_costs.size();
  }
  double stddev = std::sqrt(variance);
  
  // Forecast 24 hours
  for (int i = 0; i < 24; ++i) {
    ForecastPoint p;
    p.timestamp = now + std::chrono::hours(i);
    
    // Use hourly pattern if available, else use mean
    int forecast_hour = (current_hour + i) % 24;
    if (pimpl_->hourly_pattern[forecast_hour] > 0) {
      p.predicted_cost = pimpl_->hourly_pattern[forecast_hour];
    } else {
      p.predicted_cost = mean_cost;
    }
    
    // Day-of-week adjustment
    int forecast_day = (current_day + i / 24) % 7;
    if (pimpl_->daily_pattern[forecast_day] > 0) {
      p.predicted_query_volume = pimpl_->daily_pattern[forecast_day];
    } else {
      p.predicted_query_volume = 100;
    }
    
    // Confidence interval widens for longer forecasts
    p.confidence_interval = stddev * (1.0 + 0.1 * i);  // ±10% per hour
    
    forecast.push_back(p);
  }
  
  return forecast;
}

double CostForecastor::ForecastWeeklyCost() const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  double weekly_forecast = 0.0;
  
  auto forecast = ForecastNext24Hours();
  
  // First day is 24-hour forecast
  for (const auto& p : forecast) {
    weekly_forecast += p.predicted_cost;
  }
  
  // Extrapolate remaining 6 days using historical pattern
  if (!pimpl_->hourly_costs.empty()) {
    double daily_mean = weekly_forecast / 24.0;  // Average from first day forecast
    
    // Adjust for remaining days using day-of-week pattern
    for (int day = 1; day < 7; ++day) {
      double day_factor = 1.0;  // Default: same as first day
      
      // Look up day-of-week pattern if available
      if (pimpl_->daily_pattern[day] > 0) {
        double baseline_volume = 100.0;  // Default baseline
        day_factor = pimpl_->daily_pattern[day] / baseline_volume;
      }
      
      weekly_forecast += daily_mean * 24.0 * day_factor;
    }
  }
  
  return weekly_forecast;
}

AnomalyDetectionResult CostForecastor::DetectAnomaly(double current_cost,
                                                     uint32_t current_volume) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  AnomalyDetectionResult result;
  
  if (pimpl_->hourly_costs.empty()) {
    return result;  // No historical data yet
  }
  
  // Compute mean and standard deviation
  double mean_cost = std::accumulate(pimpl_->hourly_costs.begin(), pimpl_->hourly_costs.end(), 0.0) /
                    pimpl_->hourly_costs.size();
  
  double variance = 0.0;
  for (double cost : pimpl_->hourly_costs) {
    variance += (cost - mean_cost) * (cost - mean_cost);
  }
  double stddev = std::sqrt(variance / pimpl_->hourly_costs.size());
  
  if (stddev < 1e-9) {
    return result;  // Zero variance, can't compute Z-score
  }
  
  // Compute Z-score
  result.z_score = (current_cost - mean_cost) / stddev;
  result.expected_value = mean_cost;
  result.actual_value = current_cost;
  
  // Z-score > 3 is considered anomalous (99.7% CI)
  if (std::abs(result.z_score) > 3.0) {
    result.is_anomaly = true;
    if (result.z_score > 3.0) {
      result.reason = "Cost spike: " + std::to_string(current_cost) + " vs expected " +
                     std::to_string(mean_cost);
    } else {
      result.reason = "Cost drop: " + std::to_string(current_cost) + " vs expected " +
                     std::to_string(mean_cost);
    }
  }
  
  return result;
}

bool CostForecastor::ShouldAlert(double current_cost, uint32_t current_volume,
                                const AlertThreshold& threshold) const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  if (pimpl_->hourly_costs.empty()) {
    return false;
  }
  
  // Get forecast for current hour
  auto forecast = ForecastNext24Hours();
  if (forecast.empty()) {
    return false;
  }
  
  double forecast_cost = forecast[0].predicted_cost;
  
  // Check cost increase threshold
  if (current_cost > forecast_cost * (1.0 + threshold.cost_increase_pct / 100.0)) {
    return true;
  }
  
  // Check volume increase threshold
  uint32_t forecast_volume = forecast[0].predicted_query_volume;
  if (current_volume > forecast_volume * (1.0 + threshold.query_volume_increase_pct / 100.0)) {
    return true;
  }
  
  // Check Z-score threshold
  auto anomaly = DetectAnomaly(current_cost, current_volume);
  if (std::abs(anomaly.z_score) > threshold.z_score_threshold) {
    return true;
  }
  
  return false;
}

CostForecastor::Statistics CostForecastor::GetHistoricalStats() const {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  
  Statistics stats;
  
  if (pimpl_->hourly_costs.empty()) {
    return stats;
  }
  
  stats.sample_count = pimpl_->hourly_costs.size();
  stats.mean = std::accumulate(pimpl_->hourly_costs.begin(), pimpl_->hourly_costs.end(), 0.0) /
              stats.sample_count;
  
  stats.min_value = *std::min_element(pimpl_->hourly_costs.begin(), pimpl_->hourly_costs.end());
  stats.max_value = *std::max_element(pimpl_->hourly_costs.begin(), pimpl_->hourly_costs.end());
  
  double variance = 0.0;
  for (double cost : pimpl_->hourly_costs) {
    variance += (cost - stats.mean) * (cost - stats.mean);
  }
  stats.stddev = std::sqrt(variance / stats.sample_count);
  
  return stats;
}

void CostForecastor::SetAlertThreshold(const AlertThreshold& threshold) {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  pimpl_->alert_config = threshold;
}

void CostForecastor::Reset() {
  std::unique_lock<std::mutex> lock(pimpl_->mu);
  pimpl_->hourly_costs.clear();
  pimpl_->hourly_volumes.clear();
  pimpl_->hourly_pattern.fill(0.0);
  pimpl_->daily_pattern.fill(0);
  pimpl_->tenant_costs.clear();
}

}  // namespace themis::rag::optimization
