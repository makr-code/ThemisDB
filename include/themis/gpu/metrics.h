/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics.h                                          ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     126                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
