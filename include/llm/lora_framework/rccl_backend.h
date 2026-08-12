/**
 * @file rccl_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <memory>

#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
#include <rccl/rccl.h>
#endif
#endif

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief RCCL backend for multi-GPU communication on AMD GPUs
 * 
 * Wraps AMD RCCL library for efficient gradient synchronization.
 * Provides all-reduce, broadcast, and other collective operations.
 */
class RCCLBackend {
public:
    /**
     * @brief Initialize RCCL backend
     * @param ctx Multi-GPU context
     * @param rank Current process rank
     * @param world_size Total number of processes
     */
    RCCLBackend(const MultiGPUContext& ctx, int rank, int world_size);
    
    ~RCCLBackend();
    
    // Disable copy, enable move
    RCCLBackend(const RCCLBackend&) = delete;
    RCCLBackend& operator=(const RCCLBackend&) = delete;
    RCCLBackend(RCCLBackend&&) noexcept;
    RCCLBackend& operator=(RCCLBackend&&) noexcept;
    
    /**
     * @brief Initialize RCCL communicator
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Finalize RCCL communicator
     */
    void finalize();
    
    /**
     * @brief Check if RCCL is initialized
     */
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief All-reduce operation (sum and average gradients)
     * @param tensors Tensors to reduce across all GPUs
     * @param average If true, divide by world_size after sum
     * @return true if successful
     */
    bool allreduce(std::vector<GPUTensor*>& tensors, bool average = true);
    
    /**
     * @brief All-reduce single tensor
     * @param tensor Tensor to reduce
     * @param average If true, divide by world_size after sum
     * @return true if successful
     */
    bool allreduce(GPUTensor& tensor, bool average = true);
    
    /**
     * @brief Broadcast tensor from root to all processes
     * @param tensor Tensor to broadcast
     * @param root Root rank
     * @return true if successful
     */
    bool broadcast(GPUTensor& tensor, int root = 0);
    
    /**
     * @brief Barrier synchronization
     */
    void barrier();
    
    /**
     * @brief Get current rank
     */
    int rank() const { return rank_; }
    
    /**
     * @brief Get world size
     */
    int world_size() const { return world_size_; }
    
    /**
     * @brief Check if RCCL is available on this system
     */
    static bool is_available();
    
    /**
     * @brief Get RCCL version
     */
    static std::string get_version();
    
private:
    const MultiGPUContext& ctx_;
    int rank_ = 0;
    int world_size_ = 0;
    bool initialized_ = false;
    
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    ncclComm_t rccl_comm_;  // RCCL uses same types as NCCL
    ncclUniqueId rccl_id_;
    void* hip_stream_;  // hipStream_t
#endif
#endif
    
    bool initialize_rccl();
    void cleanup_rccl();
};

} // namespace lora
} // namespace llm
} // namespace themis
