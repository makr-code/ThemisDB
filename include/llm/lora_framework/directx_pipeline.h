/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_pipeline.h                                 ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     140                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef _WIN32

#include "directx_context.h"
#include "directx_shader.h"
#include "directx_descriptors.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief DirectX 12 compute pipeline state object
 * 
 * Manages compute PSO, root signature, and descriptor bindings.
 */
class DirectXPipeline {
public:
    /**
     * @brief Create compute pipeline
     * @param context DirectX context
     * @param shader Compiled compute shader
     * @param num_root_constants Number of 32-bit root constants
     * @param num_uavs Number of UAV descriptors
     * @param num_srvs Number of SRV descriptors
     */
    DirectXPipeline(DirectXContext* context, 
                    DirectXShader* shader,
                    uint32_t num_root_constants = 4,
                    uint32_t num_uavs = 1,
                    uint32_t num_srvs = 2);
    
    ~DirectXPipeline();
    
    // Disable copy, allow move
    DirectXPipeline(const DirectXPipeline&) = delete;
    DirectXPipeline& operator=(const DirectXPipeline&) = delete;
    DirectXPipeline(DirectXPipeline&&) noexcept;
    DirectXPipeline& operator=(DirectXPipeline&&) noexcept;
    
    /**
     * @brief Create pipeline state object
     */
    bool create();
    
    /**
     * @brief Set root constants (dimensions, parameters)
     * @param data Pointer to constant data
     * @param num_values Number of 32-bit values
     */
    void set_root_constants(const void* data, uint32_t num_values);
    
    /**
     * @brief Bind UAV descriptor table
     * @param table_index Root parameter index for UAV table
     * @param base_descriptor First descriptor in range
     */
    void bind_uav_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor);
    
    /**
     * @brief Bind SRV descriptor table
     * @param table_index Root parameter index for SRV table
     * @param base_descriptor First descriptor in range
     */
    void bind_srv_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor);
    
    /**
     * @brief Dispatch compute shader
     * @param thread_groups_x Number of thread groups in X dimension
     * @param thread_groups_y Number of thread groups in Y dimension
     * @param thread_groups_z Number of thread groups in Z dimension
     */
    void dispatch(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z);
    
    /**
     * @brief Get pipeline state object
     */
    ID3D12PipelineState* pipeline_state() const { return pipeline_state_.Get(); }
    
    /**
     * @brief Get root signature
     */
    ID3D12RootSignature* root_signature() const { return root_signature_.Get(); }
    
    /**
     * @brief Check if pipeline is created
     */
    bool is_created() const { return pipeline_state_ != nullptr; }

private:
    bool create_root_signature();
    bool create_pipeline_state();
    
    DirectXContext* context_;
    DirectXShader* shader_;
    uint32_t num_root_constants_;
    uint32_t num_uavs_;
    uint32_t num_srvs_;
    
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
    
    // Store descriptor ranges as member variables to ensure proper lifetime
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
