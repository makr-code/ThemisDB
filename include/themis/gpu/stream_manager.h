/**
 * @file stream_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/launcher.h"
#include "themis/gpu/rocm_backend.h"

namespace themis {
namespace gpu {

/**
 * @brief Named async GPU stream manager.
 *
 * Manages a collection of named GPU streams, each backed by an independent
 * GPULauncher instance.  Streams provide isolation between workloads: a slow
 * or failing stream does not block submissions on other streams.
 *
 * CPU fallback budget
 * -------------------
 * When `StreamConfig::cpu_budget_ms > 0`, the manager records a
 * budget-exceeded event if a submitted work item takes longer than that
 * budget on the CPU fallback path.  The event is visible in `StreamStats`
 * via the `budget_exceeded` counter.
 *
 * Typical usage
 * -------------
 * ```cpp
 * auto& sm = GPUStreamManager::GetInstance();
 * GPUStreamManager::StreamConfig cfg;
 * cfg.name           = "vector_search";
 * cfg.cpu_budget_ms  = 50;   // warn if CPU fallback > 50 ms
 * sm.createStream(cfg, myGPUBackend);
 *
 * auto fut = sm.submit("vector_search", {.kernel_id = "knn", ...});
 * auto res = fut.get();
 * ```
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUStreamManager {
public:
    // -----------------------------------------------------------------------
    // Stream configuration
    // -----------------------------------------------------------------------
    struct StreamConfig {
        std::string name;
        /**
         * @brief CPU fallback performance budget in milliseconds.
         * 0 = no enforcement.
         */
        uint32_t cpu_budget_ms = 0;
        /**
         * @brief When true, a failed GPU dispatch automatically retries via
         * the CPU fallback backend (null backend → always CPU).
         */
        bool auto_fallback = true;
    };

    // -----------------------------------------------------------------------
    // Per-stream statistics
    // -----------------------------------------------------------------------
    struct StreamStats {
        std::string name;
        size_t   submitted        = 0;  ///< Total work items submitted
        size_t   succeeded        = 0;  ///< Completed successfully
        size_t   failed           = 0;  ///< Completed with failure
        /**
         * @brief Work items that exceeded the cpu_budget_ms threshold.
         *
         * Populated when StreamConfig::cpu_budget_ms > 0 and the elapsed
         * time for a work item exceeds that budget.
         */
        size_t   budget_exceeded  = 0;
        uint64_t total_elapsed_ms = 0;  ///< Cumulative elapsed ms
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Default constructor.
     *
     * Allows constructing local instances for testing and non-singleton use.
     * Use GetInstance() for the process-wide singleton.
     */
    GPUStreamManager() = default;
    ~GPUStreamManager();

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUStreamManager& GetInstance() {
        static GPUStreamManager inst;
        return inst;
    }

    // -----------------------------------------------------------------------
    // Stream lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Create a new named stream.
     *
     * @param cfg      Stream configuration.
     * @param backend  GPU execution backend.  Pass nullptr to use the ROCm
     *                 backend (which creates a named HIP stream and transparently
     *                 falls back to CPU execution when `THEMIS_ENABLE_HIP` is absent).
     *                 When `THEMIS_ENABLE_CUDA` is also active a `cudaStream_t` is
     *                 created alongside and stored for future kernel dispatch.
     * @return false if a stream with that name already exists.
     */
    bool createStream(const StreamConfig&     cfg,
                      GPULauncher::BackendFn  backend = nullptr);

    /**
     * @brief Create a new named CUDA stream on @p device_index.
     *
     * Wires a CUDA-backed execution path into the stream when
     * `THEMIS_ENABLE_CUDA` is defined; falls back to the ROCm/CPU backend
     * when CUDA is unavailable so the call is always safe to use.
     *
     * This overload resolves the "Stubs: 1" noted in the stream_manager header
     * by providing a first-class CUDA stream creation path alongside the
     * existing ROCm path.
     *
     * @param cfg           Stream configuration (name must be non-empty).
     * @param device_index  CUDA device ordinal (0-based).
     * @return false if a stream with that name already exists or cfg.name is empty.
     */
    bool createCudaStream(const StreamConfig& cfg, int device_index = 0);

    /**
     * @brief Destroy a named stream.
     *
     * @return false if no stream with that name exists.
     */
    bool destroyStream(const std::string& name);

    bool hasStream(const std::string& name) const;
    std::vector<std::string> streamNames() const;
    size_t streamCount() const;

    // -----------------------------------------------------------------------
    // Work submission
    // -----------------------------------------------------------------------

    /**
     * @brief Submit a work item to the named stream.
     *
     * @return A future that resolves to the WorkResult.  The future holds an
     *         error result if the stream does not exist.
     */
    std::future<GPULauncher::WorkResult> submit(const std::string&         stream_name,
                                                 GPULauncher::WorkItem      item);

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Return stats for a single stream.
     *
     * Returns a zero-filled StreamStats (with the given name) if the stream
     * does not exist.
     */
    StreamStats getStreamStats(const std::string& name) const;

    /**
     * @brief Return stats for all registered streams.
     */
    std::vector<StreamStats> getAllStreamStats() const;

private:

    struct Stream {
        StreamConfig               config;
        std::unique_ptr<GPULauncher> launcher;
        StreamStats                stats;
        uintptr_t                  cuda_stream      = 0;     ///< cudaStream_t handle; 0 when not created.
        bool                       uses_rocm_stream = false; ///< true when a HIP stream was registered via ROCmBackend.
    };

    mutable std::mutex                          mutex_;
    std::unordered_map<std::string, Stream>     streams_;

public:
    // -----------------------------------------------------------------------
    // Injectable CUDA backend bridge (STUB #77)
    // -----------------------------------------------------------------------
    /// Callback type: given a device index, return a GPULauncher::BackendFn
    /// that dispatches work to a CUDA (or compatible) device.  Used as a
    /// replacement for the real cudaStream_t path when THEMIS_ENABLE_CUDA is
    /// not defined so that callers can inject a CUDA-like backend without
    /// rebuilding with the CUDA Toolkit.
    using CudaStreamBackendFn =
        std::function<GPULauncher::BackendFn(int device_index)>;

    /// Register a CUDA backend factory used by `createCudaStream()` when
    /// THEMIS_ENABLE_CUDA is not defined.
    /// Pass an empty `std::function` to clear and revert to the ROCm/CPU fallback.
    /// Thread-safe (guarded by a static mutex).
    static void setCudaStreamBackendFn(CudaStreamBackendFn fn);
};


} // namespace gpu
} // namespace themis
