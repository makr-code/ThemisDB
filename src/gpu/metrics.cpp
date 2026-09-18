/**
 * @file metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Metrics — in-process Prometheus-compatible counter/gauge registry.
 */

#include "themis/gpu/metrics.h"

#include <iomanip>
#include <sstream>

namespace themis {
namespace gpu {

// ============================================================================
// Private helpers
// ============================================================================

std::string GPUMetrics::buildKey(const std::string &name, const std::unordered_map<std::string, std::string> &labels) {
    if (labels.empty()) {
        return name;
    }
    std::string key = name + "{";
    bool first      = true;
    for (const auto &kv : labels) {
        if (!first) {
            key += ',';
        }
        key += kv.first + "=\"" + kv.second + "\"";
        first = false;
    }
    key += '}';
    return key;
}

void GPUMetrics::incrCounter(const std::string &name, const std::unordered_map<std::string, std::string> &labels,
                             double delta) {
    const std::string key = buildKey(name, labels);
    counters_[key] += delta;
    metric_types_[key] = "counter";
    // Also store the bare name type for snapshot metadata.
    metric_types_[name] = "counter";
}

void GPUMetrics::setGauge(const std::string &name, const std::unordered_map<std::string, std::string> &labels,
                          double value) {
    const std::string key = buildKey(name, labels);
    gauges_[key]          = value;
    metric_types_[key]    = "gauge";
    metric_types_[name]   = "gauge";
}

// ============================================================================
// Record helpers
// ============================================================================

/**
 * @brief Record Alloc Success.
 * @param[in] bytes Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: lock(), empty(), incrCounter().
 */
void GPUMetrics::recordAllocSuccess(uint64_t bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{{"result", "success"}};
    if (!tenant_id.empty()) {
        labels["tenant"] = tenant_id;
    }
    incrCounter("themis_gpu_alloc_total", labels);
    incrCounter("themis_gpu_alloc_bytes_total", labels, static_cast<double>(bytes));
}

/**
 * @brief Record Alloc Fail Global.
 * @param[in] bytes Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: lock(), empty(), incrCounter().
 */
void GPUMetrics::recordAllocFailGlobal(uint64_t bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{{"result", "fail_global_limit"}};
    if (!tenant_id.empty()) {
        labels["tenant"] = tenant_id;
    }
    incrCounter("themis_gpu_alloc_total", labels);
}

/**
 * @brief Record Alloc Fail Tenant.
 * @param[in] bytes Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: lock(), incrCounter().
 */
void GPUMetrics::recordAllocFailTenant(uint64_t bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{{"result", "fail_tenant_quota"}, {"tenant", tenant_id}};
    incrCounter("themis_gpu_alloc_total", labels);
}

/**
 * @brief Record Dealloc.
 * @param[in] bytes Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: lock(), empty(), incrCounter().
 */
void GPUMetrics::recordDealloc(uint64_t bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels = {};

    if (!tenant_id.empty()) {
        labels["tenant"] = tenant_id;
    }
    incrCounter("themis_gpu_dealloc_total", labels);
    incrCounter("themis_gpu_dealloc_bytes_total", labels, static_cast<double>(bytes));
}

/**
 * @brief Record Fallback.
 * @param[in] reason Input parameter.
 * @details Calls: lock(), incrCounter().
 */
void GPUMetrics::recordFallback(const std::string &reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    incrCounter("themis_gpu_fallback_total", {{"reason", reason}});
}

/**
 * @brief Record Circuit Open.
 * @details Calls: lock(), incrCounter().
 */
void GPUMetrics::recordCircuitOpen() {
    std::lock_guard<std::mutex> lock(mutex_);
    incrCounter("themis_gpu_circuit_open_total", {});
}

/**
 * @brief Set VRAMAllocated.
 * @param[in] bytes Input parameter.
 * @param[in] tenant_id Identifier of the tenant.
 * @details Calls: lock(), empty(), setGauge().
 */
void GPUMetrics::setVRAMAllocated(uint64_t bytes, const std::string &tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels = {};

    if (!tenant_id.empty()) {
        labels["tenant"] = tenant_id;
    }
    setGauge("themis_gpu_vram_allocated_bytes", labels, static_cast<double>(bytes));
}

/**
 * @brief Set VRAMPeak.
 * @param[in] bytes Input parameter.
 * @details Calls: lock(), setGauge().
 */
void GPUMetrics::setVRAMPeak(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    setGauge("themis_gpu_vram_peak_bytes", {}, static_cast<double>(bytes));
}

/**
 * @brief Set Temperature.
 * @param[in] device_id Identifier of the device.
 * @param[in] celsius Input parameter.
 * @details Calls: lock(), setGauge(), std::to_string().
 */
void GPUMetrics::setTemperature(int device_id, double celsius) {
    std::lock_guard<std::mutex> lock(mutex_);
    setGauge("themis_gpu_temperature_celsius", {{"device", std::to_string(device_id)}}, celsius);
}

/**
 * @brief Set Power Draw.
 * @param[in] device_id Identifier of the device.
 * @param[in] watts Input parameter.
 * @details Calls: lock(), setGauge(), std::to_string().
 */
void GPUMetrics::setPowerDraw(int device_id, double watts) {
    std::lock_guard<std::mutex> lock(mutex_);
    setGauge("themis_gpu_power_draw_watts", {{"device", std::to_string(device_id)}}, watts);
}

/**
 * @brief Set Power Limit.
 * @param[in] device_id Identifier of the device.
 * @param[in] watts Input parameter.
 * @details Calls: lock(), setGauge(), std::to_string().
 */
void GPUMetrics::setPowerLimit(int device_id, double watts) {
    std::lock_guard<std::mutex> lock(mutex_);
    setGauge("themis_gpu_power_limit_watts", {{"device", std::to_string(device_id)}}, watts);
}

/**
 * @brief Record Kernel Duration.
 * @param[in] record Input parameter.
 * @details Calls: lock(), push_back(), setGauge(), std::to_string().
 */
void GPUMetrics::recordKernelDuration(const KernelRecord &record) {
    std::lock_guard<std::mutex> lock(mutex_);
    kernels_.push_back(record);
    setGauge("themis_gpu_kernel_duration_ns", {{"kernel", record.name}, {"device", std::to_string(record.device_id)}},
             record.duration_ns);
}

// ============================================================================
// Nsight-compatible export
// ============================================================================

std::string GPUMetrics::nsight_export() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss = {};
    oss << std::fixed << std::setprecision(3);

