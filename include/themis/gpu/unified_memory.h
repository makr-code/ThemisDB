/**
 * @file unified_memory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace gpu {

/**
 * @brief Unified memory allocator — CPU+GPU shared address space.
 *
 * Allocates memory that is accessible from both the CPU and all configured
 * CUDA or HIP devices using a single pointer.  On hardware that supports
 * unified addressing the CUDA/HIP runtime transparently migrates pages
 * between CPU DRAM and GPU VRAM as the data is accessed; on CPU-only
 * builds the allocator falls back to ordinary heap (`malloc`/`free`) so
 * that the same call sites compile and run without modification.
 *
 * CUDA path (THEMIS_ENABLE_CUDA):
 *   - `cudaMallocManaged(…, cudaMemAttachGlobal)`
 *   - `cudaMemPrefetchAsync` for page migration hints
 *   - `cudaMemAdvise` for access-pattern hints (preferred location,
 *     accessed-by, read-mostly)
 *   - `cudaFree` for deallocation
 *
 * HIP path (THEMIS_ENABLE_HIP, without CUDA):
 *   - `hipMallocManaged(…, hipMemAttachGlobal)`
 *   - `hipMemPrefetchAsync`
 *   - `hipMemAdvise`
 *   - `hipFree`
 *
 * CPU fallback (neither CUDA nor HIP):
 *   - `malloc` / `free`
 *   - `prefetch()` and `advise()` are no-ops that return `true`
 *   - `isSupported()` returns `false`
 *
 * Tenant isolation
 * ----------------
 * Each allocation is tagged with an optional tenant identifier.  The
 * allocator tracks per-tenant byte usage so that callers can enforce
 * quotas before allocating.
 *
 * Thread safety: all public methods are protected by an internal mutex.
 */
class GPUUnifiedMemoryAllocator {
public:
    // -----------------------------------------------------------------------
    // Memory advice hints (mirrors cudaMemoryAdvise / hipMemoryAdvice)
    // -----------------------------------------------------------------------
    enum class MemAdvice {
        /// Pages should reside in device memory to minimise migration traffic.
        SET_PREFERRED_LOCATION,
        /// The specified processor will access this memory region frequently.
        SET_ACCESSED_BY,
        /// Most accesses will be reads; device may cache read-only copies.
        SET_READ_MOSTLY,
        /// Undo a previous SET_PREFERRED_LOCATION hint.
        UNSET_PREFERRED_LOCATION,
        /// Undo a previous SET_ACCESSED_BY hint.
        UNSET_ACCESSED_BY,
        /// Undo a previous SET_READ_MOSTLY hint.
        UNSET_READ_MOSTLY,
    };

    // -----------------------------------------------------------------------
    // Per-allocation record
    // -----------------------------------------------------------------------
    struct AllocationRecord {
        void*       ptr        = nullptr;
        size_t      bytes      = 0;
        std::string tag;
        std::string tenant_id;
    };

    // -----------------------------------------------------------------------
    // Aggregate statistics
    // -----------------------------------------------------------------------
    struct Stats {
        size_t   total_allocations   = 0;  ///< Successful allocate() calls
        size_t   total_frees         = 0;  ///< Successful free() calls
        uint64_t allocated_bytes     = 0;  ///< Currently live bytes
        uint64_t peak_bytes          = 0;  ///< High-water mark
        size_t   prefetch_calls      = 0;  ///< prefetch() calls issued
        size_t   advise_calls        = 0;  ///< advise() calls issued
        bool     hardware_unified    = false; ///< true when real CUDA/HIP managed memory is used
    };

    // -----------------------------------------------------------------------
    // Singleton
    // -----------------------------------------------------------------------
    static GPUUnifiedMemoryAllocator& GetInstance() {
        static GPUUnifiedMemoryAllocator inst;
        return inst;
    }

    GPUUnifiedMemoryAllocator() = default;
    /**
     * @brief Destructor — frees any allocations that were not explicitly
     * freed by the caller.
     *
     * For the singleton instance this runs at process exit.  On CUDA/HIP
     * builds the runtime is typically still active at that point, but
     * callers that construct local instances must ensure the runtime is
     * still active when the destructor runs (or call reset() explicitly
     * before destruction).
     */
    ~GPUUnifiedMemoryAllocator();

