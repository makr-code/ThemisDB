/**
 * @file gpu_memory_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=28; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=24, Debt=0, C=7, H=28, M=36, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/gpu_memory_manager.h"
#include "llm/gpu_memory_error_codes.h"
#include <stdexcept>
#include <limits>
#include "utils/error_registry.h"
#include "security/vram_secure_clear.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <unordered_set>
#include <optional>
#include "themis/gpu/memory_manager.h"
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
#include <dlfcn.h>
#endif

// Include actual CUDA headers when CUDA support is built
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include "utils/logger.h"

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
    
    ~MemoryHolder() noexcept {
        if (!ptr_) {
          return;
        }
        
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
        } catch (...) {
            spdlog::critical("Unknown exception during memory cleanup");
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
            // REL-73: check cudaSetDevice return value before secure-clear/free
            cudaError_t set_err = cudaSetDevice(gpu_device_id_);
            if (set_err != cudaSuccess) {
                spdlog::warn("MemoryHolder::freeGPUMemory: cudaSetDevice({}) failed: {}",
                             gpu_device_id_, cudaGetErrorString(set_err));
                // CRITICAL GAP FIX #1: Ensure secure clear happens even if device set fails
                security::VRAMSecureClear::secureClearCPU(ptr_, bytes_);
                return;
            }
            security::VRAMSecureClear::secureClearCUDA(ptr_, bytes_);
            
            // CRITICAL GAP FIX #2: Catch CUDA free errors and log distinctly
            cudaError_t free_err = cudaFree(ptr_);
            if (free_err != cudaSuccess) {
                spdlog::error("MemoryHolder::freeGPUMemory: cudaFree() failed with error: {} [{}]",
                             cudaGetErrorString(free_err), static_cast<int>(GPUMemoryErrorCode::CUDA_FREE_FAILED));
                // Don't throw - we're in destructor; ensure memory was at least cleared
            }
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
            // CRITICAL GAP FIX #2: Catch CUDA pinned free errors
            cudaError_t free_err = cudaFreeHost(ptr_);
            if (free_err != cudaSuccess) {
                spdlog::error("MemoryHolder::freePinnedMemory: cudaFreeHost() failed with error: {} [{}]",
                             cudaGetErrorString(free_err), static_cast<int>(GPUMemoryErrorCode::CUDA_PINNED_FREE_FAILED));
                // Don't throw - we're in destructor
            }
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
    
    void* ptr_ = nullptr;
    size_t bytes_ = 0;
    Type type_ = Type::CPU;
    bool gpu_available_ = false;
    int gpu_device_id_ = 0;
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

inline size_t calculateAvailableBytes(size_t total_bytes, size_t used_bytes) noexcept {
    return (used_bytes < total_bytes) ? (total_bytes - used_bytes) : 0;
}

inline bool tryAddSize(size_t lhs, size_t rhs, size_t& out) noexcept {
    if (rhs > (std::numeric_limits<size_t>::max() - lhs)) {
        return false;
    }
    out = lhs + rhs;
    return true;
}

inline size_t clampUsedVRAM(size_t used_vram, size_t max_vram_bytes) noexcept {
    return std::min(used_vram, max_vram_bytes);
}

inline float calculateUtilizationPercent(size_t used_vram, size_t max_vram_bytes) noexcept {
    return (max_vram_bytes > 0)
        ? (static_cast<float>(used_vram) * 100.0f) / static_cast<float>(max_vram_bytes)
        : 0.0f;
}

inline bool isGPUAvailableNoLock(const std::unordered_map<int, bool>& gpu_health_status,
                                 int gpu_device_id) noexcept {
    auto it = gpu_health_status.find(gpu_device_id);
    if (it == gpu_health_status.end()) {
        return false;
    }
    return it->second;
}

inline bool isTrackedGpuHealthEntryNoLock(const std::unordered_map<int, bool>& gpu_health_status,
                                          int gpu_device_id) noexcept {
    return gpu_health_status.find(gpu_device_id) != gpu_health_status.end();
}

inline bool isTrackedGpuNoLock(const std::vector<int>& available_gpus, int gpu_device_id) noexcept {
    return std::find(available_gpus.begin(), available_gpus.end(), gpu_device_id) != available_gpus.end();
}

inline GPUMemoryManager::GPUHealth buildUnavailableGpuHealth(int gpu_device_id,
                                                             float utilization_percent,
                                                             float temperature_celsius,
                                                             size_t error_count,
                                                             std::string reason) {
    GPUMemoryManager::GPUHealth health = {};
    health.device_id = gpu_device_id;
    health.is_available = false;
    health.is_healthy = false;
    health.temperature_celsius = temperature_celsius;
    health.utilization_percent = utilization_percent;
    health.error_count = error_count;
    health.last_error = std::move(reason);
    health.last_check_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return health;
}

inline std::vector<int> sanitizeGpuDeviceList(const std::vector<int>& requested_gpus,
                                              std::optional<int> max_device_count = std::nullopt) {
    std::vector<int> sanitized;
    sanitized.reserve(requested_gpus.size());
    std::unordered_set<int> seen;

    for (int gpu_id : requested_gpus) {
        if (gpu_id < 0) {
            spdlog::warn("Ignoring invalid negative GPU id {}", gpu_id);
            continue;
        }
        if (max_device_count.has_value() && gpu_id >= max_device_count.value()) {
            spdlog::warn("GPU {} requested but only {} GPUs available, skipping",
                         gpu_id, max_device_count.value());
            continue;
        }
        if (!seen.insert(gpu_id).second) {
            spdlog::warn("Ignoring duplicate GPU id {} in configuration", gpu_id);
            continue;
        }
        sanitized.push_back(gpu_id);
    }

    return sanitized;
}

inline void cleanupRawAllocation(void* ptr,
                                 size_t bytes,
                                 detail::MemoryHolder::Type type,
                                 bool gpu_available,
                                 int gpu_device_id = 0) noexcept {
    if (ptr == nullptr) {
        return;
    }

    try {
        detail::MemoryHolder cleanup_holder(ptr, bytes, type, gpu_available, gpu_device_id);
    } catch (...) {
        THEMIS_WARN("gpu_memory_manager::isTrackedGpuNoLock: unhandled exception caught");
        // Best-effort cleanup in failure fallback path.
    }
}

inline std::optional<float> queryNvmlTemperatureCelsius([[maybe_unused]] int gpu_device_id) {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    using nvmlReturn_t = int;
    using nvmlDevice_t = void*;
    constexpr nvmlReturn_t NVML_SUCCESS = 0;
    constexpr unsigned int NVML_TEMPERATURE_GPU = 0;

    void* nvml_lib = dlopen("libnvidia-ml.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (nvml_lib == nullptr) {
        nvml_lib = dlopen("libnvidia-ml.so", RTLD_LAZY | RTLD_LOCAL);
    }
    if (nvml_lib == nullptr) {
        return std::nullopt;
    }

    auto close_lib = [&nvml_lib]() {
        if (nvml_lib != nullptr) {
            dlclose(nvml_lib);
            nvml_lib = nullptr;
        }
    };

    using NvmlInitFn = nvmlReturn_t (*)();
    using NvmlShutdownFn = nvmlReturn_t (*)();
    using NvmlGetHandleFn = nvmlReturn_t (*)(unsigned int, nvmlDevice_t*);
    using NvmlGetTemperatureFn = nvmlReturn_t (*)(nvmlDevice_t, unsigned int, unsigned int*);

    auto init_fn = reinterpret_cast<NvmlInitFn>(dlsym(nvml_lib, "nvmlInit_v2"));
    if (init_fn == nullptr) {
        init_fn = reinterpret_cast<NvmlInitFn>(dlsym(nvml_lib, "nvmlInit"));
    }
    auto shutdown_fn = reinterpret_cast<NvmlShutdownFn>(dlsym(nvml_lib, "nvmlShutdown"));
    auto get_handle_fn = reinterpret_cast<NvmlGetHandleFn>(dlsym(nvml_lib, "nvmlDeviceGetHandleByIndex_v2"));
    if (get_handle_fn == nullptr) {
        get_handle_fn = reinterpret_cast<NvmlGetHandleFn>(dlsym(nvml_lib, "nvmlDeviceGetHandleByIndex"));
    }
    auto get_temp_fn = reinterpret_cast<NvmlGetTemperatureFn>(dlsym(nvml_lib, "nvmlDeviceGetTemperature"));

    if (init_fn == nullptr || shutdown_fn == nullptr || get_handle_fn == nullptr || get_temp_fn == nullptr) {
        close_lib();
        return std::nullopt;
    }

    if (init_fn() != NVML_SUCCESS) {
        close_lib();
        return std::nullopt;
    }

    std::optional<float> result = std::nullopt;
    nvmlDevice_t device = nullptr;
    if (get_handle_fn(static_cast<unsigned int>(gpu_device_id), &device) == NVML_SUCCESS && device != nullptr) {
        unsigned int temperature_c = 0;
        if (get_temp_fn(device, NVML_TEMPERATURE_GPU, &temperature_c) == NVML_SUCCESS) {
            result = static_cast<float>(temperature_c);
        }
    }

    shutdown_fn();
    close_lib();
    return result;
#else
    static_cast<void>(gpu_device_id);
    return std::nullopt;
#endif
}
} // namespace

// NVML temperature injection state (stub #309 resolution).
namespace {
std::mutex nvml_temp_fn_mutex;
GPUMemoryManager::NvmlTemperatureFn nvml_temp_fn;
} // anonymous namespace

void GPUMemoryManager::setNvmlTemperatureFn(NvmlTemperatureFn fn) {
    std::lock_guard<std::mutex> lock(nvml_temp_fn_mutex);
    nvml_temp_fn = std::move(fn);
}

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
        cudaDeviceProp prop{};
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
            available_gpus_ = sanitizeGpuDeviceList(config_.gpu_devices, deviceCount);

            if (available_gpus_.empty()) {
                spdlog::warn("No valid configured GPUs remain after validation, falling back to primary GPU {}",
                             gpu_device_id_);
                available_gpus_.push_back(gpu_device_id_);
            }

            for (int gpu_id : available_gpus_) {
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
                        cudaError_t can_access_err = cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
                        if (can_access_err != cudaSuccess) {
                            spdlog::warn("  P2P capability query failed: GPU {} -> GPU {}: {}",
                                         src_gpu, dst_gpu, cudaGetErrorString(can_access_err));
                            continue;
                        }
                        if (can_access) {
                            cudaError_t set_err = cudaSetDevice(src_gpu);
                            if (set_err != cudaSuccess) {
                                spdlog::warn("  P2P setup failed: cudaSetDevice({}) failed: {}",
                                             src_gpu, cudaGetErrorString(set_err));
                                continue;
                            }
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
        spdlog::warn("No usable CUDA GPU detected: {}", cudaGetErrorString(err));
        spdlog::warn("Running in CPU-only mode; GPU runtime remains unavailable");
        
        // Fallback to simulation mode
        gpu_device_id_ = 0;
        available_gpus_.push_back(gpu_device_id_);
        per_gpu_vram_used_[gpu_device_id_] = 0;
        gpu_health_status_[gpu_device_id_] = false;
    }
#else
    // Track configured GPU ids for diagnostics, but keep runtime availability disabled
    // when CUDA is not enabled at build time.
    gpu_available_ = false;
    gpu_device_id_ = 0;
    
    // Initialize multi-GPU support in simulation mode (v1.4.0)
    if (config_.enable_multi_gpu && !config_.gpu_devices.empty()) {
        spdlog::info("Initializing multi-GPU support (simulation) with {} GPUs", config_.gpu_devices.size());
        available_gpus_ = sanitizeGpuDeviceList(config_.gpu_devices);
        if (available_gpus_.empty()) {
            spdlog::warn("No valid configured GPUs remain after validation, falling back to primary GPU {}",
                         gpu_device_id_);
            available_gpus_.push_back(gpu_device_id_);
        }

        for (int gpu_id : available_gpus_) {
            per_gpu_vram_used_[gpu_id] = 0;
            gpu_health_status_[gpu_id] = false;
            spdlog::info("  GPU {} tracked for diagnostics only (CUDA unavailable)", gpu_id);
        }
        
        if (config_.enable_peer_access) {
            spdlog::info("P2P access requested but unavailable without CUDA");
        }
    } else {
        // Single GPU mode
        available_gpus_.push_back(gpu_device_id_);
        per_gpu_vram_used_[gpu_device_id_] = 0;
        gpu_health_status_[gpu_device_id_] = false;
    }
    
    spdlog::info("GPU Memory Manager: CUDA not enabled at build time; GPU runtime remains unavailable");
    spdlog::info("  Tracked GPU slots: {}", available_gpus_.size());
#endif

    // Apply VRAM limit fallback: if max_vram_bytes is still 0 after platform-specific
    // initialization (e.g. no GPU available or cudaGetDeviceProperties failed), use a
    // sensible simulation default so that canAllocate() and getLeastLoadedGPU() work.
    if (config_.max_vram_bytes == 0) {
        config_.max_vram_bytes = 8ULL * 1024 * 1024 * 1024;  // 8 GB simulation default
        spdlog::info("  VRAM limit defaulted to {:.2f} GB (simulation)",
                     config_.max_vram_bytes / (1024.0 * 1024 * 1024));
    }

    // Populate initial health metrics (temperature + utilization) for each GPU so that
    // callers of getGPUHealth() immediately get real data rather than zero-defaults.
    for (int gpu_id : available_gpus_) {
        updateGPUHealth(gpu_id);
    }
}

void GPUMemoryManager::shutdownGPU() {
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Disable peer access if it was enabled
        if (config_.enable_peer_access && available_gpus_.size() > 1) {
            for (size_t i = 0; i < available_gpus_.size(); ++i) {
                int src_gpu = available_gpus_[i];
                cudaError_t set_err = cudaSetDevice(src_gpu);
                if (set_err != cudaSuccess) {
                    spdlog::warn("shutdownGPU: cudaSetDevice({}) failed while disabling peer access: {}",
                                 src_gpu, cudaGetErrorString(set_err));
                    continue;
                }
                for (size_t j = 0; j < available_gpus_.size(); ++j) {
                    if (i != j) {
                        int dst_gpu = available_gpus_[j];
                        cudaError_t disable_err = cudaDeviceDisablePeerAccess(dst_gpu);
                        if (disable_err != cudaSuccess && disable_err != cudaErrorPeerAccessNotEnabled) {
                            spdlog::warn("shutdownGPU: cudaDeviceDisablePeerAccess({} -> {}) failed: {}",
                                         src_gpu, dst_gpu, cudaGetErrorString(disable_err));
                        }
                    }
                }
            }
        }
        
        // Reset all devices
        for (int gpu_id : available_gpus_) {
            cudaError_t set_err = cudaSetDevice(gpu_id);
            if (set_err != cudaSuccess) {
                spdlog::warn("shutdownGPU: cudaSetDevice({}) failed before reset: {}",
                             gpu_id, cudaGetErrorString(set_err));
                continue;
            }
            CUDA_CHECK(cudaDeviceReset());
        }
    }
#endif
}

void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes) {
    // CRITICAL GAP FIX #3: Add device pre-checks and validation
    
    // Gate through the canonical VRAM policy so that edition limits and
    // per-tenant quotas are enforced at a single, unified control point.
    auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
    if (policy.isGPUEnabled()) {
        if (!policy.TryAllocateGPU(static_cast<uint64_t>(bytes), model_id)) {
            spdlog::error("[{}] allocateGPU: canonical VRAM policy rejected {} bytes for model '{}'",
                         static_cast<int>(GPUMemoryErrorCode::ALLOCATION_EXCEEDS_LIMIT),
                         bytes, model_id);
            return nullptr;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // CRITICAL GAP FIX #4: Implement robust overflow detection
    if (bytes == 0 || bytes > std::numeric_limits<size_t>::max()) {
        spdlog::error("[{}] GPU allocation size overflow or zero: {} bytes",
                     static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_OVERFLOW), bytes);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    
    if (!canAllocate(bytes, 0)) {
        double bytes_mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        size_t available_bytes = calculateAvailableBytes(config_.max_vram_bytes, total_vram_used_.load(std::memory_order_relaxed));
        double available_mb = static_cast<double>(available_bytes) / (1024.0 * 1024.0);
        spdlog::error("[{}] GPU OOM: requested {:.1f} MB, available {:.1f} MB", 
                     static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_OOM),
                     bytes_mb, available_mb);
        // Undo canonical reservation since the local limit rejected the request.
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    
    void* ptr = nullptr;
    detail::MemoryHolder::Type alloc_type = detail::MemoryHolder::Type::CPU;
    bool was_fallback = false;
    (void)was_fallback;
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_ && !available_gpus_.empty()) {
        // CRITICAL GAP FIX #5: Verify GPU device health before allocation
        int selected_gpu = getLeastLoadedGPU();
        
        // Check if GPU is healthy
        auto gpu_health_it = gpu_health_status_.find(selected_gpu);
        if (gpu_health_it != gpu_health_status_.end() && !gpu_health_it->second) {
            spdlog::warn("[{}] GPU device {} is unhealthy, attempting fallback to CPU",
                        static_cast<int>(GPUMemoryErrorCode::GPU_DEVICE_UNHEALTHY), selected_gpu);
            was_fallback = true;
        } else {
            // Try GPU allocation
            cudaError_t set_err = cudaSetDevice(selected_gpu);
            if (set_err != cudaSuccess) {
                spdlog::warn("[{}] cudaSetDevice({}) failed: {}, attempting fallback",
                            static_cast<int>(GPUMemoryErrorCode::GPU_DEVICE_SET_FAILED),
                            selected_gpu, cudaGetErrorString(set_err));
                was_fallback = true;
            } else {
                // Device set succeeded, try allocation
                cudaError_t err = cudaMalloc(&ptr, bytes);
                if (err != cudaSuccess) {
                    spdlog::warn("[{}] cudaMalloc({} bytes) failed: {}, attempting pinned CPU fallback",
                                static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_OOM),
                                bytes, cudaGetErrorString(err));
                    was_fallback = true;
                    ptr = nullptr;
                } else {
                    alloc_type = detail::MemoryHolder::Type::GPU;
                    spdlog::debug("Successfully allocated {} MB VRAM on GPU {}", 
                                 bytes / (1024.0 * 1024), selected_gpu);
                }
            }
        }
    } else {
        was_fallback = true;
    }
    
    // CRITICAL GAP FIX #6: Implement fallback strategy: GPU -> pinned CPU -> regular CPU
    if (was_fallback || !ptr) {
        spdlog::info("[{}] Attempting fallback to pinned CPU memory",
                    static_cast<int>(GPUMemoryErrorCode::FALLBACK_TO_PINNED_CPU));
        
        // Try pinned CPU memory
        if (gpu_available_) {
            int primary_gpu = gpu_device_id_;
            cudaError_t set_err = cudaSetDevice(primary_gpu);
            if (set_err == cudaSuccess) {
                cudaError_t pinned_err = cudaMallocHost(&ptr, bytes);
                if (pinned_err == cudaSuccess) {
                    alloc_type = detail::MemoryHolder::Type::PINNED;
                    spdlog::debug("Allocated {} MB pinned CPU memory as GPU fallback", bytes / (1024.0 * 1024));
                } else {
                    spdlog::warn("[{}] cudaMallocHost failed: {}, falling back to regular CPU",
                                static_cast<int>(GPUMemoryErrorCode::CPU_PINNED_ALLOCATION_FAILED),
                                cudaGetErrorString(pinned_err));
                    ptr = nullptr;
                }
            }
        }
        
        // If pinned CPU also failed, try regular CPU memory
        if (!ptr) {
            spdlog::info("[{}] Attempting final fallback to regular CPU memory",
                        static_cast<int>(GPUMemoryErrorCode::FALLBACK_TO_CPU));
            ptr = std::malloc(bytes);
            if (!ptr) {
                spdlog::error("[{}] All allocation strategies exhausted for {} bytes",
                             static_cast<int>(GPUMemoryErrorCode::FALLBACK_EXHAUSTED), bytes);
                if (policy.isGPUEnabled()) {
                    policy.DeallocateGPU(static_cast<uint64_t>(bytes));
                }
                return nullptr;
            }
            alloc_type = detail::MemoryHolder::Type::CPU;
            spdlog::info("Allocated {} MB regular CPU memory as final fallback", bytes / (1024.0 * 1024));
        }
    }
#else
    // Simulation mode: use regular malloc when CUDA is not enabled at build time
    ptr = std::malloc(bytes);
    if (!ptr) {
        spdlog::error("[{}] malloc failed for {} bytes in simulation mode",
                     static_cast<int>(GPUMemoryErrorCode::CPU_ALLOCATION_FAILED), bytes);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    alloc_type = detail::MemoryHolder::Type::CPU;
#endif
    
    if (!ptr) {
        spdlog::error("[{}] Failed to allocate {} bytes through all available strategies",
                     static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_NO_FALLBACK), bytes);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    
    // Wrap in RAII holder immediately to prevent leaks on exceptions during bookkeeping
    auto holder = std::make_shared<detail::MemoryHolder>(ptr, bytes, alloc_type, gpu_available_, 0);
    
    // Track allocation with RAII holder for automatic cleanup.
    // Guard metadata bookkeeping so low-memory exceptions do not leak raw allocations.
    try {
        MemoryAllocation alloc;
        alloc.model_id = model_id;
        alloc.vram_bytes = (alloc_type == detail::MemoryHolder::Type::GPU) ? bytes : 0;
        alloc.ram_bytes = (alloc_type == detail::MemoryHolder::Type::GPU) ? 0 : bytes;
        alloc.gpu_ptr = (alloc_type == detail::MemoryHolder::Type::GPU) ? ptr : nullptr;
        alloc.cpu_ptr = (alloc_type != detail::MemoryHolder::Type::GPU) ? ptr : nullptr;
        alloc.is_pinned = (alloc_type == detail::MemoryHolder::Type::PINNED);
        alloc.gpu_device_id = 0;  // Default GPU device
        alloc.holder = holder;  // Use the already-created RAII holder
        allocations_[model_id].push_back(std::move(alloc));
    } catch (const std::exception& e) {
        // holder will automatically clean up ptr through RAII
        spdlog::error("[{}] allocateGPU metadata bookkeeping failed for model {}: {}",
                     static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_OOM), model_id, e.what());
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    } catch (...) {
        // holder will automatically clean up ptr through RAII
        spdlog::error("[{}] allocateGPU metadata bookkeeping failed for model {}: unknown exception",
                     static_cast<int>(GPUMemoryErrorCode::GPU_ALLOCATION_OOM), model_id);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }

    // Update tracking based on allocation type
    if (alloc_type == detail::MemoryHolder::Type::GPU) {
        total_vram_used_ += bytes;
        per_gpu_vram_used_[0] += bytes;
        spdlog::debug("Allocated {} MB VRAM for model {} (total: {} MB)", 
                     bytes / (1024.0 * 1024),
                     model_id,
                     total_vram_used_.load(std::memory_order_relaxed) / (1024.0 * 1024));
    } else {
        total_ram_used_ += bytes;
        spdlog::debug("Allocated {} MB {} memory for model {} (via GPU allocation request)",
                     bytes / (1024.0 * 1024),
                     alloc_type == detail::MemoryHolder::Type::PINNED ? "pinned" : "regular",
                     model_id);
    }
    
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
    detail::MemoryHolder::Type holder_type = detail::MemoryHolder::Type::CPU;
    
#ifdef THEMIS_ENABLE_CUDA
    if (pinned && gpu_available_) {
        // Use CUDA pinned memory for faster transfers
        cudaError_t err = cudaMallocHost(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::warn("cudaMallocHost failed: {}, falling back to regular malloc", 
                        cudaGetErrorString(err));
            pinned = false;
            ptr = std::malloc(bytes);
            holder_type = detail::MemoryHolder::Type::CPU;
        } else {
            holder_type = detail::MemoryHolder::Type::PINNED;
        }
    } else {
        ptr = std::malloc(bytes);
        pinned = false;
        holder_type = detail::MemoryHolder::Type::CPU;
    }
#else
    // Simulation mode: always use regular malloc
    ptr = std::malloc(bytes);
    pinned = false;
    holder_type = detail::MemoryHolder::Type::CPU;
#endif
    
    if (!ptr) {
        spdlog::error("Failed to allocate {} bytes RAM for model {}", bytes, model_id);
        return nullptr;
    }
    
    // Wrap in RAII holder immediately to prevent leaks on exceptions during bookkeeping
    auto holder = std::make_shared<detail::MemoryHolder>(ptr, bytes, holder_type, gpu_available_, 0);
    
    // Track allocation with RAII holder.
    // Guard metadata bookkeeping so low-memory exceptions do not leak raw allocations.
    try {
        MemoryAllocation alloc;
        alloc.model_id = model_id;
        alloc.ram_bytes = bytes;
        alloc.cpu_ptr = ptr;
        alloc.is_pinned = pinned;
        alloc.holder = holder;  // Use the already-created RAII holder
        allocations_[model_id].push_back(std::move(alloc));
    } catch (const std::exception& e) {
        cleanupRawAllocation(ptr, bytes, holder_type, gpu_available_);
        spdlog::error("allocateCPU metadata bookkeeping failed for model {}: {}", model_id, e.what());
        return nullptr;
    } catch (...) {
        cleanupRawAllocation(ptr, bytes, holder_type, gpu_available_);
        spdlog::error("allocateCPU metadata bookkeeping failed for model {}: unknown exception", model_id);
        return nullptr;
    }

    total_ram_used_ += bytes;
    
    spdlog::debug("Allocated {} MB RAM ({}) for model {} (total: {} MB)", 
                  bytes / (1024.0 * 1024),
                  pinned ? "pinned" : "pageable",
                  model_id,
                  total_ram_used_.load(std::memory_order_relaxed) / (1024.0 * 1024));
    
    return ptr;
}

bool GPUMemoryManager::freeGPU(const std::string& model_id, void* ptr) {
    if (!ptr) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    // Find the allocation and remove it
    // The holder's destructor will automatically handle cleanup via RAII
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->gpu_ptr == ptr) {
            const size_t freed_bytes = alloc_it->vram_bytes;
            const int gpu_id = alloc_it->gpu_device_id;
            if (freed_bytes > total_vram_used_.load(std::memory_order_relaxed)) {
                spdlog::error("GPUMemoryManager::freeGPU: VRAM accounting underflow for model '{}'; "
                              "clamping to 0", model_id);
                total_vram_used_.store(0, std::memory_order_relaxed);
            } else {
                total_vram_used_.fetch_sub(freed_bytes, std::memory_order_relaxed);
            }

            auto per_gpu_it = per_gpu_vram_used_.find(gpu_id);
            if (per_gpu_it != per_gpu_vram_used_.end()) {
                if (freed_bytes > per_gpu_it->second) {
                    spdlog::error("GPUMemoryManager::freeGPU: per-GPU VRAM accounting underflow for model '{}' on GPU {}; clamping to 0",
                                  model_id, gpu_id);
                    per_gpu_it->second = 0;
                } else {
                    per_gpu_it->second -= freed_bytes;
                }
            }
            
            // Erase triggers holder destructor for automatic cleanup
            it->second.erase(alloc_it);
            
            if (it->second.empty()) {
                allocations_.erase(it);
            }

            // Notify the canonical policy to keep global accounting consistent.
            auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
            if (policy.isGPUEnabled()) {
                policy.DeallocateGPU(static_cast<uint64_t>(freed_bytes));
            }
            
            return true;
        }
    }
    
    return false;
}

bool GPUMemoryManager::freeCPU(const std::string& model_id, void* ptr) {
    if (!ptr) {
      return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = allocations_.find(model_id);
    if (it == allocations_.end()) {
        return false;
    }
    
    // Find the allocation and remove it
    // The holder's destructor will automatically handle cleanup via RAII
    for (auto alloc_it = it->second.begin(); alloc_it != it->second.end(); ++alloc_it) {
        if (alloc_it->cpu_ptr == ptr) {
            if (alloc_it->ram_bytes > total_ram_used_.load(std::memory_order_relaxed)) {
                spdlog::error("GPUMemoryManager::freeCPU: RAM accounting underflow for model '{}'; "
                              "clamping to 0", model_id);
                total_ram_used_.store(0, std::memory_order_relaxed);
            } else {
                total_ram_used_.fetch_sub(alloc_it->ram_bytes, std::memory_order_relaxed);
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
    
    if (freed_vram > total_vram_used_.load(std::memory_order_relaxed)) {
        spdlog::error("GPUMemoryManager::freeModel: VRAM accounting underflow for model '{}'; "
                      "clamping to 0", model_id);
        total_vram_used_.store(0, std::memory_order_relaxed);
    } else {
        total_vram_used_.fetch_sub(freed_vram, std::memory_order_relaxed);
    }
    if (freed_ram > total_ram_used_.load(std::memory_order_relaxed)) {
        spdlog::error("GPUMemoryManager::freeModel: RAM accounting underflow for model '{}'; "
                      "clamping to 0", model_id);
        total_ram_used_.store(0, std::memory_order_relaxed);
    } else {
        total_ram_used_.fetch_sub(freed_ram, std::memory_order_relaxed);
    }
    
    // Erase all allocations for this model
    // Holders will automatically clean up via RAII
    allocations_.erase(it);
    
    spdlog::info("Freed memory for model {}: {} MB VRAM, {} MB RAM",
                 model_id,
                 freed_vram / (1024.0 * 1024),
                 freed_ram / (1024.0 * 1024));

    // Notify the canonical policy to keep global VRAM accounting consistent.
    auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
    if (policy.isGPUEnabled() && freed_vram > 0) {
        policy.DeallocateGPU(static_cast<uint64_t>(freed_vram));
    }
    
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
    return total_ram_used_.load(std::memory_order_relaxed);
}

size_t GPUMemoryManager::getFreeVRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (total_vram_used_.load(std::memory_order_relaxed) >= config_.max_vram_bytes) {
        return 0;
    }
    
    return config_.max_vram_bytes - total_vram_used_.load(std::memory_order_relaxed);
}

size_t GPUMemoryManager::getFreeRAM() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (total_ram_used_.load(std::memory_order_relaxed) >= config_.max_ram_bytes) {
        return 0;
    }
    
    return config_.max_ram_bytes - total_ram_used_.load(std::memory_order_relaxed);
}

bool GPUMemoryManager::canAllocate(size_t vram_bytes, size_t ram_bytes) const {
    // Thread-safe: uses atomic loads on total_vram_used_ and total_ram_used_

    // Guard against size_t overflow before comparing against limits.
    if (vram_bytes > 0 && total_vram_used_.load(std::memory_order_acquire) > std::numeric_limits<size_t>::max() - vram_bytes) {
        return false;
    }
    if (ram_bytes > 0 && total_ram_used_.load(std::memory_order_acquire) > std::numeric_limits<size_t>::max() - ram_bytes) {
        return false;
    }

    size_t future_vram = total_vram_used_.load(std::memory_order_acquire) + vram_bytes;
    size_t future_ram = total_ram_used_.load(std::memory_order_acquire) + ram_bytes;
    
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
            cudaError_t set_err = cudaSetDevice(device_id);
            if (set_err != cudaSuccess) {
                spdlog::warn("Defrag: cudaSetDevice({}) failed for model {}: {}",
                             device_id, model_id, cudaGetErrorString(set_err));
                continue;
            }

            cudaError_t alloc_err = cudaMalloc(&new_ptr, total_vram);
            if (alloc_err != cudaSuccess) {
                spdlog::warn("Failed to allocate consolidated GPU memory for model {} on device {}", model_id, device_id);
                continue;
            }

            size_t offset = 0;
            bool copy_ok = true;
            for (const auto& alloc : device_allocs) {
                // Wave-B L4: bounds-check added (pointer_arithmetic_unbounded fix)
                if (offset + alloc.vram_bytes > total_vram) {
                    spdlog::error("Defrag: GPU copy offset {} + {} exceeds total_vram {} for model {} on GPU {}",
                                  offset, alloc.vram_bytes, total_vram, model_id, device_id);
                    copy_ok = false;
                    break;
                }
                cudaError_t copy_err = cudaMemcpy(static_cast<char*>(new_ptr) + offset,
                                                  alloc.gpu_ptr,
                                                  alloc.vram_bytes,
                                                  cudaMemcpyDeviceToDevice);
                if (copy_err != cudaSuccess) {
                    spdlog::warn("Defrag: cudaMemcpy failed for model {} on GPU {}: {}",
                                 model_id, device_id, cudaGetErrorString(copy_err));
                    copy_ok = false;
                    break;
                }
                offset += alloc.vram_bytes;
            }
            if (!copy_ok) {
                // REL-66: check cudaFree return value in defragment cleanup path
                cudaError_t free_err = cudaFree(new_ptr);
                if (free_err != cudaSuccess) {
                    spdlog::warn("Defrag: cudaFree of scratch buffer failed: {}",
                                 cudaGetErrorString(free_err));
                }
                continue;
            }
        } else {
            new_ptr = std::malloc(total_vram);
            if (!new_ptr) {
                continue;
            }
            size_t offset = 0;
            for (const auto& alloc : device_allocs) {
                // Wave-B L4: bounds-check added (pointer_arithmetic_unbounded fix)
                if (offset + alloc.vram_bytes > total_vram) {
                    spdlog::error("Defrag: CPU copy offset {} + {} exceeds total_vram {} for model {}",
                                  offset, alloc.vram_bytes, total_vram, model_id);
                    break;
                }
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
            // Wave-B L4: bounds-check added (pointer_arithmetic_unbounded fix)
            if (offset + alloc.vram_bytes > total_vram) {
                spdlog::error("Defrag: CPU-only copy offset {} + {} exceeds total_vram {} for model {}",
                              offset, alloc.vram_bytes, total_vram, model_id);
                break;
            }
            std::memcpy(static_cast<char*>(new_ptr) + offset, alloc.gpu_ptr, alloc.vram_bytes);
            offset += alloc.vram_bytes;
        }
#endif

        // Build replacement allocation holder first so failure keeps old allocations intact.
        std::shared_ptr<detail::MemoryHolder> consolidated_holder;
        try {
            consolidated_holder = std::make_shared<detail::MemoryHolder>(
                new_ptr, total_vram, detail::MemoryHolder::Type::GPU, gpu_available_, device_id
            );
        } catch (const std::exception& e) {
            cleanupRawAllocation(new_ptr, total_vram, detail::MemoryHolder::Type::GPU, gpu_available_, device_id);
            spdlog::warn("Defrag: failed to create consolidated GPU holder for model {} on GPU {}: {}",
                         model_id, device_id, e.what());
            continue;
        } catch (...) {
            cleanupRawAllocation(new_ptr, total_vram, detail::MemoryHolder::Type::GPU, gpu_available_, device_id);
            spdlog::warn("Defrag: failed to create consolidated GPU holder for model {} on GPU {}",
                         model_id, device_id);
            continue;
        }

        // Update allocations list for this model/device.
        // Remove only the allocations that were identified as fragmented (device_allocs),
        // not all allocations for this device_id.
        auto& model_allocs = allocations_[model_id];
        std::unordered_set<void*> ptrs_to_erase;
        for (const auto& alloc : device_allocs) {
            ptrs_to_erase.insert(alloc.gpu_ptr);
        }

        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.vram_bytes = total_vram;
        consolidated.gpu_ptr = new_ptr;
        consolidated.gpu_device_id = device_id;
        consolidated.holder = std::move(consolidated_holder);
        model_allocs.push_back(std::move(consolidated));
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [&ptrs_to_erase](const MemoryAllocation& alloc) {
                    return ptrs_to_erase.count(alloc.gpu_ptr) > 0;
                }),
            model_allocs.end());

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
            // Wave-B L4: bounds-check added (pointer_arithmetic_unbounded fix)
            if (offset + alloc.ram_bytes > total_ram) {
                spdlog::error("Defrag: pinned copy offset {} + {} exceeds total_ram {} for model {}",
                              offset, alloc.ram_bytes, total_ram, model_id);
                break;
            }
            std::memcpy(static_cast<char*>(new_ptr) + offset, 
                       alloc.cpu_ptr, 
                       alloc.ram_bytes);
            offset += alloc.ram_bytes;
        }
        
        std::shared_ptr<detail::MemoryHolder> consolidated_holder;
        try {
            consolidated_holder = std::make_shared<detail::MemoryHolder>(
                new_ptr, total_ram, detail::MemoryHolder::Type::PINNED, gpu_available_
            );
        } catch (const std::exception& e) {
            cleanupRawAllocation(new_ptr, total_ram, detail::MemoryHolder::Type::PINNED, gpu_available_);
            spdlog::warn("Defrag: failed to create consolidated pinned holder for model {}: {}", model_id, e.what());
            return false;
        } catch (...) {
            cleanupRawAllocation(new_ptr, total_ram, detail::MemoryHolder::Type::PINNED, gpu_available_);
            spdlog::warn("Defrag: failed to create consolidated pinned holder for model {}", model_id);
            return false;
        }

        // Update allocations - removing old allocations will trigger cleanup via RAII.
        std::unordered_set<void*> ptrs_to_erase;
        for (const auto& alloc : pinned_allocs) {
            ptrs_to_erase.insert(alloc.cpu_ptr);
        }
        auto& model_allocs = allocations_[model_id];

        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.ram_bytes = total_ram;
        consolidated.cpu_ptr = new_ptr;
        consolidated.is_pinned = true;
        consolidated.holder = std::move(consolidated_holder);
        model_allocs.push_back(std::move(consolidated));
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [&ptrs_to_erase](const MemoryAllocation& alloc) {
                    return ptrs_to_erase.count(alloc.cpu_ptr) > 0;
                }),
            model_allocs.end()
        );
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
            // Wave-B L4: bounds-check added (pointer_arithmetic_unbounded fix)
            if (offset + alloc.ram_bytes > total_ram) {
                spdlog::error("Defrag: regular-CPU copy offset {} + {} exceeds total_ram {} for model {}",
                              offset, alloc.ram_bytes, total_ram, model_id);
                break;
            }
            std::memcpy(static_cast<char*>(new_ptr) + offset, 
                       alloc.cpu_ptr, 
                       alloc.ram_bytes);
            offset += alloc.ram_bytes;
        }
        
        std::shared_ptr<detail::MemoryHolder> consolidated_holder;
        try {
            consolidated_holder = std::make_shared<detail::MemoryHolder>(
                new_ptr, total_ram, detail::MemoryHolder::Type::CPU, gpu_available_
            );
        } catch (const std::exception& e) {
            cleanupRawAllocation(new_ptr, total_ram, detail::MemoryHolder::Type::CPU, gpu_available_);
            spdlog::warn("Defrag: failed to create consolidated CPU holder for model {}: {}", model_id, e.what());
            return false;
        } catch (...) {
            cleanupRawAllocation(new_ptr, total_ram, detail::MemoryHolder::Type::CPU, gpu_available_);
            spdlog::warn("Defrag: failed to create consolidated CPU holder for model {}", model_id);
            return false;
        }

        // Update allocations - removing old allocations will trigger cleanup via RAII.
        std::unordered_set<void*> ptrs_to_erase;
        for (const auto& alloc : regular_allocs) {
            ptrs_to_erase.insert(alloc.cpu_ptr);
        }
        auto& model_allocs = allocations_[model_id];

        // Create new consolidated allocation with RAII holder
        MemoryAllocation consolidated;
        consolidated.model_id = model_id;
        consolidated.ram_bytes = total_ram;
        consolidated.cpu_ptr = new_ptr;
        consolidated.is_pinned = false;
        consolidated.holder = std::move(consolidated_holder);
        model_allocs.push_back(std::move(consolidated));
        model_allocs.erase(
            std::remove_if(model_allocs.begin(), model_allocs.end(),
                [&ptrs_to_erase](const MemoryAllocation& alloc) {
                    return ptrs_to_erase.count(alloc.cpu_ptr) > 0;
                }),
            model_allocs.end()
        );
    }
    
    return true;
}

GPUMemoryManager::Stats GPUMemoryManager::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.total_vram_bytes = config_.max_vram_bytes;
    stats.used_vram_bytes = clampUsedVRAM(total_vram_used_.load(std::memory_order_relaxed), stats.total_vram_bytes);
    stats.free_vram_bytes = calculateAvailableBytes(stats.total_vram_bytes, total_vram_used_.load(std::memory_order_relaxed));
    
    stats.total_ram_bytes = config_.max_ram_bytes;
    stats.used_ram_bytes = std::min(total_ram_used_.load(std::memory_order_relaxed), stats.total_ram_bytes);
    stats.free_ram_bytes = calculateAvailableBytes(stats.total_ram_bytes, total_ram_used_.load(std::memory_order_relaxed));
    
    stats.num_models = allocations_.size();
    
    size_t total_allocs = 0;
    for (const auto& [_, allocs] : allocations_) {
        total_allocs += allocs.size();
    }
    stats.num_allocations = total_allocs;
    
    size_t excess_allocations = total_allocs > stats.num_models
        ? total_allocs - stats.num_models
        : 0;
    stats.fragmentation_pct = (stats.num_models > 0)
        ? std::min(static_cast<size_t>(100),
                   static_cast<size_t>((excess_allocations * 100) / stats.num_models))
        : 0;
    
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
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Recalculate from allocations
    total_vram_used_.store(0, std::memory_order_relaxed);
    total_ram_used_.store(0, std::memory_order_relaxed);
    
    // Reset per-GPU counters
    for (auto& [gpu_id, _] : per_gpu_vram_used_) {
        per_gpu_vram_used_[gpu_id] = 0;
    }
    
    for (const auto& [_, allocs] : allocations_) {
        for (const auto& alloc : allocs) {
            size_t updated_vram = 0;
            if (!tryAddSize(total_vram_used_.load(std::memory_order_relaxed), alloc.vram_bytes, updated_vram)) {
                spdlog::error("GPUMemoryManager::updateMemoryStats: VRAM accounting overflow; clamping to max");
                total_vram_used_.store(std::numeric_limits<size_t>::max(), std::memory_order_relaxed);
            } else {
                total_vram_used_.store(updated_vram, std::memory_order_relaxed);
            }

            size_t updated_ram = 0;
            if (!tryAddSize(total_ram_used_.load(std::memory_order_relaxed), alloc.ram_bytes, updated_ram)) {
                spdlog::error("GPUMemoryManager::updateMemoryStats: RAM accounting overflow; clamping to max");
                total_ram_used_.store(std::numeric_limits<size_t>::max(), std::memory_order_relaxed);
            } else {
                total_ram_used_.store(updated_ram, std::memory_order_relaxed);
            }
            
            // Track per-GPU usage (operator[] auto-initializes to 0)
            if (alloc.vram_bytes > 0) {
                size_t& per_gpu_used = per_gpu_vram_used_[alloc.gpu_device_id];
                size_t updated_per_gpu = 0;
                if (!tryAddSize(per_gpu_used, alloc.vram_bytes, updated_per_gpu)) {
                    spdlog::error("GPUMemoryManager::updateMemoryStats: per-GPU VRAM accounting overflow for GPU {}; clamping to max",
                                  alloc.gpu_device_id);
                    per_gpu_used = std::numeric_limits<size_t>::max();
                } else {
                    per_gpu_used = updated_per_gpu;
                }
            }
        }
    }
}

// Multi-GPU methods (v1.4.0)

void* GPUMemoryManager::allocateGPU(const std::string& model_id, size_t bytes, int gpu_device_id) {
    // Gate through the canonical VRAM policy (edition limit + tenant quotas).
    auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
    if (policy.isGPUEnabled()) {
        if (!policy.TryAllocateGPU(static_cast<uint64_t>(bytes), model_id)) {
            spdlog::error("allocateGPU(gpu={}): canonical VRAM policy rejected {} bytes for model '{}'",
                          gpu_device_id, bytes, model_id);
            return nullptr;
        }
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify GPU is available
    if (!isGPUAvailableNoLock(gpu_health_status_, gpu_device_id)) {
        spdlog::error("GPU {} is not available", gpu_device_id);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    
    // Check per-GPU capacity
    size_t gpu_used = per_gpu_vram_used_[gpu_device_id];
    if (gpu_used >= config_.max_vram_bytes || bytes > (config_.max_vram_bytes - gpu_used)) {
        size_t bytes_mb = bytes / (1024 * 1024);
        size_t available_mb = calculateAvailableBytes(config_.max_vram_bytes, gpu_used) / (1024 * 1024);
        errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, available_mb);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
    
    void* ptr = nullptr;
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        // Set the target GPU device
        if (cudaSetDevice(gpu_device_id) != cudaSuccess) {
            if (policy.isGPUEnabled()) {
                policy.DeallocateGPU(static_cast<uint64_t>(bytes));
            }
            return nullptr;
        }
        
        // Use actual CUDA allocation
        cudaError_t err = cudaMalloc(&ptr, bytes);
        if (err != cudaSuccess) {
            spdlog::error("cudaMalloc failed on GPU {}: {}", gpu_device_id, cudaGetErrorString(err));
            if (policy.isGPUEnabled()) {
                policy.DeallocateGPU(static_cast<uint64_t>(bytes));
            }
            return nullptr;
        }
    } else {
        // Fallback to simulation
        ptr = std::malloc(bytes);
        if (!ptr) {
            size_t bytes_mb = bytes / (1024 * 1024);
            errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
            if (policy.isGPUEnabled()) {
                policy.DeallocateGPU(static_cast<uint64_t>(bytes));
            }
            return nullptr;
        }
    }
#else
    // Simulation mode: use regular malloc when CUDA is not enabled at build time
    ptr = std::malloc(bytes);
    if (!ptr) {
        size_t bytes_mb = bytes / (1024 * 1024);
        errors::logError(errors::ErrorCode::ERR_LLM_GPU_OOM, bytes_mb, 0);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }
#endif
    
    // Track allocation with RAII holder.
    // Guard metadata bookkeeping so low-memory exceptions do not leak raw allocations.
    try {
        MemoryAllocation alloc;
        alloc.model_id = model_id;
        alloc.vram_bytes = bytes;
        alloc.gpu_ptr = ptr;
        alloc.gpu_device_id = gpu_device_id;
        alloc.holder = std::make_shared<detail::MemoryHolder>(
            ptr, bytes, detail::MemoryHolder::Type::GPU, gpu_available_, gpu_device_id
        );
        allocations_[model_id].push_back(std::move(alloc));
    } catch (const std::exception& e) {
        cleanupRawAllocation(ptr, bytes, detail::MemoryHolder::Type::GPU, gpu_available_, gpu_device_id);
        spdlog::error("allocateGPU(gpu_device_id={}) metadata bookkeeping failed for model {}: {}",
                      gpu_device_id, model_id, e.what());
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    } catch (...) {
        cleanupRawAllocation(ptr, bytes, detail::MemoryHolder::Type::GPU, gpu_available_, gpu_device_id);
        spdlog::error("allocateGPU(gpu_device_id={}) metadata bookkeeping failed for model {}: unknown exception",
                      gpu_device_id, model_id);
        if (policy.isGPUEnabled()) {
            policy.DeallocateGPU(static_cast<uint64_t>(bytes));
        }
        return nullptr;
    }

    size_t updated_vram_total = 0;
    if (!tryAddSize(total_vram_used_.load(std::memory_order_relaxed), bytes, updated_vram_total)) {
        spdlog::error("GPUMemoryManager::allocateGPU: global VRAM accounting overflow for model '{}'; clamping to max",
                      model_id);
        total_vram_used_.store(std::numeric_limits<size_t>::max(), std::memory_order_relaxed);
    } else {
        total_vram_used_.store(updated_vram_total, std::memory_order_relaxed);
    }

    size_t& per_gpu_used = per_gpu_vram_used_[gpu_device_id];
    size_t updated_per_gpu = 0;
    if (!tryAddSize(per_gpu_used, bytes, updated_per_gpu)) {
        spdlog::error("GPUMemoryManager::allocateGPU: per-GPU VRAM accounting overflow on GPU {} for model '{}'; clamping to max",
                      gpu_device_id, model_id);
        per_gpu_used = std::numeric_limits<size_t>::max();
    } else {
        per_gpu_used = updated_per_gpu;
    }
    
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
            size_t updated_freed_vram = 0;
            if (!tryAddSize(freed_vram, alloc_it->vram_bytes, updated_freed_vram)) {
                spdlog::error("GPUMemoryManager::freeModel(gpu): freed VRAM overflow for model '{}'; clamping to max",
                              model_id);
                freed_vram = std::numeric_limits<size_t>::max();
            } else {
                freed_vram = updated_freed_vram;
            }

            size_t updated_freed_ram = 0;
            if (!tryAddSize(freed_ram, alloc_it->ram_bytes, updated_freed_ram)) {
                spdlog::error("GPUMemoryManager::freeModel(gpu): freed RAM overflow for model '{}'; clamping to max",
                              model_id);
                freed_ram = std::numeric_limits<size_t>::max();
            } else {
                freed_ram = updated_freed_ram;
            }
            
            // Update per-GPU tracking for this specific allocation
            if (is_target_gpu && alloc_it->vram_bytes > 0) {
                if (per_gpu_vram_used_.find(alloc_it->gpu_device_id) != per_gpu_vram_used_.end()) {
                    size_t& per_gpu_used = per_gpu_vram_used_[alloc_it->gpu_device_id];
                    if (alloc_it->vram_bytes > per_gpu_used) {
                        spdlog::error("GPUMemoryManager::freeModel(gpu): per-GPU VRAM accounting underflow on GPU {}; clamping to 0",
                                      alloc_it->gpu_device_id);
                        per_gpu_used = 0;
                    } else {
                        per_gpu_used -= alloc_it->vram_bytes;
                    }
                }
            }
            
            // Erase triggers holder destructor for automatic cleanup
            alloc_it = allocs.erase(alloc_it);
        } else {
            ++alloc_it;
        }
    }
    
    if (freed_vram > total_vram_used_.load(std::memory_order_relaxed)) {
        spdlog::error("GPUMemoryManager::freeModel(gpu): VRAM accounting underflow for model '{}'; clamping to 0",
                      model_id);
        total_vram_used_.store(0, std::memory_order_relaxed);
    } else {
        total_vram_used_.fetch_sub(freed_vram, std::memory_order_relaxed);
    }

    if (freed_ram > total_ram_used_.load(std::memory_order_relaxed)) {
        spdlog::error("GPUMemoryManager::freeModel(gpu): RAM accounting underflow for model '{}'; clamping to 0",
                      model_id);
        total_ram_used_.store(0, std::memory_order_relaxed);
    } else {
        total_ram_used_.fetch_sub(freed_ram, std::memory_order_relaxed);
    }
    
    if (allocs.empty()) {
        allocations_.erase(it);
    }
    
    spdlog::info("Freed memory for model {} on GPU {}: {} MB VRAM, {} MB RAM",
                 model_id, gpu_device_id,
                 freed_vram / (1024.0 * 1024),
                 freed_ram / (1024.0 * 1024));

    // Notify the canonical policy to keep global VRAM accounting consistent.
    auto& policy = themis::gpu::GPUMemoryManager::GetInstance();
    if (policy.isGPUEnabled() && freed_vram > 0) {
        policy.DeallocateGPU(static_cast<uint64_t>(freed_vram));
    }
    
    return true;
}

size_t GPUMemoryManager::getGPUVRAM([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    return it != per_gpu_vram_used_.end() ? it->second : 0;
}

size_t GPUMemoryManager::getFreeGPUVRAM([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
        return 0;
    }
    size_t used = 0;
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    if (it != per_gpu_vram_used_.end()) {
        used = it->second;
    }
    return used < config_.max_vram_bytes ? (config_.max_vram_bytes - used) : 0;
}

std::vector<int> GPUMemoryManager::getAvailableGPUs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> runtime_available_gpus;
    runtime_available_gpus.reserve(available_gpus_.size());
    for (int gpu_id : available_gpus_) {
        if (isGPUAvailableNoLock(gpu_health_status_, gpu_id)) {
            runtime_available_gpus.push_back(gpu_id);
        }
    }
    return runtime_available_gpus;
}

bool GPUMemoryManager::isGPUAvailable([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return gpu_available_ && isGPUAvailableNoLock(gpu_health_status_, gpu_device_id);
}

bool GPUMemoryManager::enablePeerAccess(int src_gpu, int dst_gpu) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!config_.enable_peer_access) {
        spdlog::warn("Cannot enable peer access: peer access is disabled in configuration");
        return false;
    }

    if (src_gpu == dst_gpu) {
        spdlog::warn("Cannot enable peer access: source and destination GPU are identical ({})", src_gpu);
        return false;
    }
    
    if (!isGPUAvailableNoLock(gpu_health_status_, src_gpu) ||
        !isGPUAvailableNoLock(gpu_health_status_, dst_gpu)) {
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
    
    spdlog::warn("Cannot enable peer access without a usable CUDA GPU backend");
    return false;
}

bool GPUMemoryManager::disablePeerAccess(int src_gpu, int dst_gpu) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!config_.enable_peer_access) {
        spdlog::warn("Cannot disable peer access: peer access is disabled in configuration");
        return false;
    }

    if (src_gpu == dst_gpu) {
        spdlog::warn("Cannot disable peer access: source and destination GPU are identical ({})", src_gpu);
        return false;
    }
    
    if (!isGPUAvailableNoLock(gpu_health_status_, src_gpu) ||
        !isGPUAvailableNoLock(gpu_health_status_, dst_gpu)) {
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
    
    spdlog::warn("Cannot disable peer access without a usable CUDA GPU backend");
    return false;
}

bool GPUMemoryManager::canAccessPeer(int src_gpu, int dst_gpu) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enable_peer_access) {
        return false;
    }

    if (src_gpu == dst_gpu) {
        spdlog::warn("Cannot query peer access: source and destination GPU are identical ({})", src_gpu);
        return false;
    }
    
    if (!isGPUAvailableNoLock(gpu_health_status_, src_gpu) ||
        !isGPUAvailableNoLock(gpu_health_status_, dst_gpu)) {
        spdlog::warn("Cannot query peer access: GPU {} or {} not available", src_gpu, dst_gpu);
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        int can_access = 0;
        cudaError_t err = cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
        if (err != cudaSuccess) {
            spdlog::warn("cudaDeviceCanAccessPeer({} -> {}) failed: {}",
                         src_gpu, dst_gpu, cudaGetErrorString(err));
            return false;
        }
        return can_access != 0;
    }
#endif
    
    return false;
}

void GPUMemoryManager::setGPUTemperatureProviderFn(GPUTemperatureProviderFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    temperature_provider_fn_ = std::move(fn);
}

void GPUMemoryManager::clearGPUTemperatureProviderFn() {
    std::lock_guard<std::mutex> lock(mutex_);
    temperature_provider_fn_ = nullptr;
}

// GPU Health Monitoring Implementation

GPUMemoryManager::GPUStats GPUMemoryManager::getGPUStats([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GPUStats stats = {};
    stats.device_id = gpu_device_id;
    
    if (!isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
        return stats;
    }
    
    // Get VRAM stats
    stats.total_vram_bytes = config_.max_vram_bytes;
    stats.used_vram_bytes = 0;
    
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    if (it != per_gpu_vram_used_.end()) {
        stats.used_vram_bytes = it->second;
    }
    stats.used_vram_bytes = clampUsedVRAM(stats.used_vram_bytes, stats.total_vram_bytes);
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
        stats.utilization_percent = calculateUtilizationPercent(
            stats.used_vram_bytes,
            stats.total_vram_bytes
        );
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
        
        if (isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_id)) {
            // Get VRAM stats
            stats.total_vram_bytes = config_.max_vram_bytes;
            stats.used_vram_bytes = 0;
            
            auto it = per_gpu_vram_used_.find(gpu_id);
            if (it != per_gpu_vram_used_.end()) {
                stats.used_vram_bytes = it->second;
            }
            stats.used_vram_bytes = clampUsedVRAM(stats.used_vram_bytes, stats.total_vram_bytes);
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
                stats.utilization_percent = calculateUtilizationPercent(
                    stats.used_vram_bytes,
                    stats.total_vram_bytes
                );
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

GPUMemoryManager::GPUHealth GPUMemoryManager::getGPUHealth([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    GPUHealth health = {};
    health.device_id = gpu_device_id;
    auto it = gpu_health_data_.find(gpu_device_id);
    if (it != gpu_health_data_.end()) {
        return it->second;
    }

    health.is_available = gpu_available_ &&
        isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id);

    if (!isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
        return health;
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
        auto it = gpu_health_data_.find(gpu_id);
        if (it != gpu_health_data_.end()) {
            health = it->second;
        } else if (isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_id)) {
            health.is_available = gpu_available_ &&
                isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_id);
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
        
        all_health.push_back(health);
    }
    
    return all_health;
}

bool GPUMemoryManager::isGPUHealthy([[maybe_unused]] int gpu_device_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id)) {
        return false;
    }

    auto it = gpu_health_status_.find(gpu_device_id);
    return (it != gpu_health_status_.end()) ? it->second : false;
}

void GPUMemoryManager::markGPUUnhealthy(int gpu_device_id, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id)) {
        spdlog::warn("Ignoring markGPUUnhealthy for untracked GPU {}", gpu_device_id);
        return;
    }

    spdlog::warn("GPU {} marked as unhealthy: {}", gpu_device_id, reason);
    gpu_health_status_[gpu_device_id] = false;
    
    // Update health data
    GPUHealth health = {};
    health.device_id = gpu_device_id;
    health.is_available = gpu_available_ &&
        isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id);
    health.is_healthy = false;
    auto temp_it = gpu_temperatures_.find(gpu_device_id);
    health.temperature_celsius = (temp_it != gpu_temperatures_.end()) ? temp_it->second : 0.0f;
    auto util_it = gpu_utilizations_.find(gpu_device_id);
    health.utilization_percent = (util_it != gpu_utilizations_.end()) ? util_it->second : 0.0f;
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

void GPUMemoryManager::markGPUHealthy([[maybe_unused]] int gpu_device_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id)) {
        spdlog::warn("Ignoring markGPUHealthy for untracked GPU {}", gpu_device_id);
        return;
    }

    if (!gpu_available_) {
        spdlog::warn("Ignoring markGPUHealthy for GPU {} because no usable GPU runtime is available",
                     gpu_device_id);
        return;
    }

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

bool GPUMemoryManager::needsLoadRebalancing([[maybe_unused]] float threshold) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (available_gpus_.size() < 2) {
        return false;  // No need to rebalance with single GPU
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
        total_load += calculateUtilization(used_vram, config_.max_vram_bytes);
        healthy_count++;
    }
    float avg_load = (healthy_count > 0) ? (total_load / healthy_count) : 0.0f;
    
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

void GPUMemoryManager::updateGPUHealth([[maybe_unused]] int gpu_device_id) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||
        !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
        spdlog::warn("updateGPUHealth: ignoring untracked GPU {}", gpu_device_id);
        return;
    }

    // CRITICAL GAP FIX #7: Enhanced temperature monitoring and health detection
    // This would typically query actual GPU hardware
#ifdef THEMIS_ENABLE_CUDA
    if (gpu_available_) {
        cudaError_t set_err = cudaSetDevice(gpu_device_id);
        if (set_err != cudaSuccess) {
            spdlog::warn("[{}] updateGPUHealth: cudaSetDevice({}) failed: {}",
                         static_cast<int>(GPUMemoryErrorCode::GPU_DEVICE_SET_FAILED),
                         gpu_device_id, cudaGetErrorString(set_err));
            gpu_utilizations_[gpu_device_id] = 0.0f;
            gpu_temperatures_[gpu_device_id] = 40.0f;
            // Mark GPU as unhealthy due to device set failure
            gpu_health_status_[gpu_device_id] = false;
            return;
        }

        // Get memory info for utilization
        size_t free_mem = 0;
        size_t total_mem = 0;
        cudaError_t mem_info_err = cudaMemGetInfo(&free_mem, &total_mem);
        float utilization = 0.0f;
        if (mem_info_err != cudaSuccess) {
            spdlog::warn("[{}] updateGPUHealth: cudaMemGetInfo failed for GPU {}: {}",
                         static_cast<int>(GPUMemoryErrorCode::GPU_DEVICE_QUERY_FAILED),
                         gpu_device_id, cudaGetErrorString(mem_info_err));
        } else if (total_mem == 0) {
            spdlog::warn("[{}] updateGPUHealth: cudaMemGetInfo returned zero total memory for GPU {}",
                         static_cast<int>(GPUMemoryErrorCode::GPU_DEVICE_QUERY_FAILED),
                         gpu_device_id);
        } else {
            const size_t used_mem = total_mem - free_mem;
            utilization = (static_cast<float>(used_mem) / static_cast<float>(total_mem)) * 100.0f;
        }

        GPUMemoryManager::NvmlTemperatureFn injected_temp_provider;
        {
            std::lock_guard<std::mutex> provider_lock(nvml_temp_fn_mutex);
            injected_temp_provider = nvml_temp_fn;
        }
        // Capture instance-level provider while we hold the manager lock.
        GPUTemperatureProviderFn instance_temp_provider = temperature_provider_fn_;

        // Avoid invoking external callbacks while holding manager mutex to prevent re-entrant deadlocks.
        lock.unlock();

        std::optional<float> temperature = std::nullopt;
        if (injected_temp_provider) {
            try {
                temperature = injected_temp_provider(gpu_device_id);
            } catch (const std::exception& e) {
                spdlog::warn("[{}] updateGPUHealth: injected NVML temperature provider failed for GPU {}: {}",
                             static_cast<int>(GPUMemoryErrorCode::TEMPERATURE_QUERY_FAILED),
                             gpu_device_id, e.what());
            } catch (...) {
                spdlog::warn("[{}] updateGPUHealth: injected NVML temperature provider failed for GPU {}",
                             static_cast<int>(GPUMemoryErrorCode::TEMPERATURE_QUERY_FAILED),
                             gpu_device_id);
            }
        }
        if (!temperature.has_value()) {
            temperature = queryNvmlTemperatureCelsius(gpu_device_id);
        }
        // Last-resort: try the instance-level provider (e.g test injection or alternative NVML bridge).
        if (!temperature.has_value() && instance_temp_provider) {
            try {
                float temp_out = 0.0f;
                if (instance_temp_provider(gpu_device_id, temp_out)) {
                    temperature = temp_out;
                }
            } catch (const std::exception& e) {
                spdlog::warn("[{}] updateGPUHealth: instance temperature provider failed for GPU {}: {}",
                             static_cast<int>(GPUMemoryErrorCode::TEMPERATURE_QUERY_FAILED),
                             gpu_device_id, e.what());
            } catch (...) {
                spdlog::warn("[{}] updateGPUHealth: instance temperature provider failed for GPU {}",
                             static_cast<int>(GPUMemoryErrorCode::TEMPERATURE_QUERY_FAILED),
                             gpu_device_id);
            }
        }

        float final_temperature = temperature.value_or(40.0f + (utilization * 0.35f));
        
        // CRITICAL GAP FIX #7a: Check temperature thresholds for health detection
        bool is_healthy = true;
        if (final_temperature >= 85.0f) {
            spdlog::error("[{}] GPU {} temperature critical: {:.1f}°C",
                         static_cast<int>(GPUMemoryErrorCode::TEMPERATURE_CRITICAL),
                         gpu_device_id, final_temperature);
            is_healthy = false;
        } else if (final_temperature >= 75.0f) {
            spdlog::warn("[{}] GPU {} thermal throttling likely: {:.1f}°C",
                        static_cast<int>(GPUMemoryErrorCode::THERMAL_THROTTLING),
                        gpu_device_id, final_temperature);
        }

        lock.lock();
        if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||
            !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
            spdlog::warn("updateGPUHealth: dropping writeback for untracked GPU {}", gpu_device_id);
            return;
        }
        
        gpu_utilizations_[gpu_device_id] = utilization;
        gpu_temperatures_[gpu_device_id] = final_temperature;
        
        // CRITICAL GAP FIX #7b: Update health status based on temperature
        gpu_health_status_[gpu_device_id] = is_healthy;
        
        return;
    }
#endif

    // Non-CUDA / no-runtime fallback: preserve diagnostic telemetry only and keep
    // the device in an explicit unavailable state.
    size_t used_vram = 0;
    auto it = per_gpu_vram_used_.find(gpu_device_id);
    if (it != per_gpu_vram_used_.end()) {
        used_vram = it->second;
    }

    const float utilization = calculateUtilization(used_vram, config_.max_vram_bytes) * 100.0f;
    // Prefer the runtime instance provider (set via setGPUTemperatureProviderFn) over
    // the construction-time config provider; this allows test injection and live overrides.
    auto temperature_provider = temperature_provider_fn_
                                    ? temperature_provider_fn_
                                    : config_.temperature_provider_fn;
    lock.unlock();

    std::optional<float> measured_temperature = std::nullopt;
    if (temperature_provider) {
        try {
            float temp_out = 0.0f;
            if (temperature_provider(gpu_device_id, temp_out)) {
                measured_temperature = temp_out;
            }
        } catch (const std::exception& e) {
            spdlog::error("GPU temperature callback failed for device {}: {}",
                          gpu_device_id, e.what());
        } catch (...) {
            spdlog::error("GPU temperature callback failed for device {}", gpu_device_id);
        }
    }

    lock.lock();
    if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||
        !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
        spdlog::warn("updateGPUHealth: dropping writeback for untracked GPU {}", gpu_device_id);
        return;
    }
    gpu_utilizations_[gpu_device_id] = utilization;
    gpu_temperatures_[gpu_device_id] = measured_temperature.value_or(0.0f);
    gpu_health_status_[gpu_device_id] = false;
    const auto error_count = gpu_error_counts_.count(gpu_device_id) > 0
        ? gpu_error_counts_.at(gpu_device_id)
        : 0;
    gpu_health_data_[gpu_device_id] = buildUnavailableGpuHealth(
        gpu_device_id,
        utilization,
        gpu_temperatures_[gpu_device_id],
        error_count,
        "gpu_runtime_unavailable");
}

void GPUMemoryManager::checkGPUHealth([[maybe_unused]] int gpu_device_id) {
    updateGPUHealth(gpu_device_id);

    if (!gpu_available_) {
        markGPUUnhealthy(gpu_device_id, "gpu_runtime_unavailable");
        return;
    }

    bool is_healthy = true;
    std::string reason;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isTrackedGpuNoLock(available_gpus_, gpu_device_id) ||
            !isTrackedGpuHealthEntryNoLock(gpu_health_status_, gpu_device_id)) {
            return;
        }
        auto util_it = gpu_utilizations_.find(gpu_device_id);
        auto temp_it = gpu_temperatures_.find(gpu_device_id);

        // Check temperature
        if (temp_it != gpu_temperatures_.end() && temp_it->second > 85.0f) {
            is_healthy = false;
            reason = "Temperature too high: " + std::to_string(temp_it->second) + "°C";
        }

        // Check utilization
        if (util_it != gpu_utilizations_.end() && util_it->second > 95.0f) {
            is_healthy = false;
            if (!reason.empty()) {
              reason += "; ";
            }
            reason += "Utilization too high: " + std::to_string(util_it->second) + "%";
        }
    }
    
    if (is_healthy) {
        markGPUHealthy(gpu_device_id);
    } else {
        markGPUUnhealthy(gpu_device_id, reason);
    }
}

} // namespace llm
} // namespace themis
