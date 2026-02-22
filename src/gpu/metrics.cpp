/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics.cpp                                        ║
  Version:         0.0.27                                             ║
  Last Modified:   2026-02-22 08:56:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     170                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Metrics — in-process Prometheus-compatible counter/gauge registry.
 */

#include "themis/gpu/metrics.h"
#include <sstream>
#include <iomanip>

namespace themis {
namespace gpu {

// ============================================================================
// Private helpers
// ============================================================================

std::string GPUMetrics::buildKey(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& labels) {
    if (labels.empty()) return name;
    std::string key = name + "{";
    bool first = true;
    for (const auto& kv : labels) {
        if (!first) key += ',';
        key += kv.first + "=\"" + kv.second + "\"";
        first = false;
    }
    key += '}';
    return key;
}

void GPUMetrics::incrCounter(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& labels,
    double delta) {
    const std::string key = buildKey(name, labels);
    counters_[key] += delta;
    metric_types_[key] = "counter";
    // Also store the bare name type for snapshot metadata.
    metric_types_[name] = "counter";
}

void GPUMetrics::setGauge(
    const std::string& name,
    const std::unordered_map<std::string, std::string>& labels,
    double value) {
    const std::string key = buildKey(name, labels);
    gauges_[key] = value;
    metric_types_[key] = "gauge";
    metric_types_[name] = "gauge";
}

// ============================================================================
// Record helpers
// ============================================================================

void GPUMetrics::recordAllocSuccess(uint64_t bytes,
                                     const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{{"result", "success"}};
    if (!tenant_id.empty()) labels["tenant"] = tenant_id;
    incrCounter("themis_gpu_alloc_total", labels);
    incrCounter("themis_gpu_alloc_bytes_total", labels,
                static_cast<double>(bytes));
}

void GPUMetrics::recordAllocFailGlobal(uint64_t bytes,
                                        const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{
        {"result", "fail_global_limit"}};
    if (!tenant_id.empty()) labels["tenant"] = tenant_id;
    incrCounter("themis_gpu_alloc_total", labels);
    (void)bytes;
}

void GPUMetrics::recordAllocFailTenant(uint64_t bytes,
                                        const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels{
        {"result", "fail_tenant_quota"}, {"tenant", tenant_id}};
    incrCounter("themis_gpu_alloc_total", labels);
    (void)bytes;
}

void GPUMetrics::recordDealloc(uint64_t bytes, const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels;
    if (!tenant_id.empty()) labels["tenant"] = tenant_id;
    incrCounter("themis_gpu_dealloc_total", labels);
    incrCounter("themis_gpu_dealloc_bytes_total", labels,
                static_cast<double>(bytes));
}

void GPUMetrics::recordFallback(const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    incrCounter("themis_gpu_fallback_total", {{"reason", reason}});
}

void GPUMetrics::recordCircuitOpen() {
    std::lock_guard<std::mutex> lock(mutex_);
    incrCounter("themis_gpu_circuit_open_total", {});
}

void GPUMetrics::setVRAMAllocated(uint64_t bytes,
                                   const std::string& tenant_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::unordered_map<std::string, std::string> labels;
    if (!tenant_id.empty()) labels["tenant"] = tenant_id;
    setGauge("themis_gpu_vram_allocated_bytes", labels,
             static_cast<double>(bytes));
}

void GPUMetrics::setVRAMPeak(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    setGauge("themis_gpu_vram_peak_bytes", {},
             static_cast<double>(bytes));
}

void GPUMetrics::recordKernelDuration(const KernelRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    kernels_.push_back(record);
    setGauge("themis_gpu_kernel_duration_ns",
             {{"kernel", record.name},
              {"device", std::to_string(record.device_id)}},
             record.duration_ns);
}

// ============================================================================
// Nsight-compatible export
// ============================================================================

std::string GPUMetrics::nsight_export() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3);

    oss << "{\n";
    oss << "  \"NsightComputeVersion\": \"2024.1\",\n";
    oss << "  \"Filename\": \"themis_gpu_report\",\n";

    if (kernels_.empty()) {
        oss << "  \"Kernels\": []\n";
    } else {
        oss << "  \"Kernels\": [\n";
        for (std::size_t i = 0; i < kernels_.size(); ++i) {
            const KernelRecord& k = kernels_[i];
            oss << "    {\n";
            oss << "      \"Name\": \"" << k.name << "\",\n";
            oss << "      \"Demangled Name\": \"" << k.name << "\",\n";
            oss << "      \"Device\": " << k.device_id << ",\n";
            oss << "      \"Duration (ns)\": " << k.duration_ns << ",\n";
            oss << "      \"Grid Size\": ["
                << k.grid_x << ", " << k.grid_y << ", " << k.grid_z << "],\n";
            oss << "      \"Block Size\": ["
                << k.block_x << ", " << k.block_y << ", " << k.block_z << "]\n";
            oss << "    }";
            if (i + 1 < kernels_.size()) oss << ',';
            oss << '\n';
        }
        oss << "  ]\n";
    }

    oss << "}\n";

    return oss.str();
}

// ============================================================================
// Snapshot
// ============================================================================

std::vector<GPUMetrics::Sample> GPUMetrics::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Sample> result;
    result.reserve(counters_.size() + gauges_.size());

    for (const auto& kv : counters_) {
        Sample s;
        s.name  = kv.first;
        s.value = kv.second;
        s.type  = "counter";
        result.push_back(std::move(s));
    }
    for (const auto& kv : gauges_) {
        Sample s;
        s.name  = kv.first;
        s.value = kv.second;
        s.type  = "gauge";
        result.push_back(std::move(s));
    }
    return result;
}

void GPUMetrics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    metric_types_.clear();
    kernels_.clear();
}

} // namespace gpu
} // namespace themis
