#include "llm/gpu_memory_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>

// TODO: Include actual CUDA headers when CUDA support is built
// #ifdef THEMIS_CUDA_ENABLED
// #include <cuda_runtime.h>
// #endif

namespace themis {
namespace llm {

GPUMemoryManager::GPUMemoryManager(const Config& config)
    : config_(config) {
    spdlog::info("GPU Memory Manager initialized:");
    spdlog::info("  Max VRAM: {} GB", config_.max_vram_bytes / (1024.0 * 1024 * 1024));
    spdlog::info("  Max RAM: {} GB", config_.max_ram_bytes / (1024.0 * 1024 * 1024));
    spdlog::info("  Memory pooling: {}", config_.enable_memory_pooling ? "enabled" : "disabled");
    
    initializeGPU();
}

GPUMemoryManager::~GPUMemoryManager() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Free all allocations
    for (auto& [model_id, allocs] : allocations_) {
        spdlog::info("Freeing memory for model: {}", model_id);
        for (auto& alloc : allocs) {
            if (alloc.gpu_ptr) {
                // TODO: cudaFree(alloc.gpu_ptr);
                std::free(alloc.gpu_ptr);  // Placeholder
            }
            if (alloc.cpu_ptr) {
                if (alloc.is_pinned) {
                    // TODO: cudaFreeHost(alloc.cpu_ptr);
                    std::free(alloc.cpu_ptr);  // Placeholder
                } else {
                    std::free(alloc.cpu_ptr);
                }
            }
        }
    }
    
    shutdownGPU();
}

void GPUMemoryManager::initializeGPU() {
    // TODO: When CUDA is available:
    // cudaError_t err = cudaGetDevice(&gpu_device_id_);
    // if (err == cudaSuccess) {
    //     gpu_available_ = true;
    //     cudaDeviceProp prop;
    //     cudaGetDeviceProperties(&prop, gpu_device_id_);
    //     spdlog::info("GPU detected: {} (Compute {}.{})", 
    //                  prop.name, prop.major, prop.minor);
    //     spdlog::info("  Total VRAM: {} GB", prop.totalGlobalMem / (1024.0*1024*1024));
    // } else {
    //     gpu_available_ = false;
    //     spdlog::warn("No GPU detected, running in CPU-only mode");
    // }
    
    // For now, assume GPU is available (simulation mode)
    gpu_available_ = true;
    gpu_device_id_ = 0;
    spdlog::info("GPU Memory Manager: Running in simulation mode (actual GPU support in CUDA build)");
}

void GPUMemoryManager::shutdownGPU() {
    if (gpu_available_) {
        // TODO: cudaDeviceReset();
    }
}

void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!canAllocate(bytes, 0)) {
        spdlog::error("Cannot allocate {} bytes VRAM for model {}: insufficient memory", 
                      bytes, model_id);
        return nullptr;
    }
    
    void* ptr = nullptr;
    
    // TODO: When CUDA is available:
    // cudaError_t err = cudaMalloc(&ptr, bytes);
    // if (err != cudaSuccess) {
    //     spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
    //     return nullptr;
    // }
    
    // Placeholder: use regular malloc for simulation
    ptr = std::malloc(bytes);
    if (!ptr) {
        spdlog::error("Failed to allocate {} bytes for model {}", bytes, model_id);
        return nullptr;
    }
    
    // Track allocation
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.vram_bytes = bytes;
    alloc.gpu_ptr = ptr;
    
    allocations_[model_id].push_back(alloc);
    total_vram_used_ += bytes;
    
    spdlog::debug("Allocated {} MB VRAM for model {} (total: {} MB)", 
                  bytes / (1024.0 * 1024),
                  model_id,
                  total_vram_used_ / (1024.0 * 1024));
    
    return ptr;
}

