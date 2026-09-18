/**
 * @file llm_metrics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/grafana_metrics.h"
#include <string>
#include <chrono>
#include <memory>
#include <mutex>

namespace themis {
namespace aql {

class LLMMetricsCollector {
public:
    LLMMetricsCollector();
    ~LLMMetricsCollector() = default;
    
    /**
     * @brief Initialize.
     */
    void initialize();
    
    void recordInference(
        const std::string& model_id,
        const std::string& lora_id,
        std::chrono::milliseconds latency,
        size_t input_tokens,
        size_t output_tokens,
        bool success,
        const std::string& error_code = ""
    );
    
    void recordRAG(
        const std::string& collection,
        const std::string& lora_id,
        std::chrono::milliseconds latency,
        size_t retrieved_docs,
        size_t input_tokens,
        size_t output_tokens,
        bool success,
        const std::string& error_code = ""
    );
    
    void recordEmbedding(
        const std::string& model_id,
        std::chrono::milliseconds latency,
        size_t input_tokens,
        bool success,
        const std::string& error_code = ""
    );
    
    void recordAQLValidation(
        bool success,
        std::chrono::milliseconds duration,
        const std::string& error_reason = ""  // e.g., "parse_error", "timeout", "exception"
    );
    
    void recordAQLGenerationAttempt(
        bool success,
        int attempt_number,
        std::chrono::milliseconds duration,
        const std::string& outcome = ""  // e.g., "success", "parse_error", "max_retries_exceeded"
    );
    
    /**
     * @brief Record Validation Retry.
     * @param[in] retry_succeeded Input parameter.
     * @param[in] attempt_number Input parameter.
     */
    void recordValidationRetry(
        bool retry_succeeded,
        int attempt_number
    );
    
    /**
     * @brief Record Cache Access.
     * @param[in] cache_type Input parameter.
     * @param[in] hit Input parameter.
     */
    void recordCacheAccess(
        const std::string& cache_type,  // "prefix" or "response"
        bool hit
    );
    
    /**
     * @brief Update Model Memory.
     * @param[in] model_id Identifier of the model.
     * @param[in] memory_bytes Input parameter.
     */
    void updateModelMemory(
        const std::string& model_id,
        size_t memory_bytes
    );
    
    /**
     * @brief Record Circuit Breaker State.
     * @param[in] operation Input parameter.
     * @param[in] state Input parameter.
     */
    void recordCircuitBreakerState(
        const std::string& operation,
        const std::string& state  // "open", "closed", "half_open"
    );
    
    /**
     * @brief Instance.
     * @return Return value.
     */
    static LLMMetricsCollector& instance();

private:
    std::shared_ptr<llm::monitoring::PrometheusExporter> exporter_;
    std::mutex mutex_;
    bool initialized_ = false;
    
    /**
     * @brief Register Metrics.
     */
    void registerMetrics();
    std::unordered_map<std::string, std::string> makeLabels(
        const std::string& operation,
        const std::string& model = "",
        const std::string& status = ""
    );
};

class ScopedLatencyTracker {
public:
    ScopedLatencyTracker()
        : start_(std::chrono::steady_clock::now())
    {}
    
    std::chrono::milliseconds elapsed() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
    }

private:
    std::chrono::steady_clock::time_point start_;
};

} // namespace aql
} // namespace themis
