/**
 * @file launcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <functional>
#include <future>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Typed GPU work-item and batched async launcher.
 *
 * Provides a safe, typed API for submitting GPU work without exposing raw
 * device handles.  Work is described as `WorkItem` descriptors; the launcher
 * groups them into batches and executes them via a caller-supplied backend.
 *
 * Two execution modes
 * -------------------
 * 1. **submit()** — submit a single work item, returns a `std::future<bool>`.
 *    The future resolves to true on success or false on failure/timeout.
 * 2. **submitBatch()** — submit a vector of work items, returns a
 *    `std::future<std::vector<bool>>` (one result per item).
 *
 * Backend integration
 * -------------------
 * The actual GPU call is supplied by the caller as a `BackendFn`
 * (`std::function<bool(const WorkItem&)>`).  The launcher does not depend on
 * any specific GPU runtime, making it testable without hardware.
 *
 * Timeout
 * -------
 * If `timeout_ms > 0` the future is abandoned and the work item counted as
 * failed after the timeout elapses.  This is implemented using `std::async`.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPULauncher {
public:
    // -----------------------------------------------------------------------
    // Work item
    // -----------------------------------------------------------------------
    struct WorkItem {
        std::string kernel_id;       ///< Identifier matching KernelValidator
        std::string tag;             ///< Caller-supplied label for audit/metrics
        std::string tenant_id;       ///< Tenant owning this work item
        std::vector<uint8_t> args;   ///< Serialised kernel arguments
        uint32_t    timeout_ms = 0;  ///< 0 = no timeout
    };

    // -----------------------------------------------------------------------
    // Result
    // -----------------------------------------------------------------------
    struct WorkResult {
        bool        success = false;
        std::string kernel_id;
        std::string error_message;
        std::chrono::milliseconds elapsed{0};
    };

    // -----------------------------------------------------------------------
    // Backend function type
    // -----------------------------------------------------------------------
    using BackendFn = std::function<bool(const WorkItem&)>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------
    explicit GPULauncher(BackendFn backend);

    // -----------------------------------------------------------------------
    // Submission
    // -----------------------------------------------------------------------

    /**
     * @brief Submit a single work item for asynchronous execution.
     *
     * @return Future resolving to the WorkResult when execution completes.
     */
    std::future<WorkResult> submit(WorkItem item);

    /**
     * @brief Submit a batch of work items.
     *
     * Items are executed in submission order via std::async.  The returned
     * future resolves when all items have completed.
     *
     * @return Future resolving to one WorkResult per item.
     */
    std::future<std::vector<WorkResult>> submitBatch(
        std::vector<WorkItem> items);

    // -----------------------------------------------------------------------
    // Stats
    // -----------------------------------------------------------------------
    struct Stats {
        size_t submitted      = 0;
        size_t succeeded      = 0;
        size_t failed         = 0;
        size_t timed_out      = 0;
        size_t batches_submitted = 0;
    };

    Stats getStats() const;

private:
    BackendFn backend_;
    mutable std::mutex stats_mutex_;
    Stats stats_;

    WorkResult executeOne(WorkItem item);
};

} // namespace gpu
} // namespace themis
