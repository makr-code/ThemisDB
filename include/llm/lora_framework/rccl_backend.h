/**
 * @file rccl_backend.h
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

#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
#include <rccl/rccl.h>
#endif
#endif

namespace themis {
namespace llm {
namespace lora {

class RCCLBackend {
public:
    RCCLBackend(const MultiGPUContext& ctx, int rank, int world_size);
    
    ~RCCLBackend();
    
    // Disable copy, enable move
    RCCLBackend(const RCCLBackend&) = delete;
    RCCLBackend& operator=(const RCCLBackend&) = delete;
    RCCLBackend(RCCLBackend&&) noexcept;
    RCCLBackend& operator=(RCCLBackend&&) noexcept;
    
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
    
#ifdef THEMIS_ENABLE_HIP
#ifdef THEMIS_ENABLE_RCCL
    ncclComm_t rccl_comm_;  // RCCL uses same types as NCCL
    ncclUniqueId rccl_id_;
    void* hip_stream_;  // hipStream_t
#endif
#endif
    
    /**
     * @brief Initialize rccl.
     * @return True when the operation succeeds.
     */
    bool initialize_rccl();
    /**
     * @brief Cleanup rccl.
     */
    void cleanup_rccl();
};

} // namespace lora
} // namespace llm
} // namespace themis
