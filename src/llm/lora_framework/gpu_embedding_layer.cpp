#include "llm/lora_framework/gpu_embedding_layer.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <atomic>

namespace themis {
namespace llm {
namespace lora {

GPUEmbeddingLayer::GPUEmbeddingLayer(const float* embedding_weights,
                                      size_t vocab_size,
                                      size_t hidden_dim,
                                      const Device& device)
    : vocab_size_(vocab_size)
    , hidden_dim_(hidden_dim)
    , device_(device)
    , embedding_weights_({vocab_size, hidden_dim}, device) {
    
    if (!embedding_weights) {
        throw std::invalid_argument("Embedding weights cannot be null");
    }
    
    if (vocab_size == 0 || hidden_dim == 0) {
        throw std::invalid_argument("Vocab size and hidden dim must be > 0");
    }
    
    spdlog::info("Creating GPUEmbeddingLayer: vocab_size={}, hidden_dim={}, device={}",
                 vocab_size, hidden_dim, static_cast<int>(device.type));
    
    // Upload embedding weights to GPU
    std::vector<float> weights_vec(embedding_weights, embedding_weights + vocab_size * hidden_dim);
    embedding_weights_.upload(weights_vec);
    
    spdlog::debug("GPUEmbeddingLayer: Uploaded {} MB to GPU",
                  (vocab_size * hidden_dim * sizeof(float)) / (1024.0 * 1024.0));
}

GPUEmbeddingLayer::~GPUEmbeddingLayer() = default;

GPUEmbeddingLayer::GPUEmbeddingLayer(GPUEmbeddingLayer&& other) noexcept
    : embedding_weights_(std::move(other.embedding_weights_))
    , vocab_size_(other.vocab_size_)
    , hidden_dim_(other.hidden_dim_)
    , device_(other.device_) {
}

GPUEmbeddingLayer& GPUEmbeddingLayer::operator=(GPUEmbeddingLayer&& other) noexcept {
    if (this != &other) {
        embedding_weights_ = std::move(other.embedding_weights_);
        vocab_size_ = other.vocab_size_;
        hidden_dim_ = other.hidden_dim_;
        device_ = other.device_;
    }
    return *this;
}

GPUTensor GPUEmbeddingLayer::forward(const GPUTensor& token_ids) {
    // Validate input
    if (token_ids.ndim() < 2) {
        throw std::invalid_argument("token_ids must be at least 2D (batch_size, seq_len)");
    }
    
    // Dispatch to appropriate backend
    switch (device_.type) {
        case DeviceType::CUDA:
            return forwardCUDA(token_ids);
        case DeviceType::HIP:
            return forwardHIP(token_ids);
        case DeviceType::VULKAN:
            return forwardVulkan(token_ids);
        default:
            return forwardCPU(token_ids);
    }
}

GPUTensor GPUEmbeddingLayer::forwardCPU(const GPUTensor& token_ids) {
    // CPU-based embedding lookup with GPU upload
    // This is the fallback implementation that works on all backends
    
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    spdlog::debug("GPUEmbeddingLayer::forwardCPU: batch_size={}, seq_len={}", batch_size, seq_len);
    
    // Download token IDs to CPU
    auto token_data = token_ids.cpu_data();
    
    // Download embedding weights to CPU
    auto weights_data = embedding_weights_.cpu_data();
    
    // Allocate output buffer
    std::vector<float> embeddings_data(batch_size * seq_len * hidden_dim_);
    
    // Perform embedding lookup on CPU
    for (size_t i = 0; i < batch_size; ++i) {
        for (size_t j = 0; j < seq_len; ++j) {
            size_t token_idx = i * seq_len + j;
            // Note: Token IDs stored as floats in GPUTensor (current architecture limitation)
            // Using round() to handle potential floating point imprecision
            int token_id = static_cast<int>(std::round(token_data[token_idx]));
            
            // Bounds check
            if (token_id < 0 || token_id >= static_cast<int>(vocab_size_)) {
                // Fill with zeros - log only once (thread-safe) to avoid flooding
                static std::atomic<bool> logged_warning{false};
                bool expected = false;
                if (logged_warning.compare_exchange_strong(expected, true)) {
                    spdlog::warn("Token ID out of bounds detected (will be replaced with zeros)");
                }
                
                size_t out_idx = (i * seq_len + j) * hidden_dim_;
                std::fill(embeddings_data.begin() + out_idx, 
                         embeddings_data.begin() + out_idx + hidden_dim_, 
                         0.0f);
                continue;
            }
            
            // Copy embedding vector
            const float* src = weights_data.data() + token_id * hidden_dim_;
            float* dst = embeddings_data.data() + (i * seq_len + j) * hidden_dim_;
            std::copy(src, src + hidden_dim_, dst);
        }
    }
    
    // Create output tensor and upload to GPU
    GPUTensor embeddings({batch_size, seq_len, hidden_dim_}, device_);
    embeddings.upload(embeddings_data);
    
    return embeddings;
}

GPUTensor GPUEmbeddingLayer::forwardCUDA(const GPUTensor& token_ids) {
    // TODO: Implement CUDA kernel for embedding lookup
    // For now, fall back to CPU implementation
    spdlog::debug("CUDA embedding kernel not yet implemented, using CPU fallback");
    return forwardCPU(token_ids);
}

GPUTensor GPUEmbeddingLayer::forwardHIP(const GPUTensor& token_ids) {
    // TODO: Implement HIP kernel for embedding lookup
    // For now, fall back to CPU implementation
    spdlog::debug("HIP embedding kernel not yet implemented, using CPU fallback");
    return forwardCPU(token_ids);
}

GPUTensor GPUEmbeddingLayer::forwardVulkan(const GPUTensor& token_ids) {
    // TODO: Implement Vulkan compute shader for embedding lookup
    // For now, fall back to CPU implementation
    spdlog::debug("Vulkan embedding shader not yet implemented, using CPU fallback");
    return forwardCPU(token_ids);
}

} // namespace lora
} // namespace llm
} // namespace themis
