// THEMIS_GAP_STATS: gaps=41 unimpl=3 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_memory_manager.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   48.0/100                                       ║
    • Total Lines:     1710                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • f38c013cdc  2026-03-29  Enhance various components with improvements and fixes ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/gpu_memory_manager.h"
#include <stdexcept>
#include "utils/error_registry.h"
#include "security/vram_secure_clear.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include <unordered_set>

// Include actual CUDA headers when CUDA support is built
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            spdlog::error("CUDA error at {}:{} - {}", __FILE__, __LINE__, cudaGetErrorString(err)); \
        } \
    } while(0)

#define CUDA_CHECK_RETURN(call, retval) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            spdlog::error("CUDA error at {}:{} - {}", __FILE__, __LINE__, cudaGetErrorString(err)); \
            return retval; \
        } \
    } while(0)
#endif

namespace themis {
namespace llm {

// Internal RAII helper for memory management
namespace detail {

/**
 * @brief RAII holder for GPU/CPU memory allocations
 * 
 * Provides exception-safe memory management by automatically cleaning up
 * memory in the destructor. Uses secure clearing before deallocation.
 * 
 * Note: This class is non-movable and non-copyable to prevent accidental
 * ownership transfer. Memory is always owned by a single holder instance
 * managed via shared_ptr in MemoryAllocation.
 */
class MemoryHolder {
public:
    enum class Type {
        GPU,           // CUDA device memory
        CPU,           // Regular CPU memory
        PINNED         // CUDA pinned host memory
    };
    
    /**
     * @param ptr Pointer to memory to manage
     * @param bytes Size in bytes
     * @param type Type of memory (GPU/CPU/PINNED)
     * @param gpu_available Whether GPU is available
     * @param gpu_device_id GPU device ID (default 0). Only used for GPU memory
     *                      cleanup. CPU and PINNED allocations ignore this parameter.
     */
    MemoryHolder(void* ptr, size_t bytes, Type type, bool gpu_available, int gpu_device_id = 0)
        : ptr_(ptr), bytes_(bytes), type_(type), gpu_available_(gpu_available), 
          gpu_device_id_(gpu_device_id) {}
    
    ~MemoryHolder() {
        if (!ptr_) return;
        
        try {
            switch (type_) {
                case Type::GPU:
                    freeGPUMemory();
                    break;
                case Type::PINNED:
                    freePinnedMemory();
                    break;
                case Type::CPU:
                    freeCPUMemory();
                    break;
            }
        } catch (const std::exception& e) {
            spdlog::error("Exception during memory cleanup: {}", e.what());
        } catch (const std::exception&) {
            spdlog::error("Unknown exception during memory cleanup");
        }
    }
    
    // Prevent copying and moving - memory is owned by single holder
    MemoryHolder(const MemoryHolder&) = delete;
    MemoryHolder& operator=(const MemoryHolder&) = delete;
    MemoryHolder(MemoryHolder&&) = delete;
    MemoryHolder& operator=(MemoryHolder&&) = delete;
    
    void* get() const noexcept { return ptr_; }
    size_t size() const noexcept { return bytes_; }

private:
    void freeGPUMemory() {
#ifdef THEMIS_ENABLE_CUDA
        if (gpu_available_) {
            cudaSetDevice(gpu_device_id_);
            security::VRAMSecureClear::secureClearCUDA(ptr_, bytes_);
            CUDA_CHECK(cudaFree(ptr_));
        } else {
            security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
            std::free(ptr_);
        }
#else
        security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
        std::free(ptr_);
#endif
    }
    
    void freePinnedMemory() {
#ifdef THEMIS_ENABLE_CUDA
        if (gpu_available_) {
            security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
            CUDA_CHECK(cudaFreeHost(ptr_));
        } else {
            security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
            std::free(ptr_);
        }
#else
        security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
        std::free(ptr_);
#endif
    }
    
    void freeCPUMemory() {
        security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
        std::free(ptr_);
    }
    
    void* ptr_;
    size_t bytes_;
    Type type_;
    bool gpu_available_;
    int gpu_device_id_;
};

} // namespace detail

namespace {
// Helper: compute GPU utilization ratio in [0, 1].  Returns 0 when the
// configured VRAM capacity is 0 to avoid division by zero.
inline float calculateUtilization(size_t used_vram, size_t max_vram_bytes) noexcept {
    return (max_vram_bytes > 0)
        ? static_cast<float>(used_vram) / max_vram_bytes
        : 0.0f;
}
} // namespace

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
    
    // Free all allocations - holders will automatically clean up memory via RAII
    // Just log what we're cleaning up
    for (const auto& [model_id, allocs] : allocations_) {
        spdlog::info("Cleaning up memory for model: {} ({} allocations)", 
                     model_id, allocs.size());
    }
    
    // Clear allocations - this will trigger holder destructors
    allocations_.clear();
    
