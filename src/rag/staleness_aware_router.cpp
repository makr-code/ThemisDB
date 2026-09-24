// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/staleness_aware_router.h"

#include "rag/ingestion_latency_monitor.h"

namespace themis::rag {

StalenessAwareRouter::StalenessAwareRouter(
    std::shared_ptr<IngestionLatencyMonitor> monitor)
    : monitor_(monitor),
      staleness_target_ms_(300000),      // 5 min
      staleness_degraded_ms_(450000),    // 1.5x
      staleness_critical_ms_(600000),    // 2x
      last_staleness_ms_(0) {}

StalenessAwareRouter::RoutingDecision StalenessAwareRouter::Route(
    const std::string& query_text,
    const std::vector<std::string>& available_shards) {
  RoutingDecision decision;

  // Get current percentiles
  auto percentiles = monitor_->GetPercentiles();
  last_staleness_ms_ = percentiles.p95_latency_ms;

  decision.max_staleness_ms = last_staleness_ms_;

  if (last_staleness_ms_ <= staleness_target_ms_) {
    // Healthy: route to primary only
    decision.primary_shards = available_shards;
    decision.fallback_shards = {};
    decision.use_fallback = false;
    decision.freshness_confidence = 1.0f;
    decision.rationale = "Primary healthy (staleness < target)";
  } else if (last_staleness_ms_ <= staleness_critical_ms_) {
    // Degraded: route to primary with quality penalty
    decision.primary_shards = available_shards;
    decision.fallback_shards = {};
    decision.use_fallback = false;
    decision.freshness_confidence =
        1.0f - (last_staleness_ms_ - staleness_target_ms_) /
                   (staleness_critical_ms_ - staleness_target_ms_) * 0.3f;
    decision.rationale = "Primary degraded (staleness > target, < 2x)";
  } else {
    // Critical: fall back to replicas
    decision.primary_shards = {};
    decision.fallback_shards = available_shards;
    decision.use_fallback = true;
    decision.freshness_confidence = 0.7f;
    decision.rationale = "Primary critical (staleness > 2x target), using fallback";
  }

  return decision;
}

StalenessAwareRouter::RoutingDecision StalenessAwareRouter::RouteWithFallback(
    const std::string& query_text,
    const std::vector<std::string>& primary_shards,
    const std::vector<std::string>& fallback_shards) {
  RoutingDecision decision;

  auto percentiles = monitor_->GetPercentiles();
  last_staleness_ms_ = percentiles.p95_latency_ms;

  decision.max_staleness_ms = last_staleness_ms_;

  if (last_staleness_ms_ <= staleness_target_ms_) {
    decision.primary_shards = primary_shards;
    decision.fallback_shards = {};
    decision.use_fallback = false;
    decision.freshness_confidence = 1.0f;
  } else if (last_staleness_ms_ <= staleness_critical_ms_) {
    decision.primary_shards = primary_shards;
    decision.fallback_shards = {};
    decision.use_fallback = false;
    decision.freshness_confidence = 0.8f;
  } else {
    decision.primary_shards = {};
    decision.fallback_shards = fallback_shards.empty() ? primary_shards : fallback_shards;
    decision.use_fallback = true;
    decision.freshness_confidence = 0.7f;
  }

  return decision;
}

StalenessAwareRouter::StalenessMetadata
StalenessAwareRouter::GetStalenessMetadata() {
  StalenessMetadata metadata;
  metadata.max_staleness_ms = last_staleness_ms_;
  metadata.is_degraded = last_staleness_ms_ > staleness_target_ms_;

  if (!metadata.is_degraded) {
    metadata.freshness_note = "Index is fresh";
  } else if (last_staleness_ms_ <= staleness_critical_ms_) {
    metadata.freshness_note = "Index is slightly stale (degraded mode)";
  } else {
    metadata.freshness_note = "Index is significantly stale (using fallback replicas)";
  }

  return metadata;
}

bool StalenessAwareRouter::IsPrimaryHealthy() {
  auto percentiles = monitor_->GetPercentiles();
  return percentiles.p95_latency_ms <= staleness_target_ms_;
}

bool StalenessAwareRouter::IsPrimaryDegraded() {
  auto percentiles = monitor_->GetPercentiles();
  return percentiles.p95_latency_ms > staleness_target_ms_ &&
         percentiles.p95_latency_ms <= staleness_critical_ms_;
}

bool StalenessAwareRouter::IsPrimaryCritical() {
  auto percentiles = monitor_->GetPercentiles();
  return percentiles.p95_latency_ms > staleness_critical_ms_;
}

void StalenessAwareRouter::SetStalenessThresholds(
    uint64_t target_ms,
    uint64_t degraded_threshold_ms,
    uint64_t critical_threshold_ms) {
  staleness_target_ms_ = target_ms;
  staleness_degraded_ms_ = degraded_threshold_ms;
  staleness_critical_ms_ = critical_threshold_ms;
}

}  // namespace themis::rag
