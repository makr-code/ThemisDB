/**
 * @file ml_learning_metrics_collector.cpp
 * @brief Implementation of ML & Continuous Learning metrics collector.
 */

#include "rag/ml_learning_metrics_collector.h"

#include <spdlog/spdlog.h>

#include "core/concerns/i_context.h"
#include "utils/uuid.h"
// Include tracer utilities for trace ID and span ID generation
#include "observability/tracer_utils.h"

namespace themis::rag::learning {

using namespace themis::observability::detail;  // for generateTraceId, generateSpanId

// Static member initialization
std::shared_ptr<MLLearningMetricsCollector> MLLearningMetricsCollector::instance_;
std::mutex MLLearningMetricsCollector::instance_mutex_;

std::shared_ptr<MLLearningMetricsCollector>
MLLearningMetricsCollector::getInstance() {
    {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::shared_ptr<MLLearningMetricsCollector>(
                new MLLearningMetricsCollector());
        }
    }
    return instance_;
}

void MLLearningMetricsCollector::initialize(
    std::shared_ptr<core::concerns::IMetrics> metrics,
    std::shared_ptr<core::concerns::ILogger> logger) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    metrics_ = metrics;
    logger_ = logger;
    
    if (logger_) {
        spdlog::info("MLLearningMetricsCollector initialized with metrics and logging");
    }
}

// ============================================================================
// Learning Loop Observability
// ============================================================================

void MLLearningMetricsCollector::recordLoopStateTransition(
    const std::string& loop_id,
    const std::string& state,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        loop_states_[loop_id] = state;
    }

    // Export metrics
    core::concerns::IMetrics::Labels labels;
    labels["loop_id"] = loop_id;
    labels["state"] = state;

    metrics_->incrementCounter("themisdb_ml_loop_transitions_total", 1, labels);
    metrics_->setGauge("themisdb_ml_loop_state", 1.0, labels);

    logWithTrace("info",
                 "Loop state transition: " + loop_id + " -> " + state,
                 ctx);
}

void MLLearningMetricsCollector::recordLoopExecution(
    const std::string& loop_id,
    double duration_ms,
    bool success,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["loop_id"] = loop_id;
    labels["status"] = success ? "success" : "failure";

    metrics_->observeHistogram("themisdb_ml_loop_duration_ms", duration_ms, labels);
    metrics_->incrementCounter("themisdb_ml_loop_executions_total", 1, labels);

    if (!success) {
        metrics_->incrementCounter("themisdb_ml_loop_errors_total", 1, labels);
    }

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        loop_last_execution_[loop_id] = std::chrono::system_clock::now();
    }

    logWithTrace("debug",
                 "Loop execution: " + loop_id + " duration=" + 
                 std::to_string(duration_ms) + "ms status=" +
                 (success ? "success" : "failure"),
                 ctx);
}

// ============================================================================
// Adapter & Model Observability
// ============================================================================

void MLLearningMetricsCollector::recordAdapterVersion(
    const std::string& adapter_id,
    const std::string& version,
    const std::string& status,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        adapter_versions_[adapter_id] = version;
    }

    core::concerns::IMetrics::Labels labels;
    labels["adapter_id"] = adapter_id;
    labels["version"] = version;
    labels["status"] = status;

    // Record adapter version deployment
    metrics_->incrementCounter("themisdb_ml_adapter_deployments_total", 1, labels);
    metrics_->setGauge("themisdb_ml_adapter_version", 1.0, labels);

    logWithTrace("info",
                 "Adapter version deployed: " + adapter_id + " v" + 
                 version + " status=" + status,
                 ctx);
}

void MLLearningMetricsCollector::recordRetrainingProgress(
    const std::string& adapter_id,
    double progress,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["adapter_id"] = adapter_id;

    // Progress as percentage (0-100)
    metrics_->setGauge("themisdb_ml_retraining_progress_percent",
                      progress * 100.0,
                      labels);

    if (progress >= 1.0) {
        metrics_->incrementCounter("themisdb_ml_retraining_completions_total", 1, labels);
        logWithTrace("info",
                     "Retraining completed: " + adapter_id,
                     ctx);
    }
}

void MLLearningMetricsCollector::recordModelPerformance(
    const std::string& adapter_id,
    double accuracy,
    double latency_ms,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["adapter_id"] = adapter_id;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        adapter_accuracies_[adapter_id] = accuracy;
    }

    metrics_->setGauge("themisdb_ml_model_accuracy", accuracy, labels);
    metrics_->observeHistogram("themisdb_ml_inference_latency_ms", latency_ms, labels);

    logWithTrace("debug",
                 "Model performance: " + adapter_id + " accuracy=" +
                 std::to_string(accuracy) + " latency=" +
                 std::to_string(latency_ms) + "ms",
                 ctx);
}

// ============================================================================
// A/B Test Observability
// ============================================================================

void MLLearningMetricsCollector::recordABTestState(
    const std::string& test_id,
    const std::string& status,
    double treatment_improvement,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["test_id"] = test_id;
    labels["status"] = status;

    metrics_->incrementCounter("themisdb_ml_ab_test_state_changes_total", 1, labels);
    metrics_->setGauge("themisdb_ml_ab_test_improvement_percent",
                      treatment_improvement * 100.0,
                      labels);

    logWithTrace("info",
                 "A/B test state change: " + test_id + " status=" + status +
                 " improvement=" + std::to_string(treatment_improvement),
                 ctx);
}

// ============================================================================
// Error & Warning States
// ============================================================================

