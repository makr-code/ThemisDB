/**
 * @file llm_metrics_collector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Metrics collector for LLM operations
 * 
 * Integrates with Prometheus/Grafana for comprehensive LLM observability.
 * Tracks latency, throughput, errors, cache hits, and resource usage.
 */
class LLMMetricsCollector {
public:
    LLMMetricsCollector();
    ~LLMMetricsCollector() = default;
    
    /**
     * @brief Initialize metrics registry with all LLM metrics
     */
    void initialize();
    
    /**
     * @brief Record inference operation metrics
     */
    void recordInference(
        const std::string& model_id,
        const std::string& lora_id,
        std::chrono::milliseconds latency,
        size_t input_tokens,
        size_t output_tokens,
        bool success,
        const std::string& error_code = ""
    );
    
    /**
     * @brief Record RAG operation metrics
     */
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
    
    /**
     * @brief Record embedding operation metrics
     */
    void recordEmbedding(
        const std::string& model_id,
        std::chrono::milliseconds latency,
        size_t input_tokens,
        bool success,
        const std::string& error_code = ""
    );
    
    /**
     * @brief Record AQL parser validation metrics
     * 
     * Tracks AQL validation operations for consolidation Phase 2.
     * Metric: aql_validation_total{status="success|parse_error|timeout|exception"}
     */
    void recordAQLValidation(
        bool success,
        std::chrono::milliseconds duration,
        const std::string& error_reason = ""  // e.g., "parse_error", "timeout", "exception"
    );
    
    /**
     * @brief Record AQL generation attempt metrics
     * 
     * Tracks NL-to-AQL generation attempts for consolidation Phase 2.
     * Metric: aql_generation_attempts_total{status="success|parse_error|retry|rejected"}
     */
    void recordAQLGenerationAttempt(
        bool success,
        int attempt_number,
        std::chrono::milliseconds duration,
        const std::string& outcome = ""  // e.g., "success", "parse_error", "max_retries_exceeded"
    );
    
    /**
     * @brief Record AQL validation retry
     * 
     * Tracks retry attempts after validation failure.
     * Metric: aql_validation_retries_total{outcome="success|failed"}
     */
    void recordValidationRetry(
        bool retry_succeeded,
        int attempt_number
    );
    
    /**
     * @brief Record cache hit/miss
     */
    void recordCacheAccess(
        const std::string& cache_type,  // "prefix" or "response"
        bool hit
    );
    
    /**
     * @brief Update model memory usage
     */
    void updateModelMemory(
        const std::string& model_id,
        size_t memory_bytes
    );
    
    /**
     * @brief Record circuit breaker state change
     */
    void recordCircuitBreakerState(
        const std::string& operation,
        const std::string& state  // "open", "closed", "half_open"
    );
    
    /**
     * @brief Get singleton instance
     */
    static LLMMetricsCollector& instance();

private:
    std::shared_ptr<llm::monitoring::PrometheusExporter> exporter_;
    std::mutex mutex_;
    bool initialized_ = false;
    
    void registerMetrics();
    std::unordered_map<std::string, std::string> makeLabels(
        const std::string& operation,
        const std::string& model = "",
        const std::string& status = ""
    );
};

/**
 * @brief RAII helper for automatic latency tracking
 * 
 * Note: This is a simplified timer for elapsed time queries.
 * Actual metric recording is done explicitly by the caller with more context.
 */
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
