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

/**
 * @brief GPU memory allocation descriptor
 */
struct MemoryAllocation {
    void* device_ptr;      ///< GPU device pointer
    void* host_ptr;        ///< CPU-side mirror (pinned memory)
    size_t size;           ///< Allocation size in bytes
    uint32_t device_id;    ///< GPU device ID
    bool is_unified;       ///< true if unified memory (managed by CUDA)
    uint64_t allocation_id;  ///< Unique ID for tracking
};

/**
 * @brief GPU memory allocator with move semantics and double-free prevention
 * 
 * Manages GPU VRAM with:
 * - Automatic allocation/deallocation via RAII
 * - Move-only semantics to prevent aliasing bugs
 * - Double-free prevention via moved-from state
 * - Pinned memory for host-device transfers
 */
class GPUMemoryAllocator {
public:
    /**
     * @brief Memory allocation strategy
     */
    enum class Strategy {
        CUDAMALLOC,        ///< Direct cudaMalloc (may fragment)
        UNIFIED_MEMORY,    ///< CUDA unified memory (automatic transfers)
        PINNED_HOST,       ///< Pinned host memory for DMA
    };

    /**
     * @brief Allocator configuration
     */
    struct Config {
        Strategy strategy = Strategy::CUDAMALLOC;
        size_t pool_size = 0;          ///< Pre-allocate pool (0 = no pool)
        bool enable_defrags = true;    ///< Enable memory defragmentation
        uint32_t device_id = 0;        ///< Target GPU device
        bool enable_caching = true;    ///< Cache freed allocations for reuse
        size_t max_alloc_size = 1UL << 30;  ///< Max per-allocation size (default: 1 GB)
    };

    /**
     * @brief Default constructor - creates uninitialized allocator
     * 
     * Creates an allocator in valid-but-empty state.
     * Moved-from allocators retain this property.
     */
    GPUMemoryAllocator() noexcept = default;

    /**
     * @brief Initialize allocator with configuration
     * 
     * @param config Allocator configuration
     * @throws std::runtime_error If GPU initialization fails
     * @throws std::invalid_argument If device_id is invalid
     */
    explicit GPUMemoryAllocator(const Config& config);

    /**
     * @brief Destructor - releases all allocations and GPU resources
     * 
     * Safe on:
     * - Initialized allocators (releases memory)
     * - Moved-from allocators (no-op, no double-free)
     * - Default-constructed allocators (no-op)
     */
    ~GPUMemoryAllocator() noexcept;

    // --- Move semantics (enabled) ---

    /**
     * @brief Move constructor
     * 
     * @param other Allocator to move from
     * 
     * Transfer semantics:
     * - All allocations transferred to this allocator
     * - `other` becomes safe moved-from state (no resources)
     * - Noexcept: does not allocate
     * 
     * @post other.is_moved_from() == true
     * @post other.get_config().device_id == -1
     */
    GPUMemoryAllocator(GPUMemoryAllocator&& other) noexcept;

    /**
     * @brief Move assignment operator
     * 
     * @param other Allocator to move from
     * @return Reference to this allocator
     * 
     * Release-and-acquire:
     * - Releases current allocations (double-free safe)
     * - Acquires all of `other`'s allocations
     * - `other` becomes moved-from state
     * - Self-assignment safe
     * 
     * @post other.is_moved_from() == true
     */
    GPUMemoryAllocator& operator=(GPUMemoryAllocator&& other) noexcept;

    // --- Copy semantics (deleted) ---
    GPUMemoryAllocator(const GPUMemoryAllocator&) = delete;
    GPUMemoryAllocator& operator=(const GPUMemoryAllocator&) = delete;

    // --- Memory operations ---

    /**
     * @brief Allocate GPU memory
     * 
     * @param size Size in bytes to allocate
     * @return MemoryAllocation descriptor with device_ptr, host_ptr, etc.
     * @throws std::runtime_error If allocation fails
     * @throws std::logic_error If called on moved-from allocator
     * 
     * @pre !is_moved_from()
     * @post returned.device_ptr != nullptr
     * @post returned.size == size
     */
    MemoryAllocation allocate(size_t size);

    /**
     * @brief Deallocate GPU memory
     * 
     * @param alloc Allocation to release (obtained from allocate())
     * @throws std::runtime_error If deallocation fails
     * @throws std::logic_error If called on moved-from allocator
     * 
     * Idempotent: deallocating the same allocation twice is logged
     * as warning but does not throw.
     * 
     * @pre !is_moved_from()
     */
    void deallocate(const MemoryAllocation& alloc) noexcept;

