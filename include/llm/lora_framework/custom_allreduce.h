/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            custom_allreduce.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:10:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     139                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <memory>

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
    CustomAllReduce(CustomAllReduce&&) noexcept = default;
    CustomAllReduce& operator=(CustomAllReduce&&) noexcept = default;
    
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
     * @brief Get current rank
     */
    int rank() const { return rank_; }
    
    /**
     * @brief Get world size
     */
    int world_size() const { return world_size_; }
    
private:
    const MultiGPUContext& ctx_;
    int rank_;
    int world_size_;
    bool initialized_;
    bool p2p_enabled_;
    
    // Ring all-reduce implementation
    bool ring_allreduce(GPUTensor& tensor, bool average);
    
    // Helper: Transfer data between GPUs
    void gpu_to_gpu_copy(const GPUTensor& src, GPUTensor& dst, 
                         size_t offset, size_t count);
    
    // Helper: Enable P2P access if supported
    void enable_p2p_access();
};

} // namespace lora
} // namespace llm
} // namespace themis
