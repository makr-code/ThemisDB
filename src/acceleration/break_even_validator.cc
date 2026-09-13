/**
 * @file break_even_validator.cc
 * @brief Implementation of GPU Break-Even Validator
 * @version 1.0.0
 * @date 2026-07-06
 */

#include "acceleration/break_even_validator.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

#include "fmt/format.h"

namespace themis {
namespace acceleration {

namespace {

constexpr double kMinimumProfiledMs = 1.0;

double clampSelectivity(float selectivity) {
    return std::clamp(static_cast<double>(selectivity), 0.001, 1.0);
}

double normalizedDimension(size_t dimension) {
    if (dimension == 0) {
        return 1.0;
    }
    return std::max(1.0, static_cast<double>(dimension) / 128.0);
}

double log2Scaled(size_t value) {
    return value > 1 ? std::log2(static_cast<double>(value)) : 1.0;
}

double cpuThroughputUnitsPerMs(KernelType kernel) {
    switch (kernel) {
        case KernelType::kDistance:
            return 4'500.0;
        case KernelType::kTopK:
            return 20'000.0;
        case KernelType::kBFS:
            return 12'000.0;
        case KernelType::kDijkstra:
            return 8'000.0;
        case KernelType::kGeoDistance:
            return 16'000.0;
        case KernelType::kGeoContainment:
            return 9'500.0;
        default:
            return 0.0;
    }
}

double gpuThroughputUnitsPerMs(KernelType kernel, DeviceType device) {
    const double device_factor = [&]() {
        switch (device) {
            case DeviceType::kNVIDIA_RTX:
                return 1.0;
            case DeviceType::kAMD_MI210:
                return 0.9;
            case DeviceType::kNVIDIA_T4:
                return 0.6;
            case DeviceType::kIntel_Arc:
                return 0.45;
            default:
                return 0.0;
        }
    }();

    const double base = [&]() {
        switch (kernel) {
            case KernelType::kDistance:
                return 36'000.0;
            case KernelType::kTopK:
                return 70'000.0;
            case KernelType::kBFS:
                return 34'000.0;
            case KernelType::kDijkstra:
                return 23'000.0;
            case KernelType::kGeoDistance:
                return 42'000.0;
            case KernelType::kGeoContainment:
                return 27'000.0;
            default:
                return 0.0;
        }
    }();

    return base * device_factor;
}

double gpuLaunchOverheadMs(DeviceType device) {
    switch (device) {
        case DeviceType::kNVIDIA_RTX:
            return 2.8;
        case DeviceType::kAMD_MI210:
            return 3.4;
        case DeviceType::kNVIDIA_T4:
            return 4.2;
        case DeviceType::kIntel_Arc:
            return 5.3;
        default:
            return std::numeric_limits<double>::infinity();
    }
}

double estimateTransferBytes(const WorkloadProfile& profile) {
    const double input_size = static_cast<double>(profile.input_size);
    const double dimension = static_cast<double>(std::max<size_t>(profile.vector_dimension, 1));
    const double selectivity = clampSelectivity(profile.output_selectivity);

    switch (profile.kernel_type) {
        case KernelType::kDistance:
            return (input_size * dimension + dimension) * sizeof(float);
        case KernelType::kTopK:
            return (input_size * dimension + input_size * selectivity * 2.0) * sizeof(float);
        case KernelType::kBFS:
            return input_size * 6.0 * sizeof(std::uint32_t);
        case KernelType::kDijkstra:
            return input_size * 8.0 * sizeof(std::uint32_t);
        case KernelType::kGeoDistance:
            return input_size * 4.0 * sizeof(double);
        case KernelType::kGeoContainment:
            return input_size * 10.0 * sizeof(double);
        default:
            return 0.0;
    }
}

double effectiveBandwidthBytesPerMs(DeviceType device) {
    switch (device) {
        case DeviceType::kNVIDIA_RTX:
            return 18'000'000.0;
        case DeviceType::kAMD_MI210:
            return 16'000'000.0;
        case DeviceType::kNVIDIA_T4:
            return 11'000'000.0;
        case DeviceType::kIntel_Arc:
            return 8'500'000.0;
        default:
            return 0.0;
    }
}

} // namespace

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
        "WorkloadProfile{{kernel={}, input_size={}, dim={}, selectivity={:.2f}%, device={}}}",
        BreakEvenValidator::KernelTypeToString(kernel_type),
        input_size,
        vector_dimension,
        output_selectivity * 100.0f,
        BreakEvenValidator::DeviceTypeToString(device));
}

// ============================================================================
// Helper: BreakEvenDecision
// ============================================================================

