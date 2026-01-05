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
    
    // Initialize multi-GPU support (v1.4.0)
    if (config_.enable_multi_gpu && !config_.gpu_devices.empty()) {
        spdlog::info("Initializing multi-GPU support with {} GPUs", config_.gpu_devices.size());
        available_gpus_ = config_.gpu_devices;
        
        for (int gpu_id : config_.gpu_devices) {
            per_gpu_vram_used_[gpu_id] = 0;
            gpu_health_status_[gpu_id] = true;  // Assume healthy initially
            spdlog::info("  GPU {} initialized", gpu_id);
        }
        
        // Enable peer access if requested
        if (config_.enable_peer_access) {
            spdlog::info("Enabling CUDA peer-to-peer access between GPUs");
            // TODO: Enable P2P with CUDA
            // for (size_t i = 0; i < available_gpus_.size(); ++i) {
            //     for (size_t j = i+1; j < available_gpus_.size(); ++j) {
            //         enablePeerAccess(available_gpus_[i], available_gpus_[j]);
            //     }
            // }
        }
    } else {
        // Single GPU mode
        available_gpus_.push_back(gpu_device_id_);
        per_gpu_vram_used_[gpu_device_id_] = 0;
        gpu_health_status_[gpu_device_id_] = true;
    }
    
    spdlog::info("GPU Memory Manager: Running in simulation mode (actual GPU support in CUDA build)");
    spdlog::info("  Available GPUs: {}", available_gpus_.size());
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
    
    // Reset per-GPU counters
    for (auto& [gpu_id, _] : per_gpu_vram_used_) {
        per_gpu_vram_used_[gpu_id] = 0;
    }
    
    for (const auto& [_, allocs] : allocations_) {
        for (const auto& alloc : allocs) {
            total_vram_used_ += alloc.vram_bytes;
            total_ram_used_ += alloc.ram_bytes;
            
            // Track per-GPU usage - ensure GPU ID exists in map
            if (alloc.vram_bytes > 0) {
                // Initialize if not present (defensive programming)
                if (per_gpu_vram_used_.find(alloc.gpu_device_id) == per_gpu_vram_used_.end()) {
                    per_gpu_vram_used_[alloc.gpu_device_id] = 0;
                }
                per_gpu_vram_used_[alloc.gpu_device_id] += alloc.vram_bytes;
            }
        }
    }
}

// Multi-GPU methods (v1.4.0)

void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes, int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify GPU is available
    if (!isGPUAvailable(gpu_device_id)) {
        spdlog::error("GPU {} is not available", gpu_device_id);
        return nullptr;
    }
    
    // Check per-GPU capacity
    size_t gpu_used = per_gpu_vram_used_[gpu_device_id];
    if (gpu_used + bytes > config_.max_vram_bytes) {
        spdlog::error("Cannot allocate {} bytes on GPU {}: insufficient memory (used: {}, max: {})", 
                      bytes, gpu_device_id, gpu_used, config_.max_vram_bytes);
        return nullptr;
    }
    
    void* ptr = nullptr;
    
    // TODO: When CUDA is available:
    // cudaSetDevice(gpu_device_id);
    // cudaError_t err = cudaMalloc(&ptr, bytes);
    // if (err != cudaSuccess) {
    //     spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
    //     return nullptr;
    // }
    
    // Placeholder: use regular malloc for simulation
    ptr = std::malloc(bytes);
    if (!ptr) {
        spdlog::error("Failed to allocate {} bytes on GPU {} for model {}", bytes, gpu_device_id, model_id);
        return nullptr;
    }
    
    // Track allocation
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.vram_bytes = bytes;
    alloc.gpu_ptr = ptr;
    alloc.gpu_device_id = gpu_device_id;
    
    allocations_[model_id].push_back(alloc);
    total_vram_used_ += bytes;
    per_gpu_vram_used_[gpu_device_id] += bytes;
    
    spdlog::debug("Allocated {} MB on GPU {} for model {} (GPU total: {} MB)", 
                  bytes / (1024.0 * 1024),
                  gpu_device_id,
                  model_id,
                  per_gpu_vram_used_[gpu_device_id] / (1024.0 * 1024));
    
    return ptr;
}

