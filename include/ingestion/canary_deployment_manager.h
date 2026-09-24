/**
 * @file canary_deployment_manager.h
 * @brief Manages staged rollout of embedding version upgrades
 *
 * Orchestrates a 5-phase canary deployment:
 * - Phase 1 (Canary 5%): 5% of traffic, pass/fail metrics collection
 * - Phase 2 (Canary 10%): 10% of traffic, continue metrics collection
 * - Phase 3 (Canary 25%): 25% of traffic, aggregate with previous phases
 * - Phase 4 (Canary 50%): 50% of traffic, final metrics before production
 * - Phase 5 (Production 100%): 100% of traffic, full rollout
 *
 * Each phase progression requires:
 * 1. Metric delta check: recall@10, nDCG@10, MRR@10 all within ±2pp
 * 2. Query execution time SLA: p99 latency ≤ baseline + 100ms
 * 3. No increase in error rate (must stay ≤ baseline)
 *
 * @date 2026-09-24
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace themis::ingestion {

/**
 * @enum CanaryPhase
 * @brief Staged rollout phases
 */
enum class CanaryPhase {
  Canary5 = 0,    ///< 5% traffic
  Canary10 = 1,   ///< 10% traffic
  Canary25 = 2,   ///< 25% traffic
  Canary50 = 3,   ///< 50% traffic
  Production100 = 4  ///< 100% traffic (full rollout)
};

/**
 * @struct CanaryMetrics
 * @brief Aggregated metrics for a canary phase
 */
struct CanaryMetrics {
  CanaryPhase phase;
  double recall_at_10{0.0};
  double ndcg_at_10{0.0};
  double mrr_at_10{0.0};
  double p99_latency_ms{0.0};
  double error_rate{0.0};
  size_t query_count{0};
  std::string phase_name;

  /**
   * @brief Check if metrics are within acceptable range
   * @param baseline Previous phase metrics
   * @param metric_tolerance_pp Tolerance in percentage points (default 2pp)
   * @param latency_tolerance_ms Tolerance for latency in ms (default 100ms)
   * @return true if all metrics pass, false if regression detected
   */
  bool PassesQualityGate(const CanaryMetrics& baseline, double metric_tolerance_pp = 2.0,
                         double latency_tolerance_ms = 100.0) const;
};

/**
 * @class CanaryDeploymentManager
 * @brief Orchestrates staged canary rollout for embedding version upgrades
 *
 * Usage:
 * ```cpp
 * CanaryDeploymentManager manager(reindex_version, index_path);
 * manager.StartCanary(CanaryPhase::Canary5);
 *
 * // Collect metrics during phase
 * manager.RecordQuery(query, latency_ms, success);
 *
 * // Check if ready to progress
 * if (manager.CanProgressToNextPhase()) {
 *   manager.ProgressToNextPhase();
 * } else {
 *   manager.RollbackToPreviousVersion();
 * }
 * ```
 */
class CanaryDeploymentManager {
 public:
  /**
   * @brief Create a canary deployment manager
   * @param reindex_version Version number being deployed (for tracking)
   * @param index_path Path to index being upgraded
   */
  CanaryDeploymentManager(uint32_t reindex_version, const std::string& index_path);

  ~CanaryDeploymentManager();

  // Delete copy operations
  CanaryDeploymentManager(const CanaryDeploymentManager&) = delete;
  CanaryDeploymentManager& operator=(const CanaryDeploymentManager&) = delete;

  /**
   * @brief Start a canary deployment phase
   * @param phase Target phase (5%, 10%, 25%, 50%, or 100%)
   * @param baseline_metrics Baseline metrics from previous phase (for comparison)
   */
  void StartPhase(CanaryPhase phase, const CanaryMetrics& baseline_metrics);

  /**
   * @brief Record a query execution result
   * @param query_id Unique query identifier
   * @param latency_ms Query execution time in milliseconds
   * @param success True if query succeeded, false if failed
   * @param recall_at_10 Retrieved recall@10 for this query (if known)
   */
  void RecordQuery(const std::string& query_id, double latency_ms, bool success,
                   double recall_at_10 = 0.0);

  /**
   * @brief Get current phase metrics
   * @return Aggregated metrics for current phase
   */
  CanaryMetrics GetPhaseMetrics() const;

  /**
   * @brief Check if phase metrics meet quality gates
   * @return true if all metrics pass, false if regression detected
   */
  bool CheckQualityGate();

  /**
   * @brief Check if ready to progress to next phase
   *
   * Returns true if:
   * 1. Quality gate passed (metrics within tolerance)
   * 2. Minimum query count collected (e.g., 1000 queries)
   * 3. Phase duration completed (e.g., 1 hour per phase)
   *
   * @return true if all progression criteria met
   */
  bool CanProgressToNextPhase() const;

  /**
   * @brief Progress to next phase
   *
   * Transitions from current phase to next (e.g., 5% → 10%).
   * Updates traffic allocation and resets phase metrics.
   *
   * @throws std::logic_error if already at production phase or progression not ready
   */
  void ProgressToNextPhase();

  /**
   * @brief Rollback to previous version
   *
   * Restores previous index version and emits OTLP event.
   *
   * @return true if rollback succeeded, false otherwise
   */
  bool RollbackToPreviousVersion();

  /**
   * @brief Get current phase
   * @return Active canary phase
   */
  CanaryPhase GetCurrentPhase() const { return current_phase_; }

  /**
   * @brief Get phase start time
   * @return Timestamp when current phase started
   */
  std::chrono::system_clock::time_point GetPhaseStartTime() const {
    return phase_start_time_;
  }

  /**
   * @brief Get traffic percentage for current phase
   * @return Traffic allocation (5, 10, 25, 50, or 100)
   */
  int GetTrafficPercentage() const;

  /**
   * @brief Get all phase metrics history
   * @return Vector of CanaryMetrics for all completed phases
   */
  std::vector<CanaryMetrics> GetPhaseHistory() const { return phase_history_; }

  /**
   * @brief Emit OTLP event for phase progression or rollback
   * @param event_type Type of event (e.g., "canary_progress", "canary_rollback")
   * @param details Event details (JSON string)
   */
  void EmitOTLPEvent(const std::string& event_type, const std::string& details);

 private:
  uint32_t reindex_version_;
  std::string index_path_;
  CanaryPhase current_phase_{CanaryPhase::Canary5};
  CanaryMetrics baseline_metrics_;
  std::vector<CanaryMetrics> phase_history_;
  std::chrono::system_clock::time_point phase_start_time_;

  // Phase metrics collection
  struct QueryRecord {
    std::string query_id;
    double latency_ms;
    bool success;
    double recall_at_10;
    std::chrono::system_clock::time_point timestamp;
  };
  std::vector<QueryRecord> current_phase_queries_;

  /**
   * @brief Compute aggregated metrics from query records
   * @param records Queries recorded for this phase
   * @return Aggregated CanaryMetrics
   */
  CanaryMetrics ComputePhaseMetrics(const std::vector<QueryRecord>& records) const;

  /**
   * @brief Get minimum query count required for phase
   * @param phase Target phase
   * @return Minimum queries needed before progression allowed
   */
  static size_t GetMinQueryCountForPhase(CanaryPhase phase);

  /**
   * @brief Get minimum phase duration
   * @param phase Target phase
   * @return Minimum duration in seconds before progression allowed
   */
  static std::chrono::seconds GetMinPhaseDuration(CanaryPhase phase);
};

}  // namespace themis::ingestion
