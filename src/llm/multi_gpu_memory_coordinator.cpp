/**
 * @file multi_gpu_memory_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=10; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=7, Debt=0, C=0, H=1, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/multi_gpu_memory_coordinator.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <spdlog/spdlog.h>

// Include CUDA/HIP headers when available
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace llm {

// Private implementation
/** @brief Private implementation. */
class MultiGPUMemoryCoordinator::Impl {
public:
    std::vector<GPUDevice> gpus_;
    bool initialized_ = false;
};

MultiGPUMemoryCoordinator::MultiGPUMemoryCoordinator() 
    : impl_(std::make_unique<Impl>()) {}

MultiGPUMemoryCoordinator::~MultiGPUMemoryCoordinator() = default;

bool MultiGPUMemoryCoordinator::initialize(const std::vector<int>& gpu_ids) {
    if (gpu_ids.empty()) {
        spdlog::error("MultiGPUMemoryCoordinator: No GPU IDs provided");
        return false;
    }
    
    impl_->gpus_.clear();
    
#ifdef THEMIS_ENABLE_CUDA
    // Query CUDA devices
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    
    if (err != cudaSuccess || deviceCount == 0) {
        spdlog::error("MultiGPUMemoryCoordinator: No CUDA devices found - {}", cudaGetErrorString(err));
        return false;
    }
    
    spdlog::info("MultiGPUMemoryCoordinator: Detected {} CUDA device(s)", deviceCount);
    
    // Initialize requested GPU devices
    for (int gpu_id : gpu_ids) {
        if (gpu_id >= deviceCount || gpu_id < 0) {
            spdlog::warn("GPU {} requested but only {} GPUs available, skipping", gpu_id, deviceCount);
            continue;
        }
        
        // REL-40: check cudaSetDevice return value in initialize()
        cudaError_t set_err = cudaSetDevice(gpu_id);
        if (set_err != cudaSuccess) {
            spdlog::warn("MultiGPUMemoryCoordinator: cudaSetDevice({}) failed: {}; skipping GPU",
                         gpu_id, cudaGetErrorString(set_err));
            continue;
        }
        GPUDevice device;
        device.device_id = gpu_id;
        
        // Query device properties
        cudaDeviceProp prop{};
        if (cudaGetDeviceProperties(&prop, gpu_id) == cudaSuccess) {
            device.total_vram_bytes = prop.totalGlobalMem;
            device.compute_capability = prop.major * 10 + prop.minor;
            
            spdlog::info("  GPU {}: {} (Compute {}.{}, {:.2f} GB VRAM)",
                         gpu_id, prop.name, prop.major, prop.minor,
                         device.total_vram_bytes / (1024.0 * 1024.0 * 1024.0));
        } else {
            spdlog::warn("  Failed to query properties for GPU {}", gpu_id);
            device.total_vram_bytes = 0;
            device.compute_capability = 0;
        }
        
        // Query available memory
        size_t free_bytes, total_bytes;
        if (cudaMemGetInfo(&free_bytes, &total_bytes) == cudaSuccess) {
            device.available_vram_bytes = free_bytes;
        } else {
            device.available_vram_bytes = device.total_vram_bytes;
        }
        
        // Query temperature and utilization (if nvml is available)
        device.temperature_celsius = 0.0f;  // Would need NVML for this
        device.utilization_percent = 0.0f;  // Would need NVML for this
        device.is_healthy = true;
        
        impl_->gpus_.push_back(device);
    }
    
#elif defined(THEMIS_ENABLE_HIP)
    // Query HIP/ROCm devices
    int deviceCount = 0;
    hipError_t err = hipGetDeviceCount(&deviceCount);
    
    if (err != hipSuccess || deviceCount == 0) {
        spdlog::error("MultiGPUMemoryCoordinator: No HIP devices found - {}", hipGetErrorString(err));
        return false;
    }
    
    spdlog::info("MultiGPUMemoryCoordinator: Detected {} HIP device(s)", deviceCount);
    
    // Initialize requested GPU devices
    for (int gpu_id : gpu_ids) {
        if (gpu_id >= deviceCount || gpu_id < 0) {
            spdlog::warn("GPU {} requested but only {} GPUs available, skipping", gpu_id, deviceCount);
            continue;
        }
        
        // REL-41: check hipSetDevice return value in initialize()
        hipError_t set_err = hipSetDevice(gpu_id);
        if (set_err != hipSuccess) {
            spdlog::warn("MultiGPUMemoryCoordinator: hipSetDevice({}) failed: {}; skipping GPU",
                         gpu_id, hipGetErrorString(set_err));
            continue;
        }
        GPUDevice device;
        device.device_id = gpu_id;
        
        // Query device properties
        hipDeviceProp_t prop{};
        if (hipGetDeviceProperties(&prop, gpu_id) == hipSuccess) {
            device.total_vram_bytes = prop.totalGlobalMem;
            device.compute_capability = prop.major * 10 + prop.minor;
            
            spdlog::info("  GPU {}: {} (Compute {}.{}, {:.2f} GB VRAM)",
                         gpu_id, prop.name, prop.major, prop.minor,
                         device.total_vram_bytes / (1024.0 * 1024.0 * 1024.0));
        } else {
            spdlog::warn("  Failed to query properties for GPU {}", gpu_id);
            device.total_vram_bytes = 0;
            device.compute_capability = 0;
        }
        
        // Query available memory
        size_t free_bytes, total_bytes;
        if (hipMemGetInfo(&free_bytes, &total_bytes) == hipSuccess) {
            device.available_vram_bytes = free_bytes;
        } else {
            device.available_vram_bytes = device.total_vram_bytes;
        }
        
        device.temperature_celsius = 0.0f;
        device.utilization_percent = 0.0f;
        device.is_healthy = true;
        
        impl_->gpus_.push_back(device);
    }
    
#else
    // CPU-only fallback mode
    spdlog::warn("MultiGPUMemoryCoordinator: No GPU backend available, using CPU fallback");
    
    for (int gpu_id : gpu_ids) {
        GPUDevice device;
        device.device_id = gpu_id;
        device.total_vram_bytes = 24 * 1024 * 1024 * 1024;  // 24GB default (simulated)
        device.available_vram_bytes = 22 * 1024 * 1024 * 1024;  // 22GB available (simulated)
        device.compute_capability = 0;  // 0 indicates CPU simulation mode
        device.is_healthy = true;
        device.temperature_celsius = 45.0f;
        device.utilization_percent = 10.0f;
        
        impl_->gpus_.push_back(device);
    }
#endif
    
    if (impl_->gpus_.empty()) {
        spdlog::error("MultiGPUMemoryCoordinator: No GPUs successfully initialized");
        return false;
    }
    
    impl_->initialized_ = true;
    spdlog::info("MultiGPUMemoryCoordinator: Successfully initialized {} GPU(s)", impl_->gpus_.size());
    return true;
}

