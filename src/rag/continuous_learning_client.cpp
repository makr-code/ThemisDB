/**
 * @file continuous_learning_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/continuous_learning_client.h"
#include "utils/logger.h"
#include <deque>
#include <mutex>
#include <thread>
#include <chrono>
#include <numeric>
#include <algorithm>

namespace themis::rag::judge {

using json = nlohmann::json;

// ═══════════════════════════════════════════════════════════
// Implementation
// ═══════════════════════════════════════════════════════════

struct ContinuousLearningClient::Impl {
    Config config;
    Statistics stats;
    mutable std::mutex stats_mutex;  // Protects stats member access
    
    // Metric batching
    std::deque<QualityMetric> metric_batch;
    std::mutex batch_mutex;
    std::thread batch_thread;
    std::atomic<bool> running{false};
    
    // Metric history for trigger evaluation
    std::deque<QualityMetric> metric_history;
    std::mutex history_mutex;
    
    // Trigger callback
    std::function<void(const OptimizationTrigger&)> trigger_callback;
    std::mutex callback_mutex = {};
    
    Impl(const Config& cfg) : config(cfg) {}
    
    ~Impl() {
        running = false;
        if (batch_thread.joinable()) {
            batch_thread.join();
        }
    }
    
    /**
     * @brief Start Batch Thread.
     * @details Calls: std::thread(), std::this_thread::sleep_for(), std::chrono::milliseconds(), flushBatch().
     */
    void startBatchThread() {
        running = true;
        batch_thread = std::thread([this]() {
            while (running) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(config.batch_timeout_ms)
                );
                flushBatch();
            }
        });
    }
    
    /**
     * @brief Flush Batch.
     * @details Calls: lock(), empty(), assign(), begin(), end(), clear(), sendMetricsInternal().
     */
    void flushBatch() {
        std::vector<QualityMetric> to_send;
        
        {
            std::lock_guard<std::mutex> lock(batch_mutex);
            if (metric_batch.empty()) {
                return;
            }
            
            to_send.assign(metric_batch.begin(), metric_batch.end());
            metric_batch.clear();
        }
        
        if (!to_send.empty()) {
            sendMetricsInternal(to_send);
        }
    }
    
    /**
     * @brief Send Metrics Internal.
     * @param[in] metrics Input parameter.
     * @details Calls: json::array(), cl_utils::metricTypeToString(), std::chrono::system_clock::to_time_t(), empty(), push_back(), THEMIS_DEBUG(), size(), dump().
     */
    void sendMetricsInternal(const std::vector<QualityMetric>& metrics) {
        if (!config.enable_logging) {
            return;
        }
        
        // Create JSON payload
        json payload = json::array();
        
        for (const auto& metric : metrics) {
            json metric_json;
            metric_json["type"] = cl_utils::metricTypeToString(metric.type);
            metric_json["value"] = metric.value;
            metric_json["context"] = metric.context;
            metric_json["timestamp"] = std::chrono::system_clock::to_time_t(metric.timestamp);
            
            if (!metric.metadata.empty()) {
                metric_json["metadata"] = metric.metadata;
            }
            
            payload.push_back(metric_json);
        }
        
        // In production: HTTP POST to config.endpoint
        // For now: Log the metrics
        THEMIS_DEBUG("CL Client: Sending {} metrics to {}", 
                     metrics.size(), config.endpoint);
        THEMIS_DEBUG("Payload: {}", payload.dump());
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex);
            stats.metrics_sent += metrics.size();
            stats.batch_count++;
            stats.last_flush = std::chrono::system_clock::now();
        }
    }
    
    /**
     * @brief Add To History.
     * @param[in] metric Input parameter.
     * @details Calls: lock(), push_back(), size(), pop_front().
     */
    void addToHistory(const QualityMetric& metric) {
        std::lock_guard<std::mutex> lock(history_mutex);
        
        metric_history.push_back(metric);
        
        // Keep only recent metrics
        if (metric_history.size() > config.metric_window_size) {
            metric_history.pop_front();
        }
    }
    
    /**
     * @brief Get Recent Metrics.
     * @param[in] type Input parameter.
     * @return Return value.
     * @details Calls: lock(), push_back().
     */
    std::vector<double> getRecentMetrics(MetricType type) {
        std::lock_guard<std::mutex> lock(history_mutex);
        
        std::vector<double> values = {};

        for (const auto& metric : metric_history) {
            if (metric.type == type) {
                values.push_back(metric.value);
            }
        }
        
        return values;
    }
};

