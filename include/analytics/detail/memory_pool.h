/**
 * @file memory_pool.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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

#include <algorithm>        // std::max
#include <cstddef>
#include <cstdint>          // uintptr_t
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

    /** @returns Number of bytes currently consumed across primary and overflow blocks. */
    size_t used() const noexcept {
        size_t total = cursor_;
        for (const auto& slab : overflow_blocks_) {
            total += slab.cursor;
        }
        return total;
    }

    /** @returns Total capacity of the primary block (bytes). */
    size_t capacity() const noexcept { return capacity_; }

protected:
    // --- std::pmr::memory_resource interface ---

    void* do_allocate(size_t size, size_t align) override {
        if (size == 0) {
          return nullptr;
        }
        if (align == 0) {
          align = 1;
        }

        // Try primary block first.
        // Alignment is computed relative to the absolute address of block_
        // so that the returned pointer satisfies any power-of-two alignment,
        // even those larger than alignof(std::max_align_t).
        if (void* p = try_alloc(block_, cursor_, capacity_, size, align)) {
            return p;
        }

        // Try current overflow slab.
        if (!overflow_blocks_.empty()) {
            auto& slab = overflow_blocks_.back();
            if (void* p = try_alloc(slab.ptr, slab.cursor, slab.capacity, size, align)) {
                return p;
            }
        }

        // Allocate a new overflow slab sized to fit the request.
        // size + align gives the worst-case padding needed to honour alignment.
        size_t slab_size = std::max(capacity_, size + align);
        OverflowBlock slab;
        slab.ptr      = static_cast<uint8_t*>(
            ::operator new(slab_size, std::align_val_t{alignof(std::max_align_t)}));
        slab.capacity = slab_size;
        slab.cursor   = 0;
        overflow_blocks_.push_back(slab);

        return try_alloc(overflow_blocks_.back().ptr,
                         overflow_blocks_.back().cursor,
                         overflow_blocks_.back().capacity,
                         size, align);
    }

    // Arena allocators do not support individual deallocation.
    void do_deallocate(void* /*p*/, size_t /*bytes*/, size_t /*align*/) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

private:
    // Align a value up to the next multiple of align.
    static size_t align_up(size_t value, size_t align) noexcept {
        return (value + align - 1) & ~(align - 1);
    }

    /**
     * @brief Try to allocate @p size bytes with @p align from the block
     *        described by (@p base, @p cursor, @p cap).
     *
     * Alignment is computed on the *absolute* pointer address so that the
     * returned pointer satisfies the alignment contract for any power-of-two
     * alignment, regardless of how @p base itself was aligned.
     *
     * @returns Pointer on success, nullptr if the block is too small.
     */
    static void* try_alloc(uint8_t* base, size_t& cursor,
                            size_t cap, size_t size, size_t align) noexcept {
        // Compute alignment on the *absolute* address (base + cursor).
        // Using the absolute address — rather than just the cursor offset —
        // ensures the returned pointer satisfies the alignment contract for any
        // power-of-two alignment, even those larger than alignof(std::max_align_t)
        // (i.e., even if base is only 16-byte aligned, a 128-byte alignment
        // request is correctly satisfied by padding the absolute address).
        uintptr_t abs     = reinterpret_cast<uintptr_t>(base) + cursor;
        uintptr_t aligned = (abs + static_cast<uintptr_t>(align) - 1)
                          & ~(static_cast<uintptr_t>(align) - 1);
        size_t offset = static_cast<size_t>(aligned - reinterpret_cast<uintptr_t>(base));
        if (offset + size <= cap) {
            cursor = offset + size;
            return reinterpret_cast<void*>(aligned);
        }
        return nullptr;
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

namespace themisdb {
namespace analytics {
namespace detail {
using ::themis::analytics::detail::AnalyticsMemoryPool;
} // namespace detail
} // namespace analytics
} // namespace themisdb