MultiGPUMemoryCoordinator::DistributionPlan 
MultiGPUMemoryCoordinator::distributeModelWeights(
    const std::vector<int>& gpu_ids,
    size_t model_size_bytes
) {
    DistributionPlan plan;
    plan.strategy = DistributionStrategy::TENSOR_PARALLEL;
    plan.gpu_ids = gpu_ids;
    plan.tensor_parallel_size = static_cast<int>(gpu_ids.size());
    plan.pipeline_parallel_size = 1;
    
    // Split model evenly across GPUs (tensor parallelism)
    size_t shard_size = model_size_bytes / gpu_ids.size();
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        plan.shard_sizes.push_back(shard_size);
    }
    
    // Enable P2P for all GPU pairs
    plan.enable_p2p = true;
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        for (size_t j = i + 1; j < gpu_ids.size(); ++j) {
            plan.p2p_pairs.emplace_back(gpu_ids[i], gpu_ids[j]);
        }
    }
    
    plan.description = "Tensor Parallel: Each layer split across " + 
                       std::to_string(gpu_ids.size()) + " GPUs";
    
    return plan;
}

MultiGPUMemoryCoordinator::DistributionPlan 
MultiGPUMemoryCoordinator::distributeLayers(
    const std::vector<int>& gpu_ids,
    size_t num_layers,
    size_t layer_size_bytes
) {
    DistributionPlan plan;
    plan.strategy = DistributionStrategy::PIPELINE_PARALLEL;
    plan.gpu_ids = gpu_ids;
    plan.tensor_parallel_size = 1;
    plan.pipeline_parallel_size = static_cast<int>(gpu_ids.size());
    
    // Distribute layers across GPUs
    size_t layers_per_gpu = num_layers / gpu_ids.size();
    size_t remaining_layers = num_layers % gpu_ids.size();
    
    size_t current_layer = 0;
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        std::vector<int> gpu_layers;
        size_t num_layers_this_gpu = layers_per_gpu + (i < remaining_layers ? 1 : 0);
        
        for (size_t j = 0; j < num_layers_this_gpu; ++j) {
            gpu_layers.push_back(static_cast<int>(current_layer++));
        }
        
        plan.layer_assignments.push_back(gpu_layers);
        plan.shard_sizes.push_back(num_layers_this_gpu * layer_size_bytes);
    }
    
    // Enable P2P for adjacent GPUs (pipeline stages)
    plan.enable_p2p = true;
    for (size_t i = 0; i + 1 < gpu_ids.size(); ++i) {
        plan.p2p_pairs.emplace_back(gpu_ids[i], gpu_ids[i + 1]);
    }
    
    plan.description = "Pipeline Parallel: " + std::to_string(num_layers) + 
                       " layers distributed across " + std::to_string(gpu_ids.size()) + " GPUs";
    
    return plan;
}

