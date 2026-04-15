/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flash_attention_hip.h                              ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
