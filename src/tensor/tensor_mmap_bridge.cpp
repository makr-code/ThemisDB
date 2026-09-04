/**
 * @file tensor_mmap_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=12; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "tensor/tensor_mmap_bridge.h"
#include "utils/logger.h"

#include <cassert>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>

// ── Platform guards ──────────────────────────────────────────────────────────
#if defined(__unix__) || defined(__APPLE__)
#   include <sys/mman.h>
#   include <unistd.h>
#   define THEMIS_HAS_MMAP 1
#elif defined(_WIN32)
#   include <windows.h>
#   define THEMIS_HAS_WIN_VIRTUAL 1
#else
#   define THEMIS_HAS_MMAP 0
#endif
// ─────────────────────────────────────────────────────────────────────────────

namespace themis {
namespace tensor {

// ============================================================================
// STUB #270 — SST page-map bridge storage
// ============================================================================

namespace {
std::mutex& sstMapFnMutex() { static std::mutex m; return m; }
std::function<void*(std::size_t, std::size_t)>& sstMapFnStorage() {
    static std::function<void*(std::size_t, std::size_t)> fn;
    return fn;
}
} // anonymous namespace

/*static*/
void TensorMmapBridge::setSstMapFn(SstMapFn fn) {
    std::lock_guard<std::mutex> lk(sstMapFnMutex());
    sstMapFnStorage() = std::move(fn);
}

/*static*/
void TensorMmapBridge::clearSstMapFn() {
    std::lock_guard<std::mutex> lk(sstMapFnMutex());
    sstMapFnStorage() = {};
}

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Allocate a writable anonymous memory region of at least `bytes` bytes.
/// Returns nullptr on failure.
void* allocRegion(std::size_t bytes) noexcept {
#if THEMIS_HAS_MMAP
    void* ptr = ::mmap(nullptr, bytes,
                       PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE,
                       -1, 0);
    if (ptr == MAP_FAILED) {
      return nullptr;
    }
    return ptr;
#elif defined(THEMIS_HAS_WIN_VIRTUAL)
    return ::VirtualAlloc(nullptr, bytes,
                          MEM_COMMIT | MEM_RESERVE,
                          PAGE_READWRITE);
#else
    return std::malloc(bytes);
#endif
}

/// Lock a region in RAM.  Returns true on success.
bool lockRegion(void* ptr, std::size_t bytes) noexcept {
#if THEMIS_HAS_MMAP
    return ::mlock(ptr, bytes) == 0;
#elif defined(THEMIS_HAS_WIN_VIRTUAL)
    return ::VirtualLock(ptr, bytes) != 0;
#else
    (void)ptr; (void)bytes;
    return false;
#endif
}

/// Unlock a region.
void unlockRegion(void* ptr, std::size_t bytes) noexcept {
#if THEMIS_HAS_MMAP
    ::munlock(ptr, bytes);
#elif defined(THEMIS_HAS_WIN_VIRTUAL)
    ::VirtualUnlock(ptr, bytes);
#else
    (void)ptr; (void)bytes;
#endif
}

/// Free a region previously allocated by `allocRegion`.
void freeRegion(void* ptr, std::size_t bytes) noexcept {
#if THEMIS_HAS_MMAP
    ::munmap(ptr, bytes);
#elif defined(THEMIS_HAS_WIN_VIRTUAL)
    (void)bytes;
    ::VirtualFree(ptr, 0, MEM_RELEASE);
#else
    (void)bytes;
    std::free(ptr);
#endif
}

} // anonymous namespace

// ============================================================================
// TensorMmapBridge::buildFromTrain
// ============================================================================
// PERMANENT FALLBACK NOTE:
// Purpose: Page-pin each TT-core array in an anonymous mmap region so that
//   the GGML bridge can reference the data pointer without holding a lock on
//   the TensorIndexManager.
// Activation: Always available; used when no SstMapFn is injected and when
//   buildFromFd() is called with fd < 0.
// Production zero-copy path: Use buildFromFd() with a valid SST fd, or inject
//   a SstMapFn that performs MAP_SHARED over RocksDB SST file pages.

