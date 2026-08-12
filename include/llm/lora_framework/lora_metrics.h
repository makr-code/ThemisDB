/**
 * @file lora_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    
    explicit LoRAMetricsCollector(std::shared_ptr<prometheus::Registry> registry,
                                 const Config& config);
    explicit LoRAMetricsCollector(std::shared_ptr<prometheus::Registry> registry);
    
    // Adapter Lifecycle Metrics
    void recordAdapterLoad(const std::string& adapter_id, double duration_ms);
    void recordAdapterUnload(const std::string& adapter_id);
    void recordAdapterSwitch(const std::string& from_id, const std::string& to_id, double duration_ms);
    void recordAdapterLoadError(const std::string& adapter_id, const std::string& error);
    
    // Cache Metrics
    void recordCacheHit(const std::string& adapter_id);
    void recordCacheMiss(const std::string& adapter_id);
    void recordCacheEviction(const std::string& adapter_id);
    void updateCacheSize(size_t size);
    void updateCacheMemoryUsage(size_t bytes);
    
    // Training Metrics
    void recordTrainingStart(const std::string& adapter_id, const std::string& mode);
    void recordTrainingComplete(const std::string& adapter_id, const std::string& mode, 
                               double duration_seconds, bool success);
    void recordTrainingSamples(const std::string& adapter_id, size_t num_samples);
    void updateTrainingLoss(const std::string& adapter_id, double loss);
    void updateValidationAccuracy(const std::string& adapter_id, double accuracy);
    
    // Storage Metrics
    void recordStorageRead(const std::string& adapter_id, double duration_ms, size_t bytes);
    void recordStorageWrite(const std::string& adapter_id, double duration_ms, size_t bytes);
    void recordStorageDelete(const std::string& adapter_id);
    void recordStorageError(const std::string& operation, const std::string& error);
    
    // Versioning Metrics
    void recordVersionCreate(const std::string& adapter_id, const std::string& version);
    void recordVersionRollback(const std::string& adapter_id, const std::string& from_version,
                              const std::string& to_version);
    void updateVersionCount(const std::string& adapter_id, size_t count);
    
    // Inference Metrics
    void recordInference(const std::string& adapter_id, double duration_ms, 
                        size_t input_tokens, size_t output_tokens);
    void recordInferenceError(const std::string& adapter_id, const std::string& error);
    void updateInferenceQueueSize(size_t size);
    
    // Audit Metrics
    void recordAuditLogWrite(double duration_ms, size_t bytes);
    void recordAuditQuery(double duration_ms, size_t results);
    void updateAuditLogSize(size_t entries);
    
    // Resource Usage Metrics
    void updateMemoryUsage(const std::string& category, size_t bytes);
    void updateGPUVRAMUsage(const std::string& adapter_id, size_t bytes);
    void updateCPUUsage(double percentage);
    
    // Orchestrator Metrics
    void recordOrchestratorOperation(const std::string& operation, double duration_ms, bool success);
    void updateActiveAdapters(size_t count);
    void updateTotalAdapters(size_t count);
    
    // Get metrics in Prometheus format
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
ScopedMetric<Func> makeScopedMetric(Func func) {
    return ScopedMetric<Func>(std::move(func));
}

} // namespace themis::llm::lora::metrics
