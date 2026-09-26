// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include "rag/realtime_slo_tracker.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>

namespace themis::rag {

RealtimeSLOTracker::RealtimeSLOTracker()
    : current_state_(ComplianceState::Healthy),
      rerank_success_count_(0),
      total_queries_(0),
      last_state_change_us_(0) {}

void RealtimeSLOTracker::SetTarget(
    const std::string& metric_name,
    uint64_t threshold,
    float percentage) {
  SLOTarget target;
  target.metric_name = metric_name;
  target.threshold_value = threshold;
  target.percentage_threshold = percentage;
  target.window_seconds = 300;  // 5 min default

  targets_[metric_name] = target;
}

void RealtimeSLOTracker::RecordQuery(
    uint64_t latency_ms,
    bool rerank_success,
    float ndcg_score,
    float cost_usd) {
  latency_samples_.push_back(latency_ms);
  if (rerank_success) {
    rerank_success_count_++;
  }
  ndcg_samples_.push_back(ndcg_score);
  cost_samples_.push_back(cost_usd);
  total_queries_++;
}

bool RealtimeSLOTracker::IsCompliant() {
  return current_state_ == ComplianceState::Healthy;
}

RealtimeSLOTracker::ComplianceState RealtimeSLOTracker::GetComplianceState() {
  return current_state_;
}

RealtimeSLOTracker::ComplianceSnapshot RealtimeSLOTracker::GetComplianceSnapshot() {
  ComplianceSnapshot snapshot;
  snapshot.window_start_us =
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count() -
      300LL * 1000000LL;
  snapshot.window_end_us = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::system_clock::now().time_since_epoch())
      .count();

  // Compute compliance percentages for each metric
  if (!latency_samples_.empty()) {
    std::vector<uint64_t> sorted_latencies = latency_samples_;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());
    uint64_t p95_idx = (sorted_latencies.size() * 95) / 100;
    uint64_t p95_latency = sorted_latencies[std::min(p95_idx, sorted_latencies.size() - 1)];

    // Find target for p95_latency_ms
    auto it = targets_.find("p95_latency_ms");
    if (it != targets_.end()) {
      float compliance_pct =
          p95_latency <= it->second.threshold_value ? 100.0f : 50.0f;
      snapshot.metric_compliance["p95_latency_ms"] = compliance_pct;
    }
  }

  if (total_queries_ > 0) {
    float rerank_success_rate = (100.0f * rerank_success_count_) / total_queries_;
    snapshot.metric_compliance["rerank_success_rate"] = rerank_success_rate;
  }

  if (!ndcg_samples_.empty()) {
    float mean_ndcg = std::accumulate(ndcg_samples_.begin(), ndcg_samples_.end(), 0.0f) /
                      ndcg_samples_.size();
    auto it = targets_.find("ndcg_10_min");
    if (it != targets_.end()) {
      float compliance_pct = mean_ndcg >= 0.50f ? 100.0f : 50.0f;
      snapshot.metric_compliance["ndcg_10"] = compliance_pct;
    }
  }

  if (!cost_samples_.empty()) {
    float mean_cost = std::accumulate(cost_samples_.begin(), cost_samples_.end(), 0.0f) /
                      cost_samples_.size();
    auto it = targets_.find("cost_per_query_usd_max");
    if (it != targets_.end()) {
      float compliance_pct = mean_cost <= static_cast<float>(it->second.threshold_value) ? 100.0f : 50.0f;
      snapshot.metric_compliance["cost_per_query_usd"] = compliance_pct;
    }
  }

  snapshot.state = (current_state_ == ComplianceState::Healthy) ? "healthy" : "breach";
  snapshot.consecutive_breaches = (current_state_ == ComplianceState::Breach) ? 1 : 0;
  snapshot.consecutive_healthy = (current_state_ == ComplianceState::Healthy) ? 1 : 0;
  snapshot.average_recovery_time_sec = 60.0f;

  return snapshot;
}

std::vector<RealtimeSLOTracker::ComplianceEvent> RealtimeSLOTracker::GetRecentEvents(
    uint32_t hours) {
  return recent_events_;
}

