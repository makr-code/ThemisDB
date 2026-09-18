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

/**
 * @brief Size classes supported by the slab allocator.
 *
 * Requests are rounded up to the nearest class; requests > kMaxSlabSize
 * are delegated to the system allocator (malloc/free).
 */
enum class SlabClass : std::size_t {
    B128  =   128,
    B256  =   256,
    B512  =   512,
    KB1   =  1024,
    KB2   =  2048,
    KB4   =  4096,
};

/**
 * @brief Handle returned by @ref BufferPool::acquire().
 *
 * RAII-safe: the destructor calls @ref BufferPool::release() automatically
 * when @p auto_release is @c true (the default).
 */
class BufferHandle {
public:
    /// @brief Constructs a null / invalid handle.
    BufferHandle() = default;

    /**
     * @brief Constructs a handle owning the given raw buffer.
     *
     * @param data    Raw pointer to the allocated region.
     * @param size    Usable byte count of the region.
     * @param slab    Slab class this buffer was drawn from (kNone if OS-alloc).
     * @param pool    Back-pointer to the owning pool, used by destructor.
     * @param auto_release  If true the destructor releases the buffer.
     */
    explicit BufferHandle(void* data, std::size_t size, SlabClass slab,
                          class BufferPool* pool, bool auto_release = true) noexcept
        : data_(data), size_(size), slab_(slab), pool_(pool),
          auto_release_(auto_release) {}

    /// Non-copyable.
    BufferHandle(const BufferHandle&) = delete;
    BufferHandle& operator=(const BufferHandle&) = delete;

    /// Move-constructible.
    BufferHandle(BufferHandle&& o) noexcept
        : data_(o.data_), size_(o.size_), slab_(o.slab_),
          pool_(o.pool_), auto_release_(o.auto_release_) {
        o.data_ = nullptr;
        o.pool_ = nullptr;
    }

    /// Move-assignable.
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

    /// @brief Raw pointer to the buffer region.
    [[nodiscard]] void*       data()  noexcept { return data_; }
    /// @brief Raw pointer (const overload).
    [[nodiscard]] const void* data()  const noexcept { return data_; }
    /// @brief Usable size of the buffer in bytes.
    [[nodiscard]] std::size_t size()  const noexcept { return size_; }
    /// @brief Whether the handle currently owns a valid buffer.
    [[nodiscard]] bool        valid() const noexcept { return data_ != nullptr; }

    /// @brief Byte-span pointer (convenience).
    [[nodiscard]] uint8_t*       bytes() noexcept {
        return static_cast<uint8_t*>(data_);
    }

    /// @brief Explicitly release the buffer back to the pool.
    void release() noexcept;

private:
    void*        data_         = nullptr;
    std::size_t  size_         = 0;
    SlabClass    slab_         = SlabClass::B128;
    BufferPool*  pool_         = nullptr;
    bool         auto_release_ = true;
};

// ----------------------------------------------------------------------------

/**
 * @brief Thread-safe slab-based buffer pool allocator.
 *
 * ### Usage
 * @code
 * BufferPool pool;
 * auto buf = pool.acquire(300);          // draws from B512 slab
 * std::memcpy(buf.data(), src, 300);
 * // buf released automatically on scope exit
 * @endcode
 *
 * ### Thread safety
 * All public methods are thread-safe.  Each slab class has its own
 * @c std::mutex protecting the free list, so different classes can be
 * accessed concurrently.
 *
 * ### Failure handling
 * @ref acquire() returns an invalid handle if the pool has been
 * @ref shutdown() or if system memory is exhausted.
 *
 * @see BufferHandle, SlabClass
 */
class BufferPool {
public:
    /// @brief Statistics snapshot.
    struct Statistics {
        std::size_t total_allocations  = 0; ///< Total acquire() calls (all classes + OS).
        std::size_t slab_hits          = 0; ///< Allocations served from slab free-list.
        std::size_t slab_misses        = 0; ///< OS-level allocations (new slab block or fallback).
        std::size_t os_fallbacks       = 0; ///< Requests too large for any slab.
        std::size_t current_live       = 0; ///< Handles currently outstanding.
        /// Per-class slab allocation counters (indices match kSlabSizes).
        std::array<std::size_t, 6> per_class_allocs = {};
    };

    /**
     * @brief Configuration for the buffer pool.
     */
    struct Config {
        /// Initial free-list depth per slab class.
        std::size_t initial_per_class = 32;
        /// Maximum free-list depth per class (prevents unbounded memory use).
        std::size_t max_per_class     = 256;
    };

    /**
     * @brief Constructs the pool with default configuration.
     */
    BufferPool();

    /**
     * @brief Constructs the pool with custom configuration.
     * @param config  Pool configuration.
     */
    explicit BufferPool(const Config& config);

    /// Non-copyable, non-movable (owns raw memory).
    BufferPool(const BufferPool&)            = delete;
    BufferPool& operator=(const BufferPool&) = delete;

    /// @brief Destructor — releases all slab memory.
    ~BufferPool();

    /**
     * @brief Acquires a buffer of at least @p bytes bytes.
     *
     * The returned handle owns the buffer and will release it on
     * destruction (auto-release is enabled by default).
     *
     * @param  bytes   Minimum usable size required.
     * @return Valid @ref BufferHandle on success; invalid handle on failure
     *         (pool shut down or OOM).
     */
    [[nodiscard]] BufferHandle acquire(std::size_t bytes) noexcept;

    /**
     * @brief Releases a buffer back to the appropriate slab.
     *
     * Called automatically by @ref BufferHandle::~BufferHandle().
     * Safe to call with a null pointer (no-op).
     *
     * @param data  Pointer returned by a prior @ref acquire().
     * @param slab  Slab class the buffer was drawn from.
     */
    void release(void* data, SlabClass slab) noexcept;

    /// @brief Returns a point-in-time statistics snapshot.
    [[nodiscard]] Statistics statistics() const noexcept;

    /// @brief Signals the pool to stop accepting new acquisitions.
    void shutdown() noexcept;

    /// @brief Returns true once @ref shutdown() has been called.
    [[nodiscard]] bool is_shutdown() const noexcept {
        return shutdown_.load(std::memory_order_acquire);
    }

    // -----------------------------------------------------------------------
    // Class-level constants (public so tests can use them)
    // -----------------------------------------------------------------------

    /// Supported slab sizes in bytes.
    static constexpr std::array<std::size_t, 6> kSlabSizes = {
        128, 256, 512, 1024, 2048, 4096
    };

    /// Requests larger than this byte count fall back to the system allocator.
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