std::string BreakEvenDecision::ToString() const {
    // NOTE: Prometheus metric emission for BreakEvenDecision is not yet wired.
    // Tracked: src/acceleration/ROADMAP.md § "BreakEvenValidator Prometheus Metrics"
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

    MetricsSinkFn metrics_sink;

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
        metrics_sink = metrics_sink_;
    }

    if (metrics_sink) {
        try {
            metrics_sink(profile, decision);
        } catch (const std::exception&) {
        } catch (...) {
        }
    }

    return decision;
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::ProfileCPU(
    const WorkloadProfile& profile) {
    ProfileFn profile_fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        profile_fn = cpu_profile_fn_;
    }

    if (profile_fn) {
        try {
            return profile_fn(profile);
        } catch (const std::exception&) {
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    if (profile.input_size == 0 || profile.kernel_type == KernelType::kUnknown) {
        return std::nullopt;
    }
    if (RequiresVectorDimension(profile.kernel_type) && profile.vector_dimension == 0) {
        return std::nullopt;
    }

    const double throughput = cpuThroughputUnitsPerMs(profile.kernel_type);
    if (throughput <= 0.0) {
        return std::nullopt;
    }

    const double fixed_overhead_ms = profile.kernel_type == KernelType::kDijkstra ? 0.65 : 0.25;
    const double estimated_ms = fixed_overhead_ms + EstimateWorkUnits(profile) / throughput;
    return MillisecondsFromEstimate(estimated_ms);
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::ProfileGPU(
    const WorkloadProfile& profile) {
    ProfileFn profile_fn;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        profile_fn = gpu_profile_fn_;
    }

    if (profile_fn) {
        try {
            return profile_fn(profile);
        } catch (const std::exception&) {
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    if (profile.input_size == 0 || profile.kernel_type == KernelType::kUnknown) {
        return std::nullopt;
    }
    if (RequiresVectorDimension(profile.kernel_type) && profile.vector_dimension == 0) {
        return std::nullopt;
    }
    if (!IsGpuCapableDevice(profile.device)) {
        return std::nullopt;
    }

    const double throughput = gpuThroughputUnitsPerMs(profile.kernel_type, profile.device);
    const double bandwidth = effectiveBandwidthBytesPerMs(profile.device);
    if (throughput <= 0.0 || bandwidth <= 0.0) {
        return std::nullopt;
    }

    double gpu_work_units = EstimateWorkUnits(profile);
    if (profile.kernel_type == KernelType::kTopK) {
        gpu_work_units *= 1.5 / (0.20 + clampSelectivity(profile.output_selectivity));
    }

    const double compute_ms = gpu_work_units / throughput;
    const double transfer_ms = estimateTransferBytes(profile) / bandwidth;
    const double estimated_ms = gpuLaunchOverheadMs(profile.device) + transfer_ms + compute_ms;
    return MillisecondsFromEstimate(estimated_ms);
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

void BreakEvenValidator::SetCPUProfileFn(ProfileFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    cpu_profile_fn_ = std::move(fn);
    decision_cache_.clear();
    latest_speedup_ratios_.clear();
}

void BreakEvenValidator::SetGPUProfileFn(ProfileFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    gpu_profile_fn_ = std::move(fn);
    decision_cache_.clear();
    latest_speedup_ratios_.clear();
}

void BreakEvenValidator::SetMetricsSink(MetricsSinkFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_sink_ = std::move(fn);
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

bool BreakEvenValidator::RequiresVectorDimension(KernelType kernel) {
    return kernel == KernelType::kDistance || kernel == KernelType::kTopK;
}

bool BreakEvenValidator::IsGpuCapableDevice(DeviceType device) {
    return device != DeviceType::kCPU && device != DeviceType::kUnknown;
}

double BreakEvenValidator::EstimateWorkUnits(const WorkloadProfile& profile) {
    const double input_size = static_cast<double>(profile.input_size);
    const double dimension_factor = normalizedDimension(profile.vector_dimension);
    const double selectivity = clampSelectivity(profile.output_selectivity);

    switch (profile.kernel_type) {
        case KernelType::kDistance:
            return input_size * dimension_factor * 128.0;
        case KernelType::kTopK:
            return input_size * std::max(1.0, log2Scaled(profile.input_size))
                * (0.35 + selectivity);
        case KernelType::kBFS:
            return input_size * (4.0 + selectivity * 6.0);
        case KernelType::kDijkstra:
            return input_size * std::max(1.0, log2Scaled(profile.input_size)) * 1.6;
        case KernelType::kGeoDistance:
            return input_size * 14.0;
        case KernelType::kGeoContainment:
            return input_size * (22.0 + selectivity * 10.0);
        default:
            return 0.0;
    }
}

std::optional<std::chrono::milliseconds> BreakEvenValidator::MillisecondsFromEstimate(
    double estimated_ms) {
    if (!std::isfinite(estimated_ms) || estimated_ms <= 0.0) {
        return std::nullopt;
    }
    const auto rounded = static_cast<long long>(std::ceil(std::max(estimated_ms, kMinimumProfiledMs)));
    return std::chrono::milliseconds(rounded);
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

KernelType BreakEvenValidator::StringToKernelType(
    const std::string& s) {
    if (s == "distance") {
      return KernelType::kDistance;
    }
    if (s == "topk") {
      return KernelType::kTopK;
    }
    if (s == "bfs") {
      return KernelType::kBFS;
    }
    if (s == "dijkstra") {
      return KernelType::kDijkstra;
    }
    if (s == "geo_distance") {
      return KernelType::kGeoDistance;
    }
    if (s == "geo_containment") {
      return KernelType::kGeoContainment;
    }
    return KernelType::kUnknown;
}

DeviceType BreakEvenValidator::StringToDeviceType(
    const std::string& s) {
    if (s == "nvidia_rtx") {
      return DeviceType::kNVIDIA_RTX;
    }
    if (s == "nvidia_t4") {
      return DeviceType::kNVIDIA_T4;
    }
    if (s == "amd_mi210") {
      return DeviceType::kAMD_MI210;
    }
    if (s == "intel_arc") {
      return DeviceType::kIntel_Arc;
    }
    if (s == "cpu") {
      return DeviceType::kCPU;
    }
    return DeviceType::kUnknown;
}

}  // namespace acceleration
}  // namespace themis
