/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_metrics_collector.h                            ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:23:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     153                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
