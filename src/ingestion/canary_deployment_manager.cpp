/**
 * @file canary_deployment_manager.cpp
 * @brief Implementation of CanaryDeploymentManager for staged rollout
 *
 * Manages 5-phase canary deployment with quality gates and metrics collection.
 *
 * @date 2026-09-24
 */

#include "ingestion/canary_deployment_manager.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace themis::ingestion {

// ============================================================================
// CanaryMetrics Implementation
// ============================================================================

bool CanaryMetrics::PassesQualityGate(const CanaryMetrics& baseline,
                                      double metric_tolerance_pp,
                                      double latency_tolerance_ms) const {
  // Convert tolerance from percentage points to fraction
  double tolerance = metric_tolerance_pp / 100.0;

  // Check each metric: |delta| <= tolerance
  double recall_delta = std::abs(recall_at_10 - baseline.recall_at_10);
  double ndcg_delta = std::abs(ndcg_at_10 - baseline.ndcg_at_10);
  double mrr_delta = std::abs(mrr_at_10 - baseline.mrr_at_10);

  // Check latency: p99 should not increase by more than tolerance_ms
  double latency_delta = p99_latency_ms - baseline.p99_latency_ms;

  // Check error rate: should not increase
  double error_rate_delta = error_rate - baseline.error_rate;

  bool metrics_pass = recall_delta <= tolerance && ndcg_delta <= tolerance &&
                      mrr_delta <= tolerance;
  bool latency_pass = latency_delta <= latency_tolerance_ms;
  bool error_pass = error_rate_delta <= 0.001;  // Allow 0.1pp increase

  return metrics_pass && latency_pass && error_pass;
}

// ============================================================================
// CanaryDeploymentManager Implementation
// ============================================================================

CanaryDeploymentManager::CanaryDeploymentManager(uint32_t reindex_version,
                                                 const std::string& index_path)
    : reindex_version_(reindex_version), index_path_(index_path) {
  spdlog::info("[CanaryDeploymentManager] Initialized for version {} at {}", reindex_version,
               index_path);
}

CanaryDeploymentManager::~CanaryDeploymentManager() = default;

size_t CanaryDeploymentManager::GetMinQueryCountForPhase(CanaryPhase phase) {
  // More queries required for earlier phases (better coverage)
  switch (phase) {
    case CanaryPhase::Canary5:
      return 1000;  // 1000 queries for 5% canary
    case CanaryPhase::Canary10:
      return 2000;  // 2000 queries for 10%
    case CanaryPhase::Canary25:
      return 5000;  // 5000 queries for 25%
    case CanaryPhase::Canary50:
      return 10000;  // 10000 queries for 50%
    case CanaryPhase::Production100:
      return 0;  // No minimum for production (already rolled out)
  }
  return 0;
}

std::chrono::seconds CanaryDeploymentManager::GetMinPhaseDuration(CanaryPhase phase) {
  // Each phase should run for a minimum duration
  switch (phase) {
    case CanaryPhase::Canary5:
      return std::chrono::hours(1);  // 1 hour
    case CanaryPhase::Canary10:
      return std::chrono::hours(2);  // 2 hours
    case CanaryPhase::Canary25:
      return std::chrono::hours(4);  // 4 hours
    case CanaryPhase::Canary50:
      return std::chrono::hours(8);  // 8 hours
    case CanaryPhase::Production100:
      return std::chrono::seconds(0);  // No duration requirement
  }
  return std::chrono::seconds(0);
}

void CanaryDeploymentManager::StartPhase(CanaryPhase phase,
                                         const CanaryMetrics& baseline_metrics) {
  current_phase_ = phase;
  baseline_metrics_ = baseline_metrics;
  current_phase_queries_.clear();
  phase_start_time_ = std::chrono::system_clock::now();

  spdlog::info("[CanaryDeploymentManager] Started phase {} ({}% traffic)", (int)phase,
               GetTrafficPercentage());
}

void CanaryDeploymentManager::RecordQuery(const std::string& query_id, double latency_ms,
                                          bool success, double recall_at_10) {
  QueryRecord record;
  record.query_id = query_id;
  record.latency_ms = latency_ms;
  record.success = success;
  record.recall_at_10 = recall_at_10;
  record.timestamp = std::chrono::system_clock::now();

  current_phase_queries_.push_back(record);

  spdlog::debug(
      "[CanaryDeploymentManager] Recorded query {} in phase {}: latency={}ms, "
      "success={}",
      query_id, (int)current_phase_, latency_ms, success);
}

