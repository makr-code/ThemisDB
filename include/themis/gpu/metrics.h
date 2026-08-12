/**
 * @file metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
 * - themis_gpu_temperature_celsius     (gauge, device=<id>)
 * - themis_gpu_power_draw_watts        (gauge, device=<id>)
 * - themis_gpu_power_limit_watts       (gauge, device=<id>)
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

    /**
     * @brief Record of a single GPU kernel invocation for Nsight-compatible export.
     *
     * Compatible with the CUDA Nsight Compute JSON schema:
     * https://docs.nvidia.com/nsight-compute/2024.1/
     */
    struct KernelRecord {
        std::string name;            ///< Kernel function name (demangled)
        double      duration_ns = 0; ///< Elapsed GPU time in nanoseconds
        int         device_id  = 0;  ///< CUDA device ordinal
        int         grid_x     = 1;  ///< Grid dimension X
        int         grid_y     = 1;  ///< Grid dimension Y
        int         grid_z     = 1;  ///< Grid dimension Z
        int         block_x    = 1;  ///< Block dimension X
        int         block_y    = 1;  ///< Block dimension Y
        int         block_z    = 1;  ///< Block dimension Z
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
    // Thermal and power telemetry
    // -----------------------------------------------------------------------

    /**
     * @brief Update the live temperature gauge for a specific GPU device.
     *
     * @param device_id  CUDA/ROCm device ordinal (0-based).
     * @param celsius    Current junction temperature in degrees Celsius.
     */
    void setTemperature(int device_id, double celsius);

    /**
     * @brief Update the live power-draw gauge for a specific GPU device.
     *
     * @param device_id  CUDA/ROCm device ordinal (0-based).
     * @param watts      Current board power draw in watts.
     */
    void setPowerDraw(int device_id, double watts);

    /**
     * @brief Update the enforced power-limit gauge for a specific GPU device.
     *
     * @param device_id  CUDA/ROCm device ordinal (0-based).
     * @param watts      Configured power limit in watts.
     */
    void setPowerLimit(int device_id, double watts);

    /**
     * @brief Record a GPU kernel execution for Nsight-compatible export.
     *
     * Appends one KernelRecord to the internal kernel list and also
     * increments the themis_gpu_kernel_duration_ns gauge so the data
     * is visible in Prometheus snapshots too.
     *
     * @param record  Populated KernelRecord (name and duration_ns required).
     */
    void recordKernelDuration(const KernelRecord& record);

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
     * @brief Export kernel records in CUDA Nsight Compute-compatible JSON.
     *
     * Produces a JSON document that mirrors the top-level schema used by
     * Nsight Compute's `--export json` option so that tooling built around
     * that format (e.g. ncu-ui, custom analysis scripts) can consume the
     * output without modification.
     *
     * @return UTF-8 JSON string. When no kernels have been recorded the
     *         `"Kernels"` array is present but empty (`"Kernels": []`).
     */
    std::string nsight_export() const;

    /**
     * @brief Export kernel records in ROCm profiler Chrome trace JSON format.
     *
     * Produces a Chrome trace JSON document compatible with AMD ROCm
     * profiler's `--sys-trace` output and with Perfetto / chrome://tracing.
     * Each kernel is emitted as a complete event (`"ph": "X"`) so that
     * tooling built around the ROCm profiler JSON schema can consume the
     * output without modification.
     *
     * @return UTF-8 JSON string.  When no kernels have been recorded the
     *         `"traceEvents"` array is present but empty.
     */
    std::string rocm_profiler_export() const;

    /**
     * @brief Reset all counters, gauges, and kernel records (for testing).
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
    // Kernel records for Nsight-compatible export.
    std::vector<KernelRecord> kernels_;

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

