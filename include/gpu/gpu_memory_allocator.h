/**
 * @file gpu_memory_allocator.h
 * @brief GPU memory allocator with RAII and move semantics
 * @version 0.1.0
 * @note Maturity: 🟡 BETA
 * @note Gap Categories: CWE-415 (double-free), CWE-672 (use-after-free)
 * 
 * Provides:
 * - RAII-based GPU memory pool management
 * - Move constructors with automatic resource transfer
 * - Prevention of double-free via moved-from state tracking
 * - Use-after-move detection with noexcept guarantees
 * 
 * @see ThemisDB Remediation Roadmap: Sprint 8 Phase 1C
 */

#pragma once

#include <memory>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace themis {
namespace gpu {

struct MemoryAllocation {
    void* device_ptr;      ///< GPU device pointer
    void* host_ptr;        ///< CPU-side mirror (pinned memory)
    size_t size;           ///< Allocation size in bytes
    uint32_t device_id;    ///< GPU device ID
    bool is_unified;       ///< true if unified memory (managed by CUDA)
    uint64_t allocation_id;  ///< Unique ID for tracking
};

class GPUMemoryAllocator {
public:
    enum class Strategy {
        CUDAMALLOC,        ///< Direct cudaMalloc (may fragment)
        UNIFIED_MEMORY,    ///< CUDA unified memory (automatic transfers)
        PINNED_HOST,       ///< Pinned host memory for DMA
    };

    struct Config {
        Strategy strategy = Strategy::CUDAMALLOC;
        size_t pool_size = 0;          ///< Pre-allocate pool (0 = no pool)
        bool enable_defrags = true;    ///< Enable memory defragmentation
        uint32_t device_id = 0;        ///< Target GPU device
        bool enable_caching = true;    ///< Cache freed allocations for reuse
        size_t max_alloc_size = 1UL << 30;  ///< Max per-allocation size (default: 1 GB)
    };

    GPUMemoryAllocator() noexcept = default;

    /**
     * @brief GPUMemory Allocator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GPUMemoryAllocator(const Config& config);

    ~GPUMemoryAllocator() noexcept;

    // --- Move semantics (enabled) ---

    GPUMemoryAllocator(GPUMemoryAllocator&& other) noexcept;

    GPUMemoryAllocator& operator=(GPUMemoryAllocator&& other) noexcept;

    // --- Copy semantics (deleted) ---
    GPUMemoryAllocator(const GPUMemoryAllocator&) = delete;
    GPUMemoryAllocator& operator=(const GPUMemoryAllocator&) = delete;

    /**
     * @brief --- Memory operations ---
     * @param[in] size Input parameter.
     * @return Return value.
     */

    MemoryAllocation allocate(size_t size);

    /**
     * @brief Deallocate.
     * @param[in] alloc Input parameter.
     * @note Exception safety: noexcept.
     */
    void deallocate(const MemoryAllocation& alloc) noexcept;

    /**
     * @brief Reallocate.
     * @param[in] alloc Input parameter.
     * @param[in] new_size Input parameter.
     * @return Return value.
     */
    MemoryAllocation reallocate(const MemoryAllocation& alloc, size_t new_size);

    /**
     * @brief Copy to device.
     * @param[in] alloc Input parameter.
     * @param[in] host_data Input parameter.
     * @param[in] size Input parameter.
     */
    void copy_to_device(const MemoryAllocation& alloc, 
                        const void* host_data, size_t size) const;

    /**
     * @brief Copy from device.
     * @param[in,out] host_data Input/output parameter.
     * @param[in] alloc Input parameter.
     * @param[in] size Input parameter.
     */
    void copy_from_device(void* host_data, 
                          const MemoryAllocation& alloc, 
                          size_t size) const;

    /**
     * @brief --- State and diagnostics ---
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool is_moved_from() const noexcept;

    /**
     * @brief Is initialized.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_initialized() const noexcept;

    /**
     * @brief Get config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const Config& get_config() const noexcept;

    /**
     * @brief Available memory.
     * @return Return value.
     */
    size_t available_memory() const;

    /**
     * @brief Allocated memory.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t allocated_memory() const noexcept;

    /**
     * @brief Allocation count.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t allocation_count() const noexcept;

private:
    /**
     * @brief Cleanup.
     * @note Exception safety: noexcept.
     */
    void cleanup() noexcept;

    Config config_;
    std::vector<MemoryAllocation> allocations_;
    uint64_t next_alloc_id_;
    bool is_moved_from_;
};

class DeviceMemoryRegion {
public:
    DeviceMemoryRegion(GPUMemoryAllocator& allocator, size_t size);

    ~DeviceMemoryRegion() noexcept;

    // Move semantics
    DeviceMemoryRegion(DeviceMemoryRegion&& other) noexcept;
    DeviceMemoryRegion& operator=(DeviceMemoryRegion&& other) noexcept;

    // No copy
    DeviceMemoryRegion(const DeviceMemoryRegion&) = delete;
    DeviceMemoryRegion& operator=(const DeviceMemoryRegion&) = delete;

    /**
     * @brief Device ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    void* device_ptr() noexcept;
    /**
     * @brief Device ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    const void* device_ptr() const noexcept;

    /**
     * @brief Host ptr.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    void* host_ptr() noexcept;

    /**
     * @brief Size.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    size_t size() const noexcept;

    /**
     * @brief Is valid.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool is_valid() const noexcept;

private:
    GPUMemoryAllocator* allocator_;
    MemoryAllocation alloc_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
