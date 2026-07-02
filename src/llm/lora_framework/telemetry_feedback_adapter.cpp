/**
 * @file telemetry_feedback_adapter.cpp
 * @brief Implementation of telemetry-to-feedback conversion
 */

#include "llm/lora_framework/telemetry_feedback_adapter.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <numeric>
#include <algorithm>
#include <sstream>

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// TelemetryMetrics Implementation
// ============================================================================

json TelemetryMetrics::toJSON() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp);
    return json{
        {"adapter_id", adapter_id},
        {"request_id", request_id},
        {"timestamp", time_t},
        {"accuracy", accuracy},
        {"precision", precision},
        {"recall", recall},
        {"f1_score", f1_score},
        {"perplexity", perplexity},
        {"latency_ms", latency_ms},
        {"throughput", throughput},
        {"input_tokens", input_tokens},
        {"output_tokens", output_tokens},
        {"has_error", has_error},
        {"error_message", error_message},
        {"consecutive_errors", consecutive_errors},
        {"prompt", prompt},
        {"response", response},
        {"expected_response", expected_response},
        {"metadata", metadata}
    };
}

TelemetryMetrics TelemetryMetrics::fromJSON(const json& j) {
    TelemetryMetrics metric;
    if (j.contains("adapter_id")) metric.adapter_id = j["adapter_id"];
    if (j.contains("request_id")) metric.request_id = j["request_id"];
    if (j.contains("accuracy")) metric.accuracy = j["accuracy"];
    if (j.contains("precision")) metric.precision = j["precision"];
    if (j.contains("recall")) metric.recall = j["recall"];
    if (j.contains("f1_score")) metric.f1_score = j["f1_score"];
    if (j.contains("perplexity")) metric.perplexity = j["perplexity"];
    if (j.contains("latency_ms")) metric.latency_ms = j["latency_ms"];
    if (j.contains("throughput")) metric.throughput = j["throughput"];
    if (j.contains("input_tokens")) metric.input_tokens = j["input_tokens"];
    if (j.contains("output_tokens")) metric.output_tokens = j["output_tokens"];
    if (j.contains("has_error")) metric.has_error = j["has_error"];
    if (j.contains("error_message")) metric.error_message = j["error_message"];
    if (j.contains("consecutive_errors")) metric.consecutive_errors = j["consecutive_errors"];
    if (j.contains("prompt")) metric.prompt = j["prompt"];
    if (j.contains("response")) metric.response = j["response"];
    if (j.contains("expected_response")) metric.expected_response = j["expected_response"];
    if (j.contains("metadata")) metric.metadata = j["metadata"];
    
    metric.timestamp = std::chrono::system_clock::now();
    return metric;
}

// ============================================================================
// AdapterVersionMetrics Implementation
// ============================================================================

json AdapterVersionMetrics::toJSON() const {
    auto time_t = std::chrono::system_clock::to_time_t(deployment_time);
    return json{
        {"version", version},
        {"deployment_time", time_t},
        {"avg_accuracy", avg_accuracy},
        {"avg_latency_ms", avg_latency_ms},
        {"total_queries", total_queries},
        {"total_errors", total_errors},
        {"error_rate", error_rate},
        {"confidence_score", confidence_score},
        {"is_degraded", is_degraded}
    };
}

AdapterVersionMetrics AdapterVersionMetrics::fromJSON(const json& j) {
    AdapterVersionMetrics metrics;
    if (j.contains("version")) metrics.version = j["version"];
    if (j.contains("avg_accuracy")) metrics.avg_accuracy = j["avg_accuracy"];
    if (j.contains("avg_latency_ms")) metrics.avg_latency_ms = j["avg_latency_ms"];
    if (j.contains("total_queries")) metrics.total_queries = j["total_queries"];
    if (j.contains("total_errors")) metrics.total_errors = j["total_errors"];
    if (j.contains("error_rate")) metrics.error_rate = j["error_rate"];
    if (j.contains("confidence_score")) metrics.confidence_score = j["confidence_score"];
    if (j.contains("is_degraded")) metrics.is_degraded = j["is_degraded"];
    
    metrics.deployment_time = std::chrono::system_clock::now();
    return metrics;
}