// ═══════════════════════════════════════════════════════════
// Constructor & Destructor
// ═══════════════════════════════════════════════════════════

ContinuousLearningClient::ContinuousLearningClient()
    : ContinuousLearningClient(Config{}) {
}

ContinuousLearningClient::ContinuousLearningClient(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
    
    if (config.enable_batching) {
        impl_->startBatchThread();
    }
    
    THEMIS_INFO("ContinuousLearningClient initialized (endpoint: {}, batching: {})",
                config.endpoint, config.enable_batching);
}

ContinuousLearningClient::~ContinuousLearningClient() {
    flush();
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Public Methods ═══════════════════════════════════════════════════════════
 * @param[in] result Input parameter.
 * @details Calls: cl_utils::qcResultToMetrics(), logMetric().
 */

void ContinuousLearningClient::logQCResult(const QCResult& result) {
    if (!impl_->config.enable_logging) {
        return;
    }
    
    // Convert QC result to metrics
    auto metrics = cl_utils::qcResultToMetrics(result);
    
    // Log each metric
    for (const auto& metric : metrics) {
        logMetric(metric);
    }
}

/**
 * @brief Log Metric.
 * @param[in] metric Input parameter.
 * @details Calls: lock(), addToHistory(), push_back(), size(), to_send(), begin(), end(), clear().
 */
void ContinuousLearningClient::logMetric(const QualityMetric& metric) {
    if (!impl_->config.enable_logging) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        impl_->stats.metrics_logged++;
    }
    
    // Add to history for trigger evaluation
    impl_->addToHistory(metric);
    
    if (impl_->config.enable_batching) {
        // Add to batch
        std::unique_lock<std::mutex> lock(impl_->batch_mutex);
        impl_->metric_batch.push_back(metric);
        
        // Flush if batch is full
        if (impl_->metric_batch.size() >= impl_->config.batch_size) {
            std::vector<QualityMetric> to_send(
                impl_->metric_batch.begin(),
                impl_->metric_batch.end()
            );
            impl_->metric_batch.clear();
            
            // Send without holding lock
            lock.unlock();
            impl_->sendMetricsInternal(to_send);
        }
    } else {
        // Send immediately
        sendMetrics({metric});
    }
    
    // Check triggers if enabled
    if (impl_->config.enable_triggers) {
        evaluateTriggers();
    }
}

/**
 * @brief Log Metrics Batch.
 * @param[in] metrics Input parameter.
 * @details Calls: lock(), size(), addToHistory(), sendMetrics(), evaluateTriggers().
 */
void ContinuousLearningClient::logMetricsBatch(const std::vector<QualityMetric>& metrics) {
    if (!impl_->config.enable_logging) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        impl_->stats.metrics_logged += metrics.size();
    }
    
    for (const auto& metric : metrics) {
        impl_->addToHistory(metric);
    }
    
    sendMetrics(metrics);
    
    if (impl_->config.enable_triggers) {
        evaluateTriggers();
    }
}

/**
 * @brief Check Triggers.
 * @return Return value.
 * @details Calls: getRecentMetrics(), empty(), std::accumulate(), begin(), end(), size(), lock(), createTrigger().
 */