bool GPUMemoryManager::freeModel(const std::string& model_id, int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    size_t freed_vram = 0;
    size_t freed_ram = 0;
    
    // Free only allocations on specified GPU
    auto& allocs = it->second;
    auto alloc_it = allocs.begin();
    while (alloc_it != allocs.end()) {
        // Match allocations on this GPU or CPU-only allocations (vram_bytes == 0)
        bool is_target_gpu = (alloc_it->gpu_device_id == gpu_device_id);
        bool is_cpu_only = (alloc_it->vram_bytes == 0);
        
        if (is_target_gpu || is_cpu_only) {
            if (alloc_it->gpu_ptr && is_target_gpu) {
                // TODO: cudaSetDevice(gpu_device_id); cudaFree(alloc_it->gpu_ptr);
                std::free(alloc_it->gpu_ptr);
                freed_vram += alloc_it->vram_bytes;
                if (per_gpu_vram_used_.find(gpu_device_id) != per_gpu_vram_used_.end()) {
                    per_gpu_vram_used_[gpu_device_id] -= alloc_it->vram_bytes;
                }
            }
            if (alloc_it->cpu_ptr) {
                if (alloc_it->is_pinned) {
                    // TODO: cudaFreeHost(alloc_it->cpu_ptr);
                    std::free(alloc_it->cpu_ptr);
                } else {
                    std::free(alloc_it->cpu_ptr);
                }
                freed_ram += alloc_it->ram_bytes;
            }
            alloc_it = allocs.erase(alloc_it);
        } else {
            ++alloc_it;
        }
    }
    
    total_vram_used_ -= freed_vram;
    total_ram_used_ -= freed_ram;
    
    if (allocs.empty()) {
        allocations_.erase(it);
    }
    
    spdlog::info("Freed memory for model {} on GPU {}: {} MB VRAM, {} MB RAM",
                 model_id, gpu_device_id,
                 freed_vram / (1024.0 * 1024),
                 freed_ram / (1024.0 * 1024));
    
    return true;
}

size_t GPUMemoryManager::getGPUVRAM(int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    return it != per_gpu_vram_used_.end() ? it->second : 0;
}

size_t GPUMemoryManager::getFreeGPUVRAM(int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t used = getGPUVRAM(gpu_device_id);
    return used < config_.max_vram_bytes ? (config_.max_vram_bytes - used) : 0;
}

std::vector<int> GPUMemoryManager::getAvailableGPUs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return available_gpus_;
}

bool GPUMemoryManager::isGPUAvailable(int gpu_device_id) const {
    // Already locked by caller in most cases, but safe to lock again
    auto it = gpu_health_status_.find(gpu_device_id);
    if (it == gpu_health_status_.end()) {
        return false;
    }
    return it->second;
}

bool GPUMemoryManager::enablePeerAccess(int src_gpu, int dst_gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isGPUAvailable(src_gpu) || !isGPUAvailable(dst_gpu)) {
        spdlog::error("Cannot enable peer access: GPU {} or {} not available", src_gpu, dst_gpu);
        return false;
    }
    
    // TODO: When CUDA is available:
    // cudaSetDevice(src_gpu);
    // cudaError_t err = cudaDeviceEnablePeerAccess(dst_gpu, 0);
    // if (err != cudaSuccess && err != cudaErrorPeerAccessAlreadyEnabled) {
    //     spdlog::error("Failed to enable peer access from GPU {} to {}: {}", 
    //                   src_gpu, dst_gpu, cudaGetErrorString(err));
    //     return false;
    // }
    
    spdlog::info("Peer access enabled: GPU {} -> GPU {} (simulated)", src_gpu, dst_gpu);
    return true;
}

bool GPUMemoryManager::disablePeerAccess(int src_gpu, int dst_gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isGPUAvailable(src_gpu) || !isGPUAvailable(dst_gpu)) {
        return false;
    }
    
    // TODO: When CUDA is available:
    // cudaSetDevice(src_gpu);
    // cudaDeviceDisablePeerAccess(dst_gpu);
    
    spdlog::info("Peer access disabled: GPU {} -> GPU {} (simulated)", src_gpu, dst_gpu);
    return true;
}

bool GPUMemoryManager::canAccessPeer(int src_gpu, int dst_gpu) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enable_peer_access) {
        return false;
    }
    
    if (!isGPUAvailable(src_gpu) || !isGPUAvailable(dst_gpu)) {
        return false;
    }
    
    // TODO: When CUDA is available:
    // int can_access = 0;
    // cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
    // return can_access != 0;
    
    // Simulation: assume all GPUs can access each other if peer access is enabled
    return true;
}

} // namespace llm
} // namespace themis
