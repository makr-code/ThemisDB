/**
 * @file ml_learning_metrics_collector.h
 * @brief ML & Continuous Learning metrics exporter for observability.
 *
 * Provides comprehensive metrics export for ML learning paths including:
 * - Learning loop states and transitions
 * - Adapter versions and deployment status
 * - Optimizer states and retraining progress
 * - Error and warning states for alerting
 * - Trace ID propagation for distributed tracing
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */

#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/concerns/i_context.h"
#include "core/concerns/i_logger.h"
#include "core/concerns/i_metrics.h"
#include "learning_metrics.h"

namespace themis::rag::learning {

/**
 * @brief Trace context for a learning operation
 */
struct LearningTraceContext {
    std::string trace_id;    ///< W3C Trace Context trace ID
    std::string span_id;     ///< W3C Trace Context span ID
    std::string request_id;  ///< Request ID for correlation
};

/**
 * @brief ML Learning Metrics Collector
 *
 * Exports all continuous learning metrics to Prometheus and integrates
 * with the distributed tracing system. Provides observability into:
 * - Loop 1-4 states (HNSW query optimization, workload drift, feedback)
 * - Adapter deployment and versioning
 * - A/B test results and rollback events
 * - Error rates and warning states
 *
 * Usage:
 * @code
 * auto collector = MLLearningMetricsCollector::create();
 * collector->recordLoopStateTransition("LOOP_1", "ACTIVE", trace_ctx);
 * collector->recordAdapterVersion("adapter_v1", "deployed", trace_ctx);
 * collector->recordRetrainingProgress("adapter_v1", 0.75, trace_ctx);
 * @endcode
 */
class MLLearningMetricsCollector {
public:
    /**
     * @brief Create or get singleton instance of metrics collector.
     * @return Shared pointer to collector instance.
     */
    static std::shared_ptr<MLLearningMetricsCollector> getInstance();

    /**
     * @brief Initialize metrics collector with dependencies.
     * @param metrics Metrics backend (Prometheus, StatsD, etc.)
     * @param logger Logger for trace-ID correlation
     */
    void initialize(std::shared_ptr<core::concerns::IMetrics> metrics,
                    std::shared_ptr<core::concerns::ILogger> logger);

    // -----------------------------------------------------------------------
    // Learning Loop Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Record a learning loop state transition.
     *
     * Transitions tracked:
     * - LOOP_1: HNSW query optimization (miss-rate feedback)
     * - LOOP_2: Workload drift detection (query pattern analysis)
     * - LOOP_3: Prompt optimization (success-rate analysis)
     * - LOOP_4: LoRA retraining (feedback accumulation)
     *
     * @param loop_id Loop identifier (e.g., "LOOP_1")
     * @param state New loop state (IDLE, RUNNING, COMPLETED, FAILED)
     * @param ctx Trace context for correlation
     */
    void recordLoopStateTransition(const std::string& loop_id,
                                  const std::string& state,
                                  const LearningTraceContext& ctx);

    /**
     * @brief Record loop execution metrics.
     *
     * @param loop_id Loop identifier
     * @param duration_ms Execution time in milliseconds
     * @param success Whether execution succeeded
     * @param ctx Trace context
     */
    void recordLoopExecution(const std::string& loop_id,
                            double duration_ms,
                            bool success,
                            const LearningTraceContext& ctx);

    // -----------------------------------------------------------------------
    // Adapter & Model Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Record adapter version deployment event.
     *
     * @param adapter_id Adapter identifier
     * @param version Version string (e.g., "v1", "rlaif_v2")
     * @param status Deployment status (active, staged, rolled_back)
     * @param ctx Trace context
     */
    void recordAdapterVersion(const std::string& adapter_id,
                             const std::string& version,
                             const std::string& status,
                             const LearningTraceContext& ctx);

    /**
     * @brief Record retraining progress.
     *
     * @param adapter_id Adapter being retrained
     * @param progress Progress fraction [0.0, 1.0]
     * @param ctx Trace context
     */
    void recordRetrainingProgress(const std::string& adapter_id,
                                 double progress,
                                 const LearningTraceContext& ctx);

