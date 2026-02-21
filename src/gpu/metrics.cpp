/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metrics.cpp                                        ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     177                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Metrics — in-process Prometheus-compatible counter/gauge registry.
 */

#include "themis/gpu/metrics.h"

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
}

} // namespace gpu
} // namespace themis
