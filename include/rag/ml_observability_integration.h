/**
 * @file ml_observability_integration.h
 * @brief Integration layer between ContinuousLearningOrchestrator and metrics collector.
 *
 * Provides automatic metrics export and trace ID propagation for all ML learning paths.
 * Can be attached to a ContinuousLearningOrchestrator instance to enable full observability.
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */

#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/concerns/i_context.h"
#include "core/concerns/i_logger.h"
#include "core/concerns/i_metrics.h"
#include "learning_metrics.h"
#include "ml_learning_metrics_collector.h"

namespace themis::rag::learning {

// Forward declaration
class ContinuousLearningOrchestrator;

/**
 * @brief ML Observability Integration
 *
 * Provides hooks and callback mechanisms to integrate the MLLearningMetricsCollector
 * with the ContinuousLearningOrchestrator. Enables automatic metrics export,
 * logging with trace IDs, and error/warning state tracking.
 *
 * Usage:
 * @code
 * auto integration = std::make_shared<MLObservabilityIntegration>();
 * integration->initialize(metrics, logger);
 * integration->attachToOrchestrator(orchestrator);
 * @endcode
 */
class MLObservabilityIntegration {
public:
    /**
     * @brief Create a new observability integration instance.
     */
    MLObservabilityIntegration();

    /**
     * @brief Initialize integration with metrics and logging backends.
     *
     * @param metrics Prometheus/StatsD metrics collector
     * @param logger Structured logger for trace-ID correlation
     */
    void initialize(std::shared_ptr<core::concerns::IMetrics> metrics,
                   std::shared_ptr<core::concerns::ILogger> logger);

    /**
     * @brief Attach integration to a ContinuousLearningOrchestrator instance.
     *
     * Registers callbacks to track:
     * - Learning loop state transitions
     * - Loop execution durations
     * - Adapter deployments
     * - Model performance metrics
     * - A/B test state changes
     * - Warning/error states
     *
     * @param orchestrator The orchestrator to attach to
     */
    void attachToOrchestrator(
        std::shared_ptr<ContinuousLearningOrchestrator> orchestrator);

    /**
     * @brief Record a loop state transition with automatic trace context.
     *
     * @param loop_id Loop identifier
     * @param state New state
     */
    void recordLoopStateTransition(const std::string& loop_id,
                                  const std::string& state);

    /**
     * @brief Record loop execution result with metrics.
     *
     * @param loop_id Loop identifier
     * @param duration_ms Execution duration
     * @param success Whether execution succeeded
     */
    void recordLoopExecution(const std::string& loop_id,
                            double duration_ms,
                            bool success);

    /**
     * @brief Record adapter deployment event.
     *
     * @param adapter_id Adapter identifier
     * @param version Version string
     * @param status Deployment status
     */
    void recordAdapterDeployment(const std::string& adapter_id,
                                const std::string& version,
                                const std::string& status);

    /**
     * @brief Periodically export orchestrator statistics as metrics.
     *
     * Should be called at regular intervals (e.g., every 10 seconds)
     * to export the current LearningStats snapshot to Prometheus.
     */
    void exportOrchestrationMetrics();

    /**
     * @brief Record a warning state that warrants monitoring/alerting.
     *
     * @param warning_type Type of warning
     * @param component Component generating warning
     * @param message Human-readable message
     */
    void recordWarning(const std::string& warning_type,
                      const std::string& component,
                      const std::string& message);

    /**
     * @brief Record an error state that requires immediate attention.
     *
     * @param error_type Type of error
     * @param component Component generating error
     * @param message Human-readable message
     */
    void recordError(const std::string& error_type,
                    const std::string& component,
                    const std::string& message);

private:
    std::shared_ptr<MLLearningMetricsCollector> collector_;
    std::shared_ptr<ContinuousLearningOrchestrator> attached_orchestrator_;

    // Helper to get or create trace context
    LearningTraceContext getTraceContext();
};

}  // namespace themis::rag::learning
