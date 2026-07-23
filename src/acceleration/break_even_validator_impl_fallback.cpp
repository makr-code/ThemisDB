#include "acceleration/break_even_validator.h"
#include <algorithm>
#include <chrono>
#include <mutex>

namespace themis {
namespace acceleration {

class BEVImpl {
public:
    BEVImpl() {
        // default thresholds
        thresholds_[(int)KernelType::kDistance] = 1.5f;
        thresholds_[(int)KernelType::kTopK] = 1.5f;
        thresholds_[(int)KernelType::kBFS] = 1.3f;
        thresholds_[(int)KernelType::kDijkstra] = 1.3f;
        thresholds_[(int)KernelType::kGeoDistance] = 1.3f;
        thresholds_[(int)KernelType::kGeoContainment] = 1.3f;
    }

    std::unordered_map<int,float> thresholds_;
    std::unordered_map<int,float> latest_ratios_;
    std::unordered_map<std::string, BreakEvenDecision> cache_;
    size_t hits_ = 0;
    size_t misses_ = 0;
    std::chrono::hours ttl_{24};
    std::mutex mu_;
};

static BEVImpl g_impl;

BreakEvenValidator::BreakEvenValidator() {}
BreakEvenValidator::~BreakEvenValidator() {}

// WorkloadProfile helpers (small, stable string forms used as cache keys)
std::string WorkloadProfile::ToCacheKey() const {
    return BreakEvenValidator::KernelTypeToString(kernel_type) + ":" + std::to_string(input_size) + ":" + std::to_string(vector_dimension) + ":" + BreakEvenValidator::DeviceTypeToString(device) + ":" + std::to_string(static_cast<int>(output_selectivity * 10000));
}

std::string WorkloadProfile::ToString() const {
    return "kernel=" + BreakEvenValidator::KernelTypeToString(kernel_type) + ",size=" + std::to_string(input_size) + ",dim=" + std::to_string(vector_dimension) + ",sel=" + std::to_string(output_selectivity);
}

BreakEvenDecision BreakEvenValidator::ShouldUseGPU(const WorkloadProfile& profile) {
    // honor hints
    if (profile.force_gpu && *profile.force_gpu) return BreakEvenDecision{true, 1.0f, std::chrono::milliseconds(0), std::chrono::milliseconds(0), "force_gpu_flag", false};
    if (profile.prefer_cpu && *profile.prefer_cpu) return BreakEvenDecision{false, 0.0f, std::chrono::milliseconds(0), std::chrono::milliseconds(0), "prefer_cpu_flag", false};

    // simple cache key
    std::string key = profile.ToCacheKey();
    {
        std::lock_guard<std::mutex> lock(g_impl.mu_);
        auto it = g_impl.cache_.find(key);
        if (it != g_impl.cache_.end()) {
            g_impl.hits_++;
            auto d = it->second;
            d.from_cache = true;
            return d;
        }
    }

    g_impl.misses_++;
    auto d = Profile(profile);
    {
        std::lock_guard<std::mutex> lock(g_impl.mu_);
        g_impl.cache_[key] = d;
    }
    return d;
}

BreakEvenDecision BreakEvenValidator::Profile(const WorkloadProfile& profile) {
    // Very simple synthetic profiling: GPU faster for very large inputs
    float speedup = 0.0f;
    if (profile.input_size >= 1000000) speedup = 2.0f;
    else if (profile.input_size >= 100000) speedup = 1.2f;
    else speedup = 0.8f;

    float thr = GetSpeedupThreshold(profile.kernel_type);
    bool use_gpu = speedup >= thr;

    BreakEvenDecision dec;
    dec.use_gpu = use_gpu;
    dec.speedup_ratio = speedup;
    dec.cpu_time_ms = std::chrono::milliseconds(1000);
    dec.gpu_time_ms = std::chrono::milliseconds((int)(1000.0f / (speedup > 0 ? speedup : 1.0f)));
    dec.reason = use_gpu ? "break_even_met" : "break_even_not_met";
    dec.from_cache = false;

    g_impl.latest_ratios_[(int)profile.kernel_type] = dec.speedup_ratio;
    return dec;
}

void BreakEvenValidator::SetSpeedupThreshold(KernelType kernel, float threshold) {
    std::lock_guard<std::mutex> lock(g_impl.mu_);
    if (threshold < 1.0f) threshold = 1.0f;
    g_impl.thresholds_[(int)kernel] = threshold;
}

float BreakEvenValidator::GetSpeedupThreshold(KernelType kernel) const {
    auto it = g_impl.thresholds_.find((int)kernel);
    if (it == g_impl.thresholds_.end()) return 1.0f;
    return it->second;
}

void BreakEvenValidator::ClearCache() {
    std::lock_guard<std::mutex> lock(g_impl.mu_);
    g_impl.cache_.clear();
}

float BreakEvenValidator::GetLatestBreakEvenRatio(KernelType kernel) const {
    auto it = g_impl.latest_ratios_.find((int)kernel);
    if (it == g_impl.latest_ratios_.end()) return 0.0f;
    return it->second;
}

size_t BreakEvenValidator::GetCacheHitCount() const {
    return g_impl.hits_;
}

size_t BreakEvenValidator::GetCacheMissCount() const {
    return g_impl.misses_;
}

size_t BreakEvenValidator::GetCacheSize() const {
    std::lock_guard<std::mutex> lock(g_impl.mu_);
    return g_impl.cache_.size();
}

std::string BreakEvenValidator::KernelTypeToString(KernelType kernel) {
    switch (kernel) {
        case KernelType::kDistance: return "distance";
        case KernelType::kTopK: return "topk";
        case KernelType::kBFS: return "bfs";
        case KernelType::kDijkstra: return "dijkstra";
        case KernelType::kGeoDistance: return "geodistance";
        case KernelType::kGeoContainment: return "geocontainment";
        default: return "unknown";
    }
}

std::string BreakEvenValidator::DeviceTypeToString(DeviceType device) {
    switch (device) {
        case DeviceType::kNVIDIA_RTX: return "nvidia_rtx";
        case DeviceType::kNVIDIA_T4: return "nvidia_t4";
        case DeviceType::kAMD_MI210: return "amd_mi210";
        case DeviceType::kIntel_Arc: return "intel_arc";
        case DeviceType::kCPU: return "cpu";
        default: return "unknown";
    }
}

} // namespace acceleration
} // namespace themis
