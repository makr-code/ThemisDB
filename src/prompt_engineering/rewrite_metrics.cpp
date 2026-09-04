/**
 * @file rewrite_metrics.cpp
 * @brief Observability and metrics support for rewrite engine (Phase 2 delivery).
 * @version 1.0.0
 * @note Maturity: 🟡 IMPL/PHASE2
 * @note Status: Phase 2 observability (Q4 2026)
 *
 * Provides:
 * - Rule execution counters
 * - Latency tracking per rule and phase
 * - JSON stats export
 * - Structured logging integration
 *
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "rewrite_engine.h"
#include <chrono>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

/**
 * @class RewriteMetrics
 * @brief Standalone metrics collector for rewrite operations (optional utility).
 *
 * Can be used to supplement the built-in stats in RewriteEngine.
 */
class RewriteMetrics {
public:
    struct PerPhaseMetrics {
        uint64_t rules_evaluated = 0;
        uint64_t rules_applied = 0;
        uint64_t total_latency_micros = 0;
        uint64_t max_latency_micros = 0;
        uint64_t min_latency_micros = UINT64_MAX;
    };

    struct PerRuleMetrics {
        uint64_t match_count = 0;
        uint64_t apply_count = 0;
        uint64_t total_latency_micros = 0;
        uint64_t max_latency_micros = 0;
        uint64_t error_count = 0;
    };

    RewriteMetrics() = default;

    /**
     * @brief Record a rule match event.
     */
    void record_rule_match(const std::string& rule_id, RewritePhase phase) {
        phase_metrics_[static_cast<int>(phase)].rules_evaluated++;
        rule_metrics_[rule_id].match_count++;
    }

    /**
     * @brief Record a rule application event with latency.
     */
    void record_rule_apply(const std::string& rule_id, RewritePhase phase, uint64_t latency_micros) {
        auto& phase_metric = phase_metrics_[static_cast<int>(phase)];
        phase_metric.rules_applied++;
        phase_metric.total_latency_micros += latency_micros;
        phase_metric.max_latency_micros = std::max(phase_metric.max_latency_micros, latency_micros);
        phase_metric.min_latency_micros = std::min(phase_metric.min_latency_micros, latency_micros);

        auto& rule_metric = rule_metrics_[rule_id];
        rule_metric.apply_count++;
        rule_metric.total_latency_micros += latency_micros;
        rule_metric.max_latency_micros = std::max(rule_metric.max_latency_micros, latency_micros);
    }

    /**
     * @brief Record a rule error.
     */
    void record_rule_error(const std::string& rule_id) {
        rule_metrics_[rule_id].error_count++;
    }

    /**
     * @brief Get metrics for a specific phase.
     */
    PerPhaseMetrics get_phase_metrics(RewritePhase phase) const {
        int idx = static_cast<int>(phase);
        if (phase_metrics_.count(idx)) {
            return phase_metrics_.at(idx);
        }
        return PerPhaseMetrics{};
    }

    /**
     * @brief Get metrics for a specific rule.
     */
    PerRuleMetrics get_rule_metrics(const std::string& rule_id) const {
        if (rule_metrics_.count(rule_id)) {
            return rule_metrics_.at(rule_id);
        }
        return PerRuleMetrics{};
    }

    /**
     * @brief Export all metrics as JSON.
     */
    std::string export_json() const {
        nlohmann::json metrics_obj;

        // Phase metrics
        nlohmann::json phases_obj;
        for (int i = 1; i <= 4; ++i) {
            RewritePhase phase = static_cast<RewritePhase>(i);
            auto metrics = get_phase_metrics(phase);

            std::string phase_name = {};
            switch (phase) {
                case RewritePhase::PHASE_1_INPUT_NORMALIZATION: phase_name = "phase_1_input_normalization"; break;
                case RewritePhase::PHASE_2_POLICY_ENFORCEMENT: phase_name = "phase_2_policy_enforcement"; break;
                case RewritePhase::PHASE_3_NL_AQL_PREPROCESSING: phase_name = "phase_3_nl_aql_preprocessing"; break;
                case RewritePhase::PHASE_4_POST_GENERATION: phase_name = "phase_4_post_generation"; break;
                default: phase_name = "unknown"; break;
            }

            phases_obj[phase_name] = {
                {"rules_evaluated", metrics.rules_evaluated},
                {"rules_applied", metrics.rules_applied},
                {"total_latency_micros", metrics.total_latency_micros},
                {"avg_latency_micros", metrics.rules_applied > 0 ? metrics.total_latency_micros / metrics.rules_applied : 0},
                {"max_latency_micros", metrics.max_latency_micros},
                {"min_latency_micros", metrics.min_latency_micros == UINT64_MAX ? 0 : metrics.min_latency_micros}
            };
        }

        // Rule metrics
        nlohmann::json rules_obj;
        for (const auto& [rule_id, metrics] : rule_metrics_) {
            rules_obj[rule_id] = {
                {"match_count", metrics.match_count},
                {"apply_count", metrics.apply_count},
                {"error_count", metrics.error_count},
                {"total_latency_micros", metrics.total_latency_micros},
                {"avg_latency_micros", metrics.apply_count > 0 ? metrics.total_latency_micros / metrics.apply_count : 0},
                {"max_latency_micros", metrics.max_latency_micros}
            };
        }

        metrics_obj["phases"] = phases_obj;
        metrics_obj["rules"] = rules_obj;
        metrics_obj["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();

        return metrics_obj.dump(2);
    }

    /**
     * @brief Reset all collected metrics.
     */
    void reset() {
        phase_metrics_.clear();
        rule_metrics_.clear();
    }

private:
    std::unordered_map<int, PerPhaseMetrics> phase_metrics_;
    std::unordered_map<std::string, PerRuleMetrics> rule_metrics_;
};

// Global metrics instance (optional singleton pattern)
static RewriteMetrics g_metrics;

// Utility functions for external access
void record_rewrite_rule_match(const std::string& rule_id, RewritePhase phase) {
    g_metrics.record_rule_match(rule_id, phase);
}

void record_rewrite_rule_apply(const std::string& rule_id, RewritePhase phase, uint64_t latency_micros) {
    g_metrics.record_rule_apply(rule_id, phase, latency_micros);
}

void record_rewrite_rule_error(const std::string& rule_id) {
    g_metrics.record_rule_error(rule_id);
}

std::string export_rewrite_metrics_json() {
    return g_metrics.export_json();
}

void reset_rewrite_metrics() {
    g_metrics.reset();
}

} // namespace prompt_engineering
} // namespace themis
