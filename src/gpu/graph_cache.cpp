/**
 * @file graph_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "themis/gpu/graph_cache.h"

namespace themis {
namespace gpu {

// ============================================================================
// lookup
// ============================================================================

const GraphEntry *GPUGraphCache::lookup(const QueryShape &shape) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(shape);
    if (it == entries_.end()) {
        ++stats_.misses;
        stats_.entries = entries_.size();
        return nullptr;
    }

    GraphEntry &e = it->second;
    e.last_access = ++access_counter_;
    ++e.replay_count;
    ++stats_.hits;
    stats_.entries = entries_.size();
    return &e;
}

// ============================================================================
// capture
// ============================================================================

void GPUGraphCache::capture(const QueryShape &shape) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = entries_.find(shape);
    if (it != entries_.end()) {
        // Already captured — just increment the counter.
        ++it->second.capture_count;
        it->second.last_access = ++access_counter_;
        return;
    }

    // Evict the LRU entry if the cache is at capacity.
    if (static_cast<int>(entries_.size()) >= kMaxEntries) {
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

void GPUGraphCache::invalidate(const QueryShape &shape) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = entries_.find(shape);
    if (it == entries_.end()) {
        return;
    }

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
    return static_cast<int>(entries_.size());
}

GPUGraphCache::Stats GPUGraphCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s   = stats_;
    s.entries = entries_.size();
    return s;
}

// ============================================================================
// evictLRU (private)
// ============================================================================

void GPUGraphCache::evictLRU() {
    // Called with mutex_ already held.  O(n) scan is acceptable since n ≤ 32.
    if (entries_.empty()) {
        return;
    }

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
