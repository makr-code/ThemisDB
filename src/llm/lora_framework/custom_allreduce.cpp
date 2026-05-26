/*
 * ThemisDB | File: custom_allreduce.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 284
 * Open Issues: TODOs=1, Stubs=3, Gaps=11, Unimpl=1, Mock=1, Sim=5, Debt=0
 * Gap Correlation: internal=11 | external_v3=42 | delta=31 | status=divergent
 * External Severity (v3): C=0, H=37, M=5
 * PR: #578 [LoRA Phase 10.5] Implement Multi-GPU Training Support (2026-03-11T18:14:05Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
            if (i == root) continue;
            
            Device target_device = ctx_.get_device(i);
            GPUTensor target_tensor({tensor.size()}, target_device);
            gpu_to_gpu_copy(tensor, target_tensor, 0, tensor.size());
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

    // STUB/SIMULATION NOTE:
    // Purpose: Allow single-process multi-GPU training to run an all-reduce step
    //          without NCCL or MPI by doing peer reads through the CPU.
    // Activation: Always when world_size_ > 1, no NCCL/MPI backend is linked,
    //             and no RingAllreduceFn has been injected via setRingAllreduceFn().
    // Production Delta: For `world_size_ > 1`, all ranks read the same in-process
    //                   `tensor.cpu_data()` snapshot — peer-to-peer GPU copies are
    //                   not performed.  Gradients from ranks ≠ rank_ are the
    //                   initiating rank's own gradients, not their real gradients.
    //                   The "all-reduce" is mathematically incorrect for true
    //                   multi-process distributed training.
    // Removal Plan: Inject a real NCCL/RCCL/MPI all-reduce via setRingAllreduceFn().
    //               See src/llm/FUTURE_ENHANCEMENTS.md §CustomAllReduce NCCL Integration.

    if (world_size_ == 1) {
        return true;  // No reduction needed
    }

    // For single-process multi-GPU, we can directly access all GPU memories
    // Collect data from all GPUs
    std::vector<std::vector<float>> all_gpu_data;
    all_gpu_data.reserve(world_size_);

    for (int i = 0; i < world_size_; ++i) {
        Device device = ctx_.get_device(i);
        GPUTensor gpu_tensor({tensor.size()}, device);

        // In real implementation, this would be the gradient tensor on each GPU
        // For now, copy the current tensor (assumes same layout on all GPUs)
        if (i == rank_) {
            all_gpu_data.push_back(tensor.cpu_data());
        } else {
            // STUB: peer-to-peer GPU copy not implemented; using rank_ data as proxy
            all_gpu_data.push_back(tensor.cpu_data());
        }
    }
    
    // Sum all gradients
    size_t tensor_size = tensor.size();
    std::vector<float> reduced_data(tensor_size, 0.0f);
    
    for (const auto& gpu_data : all_gpu_data) {
        for (size_t i = 0; i < tensor_size; ++i) {
            reduced_data[i] += gpu_data[i];
        }
    }
    
    // Average if requested
    if (average && world_size_ > 1) {
        float scale = 1.0f / static_cast<float>(world_size_);
        for (auto& val : reduced_data) {
            val *= scale;
        }
    }
    
    // Upload reduced data back to this GPU
    tensor.upload(reduced_data);
    
    // Synchronize to ensure all GPUs complete
    ctx_.synchronize_all();
    
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
                if (i == j) continue;
                
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