    shutdownGPU();
}

void GPUMemoryManager::initializeGPU() {
#ifdef THEMIS_ENABLE_CUDA
    // Try to detect and initialize CUDA GPU
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    
    if (err == cudaSuccess && deviceCount > 0) {
        gpu_available_ = true;
        
        // Get current device or use device 0
        err = cudaGetDevice(&gpu_device_id_);
        if (err != cudaSuccess) {
            gpu_device_id_ = 0;
            CUDA_CHECK(cudaSetDevice(gpu_device_id_));
        }
        
        // Query GPU properties
        cudaDeviceProp prop;
        if (cudaGetDeviceProperties(&prop, gpu_device_id_) == cudaSuccess) {
            spdlog::info("GPU detected: {} (Compute {}.{})", 
                         prop.name, prop.major, prop.minor);
            spdlog::info("  Total VRAM: {:.2f} GB", prop.totalGlobalMem / (1024.0*1024*1024));
            spdlog::info("  Multiprocessors: {}", prop.multiProcessorCount);

            // Auto-detect VRAM limit from device properties (0 = auto-detect)
            if (config_.max_vram_bytes == 0) {
                config_.max_vram_bytes = prop.totalGlobalMem;
            }
        }
        
        // Initialize multi-GPU support (v1.4.0)
        if (config_.enable_multi_gpu && !config_.gpu_devices.empty()) {
            spdlog::info("Initializing multi-GPU support with {} GPUs", config_.gpu_devices.size());
            available_gpus_ = config_.gpu_devices;
            
            for (int gpu_id : config_.gpu_devices) {
                // Check if GPU exists
                if (gpu_id >= deviceCount) {
                    spdlog::warn("GPU {} requested but only {} GPUs available, skipping", gpu_id, deviceCount);
                    continue;
                }
                
                per_gpu_vram_used_[gpu_id] = 0;
                gpu_health_status_[gpu_id] = true;
                spdlog::info("  GPU {} initialized", gpu_id);
            }
            
            // Enable peer access if requested
            if (config_.enable_peer_access) {
                spdlog::info("Enabling CUDA peer-to-peer access between GPUs");
                for (size_t i = 0; i < available_gpus_.size(); ++i) {
                    for (size_t j = i+1; j < available_gpus_.size(); ++j) {
                        int src_gpu = available_gpus_[i];
                        int dst_gpu = available_gpus_[j];
                        
                        // Check if peer access is possible
                        int can_access = 0;
                        cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
                        if (can_access) {
                            cudaSetDevice(src_gpu);
                            cudaError_t p2p_err = cudaDeviceEnablePeerAccess(dst_gpu, 0);
                            if (p2p_err == cudaSuccess) {
                                spdlog::info("  P2P enabled: GPU {} -> GPU {}", src_gpu, dst_gpu);
                            } else if (p2p_err != cudaErrorPeerAccessAlreadyEnabled) {
                                spdlog::warn("  P2P failed: GPU {} -> GPU {}: {}", 
                                           src_gpu, dst_gpu, cudaGetErrorString(p2p_err));
                            }
                        }
                    }
                }
            }
        } else {
            // Single GPU mode
            available_gpus_.push_back(gpu_device_id_);
            per_gpu_vram_used_[gpu_device_id_] = 0;
            gpu_health_status_[gpu_device_id_] = true;
        }
        
        spdlog::info("GPU Memory Manager: Running with real CUDA support");
        spdlog::info("  Available GPUs: {}", available_gpus_.size());
    } else {
        gpu_available_ = false;
        spdlog::warn("No GPU detected: {}", cudaGetErrorString(err));
        spdlog::warn("Running in CPU-only mode (simulation)");
        
        // Fallback to simulation mode
        gpu_device_id_ = 0;
        available_gpus_.push_back(gpu_device_id_);
        per_gpu_vram_used_[gpu_device_id_] = 0;
        gpu_health_status_[gpu_device_id_] = true;
    }
#else
    // Simulation mode when CUDA is not enabled at build time
    gpu_available_ = false;
    gpu_device_id_ = 0;
    
    // Initialize multi-GPU support in simulation mode (v1.4.0)
    if (config_.enable_multi_gpu && !config_.gpu_devices.empty()) {
        spdlog::info("Initializing multi-GPU support (simulation) with {} GPUs", config_.gpu_devices.size());
        available_gpus_ = config_.gpu_devices;
        
        for (int gpu_id : config_.gpu_devices) {
            per_gpu_vram_used_[gpu_id] = 0;
            gpu_health_status_[gpu_id] = true;
            spdlog::info("  GPU {} initialized (simulated)", gpu_id);
        }
        
        if (config_.enable_peer_access) {
            spdlog::info("P2P access enabled (simulated)");
        }
    } else {
        // Single GPU mode
        available_gpus_.push_back(gpu_device_id_);
        per_gpu_vram_used_[gpu_device_id_] = 0;
        gpu_health_status_[gpu_device_id_] = true;
    }
    
    spdlog::info("GPU Memory Manager: Running in simulation mode (CUDA not enabled at build time)");
    spdlog::info("  Available GPUs: {} (simulated)", available_gpus_.size());
#endif

    // Apply VRAM limit fallback: if max_vram_bytes is still 0 after platform-specific
    // initialization (e.g. no GPU available or cudaGetDeviceProperties failed), use a
    // sensible simulation default so that canAllocate() and getLeastLoadedGPU() work.
    if (config_.max_vram_bytes == 0) {
        config_.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;  // 8 GB simulation default
        spdlog::info("  VRAM limit defaulted to {:.2f} GB (simulation)",
                     config_.max_vram_bytes / (1024.0 * 1024 * 1024));
    }
}

void GPUMemoryManager::shutdownGPU() {
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Disable peer access if it was enabled
        if (config_.enable_peer_access && available_gpus_.size() > 1) {
            for (size_t i = 0; i < available_gpus_.size(); ++i) {
                int src_gpu = available_gpus_[i];
                cudaSetDevice(src_gpu);
                for (size_t j = 0; j < available_gpus_.size(); ++j) {
                    if (i != j) {
                        int dst_gpu = available_gpus_[j];
                        cudaDeviceDisablePeerAccess(dst_gpu);
                    }
                }
            }
        }
        
        // Reset all devices
        for (int gpu_id : available_gpus_) {
            cudaSetDevice(gpu_id);
            CUDA_CHECK(cudaDeviceReset());
        }
    }
#endif
}