// ============================================================================
// TelemetryFeedbackAdapter Implementation
// ============================================================================

TelemetryFeedbackAdapter::TelemetryFeedbackAdapter(const Config& config)
    : config_(config) {
    spdlog::debug("TelemetryFeedbackAdapter initialized with accuracy threshold: {}", 
                  config_.min_accuracy_threshold);
}

std::optional<Feedback> TelemetryFeedbackAdapter::recordMetric(const TelemetryMetrics& metric) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Record the metric
    metrics_by_adapter_[metric.adapter_id].push_back(metric);
    
    // Enforce max history size
    if (metrics_by_adapter_[metric.adapter_id].size() > config_.max_metrics_history) {
        metrics_by_adapter_[metric.adapter_id].erase(
            metrics_by_adapter_[metric.adapter_id].begin()
        );
    }
    
    // Check if quality issue detected
    std::optional<Feedback> feedback_entry;
    if (isQualityIssue(metric)) {
        feedback_entry = generateFeedbackFromMetric(metric);
        generated_feedback_.push_back(*feedback_entry);
        
        spdlog::warn("Quality issue detected for adapter '{}': accuracy={}, latency_ms={}",
                     metric.adapter_id, metric.accuracy, metric.latency_ms);
    }
    
    // Prune old metrics
    pruneOldMetrics();
    
    return feedback_entry;
}

std::vector<TelemetryMetrics> TelemetryFeedbackAdapter::getMetricsForAdapter(
    const std::string& adapter_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = metrics_by_adapter_.find(adapter_id);
    if (it != metrics_by_adapter_.end()) {
        return it->second;
    }
    return {};
}

AdapterVersionMetrics TelemetryFeedbackAdapter::computeVersionMetrics(
    const std::string& adapter_id,
    const std::optional<std::string>& version_to_check
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    AdapterVersionMetrics metrics;
    metrics.version = version_to_check.value_or("current");
    metrics.deployment_time = std::chrono::system_clock::now();
    
    auto it = metrics_by_adapter_.find(adapter_id);
    if (it == metrics_by_adapter_.end() || it->second.empty()) {
        return metrics;
    }
    
    const auto& adapter_metrics = it->second;
    
    // Compute aggregates
    float accuracy_sum = 0.0f;
    float latency_sum = 0.0f;
    int error_count = 0;
    
    for (const auto& m : adapter_metrics) {
        accuracy_sum += m.accuracy;
        latency_sum += m.latency_ms;
        if (m.has_error) {
            error_count++;
        }
    }
    
    metrics.total_queries = adapter_metrics.size();
    metrics.total_errors = error_count;
    metrics.error_rate = metrics.total_queries > 0 
        ? static_cast<float>(error_count) / metrics.total_queries 
        : 0.0f;
    metrics.avg_accuracy = accuracy_sum / adapter_metrics.size();
    metrics.avg_latency_ms = latency_sum / adapter_metrics.size();
    
    // Compute confidence based on consistency
    float accuracy_variance = 0.0f;
    for (const auto& m : adapter_metrics) {
        accuracy_variance += (m.accuracy - metrics.avg_accuracy) * (m.accuracy - metrics.avg_accuracy);
    }
    accuracy_variance /= adapter_metrics.size();
    float accuracy_std_dev = std::sqrt(accuracy_variance);
    metrics.confidence_score = std::max(0.0f, 1.0f - accuracy_std_dev);
    
    return metrics;
}

