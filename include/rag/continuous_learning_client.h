/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            continuous_learning_client.h                       ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:41:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     240                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file continuous_learning_client.h
 * @brief Client for logging quality metrics to continuous learning system
 * 
 * Provides an interface for the quality control pipeline to log metrics
 * to the continuous learning orchestrator for automatic optimization.
 */

#pragma once

#include "rag/quality_control_pipeline.h"
#include <string>
#include <memory>
#include <chrono>
#include <functional>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis::rag::judge {

enum class QCDecision {
    ACCEPT,
    WARN,
    REJECT
};

enum class QCMode {
    FAST,
    BALANCED,
    THOROUGH
};

struct QCResult {
    double overall_score = 0.0;
    double faithfulness_score = 0.0;
    double relevance_score = 0.0;
    double completeness_score = 0.0;
    double coherence_score = 0.0;
    QCDecision decision = QCDecision::WARN;
    QCMode mode = QCMode::BALANCED;
    std::chrono::milliseconds latency{0};
    bool passed_threshold = false;
};

/**
 * @brief Metric types for continuous learning
 */
enum class MetricType {
    FAITHFULNESS,      ///< Faithfulness score
    RELEVANCE,         ///< Relevance score
    COMPLETENESS,      ///< Completeness score
    COHERENCE,         ///< Coherence score
    OVERALL_QUALITY,   ///< Overall quality score
    LATENCY,           ///< Processing latency
    DECISION,          ///< Quality control decision
    NLI_ACCURACY,      ///< NLI verification accuracy
    GEVAL_VARIANCE     ///< G-Eval score variance
};

/**
 * @brief Quality metric for continuous learning
 */
struct QualityMetric {
    MetricType type;
    double value;
    std::string context;           ///< Additional context (e.g., query, model)
    std::chrono::system_clock::time_point timestamp;
    
    // Optional detailed information
    nlohmann::json metadata;       ///< Additional metadata
};

/**
 * @brief Trigger conditions for optimization
 */
struct OptimizationTrigger {
    std::string trigger_type;      ///< "low_faithfulness", "low_relevance", etc.
    double threshold;              ///< Threshold that was violated
    double current_value;          ///< Current metric value
    size_t sample_count;           ///< Number of samples in window
    std::string recommendation;    ///< Recommended action
};

/**
 * @brief Continuous Learning Client
 * 
 * Logs quality control metrics to the continuous learning system
 * for automatic prompt optimization, retrieval tuning, and model fine-tuning.
 */
class ContinuousLearningClient {
public:
    /**
     * @brief Configuration for continuous learning client
     */
    struct Config {
        std::string endpoint;          ///< CL service endpoint
        bool enable_logging = true;    ///< Enable metric logging
        bool enable_triggers = true;   ///< Enable optimization triggers
        
        // Trigger thresholds
        double faithfulness_threshold = 0.75;
        double relevance_threshold = 0.70;
        double overall_quality_threshold = 0.70;
        
        // Window configuration
        size_t metric_window_size = 100;  ///< Samples to consider for triggers
        
        // Batch configuration
        bool enable_batching = true;      ///< Batch metrics before sending
        size_t batch_size = 10;           ///< Metrics per batch
        int batch_timeout_ms = 5000;      ///< Max time to wait for batch
    };
    
    /**
     * @brief Construct client with configuration
     * @param config Client configuration
     */
    ContinuousLearningClient();
    explicit ContinuousLearningClient(const Config& config);
    
    /**
     * @brief Destructor - flushes any pending metrics
     */
    ~ContinuousLearningClient();
    
    /**
     * @brief Log quality control result
     * @param result Quality control result
     */
    void logQCResult(const QCResult& result);
    
    /**
     * @brief Log individual metric
     * @param metric Quality metric
     */
    void logMetric(const QualityMetric& metric);
    
    /**
     * @brief Log batch of metrics
     * @param metrics Vector of metrics
     */
    void logMetricsBatch(const std::vector<QualityMetric>& metrics);
    
    /**
     * @brief Check if optimization should be triggered
     * @return Trigger information if triggered, nullptr otherwise
     */
    std::unique_ptr<OptimizationTrigger> checkTriggers();
    
    /**
     * @brief Flush pending metrics immediately
     */
    void flush();
    
    /**
     * @brief Get statistics
     */
    struct Statistics {
        size_t metrics_logged = 0;
        size_t metrics_sent = 0;
        size_t triggers_fired = 0;
        size_t batch_count = 0;
        std::chrono::system_clock::time_point last_flush;
    };
    Statistics getStatistics() const;
    
    /**
     * @brief Set callback for optimization triggers
     * @param callback Function called when trigger fires
     */
    void setTriggerCallback(
        std::function<void(const OptimizationTrigger&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Internal methods
    void sendMetrics(const std::vector<QualityMetric>& metrics);
    void evaluateTriggers();
    OptimizationTrigger createTrigger(
        const std::string& type,
        double threshold,
        double current_value,
        const std::string& recommendation
    );
};

/**
 * @brief Helper functions for continuous learning integration
 */
namespace cl_utils {

/**
 * @brief Convert QC result to metrics
 * @param result Quality control result
 * @return Vector of quality metrics
 */
std::vector<QualityMetric> qcResultToMetrics(const QCResult& result);

/**
 * @brief Create recommendation based on low scores
 * @param result Quality control result
 * @return Optimization recommendation
 */
std::string generateRecommendation(const QCResult& result);

/**
 * @brief Convert metric type to string
 * @param type Metric type
 * @return String representation
 */
std::string metricTypeToString(MetricType type);

} // namespace cl_utils

} // namespace themis::rag::judge
