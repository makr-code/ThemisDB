/**
 * @file prompt_engineering_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <atomic>
#include <functional>
#include <mutex>
#include <map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace prompt_engineering {

/**
 * @brief Prometheus metrics exporter for prompt engineering system
 * 
 * Exposes metrics in Prometheus text format for:
 * - Optimization operations
 * - A/B testing
 * - Performance tracking
 * - Feedback collection
 * - Version control operations
 *
 * Supports snapshot/restore for crash-safe persistence and threshold-based
 * alerting via pluggable callbacks.
 */
class PromptEngineeringMetrics {
public:
    struct Config {
        bool enabled = true;
        std::string namespace_prefix = "themis_prompt_engineering";
    };

    /**
     * @brief Configuration for threshold-based alerting
     */
    struct AlertConfig {
        double max_failure_rate = 0.5;     ///< Fire alert when failure_rate > threshold
        double min_success_rate = 0.5;     ///< Fire alert when success_rate < threshold
        int64_t max_hallucinations = 10;   ///< Fire alert when hallucination count exceeds this
    };

    /**
     * @brief Alert event passed to the alert callback
     */
    struct AlertEvent {
        std::string metric_name;   ///< Which metric triggered the alert
        double value;              ///< Current metric value
        double threshold;          ///< The threshold that was breached
        std::string message;       ///< Human-readable description
    };

    /// Signature of the alert callback
    using AlertCallback = std::function<void(const AlertEvent&)>;

    /**
     * @brief TBD: Describe PromptEngineeringMetrics.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PromptEngineeringMetrics(const Config& config);
    /**
     * @brief TBD: Describe PromptEngineeringMetrics.
     * @return Return value.
     */
    explicit PromptEngineeringMetrics();
    ~PromptEngineeringMetrics() = default;

    /**
     * @brief Optimization metrics
     * @param[in] prompt_id Input parameter.
     */
    void recordOptimizationAttempt(const std::string& prompt_id);
    /**
     * @brief TBD: Describe recordOptimizationSuccess.
     * @param[in] prompt_id Input parameter.
     * @param[in] improvement Input parameter.
     */
    void recordOptimizationSuccess(const std::string& prompt_id, double improvement);
    /**
     * @brief TBD: Describe recordOptimizationFailure.
     * @param[in] prompt_id Input parameter.
     * @param[in] reason Input parameter.
     */
    void recordOptimizationFailure(const std::string& prompt_id, const std::string& reason);
    /**
     * @brief TBD: Describe recordOptimizationDuration.
     * @param[in] prompt_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordOptimizationDuration(const std::string& prompt_id, double duration_ms);
    /**
     * @brief TBD: Describe recordOptimizationIterations.
     * @param[in] prompt_id Input parameter.
     * @param[in] iterations Input parameter.
     */
    void recordOptimizationIterations(const std::string& prompt_id, int iterations);

    /**
     * @brief A/B testing metrics
     * @param[in] test_id Input parameter.
     * @param[in] prompt_id Input parameter.
     */
    void recordABTestStart(const std::string& test_id, const std::string& prompt_id);
    /**
     * @brief TBD: Describe recordABTestObservation.
     * @param[in] test_id Input parameter.
     * @param[in] version Input parameter.
     * @param[in] success Input parameter.
     */
    void recordABTestObservation(const std::string& test_id, const std::string& version, bool success);
    /**
     * @brief TBD: Describe recordABTestCompletion.
     * @param[in] test_id Input parameter.
     * @param[in] winner Input parameter.
     * @param[in] confidence Input parameter.
     */
    void recordABTestCompletion(const std::string& test_id, const std::string& winner, double confidence);
    /**
     * @brief TBD: Describe recordABTestDuration.
     * @param[in] test_id Input parameter.
     * @param[in] duration_seconds Input parameter.
     */
    void recordABTestDuration(const std::string& test_id, double duration_seconds);
    /**
     * @brief TBD: Describe recordActiveABTests.
     * @param[in] count Input parameter.
     */
    void recordActiveABTests(int count);

    /**
     * @brief Performance tracking metrics
     * @param[in] prompt_id Input parameter.
     * @param[in] success Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPromptExecution(const std::string& prompt_id, bool success, double latency_ms);
    /**
     * @brief TBD: Describe recordPromptSuccessRate.
     * @param[in] prompt_id Input parameter.
     * @param[in] rate Input parameter.
     */
    void recordPromptSuccessRate(const std::string& prompt_id, double rate);
    /**
     * @brief TBD: Describe recordPromptAverageLatency.
     * @param[in] prompt_id Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordPromptAverageLatency(const std::string& prompt_id, double latency_ms);
    /**
     * @brief TBD: Describe recordPromptExecutionCount.
     * @param[in] prompt_id Input parameter.
     * @param[in] count Input parameter.
     */
    void recordPromptExecutionCount(const std::string& prompt_id, int64_t count);

