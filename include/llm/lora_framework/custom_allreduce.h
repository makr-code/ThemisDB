/**
 * @file custom_allreduce.h
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
#include <functional>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

class CustomAllReduce {
public:
    using RingAllreduceFn = std::function<bool(GPUTensor&, bool /*average*/)>;
    CustomAllReduce(const MultiGPUContext& ctx, int rank, int world_size);
    
    ~CustomAllReduce() = default;
    
    // Disable copy, enable move
    CustomAllReduce(const CustomAllReduce&) = delete;
    CustomAllReduce& operator=(const CustomAllReduce&) = delete;
    CustomAllReduce(CustomAllReduce&&) noexcept = default;
    CustomAllReduce& operator=(CustomAllReduce&&) noexcept = default;
    
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

    /**
     * @brief Set Ring Allreduce Fn.
     * @param[in] fn Input parameter.
     */
    void setRingAllreduceFn(RingAllreduceFn fn);
    
    int rank() const { return rank_; }
    
    int world_size() const { return world_size_; }
    
private:
    const MultiGPUContext& ctx_;
    int rank_ = 0;
    int world_size_ = 0;
    bool initialized_ = false;
    bool p2p_enabled_ = false;
    
    /**
     * @brief Ring all-reduce implementation
     * @param[in,out] tensor Input/output parameter.
     * @param[in] average Input parameter.
     * @return True when the operation succeeds.
     */
    bool ring_allreduce(GPUTensor& tensor, bool average);
    
    /**
     * @brief Helper: Transfer data between GPUs
     * @param[in] src Input parameter.
     * @param[in,out] dst Input/output parameter.
     * @param[in] offset Input parameter.
     * @param[in] count Input parameter.
     */
    void gpu_to_gpu_copy(const GPUTensor& src, GPUTensor& dst, 
                         size_t offset, size_t count);
    
    /**
     * @brief Helper: Enable P2P access if supported
     */
    void enable_p2p_access();

    std::optional<RingAllreduceFn> ring_allreduce_fn_;
};

} // namespace lora
} // namespace llm
} // namespace themis
