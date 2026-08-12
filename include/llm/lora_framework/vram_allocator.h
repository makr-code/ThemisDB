/**
 * @file vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "acceleration/compute_backend.h"

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief GPU VRAM memory block descriptor
 */
struct VRAMBlock {
    virtual ~VRAMBlock() = default;
    void* ptr = nullptr;           // GPU memory pointer
    size_t size = 0;               // Size in bytes
    bool is_free = true;           // Allocation status
    size_t alignment = 256;        // Memory alignment (default 256 bytes for GPU)
};

/**
 * @brief VRAM Allocator for GPU memory management
 * 
 * Manages GPU VRAM for LoRA training with:
 * - Memory pooling for efficient allocation/deallocation
 * - Zero-copy where possible
 * - OOM handling and graceful degradation
 * - VRAM usage tracking and reporting
 * - < 5% memory overhead target
 */
class VRAMAllocator {
public:
    /**
     * @brief Construct allocator for specific backend
     * @param backend GPU backend to use (CUDA, Vulkan, HIP, DirectX)
     * @param pool_size_bytes Initial memory pool size (0 = auto-detect)
     */
    explicit VRAMAllocator(acceleration::BackendType backend, 
                          size_t pool_size_bytes = 0);
    
    ~VRAMAllocator();
    
    // Disable copy, allow move
    VRAMAllocator(const VRAMAllocator&) = delete;
    VRAMAllocator& operator=(const VRAMAllocator&) = delete;
    VRAMAllocator(VRAMAllocator&&) noexcept;
    VRAMAllocator& operator=(VRAMAllocator&&) noexcept;
    
    /**
     * @brief Allocate VRAM memory
     * @param size_bytes Size to allocate
     * @param alignment Memory alignment (default 256)
     * @return Pointer to GPU memory, or nullptr on failure
     */
    void* allocate(size_t size_bytes, size_t alignment = 256);
    
    /**
     * @brief Free VRAM memory
     * @param ptr Pointer to free
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Upload data from CPU to GPU
     * @param dst GPU destination pointer
     * @param src CPU source data
     * @param size_bytes Number of bytes to transfer
     * @return true on success
     */
    bool upload(void* dst, const void* src, size_t size_bytes);
    
    /**
     * @brief Download data from GPU to CPU
     * @param dst CPU destination buffer
     * @param src GPU source pointer
     * @param size_bytes Number of bytes to transfer
     * @return true on success
     */
    bool download(void* dst, const void* src, size_t size_bytes);
    
    /**
     * @brief Get current VRAM usage statistics
     */
    struct Stats {
        size_t total_bytes = 0;        // Total VRAM available
        size_t allocated_bytes = 0;    // Currently allocated
        size_t free_bytes = 0;         // Free VRAM
        size_t overhead_bytes = 0;     // Allocator overhead
        size_t peak_usage_bytes = 0;   // Peak usage
        size_t allocation_count = 0;   // Number of allocations
        float fragmentation = 0.0f;    // Fragmentation ratio (0.0-1.0)
    };
    
    Stats get_stats() const;
    
    /**
     * @brief Check if backend is available and initialized
     */
    bool is_available() const { return initialized_; }
    
    /**
     * @brief Get backend type
     */
    acceleration::BackendType backend_type() const { return backend_; }
    
    /**
     * @brief Reset allocator (free all memory)
     */
    void reset();

private:
    acceleration::BackendType backend_;
    bool initialized_ = false;
    
    // Memory pool management
    std::vector<VRAMBlock> memory_pool_;
    size_t pool_size_bytes_ = 0;
    size_t allocated_bytes_ = 0;
    size_t peak_usage_bytes_ = 0;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Backend-specific data
    void* backend_context_ = nullptr;  // CUDA context, Vulkan device, etc.
    
    // Internal helpers
    bool initialize_backend();
    void shutdown_backend();
    void* allocate_from_backend(size_t size_bytes, size_t alignment);
    void deallocate_to_backend(void* ptr);
    // Perform the actual backend deallocation WITHOUT holding mutex_.
    // Callers must supply the known block size (for secure clearing).
    void release_backend_ptr_(void* ptr, size_t block_size) noexcept;
    VRAMBlock* find_free_block(size_t size_bytes, size_t alignment);
    void coalesce_free_blocks();  // Assumes lock is already held
};

/**
 * @brief RAII wrapper for VRAM allocations
 */
class VRAMTensor {
public:
    VRAMTensor(VRAMAllocator* allocator, size_t size_bytes);
    ~VRAMTensor();
    
    // Disable copy, allow move
    VRAMTensor(const VRAMTensor&) = delete;
    VRAMTensor& operator=(const VRAMTensor&) = delete;
    VRAMTensor(VRAMTensor&& other) noexcept;
    VRAMTensor& operator=(VRAMTensor&& other) noexcept;
    
    void* ptr() const { return ptr_; }
    size_t size() const { return size_; }
    
    bool upload(const void* src, size_t size_bytes);
    bool download(void* dst, size_t size_bytes) const;

private:
    VRAMAllocator* allocator_ = nullptr;
    void* ptr_ = nullptr;
    size_t size_ = 0;
};

} // namespace lora
} // namespace llm
} // namespace themis