    /**
     * @brief Feedback collection metrics
     * @param[in] prompt_id Input parameter.
     * @param[in] type Input parameter.
     */
    void recordFeedback(const std::string& prompt_id, const std::string& type);
    /**
     * @brief TBD: Describe recordFeedbackSeverity.
     * @param[in] prompt_id Input parameter.
     * @param[in] severity Input parameter.
     */
    void recordFeedbackSeverity(const std::string& prompt_id, double severity);
    /**
     * @brief TBD: Describe recordHallucinationDetection.
     * @param[in] prompt_id Input parameter.
     */
    void recordHallucinationDetection(const std::string& prompt_id);
    /**
     * @brief TBD: Describe recordFailedQuery.
     * @param[in] prompt_id Input parameter.
     * @param[in] failure_type Input parameter.
     */
    void recordFailedQuery(const std::string& prompt_id, const std::string& failure_type);
    /**
     * @brief TBD: Describe recordTotalFeedbackCount.
     * @param[in] prompt_id Input parameter.
     * @param[in] count Input parameter.
     */
    void recordTotalFeedbackCount(const std::string& prompt_id, int64_t count);
    /**
     * @brief TBD: Describe recordPositiveRatio.
     * @param[in] prompt_id Input parameter.
     * @param[in] ratio Input parameter.
     */
    void recordPositiveRatio(const std::string& prompt_id, double ratio);

    /**
     * @brief Version control metrics
     * @param[in] prompt_id Input parameter.
     * @param[in] branch Input parameter.
     */
    void recordVersionCommit(const std::string& prompt_id, const std::string& branch);
    /**
     * @brief TBD: Describe recordVersionRollback.
     * @param[in] prompt_id Input parameter.
     */
    void recordVersionRollback(const std::string& prompt_id);
    /**
     * @brief TBD: Describe recordBranchCreation.
     * @param[in] prompt_id Input parameter.
     */
    void recordBranchCreation(const std::string& prompt_id);
    /**
     * @brief TBD: Describe recordMergeOperation.
     * @param[in] prompt_id Input parameter.
     * @param[in] strategy Input parameter.
     * @param[in] success Input parameter.
     */
    void recordMergeOperation(const std::string& prompt_id, const std::string& strategy, bool success);
    /**
     * @brief TBD: Describe recordVersionCount.
     * @param[in] prompt_id Input parameter.
     * @param[in] count Input parameter.
     */
    void recordVersionCount(const std::string& prompt_id, int count);

    /**
     * @brief Integration metrics
     * @param[in] before Input parameter.
     */
    void recordIntegrationExecution(bool before);  // true=beforeExecution, false=afterExecution
    /**
     * @brief TBD: Describe recordBackgroundWorkerCycle.
     */
    void recordBackgroundWorkerCycle();
    /**
     * @brief TBD: Describe recordBackgroundWorkerDuration.
     * @param[in] duration_ms Input parameter.
     */
    void recordBackgroundWorkerDuration(double duration_ms);

    // -------------------------------------------------------------------------
    // Reflection Tuning metrics
    // -------------------------------------------------------------------------

    /**
     * @brief Record the start of one reflection cycle for @p prompt_id.
     *
     * Called by `PromptEngineeringIntegration` when an optional reflection
     * pass begins (i.e., `ReflectionTuner::tune()` is invoked).
     * @param[in] prompt_id Input parameter.
     */
    void recordReflectionCycleStart(const std::string& prompt_id);

    /**
     * @brief Record the completion of a reflection cycle.
     *
     * @param prompt_id   The prompt being improved.
     * @param iterations  Number of generate→critique→revise iterations run.
     * @param improved    `true` when `final_quality > initial_quality`.
     */
    void recordReflectionCycleComplete(const std::string& prompt_id,
                                        size_t iterations,
                                        bool   improved);

    /**
     * @brief Record that the `ReflectionHallucinationGuard` fired and halted
     *        the reflection cycle early.
     * @param[in] prompt_id Input parameter.
     */
    void recordReflectionGuardFired(const std::string& prompt_id);

    /**
     * @brief Record the net quality change produced by a reflection cycle.
     *
     * @p delta may be negative when the guard fires and the best-response
     * was the initial one.
     * @param[in] prompt_id Input parameter.
     * @param[in] delta Input parameter.
     */
    void recordReflectionQualityDelta(const std::string& prompt_id,
                                       double delta);

    /**
     * @brief Export all metrics in Prometheus text format
     * @return Metrics as string in Prometheus format
     */
    std::string exportMetrics() const;