void MLLearningMetricsCollector::recordWarningState(
    const std::string& warning_type,
    const std::string& component,
    const std::string& message,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["warning_type"] = warning_type;
    labels["component"] = component;

    metrics_->incrementCounter("themisdb_ml_warnings_total", 1, labels);

    logWithTrace("warn",
                 "ML warning: " + warning_type + " in " + component +
                 " - " + message,
                 ctx);
}

void MLLearningMetricsCollector::recordErrorState(
    const std::string& error_type,
    const std::string& component,
    const std::string& message,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["error_type"] = error_type;
    labels["component"] = component;

    metrics_->incrementCounter("themisdb_ml_errors_total", 1, labels);

    logWithTrace("error",
                 "ML error: " + error_type + " in " + component +
                 " - " + message,
                 ctx);
}

// ============================================================================
// Feedback Loop Observability
// ============================================================================

void MLLearningMetricsCollector::recordFeedbackCollection(
    const std::string& adapter_id,
    size_t feedback_count,
    double positive_fraction,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    core::concerns::IMetrics::Labels labels;
    labels["adapter_id"] = adapter_id;

    metrics_->setGauge("themisdb_ml_feedback_count",
                      static_cast<double>(feedback_count),
                      labels);
    metrics_->setGauge("themisdb_ml_positive_feedback_fraction",
                      positive_fraction,
                      labels);

    logWithTrace("debug",
                 "Feedback collection: " + adapter_id + " count=" +
                 std::to_string(feedback_count) + " positive_fraction=" +
                 std::to_string(positive_fraction),
                 ctx);
}

// ============================================================================
// Batch Operations
// ============================================================================

void MLLearningMetricsCollector::updateFromSnapshot(
    const LearningStats& stats,
    const LearningTraceContext& ctx) {
    if (!metrics_) return;

    // Update main statistics
    metrics_->setGauge("themisdb_ml_accuracy", stats.current_accuracy);
    metrics_->setGauge("themisdb_ml_accuracy_7d_avg", stats.accuracy_7d_avg);
    metrics_->setGauge("themisdb_ml_accuracy_trend", stats.accuracy_trend);

    // Counters
    core::concerns::IMetrics::Labels no_labels;
    metrics_->setGauge("themisdb_ml_total_interactions",
                      static_cast<double>(stats.total_interactions_logged),
                      no_labels);
    metrics_->setGauge("themisdb_ml_lora_retraining_count",
                      static_cast<double>(stats.lora_retraining_count),
                      no_labels);
    metrics_->setGauge("themisdb_ml_prompt_optimizations",
                      static_cast<double>(stats.prompt_optimizations),
                      no_labels);
    metrics_->setGauge("themisdb_ml_retrieval_optimizations",
                      static_cast<double>(stats.retrieval_optimizations),
                      no_labels);

    // A/B test information
    metrics_->setGauge("themisdb_ml_active_ab_tests",
                      static_cast<double>(stats.active_ab_tests.size()),
                      no_labels);

    logWithTrace("debug",
                 "Snapshot updated: accuracy=" +
                 std::to_string(stats.current_accuracy) +
                 " interactions=" + std::to_string(stats.total_interactions_logged),
                 ctx);
}

// ============================================================================
// Trace Context Utilities
// ============================================================================

LearningTraceContext MLLearningMetricsCollector::createTraceContext() {
    LearningTraceContext ctx;
    
    // Generate trace ID (32-char hex from 128 bits)
    ctx.trace_id = generateTraceId();
    
    // Generate span ID (16-char hex from 64 bits)
    ctx.span_id = generateSpanId();
    
    // Generate request ID (UUID)
    ctx.request_id = utils::generate_uuid_v4();
    
    return ctx;
}

LearningTraceContext MLLearningMetricsCollector::extractTraceContext(
    const core::concerns::IContext& ctx) {
    LearningTraceContext trace_ctx;
    
    // Extract from context keys using core::concerns::context_keys
    using namespace core::concerns::context_keys;
    
    auto trace_id = ctx.get(kTraceId);
    auto span_id = ctx.get(kSpanId);
    auto request_id = ctx.get(kRequestId);
    
    if (trace_id) {
        trace_ctx.trace_id = std::string(*trace_id);
    } else {
        trace_ctx.trace_id = generateTraceId();
    }
    
    if (span_id) {
        trace_ctx.span_id = std::string(*span_id);
    } else {
        trace_ctx.span_id = generateSpanId();
    }
    
    if (request_id) {
        trace_ctx.request_id = std::string(*request_id);
    } else {
        trace_ctx.request_id = utils::generate_uuid_v4();
    }
    
    return trace_ctx;
}

// ============================================================================
// Helper Methods
// ============================================================================

void MLLearningMetricsCollector::logWithTrace(
    const std::string& level,
    const std::string& message,
    const LearningTraceContext& ctx) {
    if (!logger_) {
        // Fallback to spdlog if logger not initialized
        spdlog::info("{} [trace:{}] {}", level, ctx.trace_id, message);
        return;
    }

    // Create fields with trace context
    core::concerns::ILogger::Fields fields;
    fields["trace_id"] = ctx.trace_id;
    fields["span_id"] = ctx.span_id;
    fields["request_id"] = ctx.request_id;
    fields["component"] = "MLLearningMetrics";

    // Log with appropriate level
    if (level == "debug") {
        logger_->logStructured(core::concerns::ILogger::Level::DEBUG, message, fields);
    } else if (level == "info") {
        logger_->logStructured(core::concerns::ILogger::Level::INFO, message, fields);
    } else if (level == "warn") {
        logger_->logStructured(core::concerns::ILogger::Level::WARNING, message, fields);
    } else if (level == "error") {
        logger_->logStructured(core::concerns::ILogger::Level::ERROR, message, fields);
    }
}

}  // namespace themis::rag::learning
