/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            launcher.cpp                                       ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:39:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     124                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    try {
        ok = backend_(item);
    } catch (const std::exception& e) {
        result.error_message = e.what();
        ok = false;
    } catch (...) {
        result.error_message = "unknown exception in GPU backend";
        ok = false;
    }

    result.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    result.success = ok;

    // Update stats.
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.submitted;
        if (ok) {
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
            std::vector<WorkResult> results;
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
