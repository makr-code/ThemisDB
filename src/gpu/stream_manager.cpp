/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_manager.cpp                                 ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     174                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Stream Manager — named async GPU streams with CPU fallback budget.
 */

#include "themis/gpu/stream_manager.h"
#include <stdexcept>

namespace themis {
namespace gpu {

// ============================================================================
// Stream lifecycle
// ============================================================================

bool GPUStreamManager::createStream(const StreamConfig&    cfg,
                                     GPULauncher::BackendFn backend)
{
    if (cfg.name.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (streams_.count(cfg.name)) return false;   // already exists

    // Default no-op backend: work always succeeds (CPU path).
    GPULauncher::BackendFn fn = backend
        ? backend
        : [](const GPULauncher::WorkItem&) { return true; };

    Stream s;
    s.config   = cfg;
    s.launcher = std::make_unique<GPULauncher>(std::move(fn));
    s.stats.name = cfg.name;
    streams_.emplace(cfg.name, std::move(s));
    return true;
}

bool GPUStreamManager::destroyStream(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.erase(name) > 0;
}

bool GPUStreamManager::hasStream(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.count(name) > 0;
}

std::vector<std::string> GPUStreamManager::streamNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(streams_.size());
    for (const auto& kv : streams_) {
        names.push_back(kv.first);
    }
    return names;
}

size_t GPUStreamManager::streamCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return streams_.size();
}

// ============================================================================
// Work submission
// ============================================================================

std::future<GPULauncher::WorkResult>
GPUStreamManager::submit(const std::string&    stream_name,
                          GPULauncher::WorkItem item)
{
    // Hold the mutex for the duration of launcher->submit() — that call uses
    // std::async internally and returns a future immediately (non-blocking),
    // so holding the lock here is safe and prevents a concurrent destroyStream()
    // from invalidating the Stream object while we hold a pointer to it.
    uint32_t budget = 0;
    std::future<GPULauncher::WorkResult> inner_fut;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = streams_.find(stream_name);
        if (it == streams_.end()) {
            std::promise<GPULauncher::WorkResult> p;
            GPULauncher::WorkResult r;
            r.success       = false;
            r.kernel_id     = item.kernel_id;
            r.error_message = "stream '" + stream_name + "' does not exist";
            p.set_value(r);
            return p.get_future();
        }
        it->second.stats.submitted++;
        budget     = it->second.config.cpu_budget_ms;
        inner_fut  = it->second.launcher->submit(item);
    }

    // Post-process the result asynchronously (no mutex held here).
    return std::async(std::launch::async,
        [this, stream_name, budget, f = std::move(inner_fut)]() mutable
            -> GPULauncher::WorkResult {
            auto res = f.get();
            const uint64_t elapsed = static_cast<uint64_t>(res.elapsed.count());

            std::lock_guard<std::mutex> lock(mutex_);
            auto it = streams_.find(stream_name);
            if (it != streams_.end()) {
                auto& st = it->second.stats;
                // submitted was already incremented above; only update
                // outcome counters here.
                if (res.success) {
                    ++st.succeeded;
                } else {
                    ++st.failed;
                }
                st.total_elapsed_ms += elapsed;
                if (budget > 0 && elapsed > static_cast<uint64_t>(budget)) {
                    ++st.budget_exceeded;
                }
            }
            return res;
        });
}

// ============================================================================
// Statistics
// ============================================================================

GPUStreamManager::StreamStats
GPUStreamManager::getStreamStats(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(name);
    if (it == streams_.end()) {
        StreamStats empty;
        empty.name = name;
        return empty;
    }
    return it->second.stats;
}

std::vector<GPUStreamManager::StreamStats>
GPUStreamManager::getAllStreamStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<StreamStats> result;
    result.reserve(streams_.size());
    for (const auto& kv : streams_) {
        result.push_back(kv.second.stats);
    }
    return result;
}

} // namespace gpu
} // namespace themis
