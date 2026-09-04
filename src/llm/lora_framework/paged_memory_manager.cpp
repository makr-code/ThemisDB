/**
 * @file paged_memory_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/paged_memory_manager.h"
#include "acceleration/compute_backend.h"
#include <cstring>
#include <algorithm>

namespace themis {
namespace llm {
namespace lora {

// ===== PagedMemoryManager Implementation =====

PagedMemoryManager::PagedMemoryManager(size_t active_set_size,
                                       GPUMemoryManager* gpu_manager)
    : page_cache_(active_set_size),
      active_set_size_(active_set_size),
      gpu_manager_(gpu_manager) {
    
    // Create GPU memory manager if not provided
    if (!gpu_manager_) {
        owned_gpu_manager_ = std::make_unique<GPUMemoryManager>();
        gpu_manager_ = owned_gpu_manager_.get();
    }
    
    // Get default GPU device
    gpu_device_ = gpu_manager_->default_device();
    
    // Create CPU allocator with pinned memory for fast transfers
    cpu_allocator_ = std::make_unique<VRAMAllocator>(
        acceleration::BackendType::CPU,
        0  // Auto-detect size
    );
    
    // Create GPU allocator if GPU is available
    if (gpu_manager_->is_device_available(gpu_device_)) {
        auto backend = device_to_backend(gpu_device_.type);
        gpu_allocator_ = std::make_unique<VRAMAllocator>(backend, 0);
    }
}

PagedMemoryManager::~PagedMemoryManager() {
    // Deallocate all pages
    for (auto& pair : pages_) {
        PagedBuffer& buffer = pair.second;
        
        if (buffer.cpu_ptr) {
            cpu_allocator_->deallocate(buffer.cpu_ptr);
        }
        if (buffer.gpu_ptr && gpu_allocator_) {
            gpu_allocator_->deallocate(buffer.gpu_ptr);
        }
    }
    pages_.clear();
}

acceleration::BackendType PagedMemoryManager::device_to_backend(DeviceType type) {
    switch (type) {
        case DeviceType::CUDA:
            return acceleration::BackendType::CUDA;
        case DeviceType::HIP:
            return acceleration::BackendType::HIP;
        case DeviceType::VULKAN:
            return acceleration::BackendType::VULKAN;
        case DeviceType::DIRECTX:
            return acceleration::BackendType::DIRECTX;
        default:
            return acceleration::BackendType::CPU;
    }
}

PagedBuffer PagedMemoryManager::allocate(size_t size, const Device& device) {
    PagedBuffer buffer;
    buffer.id = next_page_id_++;
    buffer.size_bytes = size;
    buffer.current_device = device;
    buffer.last_access_time = getCurrentTimestamp();
    
    // Always allocate CPU memory for paging
    buffer.cpu_ptr = cpu_allocator_->allocate(size);
    if (!buffer.cpu_ptr) {
        // Allocation failed
        buffer.id = 0;
        return buffer;
    }
    
    // Initialize to zero
    std::memset(buffer.cpu_ptr, 0, size);
    
    // If GPU requested and available, allocate GPU memory too
    if (device.type != DeviceType::CPU && gpu_allocator_ && 
        gpu_allocator_->is_available()) {
        buffer.gpu_ptr = gpu_allocator_->allocate(size);
        if (buffer.gpu_ptr) {
            buffer.is_on_gpu = true;
            
            // Copy to GPU using VRAMAllocator
            gpu_allocator_->upload(buffer.gpu_ptr, buffer.cpu_ptr, size);
        }
    }
    
    // Track page
    pages_[buffer.id] = buffer;
    
    // Add to LRU cache
    PageInfo info;
    info.id = buffer.id;
    info.size_bytes = size;
    info.device = buffer.is_on_gpu ? gpu_device_ : Device::cpu();
    info.last_access_time = buffer.last_access_time;
    info.access_count = 1;
    page_cache_.put(buffer.id, info);
    
    return buffer;
}

void PagedMemoryManager::deallocate(PagedBuffer& buffer) {
    if (buffer.id == 0) {
      return;
    }
    
    // Remove from cache
    page_cache_.remove(buffer.id);
    
    // Free memory
    if (buffer.cpu_ptr && cpu_allocator_) {
        cpu_allocator_->deallocate(buffer.cpu_ptr);
        buffer.cpu_ptr = nullptr;
    }
    
    if (buffer.gpu_ptr && gpu_allocator_) {
        gpu_allocator_->deallocate(buffer.gpu_ptr);
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
        
        PageInfo info = {};
        if (page_cache_.get(buffer.id, info)) {
            info.last_access_time = buffer.last_access_time;
            info.access_count++;
            page_cache_.put(buffer.id, info);
        }
        
        return true;
    }
    
    // GPU not available?
    if (!gpu_allocator_ || !gpu_allocator_->is_available()) {
        return false;
    }
    
    // Allocate GPU memory if needed
    if (!buffer.gpu_ptr) {
        buffer.gpu_ptr = gpu_allocator_->allocate(buffer.size_bytes);
        if (!buffer.gpu_ptr) {
            // Try evicting some pages to make space
            size_t num_evicted = evictLRU(1, stream);
            if (num_evicted > 0) {
                buffer.gpu_ptr = gpu_allocator_->allocate(buffer.size_bytes);
            }
            
            if (!buffer.gpu_ptr) {
                return false;
            }
        }
    }
    
    // Transfer CPU -> GPU using VRAMAllocator
    if (!gpu_allocator_->upload(buffer.gpu_ptr, buffer.cpu_ptr, buffer.size_bytes)) {
        return false;
    }
    
    buffer.is_on_gpu = true;
    buffer.current_device = gpu_device_;
    buffer.last_access_time = getCurrentTimestamp();
    
    // Update cache
    PageInfo info = {};
    if (page_cache_.get(buffer.id, info)) {
        info.device = gpu_device_;
        info.last_access_time = buffer.last_access_time;
        info.access_count++;
        page_cache_.put(buffer.id, info);
    }
    
    // Update tracked page
    pages_[buffer.id] = buffer;
    
    return true;
}

bool PagedMemoryManager::pageOut(PagedBuffer& buffer, void* /*stream*/) {
    if (buffer.id == 0 || !buffer.cpu_ptr) {
        return false;
    }
    
    // Not on GPU?
    if (!buffer.is_on_gpu || !buffer.gpu_ptr) {
        return true;  // Already on CPU
    }
    
    // Transfer GPU -> CPU using VRAMAllocator
    if (!gpu_allocator_->download(buffer.cpu_ptr, buffer.gpu_ptr, buffer.size_bytes)) {
        return false;
    }
    
    // Free GPU memory
    gpu_allocator_->deallocate(buffer.gpu_ptr);
    buffer.gpu_ptr = nullptr;
    buffer.is_on_gpu = false;
    buffer.current_device = Device::cpu();
    
    // Update cache
    PageInfo info = {};
    if (page_cache_.get(buffer.id, info)) {
        info.device = Device::cpu();
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
        if (it == pages_.end()) {
          continue;
        }
        
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