    oss << "{\n";
    oss << "  \"NsightComputeVersion\": \"2024.1\",\n";
    oss << "  \"Filename\": \"themis_gpu_report\",\n";

    if (kernels_.empty()) {
        oss << "  \"Kernels\": []\n";
    } else {
        oss << "  \"Kernels\": [\n";
        for (std::size_t i = 0; i < kernels_.size(); ++i) {
            const KernelRecord &k = kernels_[i];
            oss << "    {\n";
            oss << "      \"Name\": \"" << k.name << "\",\n";
            oss << "      \"Demangled Name\": \"" << k.name << "\",\n";
            oss << "      \"Device\": " << k.device_id << ",\n";
            oss << "      \"Duration (ns)\": " << k.duration_ns << ",\n";
            oss << "      \"Grid Size\": [" << k.grid_x << ", " << k.grid_y << ", " << k.grid_z << "],\n";
            oss << "      \"Block Size\": [" << k.block_x << ", " << k.block_y << ", " << k.block_z << "]\n";
            oss << "    }";
            if (i + 1 < kernels_.size()) {
                oss << ',';
            }
            oss << '\n';
        }
        oss << "  ]\n";
    }

    oss << "}\n";

    return oss.str();
}

// ============================================================================
// ROCm profiler-compatible export (Chrome trace JSON)
// ============================================================================

std::string GPUMetrics::rocm_profiler_export() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    // Chrome trace format — compatible with:
    //   - AMD ROCm profiler's --sys-trace JSON output
    //   - Perfetto / chrome://tracing
    //
    // Each kernel is emitted as a complete event ("ph": "X").
    // Timestamps follow the Chrome trace convention (microseconds).
    std::ostringstream oss = {};

    if (kernels_.empty()) {
        oss << "{\n  \"traceEvents\": []\n}\n";
        return oss.str();
    }

    oss << "{\n  \"traceEvents\": [\n";

    for (std::size_t i = 0; i < kernels_.size(); ++i) {
        const KernelRecord &k = kernels_[i];
        const uint64_t ts     = static_cast<uint64_t>(k.duration_ns) / 1000;
        const uint64_t dur    = ts; // treat duration_ns as elapsed time

        oss << "    {\"name\": \"" << k.name << "\", \"ph\": \"X\", "
            << "\"ts\": " << ts << ", "
            << "\"dur\": " << dur << ", "
            << "\"pid\": 0, \"tid\": " << k.device_id << ", "
            << "\"args\": {"
            << "\"grid\": [" << k.grid_x << "," << k.grid_y << "," << k.grid_z << "], "
            << "\"block\": [" << k.block_x << "," << k.block_y << "," << k.block_z << "]"
            << "}}";
        if (i + 1 < kernels_.size()) {
            oss << ',';
        }
        oss << '\n';
    }

    oss << "  ]\n}\n";
    return oss.str();
}

// ============================================================================
// Snapshot
// ============================================================================

std::vector<GPUMetrics::Sample> GPUMetrics::snapshot() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Sample> result = {};

    result.reserve(counters_.size() + gauges_.size() );

    for (const auto &kv : counters_) {
        Sample s;
        s.name  = kv.first;
        s.value = kv.second;
        s.type  = "counter";
        result.push_back(std::move(s));
    }
    for (const auto &kv : gauges_) {
        Sample s;
        s.name  = kv.first;
        s.value = kv.second;
        s.type  = "gauge";
        result.push_back(std::move(s));
    }
    return result;
}

/**
 * @brief Reset the modification detection flag.
 * @details Calls: lock(), clear().
 */
void GPUMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    metric_types_.clear();
    kernels_.clear();
}

} // namespace gpu
} // namespace themis
