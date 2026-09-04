/**
 * @file launcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Work Launcher — typed async work-item submission with batch support.
 */

#include "themis/gpu/launcher.h"
#include <stdexcept>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPULauncher::GPULauncher(BackendFn backend)
    : backend_(std::move(backend)) {
    if (!backend_) {
        throw std::invalid_argument(
            "GPULauncher: backend function must not be null");
    }
}

// ============================================================================
// executeOne — synchronous helper, called from std::async
// ============================================================================

GPULauncher::WorkResult GPULauncher::executeOne(WorkItem item) {
    WorkResult result;
    result.kernel_id = item.kernel_id;

    const auto start = std::chrono::steady_clock::now();

    bool ok = false;
    bool timed_out = false;

    if (item.timeout_ms > 0) {
        // Run the backend in a separate async task and wait with a timeout.
        auto exec_fut = std::async(std::launch::async,
            [this, it = item]() mutable { return backend_(it); });
        const auto status = exec_fut.wait_for(
            std::chrono::milliseconds(item.timeout_ms));
        if (status == std::future_status::timeout) {
            timed_out = true;
            result.error_message = "kernel execution timed out after " +
                                   std::to_string(item.timeout_ms) + " ms";
        } else {
            try {
                ok = exec_fut.get();
            } catch (const std::exception& e) {
                result.error_message = e.what();
            } catch (...) {
                result.error_message = "unknown exception in GPU backend";
            }
        }
    } else {
        try {
            ok = backend_(item);
        } catch (const std::exception& e) {
            result.error_message = e.what();
            ok = false;
        } catch (...) {
            result.error_message = "unknown exception in GPU backend";
            ok = false;
        }
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    result.success = ok;

    // Update stats.
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.submitted;
        if (timed_out) {
            ++stats_.failed;
            ++stats_.timed_out;
        } else if (ok) {
            ++stats_.succeeded;
        } else {
            ++stats_.failed;
        }
    }

    return result;
}

// ============================================================================
// submit
// ============================================================================

std::future<GPULauncher::WorkResult> GPULauncher::submit(WorkItem item) {
    return std::async(std::launch::async,
                      [this, item = std::move(item)]() mutable {
                          return executeOne(std::move(item));
                      });
}

// ============================================================================
// submitBatch
// ============================================================================

std::future<std::vector<GPULauncher::WorkResult>>
GPULauncher::submitBatch(std::vector<WorkItem> items) {
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.batches_submitted;
    }

    return std::async(std::launch::async,
        [this, items = std::move(items)]() mutable {
            std::vector<WorkResult> results = {};

            results.reserve(items.size());
            for (auto& item : items) {
                results.push_back(executeOne(std::move(item)));
            }
            return results;
        });
}

// ============================================================================
// Stats
// ============================================================================

GPULauncher::Stats GPULauncher::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

} // namespace gpu
} // namespace themis

