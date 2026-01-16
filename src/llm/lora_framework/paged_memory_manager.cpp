#include "llm/lora_framework/paged_memory_manager.h"
#include <cstring>
#include <algorithm>

// CUDA headers if available
#ifdef __CUDACC__
#include <cuda_runtime.h>
#endif

namespace themis {
namespace llm {
namespace lora {

// ===== PinnedMemoryPool Implementation =====

PinnedMemoryPool::PinnedMemoryPool() {
#ifdef __CUDACC__
    // Check if CUDA is available
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    cuda_available_ = (err == cudaSuccess && device_count > 0);
#else
    cuda_available_ = false;
#endif
}

PinnedMemoryPool::~PinnedMemoryPool() {
    // Free all allocations
    for (auto& pair : allocations_) {
        void* ptr = pair.first;
#ifdef __CUDACC__
        if (cuda_available_) {
            cudaFreeHost(ptr);
        } else {
            delete[] static_cast<char*>(ptr);
        }
#else
        delete[] static_cast<char*>(ptr);
#endif
    }
    allocations_.clear();
}

void* PinnedMemoryPool::allocate(size_t size) {
    void* ptr = nullptr;
    
#ifdef __CUDACC__
    if (cuda_available_) {
        // Allocate pinned memory for fast DMA
        cudaError_t err = cudaMallocHost(&ptr, size);
        if (err != cudaSuccess) {
            return nullptr;
        }
    } else {
        // Fallback to regular heap allocation
        ptr = new (std::nothrow) char[size];
    }
#else
    // CPU-only: use regular heap allocation
    ptr = new (std::nothrow) char[size];
#endif
    
    if (ptr) {
        allocations_[ptr] = size;
        total_allocated_ += size;
    }
    
    return ptr;
}

void PinnedMemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    auto it = allocations_.find(ptr);
    if (it == allocations_.end()) return;
    
    size_t size = it->second;
    
#ifdef __CUDACC__
    if (cuda_available_) {
        cudaFreeHost(ptr);
    } else {
        delete[] static_cast<char*>(ptr);
    }
#else
    delete[] static_cast<char*>(ptr);
#endif
    