void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!canAllocate(bytes, 0)) {
        double bytes_mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        double available_mb = static_cast<double>(config_.max_vram_bytes - total_vram_used_) / (1024.0 * 1024.0);
        spdlog::error("[{}] GPU OOM: requested {:.1f} MB, available {:.1f} MB", 
                      static_cast<int>(errors::ErrorCode::ERR_LLM_GPU_OOM), 
                      bytes_mb, available_mb);
        return nullptr;
    }
    
    void* ptr = nullptr;
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Use actual CUDA allocation
        cudaError_t err = cudaMalloc(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::error("cudaMalloc failed: {}", cudaGetErrorString(err));
            return nullptr;
        }
    } else {
        // Fallback to simulation when CUDA is available but no GPU detected
        ptr = std::malloc(bytes);
        if (!ptr) {
            size_t bytes_mb = bytes / (1024 * 1024);
            errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
            return nullptr;
        }
    }
#else
    // Simulation mode: use regular malloc when CUDA is not enabled at build time
    ptr = std::malloc(bytes);
    if (!ptr) {
        size_t bytes_mb = bytes / (1024 * 1024);
        errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
        return nullptr;
    }
#endif
    
    // Track allocation with RAII holder for automatic cleanup
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.vram_bytes = bytes;
    alloc.gpu_ptr = ptr;
    alloc.gpu_device_id = 0;  // Default GPU device
    alloc.holder = std::make_shared<detail::MemoryHolder>(
        ptr, bytes, detail::MemoryHolder::Type::GPU, gpu_available_, 0  // Default GPU device
    );
    
    allocations_[model_id].push_back(std::move(alloc));
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
    
#ifdef THEMIS_ENABLE_CUDA
    if (pinned && gpu_available_) {
        // Use CUDA pinned memory for faster transfers
        cudaError_t err = cudaMallocHost(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc", 
                        cudaGetErrorString(err));
            pinned = false;
            ptr = std::malloc(bytes);
        }
    } else {
        ptr = std::malloc(bytes);
        pinned = false;
    }
#else
    // Simulation mode: always use regular malloc
    ptr = std::malloc(bytes);
    pinned = false;
#endif
    
    if (!ptr) {
        spdlog::error("Failed to allocate {} bytes RAM for model {}", bytes, model_id);
        return nullptr;
    }
    
    // Track allocation with RAII holder
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.ram_bytes = bytes;
    alloc.cpu_ptr = ptr;
    alloc.is_pinned = pinned;
    
    // Create holder with appropriate type
    auto holder_type = pinned ? detail::MemoryHolder::Type::PINNED 
                              : detail::MemoryHolder::Type::CPU;
    alloc.holder = std::make_shared<detail::MemoryHolder>(
        ptr, bytes, holder_type, gpu_available_
    );
    
    allocations_[model_id].push_back(std::move(alloc));
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
    
    // Find the allocation and remove it
    // The holder's destructor will automatically handle cleanup via RAII
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->gpu_ptr == ptr) {
            if (alloc_it->vram_bytes > total_vram_used_) {
                spdlog::error("GPUMemoryManager::freeGPU: VRAM accounting underflow for model '{}'; "
                              "clamping to 0", model_id);
                total_vram_used_ = 0;
            } else {
                total_vram_used_ -= alloc_it->vram_bytes;
            }
            
            // Erase triggers holder destructor for automatic cleanup
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
    
    // Find the allocation and remove it
    // The holder's destructor will automatically handle cleanup via RAII
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->cpu_ptr == ptr) {
            if (alloc_it->ram_bytes > total_ram_used_) {
                spdlog::error("GPUMemoryManager::freeCPU: RAM accounting underflow for model '{}'; "
                              "clamping to 0", model_id);
                total_ram_used_ = 0;
            } else {
                total_ram_used_ -= alloc_it->ram_bytes;
            }
            
            // Erase triggers holder destructor for automatic cleanup
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
    
    // Calculate total freed memory for logging
    size_t freed_vram = 0;
    size_t freed_ram = 0;
    
    for (const auto& alloc : it->second) {
        freed_vram += alloc.vram_bytes;
        freed_ram += alloc.ram_bytes;
    }
    
    if (freed_vram > total_vram_used_) {
        spdlog::error("GPUMemoryManager::freeModel: VRAM accounting underflow for model '{}'; "
                      "clamping to 0", model_id);
        total_vram_used_ = 0;
    } else {
        total_vram_used_ -= freed_vram;
    }
    if (freed_ram > total_ram_used_) {
        spdlog::error("GPUMemoryManager::freeModel: RAM accounting underflow for model '{}'; "
                      "clamping to 0", model_id);
        total_ram_used_ = 0;
    } else {
        total_ram_used_ -= freed_ram;
    }
    
    // Erase all allocations for this model
    // Holders will automatically clean up via RAII
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
    return config_.max_vram_bytes;
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
    size_t fragmentation_pct = std::min(static_cast<size_t>(100), static_cast<size_t>((excess_allocations * 100) / num_models));
    
    return fragmentation_pct;
}

