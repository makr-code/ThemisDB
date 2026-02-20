#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Lightweight in-process GPU metrics registry.
 *
 * Aggregates counters and gauges that can be scraped by a Prometheus/OTel
 * exporter or logged by the admin endpoint.  All metrics are labelled so
 * they can carry tenant_id and device backend dimensions.
 *
 * Metric families
 * ---------------
 * - themis_gpu_vram_allocated_bytes    (gauge, per-tenant or global)
 * - themis_gpu_vram_peak_bytes         (gauge)
 * - themis_gpu_alloc_total             (counter, result=success|fail_global|fail_tenant)
 * - themis_gpu_dealloc_total           (counter)
 * - themis_gpu_fallback_total          (counter, reason=oom|circuit_open|device_unavailable)
 * - themis_gpu_circuit_open_total      (counter)
 *
 * Thread safety: all methods are protected by an internal mutex.
 */
class GPUMetrics {
public:
    // -----------------------------------------------------------------------
    // Sample types
    // -----------------------------------------------------------------------
    struct Sample {
        std::string name;                                     ///< Metric name
        std::unordered_map<std::string, std::string> labels; ///< Dimension labels
        double      value = 0.0;
        std::string type;  ///< "counter" | "gauge"
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUMetrics& GetInstance() {
        static GPUMetrics inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Record helpers
    // -----------------------------------------------------------------------

    void recordAllocSuccess(uint64_t bytes, const std::string& tenant_id = "");
    void recordAllocFailGlobal(uint64_t bytes, const std::string& tenant_id = "");
    void recordAllocFailTenant(uint64_t bytes, const std::string& tenant_id);
    void recordDealloc(uint64_t bytes, const std::string& tenant_id = "");
    void recordFallback(const std::string& reason = "oom");
    void recordCircuitOpen();

    /**
     * @brief Update the live VRAM gauge (must be called after each alloc/free).
     */
    void setVRAMAllocated(uint64_t bytes, const std::string& tenant_id = "");
    void setVRAMPeak(uint64_t bytes);

    // -----------------------------------------------------------------------
    // Snapshot
    // -----------------------------------------------------------------------

    /**
     * @brief Return all current metric samples.
     *
     * Suitable for serialisation into Prometheus text format or OTel OTLP.
     */
    std::vector<Sample> snapshot() const;

    /**
     * @brief Reset all counters and gauges (for testing).
     */
    void reset();

private:
    GPUMetrics() = default;
    mutable std::mutex mutex_;

    // Counters — labelled by a string key "name{label=value,...}".
    std::unordered_map<std::string, double> counters_;
    // Gauges.
    std::unordered_map<std::string, double> gauges_;
    // Metric metadata for snapshot.
    std::unordered_map<std::string, std::string> metric_types_;  // key → "counter"|"gauge"

    void incrCounter(const std::string& name,
                     const std::unordered_map<std::string, std::string>& labels,
                     double delta = 1.0);
    void setGauge(const std::string& name,
                  const std::unordered_map<std::string, std::string>& labels,
                  double value);

    static std::string buildKey(
        const std::string& name,
        const std::unordered_map<std::string, std::string>& labels);
};

} // namespace gpu
} // namespace themis