MultiGPUMemoryCoordinator::DistributionPlan 
MultiGPUMemoryCoordinator::balanceInferenceLoad(
    const std::vector<int>& gpu_ids,
    size_t total_batch_size
) {
    DistributionPlan plan;
    plan.strategy = DistributionStrategy::DATA_PARALLEL;
    plan.gpu_ids = gpu_ids;
    
    // Get GPU utilization and distribute load inversely
    std::vector<float> utilizations = {};

    for (int gpu_id : gpu_ids) {
        auto gpu = getGPUInfo(gpu_id);
        utilizations.push_back(gpu.utilization_percent);
    }
    
    // Calculate inverse utilization for load balancing
    float sum_inverse = 0.0f;
    std::vector<float> inverse_util = {};

    for (float util : utilizations) {
        float inv = 1.0f / (util + 1.0f);  // +1 to avoid division by zero
        inverse_util.push_back(inv);
        sum_inverse += inv;
    }
    
    // Distribute batch proportionally to inverse utilization
    size_t assigned = 0;
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        size_t batch_for_gpu = static_cast<size_t>(
            total_batch_size * (inverse_util[i] / sum_inverse)
        );
        
        // Ensure at least 1 if total_batch_size > 0
        if (i == static_cast<int>(gpu_ids.size()) - 1) {
            batch_for_gpu = total_batch_size - assigned;  // Give remainder to last GPU
        }
        
        plan.batch_assignments.push_back(static_cast<int>(batch_for_gpu));
        assigned += batch_for_gpu;
    }
    
    plan.description = "Data Parallel: Batch size " + std::to_string(total_batch_size) +
                       " distributed across " + std::to_string(gpu_ids.size()) + " GPUs";
    
    return plan;
}