    // Non-copyable.
    GPUUnifiedMemoryAllocator(const GPUUnifiedMemoryAllocator&) = delete;
    GPUUnifiedMemoryAllocator& operator=(const GPUUnifiedMemoryAllocator&) = delete;

    // -----------------------------------------------------------------------
    // Capability query
    // -----------------------------------------------------------------------

    /**
     * @brief Returns true when the runtime supports hardware unified memory.
     *
     * On CUDA builds: queries `cudaDeviceProp::unifiedAddressing` for device 0.
     * On HIP builds:  queries `hipDeviceProp_t::unifiedAddressing` for device 0.
     * CPU-only builds: always returns false.
     *
     * The result is cached after the first call.
     */
    static bool isSupported() noexcept;

    // -----------------------------------------------------------------------
    // Allocation / deallocation
    // -----------------------------------------------------------------------

    /**
     * @brief Allocate @p bytes of unified (CPU+GPU shared) memory.
     *
     * Uses `cudaMallocManaged` when CUDA is available, `hipMallocManaged`
     * when HIP is available, and `malloc` otherwise.
     *
     * @param bytes     Number of bytes to allocate.  Must be > 0.
     * @param tag       Human-readable owner label for diagnostics.
     * @param tenant_id Optional tenant identifier for per-tenant tracking.
     * @return Pointer to the allocated region, or nullptr on failure.
     */
    void* allocate(size_t             bytes,
                   const std::string& tag       = "unknown",
                   const std::string& tenant_id = "");

    /**
     * @brief Free memory previously returned by allocate().
     *
     * Uses `cudaFree` / `hipFree` when unified memory is hardware-backed,
     * `free()` on the CPU fallback path.
     *
     * @param ptr  Pointer returned by allocate(); passing nullptr is a no-op.
     * @return true if the pointer was found in the live-allocation table and
     *         successfully freed, false otherwise.
     */
    bool free(void* ptr);

    // -----------------------------------------------------------------------
    // Migration hints (no-ops on CPU fallback path)
    // -----------------------------------------------------------------------

    /**
     * @brief Prefetch @p bytes starting at @p ptr to @p device_id.
     *
     * On CUDA: calls `cudaMemPrefetchAsync(ptr, bytes, device_id, nullptr)`.
     * On HIP:  calls `hipMemPrefetchAsync(ptr, bytes, device_id, nullptr)`.
     * CPU fallback: no-op, returns true.
     *
     * @param device_id  Target device ordinal; pass -1 / cudaCpuDeviceId to
     *                   migrate pages back to CPU.
     * @return true on success (or when unified memory is not hardware-backed).
     */
    bool prefetch(const void* ptr, size_t bytes, int device_id = 0);

    /**
     * @brief Set a memory-access hint for the range [ptr, ptr+bytes).
     *
     * On CUDA: calls `cudaMemAdvise`.
     * On HIP:  calls `hipMemAdvise`.
     * CPU fallback: no-op, returns true.
     *
     * @param device_id  The device (or -1 for CPU) the hint applies to.
     * @return true on success (or when unified memory is not hardware-backed).
     */
    bool advise(const void* ptr, size_t bytes, MemAdvice advice,
                int device_id = 0);

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /** @brief Return a copy of the aggregate statistics. */
    Stats getStats() const;

    /** @brief Return a snapshot of all currently live allocations. */
    std::vector<AllocationRecord> getActiveAllocations() const;

    /**
     * @brief Return the total bytes currently live for @p tenant_id.
     *
     * Returns 0 if the tenant has no live allocations.
     */
    uint64_t getTenantBytes(const std::string& tenant_id) const;

    /**
     * @brief Reset all internal state (free all tracked allocations).
     *
     * Intended for unit tests; in production code call free() on each
     * individual pointer instead.
     */
    void reset();

private:
    mutable std::mutex mutex_;

    std::vector<AllocationRecord>                      active_;
    std::unordered_map<std::string, uint64_t>          tenant_bytes_;

    size_t   total_allocations_  = 0;
    size_t   total_frees_        = 0;
    uint64_t allocated_bytes_    = 0;
    uint64_t peak_bytes_         = 0;
    size_t   prefetch_calls_     = 0;
    size_t   advise_calls_       = 0;
};

} // namespace gpu
} // namespace themis