bool GPUMemoryManager::defragment() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enable_defragmentation) {
        return false;
    }
    
    auto compute_fragmentation_unlocked = [this]() -> size_t {
        size_t total_allocations = 0;
        for (const auto& [_, allocs] : allocations_) {
            total_allocations += allocs.size();
        }

        const size_t num_models = allocations_.size();
        if (num_models == 0) {
            return 0;
        }

        const size_t excess_allocations =
            total_allocations > num_models ? total_allocations - num_models : 0;
        return std::min(static_cast<size_t>(100),
                        static_cast<size_t>((excess_allocations * 100) / num_models));
    };

    size_t initial_frag = compute_fragmentation_unlocked();
    if (initial_frag < 10) {
        spdlog::debug("Fragmentation low ({}%), skipping defragmentation", initial_frag);
        return false;
    }
    
    spdlog::info("Starting memory defragmentation (current fragmentation: {}%)...", initial_frag);
    
    size_t models_defragmented = 0;
    size_t allocations_consolidated = 0;
    
    // Iterate through each model and consolidate fragmented allocations
    for (auto& [model_id, allocs] : allocations_) {
        // Skip if model has only one allocation (not fragmented)
        if (allocs.size() <= 1) {
            continue;
        }
        
        // Separate GPU and CPU allocations for this model
        std::vector<MemoryAllocation> gpu_allocs;
        std::vector<MemoryAllocation> cpu_allocs;
        
        for (const auto& alloc : allocs) {
            if (alloc.vram_bytes > 0 && alloc.gpu_ptr) {
                gpu_allocs.push_back(alloc);
            }
            if (alloc.ram_bytes > 0 && alloc.cpu_ptr) {
                cpu_allocs.push_back(alloc);
            }
        }
        
        // Defragment GPU memory if there are multiple GPU allocations
        if (gpu_allocs.size() > 1) {
            if (defragmentModelGPU(model_id, gpu_allocs)) {
                allocations_consolidated += gpu_allocs.size() - 1;
            }
        }
        
        // Defragment CPU memory if there are multiple CPU allocations
        if (cpu_allocs.size() > 1) {
            if (defragmentModelCPU(model_id, cpu_allocs)) {
                allocations_consolidated += cpu_allocs.size() - 1;
            }
        }
        
        if (gpu_allocs.size() > 1 || cpu_allocs.size() > 1) {
            models_defragmented++;
        }
    }
    
    size_t final_frag = compute_fragmentation_unlocked();
    
    if (models_defragmented > 0) {
        spdlog::info("Defragmentation complete: {} models defragmented, {} allocations consolidated", 
                     models_defragmented, allocations_consolidated);
        spdlog::info("Fragmentation reduced: {}% -> {}%", initial_frag, final_frag);
        return true;
    } else {
        spdlog::debug("No fragmented models found, defragmentation skipped");
        return false;
    }
}

bool GPUMemoryManager::defragmentModelGPU(const std::string& model_id, 
                                          const std::vector<MemoryAllocation>& gpu_allocs) {
    // Group allocations by GPU device
    std::unordered_map<int, std::vector<MemoryAllocation>> per_device_allocs;
    for (const auto& alloc : gpu_allocs) {
        per_device_allocs[alloc.gpu_device_id].push_back(alloc);
    }

    // Defragment each device separately
    for (const auto& [device_id, device_allocs] : per_device_allocs) {
        if (device_allocs.size() <= 1) {
            continue;
        }

        size_t total_vram = 0;
        for (const auto& alloc : device_allocs) {
            total_vram += alloc.vram_bytes;
        }

        void* new_ptr = nullptr;

#ifdef THEMIS_ENABLE_CUDA
        if (gpu_available_) {
            cudaSetDevice(device_id);
            if (cudaMalloc(&new_ptr, total_vram) != cudaSuccess) {
                spdlog::warn("Failed to allocate consolidated GPU memory for model {} on device {}", model_id, device_id);
                continue;
            }

            size_t offset = 0;
            for (const auto& alloc : device_allocs) {
                cudaMemcpy(static_cast<char*>(new_ptr) + offset, alloc.gpu_ptr, alloc.vram_bytes, cudaMemcpyDeviceToDevice);
                offset += alloc.vram_bytes;
            }
        } else {
            new_ptr = std::malloc(total_vram);
            if (!new_ptr) {
                continue;
            }
            size_t offset = 0;
            for (const auto& alloc : device_allocs) {
                std::memcpy(static_cast<char*>(new_ptr) + offset, alloc.gpu_ptr, alloc.vram_bytes);
                offset += alloc.vram_bytes;
            }
        }
#else
        new_ptr = std::malloc(total_vram);
        if (!new_ptr) {
            continue;
        }
        size_t offset = 0;
        for (const auto& alloc : device_allocs) {
            std::memcpy(static_cast<char*>(new_ptr) + offset, alloc.gpu_ptr, alloc.vram_bytes);
            offset += alloc.vram_bytes;
        }
#endif

        // Update allocations list for this model/device
        // Remove only the allocations that were identified as fragmented (device_allocs),
        // not all allocations for this device_id.
        auto& model_allocs = allocations_[model_id];
        std::unordered_set<void*> ptrs_to_erase;
        for (const auto& alloc : device_allocs) {
            ptrs_to_erase.insert(alloc.gpu_ptr);
        }
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [&ptrs_to_erase](const MemoryAllocation& alloc) {
                    return ptrs_to_erase.count(alloc.gpu_ptr) > 0;
                }),
            model_allocs.end());

        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.vram_bytes = total_vram;
        consolidated.gpu_ptr = new_ptr;
        consolidated.gpu_device_id = device_id;
        consolidated.holder = std::make_shared<detail::MemoryHolder>(
            new_ptr, total_vram, detail::MemoryHolder::Type::GPU, gpu_available_, device_id
        );
        model_allocs.push_back(std::move(consolidated));

        spdlog::debug("Consolidated {} GPU allocations for model {} on device {} into single {} MB block",
                      device_allocs.size(), model_id, device_id, total_vram / (1024.0 * 1024));
    }

    return true;
}

