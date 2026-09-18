/**
 * @file lora_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <memory>
#include <string>
#include <chrono>

// ============================================================================
// Compilation Guard for Prometheus
// ============================================================================
#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/summary.h>
#else
// Provide stub types when Prometheus is not available
namespace prometheus {
    class Registry {};
    template<typename T> class Family {};
    class Counter {};
    class Gauge {};
    class Histogram {};
    /** @brief Summary. */
    class Summary {};
}
#endif

namespace themis::llm::lora::metrics {

// ============================================================================
// Conditional Compilation Notice
// ============================================================================
#ifndef THEMIS_HAS_PROMETHEUS
#warning "Prometheus C++ client not found - metrics collection will be disabled"
#warning "Install with: vcpkg install prometheus-cpp"
#endif

// ============================================================================
// Metric Types
// ============================================================================

/**
 * @brief Prometheus metrics collector for LoRA framework
 */
class LoRAMetricsCollector {
public:
    struct Config {
        std::string namespace_prefix = "themis_lora";
        bool enable_detailed_metrics = true;
        bool enable_histograms = true;
    };
    
    /**
     * @brief TBD: Describe LoRAMetricsCollector.
     * @param[in] registry Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRAMetricsCollector(std::shared_ptr<prometheus::Registry> registry,
                                 const Config& config);
    /**
     * @brief TBD: Describe LoRAMetricsCollector.
     * @param[in] registry Input parameter.
     * @return Return value.
     */
    explicit LoRAMetricsCollector(std::shared_ptr<prometheus::Registry> registry);
    
    /**
     * @brief Adapter Lifecycle Metrics
     * @param[in] adapter_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordAdapterLoad(const std::string& adapter_id, double duration_ms);
    /**
     * @brief TBD: Describe recordAdapterUnload.
     * @param[in] adapter_id Input parameter.
     */
    void recordAdapterUnload(const std::string& adapter_id);
    /**
     * @brief TBD: Describe recordAdapterSwitch.
     * @param[in] from_id Input parameter.
     * @param[in] to_id Input parameter.
     * @param[in] duration_ms Input parameter.
     */
    void recordAdapterSwitch(const std::string& from_id, const std::string& to_id, double duration_ms);
    /**
     * @brief TBD: Describe recordAdapterLoadError.
     * @param[in] adapter_id Input parameter.
     * @param[in] error Input parameter.
     */
    void recordAdapterLoadError(const std::string& adapter_id, const std::string& error);
    
    /**
     * @brief Cache Metrics
     * @param[in] adapter_id Input parameter.
     */
    void recordCacheHit(const std::string& adapter_id);
    /**
     * @brief TBD: Describe recordCacheMiss.
     * @param[in] adapter_id Input parameter.
     */
    void recordCacheMiss(const std::string& adapter_id);
    /**
     * @brief TBD: Describe recordCacheEviction.
     * @param[in] adapter_id Input parameter.
     */
    void recordCacheEviction(const std::string& adapter_id);
    /**
     * @brief TBD: Describe updateCacheSize.
     * @param[in] size Input parameter.
     */
    void updateCacheSize(size_t size);
    /**
     * @brief TBD: Describe updateCacheMemoryUsage.
     * @param[in] bytes Input parameter.
     */
    void updateCacheMemoryUsage(size_t bytes);
    
    /**
     * @brief Training Metrics
     * @param[in] adapter_id Input parameter.
     * @param[in] mode Input parameter.
     */
    void recordTrainingStart(const std::string& adapter_id, const std::string& mode);
    /**
     * @brief TBD: Describe recordTrainingComplete.
     * @param[in] adapter_id Input parameter.
     * @param[in] mode Input parameter.
     * @param[in] duration_seconds Input parameter.
     * @param[in] success Input parameter.
     */
    void recordTrainingComplete(const std::string& adapter_id, const std::string& mode, 
                               double duration_seconds, bool success);
    /**
     * @brief TBD: Describe recordTrainingSamples.
     * @param[in] adapter_id Input parameter.
     * @param[in] num_samples Input parameter.
     */
    void recordTrainingSamples(const std::string& adapter_id, size_t num_samples);
    /**
     * @brief TBD: Describe updateTrainingLoss.
     * @param[in] adapter_id Input parameter.
     * @param[in] loss Input parameter.
     */
    void updateTrainingLoss(const std::string& adapter_id, double loss);
    /**
     * @brief TBD: Describe updateValidationAccuracy.
     * @param[in] adapter_id Input parameter.
     * @param[in] accuracy Input parameter.
     */
    void updateValidationAccuracy(const std::string& adapter_id, double accuracy);
    
