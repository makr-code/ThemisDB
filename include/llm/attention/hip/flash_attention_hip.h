/*
 * ThemisDB | File: flash_attention_hip.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 69
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #1031 Implement comprehensive res... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"

namespace themis {
namespace llm {
namespace attention {
namespace hip {

/**
 * @brief HIP Flash Attention implementation for AMD GPUs
 * 
 * Supports:
 * - MI300 (CDNA 3): Wave64 optimization
 * - RDNA 2/3: Consumer GPU optimization
 */
class FlashAttentionHIP : public IFlashAttention {
public:
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
     * @brief Check if HIP is available
     */
    static bool isAvailable();

private:
    FlashAttentionConfig config_;
    
    // HIP resources
    void* hip_stream_ = nullptr;
    void* d_workspace_ = nullptr;
    size_t workspace_size_ = 0;
    
    void initializeHIP();
    void cleanupHIP();
};

} // namespace hip
} // namespace attention
} // namespace llm
} // namespace themis
