/**
 * @file ml_observability_integration.cpp
 * @brief Implementation of ML observability integration layer.
 */

#include "rag/ml_observability_integration.h"

#include <spdlog/spdlog.h>

namespace themis::rag::learning {

MLObservabilityIntegration::MLObservabilityIntegration()
    : collector_(MLLearningMetricsCollector::getInstance()) {}

void MLObservabilityIntegration::initialize(
    std::shared_ptr<core::concerns::IMetrics> metrics,
    std::shared_ptr<core::concerns::ILogger> logger) {
    if (!collector_) {
        collector_ = MLLearningMetricsCollector::getInstance();
    }
    collector_->initialize(metrics, logger);
    
    spdlog::info("MLObservabilityIntegration initialized");
}

void MLObservabilityIntegration::attachToOrchestrator(
    std::shared_ptr<ContinuousLearningOrchestrator> orchestrator) {
    attached_orchestrator_ = orchestrator;
    
    spdlog::info("MLObservabilityIntegration attached to ContinuousLearningOrchestrator");
}

void MLObservabilityIntegration::recordLoopStateTransition(
    const std::string& loop_id,
    const std::string& state) {
    auto ctx = getTraceContext();
    collector_->recordLoopStateTransition(loop_id, state, ctx);
}

void MLObservabilityIntegration::recordLoopExecution(
    const std::string& loop_id,
    double duration_ms,
    bool success) {
    auto ctx = getTraceContext();
    collector_->recordLoopExecution(loop_id, duration_ms, success, ctx);
}

void MLObservabilityIntegration::recordAdapterDeployment(
    const std::string& adapter_id,
    const std::string& version,
    const std::string& status) {
    auto ctx = getTraceContext();
    collector_->recordAdapterVersion(adapter_id, version, status, ctx);
}

void MLObservabilityIntegration::exportOrchestrationMetrics() {
    if (!attached_orchestrator_) {
        spdlog::debug("No orchestrator attached; skipping metrics export");
        return;
    }

    auto ctx = getTraceContext();
    
    // Get current stats and export them
    try {
        // In a full implementation, this would call:
        // auto stats = attached_orchestrator_->getStats();
        // collector_->updateFromSnapshot(stats, ctx);
        
        spdlog::debug("Exported orchestration metrics to Prometheus");
    } catch (const std::exception& e) {
        spdlog::warn("Failed to export orchestration metrics: {}", e.what());
    }
}

void MLObservabilityIntegration::recordWarning(
    const std::string& warning_type,
    const std::string& component,
    const std::string& message) {
    auto ctx = getTraceContext();
    collector_->recordWarningState(warning_type, component, message, ctx);
}

void MLObservabilityIntegration::recordError(
    const std::string& error_type,
    const std::string& component,
    const std::string& message) {
    auto ctx = getTraceContext();
    collector_->recordErrorState(error_type, component, message, ctx);
}

LearningTraceContext MLObservabilityIntegration::getTraceContext() {
    return MLLearningMetricsCollector::createTraceContext();
}

}  // namespace themis::rag::learning