    /**
     * @brief Storage Metrics
     * @param[in] adapter_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] bytes Input parameter.
     */
    void recordStorageRead(const std::string& adapter_id, double duration_ms, size_t bytes);
    /**
     * @brief TBD: Describe recordStorageWrite.
     * @param[in] adapter_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] bytes Input parameter.
     */
    void recordStorageWrite(const std::string& adapter_id, double duration_ms, size_t bytes);
    /**
     * @brief TBD: Describe recordStorageDelete.
     * @param[in] adapter_id Input parameter.
     */
    void recordStorageDelete(const std::string& adapter_id);
    /**
     * @brief TBD: Describe recordStorageError.
     * @param[in] operation Input parameter.
     * @param[in] error Input parameter.
     */
    void recordStorageError(const std::string& operation, const std::string& error);
    
    /**
     * @brief Versioning Metrics
     * @param[in] adapter_id Input parameter.
     * @param[in] version Input parameter.
     */
    void recordVersionCreate(const std::string& adapter_id, const std::string& version);
    /**
     * @brief TBD: Describe recordVersionRollback.
     * @param[in] adapter_id Input parameter.
     * @param[in] from_version Input parameter.
     * @param[in] to_version Input parameter.
     */
    void recordVersionRollback(const std::string& adapter_id, const std::string& from_version,
                              const std::string& to_version);
    /**
     * @brief TBD: Describe updateVersionCount.
     * @param[in] adapter_id Input parameter.
     * @param[in] count Input parameter.
     */
    void updateVersionCount(const std::string& adapter_id, size_t count);
    
    /**
     * @brief Inference Metrics
     * @param[in] adapter_id Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] input_tokens Input parameter.
     * @param[in] output_tokens Input parameter.
     */
    void recordInference(const std::string& adapter_id, double duration_ms, 
                        size_t input_tokens, size_t output_tokens);
    /**
     * @brief TBD: Describe recordInferenceError.
     * @param[in] adapter_id Input parameter.
     * @param[in] error Input parameter.
     */
    void recordInferenceError(const std::string& adapter_id, const std::string& error);
    /**
     * @brief TBD: Describe updateInferenceQueueSize.
     * @param[in] size Input parameter.
     */
    void updateInferenceQueueSize(size_t size);
    
    /**
     * @brief Audit Metrics
     * @param[in] duration_ms Input parameter.
     * @param[in] bytes Input parameter.
     */
    void recordAuditLogWrite(double duration_ms, size_t bytes);
    /**
     * @brief TBD: Describe recordAuditQuery.
     * @param[in] duration_ms Input parameter.
     * @param[in] results Input parameter.
     */
    void recordAuditQuery(double duration_ms, size_t results);
    /**
     * @brief TBD: Describe updateAuditLogSize.
     * @param[in] entries Input parameter.
     */
    void updateAuditLogSize(size_t entries);
    
    /**
     * @brief Resource Usage Metrics
     * @param[in] category Input parameter.
     * @param[in] bytes Input parameter.
     */
    void updateMemoryUsage(const std::string& category, size_t bytes);
    /**
     * @brief TBD: Describe updateGPUVRAMUsage.
     * @param[in] adapter_id Input parameter.
     * @param[in] bytes Input parameter.
     */
    void updateGPUVRAMUsage(const std::string& adapter_id, size_t bytes);
    /**
     * @brief TBD: Describe updateCPUUsage.
     * @param[in] percentage Input parameter.
     */
    void updateCPUUsage(double percentage);
    
    /**
     * @brief Orchestrator Metrics
     * @param[in] operation Input parameter.
     * @param[in] duration_ms Input parameter.
     * @param[in] success Input parameter.
     */
    void recordOrchestratorOperation(const std::string& operation, double duration_ms, bool success);
    /**
     * @brief TBD: Describe updateActiveAdapters.
     * @param[in] count Input parameter.
     */
    void updateActiveAdapters(size_t count);
    /**
     * @brief TBD: Describe updateTotalAdapters.
     * @param[in] count Input parameter.
     */
    void updateTotalAdapters(size_t count);
    
    /**
     * @brief Get metrics in Prometheus format
     * @return Return value.
     */
    std::string getMetrics() const;
    
private:
    std::shared_ptr<prometheus::Registry> registry_;
    Config config_;
    