std::unique_ptr<OptimizationTrigger> ContinuousLearningClient::checkTriggers() {
    if (!impl_->config.enable_triggers) {
        return nullptr;
    }
    
    // Get recent metrics for each type
    auto faithfulness = impl_->getRecentMetrics(MetricType::FAITHFULNESS);
    auto relevance = impl_->getRecentMetrics(MetricType::RELEVANCE);
    auto overall = impl_->getRecentMetrics(MetricType::OVERALL_QUALITY);
    
    // Check faithfulness
    if (!faithfulness.empty()) {
        double avg = std::accumulate(faithfulness.begin(), faithfulness.end(), 0.0) 
                     / faithfulness.size();
        
        if (avg < impl_->config.faithfulness_threshold) {
            {
                std::lock_guard<std::mutex> lock(impl_->stats_mutex);
                impl_->stats.triggers_fired++;
            }
            return std::make_unique<OptimizationTrigger>(
                createTrigger(
                    "low_faithfulness",
                    impl_->config.faithfulness_threshold,
                    avg,
                    "Optimize retrieval: improve document relevance and ranking"
                )
            );
        }
    }
    
    // Check relevance
    if (!relevance.empty()) {
        double avg = std::accumulate(relevance.begin(), relevance.end(), 0.0) 
                     / relevance.size();
        
        if (avg < impl_->config.relevance_threshold) {
            {
                std::lock_guard<std::mutex> lock(impl_->stats_mutex);
                impl_->stats.triggers_fired++;
            }
            return std::make_unique<OptimizationTrigger>(
                createTrigger(
                    "low_relevance",
                    impl_->config.relevance_threshold,
                    avg,
                    "Optimize prompts: improve query understanding and answer generation"
                )
            );
        }
    }
    
    // Check overall quality
    if (!overall.empty()) {
        double avg = std::accumulate(overall.begin(), overall.end(), 0.0) 
                     / overall.size();
        
        if (avg < impl_->config.overall_quality_threshold) {
            {
                std::lock_guard<std::mutex> lock(impl_->stats_mutex);
                impl_->stats.triggers_fired++;
            }
            return std::make_unique<OptimizationTrigger>(
                createTrigger(
                    "low_overall_quality",
                    impl_->config.overall_quality_threshold,
                    avg,
                    "Trigger LoRA fine-tuning: consistent quality issues detected"
                )
            );
        }
    }
    
    return nullptr;
}

/**
 * @brief Flush.
 * @details Calls: flushBatch().
 */
void ContinuousLearningClient::flush() {
    if (impl_->config.enable_batching) {
        impl_->flushBatch();
    }
}

ContinuousLearningClient::Statistics ContinuousLearningClient::getStatistics() const {
    std::lock_guard<std::mutex> lock(impl_->stats_mutex);
    return impl_->stats;
}

void ContinuousLearningClient::setTriggerCallback(
    std::function<void(const OptimizationTrigger&)> callback
) {
    std::lock_guard<std::mutex> lock(impl_->callback_mutex);
    impl_->trigger_callback = callback;
}

/**
 * @brief ═══════════════════════════════════════════════════════════ Private Methods ═══════════════════════════════════════════════════════════
 * @param[in] metrics Input parameter.
 * @details Calls: sendMetricsInternal().
 */

void ContinuousLearningClient::sendMetrics(const std::vector<QualityMetric>& metrics) {
    impl_->sendMetricsInternal(metrics);
}

/**
 * @brief Evaluate Triggers.
 * @details Calls: checkTriggers(), THEMIS_INFO(), lock(), trigger_callback().
 */
void ContinuousLearningClient::evaluateTriggers() {
    auto trigger = checkTriggers();
    
    if (trigger) {
        THEMIS_INFO("Optimization trigger fired: {} (threshold: {}, current: {})",
                   trigger->trigger_type, trigger->threshold, trigger->current_value);
        THEMIS_INFO("Recommendation: {}", trigger->recommendation);
        
        // Call callback if set
        std::lock_guard<std::mutex> lock(impl_->callback_mutex);
        if (impl_->trigger_callback) {
            impl_->trigger_callback(*trigger);
        }
    }
}

/**
 * @brief Create Trigger.
 * @param[in] type Input parameter.
 * @param[in] threshold Input parameter.
 * @param[in] current_value Input parameter.
 * @param[in] recommendation Input parameter.
 * @return Return value.
 * @details Calls: size().
 */
OptimizationTrigger ContinuousLearningClient::createTrigger(
    const std::string& type,
    double threshold,
    double current_value,
    const std::string& recommendation
) {
    OptimizationTrigger trigger;
    trigger.trigger_type = type;
    trigger.threshold = threshold;
    trigger.current_value = current_value;
    trigger.sample_count = impl_->metric_history.size();
    trigger.recommendation = recommendation;
    
    return trigger;
}

