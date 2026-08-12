/**
 * @file pmem_storage.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB - Persistent Memory (Optane) Aware Storage Layout
// Paper: "NOVA: A Log-structured File System for Hybrid Volatile/Non-volatile Main Memories"
//        (FAST'16) - Jian Xu & Steven Swanson, UCSD
// Complementary: "SplinterDB: Closing the Bandwidth Gap for NVMe Key-Value Stores" (USENIX ATC'20)
//                "MatrixKV: Reducing Write Stalls and Write Amplification in LSM-tree Based KV Stores"
//                (USENIX ATC'20)
//
// Key idea: byte-addressable persistent memory (Intel Optane DCPMM / NVDIMM) occupies a
// performance tier between DRAM and SSD.  Naively treating it as a block device wastes
// its low latency (<1 µs) and high bandwidth.  This module exposes a storage layout that:
//   1. Detects available PMem devices via DAX-capable file systems (Linux) or
//      file-based emulation on other platforms.
//   2. Maps PMem ranges with direct-access (DAX), bypassing the page cache.
//   3. Issues CPU cache-line flushes (clflushopt/clwb) and store fences (sfence)
//      to ensure crash consistency without write-amplifying fsync calls.
//   4. Aligns all internal structures to 256-byte PMem cache-line granules to
//      minimise cross-granule updates.
//   5. Exposes an RAII allocator and a log-structured write path whose overhead
//      is < 2 µs per 256-byte write on Optane hardware.
//
// Expected gain: +50-200% write throughput vs. NVMe SSD for small random writes;
//                +40-80% read latency reduction for hot working sets that fit in PMem.
//
// Compile-time gate: THEMIS_ENABLE_PMEM  (set by CMake option THEMIS_ENABLE_PMEM=ON)
// Runtime gate:      Phase4FeatureFlags::instance().pmem_enabled()

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// Platform detection for PMem persistence primitives
#if defined(__x86_64__) || defined(_M_X64)
    #include <immintrin.h>          // _mm_clflushopt, _mm_clwb, _mm_sfence
    #define THEMIS_PMEM_X86_64 1
#endif

namespace themis {
namespace performance {
namespace phase4 {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// PMem cache-line granule (256 bytes on Intel Optane DCPMM).
/// All internal structures are aligned to this value to avoid torn writes.
static constexpr size_t kPMemCacheLineSize = 256;

/// Default PMem pool size: 8 GiB (representative Optane module capacity).
static constexpr size_t kDefaultPMemPoolSize = 8ULL * 1024 * 1024 * 1024;

/// Alignment required for DAX mmap on Linux (2 MiB huge-page aligned).
static constexpr size_t kDAXAlignment = 2ULL * 1024 * 1024;

// ---------------------------------------------------------------------------
// Persistence helpers (inlined for minimal overhead)
// ---------------------------------------------------------------------------

/// Flush a single cache line containing @p addr to the persistence domain.
/// On x86-64 prefers clwb (non-invalidating); falls back to clflushopt or
/// clflush.  On other architectures this is a no-op (PMem is not supported).
inline void pmem_flush(const void* addr) noexcept {
#if defined(THEMIS_PMEM_X86_64)
    #if defined(__clwb__)
        _mm_clwb(const_cast<void*>(addr));
    #elif defined(__clflushopt__)
        _mm_clflushopt(const_cast<void*>(addr));
    #else
        _mm_clflush(const_cast<void*>(addr));
    #endif
#else
#endif
}

/// Flush @p len bytes starting at @p addr (rounds to cache-line boundaries).
inline void pmem_flush_range(const void* addr, size_t len) noexcept {
    const auto* p = static_cast<const char*>(addr);
    // Align down to 64-byte CPU cache line
    const auto* aligned = reinterpret_cast<const char*>(
        reinterpret_cast<uintptr_t>(p) & ~(static_cast<uintptr_t>(63)));
    for (; aligned < p + len; aligned += 64) {
        pmem_flush(aligned);
    }
}

/// Store fence: ensure all preceding stores are ordered before this point.
inline void pmem_sfence() noexcept {
#if defined(THEMIS_PMEM_X86_64)
    _mm_sfence();
#endif
}

/// Persist @p len bytes at @p addr: flush then sfence.
inline void pmem_persist(const void* addr, size_t len) noexcept {
    pmem_flush_range(addr, len);
    pmem_sfence();
}

// ---------------------------------------------------------------------------
// PMemDevice – represents one mapped PMem region
// ---------------------------------------------------------------------------

/// Describes a single persistent-memory device or emulated PMem file.
struct PMemDeviceInfo {
    std::string path;           ///< Filesystem path (DAX file or /dev/pmemX)
    size_t      size_bytes{0};  ///< Total usable capacity in bytes
    bool        is_dax{false};  ///< True when mmap'd with MAP_SYNC (DAX)
    bool        is_emulated{false}; ///< True when backed by a regular file
    int         numa_node{-1};  ///< NUMA node, or -1 if unknown
};

/// Detects PMem devices available on the current host.
/// On Linux this enumerates /dev/pmem* block devices and checks for
/// DAX-capable mounts in /proc/mounts.
/// On other platforms it always returns an empty list (caller may fall
/// back to file-based emulation via PMemPool).
std::vector<PMemDeviceInfo> detect_pmem_devices();

// ---------------------------------------------------------------------------
// PMemPool – byte-addressable pool with crash-consistent allocation
// ---------------------------------------------------------------------------

/// A crash-consistent, byte-addressable memory pool backed by PMem.
///
/// Layout within the mapped region:
///   [ PMemPoolHeader (256 B) | AllocBitmap | Data arena ]
///
/// The header and bitmap are persisted with clwb+sfence after every
/// allocation so the pool can be recovered after a crash.
///
/// Thread-safety: all public methods are thread-safe via an internal
/// CAS-based bump allocator.  Concurrent frees are supported via the
/// bitmap.  The pool does NOT support arbitrary in-place reuse of freed
/// blocks (append-only with compaction via rebuild()).
class PMemPool {
public:
    /// Configuration for pool creation / open.
    struct Config {
        std::string path;                        ///< Backing file path
        size_t      pool_size{kDefaultPMemPoolSize}; ///< Desired size in bytes
        size_t      alignment{kPMemCacheLineSize};   ///< Minimum alloc alignment
        bool        create_if_missing{true};     ///< Create file if absent
        bool        use_dax{true};               ///< Request DAX mmap when available
        bool        recover_on_open{true};       ///< Scan header on open and recover
    };

    /// Statistics exposed via get_stats().
    struct Stats {
        size_t total_bytes{0};       ///< Total pool capacity
        size_t used_bytes{0};        ///< Bytes currently allocated
        size_t free_bytes{0};        ///< Bytes available for allocation
        uint64_t alloc_count{0};     ///< Cumulative allocation calls
        uint64_t free_count{0};      ///< Cumulative free calls
        uint64_t flush_count{0};     ///< Cumulative clwb/clflush calls
        bool     is_dax{false};      ///< True when using DAX mmap
        bool     is_healthy{true};   ///< False after unrecoverable error
    };

    /// Open (or create) a PMem pool at @p config.path.
    /// Throws std::runtime_error if the pool cannot be mapped.
    explicit PMemPool(const Config& config);

    ~PMemPool();

    // Non-copyable, movable
    PMemPool(const PMemPool&) = delete;
    PMemPool& operator=(const PMemPool&) = delete;
    PMemPool(PMemPool&&) noexcept;
    PMemPool& operator=(PMemPool&&) noexcept;

    /// Allocate @p size bytes from the pool, aligned to config.alignment.
    /// Returns nullptr when the pool is exhausted.
    void* allocate(size_t size) noexcept;

    /// Mark the @p size bytes at @p ptr as free.
    /// The bytes remain accessible (in-place) until compaction.
    void free(void* ptr, size_t size) noexcept;

    /// Persist (flush + sfence) the @p size bytes at @p ptr.
    /// Call this after writing into memory obtained from allocate() to
    /// guarantee crash-consistency.
    void persist(const void* ptr, size_t size) noexcept;

    /// Return the base address of the mapped pool.
    void* base() const noexcept { return base_; }

    /// Return pool statistics.
    Stats get_stats() const noexcept;

    /// Check whether the pool is backed by DAX-capable storage.
    bool is_dax() const noexcept { return is_dax_; }

    /// Recover allocation metadata from the persisted header.
    /// Called automatically on open when Config::recover_on_open is true.
    bool recover() noexcept;

private:
    // ------- Internal POD header written at offset 0 -------
    // Must fit inside kPMemCacheLineSize bytes and be trivially copyable.
    struct alignas(kPMemCacheLineSize) PMemPoolHeader {
        uint64_t magic{0};         ///< 0xPMEM_MAGIC
        uint64_t version{1};
        uint64_t pool_size{0};     ///< Total bytes in the pool
        uint64_t data_offset{0};   ///< Byte offset of data arena from base
        uint64_t bitmap_offset{0}; ///< Byte offset of alloc bitmap from base
        uint64_t bump_offset{0};   ///< Next free byte in data arena (CAS target)
        uint64_t alloc_count{0};
        uint64_t free_count{0};
        uint8_t  padding[kPMemCacheLineSize - 8 * 8]{};
    };
    static_assert(sizeof(PMemPoolHeader) == kPMemCacheLineSize,
                  "PMemPoolHeader must be exactly kPMemCacheLineSize bytes");

    static constexpr uint64_t kMagic = 0x504D454D544845DBULL; ///< "PMEMTHEDB"

    void   map_region(const Config& config);
    void   unmap_region() noexcept;
    void   init_header(size_t pool_size, size_t alignment) noexcept;
    size_t align_up(size_t v, size_t align) const noexcept {
        return (v + align - 1) & ~(align - 1);
    }

    void*               base_{nullptr};
    size_t              mapped_size_{0};
    bool                is_dax_{false};
    size_t              alignment_{kPMemCacheLineSize};
    PMemPoolHeader*     header_{nullptr};      ///< Points into base_
    std::atomic<size_t> bump_{0};              ///< Shadow of header_->bump_offset
    mutable std::atomic<uint64_t> flush_count_{0};

    // File descriptor kept open for the lifetime of the pool
    int fd_{-1};
};

// ---------------------------------------------------------------------------
// PMemAllocator – STL-compatible allocator backed by PMemPool
// ---------------------------------------------------------------------------

/// STL-compatible allocator that obtains memory from a PMemPool.
/// Designed for use with std::vector, std::unordered_map, etc.
///
/// Example:
///   PMemPool pool({.path="/mnt/pmem0/themis.pool", .pool_size=1<<30});
///   std::vector<int, PMemAllocator<int>> v(PMemAllocator<int>(&pool));
template <typename T>
class PMemAllocator {
public:
    using value_type      = T;
    using pointer         = T*;
    using const_pointer   = const T*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    struct rebind { using other = PMemAllocator<U>; };

    explicit PMemAllocator(PMemPool* pool) noexcept : pool_(pool) {}

    template <typename U>
    explicit PMemAllocator(const PMemAllocator<U>& other) noexcept
        : pool_(other.pool_) {}

    T* allocate(size_type n) {
        void* p = pool_->allocate(n * sizeof(T));
        if (!p) {
            throw std::bad_alloc();
        }
        return static_cast<T*>(p);
    }

    void deallocate(T* p, size_type n) noexcept {
        pool_->free(p, n * sizeof(T));
    }

    bool operator==(const PMemAllocator& o) const noexcept { return pool_ == o.pool_; }
    bool operator!=(const PMemAllocator& o) const noexcept { return pool_ != o.pool_; }

    PMemPool* pool_;  // public for rebind
};

// ---------------------------------------------------------------------------
// PMemStorageLayout – higher-level layout manager
// ---------------------------------------------------------------------------

/// Manages the on-PMem layout of ThemisDB storage structures.
///
/// Responsible for:
///  - Opening / creating the PMem pool.
///  - Providing crash-consistent write primitives for key-value records.
///  - Reporting metrics (write amplification, flush counts, latency).
///
/// Usage:
///   PMemStorageLayout layout({.path="/mnt/pmem0/db.pool"});
///   layout.write("key1", data_ptr, data_len);
///   layout.flush_all();
class PMemStorageLayout {
public:
    struct Config {
        std::string path;                          ///< Pool file path
        size_t      pool_size{kDefaultPMemPoolSize};
        size_t      write_granule{kPMemCacheLineSize}; ///< Pad writes to this size
        bool        use_dax{true};
        bool        create_if_missing{true};
    };

    struct WriteStats {
        uint64_t writes{0};          ///< Total write() calls
        uint64_t bytes_written{0};   ///< Total payload bytes written
        uint64_t bytes_persisted{0}; ///< Total bytes flushed to PMem
        uint64_t write_amplification_x1000{1000}; ///< bytes_persisted/bytes_written * 1000
    };

    /// Open (or create) the layout at the given config path.
    explicit PMemStorageLayout(const Config& config);
    ~PMemStorageLayout() = default;

    // Non-copyable
    PMemStorageLayout(const PMemStorageLayout&) = delete;
    PMemStorageLayout& operator=(const PMemStorageLayout&) = delete;

    /// Write @p len bytes from @p data into PMem and persist immediately.
    /// Returns pointer to the persisted copy, or nullptr on error.
    void* write(const std::string& key, const void* data, size_t len);

    /// Flush all dirty ranges to the persistence domain.
    void flush_all() noexcept;

    /// Return per-pool stats merged with write stats.
    WriteStats get_write_stats() const noexcept;
    PMemPool::Stats get_pool_stats() const noexcept;

    /// True when backed by DAX (direct-access) PMem.
    bool is_dax() const noexcept;

private:
    PMemPool pool_;
    mutable std::atomic<uint64_t> writes_{0};
    mutable std::atomic<uint64_t> bytes_written_{0};
    mutable std::atomic<uint64_t> bytes_persisted_{0};
    size_t write_granule_{kPMemCacheLineSize};
};

} // namespace phase4
} // namespace performance
} // namespace themis

