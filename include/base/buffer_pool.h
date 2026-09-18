/**
 * @file buffer_pool.h
 * @brief Phase 3 P3-03-D: Slab-based buffer pool allocator for ThemisDB.
 *
 * Provides a thread-safe, fixed-size slab allocator over six size classes
 * (128 B, 256 B, 512 B, 1 KB, 2 KB, 4 KB).  Allocations from an appropriate
 * slab class are constant-time and do not call into the OS allocator on the
 * hot path; oversized requests fall back to the system allocator.
 *
 * Design goals:
 *  - >90 % reuse rate (allocated from slab, not from OS)
 *  - Fragmentation-free fixed-size allocation within each class
 *  - Thread-safe acquire / release via per-class spin-lock + free-list
 *  - Statistics: total allocated, total reused, per-class counts
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Block B P3-03-D delivery
 */

#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace themis::resource {

enum class SlabClass : std::size_t {
    B128  =   128,
    B256  =   256,
    B512  =   512,
    KB1   =  1024,
    KB2   =  2048,
    KB4   =  4096,
};

class BufferHandle {
public:
    BufferHandle() = default;

    explicit BufferHandle(void* data, std::size_t size, SlabClass slab,
                          class BufferPool* pool, bool auto_release = true) noexcept
        : data_(data), size_(size), slab_(slab), pool_(pool),
          auto_release_(auto_release) {}

    BufferHandle(const BufferHandle&) = delete;
    BufferHandle& operator=(const BufferHandle&) = delete;

    BufferHandle(BufferHandle&& o) noexcept
        : data_(o.data_), size_(o.size_), slab_(o.slab_),
          pool_(o.pool_), auto_release_(o.auto_release_) {
        o.data_ = nullptr;
        o.pool_ = nullptr;
    }

    BufferHandle& operator=(BufferHandle&& o) noexcept {
        if (this != &o) {
            release();
            data_         = o.data_;
            size_         = o.size_;
            slab_         = o.slab_;
            pool_         = o.pool_;
            auto_release_ = o.auto_release_;
            o.data_       = nullptr;
            o.pool_       = nullptr;
        }
        return *this;
    }

    ~BufferHandle();  // defined after BufferPool

    [[nodiscard]] void*       data()  noexcept { return data_; }
    [[nodiscard]] const void* data()  const noexcept { return data_; }
    [[nodiscard]] std::size_t size()  const noexcept { return size_; }
    [[nodiscard]] bool        valid() const noexcept { return data_ != nullptr; }

    [[nodiscard]] uint8_t*       bytes() noexcept {
        return static_cast<uint8_t*>(data_);
    }

    /**
     * @brief Release.
     * @note Exception safety: noexcept.
     */
    void release() noexcept;

private:
    void*        data_         = nullptr;
    std::size_t  size_         = 0;
    SlabClass    slab_         = SlabClass::B128;
    BufferPool*  pool_         = nullptr;
    bool         auto_release_ = true;
};

// ----------------------------------------------------------------------------

class BufferPool {
public:
    struct Statistics {
        std::size_t total_allocations  = 0; ///< Total acquire() calls (all classes + OS).
        std::size_t slab_hits          = 0; ///< Allocations served from slab free-list.
        std::size_t slab_misses        = 0; ///< OS-level allocations (new slab block or fallback).
        std::size_t os_fallbacks       = 0; ///< Requests too large for any slab.
        std::size_t current_live       = 0; ///< Handles currently outstanding.
        std::array<std::size_t, 6> per_class_allocs = {};
    };

    struct Config {
        std::size_t initial_per_class = 32;
        std::size_t max_per_class     = 256;
    };

    BufferPool();

    /**
     * @brief Buffer Pool.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit BufferPool(const Config& config);

    BufferPool(const BufferPool&)            = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    ~BufferPool();

    [[nodiscard]] BufferHandle acquire(std::size_t bytes) noexcept;

    /**
     * @brief Release.
     * @param[in,out] data Input/output parameter.
     * @param[in] slab Input parameter.
     * @note Exception safety: noexcept.
     */
    void release(void* data, SlabClass slab) noexcept;

    [[nodiscard]] Statistics statistics() const noexcept;

    /**
     * @brief Shutdown.
     * @note Exception safety: noexcept.
     */
    void shutdown() noexcept;

    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    // -----------------------------------------------------------------------
    // Class-level constants (public so tests can use them)
    // -----------------------------------------------------------------------

    static constexpr std::array<std::size_t, 6> kSlabSizes = {
        128, 256, 512, 1024, 2048, 4096
    };

    static constexpr std::size_t kMaxSlabSize = 4096;

private:
    // Per-slab-class state.
    struct Slab {
        std::mutex            lock = {};
        std::vector<void*>    free_list;
        std::size_t           block_size   = 0;
        std::size_t           alloc_count  = 0;  ///< Total served from this slab.
        std::size_t           miss_count   = 0;  ///< Times a new block was allocated.
    };

    // Returns the slab index for a given request size, or kNone.
    static constexpr std::size_t kNone = 6;
    [[nodiscard]] static std::size_t slabIndex(std::size_t bytes) noexcept;
    [[nodiscard]] static SlabClass indexToClass(std::size_t idx) noexcept;

    /**
     * @brief Preallocate Slab.
     * @param[in,out] s Input/output parameter.
     * @param[in] count Input parameter.
     */
    void preallocateSlab(Slab& s, std::size_t count);

    mutable std::array<Slab, 6>     slabs_;
    Config                  config_;
    std::atomic<bool>       shutdown_{false};

    // Global counters (updated under slab locks or atomically).
    std::atomic<std::size_t> total_allocs_{0};
    std::atomic<std::size_t> os_fallbacks_{0};
    std::atomic<std::size_t> live_handles_{0};
};

// ---------------------------------------------------------------------------
// BufferHandle inline definitions (after BufferPool is fully declared)
// ---------------------------------------------------------------------------

inline BufferHandle::~BufferHandle() {
    if (auto_release_) {
        release();
    }
}

inline void BufferHandle::release() noexcept {
    if (data_ && pool_) {
        pool_->release(data_, slab_);
        data_ = nullptr;
        pool_ = nullptr;
    } else if (data_ && !pool_) {
        // OS-fallback path: the pool pointer is null for oversized allocations.
        std::free(data_);
        data_ = nullptr;
    }
}

}  // namespace themis::resource
