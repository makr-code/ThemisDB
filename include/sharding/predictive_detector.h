/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            predictive_detector.h                              ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:38:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

struct ShardMetrics {
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
    void recordMetrics(const ShardMetrics& metrics);
    std::vector<ShardMetrics> getMetricsHistory(const std::string& shard_id, 
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
    
private:
    // Background monitoring
    void monitoringLoop();
    void checkAllShards();
    
    // Feature extraction
    std::vector<float> extractFeatures(const std::string& shard_id);
    std::vector<float> computeStatisticalFeatures(const std::vector<ShardMetrics>& history);
    
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
    std::map<std::string, std::vector<ShardMetrics>> metrics_history_;
    
    // Prediction cache
    mutable std::mutex predictions_mutex_;
    std::map<std::string, FailurePrediction> cached_predictions_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    Stats stats_;
    
    // ML model handle (opaque pointer for ONNX Runtime)
    struct ModelImpl;
    std::unique_ptr<ModelImpl> model_;
};

} // namespace sharding
} // namespace themisdb