bool MultiGPUMemoryCoordinator::enableP2P(const std::vector<int>& gpu_ids) {
    if (static_cast<int>(gpu_ids.size()) < 2) {
        spdlog::warn("MultiGPUMemoryCoordinator::enableP2P: Need at least 2 GPUs");
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
    spdlog::info("MultiGPUMemoryCoordinator: Enabling P2P access for {} GPUs", gpu_ids.size());
    
    int success_count = 0;
    int fail_count = 0;
    
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        for (size_t j = i + 1; j < gpu_ids.size(); ++j) {
            int src_gpu = gpu_ids[i];
            int dst_gpu = gpu_ids[j];
            
            // Check if P2P access is possible
            int can_access_forward = 0;
            int can_access_backward = 0;
            
            cudaError_t err1 = cudaDeviceCanAccessPeer(&can_access_forward, src_gpu, dst_gpu);
            cudaError_t err2 = cudaDeviceCanAccessPeer(&can_access_backward, dst_gpu, src_gpu);
            
            if (err1 != cudaSuccess || err2 != cudaSuccess) {
                spdlog::warn("  Failed to check P2P capability: GPU {} <-> GPU {}", src_gpu, dst_gpu);
                fail_count++;
                continue;
            }
            
            // Enable P2P in both directions
            if (can_access_forward) {
                // REL-42: check cudaSetDevice return value before enabling P2P (forward)
                cudaError_t set_err = cudaSetDevice(src_gpu);
                if (set_err != cudaSuccess) {
                    spdlog::warn("  cudaSetDevice({}) failed before P2P enable: {}",
                                 src_gpu, cudaGetErrorString(set_err));
                    fail_count++;
                } else {
                    cudaError_t p2p_err = cudaDeviceEnablePeerAccess(dst_gpu, 0);
                    if (p2p_err == cudaSuccess || p2p_err == cudaErrorPeerAccessAlreadyEnabled) {
                        spdlog::info("  P2P enabled: GPU {} -> GPU {}", src_gpu, dst_gpu);
                        success_count++;
                    } else {
                        spdlog::warn("  Failed to enable P2P: GPU {} -> GPU {} - {}",
                                     src_gpu, dst_gpu, cudaGetErrorString(p2p_err));
                        fail_count++;
                    }
                }
            } else {
                spdlog::warn("  P2P not supported: GPU {} -> GPU {}", src_gpu, dst_gpu);
                fail_count++;
            }
            
            if (can_access_backward) {
                // REL-43: check cudaSetDevice return value before enabling P2P (backward)
                cudaError_t set_err = cudaSetDevice(dst_gpu);
                if (set_err != cudaSuccess) {
                    spdlog::warn("  cudaSetDevice({}) failed before P2P enable: {}",
                                 dst_gpu, cudaGetErrorString(set_err));
                    fail_count++;
                } else {
                    cudaError_t p2p_err = cudaDeviceEnablePeerAccess(src_gpu, 0);
                    if (p2p_err == cudaSuccess || p2p_err == cudaErrorPeerAccessAlreadyEnabled) {
                        spdlog::info("  P2P enabled: GPU {} -> GPU {}", dst_gpu, src_gpu);
                        success_count++;
                    } else {
                        spdlog::warn("  Failed to enable P2P: GPU {} -> GPU {} - {}",
                                     dst_gpu, src_gpu, cudaGetErrorString(p2p_err));
                        fail_count++;
                    }
                }
            } else {
                spdlog::warn("  P2P not supported: GPU {} -> GPU {}", dst_gpu, src_gpu);
                fail_count++;
            }
        }
    }
    
    spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)", 
                 success_count, fail_count);
    return success_count > 0;
    
