/**
 * @file profiler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
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
#include <vector>

// NVTX support for NVIDIA Nsight Systems / Nsight Compute range markers.
// Requires the CUDA Toolkit; gated to avoid hard dependency on CUDA headers.
#ifdef THEMIS_ENABLE_CUDA
#  include <nvToolsExt.h>
#endif

// rocTX support for AMD ROCm Profiler range markers.
// Requires the ROCm toolkit; gated to avoid hard dependency on ROCm headers.
#ifdef THEMIS_ENABLE_HIP
#  include <roctx.h>
#endif

namespace themis {
namespace gpu {

/**
 * @brief GPU profiling integration for NVIDIA Nsight and AMD ROCm Profiler.
 *
 * Provides named range markers and point events that are natively visible in:
 *  - NVIDIA Nsight Systems / Nsight Compute (via NVTX, when THEMIS_ENABLE_CUDA)
 *  - AMD ROCm Profiler / rocprof (via rocTX, when THEMIS_ENABLE_HIP)
 *  - CPU-only builds: events are stored internally and exportable as
 *    Chrome trace JSON compatible with ROCm profiler's `--sys-trace` output.
 *
 * Typical usage with RAII helper
 * --------------------------------
 * ```cpp
 * {
 *     ScopedGPURange range("vector_search");
 *     // work here ...
 * }  // range.endRange() called automatically
 * ```
 *
 * Manual push/pop
 * ---------------
 * ```cpp
 * auto& prof = GPUProfiler::GetInstance();
 * prof.beginRange("knn_query", 0xFF0080FF);  // blue
 * doWork();
 * prof.endRange();
 * ```
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUProfiler {
public:
    // -----------------------------------------------------------------------
    // Range record
    // -----------------------------------------------------------------------

    /**
     * @brief A completed profiling range (or point event when start_ns == end_ns).
     */
    struct Range {
        std::string name;
        uint64_t    start_ns  = 0;  ///< Wall-clock start in nanoseconds
        uint64_t    end_ns    = 0;  ///< Wall-clock end in nanoseconds
        int         device_id = 0;  ///< GPU device ordinal (0 = default)
        uint32_t    color     = 0xFF00FF00; ///< ARGB color hint for profiler UI
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUProfiler& GetInstance() {
        static GPUProfiler inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Range markers
    // -----------------------------------------------------------------------

    /**
     * @brief Push a named profiling range.
     *
     * On CUDA builds emits an NVTX `nvtxRangePushEx` with the given ARGB
     * color.  On HIP builds emits a `roctxRangePushA`.  On CPU-only builds
     * records the start timestamp internally.
     *
     * @param name       Human-readable range label (shown in profiler UI).
     * @param argb_color ARGB colour for the range in the Nsight/rocTX UI.
     *                   Default: opaque green (0xFF00FF00).
     */
    void beginRange(const std::string& name, uint32_t argb_color = 0xFF00FF00);

    /**
     * @brief Pop the most recently pushed range.
     *
     * Emits `nvtxRangePop` / `roctxRangePop` on hardware builds and records
     * the completed range internally.  A call without a matching `beginRange`
     * is silently ignored.
     */
    void endRange();

    /**
     * @brief Record a point event (instant marker).
     *
     * Emits `nvtxMarkA` / `roctxMarkA` on hardware builds.  Stored
     * internally as a zero-duration range (start_ns == end_ns).
     *
     * @param name  Event label.
     */
    void markEvent(const std::string& name);

    // -----------------------------------------------------------------------
    // Export
    // -----------------------------------------------------------------------

    /**
     * @brief Export completed ranges in Chrome trace JSON format.
     *
     * The output is compatible with:
     *  - AMD ROCm profiler's `--sys-trace` JSON output format
     *  - Perfetto / `chrome://tracing` for offline analysis
     *
     * @return UTF-8 JSON string.  The `"traceEvents"` array is present but
     *         empty when no ranges have been recorded.
     */
    std::string rocm_profiler_export() const;

    /**
     * @brief Return a copy of all completed ranges.
     */
    std::vector<Range> getRanges() const;

    /**
     * @brief Reset all recorded ranges (for testing).
     *
     * Does not affect any in-flight hardware profiling sessions.
     */
    void reset();

private:
    GPUProfiler() = default;

    mutable std::mutex mutex_;

    struct ActiveRange {
        std::string name;
        uint64_t    start_ns;
        uint32_t    color;
    };

    std::vector<ActiveRange> range_stack_;
    std::vector<Range>       completed_ranges_;

    static uint64_t nowNs();
};

/**
 * @brief RAII guard that pushes a named profiling range on construction and
 *        pops it on destruction.
 *
 * Example:
 * ```cpp
 * void processQuery() {
 *     ScopedGPURange rng("processQuery");
 *     // ...
 * }
 * ```
 */
class ScopedGPURange {
public:
    explicit ScopedGPURange(const std::string& name,
                             uint32_t argb_color = 0xFF00FF00);
    ~ScopedGPURange();

    ScopedGPURange(const ScopedGPURange&)            = delete;
    ScopedGPURange& operator=(const ScopedGPURange&) = delete;

private:
    GPUProfiler& profiler_;
};

} // namespace gpu
} // namespace themis
