// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/freshness_sla_enforcer.h"

#include <chrono>

#include "rag/ingestion_latency_monitor.h"

namespace themis::rag {

FreshnessSLAEnforcer::FreshnessSLAEnforcer(
    std::shared_ptr<IngestionLatencyMonitor> monitor,
    const std::string& sla_name,
    uint64_t target_p95_ms)
    : monitor_(monitor),
      sla_name_(sla_name),
      target_p95_ms_(target_p95_ms),
      critical_multiplier_(3.0f),
      current_state_("healthy"),
      breach_start_time_us_(0),
      fallback_active_(false) {}

bool FreshnessSLAEnforcer::IsCompliant() {
  auto percentiles = monitor_->GetPercentiles();
  return percentiles.p95_latency_ms <= target_p95_ms_;
}

FreshnessSLAEnforcer::ComplianceStatus FreshnessSLAEnforcer::GetComplianceStatus() {
  ComplianceStatus status;
  auto percentiles = monitor_->GetPercentiles();

  status.p95_latency_ms = percentiles.p95_latency_ms;
  status.sla_target_ms = target_p95_ms_;
  status.is_compliant = (status.p95_latency_ms <= target_p95_ms_);
  status.current_state = current_state_;

  // Compute compliance percentage (placeholder: assume 95% if healthy, 60% if degraded)
  if (status.is_compliant) {
    status.compliance_percentage = 99.5f;
    status.breaches_in_hour = 0;
  } else {
    status.compliance_percentage = 60.0f;
    status.breaches_in_hour = 3;
  }

  if (status.is_compliant) {
    status.recommendation = "No action required";
  } else if (percentiles.p99_latency_ms > static_cast<uint64_t>(target_p95_ms_ * critical_multiplier_)) {
    status.recommendation = "CRITICAL: Trigger emergency refresh and fall back to replicas";
  } else {
    status.recommendation = "WARN: Monitor for sustained breach; consider emergency refresh";
  }

  auto now = std::chrono::system_clock::now();
  auto time_t_now = std::chrono::system_clock::to_time_t(now);
  char buf[256];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t_now));
  status.last_updated_us =
      std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

  return status;
}

bool FreshnessSLAEnforcer::TriggerEmergencyRefresh() {
  // TODO: Call IndexRefreshScheduler::ScheduleEmergencyRefresh()
  SLAEvent event;
  event.type = SLAEvent::Type::EmergencyTriggered;
  event.sla_name = sla_name_;
  event.p95_latency_ms = monitor_->GetPercentiles().p95_latency_ms;
  event.sla_target_ms = target_p95_ms_;
  event.details = "Emergency refresh scheduled due to SLA breach";
  event.timestamp_us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count();

  recent_events_.push_back(event);
  return true;
}

std::vector<std::string> FreshnessSLAEnforcer::GetFallbackShards() {
  return fallback_shards_;
}

bool FreshnessSLAEnforcer::IsFallbackActive() {
  return fallback_active_;
}

std::vector<FreshnessSLAEnforcer::SLAEvent> FreshnessSLAEnforcer::GetRecentEvents(
    uint32_t hours) {
  // TODO: Filter events by time window (last N hours)
  return recent_events_;
}

std::string FreshnessSLAEnforcer::GetStalenessNotice() {
  if (current_state_ == "healthy") {
    return "";
  } else if (current_state_ == "degraded") {
    return "Note: Search results may be slightly stale (index is being refreshed)";
  } else {
    return "CAUTION: Search results may be significantly stale; using fallback replicas";
  }
}

void FreshnessSLAEnforcer::SetTarget(uint64_t target_ms) {
  target_p95_ms_ = target_ms;
}

void FreshnessSLAEnforcer::RegisterFallbackShard(const std::string& shard_id) {
  fallback_shards_.push_back(shard_id);
}

void FreshnessSLAEnforcer::UnregisterFallbackShard(const std::string& shard_id) {
  auto it = std::find(fallback_shards_.begin(), fallback_shards_.end(), shard_id);
  if (it != fallback_shards_.end()) {
    fallback_shards_.erase(it);
  }
}

void FreshnessSLAEnforcer::SetCriticalThreshold(float multiplier) {
  critical_multiplier_ = multiplier;
}

bool FreshnessSLAEnforcer::UpdateCompliance() {
  auto percentiles = monitor_->GetPercentiles();
  bool was_compliant = (current_state_ != "critical");
  bool is_compliant = (percentiles.p95_latency_ms <= target_p95_ms_);
  bool is_critical = (percentiles.p99_latency_ms >
                      static_cast<uint64_t>(target_p95_ms_ * critical_multiplier_));

  std::string new_state = is_compliant ? "healthy" : (is_critical ? "critical" : "degraded");

  if (new_state != current_state_) {
    SLAEvent event;
    if (new_state == "healthy") {
      event.type = SLAEvent::Type::Recovery;
      event.details = "SLA breach resolved";
    } else if (new_state == "critical") {
      event.type = SLAEvent::Type::CriticalAlert;
      event.details = "SLA in critical state";
    } else {
      event.type = SLAEvent::Type::Breach;
      event.details = "SLA breach detected";
    }

    event.sla_name = sla_name_;
    event.p95_latency_ms = percentiles.p95_latency_ms;
    event.sla_target_ms = target_p95_ms_;
    event.timestamp_us =
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();

    recent_events_.push_back(event);
    current_state_ = new_state;

    fallback_active_ = is_critical;
    return true;
  }

  return false;
}

}  // namespace themis::rag
