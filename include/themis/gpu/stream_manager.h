/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_manager.h                                   ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     185                                            ║
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

#pragma once

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include "themis/gpu/launcher.h"

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
     * @param backend  GPU execution backend.  Pass nullptr to use a no-op
     *                 backend (all work items succeed immediately via CPU).
     * @return false if a stream with that name already exists.
     */
    bool createStream(const StreamConfig&     cfg,
                      GPULauncher::BackendFn  backend = nullptr);

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
    GPUStreamManager() = default;

    struct Stream {
        StreamConfig               config;
        std::unique_ptr<GPULauncher> launcher;
        StreamStats                stats;
    };

    mutable std::mutex                          mutex_;
    std::unordered_map<std::string, Stream>     streams_;
};

} // namespace gpu
} // namespace themis