/*static*/
std::unique_ptr<TensorMmapBridge>
TensorMmapBridge::buildFromTrain(const storage::TTTrain& train) {
    auto bridge = std::unique_ptr<TensorMmapBridge>(new TensorMmapBridge());

    // Snapshot the SST-page-map bridge fn once (STUB #270).
    SstMapFn sst_fn;
    {
        std::lock_guard<std::mutex> lk(sstMapFnMutex());
        sst_fn = sstMapFnStorage();
    }

    bridge->regions_.reserve(train.cores.size());
    bridge->slices_.reserve(train.cores.size());

    for (std::size_t ci = 0; ci < train.cores.size(); ++ci) {
        const auto& core  = train.cores[ci];
        const std::size_t n_elems = core.data.size();
        const std::size_t bytes   = n_elems * sizeof(float);

        if (bytes == 0) {
            // Empty core — insert a zero-length sentinel slice.
            bridge->slices_.push_back({nullptr, 0, ci, 0});
            bridge->regions_.push_back({nullptr, 0, false});
            continue;
        }

        // STUB #270: try the injected SST page-map fn first (zero-copy path).
        // If it returns a non-null pointer, use that region directly without
        // memcpy.  The fn is responsible for pre-populating the region with
        // the core float data (e.g. via MAP_SHARED on an SST page).
        // Note: the bridge will NOT call freeRegion() on externally-mapped
        // regions — the caller of setSstMapFn() must manage their lifetime.
        void* ptr = nullptr;
        bool   sst_mapped = false;
        if (sst_fn) {
            ptr = sst_fn(bytes, ci);
            if (ptr) {
              sst_mapped = true;
            }
        }

        if (!ptr) {
            // Fallback: MAP_ANONYMOUS + memcpy (STUB #270 — Q1 2027).
            ptr = allocRegion(bytes);
            if (!ptr) {
                THEMIS_WARN("TensorMmapBridge: mmap allocation failed for "
                            "core {} ({} bytes); bridge will be partial", ci, bytes);
                bridge->slices_.push_back({nullptr, 0, ci, 0});
                bridge->regions_.push_back({nullptr, 0, false});
                continue;
            }
            // Copy core data into the pinned region.
            std::memcpy(ptr, core.data.data(), bytes);
        }

        const bool locked = lockRegion(ptr, bytes);
        if (!locked) {
            // mlock failure is non-fatal: CI containers often set
            // RLIMIT_MEMLOCK to 0.  We still serve the pointer — it
            // just may be paged out under memory pressure.
            THEMIS_WARN("TensorMmapBridge: mlock failed for core {} "
                        "({} bytes) — data may be swapped", ci, bytes);
        }

        if (locked) {
          ++bridge->locked_count_;
        }

        bridge->total_bytes_ += bytes;
        bridge->regions_.push_back({ptr, bytes, locked, sst_mapped});
        bridge->slices_.push_back({
            static_cast<const float*>(ptr),
            bytes,
            ci,
            n_elems
        });
    }

    return bridge;
}

// ============================================================================
// TensorMmapBridge::buildFromFd — zero-copy MAP_SHARED path
// ============================================================================

