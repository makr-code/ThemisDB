/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graph_cache.cpp                                    ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 07:01:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     160                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • dfa2c62531  2026-02-25  Merge branch 'develop' into copilot/implement-gpu-profili... ║
    • 2379d35368  2026-02-25  fix(gpu): remove spurious stats increment and fix data ra... ║
    • 70833d6474  2026-02-25  feat(gpu): implement CUDA graph capture for recurring que... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "themis/gpu/graph_cache.h"

namespace themis {
namespace gpu {

// ============================================================================
// lookup
// ============================================================================

const GraphEntry* GPUGraphCache::lookup(const QueryShape& shape) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(shape);
    if (it == entries_.end()) {
        ++stats_.misses;
        stats_.entries = entries_.size();
        return nullptr;
    }

    GraphEntry& e = it->second;
    e.last_access = ++access_counter_;
    ++e.replay_count;
    ++stats_.hits;
    stats_.entries = entries_.size();
    return &e;
}

// ============================================================================
// capture
// ============================================================================

void GPUGraphCache::capture(const QueryShape& shape) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(shape);
    if (it != entries_.end()) {
        // Already captured — just increment the counter.
        ++it->second.capture_count;
        it->second.last_access = ++access_counter_;
        return;
    }

    // Evict the LRU entry if the cache is at capacity.
    if (entries_.size() >= kMaxEntries) {
        evictLRU();
    }

    GraphEntry e;
    e.shape         = shape;
    e.capture_count = 1;
    e.last_access   = ++access_counter_;
    entries_.emplace(shape, std::move(e));
    stats_.entries = entries_.size();

    // Production CUDA notes (no hardware in this build):
    //   cudaStream_t captureStream;
    //   cudaStreamCreateWithFlags(&captureStream, cudaStreamNonBlocking);
    //   cudaStreamBeginCapture(captureStream, cudaStreamCaptureModeGlobal);
    //   /* ... kernel launches ... */
    //   cudaStreamEndCapture(captureStream, &e.graph);
    //   cudaGraphInstantiate(&e.exec, e.graph, 0);
    //   cudaStreamDestroy(captureStream);
}

// ============================================================================
// invalidate
// ============================================================================

void GPUGraphCache::invalidate(const QueryShape& shape) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(shape);
    if (it == entries_.end()) return;

    // Production CUDA notes:
    //   if (it->second.exec)  cudaGraphExecDestroy(it->second.exec);
    //   if (it->second.graph) cudaGraphDestroy(it->second.graph);

    entries_.erase(it);
    stats_.entries = entries_.size();
}

// ============================================================================
// clear
// ============================================================================

void GPUGraphCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    // Production CUDA notes: iterate entries_ and call cudaGraphExecDestroy /
    // cudaGraphDestroy for each valid exec/graph handle before clearing.
    entries_.clear();
    stats_.entries = 0;
}

// ============================================================================
// size / getStats
// ============================================================================

size_t GPUGraphCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_.size();
}

GPUGraphCache::Stats GPUGraphCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s      = stats_;
    s.entries    = entries_.size();
    return s;
}

// ============================================================================
// evictLRU (private)
// ============================================================================

void GPUGraphCache::evictLRU() {
    // Called with mutex_ already held.  O(n) scan is acceptable since n ≤ 32.
    if (entries_.empty()) return;

    auto oldest = entries_.begin();
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.last_access < oldest->second.last_access) {
            oldest = it;
        }
    }

    // Production CUDA notes:
    //   if (oldest->second.exec)  cudaGraphExecDestroy(oldest->second.exec);
    //   if (oldest->second.graph) cudaGraphDestroy(oldest->second.graph);

    entries_.erase(oldest);
    ++stats_.evictions;
    stats_.entries = entries_.size();
}

} // namespace gpu
} // namespace themis
