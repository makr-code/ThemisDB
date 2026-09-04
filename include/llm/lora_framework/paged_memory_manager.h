/**
 * @file paged_memory_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/vram_allocator.h"
#include "llm/lora_framework/gpu_memory.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Page identifier type
 */
using PageID = uint64_t;

/**
 * @brief Paged buffer handle
 * 
 * Represents a buffer that can be allocated on CPU or GPU
 * and paged between them.
 */
struct PagedBuffer {
    virtual ~PagedBuffer() = default;
    PageID id = 0;
    size_t size_bytes = 0;
    void* cpu_ptr = nullptr;
    void* gpu_ptr = nullptr;
    Device current_device = Device::cpu();
    bool is_on_gpu = false;
    uint64_t last_access_time = 0;  // For LRU eviction
};

/**
 * @brief Page information for tracking
 */
struct PageInfo {
    virtual ~PageInfo() = default;
    PageID id = 0;
    size_t size_bytes = 0;
    Device device = Device::cpu();
    uint64_t last_access_time = 0;
    uint64_t access_count = 0;
};

/**
 * @brief LRU cache for page eviction
 * 
 * Tracks page access patterns and determines which pages
 * should be evicted from GPU to CPU when memory is needed.
 */
template<typename Key, typename Value>
class LRUCache {
public:
    virtual ~LRUCache() = default;
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}
    
    void put(const Key& key, const Value& value) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            it->second = value;
        } else {
            if (cache_.size() >= capacity_) {
                evictLRU();
            }
            cache_[key] = value;
        }
    }
    
    bool get(const Key& key, Value& value) {
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            value = it->second;
            return true;
        }
        return false;
    }
    
    void remove(const Key& key) {
        cache_.erase(key);
    }
    
    std::vector<Key> getLRUKeys(size_t count) const {
        std::vector<std::pair<Key, uint64_t>> entries;
        entries.reserve(cache_.size());
        
        for (const auto& pair : cache_) {
            entries.push_back({pair.first, pair.second.last_access_time});
        }
        
        // Use partial_sort for better performance: O(n log count) instead of O(n log n)
        size_t sort_count = std::min(count, entries.size());
        std::partial_sort(entries.begin(), 
                         entries.begin() + sort_count, 
                         entries.end(),
                         [](const auto& a, const auto& b) {
                             return a.second < b.second;  // Oldest first
                         });
        
        std::vector<Key> result;
        result.reserve(sort_count);
        for (size_t i = 0; i < sort_count; ++i) {
            result.push_back(entries[i].first);
        }
        return result;
    }
    
    size_t size() const { return cache_.size(); }
    
private:
    void evictLRU() {
        if (cache_.empty()) {
          return;
        }
        
        auto lru = cache_.begin();
        uint64_t oldest_time = lru->second.last_access_time;
        
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->second.last_access_time < oldest_time) {
                oldest_time = it->second.last_access_time;
                lru = it;
            }
        }
        
        cache_.erase(lru);
    }
    
    size_t capacity_ = 0;
    std::unordered_map<Key, Value> cache_;
};

/**
 * @brief Paged memory manager
 * 
 * Manages memory paging between CPU and GPU for optimizer states.
 * Leverages existing VRAMAllocator and GPUMemoryManager infrastructure.
 * 
 * Features:
 * - Page-based allocation using existing memory pools
 * - Asynchronous page transfers (when stream is provided)
 * - LRU eviction policy for GPU memory
 * - Multi-backend support (CUDA/HIP/Vulkan/DirectX) via GPUMemoryManager
 * - Fallback to CPU-only if GPU unavailable
 * 
 * Example usage:
 * @code
 * PagedMemoryManager manager;
 * 
 * // Allocate buffer on CPU
 * PagedBuffer buffer = manager.allocate(1024 * 1024, Device::cpu());
 * 
 * // Page in to GPU when needed
 * manager.pageIn(buffer, nullptr);
 * 
 * // Use on GPU...
 * 
 * // Page out when done
 * manager.pageOut(buffer, nullptr);
 * 
 * // Free buffer
 * manager.deallocate(buffer);
 * @endcode
 */
