/**
 * @file predictive_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Predictive Failure Detection
 * 
 * ML-based system to predict shard failures before they occur:
 * - SMART metrics monitoring
 * - Performance degradation detection
 * - Proactive failure prediction
 * - Integration with auto-recovery system
 */

#pragma once

#include "sharding/redundancy_strategy.h"
#include "sharding/shard_topology.h"
#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace themisdb {
namespace sharding {

// ═══════════════════════════════════════════════════════════
// Configuration
// ═══════════════════════════════════════════════════════════

struct PredictiveConfig {
    bool enabled = false;
    std::string model_path;  // Path to ONNX model file
    std::chrono::seconds check_interval{3600};  // Default: 1 hour
    float failure_threshold = 0.7f;  // 70% probability threshold
    uint32_t lookback_days = 30;  // Historical data window
    
    // Alert configuration
    bool enable_alerts = true;
    std::function<void(const std::string&)> alert_callback;
    
    // Feature extraction
    bool collect_smart_metrics = true;
    bool collect_performance_metrics = true;
    bool collect_io_patterns = true;
};

// ═══════════════════════════════════════════════════════════
// Shard Metrics
// ═══════════════════════════════════════════════════════════

struct PredictiveShardMetrics {
    std::string shard_id;
    std::chrono::system_clock::time_point timestamp;
    
    // SMART metrics (if available)
    std::map<std::string, double> smart_attributes;
    
    // Performance metrics
    double avg_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double p99_latency_ms = 0.0;
    uint64_t throughput_ops_per_sec = 0;
    
    // I/O patterns
    uint64_t read_errors = 0;
    uint64_t write_errors = 0;
    uint64_t retry_count = 0;
    
    // Health check metrics
    uint32_t failed_health_checks = 0;
    uint32_t recovery_attempts = 0;
    float recovery_success_rate = 1.0f;
};

// ═══════════════════════════════════════════════════════════
// Failure Prediction
// ═══════════════════════════════════════════════════════════

struct FailurePrediction {
    std::string shard_id;
    float failure_probability;  // 0.0 to 1.0
    uint32_t predicted_days_to_failure;  // 7, 14, or 30 days
    std::chrono::system_clock::time_point prediction_time;
    
    // Contributing factors
    std::map<std::string, float> feature_importance;
    
    bool isHighRisk() const {
        return failure_probability > 0.7f;
    }
};

// ═══════════════════════════════════════════════════════════
// Predictive Failure Detector
// ═══════════════════════════════════════════════════════════

class PredictiveFailureDetector {
public:
    explicit PredictiveFailureDetector(
        const PredictiveConfig& config,
        RedundancyStrategy& strategy,
        ShardTopology& topology
    );
    
    ~PredictiveFailureDetector();
    
    // Lifecycle
    void start();
    void stop();
    bool isRunning() const;
    
    // Prediction interface
    std::vector<FailurePrediction> getPredictions();
    std::vector<FailurePrediction> getHighRiskShards();
    FailurePrediction predictShard(const std::string& shard_id);
    
    // Metrics collection
    void recordMetrics(const PredictiveShardMetrics& metrics);
    std::vector<PredictiveShardMetrics> getMetricsHistory(const std::string& shard_id, 
                                                 std::chrono::hours lookback) const;
    
    // Statistics
    struct Stats {
        uint64_t predictions_made = 0;
        uint64_t high_risk_detected = 0;
        uint64_t alerts_sent = 0;
        uint64_t true_positives = 0;  // Predicted failures that occurred
        uint64_t false_positives = 0;  // Predicted failures that didn't occur
        std::chrono::milliseconds avg_inference_time{0};
        
        float getTruePositiveRate() const {
            uint64_t total = true_positives + false_positives;
            return total > 0 ? static_cast<float>(true_positives) / total : 0.0f;
        }
        
        float getFalsePositiveRate() const {
            uint64_t total = true_positives + false_positives;
            return total > 0 ? static_cast<float>(false_positives) / total : 0.0f;
        }
    };
    
    Stats getStats() const;
    void resetStats();

    // ─── ONNX model prediction injection (stub #251) ─────────────────────────
    /**
     * @brief Type alias for an injectable ONNX-style prediction function.
     *
     * When set via @c setPredictFn(), @c predictShard() calls this function
     * in place of the built-in sigmoid-calibrated heuristic.  The function
     * receives the extracted feature vector and must return a two-element
     * vector @c {failure_probability, days_to_failure}.
     *
     * Example (test injection):
     * @code
     *   detector.setPredictFn([](const std::vector<float>&) -> std::vector<float> {
     *       return {0.9f, 1.0f};  // high-risk, 1 day
     *   });
     * @endcode
     */
    using PredictFn = std::function<std::vector<float>(const std::vector<float>&)>;

    /**
     * @brief Inject an ONNX-backed (or test-double) prediction function.
     *
     * Replaces the heuristic fallback with @p fn for all subsequent
     * @c predictShard() calls.  Passing @c nullptr resets to the heuristic.
     * Thread-safe: guarded by an internal mutex.
     *
     * @param fn Callable that maps a feature vector to {probability, days}.
     */
    void setPredictFn(PredictFn fn);
    
private:
    // Background monitoring
    void monitoringLoop();
    void checkAllShards();
    
    // Feature extraction
    std::vector<float> extractFeatures(const std::string& shard_id);
    std::vector<float> computeStatisticalFeatures(const std::vector<PredictiveShardMetrics>& history);
    
    // ML inference
    FailurePrediction runInference(const std::string& shard_id, 
                                   const std::vector<float>& features);
    bool loadModel(const std::string& model_path);
    
    // Alerting
    void sendAlert(const FailurePrediction& prediction);
    
    // Configuration and dependencies
    PredictiveConfig config_;
    RedundancyStrategy& strategy_;
    ShardTopology& topology_;
    
    // Threading
    std::atomic<bool> running_{false};
    std::thread monitoring_thread_;
    
    // Metrics storage
    mutable std::mutex metrics_mutex_;
    std::map<std::string, std::vector<PredictiveShardMetrics>> metrics_history_;
    
    // Prediction cache
    mutable std::mutex predictions_mutex_;
    std::map<std::string, FailurePrediction> cached_predictions_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // ML model handle (opaque pointer for ONNX Runtime)
    struct ModelImpl;
    std::unique_ptr<ModelImpl> model_;

    // Injection slot for ONNX / test-double predict function (stub #251)
    PredictFn predict_fn_;
    mutable std::mutex predict_fn_mutex_;
};

} // namespace sharding
} // namespace themisdb