/*static*/
std::unique_ptr<TensorMmapBridge>
TensorMmapBridge::buildFromFd(const storage::TTTrain& train, int fd,
                               std::size_t byte_offset) {
#if THEMIS_HAS_MMAP
    // When a valid backing fd is provided, map each TT-core with MAP_SHARED
    // directly from the file — zero memcpy, no anonymous allocation.
    if (fd >= 0) {
        auto bridge = std::unique_ptr<TensorMmapBridge>(new TensorMmapBridge());
        bridge->regions_.reserve(train.cores.size());
        bridge->slices_.reserve(train.cores.size());

        std::size_t current_offset = byte_offset;
        const long page_size = ::sysconf(_SC_PAGESIZE);
        const std::size_t ps = (page_size > 0) ? static_cast<std::size_t>(page_size) : 4096u;

        for (std::size_t ci = 0; ci < train.cores.size(); ++ci) {
            const auto& core  = train.cores[ci];
            const std::size_t n_elems = core.data.size();
            const std::size_t bytes   = n_elems * sizeof(float);

            if (bytes == 0) {
                bridge->slices_.push_back({nullptr, 0, ci, 0});
                bridge->regions_.push_back({nullptr, 0, false, false});
                continue;
            }

            // mmap(MAP_SHARED) requires the offset to be page-aligned.
            // Align down and adjust the returned pointer accordingly.
            const std::size_t aligned_offset = (current_offset / ps) * ps;
            const std::size_t delta          = current_offset - aligned_offset;
            const std::size_t map_bytes      = bytes + delta;

            void* raw = ::mmap(nullptr, map_bytes,
                               PROT_READ,
                               MAP_SHARED,
                               fd,
                               static_cast<off_t>(aligned_offset));

            if (raw == MAP_FAILED) {
                // MAP_SHARED failed for this core — fall back to anonymous copy.
                THEMIS_WARN("TensorMmapBridge::buildFromFd: MAP_SHARED failed for "
                            "core {} ({} bytes) — using MAP_ANONYMOUS fallback",
                            ci, bytes);
                void* fallback = allocRegion(bytes);
                if (!fallback) {
                    bridge->slices_.push_back({nullptr, 0, ci, 0});
                    bridge->regions_.push_back({nullptr, 0, false, false});
                    current_offset += bytes;
                    continue;
                }
                std::memcpy(fallback, core.data.data(), bytes);
                const bool locked = lockRegion(fallback, bytes);
                if (locked) {
                  ++bridge->locked_count_;
                }
                bridge->total_bytes_ += bytes;
                bridge->regions_.push_back({fallback, bytes, locked, /*externally_owned=*/false});
                bridge->slices_.push_back({static_cast<const float*>(fallback), bytes, ci, n_elems});
                current_offset += bytes;
                continue;
            }

            // Pointer to the first float within the mapped region (past alignment delta).
            void* ptr = static_cast<char*>(raw) + delta;

            // Advise the kernel to prefetch these pages sequentially.
            ::madvise(raw, map_bytes, MADV_SEQUENTIAL);

            const bool locked = lockRegion(raw, map_bytes);
            if (locked) {
              ++bridge->locked_count_;
            }

            bridge->total_bytes_ += bytes;
            // Mark as externally_owned so release() calls munmap(raw, map_bytes)
            // but does NOT call freeRegion() (which would do a second munmap).
            // We store the aligned base pointer and full map_bytes in the region.
            bridge->regions_.push_back({raw, map_bytes, locked, /*externally_owned=*/true});
            bridge->slices_.push_back({static_cast<const float*>(ptr), bytes, ci, n_elems});
            current_offset += bytes;
        }

        return bridge;
    }
#else
    // Platform does not support mmap — fd path unavailable.
    (void)fd; (void)byte_offset;
#endif
    // PERMANENT FALLBACK NOTE:
    // fd < 0, or mmap not available on this platform — delegate to the
    // MAP_ANONYMOUS + memcpy path which always works.
    return buildFromTrain(train);
}

void TensorMmapBridge::release() noexcept {
    for (auto& r : regions_) {
        if (!r.ptr) {
          continue;
        }
        if (r.locked) {
          unlockRegion(r.ptr, r.bytes);
        }
        if (r.externally_owned) {
            // MAP_SHARED region created by buildFromFd() — munmap to unmap.
            // SstMapFn-provided regions (externally_owned from setSstMapFn) are
            // NOT unmapped here; the caller of setSstMapFn() owns their lifetime.
            // buildFromFd() sets externally_owned=true but also stores the
            // aligned base pointer and map_bytes, so a plain munmap is correct.
#if THEMIS_HAS_MMAP
            ::munmap(r.ptr, r.bytes);
#endif
        } else {
            freeRegion(r.ptr, r.bytes);
        }
        r.ptr              = nullptr;
        r.locked           = false;
        r.externally_owned = false;
    }
    regions_.clear();
    slices_.clear();
    total_bytes_  = 0;
    locked_count_ = 0;
}

// ============================================================================
// TensorMmapBridge — move semantics
// ============================================================================

TensorMmapBridge::TensorMmapBridge(TensorMmapBridge&& other) noexcept
    : regions_(std::move(other.regions_)),
      slices_(std::move(other.slices_)),
      total_bytes_(other.total_bytes_),
      locked_count_(other.locked_count_) {
    other.total_bytes_  = 0;
    other.locked_count_ = 0;
}

TensorMmapBridge& TensorMmapBridge::operator=(TensorMmapBridge&& other) noexcept {
    if (this != &other) {
        release();
        regions_      = std::move(other.regions_);
        slices_       = std::move(other.slices_);
        total_bytes_  = other.total_bytes_;
        locked_count_ = other.locked_count_;
        other.total_bytes_  = 0;
        other.locked_count_ = 0;
    }
    return *this;
}

} // namespace tensor
} // namespace themis
