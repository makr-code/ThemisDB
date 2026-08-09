/**
 * @file tensor_mmap_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=13; TODO=1, Stub=9, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "storage/tensor_train_decomposer.h"

#include <cstddef>
#include <functional>
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
     * @brief Build a bridge from a TTTrain using MAP_ANONYMOUS + memcpy.
     *
     * Page-pins each TT-core array in an anonymous mmap region so that the
     * GGML bridge can reference data pointers without holding the index lock.
     * When an SST-map function is injected via setSstMapFn(), the zero-copy
     * MAP_SHARED path is used for any core for which the function returns a
     * non-null pointer.
     *
     * PERMANENT FALLBACK NOTE:
     * This is the always-available fallback path.  For zero-copy MAP_SHARED
     * over real SST file pages, inject a SstMapFn via setSstMapFn() or use
     * buildFromFd() when a backing file descriptor is available.
     */
    [[nodiscard]] static std::unique_ptr<TensorMmapBridge>
    buildFromTrain(const storage::TTTrain& train);

    /**
     * @brief Build a bridge using MAP_SHARED on a backing file descriptor.
     *
     * When @p fd is a valid open file descriptor (≥ 0) and the platform
     * supports mmap(MAP_SHARED), each TT-core region is mapped directly from
     * the file at the given @p byte_offset without any memcpy.  This is the
     * production zero-copy path for SST-resident TT-trains.
     *
     * Ownership: the caller retains ownership of @p fd; the bridge does not
     * close it.  Regions mapped with MAP_SHARED are tracked as
     * `externally_owned = true` (the destructor calls munmap but NOT close).
     *
     * PERMANENT FALLBACK NOTE:
     * When @p fd < 0, or when the platform does not support mmap(MAP_SHARED),
     * the call transparently falls back to buildFromTrain(train) (MAP_ANONYMOUS
     * + memcpy).
     *
     * @param train        Decomposed TT-train whose float data populates the map.
     * @param fd           Open read-only file descriptor for the backing SST file.
     *                     Pass -1 to always use the MAP_ANONYMOUS fallback.
     * @param byte_offset  Byte offset inside @p fd where the TT data starts.
     */
    [[nodiscard]] static std::unique_ptr<TensorMmapBridge>
    buildFromFd(const storage::TTTrain& train, int fd, std::size_t byte_offset = 0);

public:
    // ─── Bridge injection API (STUB #270) ────────────────────────────────────

    /**
     * @brief Injectable SST page-map function (STUB #270).
     *
     * Signature: `void* fn(std::size_t bytes, std::size_t core_idx)`.
     *
     * When set, `buildFromTrain()` calls this function for each TT-core
     * before falling back to `MAP_ANONYMOUS + memcpy`.  The function should
     * return a pointer to a readable memory region of exactly `bytes` bytes
     * that is pre-populated with the core's float data (e.g. via MAP_SHARED
     * on a RocksDB SST file page).  Returning `nullptr` triggers the fallback
     * for that core only.
     *
     * The bridge does NOT call `munmap` / `VirtualFree` on regions returned
     * by this function (ownership remains with the caller of the fn).
     * To signal that a region should be freed by the bridge destructor via
     * the normal `freeRegion()` path, return the value from `allocRegion()`.
     * For zero-copy MAP_SHARED regions the caller must unmap them separately.
     *
     * Thread-safety: set/clear are mutex-guarded; `buildFromTrain()` acquires
     * the lock once per call to snapshot the fn.
     */
    using SstMapFn = std::function<void*(std::size_t bytes, std::size_t core_idx)>;

    /** @brief Inject the SST page-map backend (STUB #270). Thread-safe. */
    static void setSstMapFn(SstMapFn fn);
    /** @brief Clear the SST page-map backend. Falls back to MAP_ANONYMOUS+memcpy. */
    static void clearSstMapFn();

private:
    // ---- internal region tracking ----

    struct Region {
        void*       ptr             = nullptr;
        std::size_t bytes           = 0;
        bool        locked          = false;
        /// When true, the region was provided by SstMapFn (caller owns the mapping);
        /// the bridge destructor must NOT call freeRegion() on this pointer.
        bool        externally_owned = false;
    };

    std::vector<Region>        regions_;
    std::vector<MmapCoreSlice> slices_;
    std::size_t                total_bytes_   = 0;
    std::size_t                locked_count_  = 0;
};

} // namespace tensor
} // namespace themis
