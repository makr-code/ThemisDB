/**
 * @file ssm_drift_metrics.h
 * @brief SSM drift metrics for Prometheus export (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Status: Phase 1 PoC metrics
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <string>

namespace themis::llm::metrics {

class SSMDriftMetrics {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static SSMDriftMetrics& instance();

    /**
     * @brief Record Factual Drift Score.
     * @param[in] session_id Identifier of the session.
     * @param[in] drift_value Input parameter.
     */
    void recordFactualDriftScore(const std::string& session_id,
                                  double drift_value);

    /**
     * @brief Record SSMState Checkpoint.
     * @param[in] session_id Identifier of the session.
     * @param[in] snapshot_size_bytes Input parameter.
     */
    void recordSSMStateCheckpoint(const std::string& session_id,
                                   uint64_t snapshot_size_bytes);

    /**
     * @brief Record Hybrid Router Decision.
     * @param[in] architecture_path Path to the architecture.
     */
    void recordHybridRouterDecision(const std::string& architecture_path);

    /**
     * @brief Get Factual Drift Score.
     * @param[in] session_id Identifier of the session.
     * @return Return value.
     */
    double getFactualDriftScore(const std::string& session_id) const;

    uint64_t getTotalCheckpoints() const { return total_checkpoints_.load(); }

    /**
     * @brief Get Router Decision Stats.
     * @return Return value.
     */
    std::string getRouterDecisionStats() const;

    /**
     * @brief Export Prometheus.
     * @return Return value.
     */
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

// Inline implementations
namespace themis::llm::metrics {

/**
 * @brief Instance.
 * @return Return value.
 * @details Implements instance without additional internal calls.
 */
inline SSMDriftMetrics& SSMDriftMetrics::instance() {
    static SSMDriftMetrics inst;
    return inst;
}

/**
 * @brief Record Factual Drift Score.
 * @param[in] param Input parameter.
 * @param[in] drift_value Input parameter.
 * @details Calls: load(), store().
 */
inline void SSMDriftMetrics::recordFactualDriftScore(const std::string& /*session_id*/, double drift_value) {
    // simple EMA: alpha = 0.1
    double old = global_drift_ema_.load();
    double next = old * 0.9 + drift_value * 0.1;
    global_drift_ema_.store(next);
}

/**
 * @brief Record SSMState Checkpoint.
 * @param[in] param Input parameter.
 * @param[in] snapshot_size_bytes Input parameter.
 * @details Calls: fetch_add().
 */
inline void SSMDriftMetrics::recordSSMStateCheckpoint(const std::string& /*session_id*/, uint64_t snapshot_size_bytes) {
    total_checkpoints_.fetch_add(1);
    checkpoint_size_sum_.fetch_add(snapshot_size_bytes);
}

/**
 * @brief Record Hybrid Router Decision.
 * @param[in] architecture_path Path to the architecture.
 * @details Calls: fetch_add().
 */
inline void SSMDriftMetrics::recordHybridRouterDecision(const std::string& architecture_path) {
    if (architecture_path == "transformer") {
      router_transformer_count_.fetch_add(1);
    }
    else if (architecture_path == "infini") router_infini_count_.fetch_add(1);
    else if (architecture_path == "ssm") router_ssm_count_.fetch_add(1);
}

inline double SSMDriftMetrics::getFactualDriftScore(const std::string& /*session_id*/) const {
    return global_drift_ema_.load();
}

inline std::string SSMDriftMetrics::getRouterDecisionStats() const {
    return std::string("{\"router_transformer\":") + std::to_string(router_transformer_count_.load()) +
           ",\"router_infini\": " + std::to_string(router_infini_count_.load()) +
           ",\"router_ssm\": " + std::to_string(router_ssm_count_.load()) + "}";
}

inline std::string SSMDriftMetrics::exportPrometheus() const {
    std::string out = {};
    out += "# HELP themis_factual_drift_score Global factual drift EMA\n";
    out += "themis_factual_drift_score " + std::to_string(global_drift_ema_.load()) + "\n";
    out += "# HELP themis_ssm_state_checkpoints_total Total SSM checkpoints\n";
    out += "themis_ssm_state_checkpoints_total " + std::to_string(total_checkpoints_.load()) + "\n";
    out += "# HELP themis_hybrid_router_decision_total Router decision counts\n";
    out += "themis_hybrid_router_decision_total{path=\"transformer\"} " + std::to_string(router_transformer_count_.load()) + "\n";
    out += "themis_hybrid_router_decision_total{path=\"infini\"} " + std::to_string(router_infini_count_.load()) + "\n";
    out += "themis_hybrid_router_decision_total{path=\"ssm\"} " + std::to_string(router_ssm_count_.load()) + "\n";
    return out;
}

} // namespace themis::llm::metrics

