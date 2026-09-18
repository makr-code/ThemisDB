/**
 * @file directx_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class DirectXPipeline {
public:
    DirectXPipeline(DirectXContext* context, 
                    DirectXShader* shader,
                    uint32_t num_root_constants = 4,
                    uint32_t num_uavs = 1,
                    uint32_t num_srvs = 2);
    
    ~DirectXPipeline() noexcept;
    
    // Disable copy, allow move
    DirectXPipeline(const DirectXPipeline&) = delete;
    DirectXPipeline& operator=(const DirectXPipeline&) = delete;
    DirectXPipeline(DirectXPipeline&&) noexcept;
    DirectXPipeline& operator=(DirectXPipeline&&) noexcept;
    
    /**
     * @brief Create.
     * @return True when the operation succeeds.
     */
    bool create();
    
    /**
     * @brief Set root constants.
     * @param[in] data Input parameter.
     * @param[in] num_values Input parameter.
     */
    void set_root_constants(const void* data, uint32_t num_values);
    
    /**
     * @brief Bind uav table.
     * @param[in] table_index Input parameter.
     * @param[in] base_descriptor Input parameter.
     */
    void bind_uav_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor);
    
    /**
     * @brief Bind srv table.
     * @param[in] table_index Input parameter.
     * @param[in] base_descriptor Input parameter.
     */
    void bind_srv_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor);
    
    /**
     * @brief Dispatch.
     * @param[in] thread_groups_x Input parameter.
     * @param[in] thread_groups_y Input parameter.
     * @param[in] thread_groups_z Input parameter.
     */
    void dispatch(uint32_t thread_groups_x, uint32_t thread_groups_y, uint32_t thread_groups_z);
    
    ID3D12PipelineState* pipeline_state() const { return pipeline_state_.Get(); }
    
    ID3D12RootSignature* root_signature() const { return root_signature_.Get(); }
    
    bool is_created() const { return pipeline_state_ != nullptr; }

private:
    /**
     * @brief Create root signature.
     * @return True when the operation succeeds.
     */
    bool create_root_signature();
    /**
     * @brief Create pipeline state.
     * @return True when the operation succeeds.
     */
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