bool TelemetryFeedbackAdapter::isQualityDegraded(
    const std::string& adapter_id,
    const AdapterVersionMetrics& baseline_metrics
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto current = computeVersionMetrics(adapter_id);
    
    // Check accuracy drop
    float accuracy_drop = baseline_metrics.avg_accuracy - current.avg_accuracy;
    if (accuracy_drop > (baseline_metrics.avg_accuracy * 0.1f)) {  // 10% relative drop
        return true;
    }
    
    // Check error rate increase
    if (current.error_rate > baseline_metrics.error_rate * 2.0f) {
        return true;
    }
    
    // Check latency increase
    float latency_increase = current.avg_latency_ms / baseline_metrics.avg_latency_ms;
    if (latency_increase > 1.3f) {  // 30% increase
        return true;
    }
    
    return false;
}

std::vector<Feedback> TelemetryFeedbackAdapter::getGeneratedFeedback(
    const std::optional<std::string>& adapter_id,
    const std::optional<std::chrono::system_clock::time_point>& since_timestamp
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Feedback> result;
    
    for (const auto& feedback : generated_feedback_) {
        // Filter by adapter if specified
        if (adapter_id && feedback.adapter_id != *adapter_id) {
            continue;
        }
        
        // Filter by timestamp if specified
        if (since_timestamp && feedback.timestamp < *since_timestamp) {
            continue;
        }
        
        result.push_back(feedback);
    }
    
    return result;
}

void TelemetryFeedbackAdapter::clearMetricsHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_by_adapter_.clear();
    generated_feedback_.clear();
    baseline_metrics_.clear();
}

void TelemetryFeedbackAdapter::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

// ============================================================================
// Private Methods
// ============================================================================

bool TelemetryFeedbackAdapter::isQualityIssue(const TelemetryMetrics& metric) const {
    // Multiple quality issue indicators
    
    // 1. Accuracy too low
    if (metric.accuracy < config_.min_accuracy_threshold && metric.accuracy > 0.0f) {
        return true;
    }
    
    // 2. Latency too high
    if (metric.latency_ms > config_.max_latency_ms) {
        return true;
    }
    
    // 3. Explicit error
    if (metric.has_error) {
        return true;
    }
    
    // 4. Too many consecutive errors
    if (metric.consecutive_errors >= config_.max_consecutive_errors) {
        return true;
    }
    
    return false;
}

Feedback TelemetryFeedbackAdapter::generateFeedbackFromMetric(const TelemetryMetrics& metric) {
    // Generate UUID for feedback ID using Boost
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    std::stringstream ss;
    ss << uuid;
    
    Feedback feedback;
    feedback.id = ss.str();
    feedback.adapter_id = metric.adapter_id;
    feedback.user_id = "telemetry_system";
    
    feedback.prompt = metric.prompt;
    feedback.response = metric.response;
    feedback.timestamp = metric.timestamp;
    
    // Set rating based on accuracy
    if (metric.accuracy < 0.5f) {
        feedback.rating = 1;  // Poor
        feedback.training_category = "negative";
    } else if (metric.accuracy < 0.7f) {
        feedback.rating = 2;  // Fair
        feedback.training_category = "negative";
    } else if (metric.accuracy < 0.85f) {
        feedback.rating = 3;  // Good
        feedback.training_category = "neutral";
    } else {
        feedback.rating = 4;  // Very good
        feedback.training_category = "positive";
    }
    
    // Set training weight based on confidence
    feedback.training_weight = 0.7f;  // Telemetry feedback is weighted lower than user feedback
    feedback.flagged_for_training = true;
    
    // Add metadata
    feedback.custom_metadata = metric.metadata;
    feedback.custom_metadata["source"] = "telemetry";
    feedback.custom_metadata["latency_ms"] = metric.latency_ms;
    feedback.custom_metadata["perplexity"] = metric.perplexity;
    
    return feedback;
}

void TelemetryFeedbackAdapter::pruneOldMetrics() {
    auto cutoff_time = std::chrono::system_clock::now() - config_.aggregation_window;
    
    for (auto& [adapter_id, metrics] : metrics_by_adapter_) {
        // Remove metrics older than aggregation window
        metrics.erase(
            std::remove_if(
                metrics.begin(),
                metrics.end(),
                [cutoff_time](const TelemetryMetrics& m) { return m.timestamp < cutoff_time; }
            ),
            metrics.end()
        );
    }
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
