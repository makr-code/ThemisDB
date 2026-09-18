/**
 * @file flash_attention.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct Tensor {
    /**
     * @brief Tensor.
     * @return Return value.
     */
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

class IFlashAttention {
public:
    /**
     * @brief IFlash Attention.
     * @return Return value.
     */
    virtual ~IFlashAttention() = default;
    
    virtual Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    ) = 0;
    
    /**
     * @brief Backward.
     * @param[in] dO Input parameter.
     * @param[in,out] dQ Input/output parameter.
     * @param[in,out] dK Input/output parameter.
     * @param[in,out] dV Input/output parameter.
     * @return Return value.
     */
    virtual Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    ) = 0;
    
    /**
     * @brief Get Backend Name.
     * @return Return value.
     */
    virtual std::string getBackendName() const = 0;
    
    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    virtual AttentionMemoryStats getMemoryStats() const = 0;
};

class FlashAttention {
public:
    FlashAttention(Backend backend, const FlashAttentionConfig& config);
    
    ~FlashAttention() noexcept;
    
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    );
    
    /**
     * @brief Backward.
     * @param[in] dO Input parameter.
     * @param[in,out] dQ Input/output parameter.
     * @param[in,out] dK Input/output parameter.
     * @param[in,out] dV Input/output parameter.
     * @return Return value.
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    );
    
    /**
     * @brief Select Best Backend.
     * @return Return value.
     */
    static Backend selectBestBackend();
    
    /**
     * @brief Is Backend Available.
     * @param[in] backend Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isBackendAvailable(Backend backend);
    
    /**
     * @brief Get Backend Name.
     * @return Return value.
     */
    std::string getBackendName() const;
    
    const FlashAttentionConfig& getConfig() const { return config_; }
    
    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    AttentionMemoryStats getMemoryStats() const;
    
    /**
     * @brief Get Expected Speedup.
     * @return Return value.
     */
    double getExpectedSpeedup() const;
    
private:
    Backend backend_;
    FlashAttentionConfig config_;
    std::unique_ptr<IFlashAttention> impl_;
    
    // Backend detection helpers
    /**
     * @brief Detect CUDABackend.
     * @return Return value.
     */
    static Backend detectCUDABackend();
    /**
     * @brief Detect Vulkan Backend.
     * @return Return value.
     */
    static Backend detectVulkanBackend();
    /**
     * @brief Detect HIPBackend.
     * @return Return value.
     */
    static Backend detectHIPBackend();
    
    // Backend factory
    /**
     * @brief Create Backend.
     * @param[in] backend Input parameter.
     * @return Return value.
     */
    std::unique_ptr<IFlashAttention> createBackend(Backend backend);
};

/**
 * @brief Get Backend Name.
 * @param[in] backend Input parameter.
 * @return Pointer to the result.
 */
const char* getBackendName(Backend backend);

/**
 * @brief Get Status Message.
 * @param[in] status Input parameter.
 * @return Pointer to the result.
 */
const char* getStatusMessage(Status status);

} // namespace attention
} // namespace llm
} // namespace themis

