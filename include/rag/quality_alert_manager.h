/**
 * @file quality_alert_manager.h
 * @brief Alert generation and SLA tracking for RAG Phase 13
 *
 * Multi-level alerting (warning/critical/escalation) with deduplication
 * and alert history tracking.
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
 * @brief Alert severity levels
 */
enum class AlertSeverity { kInfo, kWarning, kCritical, kEscalation };

/**
 * @brief Alert object
 */
struct Alert {
  AlertSeverity severity = AlertSeverity::kInfo;
  std::string alert_id;                        ///< Unique alert identifier
  std::string title;                           ///< Human-readable title
  std::string message;                         ///< Detailed message
  std::string metric_name;                     ///< Which metric triggered (e.g., "recall@10")
  double metric_value = 0.0;                   ///< Current metric value
  double threshold = 0.0;                      ///< Alert threshold
  double percent_below_threshold = 0.0;        ///< How far below (%)
  std::chrono::system_clock::time_point timestamp;
  uint32_t model_version = 0;                  ///< Model this alert is about
  bool is_duplicate = false;                   ///< true if suppressed as duplicate
};

/**
 * @brief Alert SLA tracking
 */
struct AlertSLA {
  AlertSeverity severity = AlertSeverity::kInfo;
  std::chrono::milliseconds max_response_time = std::chrono::milliseconds(300000);  ///< 5 min
  uint32_t escalation_count = 0;               ///< Alert if unresolved >N times
  
  // Tracking
  std::chrono::system_clock::time_point first_triggered;
  std::chrono::system_clock::time_point last_triggered;
  uint32_t total_occurrences = 0;
  bool is_breached = false;
};

/**
 * @brief Quality Alert Manager — Multi-level alerting
 *
 * Generates and tracks alerts on quality metric degradation. Suppresses
 * duplicate alerts and tracks SLA compliance for alert response times.
 *
 * Thread-safe for concurrent alert generation.
 */
class QualityAlertManager {
 public:
  /**
   * @brief Constructor
   */
  QualityAlertManager();
  ~QualityAlertManager();

  /**
   * @brief Report metric value and check alert triggers
   *
   * Called with updated metric value. Automatically determines alert level
   * based on thresholds and generates alerts as needed.
   *
   * @param metric_name (e.g., "recall_10", "ndcg_10", "mrr")
   * @param current_value Current metric value
   * @param warning_threshold Trigger warning alert if < this
   * @param critical_threshold Trigger critical alert if < this
   * @param model_version Model this metric is for
   * @return Generated alerts (may be empty if no threshold breach)
   */
  std::vector<Alert> ReportMetric(const std::string& metric_name, double current_value,
                                 double warning_threshold, double critical_threshold,
                                 uint32_t model_version = 0);

  /**
   * @brief Configure alert thresholds for a metric
   *
   * @param metric_name
   * @param warning_threshold Alert level for warning
   * @param critical_threshold Alert level for critical
   */
  void SetAlertThresholds(const std::string& metric_name, double warning_threshold,
                         double critical_threshold);

  /**
   * @brief Configure deduplication window
   *
   * Alerts with same metric_name within this time window are treated as duplicates.
   *
   * @param duration Deduplication window (default 5 minutes)
   */
  void SetDeduplicationWindow(std::chrono::system_clock::duration duration);

  /**
   * @brief Get recent alerts
   *
   * @param max_count Maximum alerts to return (most recent first)
   * @return List of recent alerts
   */
  std::vector<Alert> GetRecentAlerts(uint32_t max_count = 100);

  /**
   * @brief Get alerts for specific metric
   *
   * @param metric_name
   * @param time_range Look back this far in history
   * @return Alerts for this metric
   */
  std::vector<Alert> GetAlertsForMetric(const std::string& metric_name,
                                       std::chrono::system_clock::duration time_range);

  /**
   * @brief Get alert SLA status
   *
   * Tracks whether alerts are being responded to in time.
   *
   * @param severity
   * @return SLA status including response time performance
   */
  AlertSLA GetSLAStatus(AlertSeverity severity);

  /**
   * @brief Mark alert as resolved
   *
   * Called when alert is acknowledged/fixed.
   *
   * @param alert_id
   * @param resolution_note Description of resolution
   */
  void ResolveAlert(const std::string& alert_id, const std::string& resolution_note);

  /**
   * @brief Get alert history for trend analysis
   *
   * @param metric_name
   * @param days How many days to look back
   * @return Historical alert data
   */
  struct AlertTrend {
    std::string metric_name;
    uint32_t warning_count = 0;
    uint32_t critical_count = 0;
    double trend_slope = 0.0;  ///< Alert frequency trend (+ve = increasing)
    bool is_escalating = false;
  };
  AlertTrend GetAlertTrend(const std::string& metric_name, uint32_t days = 7);

  /**
   * @brief Clear alert history
   *
   * Useful for testing.
   */
  void Reset();

 private:
  struct Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace themis::rag::quality
