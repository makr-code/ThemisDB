// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis::rag {

/// @brief Real-time SLO compliance tracking with health metrics.
///
/// Monitors compliance windows (5-min, 1-hour, daily) against configurable
/// thresholds and tracks compliance percentage, breach count, and recovery
/// time. Emits compliance state changes as events for alerting.
///
/// @details
/// Compliance metrics tracked:
/// - p95 latency: staleness percentile
/// - query throughput: QPS
/// - rerank success rate: % queries reranked successfully
/// - retrieval recall: NDCG@K
/// - cost per query: average $/query
///
/// Windows:
/// - 5-min: Immediate breach detection
/// - 1-hour: Performance trending
/// - Daily: Policy enforcement
///
/// States:
/// - "healthy": All metrics within targets
/// - "warning": One metric approaching threshold
/// - "breach": One or more metrics out of compliance
/// - "critical": Multiple metrics out of compliance or recovery failure
///
/// @code
/// auto tracker = std::make_unique<RealtimeSLOTracker>();
/// tracker->SetTarget("p95_latency_ms", 300000);  // 5 min
/// tracker->SetTarget("query_throughput_qps", 1000);
/// tracker->RecordQuery(query_latency_ms, rerank_success, ndcg_score);
/// if (!tracker->IsCompliant()) {
///   scheduler->TriggerEmergencyRefresh();
/// }
/// @endcode
class RealtimeSLOTracker {
 public:
  /// @brief Compliance state.
  enum class ComplianceState {
    Healthy,   ///< All metrics within SLA
    Warning,   ///< One metric approaching threshold
    Breach,    ///< One or more metrics out of compliance
    Critical   ///< Multiple breaches or severe
  };

  /// @brief SLO target definition.
  struct SLOTarget {
    std::string metric_name;
    uint64_t threshold_value;      ///< Target value (unit depends on metric)
    float percentage_threshold;    ///< % compliance required (default 99.5)
    uint32_t window_seconds;       ///< Measurement window (default 300)
  };

  /// @brief Per-window compliance snapshot.
  struct ComplianceSnapshot {
    uint64_t window_start_us;
    uint64_t window_end_us;
    std::string state;                              ///< "healthy" | "warning" | "breach" | "critical"
    std::map<std::string, float> metric_compliance; ///< metric_name → compliance_percentage
    std::map<std::string, uint64_t> breach_counts;  ///< metric_name → # breaches
    uint32_t consecutive_breaches;
    uint32_t consecutive_healthy;
    float average_recovery_time_sec;
  };

  /// @brief Compliance event.
  struct ComplianceEvent {
    enum class Type {
      StateChange,        ///< health state changed
      MetricBreach,       ///< single metric out of compliance
      RecoveryStarted,    ///< recovering from breach
      RecoveryFailed      ///< unable to recover after timeout
    };

    Type type;
    std::string trigger_metric;
    ComplianceState old_state;
    ComplianceState new_state;
    std::string details;
    int64_t timestamp_us;
  };

  /// @brief Constructor.
  RealtimeSLOTracker();

  /// @brief Set SLO target for metric.
  /// @param metric_name Metric identifier (e.g., "p95_latency_ms").
  /// @param threshold Threshold value (must be within reasonable range).
  /// @param percentage % of time metric must be in compliance (default 99.5).
  void SetTarget(
      const std::string& metric_name,
      uint64_t threshold,
      float percentage = 99.5f);

  /// @brief Record query execution metrics.
  /// @param latency_ms Query latency in milliseconds.
  /// @param rerank_success true if reranking succeeded.
  /// @param ndcg_score Retrieved NDCG score.
  /// @param cost_usd Query cost.
  void RecordQuery(
      uint64_t latency_ms,
      bool rerank_success,
      float ndcg_score,
      float cost_usd);

  /// @brief Check if currently compliant.
  /// @return true if all metrics within targets.
  bool IsCompliant();

  /// @brief Get current compliance state.
  /// @return State enum.
  ComplianceState GetComplianceState();

  /// @brief Get compliance snapshot (5-min window).
  /// @return Snapshot of current compliance.
  ComplianceSnapshot GetComplianceSnapshot();

  /// @brief Get recent compliance events.
  /// @param hours Hours to look back.
  /// @return List of events.
  std::vector<ComplianceEvent> GetRecentEvents(uint32_t hours);

  /// @brief Update compliance check (call periodically).
  /// @return true if state changed.
  bool UpdateCompliance();

  /// @brief Get health score [0, 100].
  /// @return Composite health metric.
  float GetHealthScore();

  /// @brief Get detailed metrics for dashboard.
  /// @return Map of metric_name → current_value.
  std::map<std::string, uint64_t> GetCurrentMetrics();

 private:
  std::map<std::string, SLOTarget> targets_;
  std::vector<uint64_t> latency_samples_;
  uint32_t rerank_success_count_;
  uint32_t total_queries_;
  std::vector<float> ndcg_samples_;
  std::vector<float> cost_samples_;
  ComplianceState current_state_;
  std::vector<ComplianceEvent> recent_events_;
  int64_t last_state_change_us_;
};

}  // namespace themis::rag
