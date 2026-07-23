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
#include <optional>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace themis {
namespace acceleration {

/**
 * @brief GPU kernel type classification for break-even analysis.
 *
 * Category A (Distance + TopK) and Category B (Graph algorithms) have different
 * speedup requirements based on theoretical parallelism and communication overhead.
 */
enum class KernelType {
    /// L2 distance computation (Category A)
    kDistance = 0,
    /// TopK selection kernel (Category A)
    kTopK = 1,
    /// Breadth-First Search (Category B)
    kBFS = 2,
    /// Dijkstra shortest path (Category B)
    kDijkstra = 3,
    /// Geospatial distance computation (Category B)
    kGeoDistance = 4,
    /// Geospatial containment check (Category B)
    kGeoContainment = 5,
    /// Invalid/unknown kernel type
    kUnknown = -1
};

/**
 * @brief GPU device type for break-even tracking.
 *
 * Different GPU architectures and generations have varying break-even points
 * due to memory bandwidth, compute density, and allocation overhead differences.
 */
enum class DeviceType {
    /// NVIDIA RTX 40-series, H100, etc.
    kNVIDIA_RTX = 0,
    /// NVIDIA T4, A100, L4, etc.
    kNVIDIA_T4 = 1,
    /// AMD MI210, MI300 series
    kAMD_MI210 = 2,
    /// Intel Arc A-series
    kIntel_Arc = 3,
    /// CPU-only fallback (no GPU)
    kCPU = 4,
    /// Unknown device type
    kUnknown = -1
};

/**
 * @brief Workload profile for break-even decision.
 *
 * Captures the input characteristics that determine whether GPU acceleration
 * is beneficial. Used as cache key and profiling input.
 */
struct WorkloadProfile {
    /// Kernel type (distance, topk, bfs, dijkstra, geo*)
    KernelType kernel_type = KernelType::kUnknown;

    /// Input cardinality (vector count or node count)
    size_t input_size = 0;

    /// Output selectivity ratio [0, 1] (e.g., 0.01 = top 1%)
    float output_selectivity = 1.0f;

    /// Vector dimension (for distance kernels)
    size_t vector_dimension = 0;

    /// Target device type
    DeviceType device = DeviceType::kCPU;

    /// Caller hint: force GPU usage regardless of break-even
    std::optional<bool> force_gpu;

    /// Caller hint: prefer CPU regardless of break-even
    std::optional<bool> prefer_cpu;

    /**
     * @brief Generate a stable cache key from profile.
     *
     * @return String key incorporating kernel, size, selectivity, device
     */
    std::string ToCacheKey() const;

    /**
     * @brief Human-readable representation for logging.
     *
     * @return Formatted profile string
     */
    std::string ToString() const;
};

/**
 * @brief Break-even validation decision with profiling results.
 *
 * Output of the break-even decision engine. Contains the decision (use GPU or
 * CPU fallback), the speedup ratio, individual timings, and reason code.
 */
struct BreakEvenDecision {
    /// True if GPU is recommended, false if CPU fallback is preferred
    bool use_gpu = false;

    /// Speedup ratio: CPU time / GPU time (> 1.0 means GPU is faster)
    float speedup_ratio = 0.0f;

    /// CPU path execution time (milliseconds)
    std::chrono::milliseconds cpu_time_ms;

    /// GPU path execution time including transfer (milliseconds)
    std::chrono::milliseconds gpu_time_ms;

    /// Human-readable reason: "break_even_met", "gpu_unavailable", etc.
    std::string reason;

    /// Whether this decision came from cache (for metrics)
    bool from_cache = false;

    /**
     * @brief Human-readable representation of decision and metrics.
     *
     * @return Formatted decision string with speedup and timing info
     */
    std::string ToString() const;
};

/**
 * @brief Break-Even Validator: GPU acceleration decision framework.
 *
 * Profiles GPU vs. CPU paths for target workloads and maintains a cache of
 * decisions. Supports custom thresholds per kernel type and cache invalidation.
 *
 * Thread-safe: all public methods are protected by internal mutex.
 *
 * Example usage:
 * @code
 *   BreakEvenValidator validator;
 *   WorkloadProfile profile{
 *       .kernel_type = KernelType::kDistance,
 *       .input_size = 1'000'000,
 *       .vector_dimension = 128,
 *       .device = DeviceType::kNVIDIA_RTX,
 *   };
 *   auto decision = validator.ShouldUseGPU(profile);
 *   if (decision.use_gpu) {
 *       DispatchToGPU(profile);
 *   } else {
 *       DispatchToCPU(profile);
 *   }
 * @endcode
 */
class BreakEvenValidator {
public:
    /**
     * @brief Construct a new BreakEvenValidator instance.
     *
     * Initializes with default speedup thresholds:
     * - Category A (Distance, TopK): 1.5x
     * - Category B (Graph): 1.3x
     *
     * Cache validity duration defaults to 24 hours.
     */
    BreakEvenValidator();

    /**
     * @brief Destructor (default).
     */
    ~BreakEvenValidator();

    /**
     * @brief Make a GPU vs. CPU decision for the given workload.
     *
     * Primary decision interface. Checks cache first; if cache miss, profiles
     * both CPU and GPU paths and makes a decision based on speedup thresholds.
     *
     * Respects force_gpu and prefer_cpu flags in the profile.
     *
     * @param profile Workload profile defining input characteristics
     * @return BreakEvenDecision with recommendation and metrics
     *
     * @thread Fully thread-safe; protected by internal mutex
     */
    BreakEvenDecision ShouldUseGPU(const WorkloadProfile& profile);

