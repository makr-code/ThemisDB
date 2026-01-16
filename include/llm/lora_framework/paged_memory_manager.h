#pragma once

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
 * @brief Device type for memory allocation
 */
enum class DeviceType {
    CPU,    // CPU memory (pinned for fast transfers)
    GPU     // GPU device memory
};

/**
 * @brief Paged buffer handle
 * 
 * Represents a buffer that can be allocated on CPU or GPU
 * and paged between them.
 */
struct PagedBuffer {
    PageID id = 0;
    size_t size_bytes = 0;
    void* cpu_ptr = nullptr;
    void* gpu_ptr = nullptr;
    DeviceType current_location = DeviceType::CPU;
    bool is_on_gpu = false;
    uint64_t last_access_time = 0;  // For LRU eviction
};

/**
 * @brief Page information for tracking
 */
struct PageInfo {
    PageID id = 0;
    size_t size_bytes = 0;
    DeviceType location = DeviceType::CPU;
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
        if (cache_.empty()) return;
        
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
    
    size_t capacity_;
    std::unordered_map<Key, Value> cache_;
};

/**
 * @brief CPU memory pool using pinned memory for fast transfers
 * 
 * Manages pinned (page-locked) CPU memory for fast DMA transfers
 * to/from GPU. On systems without CUDA, uses regular heap allocation.
 */
class PinnedMemoryPool {
public:
    PinnedMemoryPool();
    ~PinnedMemoryPool();
    
    /**
     * @brief Allocate pinned CPU memory
     * @param size Size in bytes
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* allocate(size_t size);
    
    /**
     * @brief Free pinned CPU memory
     * @param ptr Pointer to memory to free
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Get total allocated bytes
     */
    size_t total_allocated() const { return total_allocated_; }
    
private:
    size_t total_allocated_ = 0;
    std::unordered_map<void*, size_t> allocations_;
    bool cuda_available_ = false;
};

/**
 * @brief GPU memory pool
 * 
 * Manages GPU device memory allocation. Falls back to CPU
 * if CUDA is not available.
 */
class GPUMemoryPool {
public:
    GPUMemoryPool();
    ~GPUMemoryPool();
    
    /**
     * @brief Allocate GPU memory
     * @param size Size in bytes
     * @return Pointer to allocated memory, or nullptr on failure
     */
    void* allocate(size_t size);
    
    /**
     * @brief Free GPU memory
     * @param ptr Pointer to memory to free
     */
    void deallocate(void* ptr);
    
    /**
     * @brief Get total allocated bytes
     */
    size_t total_allocated() const { return total_allocated_; }
    
    /**
     * @brief Check if CUDA is available
     */
    bool is_cuda_available() const { return cuda_available_; }
    
private:
    size_t total_allocated_ = 0;
    std::unordered_map<void*, size_t> allocations_;
    bool cuda_available_ = false;
};

/**
 * @brief Paged memory manager
 * 
 * Manages memory paging between CPU and GPU for optimizer states.
 * Implements automatic page-in/page-out based on LRU eviction policy.
 * 
 * Features:
 * - Page-based allocation on CPU or GPU
 * - Asynchronous page transfers (when stream is provided)
 * - LRU eviction policy for GPU memory
 * - Pinned CPU memory for fast transfers
 * - Fallback to CPU-only if CUDA unavailable
 * 
 * Example usage:
 * @code
 * PagedMemoryManager manager;
 * 
 * // Allocate buffer on CPU
 * PagedBuffer buffer = manager.allocate(1024 * 1024, DeviceType::CPU);
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
     */
    explicit PagedMemoryManager(size_t active_set_size = 1024);
    
    ~PagedMemoryManager();
    
    /**
     * @brief Allocate paged buffer
     * @param size Size in bytes
     * @param device Initial device location
     * @return Allocated buffer handle
     */
    PagedBuffer allocate(size_t size, DeviceType device = DeviceType::CPU);
    
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
        return gpu_pool_ ? gpu_pool_->total_allocated() : 0;
    }
    
    /**
     * @brief Get total CPU memory allocated
     */
    size_t cpu_memory_used() const {
        return cpu_pool_ ? cpu_pool_->total_allocated() : 0;
    }
    
    /**
     * @brief Check if CUDA is available
     */
    bool is_cuda_available() const {
        return gpu_pool_ ? gpu_pool_->is_cuda_available() : false;
    }
    
private:
    // Memory pools
    std::unique_ptr<PinnedMemoryPool> cpu_pool_;
    std::unique_ptr<GPUMemoryPool> gpu_pool_;
    
    // LRU cache for page eviction
    LRUCache<PageID, PageInfo> page_cache_;
    
    // Page tracking
    std::unordered_map<PageID, PagedBuffer> pages_;
    
    // Next page ID
    PageID next_page_id_ = 1;
    
    // Active set size (max pages on GPU)
    size_t active_set_size_;
    
    // Access counter for LRU
    uint64_t access_counter_ = 0;
    
    // Helper to get current timestamp for LRU
    uint64_t getCurrentTimestamp() {
        return access_counter_++;
    }
};

} // namespace lora
} // namespace llm
} // namespace themis