#elif defined(THEMIS_ENABLE_HIP)
    spdlog::info("MultiGPUMemoryCoordinator: Enabling P2P access for {} HIP GPUs", gpu_ids.size());
    
    int success_count = 0;
    int fail_count = 0;
    
    for (size_t i = 0; i < gpu_ids.size(); ++i) {
        for (size_t j = i + 1; j < gpu_ids.size(); ++j) {
            int src_gpu = gpu_ids[i];
            int dst_gpu = gpu_ids[j];
            
            // Check if P2P access is possible
            int can_access_forward = 0;
            int can_access_backward = 0;
            
            hipError_t err1 = hipDeviceCanAccessPeer(&can_access_forward, src_gpu, dst_gpu);
            hipError_t err2 = hipDeviceCanAccessPeer(&can_access_backward, dst_gpu, src_gpu);
            
            if (err1 != hipSuccess || err2 != hipSuccess) {
                spdlog::warn("  Failed to check P2P capability: GPU {} <-> GPU {}", src_gpu, dst_gpu);
                fail_count++;
                continue;
            }
            
            // Enable P2P in both directions
            if (can_access_forward) {
                // REL-44: check hipSetDevice return value before enabling P2P (forward)
                hipError_t set_err = hipSetDevice(src_gpu);
                if (set_err != hipSuccess) {
                    spdlog::warn("  hipSetDevice({}) failed before P2P enable: {}",
                                 src_gpu, hipGetErrorString(set_err));
                    fail_count++;
                } else {
                    hipError_t p2p_err = hipDeviceEnablePeerAccess(dst_gpu, 0);
                    if (p2p_err == hipSuccess || p2p_err == hipErrorPeerAccessAlreadyEnabled) {
                        spdlog::info("  P2P enabled: GPU {} -> GPU {}", src_gpu, dst_gpu);
                        success_count++;
                    } else {
                        spdlog::warn("  Failed to enable P2P: GPU {} -> GPU {} - {}",
                                     src_gpu, dst_gpu, hipGetErrorString(p2p_err));
                        fail_count++;
                    }
                }
            }
            
            if (can_access_backward) {
                // REL-45: check hipSetDevice return value before enabling P2P (backward)
                hipError_t set_err = hipSetDevice(dst_gpu);
                if (set_err != hipSuccess) {
                    spdlog::warn("  hipSetDevice({}) failed before P2P enable: {}",
                                 dst_gpu, hipGetErrorString(set_err));
                    fail_count++;
                } else {
                    hipError_t p2p_err = hipDeviceEnablePeerAccess(src_gpu, 0);
                    if (p2p_err == hipSuccess || p2p_err == hipErrorPeerAccessAlreadyEnabled) {
                        spdlog::info("  P2P enabled: GPU {} -> GPU {}", dst_gpu, src_gpu);
                        success_count++;
                    } else {
                        spdlog::warn("  Failed to enable P2P: GPU {} -> GPU {} - {}",
                                     dst_gpu, src_gpu, hipGetErrorString(p2p_err));
                        fail_count++;
                    }
                }
            }
        }
    }
    
    spdlog::info("MultiGPUMemoryCoordinator: P2P setup complete ({} success, {} failed)", 
                 success_count, fail_count);
    return success_count > 0;
    
#else
    // CPU-only fallback
    spdlog::warn("MultiGPUMemoryCoordinator::enableP2P: No GPU backend available");
    return true;  // Return true in simulation mode
#endif
}

MultiGPUMemoryCoordinator::GPUDevice 
MultiGPUMemoryCoordinator::getGPUInfo([[maybe_unused]] int device_id) const {
    for (const auto& gpu : impl_->gpus_) {
        if (gpu.device_id == device_id) {
            return gpu;
        }
    }
    
    // Return default device if not found
    GPUDevice device;
    device.device_id = device_id;
    device.is_healthy = false;
    return device;
}

std::vector<MultiGPUMemoryCoordinator::GPUDevice> 
MultiGPUMemoryCoordinator::getAllGPUs() const {
    return impl_->gpus_;
}

int MultiGPUMemoryCoordinator::getLeastLoadedGPU() const {
    if (impl_->gpus_.empty()) {
        return -1;
    }
    
    int least_loaded = impl_->gpus_[0].device_id;
    float min_util = impl_->gpus_[0].utilization_percent;
    
    for (const auto& gpu : impl_->gpus_) {
        if (gpu.is_healthy && gpu.utilization_percent < min_util) {
            min_util = gpu.utilization_percent;
            least_loaded = gpu.device_id;
        }
    }
    
    return least_loaded;
}

bool MultiGPUMemoryCoordinator::canAccessPeer(int src_gpu, int dst_gpu) const {
    if (src_gpu == dst_gpu) {
        return false;  // Same GPU, no P2P needed
    }
    
#ifdef THEMIS_ENABLE_CUDA
    int can_access = 0;
    cudaError_t err = cudaDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
    if (err != cudaSuccess) {
        spdlog::debug("Failed to check P2P capability: GPU {} -> GPU {} - {}", 
                      src_gpu, dst_gpu, cudaGetErrorString(err));
        return false;
    }
    return can_access != 0;
    
#elif defined(THEMIS_ENABLE_HIP)
    int can_access = 0;
    hipError_t err = hipDeviceCanAccessPeer(&can_access, src_gpu, dst_gpu);
    if (err != hipSuccess) {
        spdlog::debug("Failed to check P2P capability: GPU {} -> GPU {} - {}", 
                      src_gpu, dst_gpu, hipGetErrorString(err));
        return false;
    }
    return can_access != 0;
    
#else
    // CPU-only fallback - simulate P2P capability
    return true;
#endif
}

