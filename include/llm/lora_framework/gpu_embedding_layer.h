/**
 * @file gpu_embedding_layer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/gpu_tensor.h"
#include "llm/lora_framework/gpu_memory.h"
#include <vector>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {

class GPUEmbeddingLayer {
public:
    GPUEmbeddingLayer(const float* embedding_weights, 
                      size_t vocab_size,
                      size_t hidden_dim,
                      const Device& device);
    
    ~GPUEmbeddingLayer();
    
    // Disable copy, enable move
    GPUEmbeddingLayer(const GPUEmbeddingLayer&) = delete;
    GPUEmbeddingLayer& operator=(const GPUEmbeddingLayer&) = delete;
    GPUEmbeddingLayer(GPUEmbeddingLayer&& other) noexcept;
    GPUEmbeddingLayer& operator=(GPUEmbeddingLayer&& other) noexcept;
    
    /**
     * @brief Forward.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forward(const GPUTensor& token_ids);
    
    const GPUTensor& weights() const { return embedding_weights_; }
    
    size_t vocab_size() const { return vocab_size_; }
    
    size_t hidden_dim() const { return hidden_dim_; }
    
    const Device& device() const { return device_; }
    
private:
    GPUTensor embedding_weights_;  // [vocab_size, hidden_dim] on GPU
    size_t vocab_size_ = 0;
    size_t hidden_dim_ = 0;
    Device device_;
    
    // Helper methods
    /**
     * @brief Forward CPU.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forwardCPU(const GPUTensor& token_ids);
    /**
     * @brief Forward CUDA.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forwardCUDA(const GPUTensor& token_ids);
    /**
     * @brief Forward HIP.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forwardHIP(const GPUTensor& token_ids);
    /**
     * @brief Forward Vulkan.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forwardVulkan(const GPUTensor& token_ids);
    /**
     * @brief Forward Direct X.
     * @param[in] token_ids Input parameter.
     * @return Return value.
     */
    GPUTensor forwardDirectX(const GPUTensor& token_ids);
};

} // namespace lora
} // namespace llm
} // namespace themis
