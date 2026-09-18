/**
 * @file flash_attention_hip.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"

namespace themis {
namespace llm {
namespace attention {
namespace hip {

class FlashAttentionHIP : public IFlashAttention {
public:
    /**
     * @brief Flash Attention HIP.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit FlashAttentionHIP(const FlashAttentionConfig& config);
    ~FlashAttentionHIP() override;
    
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    ) override;
    
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    ) override;
    
    std::string getBackendName() const override;
    AttentionMemoryStats getMemoryStats() const override;
    
    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

private:
    FlashAttentionConfig config_;
    
    // HIP resources
    void* hip_stream_ = nullptr;
    void* d_workspace_ = nullptr;
    size_t workspace_size_ = 0;
    
    /**
     * @brief Initialize HIP.
     */
    void initializeHIP();
    /**
     * @brief Cleanup HIP.
     */
    void cleanupHIP();
};

} // namespace hip
} // namespace attention
} // namespace llm
} // namespace themis
