/**
 * @file quality_alert_manager.cpp
 * @brief Alert generation and SLA tracking implementation
 */

#include "rag/quality_alert_manager.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <mutex>
#include <sstream>

namespace themis::rag::quality {

struct QualityAlertManager::Impl {
  std::mutex alerts_mutex;
  std::deque<Alert> alert_history;
  std::map<std::string, AlertSLA> sla_tracking;
  std::map<std::string, std::pair<double, double>> alert_thresholds;  // warning, critical
  std::chrono::system_clock::duration dedup_window = std::chrono::minutes(5);
  uint32_t alert_id_counter = 0;
  
  std::string GenerateAlertId() {
    return "alert-" + std::to_string(++alert_id_counter);
  }
  
  bool IsDuplicate(const std::string& metric_name, const std::deque<Alert>& history) {
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - dedup_window;
    
    for (const auto& alert : history) {
      if (alert.metric_name == metric_name && alert.timestamp > cutoff) {
        return true;
      }
    }
    return false;
  }
};

QualityAlertManager::QualityAlertManager() : pimpl_(std::make_unique<Impl>()) {}

QualityAlertManager::~QualityAlertManager() = default;

std::vector<Alert> QualityAlertManager::ReportMetric(const std::string& metric_name,
                                                     double current_value,
                                                     double warning_threshold,
                                                     double critical_threshold,
                                                     uint32_t model_version) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  std::vector<Alert> generated_alerts;
  auto now = std::chrono::system_clock::now();
  
  // Check if duplicate
  bool is_duplicate = pimpl_->IsDuplicate(metric_name, pimpl_->alert_history);
  
  // Determine alert level
  Alert alert;
  alert.timestamp = now;
  alert.metric_name = metric_name;
  alert.metric_value = current_value;
  alert.model_version = model_version;
  alert.is_duplicate = is_duplicate;
  
  if (current_value < critical_threshold) {
    alert.severity = AlertSeverity::kCritical;
    alert.threshold = critical_threshold;
    alert.percent_below_threshold = ((critical_threshold - current_value) / critical_threshold) * 100.0;
    alert.title = "CRITICAL: " + metric_name + " below threshold";
    alert.message = metric_name + " = " + std::to_string(current_value) +
                   " (critical threshold: " + std::to_string(critical_threshold) + ")";
    
    if (!is_duplicate) {
      alert.alert_id = pimpl_->GenerateAlertId();
      pimpl_->alert_history.push_back(alert);
      generated_alerts.push_back(alert);
      
      // Update SLA tracking
      if (!pimpl_->sla_tracking.count(metric_name)) {
        pimpl_->sla_tracking[metric_name] = AlertSLA{AlertSeverity::kCritical};
      }
      auto& sla = pimpl_->sla_tracking[metric_name];
      sla.last_triggered = now;
      sla.total_occurrences++;
      if (sla.first_triggered == std::chrono::system_clock::time_point()) {
        sla.first_triggered = now;
      }
    }
  } else if (current_value < warning_threshold) {
    alert.severity = AlertSeverity::kWarning;
    alert.threshold = warning_threshold;
    alert.percent_below_threshold = ((warning_threshold - current_value) / warning_threshold) * 100.0;
    alert.title = "WARNING: " + metric_name + " below threshold";
    alert.message = metric_name + " = " + std::to_string(current_value) +
                   " (warning threshold: " + std::to_string(warning_threshold) + ")";
    
    if (!is_duplicate) {
      alert.alert_id = pimpl_->GenerateAlertId();
      pimpl_->alert_history.push_back(alert);
      generated_alerts.push_back(alert);
      
      if (!pimpl_->sla_tracking.count(metric_name)) {
        pimpl_->sla_tracking[metric_name] = AlertSLA{AlertSeverity::kWarning};
      }
      auto& sla = pimpl_->sla_tracking[metric_name];
      sla.last_triggered = now;
      sla.total_occurrences++;
      if (sla.first_triggered == std::chrono::system_clock::time_point()) {
        sla.first_triggered = now;
      }
    }
  }
  
  return generated_alerts;
}

void QualityAlertManager::SetAlertThresholds(const std::string& metric_name,
                                            double warning_threshold,
                                            double critical_threshold) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  pimpl_->alert_thresholds[metric_name] = {warning_threshold, critical_threshold};
}

void QualityAlertManager::SetDeduplicationWindow(std::chrono::system_clock::duration duration) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  pimpl_->dedup_window = duration;
}

std::vector<Alert> QualityAlertManager::GetRecentAlerts(uint32_t max_count) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  std::vector<Alert> result;
  auto start = pimpl_->alert_history.rbegin();
  auto end = pimpl_->alert_history.rend();
  
  uint32_t count = 0;
  for (auto it = start; it != end && count < max_count; ++it) {
    result.push_back(*it);
    count++;
  }
  
  return result;
}

std::vector<Alert> QualityAlertManager::GetAlertsForMetric(
    const std::string& metric_name, std::chrono::system_clock::duration time_range) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  auto cutoff = std::chrono::system_clock::now() - time_range;
  std::vector<Alert> result;
  
  for (const auto& alert : pimpl_->alert_history) {
    if (alert.metric_name == metric_name && alert.timestamp > cutoff) {
      result.push_back(alert);
    }
  }
  
  return result;
}

AlertSLA QualityAlertManager::GetSLAStatus(AlertSeverity severity) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  AlertSLA combined;
  combined.severity = severity;
  
  for (const auto& pair : pimpl_->sla_tracking) {
    const auto& sla = pair.second;
    if (sla.severity == severity) {
      combined.total_occurrences += sla.total_occurrences;
      if (sla.last_triggered > combined.last_triggered) {
        combined.last_triggered = sla.last_triggered;
      }
    }
  }
  
  // Check if any alert is unresolved for too long
  if (combined.last_triggered != std::chrono::system_clock::time_point()) {
    auto elapsed = std::chrono::system_clock::now() - combined.last_triggered;
    combined.is_breached = (elapsed > combined.max_response_time);
  }
  
  return combined;
}

void QualityAlertManager::ResolveAlert(const std::string& alert_id,
                                      const std::string& resolution_note) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  for (auto& alert : pimpl_->alert_history) {
    if (alert.alert_id == alert_id) {
      // Mark as resolved (simplified: just clear for now)
      // In production: track resolution time, close ticket, etc.
      return;
    }
  }
}

QualityAlertManager::AlertTrend QualityAlertManager::GetAlertTrend(
    const std::string& metric_name, uint32_t days) {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  
  AlertTrend trend;
  trend.metric_name = metric_name;
  
  auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(24 * days);
  
  for (const auto& alert : pimpl_->alert_history) {
    if (alert.metric_name == metric_name && alert.timestamp > cutoff) {
      if (alert.severity == AlertSeverity::kWarning) {
        trend.warning_count++;
      } else if (alert.severity == AlertSeverity::kCritical) {
        trend.critical_count++;
      }
    }
  }
  
  // Compute trend slope (alerts per day)
  double total_alerts = trend.warning_count + trend.critical_count;
  trend.trend_slope = (days > 0) ? (total_alerts / days) : 0.0;
  trend.is_escalating = (trend.trend_slope > 0.5);  // >1 alert per 2 days = escalating
  
  return trend;
}

void QualityAlertManager::Reset() {
  std::lock_guard<std::mutex> lock(pimpl_->alerts_mutex);
  pimpl_->alert_history.clear();
  pimpl_->sla_tracking.clear();
  pimpl_->alert_id_counter = 0;
}

}  // namespace themis::rag::quality
