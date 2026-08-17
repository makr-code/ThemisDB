/**
 * @file custom_allreduce.h
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
#include <functional>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Custom all-reduce implementation (fallback when NCCL/RCCL unavailable)
 * 
 * Implements ring all-reduce algorithm for gradient synchronization.
 * Supports mixed GPU vendors (CUDA + HIP) and CPU fallback.
 * 
 * Ring All-Reduce:
 * - Step 1: Each GPU i sends chunk to GPU (i+1) % N
 * - Step 2: Each GPU receives chunk from GPU (i-1) % N
 * - Repeat N-1 times for full all-reduce
 */
class CustomAllReduce {
public:
    /**
     * @brief Function type for a production ring all-reduce backend.
     *
     * Callers can inject a real NCCL/RCCL/MPI all-reduce via
     * setRingAllreduceFn().  The function receives the local tensor and
     * an `average` flag; it must aggregate values across all ranks and
     * write the result back into the tensor.
     */
    using RingAllreduceFn = std::function<bool(GPUTensor&, bool /*average*/)>;
    /**
     * @brief Initialize custom all-reduce
     * @param ctx Multi-GPU context
     * @param rank Current process rank
     * @param world_size Total number of processes
     */
    CustomAllReduce(const MultiGPUContext& ctx, int rank, int world_size);
    
    ~CustomAllReduce() = default;
    
    // Disable copy, enable move
    CustomAllReduce(const CustomAllReduce&) = delete;
    CustomAllReduce& operator=(const CustomAllReduce&) = delete;
    CustomAllReduce(CustomAllReduce&&) noexcept noexcept = default;
    CustomAllReduce& operator=(CustomAllReduce&&) noexcept noexcept = default;
    
    /**
     * @brief Initialize custom all-reduce (setup P2P if available)
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Finalize custom all-reduce
     */
    void finalize();
    
    /**
     * @brief Check if initialized
     */
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief All-reduce operation using ring algorithm
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
     * @brief Inject a production ring all-reduce implementation.
     *
     * When set, ring_allreduce() delegates to this function instead of the
     * default failure path. This allows wiring a real NCCL/RCCL/MPI
     * collective at runtime without recompiling.
     * @param fn Callable that performs the actual collective and writes the
     *           reduced result back into the tensor in-place.
     */
    void setRingAllreduceFn(RingAllreduceFn fn);
    
    /**
     * @brief Get current rank
     */
    int rank() const { return rank_; }
    
    /**
     * @brief Get world size
     */
    int world_size() const { return world_size_; }
    
private:
    const MultiGPUContext& ctx_;
    int rank_ = 0;
    int world_size_ = 0;
    bool initialized_ = false;
    bool p2p_enabled_ = false;
    
    // Ring all-reduce implementation
    bool ring_allreduce(GPUTensor& tensor, bool average);
    
    // Helper: Transfer data between GPUs
    void gpu_to_gpu_copy(const GPUTensor& src, GPUTensor& dst, 
                         size_t offset, size_t count);
    
    // Helper: Enable P2P access if supported
    void enable_p2p_access();

    std::optional<RingAllreduceFn> ring_allreduce_fn_;
};

} // namespace lora
} // namespace llm
} // namespace themis
