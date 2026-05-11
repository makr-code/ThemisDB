/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flash_attention.h                                  ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:45:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    float* data = nullptr;
    size_t size = 0;
    std::vector<int> shape;  // [batch, seq_len, num_heads, head_dim]
    
    bool isValid() const {
        return data != nullptr && size > 0 && !shape.empty();
    }
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
    ~FlashAttention();
    
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

