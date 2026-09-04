/**
 * @file prompt_engineering_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    optimization_attempts_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationSuccess(
    const std::string& prompt_id,
    double improvement
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)improvement;
    optimization_successes_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationFailure(
    const std::string& prompt_id,
    const std::string& reason
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)reason;
    optimization_failures_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationDuration(
    const std::string& prompt_id,
    double duration_ms
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    optimization_total_duration_ms_.fetch_add(duration_ms, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordOptimizationIterations(
    const std::string& prompt_id,
    int iterations
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    optimization_total_iterations_.fetch_add(iterations, std::memory_order_relaxed);
}

// A/B testing metrics
void PromptEngineeringMetrics::recordABTestStart(
    const std::string& test_id,
    const std::string& prompt_id
) {
    if (!config_.enabled) {
      return;
    }
    (void)test_id;
    (void)prompt_id;
    ab_test_starts_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestObservation(
    const std::string& test_id,
    const std::string& version,
    bool success
) {
    if (!config_.enabled) {
      return;
    }
    (void)test_id;
    (void)version;
    (void)success;
    ab_test_observations_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestCompletion(
    const std::string& test_id,
    const std::string& winner,
    double confidence
) {
    if (!config_.enabled) {
      return;
    }
    (void)test_id;
    (void)winner;
    (void)confidence;
    ab_test_completions_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordABTestDuration(
    const std::string& test_id,
    double duration_seconds
) {
    if (!config_.enabled) {
      return;
    }
    (void)test_id;
    (void)duration_seconds;
    // Could track per-test durations if needed
}

void PromptEngineeringMetrics::recordActiveABTests([[maybe_unused]] int count) {
    if (!config_.enabled) {
      return;
    }
    active_ab_tests_.store(count, std::memory_order_relaxed);
}

// Performance tracking metrics
void PromptEngineeringMetrics::recordPromptExecution(
    const std::string& prompt_id,
    bool success,
    double latency_ms
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    prompt_executions_.fetch_add(1, std::memory_order_relaxed);
    if (success) {
        prompt_successes_.fetch_add(1, std::memory_order_relaxed);
    } else {
        prompt_failures_.fetch_add(1, std::memory_order_relaxed);
    }
    prompt_total_latency_ms_.fetch_add(latency_ms, std::memory_order_relaxed);

    // Check alert thresholds
    if ([[maybe_unused]] alert_callback_) {
        int64_t execs    = prompt_executions_.load(std::memory_order_relaxed);
        int64_t failures = prompt_failures_.load(std::memory_order_relaxed);
        if (execs > 0) {
            double failure_rate = static_cast<double>(failures) / execs;
            if (failure_rate > alert_config_.max_failure_rate) {
                AlertEvent ev;
                ev.metric_name = "prompt_failure_rate";
                ev.value       = failure_rate;
                ev.threshold   = alert_config_.max_failure_rate;
                ev.message     = "Prompt failure rate exceeded threshold";
                alert_callback_([[maybe_unused]] ev);
            }
        }
    }
}

void PromptEngineeringMetrics::recordPromptSuccessRate(
    const std::string& prompt_id,
    double rate
) {
    if (!config_.enabled) {
      return;
    }
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_success_rates_[prompt_id] = rate;
}

void PromptEngineeringMetrics::recordPromptAverageLatency(
    const std::string& prompt_id,
    double latency_ms
) {
    if (!config_.enabled) {
      return;
    }
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_avg_latencies_[prompt_id] = latency_ms;
}

void PromptEngineeringMetrics::recordPromptExecutionCount(
    const std::string& prompt_id,
    int64_t count
) {
    if (!config_.enabled) {
      return;
    }
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_execution_counts_[prompt_id] = count;
}

// Feedback collection metrics
void PromptEngineeringMetrics::recordFeedback(
    const std::string& prompt_id,
    const std::string& type
) {
    if (!config_.enabled) {
      return;
    }
    feedback_total_.fetch_add(1, std::memory_order_relaxed);
    (void)prompt_id;
    
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
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)severity;
    // Could track severity distribution
}

void PromptEngineeringMetrics::recordHallucinationDetection(
    const std::string& prompt_id
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    int64_t count = hallucination_detections_.fetch_add(1, std::memory_order_relaxed) + 1;

    if ([[maybe_unused]] alert_callback_ && count > alert_config_.max_hallucinations) {
        AlertEvent ev;
        ev.metric_name = "hallucination_count";
        ev.value       = static_cast<double>(count);
        ev.threshold   = static_cast<double>(alert_config_.max_hallucinations);
        ev.message     = "Hallucination count exceeded threshold";
        alert_callback_([[maybe_unused]] ev);
    }
}

void PromptEngineeringMetrics::recordFailedQuery(
    const std::string& prompt_id,
    const std::string& failure_type
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)failure_type;
    failed_queries_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordTotalFeedbackCount(
    const std::string& prompt_id,
    int64_t count
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)count;
    // Per-prompt feedback count
}

void PromptEngineeringMetrics::recordPositiveRatio(
    const std::string& prompt_id,
    double ratio
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)ratio;
    // Per-prompt positive ratio
}

// Version control metrics
void PromptEngineeringMetrics::recordVersionCommit(
    const std::string& prompt_id,
    const std::string& branch
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)branch;
    version_commits_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordVersionRollback(const std::string& prompt_id) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    version_rollbacks_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordBranchCreation(const std::string& prompt_id) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    branch_creations_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordMergeOperation(
    const std::string& prompt_id,
    const std::string& strategy,
    bool success
) {
    if (!config_.enabled) {
      return;
    }
    (void)prompt_id;
    (void)strategy;
    merge_operations_.fetch_add(1, std::memory_order_relaxed);
    if (success) {
        merge_successes_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordVersionCount(
    const std::string& prompt_id,
    int count
) {
    if (!config_.enabled) {
      return;
    }
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    prompt_version_counts_[prompt_id] = count;
}

// Integration metrics
void PromptEngineeringMetrics::recordIntegrationExecution(bool before) {
    if (!config_.enabled) {
      return;
    }
    if (before) {
        integration_before_calls_.fetch_add(1, std::memory_order_relaxed);
    } else {
        integration_after_calls_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordBackgroundWorkerCycle() {
    if (!config_.enabled) {
      return;
    }
    background_worker_cycles_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordBackgroundWorkerDuration([[maybe_unused]] double duration_ms) {
    if (!config_.enabled) {
      return;
    }
    background_worker_total_duration_ms_.fetch_add(duration_ms, std::memory_order_relaxed);
}

// Export metrics in Prometheus format
std::string PromptEngineeringMetrics::exportMetrics() const {
    std::ostringstream oss = {};
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
        avg_duration = optimization_total_duration_ms_.load(std::memory_order_relaxed) / static_cast<double>(attempts);
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
        success_rate = static_cast<double>(prompt_successes_.load(std::memory_order_relaxed)) / static_cast<double>(executions);
    }
    oss << formatMetric(
        prefix + "_prompt_success_rate",
        "Overall prompt success rate",
        "gauge",
        success_rate
    );

    double avg_latency = 0.0;
    if (executions > 0) {
        avg_latency = prompt_total_latency_ms_.load(std::memory_order_relaxed) / static_cast<double>(executions);
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

    // Reflection Tuning metrics
    oss << formatMetric(
        prefix + "_reflection_cycle_starts_total",
        "Total number of reflection tuning cycles started",
        "counter",
        reflection_cycle_starts_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_reflection_cycle_completions_total",
        "Total number of reflection tuning cycles completed",
        "counter",
        reflection_cycle_completions_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_reflection_iterations_total",
        "Total number of generate-critique-revise iterations across all cycles",
        "counter",
        reflection_iterations_total_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_reflection_improvements_total",
        "Total number of reflection cycles that improved response quality",
        "counter",
        reflection_improvements_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_reflection_guard_fires_total",
        "Total number of times ReflectionHallucinationGuard halted a cycle",
        "counter",
        reflection_guard_fires_.load(std::memory_order_relaxed)
    );

    oss << formatMetric(
        prefix + "_reflection_quality_delta_sum",
        "Sum of all quality deltas produced by reflection cycles",
        "gauge",
        reflection_quality_delta_sum_.load(std::memory_order_relaxed)
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

    reflection_cycle_starts_.store(0);
    reflection_cycle_completions_.store(0);
    reflection_iterations_total_.store(0);
    reflection_improvements_.store(0);
    reflection_guard_fires_.store(0);
    reflection_quality_delta_sum_.store(0.0);

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

std::string PromptEngineeringMetrics::formatMetric(
    const std::string& name,
    const std::string& help,
    const std::string& type,
    int64_t value,
    const std::map<std::string, std::string>& labels
) const {
    return formatMetric(name, help, type, static_cast<double>(value), labels);
}

std::string PromptEngineeringMetrics::formatMetric(
    const std::string& name,
    const std::string& help,
    const std::string& type,
    int value,
    const std::map<std::string, std::string>& labels
) const {
    return formatMetric(name, help, type, static_cast<double>(value), labels);
}

std::string PromptEngineeringMetrics::formatLabels(
    const std::map<std::string, std::string>& labels
) const {
    if (labels.empty()) {
      return "";
    }
    
    std::ostringstream oss = {};
    oss << "{";
    bool first = true;
    for (const auto& [key, value] : labels) {
        if (!first) {
          oss << ",";
        }
        oss << key << "=\"" << value << "\"";
        first = false;
    }
    oss << "}";
    return oss.str();
}

// ============================================================================
// Persistence: snapshot / restore
// ============================================================================

nlohmann::json PromptEngineeringMetrics::snapshotToJson() const {
    nlohmann::json j;
    j["optimization_attempts"]           = optimization_attempts_.load(std::memory_order_relaxed);
    j["optimization_successes"]          = optimization_successes_.load(std::memory_order_relaxed);
    j["optimization_failures"]           = optimization_failures_.load(std::memory_order_relaxed);
    j["optimization_total_duration_ms"]  = optimization_total_duration_ms_.load(std::memory_order_relaxed);
    j["optimization_total_iterations"]   = optimization_total_iterations_.load(std::memory_order_relaxed);
    j["ab_test_starts"]                  = ab_test_starts_.load(std::memory_order_relaxed);
    j["ab_test_completions"]             = ab_test_completions_.load(std::memory_order_relaxed);
    j["ab_test_observations"]            = ab_test_observations_.load(std::memory_order_relaxed);
    j["active_ab_tests"]                 = active_ab_tests_.load(std::memory_order_relaxed);
    j["prompt_executions"]               = prompt_executions_.load(std::memory_order_relaxed);
    j["prompt_successes"]                = prompt_successes_.load(std::memory_order_relaxed);
    j["prompt_failures"]                 = prompt_failures_.load(std::memory_order_relaxed);
    j["prompt_total_latency_ms"]         = prompt_total_latency_ms_.load(std::memory_order_relaxed);
    j["feedback_total"]                  = feedback_total_.load(std::memory_order_relaxed);
    j["feedback_positive"]               = feedback_positive_.load(std::memory_order_relaxed);
    j["feedback_negative"]               = feedback_negative_.load(std::memory_order_relaxed);
    j["hallucination_detections"]        = hallucination_detections_.load(std::memory_order_relaxed);
    j["failed_queries"]                  = failed_queries_.load(std::memory_order_relaxed);
    j["version_commits"]                 = version_commits_.load(std::memory_order_relaxed);
    j["version_rollbacks"]               = version_rollbacks_.load(std::memory_order_relaxed);
    j["branch_creations"]                = branch_creations_.load(std::memory_order_relaxed);
    j["merge_operations"]                = merge_operations_.load(std::memory_order_relaxed);
    j["merge_successes"]                 = merge_successes_.load(std::memory_order_relaxed);
    j["integration_before_calls"]        = integration_before_calls_.load(std::memory_order_relaxed);
    j["integration_after_calls"]         = integration_after_calls_.load(std::memory_order_relaxed);
    j["background_worker_cycles"]        = background_worker_cycles_.load(std::memory_order_relaxed);
    j["background_worker_total_duration_ms"] = background_worker_total_duration_ms_.load(std::memory_order_relaxed);
    j["reflection_cycle_starts"]         = reflection_cycle_starts_.load(std::memory_order_relaxed);
    j["reflection_cycle_completions"]    = reflection_cycle_completions_.load(std::memory_order_relaxed);
    j["reflection_iterations_total"]     = reflection_iterations_total_.load(std::memory_order_relaxed);
    j["reflection_improvements"]         = reflection_improvements_.load(std::memory_order_relaxed);
    j["reflection_guard_fires"]          = reflection_guard_fires_.load(std::memory_order_relaxed);
    j["reflection_quality_delta_sum"]    = reflection_quality_delta_sum_.load(std::memory_order_relaxed);
    return j;
}

void PromptEngineeringMetrics::restoreFromJson(const nlohmann::json& snapshot) {
    auto load_i64 = [&](const char* key, std::atomic<int64_t>& target) {
        if (snapshot.contains(key)) {
          target.store(snapshot[key].get<int64_t>(), std::memory_order_relaxed);
        }
    };
    auto load_i32 = [&](const char* key, std::atomic<int>& target) {
        if (snapshot.contains(key)) {
          target.store(snapshot[key].get<int>(), std::memory_order_relaxed);
        }
    };
    auto load_dbl = [&](const char* key, std::atomic<double>& target) {
        if (snapshot.contains(key)) {
          target.store(snapshot[key].get<double>(), std::memory_order_relaxed);
        }
    };

    load_i64("optimization_attempts",           optimization_attempts_);
    load_i64("optimization_successes",          optimization_successes_);
    load_i64("optimization_failures",           optimization_failures_);
    load_dbl("optimization_total_duration_ms",  optimization_total_duration_ms_);
    load_i64("optimization_total_iterations",   optimization_total_iterations_);
    load_i64("ab_test_starts",                  ab_test_starts_);
    load_i64("ab_test_completions",             ab_test_completions_);
    load_i64("ab_test_observations",            ab_test_observations_);
    load_i32("active_ab_tests",                 active_ab_tests_);
    load_i64("prompt_executions",               prompt_executions_);
    load_i64("prompt_successes",                prompt_successes_);
    load_i64("prompt_failures",                 prompt_failures_);
    load_dbl("prompt_total_latency_ms",         prompt_total_latency_ms_);
    load_i64("feedback_total",                  feedback_total_);
    load_i64("feedback_positive",               feedback_positive_);
    load_i64("feedback_negative",               feedback_negative_);
    load_i64("hallucination_detections",        hallucination_detections_);
    load_i64("failed_queries",                  failed_queries_);
    load_i64("version_commits",                 version_commits_);
    load_i64("version_rollbacks",               version_rollbacks_);
    load_i64("branch_creations",                branch_creations_);
    load_i64("merge_operations",                merge_operations_);
    load_i64("merge_successes",                 merge_successes_);
    load_i64("integration_before_calls",        integration_before_calls_);
    load_i64("integration_after_calls",         integration_after_calls_);
    load_i64("background_worker_cycles",        background_worker_cycles_);
    load_dbl("background_worker_total_duration_ms", background_worker_total_duration_ms_);
    load_i64("reflection_cycle_starts",         reflection_cycle_starts_);
    load_i64("reflection_cycle_completions",    reflection_cycle_completions_);
    load_i64("reflection_iterations_total",     reflection_iterations_total_);
    load_i64("reflection_improvements",         reflection_improvements_);
    load_i64("reflection_guard_fires",          reflection_guard_fires_);
    load_dbl("reflection_quality_delta_sum",    reflection_quality_delta_sum_);
}

// ============================================================================
// Alerting
// ============================================================================

void PromptEngineeringMetrics::setAlertConfig(const AlertConfig& cfg) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    alert_config_ = cfg;
}

void PromptEngineeringMetrics::setAlertCallback([[maybe_unused]] AlertCallback cb) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    alert_callback_ = std::move([[maybe_unused]] cb);
}

// ============================================================================
// Reflection Tuning metrics
// ============================================================================

void PromptEngineeringMetrics::recordReflectionCycleStart(
    const std::string& /*prompt_id*/) {
    if (!config_.enabled) {
      return;
    }
    reflection_cycle_starts_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordReflectionCycleComplete(
    const std::string& /*prompt_id*/,
    size_t iterations,
    bool   improved) {
    if (!config_.enabled) {
      return;
    }
    reflection_cycle_completions_.fetch_add(1, std::memory_order_relaxed);
    reflection_iterations_total_.fetch_add(static_cast<int64_t>(iterations),
                                           std::memory_order_relaxed);
    if (improved) {
        reflection_improvements_.fetch_add(1, std::memory_order_relaxed);
    }
}

void PromptEngineeringMetrics::recordReflectionGuardFired(
    const std::string& /*prompt_id*/) {
    if (!config_.enabled) {
      return;
    }
    reflection_guard_fires_.fetch_add(1, std::memory_order_relaxed);
}

void PromptEngineeringMetrics::recordReflectionQualityDelta(
    const std::string& /*prompt_id*/,
    double delta) {
    if (!config_.enabled) {
      return;
    }
    // Atomic double accumulation via CAS loop (same pattern as existing code).
    double current = reflection_quality_delta_sum_.load(std::memory_order_relaxed);
    double desired = 0;
    do {
        desired = current + delta;
    } while (!reflection_quality_delta_sum_.compare_exchange_weak(
                 current, desired,
                 std::memory_order_relaxed,
                 std::memory_order_relaxed));
}

} // namespace prompt_engineering
} // namespace themis