bool MultiGPUMemoryCoordinator::transferP2P(
    int src_gpu,
    int dst_gpu,
    const void* src_ptr,
    void* dst_ptr,
    size_t bytes
) {
    if (src_gpu == dst_gpu || !src_ptr || !dst_ptr || bytes == 0) {
        spdlog::warn("MultiGPUMemoryCoordinator::transferP2P: Invalid parameters");
        return false;
    }
    
#ifdef THEMIS_ENABLE_CUDA
    // Use cudaMemcpyPeer for direct GPU-to-GPU transfer
    cudaError_t err = cudaMemcpyPeer(dst_ptr, dst_gpu, src_ptr, src_gpu, bytes);
    if (err != cudaSuccess) {
        spdlog::error("P2P transfer failed: GPU {} -> GPU {} ({} bytes) - {}", 
                      src_gpu, dst_gpu, bytes, cudaGetErrorString(err));
        return false;
    }
    
    spdlog::debug("P2P transfer success: GPU {} -> GPU {} ({} bytes)", 
                  src_gpu, dst_gpu, bytes);
    return true;
    
#elif defined(THEMIS_ENABLE_HIP)
    // Use hipMemcpyPeer for direct GPU-to-GPU transfer
    hipError_t err = hipMemcpyPeer(dst_ptr, dst_gpu, src_ptr, src_gpu, bytes);
    if (err != hipSuccess) {
        spdlog::error("P2P transfer failed: GPU {} -> GPU {} ({} bytes) - {}", 
                      src_gpu, dst_gpu, bytes, hipGetErrorString(err));
        return false;
    }
    
    spdlog::debug("P2P transfer success: GPU {} -> GPU {} ({} bytes)", 
                  src_gpu, dst_gpu, bytes);
    return true;
    
#else
    // CPU-only fallback - simulate transfer
    spdlog::debug("Simulated P2P transfer: GPU {} -> GPU {} ({} bytes)", 
                  src_gpu, dst_gpu, bytes);
    return true;
#endif
}

void MultiGPUMemoryCoordinator::synchronizeAll() {
#ifdef THEMIS_ENABLE_CUDA
    // Synchronize all GPUs
    for (const auto& gpu : impl_->gpus_) {
        // REL-46: check cudaSetDevice return value before synchronize
        cudaError_t set_err = cudaSetDevice(gpu.device_id);
        if (set_err != cudaSuccess) {
            spdlog::warn("synchronizeAll: cudaSetDevice({}) failed: {}; skipping sync",
                         gpu.device_id, cudaGetErrorString(set_err));
            continue;
        }
        cudaError_t err = cudaDeviceSynchronize();
        if (err != cudaSuccess) {
            spdlog::warn("Failed to synchronize GPU {} - {}", 
                         gpu.device_id, cudaGetErrorString(err));
        }
    }
    
#elif defined(THEMIS_ENABLE_HIP)
    // Synchronize all GPUs
    for (const auto& gpu : impl_->gpus_) {
        // REL-47: check hipSetDevice return value before synchronize
        hipError_t set_err = hipSetDevice(gpu.device_id);
        if (set_err != hipSuccess) {
            spdlog::warn("synchronizeAll: hipSetDevice({}) failed: {}; skipping sync",
                         gpu.device_id, hipGetErrorString(set_err));
            continue;
        }
        hipError_t err = hipDeviceSynchronize();
        if (err != hipSuccess) {
            spdlog::warn("Failed to synchronize GPU {} - {}", 
                         gpu.device_id, hipGetErrorString(err));
        }
    }
    
#else
    // CPU-only fallback - nothing to synchronize
    spdlog::debug("MultiGPUMemoryCoordinator::synchronizeAll: CPU mode, no GPU sync needed");
#endif
}

std::vector<std::pair<int, bool>> 
MultiGPUMemoryCoordinator::getHealthStatus() const {
    std::vector<std::pair<int, bool>> status;
    for (const auto& gpu : impl_->gpus_) {
        status.emplace_back(gpu.device_id, gpu.is_healthy);
    }
    return status;
}

} // namespace llm
} // namespace themis
