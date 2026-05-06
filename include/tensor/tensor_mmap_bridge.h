/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/tensor_mmap_bridge.h                        ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/tensor_mmap_bridge.h
 * @brief Zero-copy mmap bridge for TT-core data — Phase 3 (TIM-01).
 *
 * ## Overview (paper §Zero-Copy Inference — Null-Pointer Transfer)
 *
 * `TensorMmapBridge` wraps one or more TT-core arrays in OS-pinned
 * memory regions so that the GGML bridge can expose them to llama.cpp
 * without any further copies.  Each core is placed into a dedicated
 * anonymous `mmap(MAP_PRIVATE)` region and locked via `mlock()`.
 *
 * The bridge has RAII semantics: the destructor calls `munlock()` and
 * `munmap()` on all pinned regions.  Clients should hold the bridge
 * object alive as long as any pointer from `slices()` is in use.
 *
 * ## STUB #176 — MAP_ANONYMOUS vs. MAP_SHARED on SST files
 *
 * The current implementation allocates anonymous (`MAP_ANONYMOUS |
 * MAP_PRIVATE`) regions and `memcpy`s core data from the in-memory
 * store.  The production path (Q1 2027) will use `MAP_SHARED` directly
 * on RocksDB SST file pages so that no CPU copy is required at all.
 *
 * ```
 * // STUB/SIMULATION NOTE:
 * // Purpose: Page-pin TT-core data in RAM for zero-copy GGML injection.
 * // Activation: Always (STUB #176; MAP_SHARED SST path deferred Q1 2027).
 * // Production Delta: memcpy from in-memory store instead of MAP_SHARED;
 * //   mlock() may silently fail when RLIMIT_MEMLOCK is 0 (CI containers).
 * // Removal Plan: replace memcpy path with mmap(MAP_SHARED, sst_fd, offset)
 * //   once RocksDB SST mmap integration is implemented.
 * ```
 *
 * ## Thread Safety
 *
 * Instances are NOT thread-safe.  The owning thread must coordinate
 * access; typically a single inference thread holds the bridge.
 *
 * ## References
 * - ThemisDB Research Group (2026). §Zero-Copy Inference. Internal pre-print.
 * - POSIX `mmap(2)` / `mlock(2)` specifications.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace themis {
namespace tensor {

// ============================================================================
// MmapCoreSlice — a single page-pinned TT-core view
// ============================================================================

/**
 * @brief Lightweight view of one pinned TT-core array.
 *
 * Lifetime is tied to the parent `TensorMmapBridge`.  Do NOT store
 * slices beyond the bridge's lifetime.
 */
struct MmapCoreSlice {
    const float* data      = nullptr; ///< Start of the pinned float data
    std::size_t  bytes     = 0;       ///< Byte length of the region
    std::size_t  core_idx  = 0;       ///< Index of this core in the TTTrain
    std::size_t  num_elems = 0;       ///< Number of floats (bytes / sizeof(float))
};

// ============================================================================
// TensorMmapBridge
// ============================================================================

/**
 * @brief RAII owner of mmap-pinned TT-core pages.
 *
 * Constructed exclusively by `TensorIndexManager::mapCores()`.
 * Clients should hold this object as long as they need the pinned
 * pointers; destruction frees all resources.
 *
 * @code
 * auto bridge = mgr->mapCores(tenant, coll, field, id);
 * if (!bridge) { // vector not found
 *     return {};
 * }
 * for (const auto& s : bridge->slices()) {
 *     // use s.data[0..s.num_elems-1]
 * }
 * // bridge destructor calls munlock+munmap
 * @endcode
 */
class TensorMmapBridge {
public:
    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    /// All pinned core slices, in TT-core order (core 0 … core d-1).
    [[nodiscard]] const std::vector<MmapCoreSlice>& slices() const noexcept {
        return slices_;
    }

    /// Number of TT-cores (equals the order of the represented tensor).
    [[nodiscard]] std::size_t coreCount() const noexcept {
        return slices_.size();
    }

    /// Total bytes across all pinned regions.
    [[nodiscard]] std::size_t totalBytes() const noexcept {
        return total_bytes_;
    }

    /**
     * @brief Whether `mlock()` succeeded for at least one region.
     *
     * Returns false when running in environments where `RLIMIT_MEMLOCK`
     * is 0 (e.g. unprivileged CI containers).  Data pointers remain
     * valid regardless; only swapability is affected.
     */
    [[nodiscard]] bool isLocked() const noexcept { return locked_count_ > 0; }

    /// Number of regions for which `mlock()` succeeded.
    [[nodiscard]] std::size_t lockedRegions() const noexcept {
        return locked_count_;
    }

    // ------------------------------------------------------------------
    // Explicit release (also called by destructor)
    // ------------------------------------------------------------------

    /**
     * @brief Release all pinned regions early.
     *
     * After `release()`, all pointers from `slices()` are invalid.
     * Calling `release()` multiple times is safe.
     */
    void release() noexcept;

    // ------------------------------------------------------------------
    // Lifecycle
    // ------------------------------------------------------------------

    ~TensorMmapBridge() noexcept { release(); }

    TensorMmapBridge(const TensorMmapBridge&)            = delete;
    TensorMmapBridge& operator=(const TensorMmapBridge&) = delete;

    TensorMmapBridge(TensorMmapBridge&&) noexcept;
    TensorMmapBridge& operator=(TensorMmapBridge&&) noexcept;

private:
    // ---- private construction (factory friend) ----

    friend class TensorIndexManager;

    TensorMmapBridge() = default;

    /**
     * @brief Build a bridge from a TTTrain.
     *
     * // STUB/SIMULATION NOTE:
     * // Purpose: Page-pin each TT-core in an anonymous mmap region.
     * // Activation: Always (STUB #176).
     * // Production Delta: memcpy from in-memory data instead of MAP_SHARED.
     * // Removal Plan: Q1 2027 — map SST file pages directly.
     */
    [[nodiscard]] static std::unique_ptr<TensorMmapBridge>
    buildFromTrain(const storage::TTTrain& train);

    // ---- internal region tracking ----

    struct Region {
        void*       ptr    = nullptr;
        std::size_t bytes  = 0;
        bool        locked = false;
    };

    std::vector<Region>        regions_;
    std::vector<MmapCoreSlice> slices_;
    std::size_t                total_bytes_   = 0;
    std::size_t                locked_count_  = 0;
};

} // namespace tensor
} // namespace themis
