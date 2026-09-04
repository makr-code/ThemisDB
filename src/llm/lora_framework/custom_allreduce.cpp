/**
 * @file custom_allreduce.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=11; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=5, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
    
    if (tensors.empty() || world_size_ == 1) {
        return true;  // Single GPU, no reduction needed
    }

    std::vector<std::vector<float>> contributions;
    contributions.reserve(tensors.size());
    std::size_t tensor_size = 0;

    for (auto* tensor : tensors) {
        if (!tensor) {
            spdlog::warn("Skipping null tensor in allreduce");
            continue;
        }

        auto data = tensor->cpu_data();
        if (tensor_size == 0) {
            tensor_size = data.size();
        } else if (static_cast<int>(data.size()) != tensor_size) {
            spdlog::error("CustomAllReduce shape mismatch: {} vs {}",static_cast<int>(data.size()), tensor_size);
            return false;
        }

        contributions.push_back(std::move(data));
    }

    if (contributions.empty()) {
        return true;
    }

    std::vector<float> reduced(tensor_size, 0.0f);
    for (const auto& contribution : contributions) {
        for (std::size_t i = 0; i < tensor_size; ++i) {
            reduced[i] += contribution[i];
        }
    }

    if (average) {
        const float scale = 1.0f / static_cast<float>(contributions.size());
        for (float& value : reduced) {
            value *= scale;
        }
    }

    for (auto* tensor : tensors) {
        if (!tensor) {
            continue;
        }
        tensor->upload(reduced);
    }

    ctx_.synchronize_all();
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
    
    if (world_size_ == 1) {
        return true;  // Single GPU, no broadcast needed
    }
    
    // Simple broadcast: root copies to all other GPUs
    // Note: In a real distributed implementation with multiple processes,
    // this would use MPI or network communication. For single-process
    // multi-GPU, we copy data between GPU memory spaces.
    
    if (rank_ == root) {
        // Root broadcasts to all other GPUs
        for (int i = 0; i < world_size_; ++i) {
            if (i == root) {
              continue;
            }
            
            Device target_device = ctx_.get_device(i);
            GPUTensor target_tensor({tensor.size()}, target_device);
            gpu_to_gpu_copy(tensor, target_tensor, 0,static_cast<int>(tensor.size()));
        }
    }
    
    // All GPUs wait for broadcast to complete
    ctx_.synchronize_all();
    
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

void CustomAllReduce::setRingAllreduceFn(RingAllreduceFn fn) {
    ring_allreduce_fn_ = std::move(fn);
}

bool CustomAllReduce::ring_allreduce(GPUTensor& tensor, bool average) {
    if (ring_allreduce_fn_) {
        return (*ring_allreduce_fn_)(tensor, average);
    }

    if (world_size_ == 1) {
        return true;  // No reduction needed
    }

    spdlog::error(
        "CustomAllReduce ring_allreduce requires an injected backend when world_size > 1");
    return false;
}

void CustomAllReduce::gpu_to_gpu_copy(const GPUTensor& src, GPUTensor& dst,
                                       size_t offset, size_t count) {
    // Simple implementation: download to CPU then upload to target GPU
    // Real implementation would use cudaMemcpyPeer or hipMemcpyPeer
    
    std::vector<float> cpu_buffer = src.cpu_data();
    
    if (offset > 0  || static_cast<size_t>(count) <static_cast<int>(cpu_buffer.size())) {
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
                if (i == j) {
                  continue;
                }
                
                int can_access = 0;
                if (cudaDeviceCanAccessPeer(&can_access,
                                            ctx_.get_device(i).id,
                                            ctx_.get_device(j).id) != cudaSuccess) {
                    spdlog::warn("CustomAllReduce: cudaDeviceCanAccessPeer({},{}) failed; skipping P2P",
                                 i, j);
                    can_access = 0;
                }

                if (can_access) {
                    // REL-38: check cudaSetDevice return value before enabling P2P
                    cudaError_t set_err = cudaSetDevice(ctx_.get_device(i).id);
                    if (set_err != cudaSuccess) {
                        spdlog::warn("CustomAllReduce::enable_p2p_access: cudaSetDevice({}) failed: {}",
                                     ctx_.get_device(i).id, cudaGetErrorString(set_err));
                        p2p_enabled_ = false;
                        continue;
                    }
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
                if (i == j) {
                  continue;
                }
                
                int can_access = 0;
                if (hipDeviceCanAccessPeer(&can_access,
                                           ctx_.get_device(i).id,
                                           ctx_.get_device(j).id) != hipSuccess) {
                    spdlog::warn("CustomAllReduce: hipDeviceCanAccessPeer({},{}) failed; skipping P2P",
                                 i, j);
                    can_access = 0;
                }

                if (can_access) {
                    // REL-39: check hipSetDevice return value before enabling P2P
                    hipError_t set_err = hipSetDevice(ctx_.get_device(i).id);
                    if (set_err != hipSuccess) {
                        spdlog::warn("CustomAllReduce::enable_p2p_access: hipSetDevice({}) failed: {}",
                                     ctx_.get_device(i).id, hipGetErrorString(set_err));
                        p2p_enabled_ = false;
                        continue;
                    }
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
