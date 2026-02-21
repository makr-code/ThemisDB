/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            prompt_engineering_metrics.cpp                     ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     545                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file prompt_engineering_metrics.cpp
 * @brief Implementation of Prometheus metrics for prompt engineering
 */

#include "prompt_engineering/prompt_engineering_metrics.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace prompt_engineering {

PromptEngineeringMetrics::PromptEngineeringMetrics(const Config& config)
    : config_(config)
{
}

PromptEngineeringMetrics::PromptEngineeringMetrics()
    : config_(Config{})
{
}

// Optimization metrics
void PromptEngineeringMetrics::recordOptimizationAttempt(const std::string& prompt_id) {
    if (!config_.enabled) return;
    optimization_attempts_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationSuccess(
    const std::string& prompt_id,
    double improvement
) {
    if (!config_.enabled) return;
    optimization_successes_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationFailure(
    const std::string& prompt_id,
    const std::string& reason
) {
    if (!config_.enabled) return;
    optimization_failures_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationDuration(
    const std::string& prompt_id,
    double duration_ms
) {
    if (!config_.enabled) return;
    optimization_total_duration_ms_.fetch_add(duration_ms, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationIterations(
    const std::string& prompt_id,
    int iterations
) {
    if (!config_.enabled) return;
    optimization_total_iterations_.fetch_add(iterations, std::memory_order_relaxed);
}

// A/B testing metrics
void PromptEngineeringMetrics::recordABTestStart(
    const std::string& test_id,
    const std::string& prompt_id
) {
    if (!config_.enabled) return;
    ab_test_starts_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestObservation(
    const std::string& test_id,
    const std::string& version,
    bool success
) {
    if (!config_.enabled) return;
    ab_test_observations_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestCompletion(
    const std::string& test_id,
    const std::string& winner,
    double confidence
) {
    if (!config_.enabled) return;
    ab_test_completions_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestDuration(
    const std::string& test_id,
    double duration_seconds
) {
    if (!config_.enabled) return;
    // Could track per-test durations if needed
}

void PromptEngineeringMetrics::recordActiveABTests(int count) {
    if (!config_.enabled) return;
    active_ab_tests_.store(count, std::memory_order_relaxed);
}

// Performance tracking metrics
void PromptEngineeringMetrics::recordPromptExecution(
    const std::string& prompt_id,
    bool success,
    double latency_ms
) {
    if (!config_.enabled) return;
    prompt_executions_.fetch_add(1, std::memory_order_relaxed);
    if (success) {
        prompt_successes_.fetch_add(1, std::memory_order_relaxed);
    } else {
        prompt_failures_.fetch_add(1, std::memory_order_relaxed);
    }
    prompt_total_latency_ms_.fetch_add(latency_ms, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordPromptSuccessRate(
    const std::string& prompt_id,
    double rate
) {
    if (!config_.enabled) return;
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_success_rates_[prompt_id] = rate;
}

void PromptEngineeringMetrics::recordPromptAverageLatency(
    const std::string& prompt_id,
    double latency_ms
) {
    if (!config_.enabled) return;
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_avg_latencies_[prompt_id] = latency_ms;
}

void PromptEngineeringMetrics::recordPromptExecutionCount(
    const std::string& prompt_id,
    int64_t count
) {
    if (!config_.enabled) return;
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_execution_counts_[prompt_id] = count;
}

// Feedback collection metrics
void PromptEngineeringMetrics::recordFeedback(
    const std::string& prompt_id,
    const std::string& type
) {
    if (!config_.enabled) return;
    feedback_total_.fetch_add(1, std::memory_order_relaxed);
    
    if (type == "USER_POSITIVE" || type == "POSITIVE") {
        feedback_positive_.fetch_add(1, std::memory_order_relaxed);
    } else if (type == "USER_NEGATIVE" || type == "NEGATIVE") {
        feedback_negative_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordFeedbackSeverity(
    const std::string& prompt_id,
    double severity
) {
    if (!config_.enabled) return;
    // Could track severity distribution
}

void PromptEngineeringMetrics::recordHallucinationDetection(
    const std::string& prompt_id
) {
    if (!config_.enabled) return;
    hallucination_detections_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordFailedQuery(
    const std::string& prompt_id,
    const std::string& failure_type
) {
    if (!config_.enabled) return;
    failed_queries_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordTotalFeedbackCount(
    const std::string& prompt_id,
    int64_t count
) {
    if (!config_.enabled) return;
    // Per-prompt feedback count
}

void PromptEngineeringMetrics::recordPositiveRatio(
    const std::string& prompt_id,
    double ratio
) {
    if (!config_.enabled) return;
    // Per-prompt positive ratio
}

// Version control metrics
void PromptEngineeringMetrics::recordVersionCommit(
    const std::string& prompt_id,
    const std::string& branch
) {
    if (!config_.enabled) return;
    version_commits_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordVersionRollback(const std::string& prompt_id) {
    if (!config_.enabled) return;
    version_rollbacks_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordBranchCreation(const std::string& prompt_id) {
    if (!config_.enabled) return;
    branch_creations_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordMergeOperation(
    const std::string& prompt_id,
    const std::string& strategy,
    bool success
) {
    if (!config_.enabled) return;
    merge_operations_.fetch_add(1, std::memory_order_relaxed);
    if (success) {
        merge_successes_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordVersionCount(
    const std::string& prompt_id,
    int count
) {
    if (!config_.enabled) return;
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_version_counts_[prompt_id] = count;
}

// Integration metrics
void PromptEngineeringMetrics::recordIntegrationExecution(bool before) {
    if (!config_.enabled) return;
    if (before) {
        integration_before_calls_.fetch_add(1, std::memory_order_relaxed);
    } else {
        integration_after_calls_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordBackgroundWorkerCycle() {
    if (!config_.enabled) return;
    background_worker_cycles_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordBackgroundWorkerDuration(double duration_ms) {
    if (!config_.enabled) return;
    background_worker_total_duration_ms_.fetch_add(duration_ms, std::memory_order_relaxed);
}

// Export metrics in Prometheus format
std::string PromptEngineeringMetrics::exportMetrics() const {
    std::ostringstream oss;
    const std::string prefix = config_.namespace_prefix;

    // Optimization metrics
    oss << formatMetric(
        prefix + "_optimization_attempts_total",
        "Total number of optimization attempts",
        "counter",
        optimization_attempts_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_optimization_successes_total",
        "Total number of successful optimizations",
        "counter",
        optimization_successes_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_optimization_failures_total",
        "Total number of failed optimizations",
        "counter",
        optimization_failures_.load(std::memory_order_relaxed)
    );

    double avg_duration = 0.0;
    auto attempts = optimization_attempts_.load(std::memory_order_relaxed);
    if (attempts > 0) {
        avg_duration = optimization_total_duration_ms_.load(std::memory_order_relaxed) / attempts;
    }
    oss << formatMetric(
        prefix + "_optimization_duration_ms_avg",
        "Average optimization duration in milliseconds",
        "gauge",
        avg_duration
    );

    // A/B testing metrics
    oss << formatMetric(
        prefix + "_ab_test_starts_total",
        "Total number of A/B tests started",
        "counter",
        ab_test_starts_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_ab_test_completions_total",
        "Total number of A/B tests completed",
        "counter",
        ab_test_completions_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_ab_test_observations_total",
        "Total number of A/B test observations",
        "counter",
        ab_test_observations_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_ab_tests_active",
        "Number of currently active A/B tests",
        "gauge",
        active_ab_tests_.load(std::memory_order_relaxed)
    );

    // Performance tracking metrics
    oss << formatMetric(
        prefix + "_prompt_executions_total",
        "Total number of prompt executions",
        "counter",
        prompt_executions_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_prompt_successes_total",
        "Total number of successful prompt executions",
        "counter",
        prompt_successes_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_prompt_failures_total",
        "Total number of failed prompt executions",
        "counter",
        prompt_failures_.load(std::memory_order_relaxed)
    );

    double success_rate = 0.0;
    auto executions = prompt_executions_.load(std::memory_order_relaxed);
    if (executions > 0) {
        success_rate = static_cast<double>(prompt_successes_.load(std::memory_order_relaxed)) / executions;
    }
    oss << formatMetric(
        prefix + "_prompt_success_rate",
        "Overall prompt success rate",
        "gauge",
        success_rate
    );

    double avg_latency = 0.0;
    if (executions > 0) {
        avg_latency = prompt_total_latency_ms_.load(std::memory_order_relaxed) / executions;
    }
    oss << formatMetric(
        prefix + "_prompt_latency_ms_avg",
        "Average prompt execution latency in milliseconds",
        "gauge",
        avg_latency
    );

    // Feedback metrics
    oss << formatMetric(
        prefix + "_feedback_total",
        "Total number of feedback entries",
        "counter",
        feedback_total_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_feedback_positive_total",
        "Total number of positive feedback entries",
        "counter",
        feedback_positive_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_feedback_negative_total",
        "Total number of negative feedback entries",
        "counter",
        feedback_negative_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_hallucination_detections_total",
        "Total number of hallucination detections",
        "counter",
        hallucination_detections_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_failed_queries_total",
        "Total number of failed queries",
        "counter",
        failed_queries_.load(std::memory_order_relaxed)
    );

    // Version control metrics
    oss << formatMetric(
        prefix + "_version_commits_total",
        "Total number of version commits",
        "counter",
        version_commits_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_version_rollbacks_total",
        "Total number of version rollbacks",
        "counter",
        version_rollbacks_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_branch_creations_total",
        "Total number of branch creations",
        "counter",
        branch_creations_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_merge_operations_total",
        "Total number of merge operations",
        "counter",
        merge_operations_.load(std::memory_order_relaxed)
    );

    // Integration metrics
    oss << formatMetric(
        prefix + "_integration_before_calls_total",
        "Total number of beforeExecution calls",
        "counter",
        integration_before_calls_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_integration_after_calls_total",
        "Total number of afterExecution calls",
        "counter",
        integration_after_calls_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_background_worker_cycles_total",
        "Total number of background worker cycles",
        "counter",
        background_worker_cycles_.load(std::memory_order_relaxed)
    );

    return oss.str();
}

void PromptEngineeringMetrics::reset() {
    optimization_attempts_.store(0);
    optimization_successes_.store(0);
    optimization_failures_.store(0);
    optimization_total_duration_ms_.store(0.0);
    optimization_total_iterations_.store(0);

    ab_test_starts_.store(0);
    ab_test_completions_.store(0);
    ab_test_observations_.store(0);
    active_ab_tests_.store(0);

    prompt_executions_.store(0);
    prompt_successes_.store(0);
    prompt_failures_.store(0);
    prompt_total_latency_ms_.store(0.0);

    feedback_total_.store(0);
    feedback_positive_.store(0);
    feedback_negative_.store(0);
    hallucination_detections_.store(0);
    failed_queries_.store(0);

    version_commits_.store(0);
    version_rollbacks_.store(0);
    branch_creations_.store(0);
    merge_operations_.store(0);
    merge_successes_.store(0);

    integration_before_calls_.store(0);
    integration_after_calls_.store(0);
    background_worker_cycles_.store(0);
    background_worker_total_duration_ms_.store(0.0);

    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_success_rates_.clear();
    prompt_avg_latencies_.clear();
    prompt_execution_counts_.clear();
    prompt_version_counts_.clear();
}

std::string PromptEngineeringMetrics::formatMetric(
    const std::string& name,
    const std::string& help,
    const std::string& type,
    double value,
    const std::map<std::string, std::string>& labels
) const {
    std::ostringstream oss;
    
    // HELP line
    oss << "# HELP " << name << " " << help << "\n";
    
    // TYPE line
    oss << "# TYPE " << name << " " << type << "\n";
    
    // Metric line
    oss << name;
    if (!labels.empty()) {
        oss << formatLabels(labels);
    }
    oss << " " << std::fixed << std::setprecision(2) << value << "\n";
    
    return oss.str();
}

std::string PromptEngineeringMetrics::formatLabels(
    const std::map<std::string, std::string>& labels
) const {
    if (labels.empty()) return "";
    
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) oss << ",";
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

} // namespace prompt_engineering
} // namespace themis
