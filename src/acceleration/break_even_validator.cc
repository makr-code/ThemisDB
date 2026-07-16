/**
 * @file break_even_validator.cc
 * @brief Implementation of GPU Break-Even Validator
 * @version 1.0.0
 * @date 2026-07-06
 */

#include "acceleration/break_even_validator.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

#include "fmt/format.h"

namespace themis {
namespace acceleration {

// ============================================================================
// Helper: WorkloadProfile
// ============================================================================

std::string WorkloadProfile::ToCacheKey() const {
    return fmt::format("{}_{}_{}_{:.2f}",
        BreakEvenValidator::KernelTypeToString(kernel_type),
        input_size,
        vector_dimension,
        output_selectivity);
}

std::string WorkloadProfile::ToString() const {
    return fmt::format(
        "WorkloadProfile{{kernel={}, input_size={}, dim={}, selectivity={:.2%}, device={}}}",
        BreakEvenValidator::KernelTypeToString(kernel_type),
        input_size,
        vector_dimension,
        output_selectivity,
        BreakEvenValidator::DeviceTypeToString(device));
}

// ============================================================================
// Helper: BreakEvenDecision
// ============================================================================

std::string BreakEvenDecision::ToString() const {
    return fmt::format(
        "BreakEvenDecision{{use_gpu={}, speedup={:.2f}x, cpu={}ms, gpu={}ms, reason={}, cached={}}}",
        use_gpu ? "true" : "false",
        speedup_ratio,
        cpu_time_ms.count(),
        gpu_time_ms.count(),
        reason,
        from_cache ? "true" : "false");
}

// ============================================================================
// Helper: CacheEntry
// ============================================================================

bool BreakEvenValidator::CacheEntry::IsExpired(std::chrono::hours ttl) const {
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - timestamp);
    return age >= ttl;
}

// ============================================================================
// BreakEvenValidator Implementation
// ============================================================================

BreakEvenValidator::BreakEvenValidator()
    : cache_validity_duration_(std::chrono::hours(24)) {
    // Initialize default speedup thresholds
    // Category A (Distance, TopK): 1.5x speedup required
    speedup_thresholds_[static_cast<int>(KernelType::kDistance)] = 1.5f;
    speedup_thresholds_[static_cast<int>(KernelType::kTopK)] = 1.5f;

    // Category B (Graph algorithms): 1.3x speedup required
    speedup_thresholds_[static_cast<int>(KernelType::kBFS)] = 1.3f;
    speedup_thresholds_[static_cast<int>(KernelType::kDijkstra)] = 1.3f;
    speedup_thresholds_[static_cast<int>(KernelType::kGeoDistance)] = 1.3f;
    speedup_thresholds_[static_cast<int>(KernelType::kGeoContainment)] = 1.3f;
}

BreakEvenValidator::~BreakEvenValidator() = default;

BreakEvenDecision BreakEvenValidator::ShouldUseGPU(
    const WorkloadProfile& profile) {
    // Honor explicit caller overrides
    if (profile.force_gpu && *profile.force_gpu) {
        return {
            .use_gpu = true,
            .speedup_ratio = 0.0f,
            .reason = "force_gpu_flag",
            .from_cache = false,
        };
    }
    if (profile.prefer_cpu && *profile.prefer_cpu) {
        return {
            .use_gpu = false,
            .speedup_ratio = 0.0f,
            .reason = "prefer_cpu_flag",
            .from_cache = false,
        };
    }

    // Check cache
    std::string cache_key = profile.ToCacheKey();
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = decision_cache_.find(cache_key);
        if (it != decision_cache_.end()) {
            const auto& entry = it->second;
            if (!entry.IsExpired(cache_validity_duration_)) {
                cache_hits_++;
                auto decision = entry.decision;
                decision.from_cache = true;
                return decision;
            } else {
                // Expired, will re-profile below
                decision_cache_.erase(it);
            }
        }
    }

    // Cache miss or expired: profile both paths
    return Profile(profile);
}

