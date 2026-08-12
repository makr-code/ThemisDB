/**
 * @file directx_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef _WIN32

#include "directx_context.h"
#include "directx_shader.h"
#include "directx_descriptors.h"
#include <d3d12.h>
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
    uint32_t num_root_constants_ = 0;
    uint32_t num_uavs_ = 0;
    uint32_t num_srvs_ = 0;
    
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> pipeline_state_;
    
    // Store descriptor ranges as member variables to ensure proper lifetime
    std::vector<D3D12_DESCRIPTOR_RANGE> descriptor_ranges_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
