#include "llm/multi_gpu_memory_coordinator.h"
#include <algorithm>
#include <numeric>
#include <cmath>

namespace themis {
namespace llm {

// Private implementation
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
        return false;
    }
    
    impl_->gpus_.clear();
    
    // Initialize GPU devices (stub - would query actual GPUs)
    for (int gpu_id : gpu_ids) {
        GPUDevice device;
        device.device_id = gpu_id;
        device.total_vram_bytes = 24ULL * 1024 * 1024 * 1024;  // 24GB default
        device.available_vram_bytes = 22ULL * 1024 * 1024 * 1024;  // 22GB available
        device.compute_capability = 80;  // SM 8.0 (A100/RTX 30xx)
        device.is_healthy = true;
        device.temperature_celsius = 45.0f;
        device.utilization_percent = 10.0f;
        
        impl_->gpus_.push_back(device);
    }
    
    impl_->initialized_ = true;
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
    std::vector<float> utilizations;
    for (int gpu_id : gpu_ids) {
        auto gpu = getGPUInfo(gpu_id);
        utilizations.push_back(gpu.utilization_percent);
    }
    
    // Calculate inverse utilization for load balancing
    float sum_inverse = 0.0f;
    std::vector<float> inverse_util;
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
        if (i == gpu_ids.size() - 1) {
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
    // Stub implementation - would enable CUDA P2P access
    // In production: cudaDeviceEnablePeerAccess for each GPU pair
    return gpu_ids.size() >= 2;
}

MultiGPUMemoryCoordinator::GPUDevice 
MultiGPUMemoryCoordinator::getGPUInfo(int device_id) const {
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
    // Stub implementation - would check CUDA P2P capabilities
    // In production: cudaDeviceCanAccessPeer
    return src_gpu != dst_gpu;
}

bool MultiGPUMemoryCoordinator::transferP2P(
    int src_gpu,
    int dst_gpu,
    const void* src_ptr,
    void* dst_ptr,
    size_t bytes
) {
    // Stub implementation - would perform actual P2P transfer
    // In production: cudaMemcpyPeer
    return src_gpu != dst_gpu && src_ptr != nullptr && dst_ptr != nullptr && bytes > 0;
}

void MultiGPUMemoryCoordinator::synchronizeAll() {
    // Stub implementation - would synchronize all GPU streams
    // In production: cudaDeviceSynchronize for each GPU
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
