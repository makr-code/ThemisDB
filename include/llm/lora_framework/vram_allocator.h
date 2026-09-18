/**
 * @file vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct VRAMBlock {
    /**
     * @brief VRAMBlock.
     * @return Return value.
     */
    virtual ~VRAMBlock() = default;
    void* ptr = nullptr;           // GPU memory pointer
    size_t size = 0;               // Size in bytes
    bool is_free = true;           // Allocation status
    size_t alignment = 256;        // Memory alignment (default 256 bytes for GPU)
};

class VRAMAllocator {
public:
    explicit VRAMAllocator(acceleration::BackendType backend, 
                          size_t pool_size_bytes = 0);
    
    ~VRAMAllocator();
    
    // Disable copy, allow move
    VRAMAllocator(const VRAMAllocator&) = delete;
    VRAMAllocator& operator=(const VRAMAllocator&) = delete;
    VRAMAllocator(VRAMAllocator&&) noexcept;
    VRAMAllocator& operator=(VRAMAllocator&&) noexcept;
    
    void* allocate(size_t size_bytes, size_t alignment = 256);
    
    /**
     * @brief Deallocate.
     * @param[in,out] ptr Input/output parameter.
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Upload.
     * @param[in,out] dst Input/output parameter.
     * @param[in] src Input parameter.
     * @param[in] size_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool upload(void* dst, const void* src, size_t size_bytes);
    
    /**
     * @brief Download.
     * @param[in,out] dst Input/output parameter.
     * @param[in] src Input parameter.
     * @param[in] size_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool download(void* dst, const void* src, size_t size_bytes);
    
    struct Stats {
        size_t total_bytes = 0;        // Total VRAM available
        size_t allocated_bytes = 0;    // Currently allocated
        size_t free_bytes = 0;         // Free VRAM
        size_t overhead_bytes = 0;     // Allocator overhead
        size_t peak_usage_bytes = 0;   // Peak usage
        size_t allocation_count = 0;   // Number of allocations
        float fragmentation = 0.0f;    // Fragmentation ratio (0.0-1.0)
    };
    
    /**
     * @brief Get stats.
     * @return Return value.
     */
    Stats get_stats() const;
    
    bool is_available() const { return initialized_; }
    
    acceleration::BackendType backend_type() const { return backend_; }
    
    /**
     * @brief Reset the modification detection flag.
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
    /**
     * @brief Initialize backend.
     * @return True when the operation succeeds.
     */
    bool initialize_backend();
    /**
     * @brief Shutdown backend.
     */
    void shutdown_backend();
    /**
     * @brief Allocate from backend.
     * @param[in] size_bytes Input parameter.
     * @param[in] alignment Input parameter.
     * @return Pointer to the result.
     */
    void* allocate_from_backend(size_t size_bytes, size_t alignment);
    /**
     * @brief Deallocate to backend.
     * @param[in,out] ptr Input/output parameter.
     */
    void deallocate_to_backend(void* ptr);
    /**
     * @brief Perform the actual backend deallocation WITHOUT holding mutex_.
     * @param[in,out] ptr Input/output parameter.
     * @param[in] block_size Input parameter.
     * @note Exception safety: noexcept.
     * @details Callers must supply the known block size (for secure clearing).
     */
    void release_backend_ptr_(void* ptr, size_t block_size) noexcept;
    /**
     * @brief Find free block.
     * @param[in] size_bytes Input parameter.
     * @param[in] alignment Input parameter.
     * @return Pointer to the result.
     */
    VRAMBlock* find_free_block(size_t size_bytes, size_t alignment);
    /**
     * @brief Coalesce free blocks.
     */
    void coalesce_free_blocks();  // Assumes lock is already held
};

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
    
    /**
     * @brief Upload.
     * @param[in] src Input parameter.
     * @param[in] size_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool upload(const void* src, size_t size_bytes);
    /**
     * @brief Download.
     * @param[in,out] dst Input/output parameter.
     * @param[in] size_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool download(void* dst, size_t size_bytes) const;

private:
    VRAMAllocator* allocator_ = nullptr;
    void* ptr_ = nullptr;
    size_t size_ = 0;
};

} // namespace lora
} // namespace llm
} // namespace themis
