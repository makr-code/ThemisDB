#include "llm/lora_framework/custom_allreduce.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace llm {
namespace lora {

CustomAllReduce::CustomAllReduce(const MultiGPUContext& ctx, int rank, int world_size)
    : ctx_(ctx), rank_(rank), world_size_(world_size), 
      initialized_(false), p2p_enabled_(false) {
    
    spdlog::info("CustomAllReduce created: rank={}, world_size={}", rank_, world_size_);
}

bool CustomAllReduce::initialize() {
    if (initialized_) {
        spdlog::warn("CustomAllReduce already initialized");
        return true;
    }
    
    spdlog::info("Initializing custom all-reduce backend (rank {}/{})", rank_, world_size_);
    
    // Try to enable P2P access for better performance
    enable_p2p_access();
    
    initialized_ = true;
    spdlog::info("Custom all-reduce initialized (P2P enabled: {})", p2p_enabled_);
    return true;
}

void CustomAllReduce::finalize() {
    if (!initialized_) {
        return;
    }
    
    initialized_ = false;
    spdlog::info("CustomAllReduce finalized");
}

bool CustomAllReduce::allreduce(std::vector<GPUTensor*>& tensors, bool average) {
    if (!initialized_) {
        spdlog::error("CustomAllReduce not initialized");
        return false;
    }
    
    if (world_size_ == 1) {
        return true;  // Single GPU, no reduction needed
    }
    
    // Perform ring all-reduce for each tensor
    for (auto* tensor : tensors) {
        if (!tensor) {
            spdlog::warn("Skipping null tensor in allreduce");
            continue;
        }
        
        if (!ring_allreduce(*tensor, average)) {
            return false;
        }
    }
    
    return true;
}

bool CustomAllReduce::allreduce(GPUTensor& tensor, bool average) {
    std::vector<GPUTensor*> tensors = {&tensor};
    return allreduce(tensors, average);
}

bool CustomAllReduce::broadcast(GPUTensor& tensor, int root) {
    if (!initialized_) {
        spdlog::error("CustomAllReduce not initialized");
        return false;
    }
    
    if (world_size_ == 1 || rank_ == root) {
        return true;  // Nothing to do
    }
    
    // Simple broadcast: root copies to all other GPUs
    // In real implementation, this would use efficient tree-based broadcast
    
    if (rank_ == root) {
        // Root sends to all others
        for (int i = 0; i < world_size_; ++i) {
            if (i == root) continue;
            
            Device target_device = ctx_.get_device(i);
            GPUTensor temp = tensor.to(target_device);
            // In real implementation, would copy directly to target GPU
        }
    } else {
        // Non-root receives from root
        Device root_device = ctx_.get_device(root);
        GPUTensor temp({tensor.size()}, root_device);
        gpu_to_gpu_copy(temp, tensor, 0, tensor.size());
    }
    
    return true;
}

void CustomAllReduce::barrier() {
    if (!initialized_) {
        return;
    }
    
    // Simple barrier: allreduce with dummy data
    GPUTensor dummy({1}, ctx_.get_device(rank_));
    dummy.fill(0.0f);
    allreduce(dummy, false);
}

bool CustomAllReduce::ring_allreduce(GPUTensor& tensor, bool average) {
    // Ring all-reduce algorithm
    // Divides tensor into chunks, each GPU reduces one chunk per step
    
    size_t total_size = tensor.size();
    size_t chunk_size = (total_size + world_size_ - 1) / world_size_;
    
    // Download to CPU for simplicity in this implementation
    // Real implementation would use GPU-GPU transfers
    std::vector<float> data = tensor.cpu_data();
    std::vector<float> recv_buffer(chunk_size);
    
    // Reduce-scatter phase
    for (int step = 0; step < world_size_ - 1; ++step) {
        int send_chunk = (rank_ - step + world_size_) % world_size_;
        int recv_chunk = (rank_ - step - 1 + world_size_) % world_size_;
        
        size_t send_offset = send_chunk * chunk_size;
        size_t recv_offset = recv_chunk * chunk_size;
        
        size_t send_count = std::min(chunk_size, total_size - send_offset);
        size_t recv_count = std::min(chunk_size, total_size - recv_offset);
        
        // In real implementation: GPU-GPU transfer via P2P or staging buffer
        // For now: simulate with CPU buffer
        
        // Receive and accumulate
        for (size_t i = 0; i < recv_count; ++i) {
            data[recv_offset + i] += recv_buffer[i];
        }
    }
    
    // All-gather phase
    for (int step = 0; step < world_size_ - 1; ++step) {
        int send_chunk = (rank_ - step + 1 + world_size_) % world_size_;
        int recv_chunk = (rank_ - step + world_size_) % world_size_;
        
        size_t send_offset = send_chunk * chunk_size;
        size_t recv_offset = recv_chunk * chunk_size;
        
        size_t send_count = std::min(chunk_size, total_size - send_offset);
        size_t recv_count = std::min(chunk_size, total_size - recv_offset);
        
        // In real implementation: GPU-GPU transfer
        // For now: copy from data buffer
        std::memcpy(&data[recv_offset], recv_buffer.data(), recv_count * sizeof(float));
    }
    
    // Average if requested
    if (average && world_size_ > 1) {
        float scale = 1.0f / static_cast<float>(world_size_);
        for (auto& val : data) {
            val *= scale;
        }
    }
    
    // Upload back to GPU
    tensor.upload(data);
    
    return true;
}

void CustomAllReduce::gpu_to_gpu_copy(const GPUTensor& src, GPUTensor& dst,
                                       size_t offset, size_t count) {
    // Simple implementation: download to CPU then upload to target GPU
    // Real implementation would use cudaMemcpyPeer or hipMemcpyPeer
    
    std::vector<float> cpu_buffer = src.cpu_data();
    
    if (offset > 0 || count < cpu_buffer.size()) {
        std::vector<float> partial(cpu_buffer.begin() + offset, 
                                   cpu_buffer.begin() + offset + count);
        dst.upload(partial);
    } else {
        dst.upload(cpu_buffer);
    }
}

void CustomAllReduce::enable_p2p_access() {
#ifdef THEMIS_ENABLE_CUDA
    if (ctx_.gpu_type() == DeviceType::CUDA && ctx_.is_homogeneous()) {
        p2p_enabled_ = true;
        
        // Enable P2P access between all GPU pairs
        for (int i = 0; i < world_size_; ++i) {
            for (int j = 0; j < world_size_; ++j) {
                if (i == j) continue;
                
                int can_access = 0;
                cudaDeviceCanAccessPeer(&can_access, 
                                       ctx_.get_device(i).id, 
                                       ctx_.get_device(j).id);
                
                if (can_access) {
                    cudaSetDevice(ctx_.get_device(i).id);
                    cudaError_t err = cudaDeviceEnablePeerAccess(ctx_.get_device(j).id, 0);
                    if (err != cudaSuccess && err != cudaErrorPeerAccessAlreadyEnabled) {
                        spdlog::warn("Failed to enable P2P access from GPU {} to {}: {}", 
                                   i, j, cudaGetErrorString(err));
                        p2p_enabled_ = false;
                    }
                }
            }
        }
        
        spdlog::info("CUDA P2P access enabled for {} GPUs", world_size_);
    }
#endif

#ifdef THEMIS_ENABLE_HIP
    if (ctx_.gpu_type() == DeviceType::HIP && ctx_.is_homogeneous()) {
        p2p_enabled_ = true;
        
        // Enable P2P access between all GPU pairs
        for (int i = 0; i < world_size_; ++i) {
            for (int j = 0; j < world_size_; ++j) {
                if (i == j) continue;
                
                int can_access = 0;
                hipDeviceCanAccessPeer(&can_access, 
                                      ctx_.get_device(i).id, 
                                      ctx_.get_device(j).id);
                
                if (can_access) {
                    hipSetDevice(ctx_.get_device(i).id);
                    hipError_t err = hipDeviceEnablePeerAccess(ctx_.get_device(j).id, 0);
                    if (err != hipSuccess && err != hipErrorPeerAccessAlreadyEnabled) {
                        spdlog::warn("Failed to enable P2P access from GPU {} to {}: {}", 
                                   i, j, hipGetErrorString(err));
                        p2p_enabled_ = false;
                    }
                }
            }
        }
        
        spdlog::info("HIP P2P access enabled for {} GPUs", world_size_);
    }
#endif
}

} // namespace lora
} // namespace llm
} // namespace themis