class PagedMemoryManager {
public:
    /**
     * @brief Construct paged memory manager
     * @param active_set_size Maximum number of pages to keep on GPU (LRU cache size)
     * @param gpu_manager Optional GPU memory manager (creates one if nullptr)
     */
    explicit PagedMemoryManager(size_t active_set_size = 1024,
                                GPUMemoryManager* gpu_manager = nullptr);
    
    ~PagedMemoryManager();
    
    /**
     * @brief Allocate paged buffer
     * @param size Size in bytes
     * @param device Initial device location
     * @return Allocated buffer handle
     */
    PagedBuffer allocate(size_t size, const Device& device = Device::cpu());
    
    /**
     * @brief Deallocate paged buffer
     * @param buffer Buffer to deallocate
     */
    void deallocate(PagedBuffer& buffer);
    
    /**
     * @brief Page in buffer from CPU to GPU
     * @param buffer Buffer to page in
     * @param stream CUDA stream for async transfer (nullptr for sync)
     * @return True if successful, false otherwise
     */
    bool pageIn(PagedBuffer& buffer, void* stream = nullptr);
    
    /**
     * @brief Page out buffer from GPU to CPU
     * @param buffer Buffer to page out
     * @param stream CUDA stream for async transfer (nullptr for sync)
     * @return True if successful, false otherwise
     */
    bool pageOut(PagedBuffer& buffer, void* stream = nullptr);
    
    /**
     * @brief Check if buffer is currently on GPU
     * @param buffer Buffer to check
     * @return True if on GPU, false otherwise
     */
    bool isOnGPU(const PagedBuffer& buffer) const {
        return buffer.is_on_gpu;
    }
    
    /**
     * @brief Evict least recently used pages from GPU
     * @param num_pages Number of pages to evict
     * @param stream CUDA stream for async transfer (nullptr for sync)
     * @return Number of pages actually evicted
     */
    size_t evictLRU(size_t num_pages, void* stream = nullptr);
    
    /**
     * @brief Get total GPU memory allocated
     */
    size_t gpu_memory_used() const {
        return gpu_allocator_ ? gpu_allocator_->get_stats().allocated_bytes : 0;
    }
    
    /**
     * @brief Get total CPU memory allocated
     */
    size_t cpu_memory_used() const {
        return cpu_allocator_ ? cpu_allocator_->get_stats().allocated_bytes : 0;
    }
    
    /**
     * @brief Check if GPU backend is available
     */
    bool is_cuda_available() const {
        return gpu_allocator_ && gpu_allocator_->is_available();
    }
    
private:
    // Memory allocators (use existing infrastructure)
    std::unique_ptr<VRAMAllocator> cpu_allocator_;    // CPU with pinned memory
    std::unique_ptr<VRAMAllocator> gpu_allocator_;    // GPU device memory
    
    // GPU memory manager (optional, owns lifecycle if created internally)
    std::unique_ptr<GPUMemoryManager> owned_gpu_manager_;
    GPUMemoryManager* gpu_manager_ = nullptr;
    
    // Default device for GPU operations
    Device gpu_device_;
    
    // LRU cache for page eviction
    LRUCache<PageID, PageInfo> page_cache_;
    
    // Page tracking
    std::unordered_map<PageID, PagedBuffer> pages_;
    
    // Next page ID
    PageID next_page_id_ = 1;
    
    // Active set size (max pages on GPU)
    size_t active_set_size_ = 0;
    
    // Access counter for LRU
    mutable uint64_t access_counter_ = 0;
    
    // Helper to get current timestamp for LRU
    uint64_t getCurrentTimestamp() const {
        return access_counter_++;
    }
    
    // Helper to convert Device to backend type
    static acceleration::BackendType device_to_backend(DeviceType type);
};

} // namespace lora
} // namespace llm
} // namespace themis
