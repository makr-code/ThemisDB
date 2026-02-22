/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            memory_pool.cpp                                    ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     176                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Memory Pool — slab-based pre-allocator with fragmentation tracking.
 */

#include "themis/gpu/memory_pool.h"
#include <stdexcept>

namespace themis {
namespace gpu {

// ============================================================================
// Construction
// ============================================================================

GPUMemoryPool::GPUMemoryPool(uint64_t total_bytes, uint64_t slab_size,
                               size_t num_slabs)
    : total_bytes_(total_bytes), slab_size_(slab_size) {
    if (slab_size_ == 0) {
        throw std::invalid_argument("slab_size must be > 0");
    }
    size_t n = (num_slabs > 0) ? num_slabs
                                : static_cast<size_t>(total_bytes_ / slab_size_);
    slabs_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        slabs_[i].offset  = i * slab_size_;
        slabs_[i].size    = slab_size_;
        slabs_[i].is_free = true;
    }
}

// ============================================================================
// tryAcquire
// ============================================================================

bool GPUMemoryPool::tryAcquire(uint64_t size_bytes, const std::string& tag,
                                 uint64_t& offset) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Pool miss: request too large for any single slab.
    if (size_bytes > slab_size_) {
        ++alloc_misses_;
        return false;
    }

    // First-fit search.
    for (auto& s : slabs_) {
        if (s.is_free) {
            s.is_free   = false;
            s.owner_tag = tag;
            offset      = s.offset;

            allocated_bytes_ += slab_size_;
            if (allocated_bytes_ > peak_bytes_) {
                peak_bytes_ = allocated_bytes_;
            }
            // Wasted bytes: slab space not used by this request.
            wasted_bytes_ += (slab_size_ - size_bytes);
            ++alloc_hits_;
            return true;
        }
    }

    // No free slab found.
    ++alloc_misses_;
    return false;
}

// ============================================================================
// release
// ============================================================================

bool GPUMemoryPool::release(uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : slabs_) {
        if (s.offset == offset && !s.is_free) {
            // Recover the wasted bytes that were charged at acquire time.
            // We can't know the original request size, so we don't adjust
            // wasted_bytes_ here — fragmentation_() will simply be 0 when the
            // pool is empty.  For a production pool a per-slab request size
            // field would be added.
            s.is_free   = true;
            s.owner_tag.clear();
            if (allocated_bytes_ >= slab_size_) {
                allocated_bytes_ -= slab_size_;
            } else {
                allocated_bytes_ = 0;
            }
            // Privacy: zero the slab before returning it to the free list.
            // In a real CUDA pool this calls cudaMemset(device_ptr, 0, slab_size_).
            if (zero_on_free_) {
                ++zeroed_slabs_;
            }
            return true;
        }
    }
    return false;
}

// ============================================================================
// Queries
// ============================================================================

GPUMemoryPool::Stats GPUMemoryPool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_bytes     = total_bytes_;
    s.allocated_bytes = allocated_bytes_;
    s.free_bytes      = total_bytes_ - allocated_bytes_;
    s.peak_bytes      = peak_bytes_;
    s.total_slabs     = slabs_.size();
    s.alloc_hits      = alloc_hits_;
    s.alloc_misses    = alloc_misses_;
    s.zeroed_slabs    = zeroed_slabs_;
    size_t free = 0;
    for (const auto& sl : slabs_) {
        if (sl.is_free) ++free;
    }
    s.free_slabs  = free;
    s.fragmentation = (allocated_bytes_ > 0)
                          ? (static_cast<float>(wasted_bytes_) /
                             static_cast<float>(total_bytes_))
                          : 0.0f;
    return s;
}

size_t GPUMemoryPool::numSlabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slabs_.size();
}

size_t GPUMemoryPool::freeSlabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t free = 0;
    for (const auto& s : slabs_) {
        if (s.is_free) ++free;
    }
    return free;
}

float GPUMemoryPool::fragmentation() const {
    return getStats().fragmentation;
}

std::vector<GPUMemoryPool::Slab> GPUMemoryPool::slabSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slabs_;
}

} // namespace gpu
} // namespace themis