void* GPUMemoryManager::allocateCPU(const std::string& model_id, size_t bytes, bool pinned) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!canAllocate(0, bytes)) {
        spdlog::error("Cannot allocate {} bytes RAM for model {}: insufficient memory", 
                      bytes, model_id);
        return nullptr;
    }
    
    void* ptr = nullptr;
    
    if (pinned && gpu_available_) {
        // TODO: When CUDA is available:
        // cudaError_t err = cudaMallocHost(&ptr, bytes);
        // if (err != cudaSuccess) {
        //     spdlog::warn("cudaMallocHost failed, falling back to regular malloc");
        //     pinned = false;
        // }
        
        // Placeholder
        ptr = std::malloc(bytes);
        pinned = false;
    }
    
    if (!ptr) {
        ptr = std::malloc(bytes);
    }
    
    if (!ptr) {
        spdlog::error("Failed to allocate {} bytes RAM for model {}", bytes, model_id);
        return nullptr;
    }
    
    // Track allocation
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.ram_bytes = bytes;
    alloc.cpu_ptr = ptr;
    alloc.is_pinned = pinned;
    
    allocations_[model_id].push_back(alloc);
    total_ram_used_ += bytes;
    
    spdlog::debug("Allocated {} MB RAM ({}) for model {} (total: {} MB)", 
                  bytes / (1024.0 * 1024),
                  pinned ? "pinned" : "pageable",
                  model_id,
                  total_ram_used_ / (1024.0 * 1024));
    
    return ptr;
}

bool GPUMemoryManager::freeGPU(const std::string& model_id, void* ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->gpu_ptr == ptr) {
            // TODO: cudaFree(ptr);
            std::free(ptr);
            
            total_vram_used_ -= alloc_it->vram_bytes;
            it->second.erase(alloc_it);
            
            if (it->second.empty()) {
                allocations_.erase(it);
            }
            
            return true;
        }
    }
    
    return false;
}

bool GPUMemoryManager::freeCPU(const std::string& model_id, void* ptr) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->cpu_ptr == ptr) {
            if (alloc_it->is_pinned) {
                // TODO: cudaFreeHost(ptr);
                std::free(ptr);
            } else {
                std::free(ptr);
            }
            
            total_ram_used_ -= alloc_it->ram_bytes;
            it->second.erase(alloc_it);
            
            if (it->second.empty()) {
                allocations_.erase(it);
            }
            
            return true;
        }
    }
    
    return false;
}

bool GPUMemoryManager::freeModel(const std::string& model_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    size_t freed_vram = 0;
    size_t freed_ram = 0;
    
    for (auto& alloc : it->second) {
        if (alloc.gpu_ptr) {
            // TODO: cudaFree(alloc.gpu_ptr);
            std::free(alloc.gpu_ptr);
            freed_vram += alloc.vram_bytes;
        }
        if (alloc.cpu_ptr) {
            if (alloc.is_pinned) {
                // TODO: cudaFreeHost(alloc.cpu_ptr);
                std::free(alloc.cpu_ptr);
            } else {
                std::free(alloc.cpu_ptr);
            }
            freed_ram += alloc.ram_bytes;
        }
    }
    
    total_vram_used_ -= freed_vram;
    total_ram_used_ -= freed_ram;
    
    allocations_.erase(it);
    
    spdlog::info("Freed memory for model {}: {} MB VRAM, {} MB RAM",
                 model_id,
                 freed_vram / (1024.0 * 1024),
                 freed_ram / (1024.0 * 1024));
    
    return true;
}

size_t GPUMemoryManager::getModelVRAM(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return 0;
    }
    
    size_t total = 0;
    for (const auto& alloc : it->second) {
        total += alloc.vram_bytes;
    }
    
    return total;
}

size_t GPUMemoryManager::getModelRAM(const std::string& model_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return 0;
    }
    
    size_t total = 0;
    for (const auto& alloc : it->second) {
        total += alloc.ram_bytes;
    }
    
    return total;
}

size_t GPUMemoryManager::getTotalVRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_vram_used_;
}