BreakEvenDecision BreakEvenValidator::Profile(
    const WorkloadProfile& profile) {
    // Profile CPU path
    auto cpu_time = ProfileCPU(profile);
    if (!cpu_time) {
        // CPU profiling failed
        return {
            .use_gpu = false,
            .speedup_ratio = 0.0f,
            .reason = "cpu_profile_failed",
            .from_cache = false,
        };
    }

    // Profile GPU path
    auto gpu_time = ProfileGPU(profile);
    if (!gpu_time) {
        // GPU unavailable or profiling failed
        return {
            .use_gpu = false,
            .speedup_ratio = 0.0f,
            .cpu_time_ms = *cpu_time,
            .reason = "gpu_unavailable",
            .from_cache = false,
        };
    }

    // Compute speedup ratio
    float speedup_ratio = static_cast<float>(cpu_time->count()) /
                          static_cast<float>(gpu_time->count());
    float threshold = GetSpeedupThreshold(profile.kernel_type);

    BreakEvenDecision decision;
    decision.cpu_time_ms = *cpu_time;
    decision.gpu_time_ms = *gpu_time;
    decision.speedup_ratio = speedup_ratio;
    decision.use_gpu = (speedup_ratio >= threshold);
    decision.reason = decision.use_gpu ? "break_even_met" : "break_even_not_met";
    decision.from_cache = false;

    // Record latest speedup ratio for metrics
    {
        std::lock_guard<std::mutex> lock(mutex_);
        latest_speedup_ratios_[static_cast<int>(profile.kernel_type)] = speedup_ratio;
        cache_misses_++;

        // Cache the decision
        decision_cache_[profile.ToCacheKey()] = {
            .decision = decision,
            .timestamp = std::chrono::steady_clock::now(),
        };
    }

    // Export metrics (TODO: integrate with Prometheus)
    // metrics::prometheus::RecordHistogram("gpu_acceleration_break_even_ratio",
    //     speedup_ratio,
    //     {{"kernel", KernelTypeToString(profile.kernel_type)}});

    return decision;
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::ProfileCPU(
    const WorkloadProfile& profile) {
    // TODO: Delegate to CPU reference kernel implementations
    // For now, return a placeholder timing
    // In production, this would:
    // 1. Call CPU reference kernel (distance, topk, bfs, dijkstra, geo*)
    // 2. Measure actual execution time with high-resolution timer
    // 3. Repeat multiple times and take median

    // Placeholder: CPU kernels typically take 10-100ms depending on input size
    if (profile.input_size < 1000) {
        return std::chrono::milliseconds(5);
    } else if (profile.input_size < 100000) {
        return std::chrono::milliseconds(50);
    } else {
        return std::chrono::milliseconds(500);
    }
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::ProfileGPU(
    const WorkloadProfile& profile) {
    // TODO: Delegate to GPU kernel implementation
    // For now, return a placeholder timing
    // In production, this would:
    // 1. Check GPU availability and select best device
    // 2. Allocate GPU memory for input/output
    // 3. Transfer input data to GPU
    // 4. Launch kernel and measure execution time
    // 5. Transfer results back to host
    // 6. Return total time (alloc + transfer + compute + sync)

    // Placeholder: GPU kernels have fixed overhead (~20ms) + execution
    if (profile.input_size < 10000) {
        // Too small: overhead dominates
        return std::nullopt;  // Not worth it
    } else if (profile.input_size < 1000000) {
        return std::chrono::milliseconds(35);
    } else {
        return std::chrono::milliseconds(300);
    }
}

void BreakEvenValidator::SetSpeedupThreshold(KernelType kernel, float threshold) {
    std::lock_guard<std::mutex> lock(mutex_);
    speedup_thresholds_[static_cast<int>(kernel)] = std::max(1.0f, threshold);
}

float BreakEvenValidator::GetSpeedupThreshold(KernelType kernel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = speedup_thresholds_.find(static_cast<int>(kernel));
    return it != speedup_thresholds_.end() ? it->second : 1.5f;
}

void BreakEvenValidator::ClearCache() {
    std::lock_guard<std::mutex> lock(mutex_);
    decision_cache_.clear();
}

void BreakEvenValidator::SetCacheValidityDuration(std::chrono::hours duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_validity_duration_ = duration;
}

float BreakEvenValidator::GetLatestBreakEvenRatio(KernelType kernel) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = latest_speedup_ratios_.find(static_cast<int>(kernel));
    return it != latest_speedup_ratios_.end() ? it->second : 0.0f;
}

size_t BreakEvenValidator::GetCacheHitCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_hits_;
}

size_t BreakEvenValidator::GetCacheMissCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_misses_;
}

size_t BreakEvenValidator::GetCacheSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return decision_cache_.size();
}

std::string BreakEvenValidator::KernelTypeToString(KernelType kernel) {
    switch (kernel) {
        case KernelType::kDistance:
            return "distance";
        case KernelType::kTopK:
            return "topk";
        case KernelType::kBFS:
            return "bfs";
        case KernelType::kDijkstra:
            return "dijkstra";
        case KernelType::kGeoDistance:
            return "geo_distance";
        case KernelType::kGeoContainment:
            return "geo_containment";
        default:
            return "unknown";
    }
}

std::string BreakEvenValidator::DeviceTypeToString(DeviceType device) {
    switch (device) {
        case DeviceType::kNVIDIA_RTX:
            return "nvidia_rtx";
        case DeviceType::kNVIDIA_T4:
            return "nvidia_t4";
        case DeviceType::kAMD_MI210:
            return "amd_mi210";
        case DeviceType::kIntel_Arc:
            return "intel_arc";
        case DeviceType::kCPU:
            return "cpu";
        default:
            return "unknown";
    }
}

BreakEvenValidator::KernelType BreakEvenValidator::StringToKernelType(
    const std::string& s) {
    if (s == "distance") return KernelType::kDistance;
    if (s == "topk") return KernelType::kTopK;
    if (s == "bfs") return KernelType::kBFS;
    if (s == "dijkstra") return KernelType::kDijkstra;
    if (s == "geo_distance") return KernelType::kGeoDistance;
    if (s == "geo_containment") return KernelType::kGeoContainment;
    return KernelType::kUnknown;
}

BreakEvenValidator::DeviceType BreakEvenValidator::StringToDeviceType(
    const std::string& s) {
    if (s == "nvidia_rtx") return DeviceType::kNVIDIA_RTX;
    if (s == "nvidia_t4") return DeviceType::kNVIDIA_T4;
    if (s == "amd_mi210") return DeviceType::kAMD_MI210;
    if (s == "intel_arc") return DeviceType::kIntel_Arc;
    if (s == "cpu") return DeviceType::kCPU;
    return DeviceType::kUnknown;
}

}  // namespace acceleration
}  // namespace themis
