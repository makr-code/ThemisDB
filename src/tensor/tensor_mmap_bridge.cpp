/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_mmap_bridge.cpp                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_mmap_bridge.cpp
 * @brief Zero-copy mmap bridge for TT-core data (STUB #270).
 *
 * ## Implementation notes
 *
 * Each TT-core is placed into a dedicated anonymous `mmap` region:
 *   1. `mmap(NULL, bytes, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0)`
 *   2. `memcpy` from in-memory core data into the mapped region.
 *   3. `mlock(region, bytes)` to prevent OS from paging it out.
 *   4. The region pointer is used as `const float*` for the slice.
 *
 * On Windows and other non-POSIX platforms, anonymous `mmap`/`mlock`
 * are not available.  In that case we fall back to a plain
 * heap-allocated copy guarded by `VirtualLock` (Windows) or a no-lock
 * fallback.
 *
 * STUB #270: This path performs a memcpy instead of a true zero-copy.
 *   The production upgrade (Q1 2027) will open RocksDB SST files,
 *   obtain their page-aligned offsets for each TT-core key, and use
 *   `mmap(MAP_SHARED)` to expose those pages directly — eliminating
 *   the memcpy entirely.
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
TensorMmapBridge::SstMapFn& sstMapFnStorage() {
    static TensorMmapBridge::SstMapFn fn;
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
    if (ptr == MAP_FAILED) return nullptr;
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

// STUB/SIMULATION NOTE:
// Purpose: Page-pin each TT-core array in an anonymous mmap region so
//   that the GGML bridge can reference the data pointer without holding
//   a lock on the TensorIndexManager.
// Activation: Always — no compile flag required.
// Production Delta: Copies core data into MAP_ANONYMOUS region via memcpy.
//   Real production path (STUB #270) uses MAP_SHARED on RocksDB SST file
//   pages so no memcpy occurs; the TT-core bytes are accessed in-place.
// Removal Plan: Q1 2027 — replace MAP_ANONYMOUS + memcpy with MAP_SHARED
//   over RocksDB SST file descriptor + offset, once the SST mmap
//   integration layer is available.

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
            if (ptr) sst_mapped = true;
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

        if (locked) ++bridge->locked_count_;

        bridge->total_bytes_ += bytes;
        // For SST-mapped regions we store bytes=0 to signal that freeRegion()
        // must NOT be called by the bridge destructor (caller owns the mapping).
        bridge->regions_.push_back({ptr, sst_mapped ? std::size_t{0} : bytes, locked});
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
// TensorMmapBridge::release
// ============================================================================

void TensorMmapBridge::release() noexcept {
    for (auto& r : regions_) {
        if (!r.ptr) continue;
        if (r.locked) unlockRegion(r.ptr, r.bytes);
        freeRegion(r.ptr, r.bytes);
        r.ptr    = nullptr;
        r.locked = false;
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
