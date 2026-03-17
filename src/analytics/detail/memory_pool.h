/*
 * ThemisDB Analytics – AnalyticsMemoryPool
 *
 * Arena allocator for hot analytics paths (GROUP BY intermediates,
 * AggState maps, scratch buffers).  Designed for per-query lifetime:
 *   1. Call reset() at the start of each query/execute() invocation.
 *   2. Allocate scratch memory via allocate(size, align).
 *   3. No individual free — the entire arena is reclaimed by reset().
 *
 * Derives from std::pmr::memory_resource so it can be used directly as
 * the upstream resource for std::pmr containers (pmr::unordered_map,
 * pmr::string, pmr::vector, …) — no custom allocator boilerplate needed.
 *
 * Thread-safety: NOT thread-safe.  Each OLAPEngine::Impl instance owns
 * its own pool; each thread must use its own pool.
 * Never share a pool across threads.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <new>
#include <vector>

namespace themis {
namespace analytics {
namespace detail {

/**
 * @brief Arena (bump-pointer) allocator for analytics hot paths.
 *
 * Implements std::pmr::memory_resource so that any std::pmr container
 * (unordered_map, string, vector, …) can draw memory directly from this
 * arena rather than the global heap.
 *
 * Keeps one large contiguous primary block (default 64 MiB).  When that
 * block is exhausted, overflow slabs are appended.  reset() rewinds the
 * cursor to 0 and releases overflow slabs — the primary block is retained
 * so steady-state allocations never touch the global heap.
 */
class AnalyticsMemoryPool : public std::pmr::memory_resource {
public:
    static constexpr size_t kDefaultCapacity = 64ULL * 1024 * 1024; // 64 MiB

    explicit AnalyticsMemoryPool(size_t initial_capacity = kDefaultCapacity)
        : capacity_(initial_capacity)
        , cursor_(0)
        , block_(static_cast<uint8_t*>(
              ::operator new(initial_capacity, std::align_val_t{alignof(std::max_align_t)})))
    {}

    ~AnalyticsMemoryPool() noexcept override {
        ::operator delete(block_, std::align_val_t{alignof(std::max_align_t)});
        for (auto& slab : overflow_blocks_) {
            ::operator delete(slab.ptr, std::align_val_t{alignof(std::max_align_t)});
        }
    }

    // Non-copyable, non-movable.
    AnalyticsMemoryPool(const AnalyticsMemoryPool&)            = delete;
    AnalyticsMemoryPool& operator=(const AnalyticsMemoryPool&) = delete;
    AnalyticsMemoryPool(AnalyticsMemoryPool&&)                 = delete;
    AnalyticsMemoryPool& operator=(AnalyticsMemoryPool&&)      = delete;

    /**
     * @brief Allocate @p size bytes aligned to @p align from the arena.
     *
     * @param size   Number of bytes to allocate (must be > 0).
     * @param align  Required alignment (must be a power of two, ≥ 1).
     * @returns Pointer to the allocated region.  Valid until reset() is called.
     * @throws std::bad_alloc if an overflow slab cannot be obtained.
     */
    void* allocate(size_t size, size_t align = alignof(std::max_align_t)) {
        return do_allocate(size, align);
    }

    /**
     * @brief Reset the arena, making all previously allocated memory available
     *        for reuse without releasing the primary backing storage.
     *
     * Call this at the start of each query / execute() invocation.
     * Overflow slabs (if any) are freed to avoid unbounded memory growth.
     */
    void reset() noexcept {
        cursor_ = 0;
        for (auto& slab : overflow_blocks_) {
            ::operator delete(slab.ptr, std::align_val_t{alignof(std::max_align_t)});
        }
        overflow_blocks_.clear();
    }

    /** @returns Number of bytes consumed in the primary block. */
    size_t used() const noexcept { return cursor_; }

    /** @returns Total capacity of the primary block (bytes). */
    size_t capacity() const noexcept { return capacity_; }

protected:
    // --- std::pmr::memory_resource interface ---

    void* do_allocate(size_t size, size_t align) override {
        if (size == 0) return nullptr;
        if (align == 0) align = 1;

        // Try primary block first.
        size_t aligned_cursor = align_up(cursor_, align);
        if (aligned_cursor + size <= capacity_) {
            void* ptr = block_ + aligned_cursor;
            cursor_   = aligned_cursor + size;
            return ptr;
        }

        // Try current overflow slab.
        if (!overflow_blocks_.empty()) {
            auto& slab = overflow_blocks_.back();
            size_t sc  = align_up(slab.cursor, align);
            if (sc + size <= slab.capacity) {
                void* ptr  = slab.ptr + sc;
                slab.cursor = sc + size;
                return ptr;
            }
        }

        // Allocate a new overflow slab sized to fit the request.
        size_t slab_size = std::max(capacity_, size + align);
        OverflowBlock slab;
        slab.ptr      = static_cast<uint8_t*>(
            ::operator new(slab_size, std::align_val_t{alignof(std::max_align_t)}));
        slab.capacity = slab_size;
        slab.cursor   = 0;

        size_t sc   = align_up(slab.cursor, align);
        void*  ptr  = slab.ptr + sc;
        slab.cursor = sc + size;
        overflow_blocks_.push_back(slab);
        return ptr;
    }

    // Arena allocators do not support individual deallocation.
    void do_deallocate(void* /*p*/, size_t /*bytes*/, size_t /*align*/) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

private:
    static size_t align_up(size_t value, size_t align) noexcept {
        return (value + align - 1) & ~(align - 1);
    }

    struct OverflowBlock {
        uint8_t* ptr      = nullptr;
        size_t   capacity = 0;
        size_t   cursor   = 0;
    };

    size_t    capacity_;
    size_t    cursor_;
    uint8_t*  block_;
    std::vector<OverflowBlock> overflow_blocks_;
};

} // namespace detail
} // namespace analytics
} // namespace themis
