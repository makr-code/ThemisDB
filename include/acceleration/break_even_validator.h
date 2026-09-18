/**
 * @file break_even_validator.h
 * @brief GPU Break-Even Decision Framework for ThemisDB Hybrid Retrieval
 * @version 1.0.0
 * @date 2026-07-06
 *
 * Provides deterministic, reproducible break-even thresholds for GPU acceleration
 * decisions. Profiles CPU vs. GPU paths and caches decisions based on workload profile.
 *
 * @note Maturity: 🟡 BETA (Phase D1)
 * @note Status: Implementation in progress
 */

#pragma once

#include <string>
#include <chrono>
#include <functional>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace themis {
namespace acceleration {

enum class KernelType {
    kDistance = 0,
    kTopK = 1,
    kBFS = 2,
    kDijkstra = 3,
    kGeoDistance = 4,
    kGeoContainment = 5,
    kUnknown = -1
};

enum class DeviceType {
    kNVIDIA_RTX = 0,
    kNVIDIA_T4 = 1,
    kAMD_MI210 = 2,
    kIntel_Arc = 3,
    kCPU = 4,
    kUnknown = -1
};

struct WorkloadProfile {
    KernelType kernel_type = KernelType::kUnknown;

    size_t input_size = 0;

    float output_selectivity = 1.0f;

    size_t vector_dimension = 0;

    DeviceType device = DeviceType::kCPU;

    std::optional<bool> force_gpu;

    std::optional<bool> prefer_cpu;

    /**
     * @brief To Cache Key.
     * @return Return value.
     */
    std::string ToCacheKey() const;

    /**
     * @brief To String.
     * @return Return value.
     */
    std::string ToString() const;
};

struct BreakEvenDecision {
    bool use_gpu = false;

    float speedup_ratio = 0.0f;

    std::chrono::milliseconds cpu_time_ms{0};

    std::chrono::milliseconds gpu_time_ms{0};

    std::string reason;

    bool from_cache = false;

    /**
     * @brief To String.
     * @return Return value.
     */
    std::string ToString() const;
};

class BreakEvenValidator {
public:
    using ProfileFn = std::function<std::optional<std::chrono::milliseconds>(
        const WorkloadProfile&)>;
    using MetricsSinkFn = std::function<void(
        const WorkloadProfile&, const BreakEvenDecision&)>;

    BreakEvenValidator();

    ~BreakEvenValidator();

    /**
     * @brief Should Use GPU.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    BreakEvenDecision ShouldUseGPU(const WorkloadProfile& profile);

    /**
     * @brief Profile.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    BreakEvenDecision Profile(const WorkloadProfile& profile);

    /**
     * @brief Set Speedup Threshold.
     * @param[in] kernel Input parameter.
     * @param[in] threshold Input parameter.
     */
    void SetSpeedupThreshold(KernelType kernel, float threshold);

    /**
     * @brief Get Speedup Threshold.
     * @param[in] kernel Input parameter.
     * @return Return value.
     */
    float GetSpeedupThreshold(KernelType kernel) const;

    /**
     * @brief Clear Cache.
     */
    void ClearCache();

    /**
     * @brief Set Cache Validity Duration.
     * @param[in] duration Input parameter.
     */
    void SetCacheValidityDuration(std::chrono::hours duration);

    /**
     * @brief Set CPUProfile Fn.
     * @param[in] fn Input parameter.
     */
    void SetCPUProfileFn(ProfileFn fn);

    /**
     * @brief Set GPUProfile Fn.
     * @param[in] fn Input parameter.
     */
    void SetGPUProfileFn(ProfileFn fn);

    /**
     * @brief Set Metrics Sink.
     * @param[in] fn Input parameter.
     */
    void SetMetricsSink(MetricsSinkFn fn);

    /**
     * @brief Get Latest Break Even Ratio.
     * @param[in] kernel Input parameter.
     * @return Return value.
     */
    float GetLatestBreakEvenRatio(KernelType kernel) const;

    /**
     * @brief Get Cache Hit Count.
     * @return Return value.
     */
    size_t GetCacheHitCount() const;

    /**
     * @brief Get Cache Miss Count.
     * @return Return value.
     */
    size_t GetCacheMissCount() const;

    /**
     * @brief Get Cache Size.
     * @return Return value.
     */
    size_t GetCacheSize() const;

    /**
     * @brief Kernel Type To String.
     * @param[in] kernel Input parameter.
     * @return Return value.
     */
    static std::string KernelTypeToString(KernelType kernel);

    /**
     * @brief Device Type To String.
     * @param[in] device Input parameter.
     * @return Return value.
     */
    static std::string DeviceTypeToString(DeviceType device);

private:
    /**
     * @brief Profile CPU.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    std::optional<std::chrono::milliseconds> ProfileCPU(
        const WorkloadProfile& profile);

    /**
     * @brief Profile GPU.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    std::optional<std::chrono::milliseconds> ProfileGPU(
        const WorkloadProfile& profile);

    /**
     * @brief Requires Vector Dimension.
     * @param[in] kernel Input parameter.
     * @return True when the operation succeeds.
     */
    static bool RequiresVectorDimension(KernelType kernel);
    /**
     * @brief Is Gpu Capable Device.
     * @param[in] device Input parameter.
     * @return True when the operation succeeds.
     */
    static bool IsGpuCapableDevice(DeviceType device);
    /**
     * @brief Estimate Work Units.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    static double EstimateWorkUnits(const WorkloadProfile& profile);
    /**
     * @brief Milliseconds From Estimate.
     * @param[in] estimated_ms Input parameter.
     * @return Return value.
     */
    static std::optional<std::chrono::milliseconds> MillisecondsFromEstimate(
        double estimated_ms);

    /**
     * @brief String To Kernel Type.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static KernelType StringToKernelType(const std::string& s);

    /**
     * @brief String To Device Type.
     * @param[in] s Input parameter.
     * @return Return value.
     */
    static DeviceType StringToDeviceType(const std::string& s);

    struct CacheEntry {
        BreakEvenDecision decision;
        std::chrono::steady_clock::time_point timestamp;

        /**
         * @brief Is Expired.
         * @param[in] ttl Input parameter.
         * @return True when the operation succeeds.
         */
        bool IsExpired(std::chrono::hours ttl) const;
    };

    // Mutable state protected by mutex_
    mutable std::mutex mutex_;

    // Decision cache: WorkloadProfile cache key -> cached decision + timestamp
    std::unordered_map<std::string, CacheEntry> decision_cache_;

    // Per-kernel speedup thresholds
    std::unordered_map<int, float> speedup_thresholds_;

    // Latest speedup ratio per kernel (for metrics)
    std::unordered_map<int, float> latest_speedup_ratios_;

    // Cache statistics
    size_t cache_hits_ = 0;
    size_t cache_misses_ = 0;

    // Cache configuration
    std::chrono::hours cache_validity_duration_;

    // Optional profiling/metrics hooks
    ProfileFn cpu_profile_fn_;
    ProfileFn gpu_profile_fn_;
    MetricsSinkFn metrics_sink_;
};

}  // namespace acceleration
}  // namespace themis