bool GPUMemoryManager::defragmentModelCPU(const std::string& model_id, 
                                          const std::vector<MemoryAllocation>& cpu_allocs) {
    if (cpu_allocs.size() <= 1) {
        return false;
    }
    
    // Separate pinned and non-pinned allocations
    std::vector<MemoryAllocation> pinned_allocs;
    std::vector<MemoryAllocation> regular_allocs;
    
    for (const auto& alloc : cpu_allocs) {
        if (alloc.is_pinned) {
            pinned_allocs.push_back(alloc);
        } else {
            regular_allocs.push_back(alloc);
        }
    }
    
    // Consolidate pinned allocations
    if (pinned_allocs.size() > 1) {
        size_t total_ram = 0;
        for (const auto& alloc : pinned_allocs) {
            total_ram += alloc.ram_bytes;
        }
        
        void* new_ptr = nullptr;
        
#ifdef THEMIS_ENABLE_CUDA
        if (gpu_available_) {
            cudaError_t err = cudaMallocHost(&new_ptr, total_ram);
            if (err != cudaSuccess) {
                spdlog::warn("Failed to allocate consolidated pinned memory for model {}: {}", 
                           model_id, cudaGetErrorString(err));
                // Fall back to regular malloc
                new_ptr = std::malloc(total_ram);
            }
        } else {
            new_ptr = std::malloc(total_ram);
        }
#else
        new_ptr = std::malloc(total_ram);
#endif
        
        if (!new_ptr) {
            return false;
        }
        
        // Copy data
        size_t offset = 0;
        for (const auto& alloc : pinned_allocs) {
            std::memcpy(static_cast<char*>(new_ptr) + offset, 
                       alloc.cpu_ptr, 
                       alloc.ram_bytes);
            offset += alloc.ram_bytes;
        }
        
        // Update allocations - removing old allocations will trigger cleanup via RAII
        auto& model_allocs = allocations_[model_id];
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [](const MemoryAllocation& alloc) {
                    return alloc.is_pinned && alloc.ram_bytes > 0;
                }),
            model_allocs.end()
        );
        
        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.ram_bytes = total_ram;
        consolidated.cpu_ptr = new_ptr;
        consolidated.is_pinned = true;
        consolidated.holder = std::make_shared<detail::MemoryHolder>(
            new_ptr, total_ram, detail::MemoryHolder::Type::PINNED, gpu_available_
        );
        model_allocs.push_back(std::move(consolidated));
    }
    
    // Consolidate regular allocations
    if (regular_allocs.size() > 1) {
        size_t total_ram = 0;
        for (const auto& alloc : regular_allocs) {
            total_ram += alloc.ram_bytes;
        }
        
        void* new_ptr = std::malloc(total_ram);
        if (!new_ptr) {
            return false;
        }
        
        // Copy data
        size_t offset = 0;
        for (const auto& alloc : regular_allocs) {
            std::memcpy(static_cast<char*>(new_ptr) + offset, 
                       alloc.cpu_ptr, 
                       alloc.ram_bytes);
            offset += alloc.ram_bytes;
        }
        
        // Update allocations - removing old allocations will trigger cleanup via RAII
        auto& model_allocs = allocations_[model_id];
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [](const MemoryAllocation& alloc) {
                    return !alloc.is_pinned && alloc.ram_bytes > 0;
                }),
            model_allocs.end()
        );
        
        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.ram_bytes = total_ram;
        consolidated.cpu_ptr = new_ptr;
        consolidated.is_pinned = false;
        consolidated.holder = std::make_shared<detail::MemoryHolder>(
            new_ptr, total_ram, detail::MemoryHolder::Type::CPU, gpu_available_
        );
        model_allocs.push_back(std::move(consolidated));
    }
    
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
            
            // Track per-GPU usage (operator[] auto-initializes to 0)
            if (alloc.vram_bytes > 0) {
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
        size_t bytes_mb = bytes / (1024 * 1024);
        size_t available_mb = (config_.max_vram_bytes - gpu_used) / (1024 * 1024);
        errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, available_mb);
        return nullptr;
    }
    
    void* ptr = nullptr;
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Set the target GPU device
        CUDA_CHECK_RETURN(cudaSetDevice(gpu_device_id), nullptr);
        
        // Use actual CUDA allocation
        cudaError_t err = cudaMalloc(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
            return nullptr;
        }
    } else {
        // Fallback to simulation
        ptr = std::malloc(bytes);
        if (!ptr) {
            size_t bytes_mb = bytes / (1024 * 1024);
            errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
            return nullptr;
        }
    }
#else
    // Simulation mode: use regular malloc when CUDA is not enabled at build time
    ptr = std::malloc(bytes);
    if (!ptr) {
        size_t bytes_mb = bytes / (1024 * 1024);
        errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
        return nullptr;
    }