    /**
     * @brief Record model performance metrics.
     *
     * @param adapter_id Adapter identifier
     * @param accuracy Model accuracy [0.0, 1.0]
     * @param latency_ms Inference latency in ms
     * @param ctx Trace context
     */
    void recordModelPerformance(const std::string& adapter_id,
                               double accuracy,
                               double latency_ms,
                               const LearningTraceContext& ctx);

    // -----------------------------------------------------------------------
    // A/B Test Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Record A/B test state change.
     *
     * @param test_id Test identifier
     * @param status Test status (running, completed, promoted, rolled_back)
     * @param treatment_improvement Observed improvement (treatment vs control)
     * @param ctx Trace context
     */
    void recordABTestState(const std::string& test_id,
                          const std::string& status,
                          double treatment_improvement,
                          const LearningTraceContext& ctx);

    // -----------------------------------------------------------------------
    // Error & Warning States
    // -----------------------------------------------------------------------

    /**
     * @brief Record a warning state.
     *
     * Warning states indicate potential issues that should trigger alerts:
     * - Circuit breaker opened
     * - High error rate detected
     * - Performance degradation
     * - Missing signal provider
     *
     * @param warning_type Type of warning
     * @param component Component that issued warning
     * @param message Warning message
     * @param ctx Trace context
     */
    void recordWarningState(const std::string& warning_type,
                           const std::string& component,
                           const std::string& message,
                           const LearningTraceContext& ctx);

    /**
     * @brief Record an error state.
     *
     * Error states indicate critical failures that require immediate attention:
     * - Provider unavailable in production mode
     * - Retraining failed
     * - Data corruption detected
     * - System deadlock
     *
     * @param error_type Type of error
     * @param component Component that issued error
     * @param message Error message
     * @param ctx Trace context
     */
    void recordErrorState(const std::string& error_type,
                         const std::string& component,
                         const std::string& message,
                         const LearningTraceContext& ctx);

    // -----------------------------------------------------------------------
    // Feedback Loop Observability
    // -----------------------------------------------------------------------

    /**
     * @brief Record feedback collection metrics.
     *
     * @param adapter_id Adapter receiving feedback
     * @param feedback_count Total feedback samples collected
     * @param positive_fraction Fraction of positive feedback [0.0, 1.0]
     * @param ctx Trace context
     */
    void recordFeedbackCollection(const std::string& adapter_id,
                                 size_t feedback_count,
                                 double positive_fraction,
                                 const LearningTraceContext& ctx);

    // -----------------------------------------------------------------------
    // Batch Operations
    // -----------------------------------------------------------------------

    /**
     * @brief Update all learning metrics from LearningStats snapshot.
     *
     * Exports all statistics from the orchestrator as Prometheus metrics.
     * Should be called periodically or after major state changes.
     *
     * @param stats Current learning statistics
     * @param ctx Trace context
     */
    void updateFromSnapshot(const LearningStats& stats,
                           const LearningTraceContext& ctx);

    /**
     * @brief Get current trace context with generated IDs if needed.
     *
     * @return Trace context with trace_id, span_id, and request_id set.
     */
    static LearningTraceContext createTraceContext();

    /**
     * @brief Get trace context from IContext if available.
     *
     * @param ctx Generic context object
     * @return Learning trace context extracted from ctx
     */
    static LearningTraceContext extractTraceContext(
        const core::concerns::IContext& ctx);

private:
    // Singleton instance
    static std::shared_ptr<MLLearningMetricsCollector> instance_;
    static std::mutex instance_mutex_;

    // Dependencies
    std::shared_ptr<core::concerns::IMetrics> metrics_;
    std::shared_ptr<core::concerns::ILogger> logger_;

    // State tracking
    std::mutex state_mutex_;
    std::unordered_map<std::string, std::string> loop_states_;           // loop_id -> state
    std::unordered_map<std::string, std::string> adapter_versions_;      // adapter_id -> version
    std::unordered_map<std::string, double> adapter_accuracies_;         // adapter_id -> accuracy
    std::unordered_map<std::string, std::chrono::system_clock::time_point>
        loop_last_execution_;  // loop_id -> last_execution_time

    // Private constructor
    MLLearningMetricsCollector() = default;

    // Helper methods
    void logWithTrace(const std::string& level,
                     const std::string& message,
                     const LearningTraceContext& ctx);

    void exportLoopMetrics();
    void exportAdapterMetrics();
    void exportErrorMetrics();
};

}  // namespace themis::rag::learning