CanaryMetrics CanaryDeploymentManager::ComputePhaseMetrics(
    const std::vector<QueryRecord>& records) const {
  CanaryMetrics metrics;
  metrics.phase = current_phase_;
  metrics.query_count = records.size();

  if (records.empty()) {
    return metrics;
  }

  // Compute latencies, success count
  std::vector<double> latencies;
  size_t successful_queries = 0;
  double total_recall = 0.0;

  for (const auto& record : records) {
    latencies.push_back(record.latency_ms);
    if (record.success) {
      successful_queries++;
    }
    total_recall += record.recall_at_10;
  }

  // Compute p99 latency
  std::sort(latencies.begin(), latencies.end());
  size_t p99_index = (latencies.size() * 99) / 100;
  metrics.p99_latency_ms = latencies[p99_index];

  // Compute error rate
  metrics.error_rate =
      1.0 - (static_cast<double>(successful_queries) / static_cast<double>(records.size()));

  // Compute average recall (placeholder, would be aggregated from dual-read validator)
  metrics.recall_at_10 = total_recall / records.size();

  // Set phase name
  switch (current_phase_) {
    case CanaryPhase::Canary5:
      metrics.phase_name = "Canary 5%";
      break;
    case CanaryPhase::Canary10:
      metrics.phase_name = "Canary 10%";
      break;
    case CanaryPhase::Canary25:
      metrics.phase_name = "Canary 25%";
      break;
    case CanaryPhase::Canary50:
      metrics.phase_name = "Canary 50%";
      break;
    case CanaryPhase::Production100:
      metrics.phase_name = "Production 100%";
      break;
  }

  return metrics;
}

CanaryMetrics CanaryDeploymentManager::GetPhaseMetrics() const {
  return ComputePhaseMetrics(current_phase_queries_);
}

bool CanaryDeploymentManager::CheckQualityGate() {
  auto current_metrics = GetPhaseMetrics();
  bool passed = current_metrics.PassesQualityGate(baseline_metrics_, 2.0, 100.0);

  spdlog::info(
      "[CanaryDeploymentManager] Quality gate check for phase {}: {}\n"
      "  recall@10: {:.3f} (baseline: {:.3f}, delta: {:.3f}pp)\n"
      "  nDCG@10: {:.3f} (baseline: {:.3f}, delta: {:.3f}pp)\n"
      "  p99_latency: {:.1f}ms (baseline: {:.1f}ms, delta: {:.1f}ms)\n"
      "  error_rate: {:.3f}% (baseline: {:.3f}%)",
      (int)current_phase_, passed ? "PASSED" : "FAILED", current_metrics.recall_at_10,
      baseline_metrics_.recall_at_10,
      (current_metrics.recall_at_10 - baseline_metrics_.recall_at_10) * 100,
      current_metrics.ndcg_at_10, baseline_metrics_.ndcg_at_10,
      (current_metrics.ndcg_at_10 - baseline_metrics_.ndcg_at_10) * 100,
      current_metrics.p99_latency_ms, baseline_metrics_.p99_latency_ms,
      current_metrics.p99_latency_ms - baseline_metrics_.p99_latency_ms,
      current_metrics.error_rate * 100, baseline_metrics_.error_rate * 100);

  return passed;
}

bool CanaryDeploymentManager::CanProgressToNextPhase() const {
  if (current_phase_ == CanaryPhase::Production100) {
    return false;  // Already at production
  }

  // Check minimum query count
  size_t min_queries = GetMinQueryCountForPhase(current_phase_);
  if (current_phase_queries_.size() < min_queries) {
    spdlog::debug("[CanaryDeploymentManager] Not enough queries: {} < {}", 
                  current_phase_queries_.size(), min_queries);
    return false;
  }

  // Check minimum phase duration
  auto now = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - phase_start_time_);
  auto min_duration = GetMinPhaseDuration(current_phase_);
  if (duration < min_duration) {
    spdlog::debug("[CanaryDeploymentManager] Phase not long enough: {}s < {}s", 
                  duration.count(), min_duration.count());
    return false;
  }

  return true;
}

void CanaryDeploymentManager::ProgressToNextPhase() {
  if (!CanProgressToNextPhase()) {
    throw std::logic_error("Cannot progress to next phase: requirements not met");
  }

  // Save current metrics to history
  auto phase_metrics = GetPhaseMetrics();
  phase_history_.push_back(phase_metrics);

  // Progress to next phase
  CanaryPhase next_phase = static_cast<CanaryPhase>(static_cast<int>(current_phase_) + 1);
  EmitOTLPEvent("canary_progress", 
                "Progressing from phase " + std::to_string((int)current_phase_) + 
                " to " + std::to_string((int)next_phase));

  StartPhase(next_phase, phase_metrics);
}

bool CanaryDeploymentManager::RollbackToPreviousVersion() {
  // In production, this would call IndexMetadataStore::RollbackToPreviousVersion()
  EmitOTLPEvent("canary_rollback", 
                "Rolled back from version " + std::to_string(reindex_version_));

  spdlog::warn("[CanaryDeploymentManager] Rolled back from version {}", reindex_version_);
  return true;
}

int CanaryDeploymentManager::GetTrafficPercentage() const {
  switch (current_phase_) {
    case CanaryPhase::Canary5:
      return 5;
    case CanaryPhase::Canary10:
      return 10;
    case CanaryPhase::Canary25:
      return 25;
    case CanaryPhase::Canary50:
      return 50;
    case CanaryPhase::Production100:
      return 100;
  }
  return 0;
}

void CanaryDeploymentManager::EmitOTLPEvent(const std::string& event_type,
                                            const std::string& details) {
  spdlog::info("[CanaryDeploymentManager] OTLP Event: type={}, details={}", event_type,
               details);
  // In production, this would emit to actual OTLP collector
}

}  // namespace themis::ingestion