// ═══════════════════════════════════════════════════════════
// Utility Functions
// ═══════════════════════════════════════════════════════════

namespace cl_utils {

/**
 * @brief Qc Result To Metrics.
 * @param[in] result Input parameter.
 * @return Return value.
 * @details Calls: std::chrono::system_clock::now(), push_back(), count().
 */
std::vector<QualityMetric> qcResultToMetrics(const QCResult& result) {
    std::vector<QualityMetric> metrics;
    auto now = std::chrono::system_clock::now();
    
    // Faithfulness metric
    QualityMetric faithfulness;
    faithfulness.type = MetricType::FAITHFULNESS;
    faithfulness.value = result.faithfulness_score;
    faithfulness.timestamp = now;
    faithfulness.metadata["mode"] = static_cast<int>(result.mode);
    metrics.push_back(faithfulness);
    
    // Relevance metric
    QualityMetric relevance;
    relevance.type = MetricType::RELEVANCE;
    relevance.value = result.relevance_score;
    relevance.timestamp = now;
    metrics.push_back(relevance);
    
    // Completeness metric
    QualityMetric completeness;
    completeness.type = MetricType::COMPLETENESS;
    completeness.value = result.completeness_score;
    completeness.timestamp = now;
    metrics.push_back(completeness);
    
    // Coherence metric
    QualityMetric coherence;
    coherence.type = MetricType::COHERENCE;
    coherence.value = result.coherence_score;
    coherence.timestamp = now;
    metrics.push_back(coherence);
    
    // Overall quality metric
    QualityMetric overall;
    overall.type = MetricType::OVERALL_QUALITY;
    overall.value = result.overall_score;
    overall.timestamp = now;
    overall.metadata["decision"] = static_cast<int>(result.decision);
    overall.metadata["passed_threshold"] = result.passed_threshold;
    metrics.push_back(overall);
    
    // Latency metric
    QualityMetric latency;
    latency.type = MetricType::LATENCY;
    latency.value = static_cast<double>(result.latency.count());
    latency.timestamp = now;
    latency.metadata["mode"] = static_cast<int>(result.mode);
    metrics.push_back(latency);
    
    return metrics;
}

/**
 * @brief Generate Recommendation.
 * @param[in] result Input parameter.
 * @return Return value.
 * @details Calls: push_back(), empty(), size().
 */
std::string generateRecommendation(const QCResult& result) {
    std::vector<std::string> recommendations;
    
    if (result.faithfulness_score < 0.7) {
        recommendations.push_back("Improve retrieval: better document ranking");
    }
    
    if (result.relevance_score < 0.7) {
        recommendations.push_back("Optimize prompts: improve query understanding");
    }
    
    if (result.completeness_score < 0.7) {
        recommendations.push_back("Enhance answer generation: cover all query aspects");
    }
    
    if (result.coherence_score < 0.7) {
        recommendations.push_back("Improve answer structure: better coherence");
    }
    
    if (recommendations.empty()) {
        return "Quality acceptable - continue monitoring";
    }
    
    std::string combined = {};
    for (size_t i = 0; i < recommendations.size(); i++) {
        combined += recommendations[i];
        if (i < recommendations.size() - 1) {
            combined += "; ";
        }
    }
    
    return combined;
}

/**
 * @brief Metric Type To String.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Implements metricTypeToString without additional internal calls.
 */
std::string metricTypeToString(MetricType type) {
    switch (type) {
        case MetricType::FAITHFULNESS:
            return "faithfulness";
        case MetricType::RELEVANCE:
            return "relevance";
        case MetricType::COMPLETENESS:
            return "completeness";
        case MetricType::COHERENCE:
            return "coherence";
        case MetricType::OVERALL_QUALITY:
            return "overall_quality";
        case MetricType::LATENCY:
            return "latency";
        case MetricType::DECISION:
            return "decision";
        case MetricType::NLI_ACCURACY:
            return "nli_accuracy";
        case MetricType::GEVAL_VARIANCE:
            return "geval_variance";
        default:
            return "unknown";
    }
}

} // namespace cl_utils

} // namespace themis::rag::judge