#endif
    
    // Track allocation with RAII holder
    MemoryAllocation alloc;
    alloc.model_id = model_id;
    alloc.vram_bytes = bytes;
    alloc.gpu_ptr = ptr;
    alloc.gpu_device_id = gpu_device_id;
    alloc.holder = std::make_shared<detail::MemoryHolder>(
        ptr, bytes, detail::MemoryHolder::Type::GPU, gpu_available_, gpu_device_id
    );
    
    allocations_[model_id].push_back(std::move(alloc));
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
    // Holders will automatically clean up via RAII
    auto& allocs = it->second;
    auto alloc_it = allocs.begin();
    while (alloc_it != allocs.end()) {
        // Match allocations on this GPU or CPU-only allocations (vram_bytes == 0)
        bool is_target_gpu = (alloc_it->gpu_device_id == gpu_device_id);
        bool is_cpu_only = (alloc_it->vram_bytes == 0);
        
        if (is_target_gpu || is_cpu_only) {
            freed_vram += alloc_it->vram_bytes;
            freed_ram += alloc_it->ram_bytes;
            
            // Update per-GPU tracking for this specific allocation
            if (is_target_gpu && alloc_it->vram_bytes > 0) {
                if (per_gpu_vram_used_.find(alloc_it->gpu_device_id) != per_gpu_vram_used_.end()) {
                    per_gpu_vram_used_[alloc_it->gpu_device_id] -= alloc_it->vram_bytes;
                }
            }
            
            // Erase triggers holder destructor for automatic cleanup
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
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Check if peer access is possible
        int can_access = 0;
        CUDA_CHECK_RETURN(cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu), false);
        
        if (!can_access) {
            spdlog::warn("Peer access not supported between GPU {} and GPU {}", src_gpu, dst_gpu);
            return false;
        }
        
        // Enable peer access
        CUDA_CHECK_RETURN(cudaSetDevice(src_gpu), false);
        cudaError_t err = cudaDeviceEnablePeerAccess(dst_gpu, 0);
        if (err != cudaSuccess && err != cudaErrorPeerAccessAlreadyEnabled) {
            spdlog::error("Failed to enable peer access from GPU {} to {}: {}", 
                          src_gpu, dst_gpu, cudaGetErrorString(err));
            return false;
        }
        
        spdlog::info("Peer access enabled: GPU {} -> GPU {}", src_gpu, dst_gpu);
        return true;
    }
#endif
    
    // Simulation mode
    spdlog::info("Peer access enabled: GPU {} -> GPU {} (simulated)", src_gpu, dst_gpu);
    return true;
}

bool GPUMemoryManager::disablePeerAccess(int src_gpu, int dst_gpu) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isGPUAvailable(src_gpu) || !isGPUAvailable(dst_gpu)) {
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        CUDA_CHECK_RETURN(cudaSetDevice(src_gpu), false);
        cudaError_t err = cudaDeviceDisablePeerAccess(dst_gpu);
        if (err != cudaSuccess && err != cudaErrorPeerAccessNotEnabled) {
            spdlog::warn("Failed to disable peer access from GPU {} to {}: {}", 
                        src_gpu, dst_gpu, cudaGetErrorString(err));
            return false;
        }
        
        spdlog::info("Peer access disabled: GPU {} -> GPU {}", src_gpu, dst_gpu);
        return true;
    }
#endif
    
    // Simulation mode
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
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        int can_access = 0;
        cudaError_t err = cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
        if (err != cudaSuccess) {
            return false;
        }
        return can_access != 0;
    }
#endif
    
    // Simulation: assume all GPUs can access each other if peer access is enabled
    return true;
}

// GPU Health Monitoring Implementation

GPUMemoryManager::GPUStats GPUMemoryManager::getGPUStats(int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GPUStats stats = {};
    stats.device_id = gpu_device_id;
    
    if (!isGPUAvailable(gpu_device_id)) {
        return stats;
    }
    
    // Get VRAM stats
    stats.total_vram_bytes = config_.max_vram_bytes;
    stats.used_vram_bytes = 0;
    
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    if (it != per_gpu_vram_used_.end()) {
        stats.used_vram_bytes = it->second;
    }
    
    stats.free_vram_bytes = stats.total_vram_bytes - stats.used_vram_bytes;
    
    // Count allocations on this GPU
    stats.num_allocations = 0;
    for (const auto& [model_id, allocs] : allocations_) {
        for (const auto& alloc : allocs) {
            if (alloc.gpu_device_id == gpu_device_id && alloc.gpu_ptr != nullptr) {
                stats.num_allocations++;
                
                // Track loaded models and adapters
                if (model_id.find("adapter_") != std::string::npos) {
                    stats.loaded_adapters.push_back(model_id);
                } else {
                    stats.loaded_models.push_back(model_id);
                }
            }
        }
    }
    
    // Get utilization
    auto util_it = gpu_utilizations_.find(gpu_device_id);
    if (util_it != gpu_utilizations_.end()) {
        stats.utilization_percent = util_it->second;
    } else {
        stats.utilization_percent = (stats.used_vram_bytes * 100.0f) / stats.total_vram_bytes;
    }
    
    // Get temperature
    auto temp_it = gpu_temperatures_.find(gpu_device_id);
    if (temp_it != gpu_temperatures_.end()) {
        stats.temperature_celsius = temp_it->second;
    }
    
    // Get health status
    auto health_it = gpu_health_status_.find(gpu_device_id);
    stats.is_healthy = (health_it != gpu_health_status_.end()) ? health_it->second : true;
    
    return stats;
}