    // Adapter Lifecycle
    prometheus::Family<prometheus::Histogram>& adapter_load_duration_;
    prometheus::Family<prometheus::Counter>& adapter_loads_total_;
    prometheus::Family<prometheus::Counter>& adapter_unloads_total_;
    prometheus::Family<prometheus::Histogram>& adapter_switch_duration_;
    prometheus::Family<prometheus::Counter>& adapter_load_errors_total_;
    
    // Cache
    prometheus::Family<prometheus::Counter>& cache_hits_total_;
    prometheus::Family<prometheus::Counter>& cache_misses_total_;
    prometheus::Family<prometheus::Counter>& cache_evictions_total_;
    prometheus::Gauge& cache_size_;
    prometheus::Gauge& cache_memory_bytes_;
    
    // Training
    prometheus::Family<prometheus::Counter>& training_starts_total_;
    prometheus::Family<prometheus::Counter>& training_completes_total_;
    prometheus::Family<prometheus::Histogram>& training_duration_;
    prometheus::Family<prometheus::Counter>& training_samples_total_;
    prometheus::Family<prometheus::Gauge>& training_loss_;
    prometheus::Family<prometheus::Gauge>& training_accuracy_;
    
    // Storage
    prometheus::Family<prometheus::Histogram>& storage_read_duration_;
    prometheus::Family<prometheus::Histogram>& storage_write_duration_;
    prometheus::Family<prometheus::Counter>& storage_reads_total_;
    prometheus::Family<prometheus::Counter>& storage_writes_total_;
    prometheus::Family<prometheus::Counter>& storage_deletes_total_;
    prometheus::Family<prometheus::Counter>& storage_errors_total_;
    prometheus::Summary& storage_read_bytes_;
    prometheus::Summary& storage_write_bytes_;
    
    // Versioning
    prometheus::Family<prometheus::Counter>& version_creates_total_;
    prometheus::Family<prometheus::Counter>& version_rollbacks_total_;
    prometheus::Family<prometheus::Gauge>& version_count_;
    
    // Inference
    prometheus::Family<prometheus::Histogram>& inference_duration_;
    prometheus::Family<prometheus::Counter>& inference_total_;
    prometheus::Family<prometheus::Counter>& inference_errors_total_;
    prometheus::Family<prometheus::Counter>& inference_tokens_total_;
    prometheus::Gauge& inference_queue_size_;
    
    // Audit
    prometheus::Histogram& audit_log_write_duration_;
    prometheus::Histogram& audit_query_duration_;
    prometheus::Gauge& audit_log_entries_;
    prometheus::Summary& audit_log_bytes_;
    
    // Resources
    prometheus::Family<prometheus::Gauge>& memory_usage_bytes_;
    prometheus::Family<prometheus::Gauge>& gpu_vram_bytes_;
    prometheus::Gauge& cpu_usage_percent_;
    
    // Orchestrator
    prometheus::Family<prometheus::Histogram>& orchestrator_operation_duration_;
    prometheus::Family<prometheus::Counter>& orchestrator_operations_total_;
    prometheus::Gauge& active_adapters_;
    prometheus::Gauge& total_adapters_;
};

// ============================================================================
// Metrics Helper Classes
// ============================================================================

/**
 * @brief RAII timer for automatic duration measurement
 */
class MetricTimer {
public:
    explicit MetricTimer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsedMilliseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_).count();
    }
    
    double elapsedSeconds() const {
        return elapsedMilliseconds() / 1000.0;
    }
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

/**
 * @brief Scoped metric recording for operations
 */
template<typename Func>
class ScopedMetric {
public:
    ScopedMetric(Func on_complete) : on_complete_(std::move(on_complete)) {}
    
    ~ScopedMetric() {
        try {
            on_complete_(timer_.elapsedMilliseconds());
        } catch (...) {
            // Ignore errors in metric recording
        }
    }
    
private:
    MetricTimer timer_;
    Func on_complete_;
};

// Helper function to create scoped metrics
template<typename Func>
/**
 * @brief TBD: Describe makeScopedMetric.
 * @param[in] func Input parameter.
 * @return Return value.
 * @details Calls: std::move().
 */
ScopedMetric<Func> makeScopedMetric(Func func) {
    return ScopedMetric<Func>(std::move(func));
}

} // namespace themis::llm::lora::metrics