bool RealtimeSLOTracker::UpdateCompliance() {
  auto old_state = current_state_;

  // Determine new state based on current metrics
  bool all_compliant = true;
  for (const auto& [metric, target] : targets_) {
    // Simplified: assume compliant if we have recent data
    if (metric.find("latency") != std::string::npos && !latency_samples_.empty()) {
      std::vector<uint64_t> sorted_latencies = latency_samples_;
      std::sort(sorted_latencies.begin(), sorted_latencies.end());
      uint64_t p95_idx = (sorted_latencies.size() * 95) / 100;
      uint64_t p95_latency = sorted_latencies[std::min(p95_idx, sorted_latencies.size() - 1)];

      if (p95_latency > target.threshold_value) {
        all_compliant = false;
        break;
      }
    }
  }

  ComplianceState new_state = all_compliant ? ComplianceState::Healthy : ComplianceState::Breach;

  if (new_state != old_state) {
    current_state_ = new_state;

    ComplianceEvent event;
    event.trigger_metric = "combined";
    event.old_state = old_state;
    event.new_state = new_state;
    event.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch())
        .count();
    event.details = (new_state == ComplianceState::Healthy) ? "Recovered to healthy state"
                                                              : "Entered breach state";

    recent_events_.push_back(event);
    last_state_change_us_ = event.timestamp_us;

    return true;
  }

  return false;
}

float RealtimeSLOTracker::GetHealthScore() {
  // Composite score 0-100 based on all metrics
  float score = 100.0f;

  // Latency impact
  if (!latency_samples_.empty()) {
    std::vector<uint64_t> sorted_latencies = latency_samples_;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());
    uint64_t p95_idx = (sorted_latencies.size() * 95) / 100;
    uint64_t p95_latency = sorted_latencies[std::min(p95_idx, sorted_latencies.size() - 1)];

    auto it = targets_.find("p95_latency_ms");
    if (it != targets_.end()) {
      if (p95_latency > it->second.threshold_value * 2) {
        score -= 50.0f;
      } else if (p95_latency > it->second.threshold_value) {
        score -= 20.0f;
      }
    }
  }

  // Rerank success rate
  if (total_queries_ > 0) {
    float success_rate = static_cast<float>(rerank_success_count_) / total_queries_;
    if (success_rate < 0.7f) {
      score -= 15.0f;
    }
  }

  // NDCG
  if (!ndcg_samples_.empty()) {
    float mean_ndcg = std::accumulate(ndcg_samples_.begin(), ndcg_samples_.end(), 0.0f) /
                      ndcg_samples_.size();
    if (mean_ndcg < 0.50f) {
      score -= 20.0f;
    } else if (mean_ndcg < 0.60f) {
      score -= 10.0f;
    }
  }

  return std::max(0.0f, score);
}

std::map<std::string, uint64_t> RealtimeSLOTracker::GetCurrentMetrics() {
  std::map<std::string, uint64_t> metrics;

  if (!latency_samples_.empty()) {
    std::vector<uint64_t> sorted_latencies = latency_samples_;
    std::sort(sorted_latencies.begin(), sorted_latencies.end());
    uint64_t p50_idx = (sorted_latencies.size() * 50) / 100;
    uint64_t p95_idx = (sorted_latencies.size() * 95) / 100;
    uint64_t p99_idx = (sorted_latencies.size() * 99) / 100;

    metrics["p50_latency_ms"] = sorted_latencies[std::min(p50_idx, sorted_latencies.size() - 1)];
    metrics["p95_latency_ms"] = sorted_latencies[std::min(p95_idx, sorted_latencies.size() - 1)];
    metrics["p99_latency_ms"] = sorted_latencies[std::min(p99_idx, sorted_latencies.size() - 1)];
  }

  metrics["total_queries"] = total_queries_;
  metrics["rerank_successes"] = rerank_success_count_;

  if (!cost_samples_.empty()) {
    float mean_cost = std::accumulate(cost_samples_.begin(), cost_samples_.end(), 0.0f) /
                      cost_samples_.size();
    metrics["avg_cost_usd_millicents"] = static_cast<uint64_t>(mean_cost * 100000);
  }

  return metrics;
}

}  // namespace themis::rag