std::vector<GPUMemoryManager::GPUStats> GPUMemoryManager::getAllGPUStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GPUStats> all_stats;
    for (int gpu_id : available_gpus_) {
        // Get stats inline to avoid deadlock (mutex already locked)
        GPUStats stats = {};
        stats.device_id = gpu_id;
        
        if (isGPUAvailable(gpu_id)) {
            // Get VRAM stats
            stats.total_vram_bytes = config_.max_vram_bytes;
            stats.used_vram_bytes = 0;
            
            auto it = per_gpu_vram_used_.find(gpu_id);
            if (it != per_gpu_vram_used_.end()) {
                stats.used_vram_bytes = it->second;
            }
            
            stats.free_vram_bytes = stats.total_vram_bytes - stats.used_vram_bytes;
            
            // Count allocations on this GPU
            stats.num_allocations = 0;
            for (const auto& [model_id, allocs] : allocations_) {
                for (const auto& alloc : allocs) {
                    if (alloc.gpu_device_id == gpu_id && alloc.gpu_ptr != nullptr) {
                        stats.num_allocations++;
                        
                        // Track loaded models and adapters
                        if (model_id.find("adapter_") != std::string::npos) {
                            stats.loaded_adapters.push_back(model_id);
                        } else {
                            stats.loaded_models.push_back(model_id);
                        }
                    }
                }
            }
            
            // Get utilization
            auto util_it = gpu_utilizations_.find(gpu_id);
            if (util_it != gpu_utilizations_.end()) {
                stats.utilization_percent = util_it->second;
            } else {
                stats.utilization_percent = (stats.used_vram_bytes * 100.0f) / stats.total_vram_bytes;
            }
            
            // Get temperature
            auto temp_it = gpu_temperatures_.find(gpu_id);
            if (temp_it != gpu_temperatures_.end()) {
                stats.temperature_celsius = temp_it->second;
            }
            
            // Get health status
            auto health_it = gpu_health_status_.find(gpu_id);
            stats.is_healthy = (health_it != gpu_health_status_.end()) ? health_it->second : true;
        }
        
        all_stats.push_back(stats);
    }
    
    return all_stats;
}

GPUMemoryManager::GPUHealth GPUMemoryManager::getGPUHealth(int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GPUHealth health = {};
    health.device_id = gpu_device_id;
    health.is_available = isGPUAvailable(gpu_device_id);
    
    if (!health.is_available) {
        return health;
    }
    
    // Check if we have stored health data
    auto it = gpu_health_data_.find(gpu_device_id);
    if (it != gpu_health_data_.end()) {
        return it->second;
    }
    
    // Generate default health data
    auto health_it = gpu_health_status_.find(gpu_device_id);
    health.is_healthy = (health_it != gpu_health_status_.end()) ? health_it->second : true;
    
    auto temp_it = gpu_temperatures_.find(gpu_device_id);
    health.temperature_celsius = (temp_it != gpu_temperatures_.end()) ? temp_it->second : 0.0f;
    
    auto util_it = gpu_utilizations_.find(gpu_device_id);
    health.utilization_percent = (util_it != gpu_utilizations_.end()) ? util_it->second : 0.0f;
    
    auto err_it = gpu_error_counts_.find(gpu_device_id);
    health.error_count = (err_it != gpu_error_counts_.end()) ? err_it->second : 0;
    
    health.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return health;
}

std::vector<GPUMemoryManager::GPUHealth> GPUMemoryManager::getAllGPUHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<GPUHealth> all_health;
    for (int gpu_id : available_gpus_) {
        // Get health inline to avoid deadlock (mutex already locked)
        GPUHealth health = {};
        health.device_id = gpu_id;
        health.is_available = isGPUAvailable(gpu_id);
        
        if (health.is_available) {
            // Check if we have stored health data
            auto it = gpu_health_data_.find(gpu_id);
            if (it != gpu_health_data_.end()) {
                health = it->second;
            } else {
                // Generate default health data
                auto health_it = gpu_health_status_.find(gpu_id);
                health.is_healthy = (health_it != gpu_health_status_.end()) ? health_it->second : true;
                
                auto temp_it = gpu_temperatures_.find(gpu_id);
                health.temperature_celsius = (temp_it != gpu_temperatures_.end()) ? temp_it->second : 0.0f;
                
                auto util_it = gpu_utilizations_.find(gpu_id);
                health.utilization_percent = (util_it != gpu_utilizations_.end()) ? util_it->second : 0.0f;
                
                auto err_it = gpu_error_counts_.find(gpu_id);
                health.error_count = (err_it != gpu_error_counts_.end()) ? err_it->second : 0;
                
                health.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
            }
        }
        
        all_health.push_back(health);
    }
    
    return all_health;
}

bool GPUMemoryManager::isGPUHealthy(int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = gpu_health_status_.find(gpu_device_id);
    return (it != gpu_health_status_.end()) ? it->second : false;
}

void GPUMemoryManager::markGPUUnhealthy(int gpu_device_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::warn("GPU {} marked as unhealthy: {}", gpu_device_id, reason);
    gpu_health_status_[gpu_device_id] = false;
    
    // Update health data
    GPUHealth health = {};
    health.device_id = gpu_device_id;
    health.is_available = isGPUAvailable(gpu_device_id);
    health.is_healthy = false;
    health.last_error = reason;
    
    auto err_it = gpu_error_counts_.find(gpu_device_id);
    if (err_it != gpu_error_counts_.end()) {
        health.error_count = ++err_it->second;
    } else {
        gpu_error_counts_[gpu_device_id] = 1;
        health.error_count = 1;
    }
    
    health.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    gpu_health_data_[gpu_device_id] = health;
}

void GPUMemoryManager::markGPUHealthy(int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    spdlog::info("GPU {} marked as healthy", gpu_device_id);
    gpu_health_status_[gpu_device_id] = true;
    
    // Update health data
    auto it = gpu_health_data_.find(gpu_device_id);
    if (it != gpu_health_data_.end()) {
        it->second.is_healthy = true;
        it->second.last_error = "";
        it->second.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
}

// Load Balancing Queries

int GPUMemoryManager::getLeastLoadedGPU() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int least_loaded_gpu = -1;
    float min_utilization = 1.0f;
    
    for (int gpu_id : available_gpus_) {
        // Only consider healthy GPUs
        auto health_it = gpu_health_status_.find(gpu_id);
        if (health_it == gpu_health_status_.end() || !health_it->second) {
            continue;
        }
        
        // Calculate utilization (guard against division by zero when max_vram_bytes is 0)
        size_t used_vram = 0;
        auto vram_it = per_gpu_vram_used_.find(gpu_id);
        if (vram_it != per_gpu_vram_used_.end()) {
            used_vram = vram_it->second;
        }
        
        float utilization = calculateUtilization(used_vram, config_.max_vram_bytes);
        
        if (utilization < min_utilization) {
            min_utilization = utilization;
            least_loaded_gpu = gpu_id;
        }
    }
    
    return least_loaded_gpu;
}

