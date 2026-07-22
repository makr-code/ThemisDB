/**
 * @file ssm_drift_metrics.h
 * @brief SSM drift metrics for Prometheus export (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Gap Summary: Drift telemetry and router decision tracking
 * @note Status: Phase 1 PoC metrics
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace themis::llm::metrics {

/**
 * @brief SSM state drift metrics registry (P1-D05).
 *
 * Tracks:
 * - Factual drift score (0.0–1.0): semantic coherence degradation
 * - SSM state checkpoints: total count and session-level accumulation
 * - Hybrid router decisions: architecture selection per request
 *
 * Integration: Prometheus scraping via existing `MetricsRegistry::instance()`
 * Format: `HELP` + `TYPE` + `samples` + `TIMESTAMP`
 */
class SSMDriftMetrics {
public:
    /// Access singleton instance
    static SSMDriftMetrics& instance();

    /// Increment factual drift score observation
    /// @param session_id Session identifier
    /// @param drift_value Score in [0.0, 1.0]
    void recordFactualDriftScore(const std::string& session_id,
                                  double drift_value);

    /// Increment SSM state checkpoint counter
    /// @param session_id Session identifier
    /// @param snapshot_size_bytes Checkpoint size for size histogram
    void recordSSMStateCheckpoint(const std::string& session_id,
                                   uint64_t snapshot_size_bytes);

    /// Record hybrid router architecture decision
    /// @param architecture_path Selected path ("transformer" / "infini" / "ssm")
    void recordHybridRouterDecision(const std::string& architecture_path);

    /// Get current factual drift score for session (exponential moving average)
    double getFactualDriftScore(const std::string& session_id) const;

    /// Get total checkpoint count
    uint64_t getTotalCheckpoints() const { return total_checkpoints_.load(); }

    /// Get router decision distribution as JSON
    std::string getRouterDecisionStats() const;

    /// Export metrics in Prometheus text format
    std::string exportPrometheus() const;

private:
    SSMDriftMetrics() = default;

    // Counters (atomic)
    std::atomic<uint64_t> total_checkpoints_{0};
    std::atomic<uint64_t> checkpoint_size_sum_{0};  // For average calculation

    // Router decisions (atomic for each path)
    std::atomic<uint64_t> router_transformer_count_{0};
    std::atomic<uint64_t> router_infini_count_{0};
    std::atomic<uint64_t> router_ssm_count_{0};

    // Drift tracking (simplified: global EMA)
    std::atomic<double> global_drift_ema_{0.0};
};

}  // namespace themis::llm::metrics