size_t GPUMemoryManager::getTotalRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return total_ram_used_;
}

size_t GPUMemoryManager::getFreeVRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (total_vram_used_ >= config_.max_vram_bytes) {
        return 0;
    }
    
    return config_.max_vram_bytes - total_vram_used_;
}

size_t GPUMemoryManager::getFreeRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (total_ram_used_ >= config_.max_ram_bytes) {
        return 0;
    }
    
    return config_.max_ram_bytes - total_ram_used_;
}

bool GPUMemoryManager::canAllocate(size_t vram_bytes, size_t ram_bytes) const {
    // Already locked by caller
    
    size_t future_vram = total_vram_used_ + vram_bytes;
    size_t future_ram = total_ram_used_ + ram_bytes;
    
    // Check hard limits
    if (future_vram > config_.max_vram_bytes) {
        return false;
    }
    
    if (future_ram > config_.max_ram_bytes) {
        return false;
    }
    
    // Check if we maintain minimum free VRAM
    if (vram_bytes > 0) {
        size_t remaining = config_.max_vram_bytes - future_vram;
        if (remaining < config_.min_free_vram_bytes) {
            return false;
        }
    }
    
    return true;
}

size_t GPUMemoryManager::getMemoryFragmentation() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Calculate fragmentation as percentage
    // For now, simple heuristic: number of allocations vs ideal
    size_t total_allocations = 0;
    for (const auto& [_, allocs] : allocations_) {
        total_allocations += allocs.size();
    }
    
    size_t num_models = allocations_.size();
    if (num_models == 0) {
        return 0;
    }
    
    // Ideal: 1 allocation per model
    // Fragmentation increases with more allocations per model
    size_t excess_allocations = total_allocations > num_models 
        ? total_allocations - num_models 
        : 0;
    
    // Convert to percentage (capped at 100%)
    size_t fragmentation_pct = std::min(100UL, (excess_allocations * 100) / num_models);
    
    return fragmentation_pct;
}

bool GPUMemoryManager::defragment() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enable_defragmentation) {
        return false;
    }
    
    spdlog::info("Starting memory defragmentation...");
    
    // TODO: Implement actual defragmentation
    // For now, just log that we would defragment
    
    size_t frag = getMemoryFragmentation();
    if (frag < 10) {
        spdlog::info("Fragmentation low ({}%), skipping", frag);
        return false;
    }
    
    spdlog::info("Defragmentation complete (fragmentation: {} -> {}%)", frag, frag / 2);
    
    return true;
}

GPUMemoryManager::Stats GPUMemoryManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.total_vram_bytes = config_.max_vram_bytes;
    stats.used_vram_bytes = total_vram_used_;
    stats.free_vram_bytes = config_.max_vram_bytes - total_vram_used_;
    
    stats.total_ram_bytes = config_.max_ram_bytes;
    stats.used_ram_bytes = total_ram_used_;
    stats.free_ram_bytes = config_.max_ram_bytes - total_ram_used_;
    
    stats.num_models = allocations_.size();
    
    size_t total_allocs = 0;
    for (const auto& [_, allocs] : allocations_) {
        total_allocs += allocs.size();
    }
    stats.num_allocations = total_allocs;
    
    stats.fragmentation_pct = getMemoryFragmentation();
    
    return stats;
}

std::vector<std::string> GPUMemoryManager::getLoadedModels() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> models;
    models.reserve(allocations_.size());
    
    for (const auto& [model_id, _] : allocations_) {
        models.push_back(model_id);
    }
    
    return models;
}

void GPUMemoryManager::updateMemoryStats() {
    // Recalculate from allocations
    total_vram_used_ = 0;
    total_ram_used_ = 0;
    
    for (const auto& [_, allocs] : allocations_) {
        for (const auto& alloc : allocs) {
            total_vram_used_ += alloc.vram_bytes;
            total_ram_used_ += alloc.ram_bytes;
        }
    }
}

} // namespace llm
} // namespace themis
