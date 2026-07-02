/**
 * @file example_ml_observability.cpp
 * @brief Example of how to attach ML observability to ContinuousLearningOrchestrator
 *
 * This example demonstrates how to use the MLLearningMetricsCollector and
 * MLObservabilityIntegration to add comprehensive observability to the
 * continuous learning orchestrator, enabling monitoring of all learning paths.
 */

#include <iostream>
#include <memory>
#include <thread>

#include "core/concerns/prometheus_metrics_adapter.h"
#include "rag/continuous_learning_orchestrator.h"
#include "rag/ml_learning_metrics_collector.h"
#include "rag/ml_observability_integration.h"
#include "utils/logger.h"

namespace themis::rag::learning {

/**
 * @brief Example: Setup ML Observability for Continuous Learning
 *
 * This example shows:
 * 1. Creating a ContinuousLearningOrchestrator
 * 2. Initializing the ML observability stack
 * 3. Recording learning operations with automatic metrics export
 * 4. Exporting metrics to Prometheus
 */
void example_ml_observability() {
    using namespace themis::core::concerns;

    // Step 1: Initialize metrics backend (Prometheus)
    auto metrics = std::make_shared<PrometheusMetricsAdapter>();

    // Step 2: Initialize logger with structured logging support
    auto logger = themis::utils::Logger::getInstance();

    // Step 3: Create ContinuousLearningOrchestrator
    ContinuousLearningConfig config;
    config.min_feedback_samples = 100;
    config.min_accuracy_drop = 0.05;
    config.retraining_interval = std::chrono::hours(24);
    config.enable_ab_testing = true;

    auto orchestrator = std::make_shared<ContinuousLearningOrchestrator>(config);

    // Step 4: Initialize ML observability
    auto integration = std::make_shared<MLObservabilityIntegration>();
    integration->initialize(metrics, logger);
    integration->attachToOrchestrator(orchestrator);

    // Step 5: Start the orchestrator
    orchestrator->startLearningLoop();

    // Step 6: Record a learning operation with automatic trace context
    integration->recordLoopStateTransition("LOOP_1", "ACTIVE");

    // Step 7: Record loop execution
    integration->recordLoopExecution("LOOP_1", 75.5, true);

    // Step 8: Record adapter deployment
    integration->recordAdapterDeployment("adapter_v2", "2.0", "active");

    // Step 9: Periodically export metrics
    // In a real application, this would be called by a timer or monitoring task
    integration->exportOrchestrationMetrics();

    // Step 10: Record warnings/errors
    // Note: In production, these would be triggered by actual errors
    integration->recordWarning("circuit_breaker_open", "LOOP_2", "Too many failures");
    integration->recordError("provider_unavailable", "LOOP_1", "BaoOptimizer not wired");

    // Clean up
    orchestrator->stopLearningLoop();

    std::cout << "ML Observability Example Completed Successfully\n";
    std::cout << "\nMetrics exported to Prometheus at http://localhost:9091/metrics\n";
    std::cout << "Logs are correlated with Trace IDs for distributed tracing\n";
}

}  // namespace themis::rag::learning

// Example main function (for testing/demo purposes)
// int main() {
//     try {
//         themis::rag::learning::example_ml_observability();
//         return 0;
//     } catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         return 1;
//     }
// }
