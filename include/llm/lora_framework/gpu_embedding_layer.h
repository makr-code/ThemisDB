/**
 * @file gpu_embedding_layer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief GPU Embedding Layer - Converts token IDs to embeddings on GPU
 * 
 * This class provides efficient embedding lookup on GPU by:
 * 1. Storing embedding weights directly in GPU memory
 * 2. Performing lookups via GPU kernels (when available)
 * 3. Falling back to CPU-based lookup with GPU upload (initial implementation)
 * 
 * Design:
 * - Embedding weights are loaded from base model and kept on GPU
 * - Token IDs are looked up to produce embedding vectors
 * - No CPU-GPU transfers in the hot path (GPU kernels)
 * - CPU fallback available for compatibility
 */
class GPUEmbeddingLayer {
public:
    /**
     * @brief Construct GPU embedding layer from embedding weights
     * @param embedding_weights Embedding matrix [vocab_size, hidden_dim] (CPU memory)
     * @param vocab_size Vocabulary size
     * @param hidden_dim Embedding dimension
     * @param device Target GPU device (CUDA, HIP, Vulkan, DirectX)
     */
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
     * @brief Forward pass: token IDs → embeddings
     * 
     * Input: token_ids [batch_size, seq_len] on GPU
     * Output: embeddings [batch_size, seq_len, hidden_dim] on GPU
     * 
     * Implementation:
     * - For each token ID, lookup corresponding embedding vector
     * - Use GPU kernel when available (CUDA/HIP/Vulkan)
     * - Fall back to CPU lookup + GPU upload when needed
     * 
     * @param token_ids Token ID tensor (batch_size, seq_len) on GPU
     * @return Embedding tensor (batch_size, seq_len, hidden_dim) on GPU
     */
    GPUTensor forward(const GPUTensor& token_ids);
    
    /**
     * @brief Get embedding weights tensor (read-only)
     * @return Embedding weight matrix [vocab_size, hidden_dim] on GPU
     */
    const GPUTensor& weights() const { return embedding_weights_; }
    
    /**
     * @brief Get vocabulary size
     */
    size_t vocab_size() const { return vocab_size_; }
    
    /**
     * @brief Get embedding dimension
     */
    size_t hidden_dim() const { return hidden_dim_; }
    
    /**
     * @brief Get device
     */
    const Device& device() const { return device_; }
    
private:
    GPUTensor embedding_weights_;  // [vocab_size, hidden_dim] on GPU
    size_t vocab_size_ = 0;
    size_t hidden_dim_ = 0;
    Device device_;
    
    // Helper methods
    GPUTensor forwardCPU(const GPUTensor& token_ids);
    GPUTensor forwardCUDA(const GPUTensor& token_ids);
    GPUTensor forwardHIP(const GPUTensor& token_ids);
    GPUTensor forwardVulkan(const GPUTensor& token_ids);
    GPUTensor forwardDirectX(const GPUTensor& token_ids);
};

} // namespace lora
} // namespace llm
} // namespace themis