    /**
     * @brief Reallocate GPU memory (move contents)
     * 
     * @param alloc Current allocation
     * @param new_size New size in bytes
     * @return New MemoryAllocation with contents moved
     * @throws std::runtime_error If reallocation fails
     * @throws std::logic_error If called on moved-from allocator
     * 
     * The old allocation is automatically freed after successful
     * content transfer.
     */
    MemoryAllocation reallocate(const MemoryAllocation& alloc, size_t new_size);

    /**
     * @brief Copy memory from host to GPU
     * 
     * @param alloc Target allocation on GPU
     * @param host_data Source data on host
     * @param size Bytes to copy
     * @throws std::runtime_error If copy fails
     * @throws std::logic_error If called on moved-from allocator
     */
    void copy_to_device(const MemoryAllocation& alloc, 
                        const void* host_data, size_t size) const;

    /**
     * @brief Copy memory from GPU to host
     * 
     * @param host_data Target buffer on host
     * @param alloc Source allocation on GPU
     * @param size Bytes to copy
     * @throws std::runtime_error If copy fails
     * @throws std::logic_error If called on moved-from allocator
     */
    void copy_from_device(void* host_data, 
                          const MemoryAllocation& alloc, 
                          size_t size) const;

    // --- State and diagnostics ---

    /**
     * @brief Check if allocator is in moved-from state
     * 
     * @return true if all resources have been moved out
     */
    bool is_moved_from() const noexcept;

    /**
     * @brief Check if allocator is initialized
     * 
     * @return true if GPU device is ready for allocation
     */
    bool is_initialized() const noexcept;

    /**
     * @brief Get allocator configuration
     * 
     * @return Current Config
     */
    const Config& get_config() const noexcept;

    /**
     * @brief Query total GPU memory available
     * 
     * @return Available GPU memory in bytes
     * @throws std::runtime_error If query fails
     */
    size_t available_memory() const;

    /**
     * @brief Query allocated GPU memory
     * 
     * @return Currently allocated GPU memory in bytes
     */
    size_t allocated_memory() const noexcept;

    /**
     * @brief Get number of active allocations
     * 
     * @return Count of non-freed allocations
     */
    size_t allocation_count() const noexcept;

private:
    void cleanup() noexcept;

    Config config_;
    std::vector<MemoryAllocation> allocations_;
    uint64_t next_alloc_id_;
    bool is_moved_from_;
};

/**
 * @brief RAII wrapper for GPU device memory regions
 * 
 * Automatically manages memory lifetime and detects use-after-move.
 */
class DeviceMemoryRegion {
public:
    /**
     * @brief Create managed GPU memory region
     * 
     * @param allocator Allocator to use (must outlive this object)
     * @param size Region size in bytes
     * @throws std::runtime_error If allocation fails
     * @throws std::invalid_argument If size is 0
     */
    DeviceMemoryRegion(GPUMemoryAllocator& allocator, size_t size);

    /**
     * @brief Destructor - releases GPU memory
     * 
     * Automatic cleanup prevents resource leaks.
     */
    ~DeviceMemoryRegion() noexcept;

    // Move semantics
    DeviceMemoryRegion(DeviceMemoryRegion&& other) noexcept;
    DeviceMemoryRegion& operator=(DeviceMemoryRegion&& other) noexcept;

    // No copy
    DeviceMemoryRegion(const DeviceMemoryRegion&) = delete;
    DeviceMemoryRegion& operator=(const DeviceMemoryRegion&) = delete;

    /**
     * @brief Get GPU device pointer
     * 
     * @return GPU pointer, or nullptr if moved-from
     */
    void* device_ptr() noexcept;
    const void* device_ptr() const noexcept;

    /**
     * @brief Get CPU mirror pointer (if available)
     * 
     * @return CPU pointer, or nullptr if unavailable or moved-from
     */
    void* host_ptr() noexcept;

    /**
     * @brief Get region size in bytes
     * 
     * @return Size, or 0 if moved-from
     */
    size_t size() const noexcept;

    /**
     * @brief Check if region is valid
     * 
     * @return true if GPU memory is allocated
     */
    bool is_valid() const noexcept;

private:
    GPUMemoryAllocator* allocator_;
    MemoryAllocation alloc_;
    bool is_moved_from_;
};

} // namespace gpu
} // namespace themis