    /**
     * @brief Reset all metrics (for testing)
     */
    void reset();

    // -------------------------------------------------------------------------
    // Persistence: snapshot / restore
    // -------------------------------------------------------------------------

    /**
     * @brief Serialize all current counter values to JSON
     *
     * Suitable for persisting to RocksDB, a file, or any key-value store so
     * metrics survive a process restart.
     *
     * @return JSON object with all metric counter values
     */
    nlohmann::json snapshotToJson() const;

    /**
     * @brief Restore counter values from a previously created snapshot
     *
     * Overwrites the current in-memory counters.  Unknown keys are ignored.
     *
     * @param snapshot JSON object produced by snapshotToJson()
     */
    void restoreFromJson(const nlohmann::json& snapshot);

    // -------------------------------------------------------------------------
    // Alerting
    // -------------------------------------------------------------------------

    /**
     * @brief Set threshold configuration for automatic alerting
     * @param cfg Alert thresholds
     */
    void setAlertConfig(const AlertConfig& cfg);

    /**
     * @brief Register a callback that is invoked whenever an alert threshold is breached
     *
     * The callback is called from whichever thread records the triggering metric,
     * so it must be safe to call from multiple threads (or use internal locking).
     *
     * @param cb Callback function
     */
    void setAlertCallback(AlertCallback cb);

private:
    Config config_;
    AlertConfig alert_config_;
    AlertCallback alert_callback_;  ///< May be null (no alerting)

    // Optimization counters
    std::atomic<int64_t> optimization_attempts_{0};
    std::atomic<int64_t> optimization_successes_{0};
    std::atomic<int64_t> optimization_failures_{0};
    std::atomic<double> optimization_total_duration_ms_{0.0};
    std::atomic<int64_t> optimization_total_iterations_{0};

    // A/B testing counters
    std::atomic<int64_t> ab_test_starts_{0};
    std::atomic<int64_t> ab_test_completions_{0};
    std::atomic<int64_t> ab_test_observations_{0};
    std::atomic<int> active_ab_tests_{0};

    // Performance tracking counters
    std::atomic<int64_t> prompt_executions_{0};
    std::atomic<int64_t> prompt_successes_{0};
    std::atomic<int64_t> prompt_failures_{0};
    std::atomic<double> prompt_total_latency_ms_{0.0};

    // Feedback counters
    std::atomic<int64_t> feedback_total_{0};
    std::atomic<int64_t> feedback_positive_{0};
    std::atomic<int64_t> feedback_negative_{0};
    std::atomic<int64_t> hallucination_detections_{0};
    std::atomic<int64_t> failed_queries_{0};

    // Version control counters
    std::atomic<int64_t> version_commits_{0};
    std::atomic<int64_t> version_rollbacks_{0};
    std::atomic<int64_t> branch_creations_{0};
    std::atomic<int64_t> merge_operations_{0};
    std::atomic<int64_t> merge_successes_{0};

    // Integration counters
    std::atomic<int64_t> integration_before_calls_{0};
    std::atomic<int64_t> integration_after_calls_{0};
    std::atomic<int64_t> background_worker_cycles_{0};
    std::atomic<double> background_worker_total_duration_ms_{0.0};

    // Reflection Tuning counters
    std::atomic<int64_t> reflection_cycle_starts_{0};
    std::atomic<int64_t> reflection_cycle_completions_{0};
    std::atomic<int64_t> reflection_iterations_total_{0};
    std::atomic<int64_t> reflection_improvements_{0};
    std::atomic<int64_t> reflection_guard_fires_{0};
    std::atomic<double>  reflection_quality_delta_sum_{0.0};

    // Per-prompt metrics (protected by mutex)
    mutable std::mutex metrics_mutex_;
    std::map<std::string, double> prompt_success_rates_;
    std::map<std::string, double> prompt_avg_latencies_;
    std::map<std::string, int64_t> prompt_execution_counts_;
    std::map<std::string, int> prompt_version_counts_;

    // Helper methods
    std::string formatMetric(
        const std::string& name,
        const std::string& help,
        const std::string& type,
        double value,
        const std::map<std::string, std::string>& labels = {}) const;

    std::string formatMetric(
      const std::string& name,
      const std::string& help,
      const std::string& type,
      int64_t value,
      const std::map<std::string, std::string>& labels = {}) const;

    std::string formatMetric(
      const std::string& name,
      const std::string& help,
      const std::string& type,
      int value,
      const std::map<std::string, std::string>& labels = {}) const;

    std::string formatLabels(const std::map<std::string, std::string>& labels) const;
};

} // namespace prompt_engineering
} // namespace themis

