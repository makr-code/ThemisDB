/**
 * @file telemetry_feedback_adapter.h
 * @brief Converts telemetry metrics to feedback entries for continuous learning
 * 
 * This component bridges the gap between system telemetry (latency, accuracy, 
 * error rates) and the feedback system, enabling automatic retraining triggers
 * based on observed performance degradation.
 */

#pragma once

#include "lora_feedback.h"
#include <memory>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

/**
 * @brief Telemetry metrics from inference/query operations
 */
struct TelemetryMetrics {
    virtual ~TelemetryMetrics() = default;
    
    // Time and ID
    std::chrono::system_clock::time_point timestamp;
    std::string adapter_id;
    std::string request_id;
    
    // Quality metrics
    float accuracy = 0.0f;              // Prediction accuracy (0-1)
    float precision = 0.0f;             // Precision metric (0-1)
    float recall = 0.0f;                // Recall metric (0-1)
    float f1_score = 0.0f;              // F1 score (0-1)
    float perplexity = 0.0f;            // Model perplexity
    
    // Performance metrics
    float latency_ms = 0.0f;            // Query latency in ms
    float throughput = 0.0f;            // Queries per second
    int input_tokens = 0;
    int output_tokens = 0;
    
    // Error metrics
    bool has_error = false;
    std::string error_message;
    int consecutive_errors = 0;
    
    // Context
    std::string prompt;
    std::string response;
    std::string expected_response;
    json metadata;
    
    json toJSON() const;
    static TelemetryMetrics fromJSON(const json& j);
};

/**
 * @brief Adapter version metrics for quality tracking
 */
struct AdapterVersionMetrics {
    virtual ~AdapterVersionMetrics() = default;
    
    std::string version;
    std::chrono::system_clock::time_point deployment_time;
    
    // Aggregate metrics
    float avg_accuracy = 0.0f;
    float avg_latency_ms = 0.0f;
    int total_queries = 0;
    int total_errors = 0;
    float error_rate = 0.0f;
    
    // Quality indicators
    float confidence_score = 0.0f;      // Overall confidence
    bool is_degraded = false;           // Flag if quality below threshold
    
    json toJSON() const;
    static AdapterVersionMetrics fromJSON(const json& j);
};

/**
 * @brief Converts telemetry into actionable feedback for training
 * 
 * Key responsibilities:
 * 1. Aggregate telemetry from multiple sources
 * 2. Detect quality degradation patterns
 * 3. Generate synthetic feedback entries for negative patterns
 * 4. Compute decision metrics for retraining triggers
 * 
 * Thread-safe for concurrent metric collection.
 */
class TelemetryFeedbackAdapter {
public:
    struct Config {
        // Thresholds for feedback generation
        float min_accuracy_threshold = 0.85f;
        float max_latency_ms = 500.0f;
        int max_consecutive_errors = 3;
        float error_rate_threshold = 0.1f;  // 10% error rate
        
        // Aggregation window
        std::chrono::seconds aggregation_window{300};  // 5 minutes
        
        // Metrics retention
        size_t max_metrics_history = 1000;
    };
    
    explicit TelemetryFeedbackAdapter(const Config& config = Config{});
    ~TelemetryFeedbackAdapter() = default;
    
    /**
     * @brief Record a telemetry metric
     * 
     * @param metric The telemetry metric to record
     * @return Optional feedback entry if quality issue detected
     */
    std::optional<Feedback> recordMetric(const TelemetryMetrics& metric);
    
    /**
     * @brief Get all metrics for an adapter in aggregation window
     * 
     * @param adapter_id Adapter to query
     * @return List of recent telemetry metrics
     */
    std::vector<TelemetryMetrics> getMetricsForAdapter(const std::string& adapter_id) const;
    
    /**
     * @brief Compute aggregate metrics for version comparison
     * 
     * @param adapter_id Adapter to analyze
     * @param version_to_check Version to evaluate (optional)
     * @return Aggregate metrics for the version
     */
    AdapterVersionMetrics computeVersionMetrics(
        const std::string& adapter_id,
        const std::optional<std::string>& version_to_check = std::nullopt
    ) const;
    
    /**
     * @brief Check if adapter quality has degraded significantly
     * 
     * @param adapter_id Adapter to check
     * @param baseline_metrics Baseline metrics to compare against
     * @return true if quality degradation detected
     */
    bool isQualityDegraded(
        const std::string& adapter_id,
        const AdapterVersionMetrics& baseline_metrics
    ) const;
    
    /**
     * @brief Get feedback entries generated from telemetry
     * 
     * @param adapter_id Filter by adapter (optional)
     * @param since_timestamp Only entries after this time
     * @return Vector of generated feedback entries
     */
    std::vector<Feedback> getGeneratedFeedback(
        const std::optional<std::string>& adapter_id = std::nullopt,
        const std::optional<std::chrono::system_clock::time_point>& since_timestamp = std::nullopt
    ) const;
    
    /**
     * @brief Clear metrics history (for testing or cleanup)
     */
    void clearMetricsHistory();
    
    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration at runtime
     */
    void setConfig(const Config& config);
    
private:
    Config config_;
    mutable std::mutex mutex_;
    
    // Metrics tracking per adapter
    std::unordered_map<std::string, std::vector<TelemetryMetrics>> metrics_by_adapter_;
    
    // Generated feedback entries
    std::vector<Feedback> generated_feedback_;
    
    // Last baseline metrics per adapter for degradation detection
    std::unordered_map<std::string, AdapterVersionMetrics> baseline_metrics_;
    
    /**
     * @brief Check if metric indicates quality issue
     * 
     * @return true if quality issue detected
     */
    bool isQualityIssue(const TelemetryMetrics& metric) const;
    
    /**
     * @brief Generate feedback entry from telemetry metric
     */
    Feedback generateFeedbackFromMetric(const TelemetryMetrics& metric);
    
    /**
     * @brief Clean up old metrics outside aggregation window
     */
    void pruneOldMetrics();
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
