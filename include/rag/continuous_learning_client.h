/**
 * @file continuous_learning_client.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @brief Construct client with default configuration.
     */
    ContinuousLearningClient();
    /**
     * @brief Construct client with configuration.
     * @param config Client configuration.
     */
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
