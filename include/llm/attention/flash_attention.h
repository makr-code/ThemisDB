/**
 * @file flash_attention.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "flash_attention_config.h"
#include "kv_cache_manager.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace attention {

/**
 * @brief Status enum for attention operations
 */
enum class Status {
    SUCCESS = 0,
    ERROR_INVALID_CONFIG,
    ERROR_BACKEND_NOT_AVAILABLE,
    ERROR_OUT_OF_MEMORY,
    ERROR_INVALID_TENSOR,
    ERROR_CUDA_ERROR,
    ERROR_VULKAN_ERROR,
    ERROR_HIP_ERROR,
    ERROR_NOT_IMPLEMENTED
};

/**
 * @brief Tensor wrapper for attention operations
 */
struct Tensor {
    virtual ~Tensor() = default;
    Tensor() = default;
    Tensor(size_t n, float init_value) {
        owned_data = std::make_shared<std::vector<float>>(n, init_value);
        data = owned_data->data();
        size = owned_data->size();
    }
    // Allow moving ownership from a std::vector
    Tensor(std::shared_ptr<std::vector<float>> vec) {
        owned_data = std::move(vec);
        if (owned_data) {
            data = owned_data->data();
            size = owned_data->size();
        }
    }
    float* data = nullptr;
    size_t size = 0;
    std::vector<int> shape;  // [batch, seq_len, num_heads, head_dim]
    
    bool isValid() const {
        return data != nullptr && size > 0 && !shape.empty();
    }
private:
    std::shared_ptr<std::vector<float>> owned_data;
};

/**
 * @brief Backend types for Flash Attention
 */
enum class Backend {
    AUTO,           // Auto-detect best backend
    CUDA_SM90,      // NVIDIA H100, RTX 6000 Ada (Hopper architecture)
    CUDA_SM86,      // NVIDIA A100, RTX 4090 (Ampere architecture)
    CUDA_SM80,      // NVIDIA A100 (earlier Ampere)
    VULKAN,         // Cross-platform Vulkan compute
    HIP_MI300,      // AMD MI300 (CDNA 3)
    HIP_RDNA,       // AMD RDNA 2/3 (consumer GPUs)
    CPU             // CPU fallback (slow)
};

/**
 * @brief Abstract interface for Flash Attention implementations
 */
class IFlashAttention {
public:
    virtual ~IFlashAttention() = default;
    
    /**
     * @brief Forward pass of attention
     */
    virtual Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    ) = 0;
    
    /**
     * @brief Backward pass (for training)
     */
    virtual Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    ) = 0;
    
    /**
     * @brief Get backend name
     */
    virtual std::string getBackendName() const = 0;
    
    /**
     * @brief Get memory statistics
     */
    virtual AttentionMemoryStats getMemoryStats() const = 0;
};

/**
 * @brief Main Flash Attention v3 class with multi-backend support
 * 
 * Provides unified interface for Flash Attention across:
 * - NVIDIA GPUs (CUDA SM80/SM86/SM90)
 * - AMD GPUs (HIP MI300, RDNA)
 * - Cross-platform (Vulkan)
 * - CPU fallback
 */
class FlashAttention {
public:
    /**
     * @brief Construct Flash Attention with specified backend
     * @param backend Backend type (use AUTO for auto-detection)
     * @param config Flash Attention configuration
     */
    FlashAttention(Backend backend, const FlashAttentionConfig& config);
    
    /**
     * @brief Destructor
     */
    ~FlashAttention() noexcept;
    
    /**
     * @brief Forward pass of attention
     * 
     * Computes: O = softmax(Q * K^T / scale) * V
     * 
     * @param Q Query tensor [batch, seq_len, num_heads, head_dim]
     * @param K Key tensor [batch, seq_len, num_heads, head_dim]
     * @param V Value tensor [batch, seq_len, num_heads, head_dim]
     * @param O Output tensor [batch, seq_len, num_heads, head_dim]
     * @param kv_cache Optional KV cache manager
     * @return Status code
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    );
    
    /**
     * @brief Backward pass (for training)
     * 
     * @param dO Gradient of output
     * @param dQ Gradient of query (output)
     * @param dK Gradient of key (output)
     * @param dV Gradient of value (output)
     * @return Status code
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    );
    
    /**
     * @brief Auto-select best backend for current hardware
     * @return Recommended backend
     */
    static Backend selectBestBackend();
    
    /**
     * @brief Check if backend is available
     * @param backend Backend to check
     * @return true if backend is available
     */
    static bool isBackendAvailable(Backend backend);
    
    /**
     * @brief Get backend name
     */
    std::string getBackendName() const;
    
    /**
     * @brief Get configuration
     */
    const FlashAttentionConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get memory statistics
     */
    AttentionMemoryStats getMemoryStats() const;
    
    /**
     * @brief Get expected speedup for this backend vs standard attention
     */
    double getExpectedSpeedup() const;
    
private:
    Backend backend_;
    FlashAttentionConfig config_;
    std::unique_ptr<IFlashAttention> impl_;
    
    // Backend detection helpers
    static Backend detectCUDABackend();
    static Backend detectVulkanBackend();
    static Backend detectHIPBackend();
    
    // Backend factory
    std::unique_ptr<IFlashAttention> createBackend(Backend backend);
};

/**
 * @brief Get human-readable backend name
 */
const char* getBackendName(Backend backend);

/**
 * @brief Get status message
 */
const char* getStatusMessage(Status status);

} // namespace attention
} // namespace llm
} // namespace themis