    total_allocated_ -= size;
    allocations_.erase(it);
}

// ===== GPUMemoryPool Implementation =====

GPUMemoryPool::GPUMemoryPool() {
#ifdef __CUDACC__
    // Check if CUDA is available
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    cuda_available_ = (err == cudaSuccess && device_count > 0);
#else
    cuda_available_ = false;
#endif
}

GPUMemoryPool::~GPUMemoryPool() {
    // Free all allocations
    for (auto& pair : allocations_) {
        void* ptr = pair.first;
#ifdef __CUDACC__
        if (cuda_available_) {
            cudaFree(ptr);
        }
#endif
    }
    allocations_.clear();
}

void* GPUMemoryPool::allocate(size_t size) {
    void* ptr = nullptr;
    
#ifdef __CUDACC__
    if (cuda_available_) {
        cudaError_t err = cudaMalloc(&ptr, size);
        if (err != cudaSuccess) {
            return nullptr;
        }
        
        allocations_[ptr] = size;
        total_allocated_ += size;
    }
#endif
    
    return ptr;
}

void GPUMemoryPool::deallocate(void* ptr) {
    if (!ptr) return;
    
    auto it = allocations_.find(ptr);
    if (it == allocations_.end()) return;
    
    size_t size = it->second;
    
#ifdef __CUDACC__
    if (cuda_available_) {
        cudaFree(ptr);
    }
#endif
    
    total_allocated_ -= size;
    allocations_.erase(it);
}

// ===== PagedMemoryManager Implementation =====

PagedMemoryManager::PagedMemoryManager(size_t active_set_size)
    : page_cache_(active_set_size),
      active_set_size_(active_set_size) {
    cpu_pool_ = std::make_unique<PinnedMemoryPool>();
    gpu_pool_ = std::make_unique<GPUMemoryPool>();
}

PagedMemoryManager::~PagedMemoryManager() {
    // Deallocate all pages
    for (auto& pair : pages_) {
        PagedBuffer& buffer = pair.second;
        
        if (buffer.cpu_ptr) {
            cpu_pool_->deallocate(buffer.cpu_ptr);
        }
        if (buffer.gpu_ptr) {
            gpu_pool_->deallocate(buffer.gpu_ptr);
        }
    }
    pages_.clear();
}

PagedBuffer PagedMemoryManager::allocate(size_t size, DeviceType device) {
    PagedBuffer buffer;
    buffer.id = next_page_id_++;
    buffer.size_bytes = size;
    buffer.current_location = device;
    buffer.last_access_time = getCurrentTimestamp();
    
    // Allocate CPU memory (always allocate for paging)
    buffer.cpu_ptr = cpu_pool_->allocate(size);
    if (!buffer.cpu_ptr) {
        // Allocation failed
        buffer.id = 0;
        return buffer;
    }
    
    // Initialize to zero
    std::memset(buffer.cpu_ptr, 0, size);
    
    // If GPU requested and available, allocate GPU memory too
    if (device == DeviceType::GPU && gpu_pool_->is_cuda_available()) {
        buffer.gpu_ptr = gpu_pool_->allocate(size);
        if (buffer.gpu_ptr) {
            buffer.is_on_gpu = true;
            
            // Copy to GPU
#ifdef __CUDACC__
            cudaMemcpy(buffer.gpu_ptr, buffer.cpu_ptr, size, cudaMemcpyHostToDevice);
#endif
        }
    }
    
    // Track page
    pages_[buffer.id] = buffer;
    
    // Add to LRU cache
    PageInfo info;
    info.id = buffer.id;
    info.size_bytes = size;
    info.location = buffer.is_on_gpu ? DeviceType::GPU : DeviceType::CPU;
    info.last_access_time = buffer.last_access_time;
    info.access_count = 1;
    page_cache_.put(buffer.id, info);
    
    return buffer;
}

void PagedMemoryManager::deallocate(PagedBuffer& buffer) {
    if (buffer.id == 0) return;
    
    // Remove from cache
    page_cache_.remove(buffer.id);
    
    // Free memory
    if (buffer.cpu_ptr) {
        cpu_pool_->deallocate(buffer.cpu_ptr);
        buffer.cpu_ptr = nullptr;
    }
    
    if (buffer.gpu_ptr) {
        gpu_pool_->deallocate(buffer.gpu_ptr);
        buffer.gpu_ptr = nullptr;
    }
    
    // Remove from tracking
    pages_.erase(buffer.id);
    
    buffer.id = 0;
    buffer.size_bytes = 0;
    buffer.is_on_gpu = false;
}

bool PagedMemoryManager::pageIn(PagedBuffer& buffer, void* stream) {
    if (buffer.id == 0 || !buffer.cpu_ptr) {
        return false;
    }
    
    // Already on GPU?
    if (buffer.is_on_gpu && buffer.gpu_ptr) {
        // Update access time
        buffer.last_access_time = getCurrentTimestamp();
        
        PageInfo info;
        if (page_cache_.get(buffer.id, info)) {
            info.last_access_time = buffer.last_access_time;
            info.access_count++;
            page_cache_.put(buffer.id, info);
        }
        
        return true;
    }
    
    // CUDA not available?
    if (!gpu_pool_->is_cuda_available()) {
        return false;
    }
    
    // Allocate GPU memory if needed
    if (!buffer.gpu_ptr) {
        buffer.gpu_ptr = gpu_pool_->allocate(buffer.size_bytes);
        if (!buffer.gpu_ptr) {
            // Try evicting some pages to make space
            size_t num_evicted = evictLRU(1, stream);
            if (num_evicted > 0) {
                buffer.gpu_ptr = gpu_pool_->allocate(buffer.size_bytes);
            }
            
            if (!buffer.gpu_ptr) {
                return false;
            }
        }
    }
    
    // Transfer CPU -> GPU
#ifdef __CUDACC__
    cudaError_t err;
    if (stream) {
        err = cudaMemcpyAsync(buffer.gpu_ptr, buffer.cpu_ptr, buffer.size_bytes,
                              cudaMemcpyHostToDevice, static_cast<cudaStream_t>(stream));
    } else {
        err = cudaMemcpy(buffer.gpu_ptr, buffer.cpu_ptr, buffer.size_bytes,
                         cudaMemcpyHostToDevice);
    }
    
    if (err != cudaSuccess) {
        return false;
    }
#endif
    
    buffer.is_on_gpu = true;
    buffer.current_location = DeviceType::GPU;
    buffer.last_access_time = getCurrentTimestamp();
    
    // Update cache
    PageInfo info;
    if (page_cache_.get(buffer.id, info)) {
        info.location = DeviceType::GPU;
        info.last_access_time = buffer.last_access_time;
        info.access_count++;
        page_cache_.put(buffer.id, info);
    }
    
    // Update tracked page
    pages_[buffer.id] = buffer;
    
    return true;
}

bool PagedMemoryManager::pageOut(PagedBuffer& buffer, void* stream) {
    if (buffer.id == 0 || !buffer.cpu_ptr) {
        return false;
    }
    
    // Not on GPU?
    if (!buffer.is_on_gpu || !buffer.gpu_ptr) {
        return true;  // Already on CPU
    }
    
    // Transfer GPU -> CPU
#ifdef __CUDACC__
    cudaError_t err;
    if (stream) {
        err = cudaMemcpyAsync(buffer.cpu_ptr, buffer.gpu_ptr, buffer.size_bytes,
                              cudaMemcpyDeviceToHost, static_cast<cudaStream_t>(stream));
    } else {
        err = cudaMemcpy(buffer.cpu_ptr, buffer.gpu_ptr, buffer.size_bytes,
                         cudaMemcpyDeviceToHost);
    }
    
    if (err != cudaSuccess) {
        return false;
    }
#endif
    
    // Free GPU memory
    gpu_pool_->deallocate(buffer.gpu_ptr);
    buffer.gpu_ptr = nullptr;
    buffer.is_on_gpu = false;
    buffer.current_location = DeviceType::CPU;
    
    // Update cache
    PageInfo info;
    if (page_cache_.get(buffer.id, info)) {
        info.location = DeviceType::CPU;
        page_cache_.put(buffer.id, info);
    }
    
    // Update tracked page
    pages_[buffer.id] = buffer;
    
    return true;
}

size_t PagedMemoryManager::evictLRU(size_t num_pages, void* stream) {
    // Get LRU pages
    std::vector<PageID> lru_pages = page_cache_.getLRUKeys(num_pages);
    
    size_t evicted = 0;
    for (PageID page_id : lru_pages) {
        auto it = pages_.find(page_id);
        if (it == pages_.end()) continue;
        
        PagedBuffer& buffer = it->second;
        
        // Only evict if on GPU
        if (buffer.is_on_gpu && buffer.gpu_ptr) {
            if (pageOut(buffer, stream)) {
                evicted++;
            }
        }
    }
    
    return evicted;
}

} // namespace lora
} // namespace llm
} // namespace themis
