/**
 * @file gpu_embedding_layer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_embedding_layer.h"
#include "llm/lora_framework/cuda_kernels.h"
#include "llm/lora_framework/hip_kernels.h"
#ifdef THEMIS_ENABLE_VULKAN
#include "llm/lora_framework/vulkan_kernels.h"
#endif
#ifdef THEMIS_ENABLE_DIRECTX
#include "llm/lora_framework/directx_kernels.h"
#endif
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
#ifdef THEMIS_ENABLE_VULKAN
        case DeviceType::VULKAN:
            return forwardVulkan(token_ids);
#endif
#ifdef THEMIS_ENABLE_DIRECTX
        case DeviceType::DIRECTX:
            return forwardDirectX(token_ids);
#endif
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
            // Note: Token IDs stored as floats in GPUTensor (architecture limitation - no int32 tensor support yet)
            // Using round() to handle potential floating point imprecision.
            // Consider adding integer tensor support to GPUTensor to avoid this conversion.
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
#ifdef THEMIS_ENABLE_CUDA
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    spdlog::debug("GPUEmbeddingLayer::forwardCUDA: batch_size={}, seq_len={}", batch_size, seq_len);
    
    // Create output tensor on GPU
    GPUTensor embeddings({batch_size, seq_len, hidden_dim_}, device_);
    
    // Launch CUDA kernel for embedding lookup
    cudaError_t err = cuda::launch_embedding_lookup_kernel(
        static_cast<float*>(embeddings.gpu_ptr()),
        static_cast<const float*>(token_ids.gpu_ptr()),
        static_cast<const float*>(embedding_weights_.gpu_ptr()),
        batch_size,
        seq_len,
        hidden_dim_,
        vocab_size_,
        nullptr  // Use default stream
    );
    
    if (err != cudaSuccess) {
        spdlog::error("CUDA embedding lookup kernel failed: {}", cudaGetErrorString(err));
        throw std::runtime_error("CUDA embedding lookup kernel failed");
    }
    
    spdlog::debug("CUDA embedding lookup completed successfully");
    return embeddings;
#else
    // CUDA not enabled, fall back to CPU
    spdlog::warn("CUDA not enabled at compile time, using CPU fallback");
    return forwardCPU(token_ids);
#endif
}

GPUTensor GPUEmbeddingLayer::forwardHIP(const GPUTensor& token_ids) {
#ifdef THEMIS_ENABLE_HIP
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    spdlog::debug("GPUEmbeddingLayer::forwardHIP: batch_size={}, seq_len={}", batch_size, seq_len);
    
    // Create output tensor on GPU
    GPUTensor embeddings({batch_size, seq_len, hidden_dim_}, device_);
    
    // Launch HIP kernel for embedding lookup
    hipError_t err = hip::launch_embedding_lookup_kernel(
        static_cast<float*>(embeddings.gpu_ptr()),
        static_cast<const float*>(token_ids.gpu_ptr()),
        static_cast<const float*>(embedding_weights_.gpu_ptr()),
        batch_size,
        seq_len,
        hidden_dim_,
        vocab_size_,
        nullptr  // Use default stream
    );
    
    if (err != hipSuccess) {
        spdlog::error("HIP embedding lookup kernel failed: {}", hipGetErrorString(err));
        throw std::runtime_error("HIP embedding lookup kernel failed");
    }
    
    spdlog::debug("HIP embedding lookup completed successfully");
    return embeddings;
#else
    // HIP not enabled, fall back to CPU
    spdlog::warn("HIP not enabled at compile time, using CPU fallback");
    return forwardCPU(token_ids);
#endif
}

#ifdef THEMIS_ENABLE_VULKAN
GPUTensor GPUEmbeddingLayer::forwardVulkan(const GPUTensor& token_ids) {
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    spdlog::debug("GPUEmbeddingLayer::forwardVulkan: batch_size={}, seq_len={}", batch_size, seq_len);
    
    // Note: Vulkan implementation uses CPU-side buffer management
    // Download token IDs and embedding weights
    auto token_data = token_ids.cpu_data();
    auto weights_data = embedding_weights_.cpu_data();
    
    // Allocate output buffer
    size_t output_size = batch_size * seq_len * hidden_dim_;
    std::vector<float> embeddings_data(output_size);
    
    // Launch Vulkan compute shader
    try {
        ::themis::lora::vulkan::launch_embedding_lookup_shader(
            embeddings_data.data(),
            token_data.data(),
            weights_data.data(),
            static_cast<int>(batch_size),
            static_cast<int>(seq_len),
            static_cast<int>(hidden_dim_),
            static_cast<int>(vocab_size_)
        );
        
        spdlog::debug("Vulkan embedding lookup completed successfully");
    } catch (const std::exception& e) {
        spdlog::warn("Vulkan embedding lookup failed: {}, falling back to CPU", e.what());
        return forwardCPU(token_ids);
    }
    
    // Create output tensor and upload
    GPUTensor embeddings({batch_size, seq_len, hidden_dim_}, device_);
    embeddings.upload(embeddings_data);
    
    return embeddings;
}
#else
GPUTensor GPUEmbeddingLayer::forwardVulkan(const GPUTensor& token_ids) {
    spdlog::warn("Vulkan not enabled at compile time, using CPU fallback");
    return forwardCPU(token_ids);
}
#endif

#ifdef THEMIS_ENABLE_DIRECTX
GPUTensor GPUEmbeddingLayer::forwardDirectX(const GPUTensor& token_ids) {
    auto shape = token_ids.shape();
    size_t batch_size = shape[0];
    size_t seq_len = shape[1];
    
    spdlog::debug("GPUEmbeddingLayer::forwardDirectX: batch_size={}, seq_len={}", batch_size, seq_len);
    
    // Note: DirectX implementation uses CPU-side buffer management (similar to Vulkan)
    // Download token IDs and embedding weights
    auto token_data = token_ids.cpu_data();
    auto weights_data = embedding_weights_.cpu_data();
    
    // Allocate output buffer
    size_t output_size = batch_size * seq_len * hidden_dim_;
    std::vector<float> embeddings_data(output_size);
    
    // Launch DirectX compute shader
    try {
        directx::launch_embedding_lookup_shader(
            embeddings_data.data(),
            token_data.data(),
            weights_data.data(),
            static_cast<int>(batch_size),
            static_cast<int>(seq_len),
            static_cast<int>(hidden_dim_),
            static_cast<int>(vocab_size_)
        );
        
        spdlog::debug("DirectX embedding lookup completed successfully");
    } catch (const std::exception& e) {
        spdlog::warn("DirectX embedding lookup failed: {}, falling back to CPU", e.what());
        return forwardCPU(token_ids);
    }
    
    // Create output tensor and upload
    GPUTensor embeddings({batch_size, seq_len, hidden_dim_}, device_);
    embeddings.upload(embeddings_data);
    
    return embeddings;
}
#else
GPUTensor GPUEmbeddingLayer::forwardDirectX(const GPUTensor& token_ids) {
    spdlog::warn("DirectX not enabled at compile time, using CPU fallback");
    return forwardCPU(token_ids);
}
#endif
} // namespace lora
} // namespace llm
} // namespace themis

