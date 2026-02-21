/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flash_attention_vulkan.h                           ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   99.0/100                                       ║
    • Total Lines:     86                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
namespace vulkan {

/**
 * @brief Vulkan Flash Attention implementation (cross-platform)
 * 
 * Uses Vulkan compute shaders for GPU acceleration
 * Compatible with NVIDIA, AMD, Intel, ARM GPUs
 */
class FlashAttentionVulkan : public IFlashAttention {
public:
    explicit FlashAttentionVulkan(const FlashAttentionConfig& config);
    ~FlashAttentionVulkan() override;
    
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
     * @brief Check if Vulkan is available
     */
    static bool isAvailable();

private:
    FlashAttentionConfig config_;
    
    // Vulkan resources (placeholders)
    void* vk_device_ = nullptr;
    void* vk_pipeline_ = nullptr;
    void* vk_descriptor_set_ = nullptr;
    
    void initializeVulkan();
    void cleanupVulkan();
};

} // namespace vulkan
} // namespace attention
} // namespace llm
} // namespace themis
