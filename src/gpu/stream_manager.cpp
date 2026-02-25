/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stream_manager.cpp                                 ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     169                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 140dad5bc  2026-02-22  feat(gpu): implement ROCm/HIP backend parity with CUDA fe... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Stream Manager — named async GPU streams with CPU fallback budget.
 */

#include "themis/gpu/stream_manager.h"
#include "themis/gpu/rocm_backend.h"
#include <stdexcept>

namespace themis {
namespace gpu {

// ============================================================================
// Construction / destruction
// ============================================================================

GPUStreamManager::~GPUStreamManager() {
    // Clean up any HIP streams that were created via ROCmBackend.
    for (const auto& kv : streams_) {
        if (kv.second.uses_rocm_stream) {
            ROCmBackend::GetInstance().destroyStream(kv.first);
        }
    }
}

// ============================================================================
// Stream lifecycle
// ============================================================================

bool GPUStreamManager::createStream(const StreamConfig&    cfg,
                                     GPULauncher::BackendFn backend)
{
    if (cfg.name.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (streams_.count(cfg.name)) return false;   // already exists

    Stream s;
    s.config     = cfg;
    s.stats.name = cfg.name;

    if (backend) {
        s.launcher = std::make_unique<GPULauncher>(std::move(backend));
    } else {
        // When no backend is supplied, create a real HIP stream via the ROCm
        // backend (which transparently falls back to CPU execution when
        // THEMIS_ENABLE_HIP is not defined) and use it as the execution backend.
        ROCmBackend::GetInstance().createStream(cfg.name);
        s.uses_rocm_stream = true;
        s.launcher = std::make_unique<GPULauncher>(
            ROCmBackend::GetInstance().createBackendFn());
    }

    streams_.emplace(cfg.name, std::move(s));
    return true;
}

bool GPUStreamManager::destroyStream(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = streams_.find(name);
    if (it == streams_.end()) return false;

    if (it->second.uses_rocm_stream) {
        ROCmBackend::GetInstance().destroyStream(name);
    }
    streams_.erase(it);
    return true;
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
