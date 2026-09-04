/**
 * @file buffer_pool.cpp
 * @brief Phase 3 P3-03-D: Slab-based buffer pool — implementation.
 * @version 1.0.0
 * @note Status: Block B P3-03-D delivery
 */

#include "base/buffer_pool.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace themis::resource {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::size_t BufferPool::slabIndex(std::size_t bytes) noexcept {
    for (std::size_t i = 0; i <static_cast<int>(kSlabSizes.size()); ++i) {
        if (bytes <= kSlabSizes[i]) {
            return i;
        }
    }
    return kNone;  // Too large for any slab.
}

SlabClass BufferPool::indexToClass(std::size_t idx) noexcept {
    switch (idx) {
        case 0: return SlabClass::B128;
        case 1: return SlabClass::B256;
        case 2: return SlabClass::B512;
        case 3: return SlabClass::KB1;
        case 4: return SlabClass::KB2;
        case 5: return SlabClass::KB4;
        default: return SlabClass::B128;  // unreachable
    }
}

// ---------------------------------------------------------------------------
// Pre-allocation
// ---------------------------------------------------------------------------

void BufferPool::preallocateSlab(Slab& s, std::size_t count) {
    s.free_list.reserve(s.free_list.size() + count);
    for (std::size_t i = 0; i < count; ++i) {
        void* p = std::malloc(s.block_size);
        if (!p) {
            break;  // OOM; leave partial pre-allocation in place.
        }
        s.free_list.push_back(p);
    }
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

BufferPool::BufferPool() : BufferPool(Config{}) {}

BufferPool::BufferPool(const Config& config)
    : config_(config) {
    for (std::size_t i = 0; i <static_cast<int>(slabs_.size()); ++i) {
        slabs_[i].block_size = kSlabSizes[i];
        preallocateSlab(slabs_[i], config_.initial_per_class);
    }
}

BufferPool::~BufferPool() {
    shutdown_.store(true, std::memory_order_release);
    for (auto& s : slabs_) {
        std::lock_guard<std::mutex> lk(s.lock);
        for (void* p : s.free_list) {
            std::free(p);
        }
        s.free_list.clear();
    }
}

// ---------------------------------------------------------------------------
// acquire
// ---------------------------------------------------------------------------

BufferHandle BufferPool::acquire(std::size_t bytes) noexcept {
    if (shutdown_.load(std::memory_order_acquire)) {
        return BufferHandle{};  // Invalid handle.
    }

    total_allocs_.fetch_add(1, std::memory_order_relaxed);

    const std::size_t idx = slabIndex(bytes);
    if (idx == kNone) {
        // Oversized: fall back to system allocator.
        os_fallbacks_.fetch_add(1, std::memory_order_relaxed);
        void* p = std::malloc(bytes);
        if (!p) {
            return BufferHandle{};
        }
        live_handles_.fetch_add(1, std::memory_order_relaxed);
        // pool_ = nullptr signals OS-fallback path in BufferHandle::release().
        return BufferHandle{p, bytes, SlabClass::B128, nullptr, true};
    }

    Slab& s = slabs_[idx];
    {
        std::lock_guard<std::mutex> lk(s.lock);
        if (!s.free_list.empty()) {
            void* p = s.free_list.back();
            s.free_list.pop_back();
            ++s.alloc_count;
            live_handles_.fetch_add(1, std::memory_order_relaxed);
            return BufferHandle{p, kSlabSizes[idx], indexToClass(idx), this, true};
        }
        // Free list empty — allocate a new block.
        ++s.miss_count;
    }

    void* p = std::malloc(kSlabSizes[idx]);
    if (!p) {
        return BufferHandle{};
    }
    {
        std::lock_guard<std::mutex> lk(s.lock);
        ++s.alloc_count;
    }
    live_handles_.fetch_add(1, std::memory_order_relaxed);
    return BufferHandle{p, kSlabSizes[idx], indexToClass(idx), this, true};
}

// ---------------------------------------------------------------------------
// release
// ---------------------------------------------------------------------------

void BufferPool::release(void* data, SlabClass slab) noexcept {
    if (!data) {
        return;
    }

    live_handles_.fetch_sub(1, std::memory_order_relaxed);

    // Find slab index.
    const std::size_t sz  = static_cast<std::size_t>(slab);
    const std::size_t idx = slabIndex(sz);
    if (idx == kNone) {
        std::free(data);
        return;
    }

    Slab& s = slabs_[idx];
    std::lock_guard<std::mutex> lk(s.lock);
    if (static_cast<int>(s.free_list.size()) < config_.max_per_class) {
        s.free_list.push_back(data);
    } else {
        // Free list full — discard excess to avoid unbounded growth.
        std::free(data);
    }
}

// ---------------------------------------------------------------------------
// statistics
// ---------------------------------------------------------------------------

BufferPool::Statistics BufferPool::statistics() const noexcept {
    Statistics st;
    st.total_allocations = total_allocs_.load(std::memory_order_relaxed);
    st.os_fallbacks      = os_fallbacks_.load(std::memory_order_relaxed);
    st.current_live      = live_handles_.load(std::memory_order_relaxed);

    for (std::size_t i = 0; i <static_cast<int>(slabs_.size()); ++i) {
        std::lock_guard<std::mutex> lk(slabs_[i].lock);
        st.per_class_allocs[i] = slabs_[i].alloc_count;
        st.slab_hits           += slabs_[i].alloc_count - slabs_[i].miss_count;
        st.slab_misses         += slabs_[i].miss_count;
    }
    return st;
}

// ---------------------------------------------------------------------------
// shutdown
// ---------------------------------------------------------------------------

void BufferPool::shutdown() noexcept {
    shutdown_.store(true, std::memory_order_release);
}

}  // namespace themis::resource