std::vector<int> GPUMemoryManager::getHealthyGPUs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<int> healthy_gpus;
    for (int gpu_id : available_gpus_) {
        auto it = gpu_health_status_.find(gpu_id);
        if (it != gpu_health_status_.end() && it->second) {
            healthy_gpus.push_back(gpu_id);
        }
    }
    
    return healthy_gpus;
}

float GPUMemoryManager::getAverageGPULoad() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (available_gpus_.empty()) {
        return 0.0f;
    }
    
    float total_load = 0.0f;
    int healthy_count = 0;
    
    for (int gpu_id : available_gpus_) {
        auto health_it = gpu_health_status_.find(gpu_id);
        if (health_it == gpu_health_status_.end() || !health_it->second) {
            continue;
        }
        
        size_t used_vram = 0;
        auto vram_it = per_gpu_vram_used_.find(gpu_id);
        if (vram_it != per_gpu_vram_used_.end()) {
            used_vram = vram_it->second;
        }
        
        float utilization = calculateUtilization(used_vram, config_.max_vram_bytes);
        total_load += utilization;
        healthy_count++;
    }
    
    return (healthy_count > 0) ? (total_load / healthy_count) : 0.0f;
}

bool GPUMemoryManager::needsLoadRebalancing(float threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (available_gpus_.size() < 2) {
        return false;  // No need to rebalance with single GPU
    }
    
    float avg_load = getAverageGPULoad();
    
    // Check if any GPU is significantly overloaded compared to average
    for (int gpu_id : available_gpus_) {
        auto health_it = gpu_health_status_.find(gpu_id);
        if (health_it == gpu_health_status_.end() || !health_it->second) {
            continue;
        }
        
        size_t used_vram = 0;
        auto vram_it = per_gpu_vram_used_.find(gpu_id);
        if (vram_it != per_gpu_vram_used_.end()) {
            used_vram = vram_it->second;
        }
        
        float utilization = calculateUtilization(used_vram, config_.max_vram_bytes);
        
        // If any GPU's load differs from average by more than threshold, rebalancing needed
        if (std::abs(utilization - avg_load) > threshold) {
            return true;
        }
    }
    
    return false;
}

void GPUMemoryManager::updateGPUHealth(int gpu_device_id) {
    // This would typically query actual GPU hardware
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        CUDA_CHECK(cudaSetDevice(gpu_device_id));
        
        // STUB/SIMULATION NOTE (stub #309):
        // Purpose: Keep GPU health polling functional in CUDA builds before NVML
        //          integration is wired for real temperature telemetry.
        // Activation: THEMIS_ENABLE_CUDA with gpu_available_=true in updateGPUHealth().
        // Production Delta: Temperature is hardcoded to 0.0°C; thermal throttling
        //                   and overheating signals are invisible to health checks.
        // Removal Plan: Integrate NVML temperature queries (per device) and propagate
        //               real sensor values into gpu_temperatures_.
        //               See src/llm/FUTURE_ENHANCEMENTS.md (GPU utilization/observability targets).
        //               Target: v2.2.0.
        // Get temperature (if available through NVIDIA Management Library - NVML)
        // This is a placeholder - actual implementation would use NVML
        gpu_temperatures_[gpu_device_id] = 0.0f;
        
        // Get memory info for utilization
        size_t free_mem, total_mem;
        CUDA_CHECK(cudaMemGetInfo(&free_mem, &total_mem));
        size_t used_mem = total_mem - free_mem;
        float utilization = static_cast<float>(used_mem) / total_mem * 100.0f;
        gpu_utilizations_[gpu_device_id] = utilization;
    }
#else
    // Simulation mode - calculate based on allocations
    size_t used_vram = 0;
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    if (it != per_gpu_vram_used_.end()) {
        used_vram = it->second;
    }
    
    float utilization = calculateUtilization(used_vram, config_.max_vram_bytes) * 100.0f;
    gpu_utilizations_[gpu_device_id] = utilization;
    gpu_temperatures_[gpu_device_id] = 45.0f + (utilization * 0.4f);  // Simulated temp
#endif
}

void GPUMemoryManager::checkGPUHealth(int gpu_device_id) {
    updateGPUHealth(gpu_device_id);
    
    auto util_it = gpu_utilizations_.find(gpu_device_id);
    auto temp_it = gpu_temperatures_.find(gpu_device_id);
    
    bool is_healthy = true;
    std::string reason;
    
    // Check temperature
    if (temp_it != gpu_temperatures_.end() && temp_it->second > 85.0f) {
        is_healthy = false;
        reason = "Temperature too high: " + std::to_string(temp_it->second) + "°C";
    }
    
    // Check utilization
    if (util_it != gpu_utilizations_.end() && util_it->second > 95.0f) {
        is_healthy = false;
        if (!reason.empty()) reason += "; ";
        reason += "Utilization too high: " + std::to_string(util_it->second) + "%";
    }
    
    if (is_healthy) {
        markGPUHealthy(gpu_device_id);
    } else {
        markGPUUnhealthy(gpu_device_id, reason);
    }
}

} // namespace llm
} // namespace themis

