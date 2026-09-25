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

// Forward declarations
class IngestionLatencyMonitor;

/// @brief Freshness SLA enforcement with compliance tracking and fallback.
///
/// Monitors SLA compliance across time windows and triggers fallback
/// strategies (emergency refresh, degraded routing, user notification)
/// when breaches occur. Computes compliance metrics and alerts on trends.
///
/// @details
/// SLA definition:
/// - Target: p95 staleness < 5 minutes (configurable)
/// - Breach: p95 staleness > target for >60 seconds
/// - Critical: p99 staleness > 3x target
///
/// Compliance windows:
/// - 5-minute: Immediate breach detection (for alerts)
/// - 1-hour: Performance trend (for adjustments)
/// - Daily: Policy enforcement (for billing/SLAs)
///
/// Fallback strategies:
/// 1. No breach: Normal routing (primary only)
/// 2. Degraded (p95 > 1.5x): Primary + quality penalty
/// 3. Breach (p95 > target, sustained): Trigger emergency refresh
/// 4. Critical (p99 > 3x): Fall back to replicas, notify ops
///
/// @code
/// auto enforcer = std::make_unique<FreshnessSLAEnforcer>(monitor, "default");
/// enforcer->SetTarget(300000);  // 5 min in ms
/// if (!enforcer->IsCompliant()) {
///   enforcer->TriggerEmergencyRefresh();
/// }
/// @endcode
class FreshnessSLAEnforcer {
 public:
  /// @brief SLA compliance status.
  struct ComplianceStatus {
    bool is_compliant;              ///< true if within SLA
    std::string sla_name;           ///< SLA identifier for this compliance snapshot
    uint64_t p95_latency_ms;        ///< Current p95 staleness
    uint64_t sla_target_ms;         ///< Target p95 staleness
    float compliance_percentage;    ///< % of time compliant (1-hour window)
    uint32_t breaches_in_hour;      ///< Number of breaches (1-hour window)
    std::string current_state;      ///< "healthy" | "degraded" | "critical"
    std::string recommendation;     ///< Suggested action
    int64_t last_updated_us;        ///< UTC microseconds
  };

  /// @brief SLA event (breach, recovery, etc).
  struct SLAEvent {
    enum class Type {
      Breach,        ///< Entered breach state
      Recovery,      ///< Exited breach state
      CriticalAlert, ///< Entered critical state (p99 > 3x)
      EmergencyTriggered
    };

    Type type;
    std::string sla_name;
    uint64_t p95_latency_ms;
    uint64_t sla_target_ms;
    std::string details;
    int64_t timestamp_us;
  };

  /// @brief Constructor.
  /// @param monitor Shared latency monitor.
  /// @param sla_name SLA identifier (e.g., "default", "premium").
  /// @param target_p95_ms Target p95 staleness in milliseconds.
  FreshnessSLAEnforcer(
      std::shared_ptr<IngestionLatencyMonitor> monitor,
      const std::string& sla_name,
      uint64_t target_p95_ms = 300000);  // 5 min default

  /// @brief Check if currently compliant.
  /// @return true if p95 ≤ target.
  bool IsCompliant();

  /// @brief Get detailed compliance status.
  /// @return Compliance snapshot.
  ComplianceStatus GetComplianceStatus();

  /// @brief Trigger emergency refresh on breach.
  /// @return true if emergency refresh scheduled.
  bool TriggerEmergencyRefresh();

  /// @brief Fallback to replica routing.
  /// @return List of fallback shards (replicas).
  std::vector<std::string> GetFallbackShards();

  /// @brief Check if fallback is active.
  /// @return true if primary is too stale, use replicas.
  bool IsFallbackActive();

  /// @brief Get recent SLA events.
  /// @param hours Hours to look back.
  /// @return List of events (recent first).
  std::vector<SLAEvent> GetRecentEvents(uint32_t hours);

  /// @brief Notify user of staleness (attach to response).
  /// @return Message to include in response metadata (empty if healthy).
  std::string GetStalenessNotice();

  /// @brief Set SLA target.
  /// @param target_ms Target p95 staleness (ms).
  void SetTarget(uint64_t target_ms);

  /// @brief Register fallback shard (replica).
  /// @param shard_id Replica shard ID.
  void RegisterFallbackShard(const std::string& shard_id);

  /// @brief Unregister fallback shard.
  /// @param shard_id Replica to remove.
  void UnregisterFallbackShard(const std::string& shard_id);

  /// @brief Set critical threshold (p99 > N * target).
  /// @param multiplier Threshold multiplier (default 3.0).
  void SetCriticalThreshold(float multiplier);

  /// @brief Process compliance check (call periodically).
  /// @return true if any change in state.
  bool UpdateCompliance();

 private:
  std::shared_ptr<IngestionLatencyMonitor> monitor_;
  std::string sla_name_;
  uint64_t target_p95_ms_;
  float critical_multiplier_;
  std::vector<std::string> fallback_shards_;
  std::string current_state_;  // "healthy" | "degraded" | "critical"
  std::vector<SLAEvent> recent_events_;
  int64_t breach_start_time_us_;
  bool fallback_active_;
};

}  // namespace themis::rag
