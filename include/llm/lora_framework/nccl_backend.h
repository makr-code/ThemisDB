/**
 * @file nccl_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/multi_gpu.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <vector>
#include <memory>

#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
#include <nccl.h>
#endif
#endif

namespace themis {
namespace llm {
namespace lora {

class NCCLBackend {
public:
    NCCLBackend(const MultiGPUContext& ctx, int rank, int world_size);
    
    ~NCCLBackend();
    
    // Disable copy, enable move
    NCCLBackend(const NCCLBackend&) = delete;
    NCCLBackend& operator=(const NCCLBackend&) = delete;
    NCCLBackend(NCCLBackend&&) noexcept;
    NCCLBackend& operator=(NCCLBackend&&) noexcept;
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Finalize.
     */
    void finalize();
    
    bool is_initialized() const { return initialized_; }
    
    bool allreduce(std::vector<GPUTensor*>& tensors, bool average = true);
    
    bool allreduce(GPUTensor& tensor, bool average = true);
    
    bool broadcast(GPUTensor& tensor, int root = 0);
    
    /**
     * @brief Barrier.
     */
    void barrier();
    
    int rank() const { return rank_; }
    
    int world_size() const { return world_size_; }
    
    /**
     * @brief Is available.
     * @return True when the operation succeeds.
     */
    static bool is_available();
    
    /**
     * @brief Get version.
     * @return Return value.
     */
    static std::string get_version();
    
private:
    const MultiGPUContext& ctx_;
    int rank_ = 0;
    int world_size_ = 0;
    bool initialized_ = false;
    
#ifdef THEMIS_ENABLE_CUDA
#ifdef THEMIS_ENABLE_NCCL
    ncclComm_t nccl_comm_;
    ncclUniqueId nccl_id_;
    void* cuda_stream_;  // cudaStream_t
#endif
#endif
    
    /**
     * @brief Initialize nccl.
     * @return True when the operation succeeds.
     */
    bool initialize_nccl();
    /**
     * @brief Cleanup nccl.
     */
    void cleanup_nccl();
};

} // namespace lora
} // namespace llm
} // namespace themis