    /**
     * @brief Explicit profiling of CPU and GPU paths (cache bypass).
     *
     * Bypasses cache and profiles both CPU and GPU execution times,
     * then caches the result. Useful for:
     * - Diagnostics and debugging
     * - Refreshing cache entries
     * - Measuring performance regression
     *
     * @param profile Workload profile to profile
     * @return BreakEvenDecision with fresh profiling results
     *
     * @thread Fully thread-safe
     */
    BreakEvenDecision Profile(const WorkloadProfile& profile);

    /**
     * @brief Set the speedup threshold for a kernel type.
     *
     * Speedup threshold is the minimum CPU/GPU time ratio to recommend GPU.
     * For example, threshold=1.5 means GPU is used only if CPU is 1.5x slower.
     *
     * @param kernel Kernel type to set threshold for
     * @param threshold Minimum speedup ratio (must be >= 1.0)
     *
     * @thread Fully thread-safe
     */
    void SetSpeedupThreshold(KernelType kernel, float threshold);

    /**
     * @brief Get the current speedup threshold for a kernel type.
     *
     * @param kernel Kernel type
     * @return Current threshold for this kernel (or default 1.5 if not set)
     *
     * @thread Fully thread-safe
     */
    float GetSpeedupThreshold(KernelType kernel) const;

    /**
     * @brief Clear the entire decision cache.
     *
     * Next call to ShouldUseGPU() will trigger profiling (cache miss).
     *
     * @thread Fully thread-safe
     */
    void ClearCache();

    /**
     * @brief Set the time-to-live for cached decisions.
     *
     * Cached decisions expire after this duration. Default: 24 hours.
     * After expiry, ShouldUseGPU() will re-profile and refresh the cache.
     *
     * @param duration Cache validity duration
     *
     * @thread Fully thread-safe
     */
    void SetCacheValidityDuration(std::chrono::hours duration);

    /**
     * @brief Get the latest break-even speedup ratio for a kernel type.
     *
     * Returns the most recent speedup ratio observed for this kernel,
     * or 0.0 if no profile has been run yet.
     *
     * @param kernel Kernel type to query
     * @return Latest observed speedup ratio, or 0.0 if no data
     *
     * @thread Fully thread-safe
     */
    float GetLatestBreakEvenRatio(KernelType kernel) const;

    /**
     * @brief Get cumulative cache hit count for metrics.
     *
     * @return Number of ShouldUseGPU() calls that hit the cache
     *
     * @thread Fully thread-safe
     */
    size_t GetCacheHitCount() const;

    /**
     * @brief Get cumulative cache miss count for metrics.
     *
     * @return Number of ShouldUseGPU() calls that missed the cache
     *
     * @thread Fully thread-safe
     */
    size_t GetCacheMissCount() const;

    /**
     * @brief Get current cache size (number of cached decisions).
     *
     * @return Number of cached decision entries
     *
     * @thread Fully thread-safe
     */
    size_t GetCacheSize() const;

    /**
     * @brief Convert KernelType enum to string for logging/metrics.
     *
     * @param kernel Kernel type
     * @return String representation (e.g., "distance", "bfs")
     */
    static std::string KernelTypeToString(KernelType kernel);

    /**
     * @brief Convert DeviceType enum to string for logging/metrics.
     *
     * @param device Device type
     * @return String representation (e.g., "nvidia_rtx", "cpu")
     */
    static std::string DeviceTypeToString(DeviceType device);

private:
    /**
     * @brief Profile CPU execution time for the given workload.
     *
     * Delegates to CPU reference kernel implementations to measure
     * end-to-end execution time (no GPU transfer overhead).
     *
     * @param profile Workload to profile on CPU
     * @return CPU execution time, or nullopt if profiling failed
     */
    std::optional<std::chrono::milliseconds> ProfileCPU(
        const WorkloadProfile& profile);

    /**
     * @brief Profile GPU execution time for the given workload.
     *
     * Includes GPU allocation, data transfer, kernel execution, and sync time.
     * Returns nullopt if GPU is unavailable or profiling fails.
     *
     * @param profile Workload to profile on GPU
     * @return GPU execution time (transfer + compute), or nullopt if GPU unavailable
     */
    std::optional<std::chrono::milliseconds> ProfileGPU(
        const WorkloadProfile& profile);

    

    /**
     * @brief Parse KernelType from string.
     *
     * @param s String representation
     * @return Parsed KernelType, or kUnknown if not recognized
     */
    static KernelType StringToKernelType(const std::string& s);

    /**
     * @brief Parse DeviceType from string.
     *
     * @param s String representation
     * @return Parsed DeviceType, or kUnknown if not recognized
     */
    static DeviceType StringToDeviceType(const std::string& s);

    /**
     * @brief Cache entry with timestamp for expiry checking.
     */
    struct CacheEntry {
        BreakEvenDecision decision;
        std::chrono::steady_clock::time_point timestamp;

        /**
         * @brief Check if this entry has expired.
         *
         * @param ttl Cache time-to-live duration
         * @return True if entry is older than ttl
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
};

}  // namespace acceleration
}  // namespace themis
