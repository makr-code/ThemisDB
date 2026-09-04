/**
 * @file directx_pipeline.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/directx_pipeline.h"

#ifdef _WIN32

#include <stdexcept>
#include <iostream>
#include <vector>

namespace themis {
namespace lora {
namespace directx {

DirectXPipeline::DirectXPipeline(DirectXContext* context,
                                 DirectXShader* shader,
                                 uint32_t num_root_constants,
                                 uint32_t num_uavs,
                                 uint32_t num_srvs)
    : context_(context)
    , shader_(shader)
    , num_root_constants_(num_root_constants)
    , num_uavs_(num_uavs)
    , num_srvs_(num_srvs) {
    
    if (!context_ || !context_->is_initialized()) {
        throw std::runtime_error("DirectXPipeline: Invalid context");
    }
    
    if (!shader_ || !shader_->is_loaded()) {
        throw std::runtime_error("DirectXPipeline: Invalid shader");
    }
}

DirectXPipeline::~DirectXPipeline() noexcept {
    // Phase2-LLM-B1: exception_in_destructor — COM Release() may throw on
    // debug-layer paths; suppress exceptions to satisfy noexcept contract.
    try {
        // COM smart-pointer members (root_signature_, pipeline_state_) released automatically.
    } catch (...) {}
}

DirectXPipeline::DirectXPipeline(DirectXPipeline&& other) noexcept
    : context_(other.context_)
    , shader_(other.shader_)
    , num_root_constants_(other.num_root_constants_)
    , num_uavs_(other.num_uavs_)
    , num_srvs_(other.num_srvs_)
    , root_signature_(std::move(other.root_signature_))
    , pipeline_state_(std::move(other.pipeline_state_)) {
    
    other.context_ = nullptr;
    other.shader_ = nullptr;
}

DirectXPipeline& DirectXPipeline::operator=(DirectXPipeline&& other) noexcept {
    if (this != &other) {
        context_ = other.context_;
        shader_ = other.shader_;
        num_root_constants_ = other.num_root_constants_;
        num_uavs_ = other.num_uavs_;
        num_srvs_ = other.num_srvs_;
        root_signature_ = std::move(other.root_signature_);
        pipeline_state_ = std::move(other.pipeline_state_);
        
        other.context_ = nullptr;
        other.shader_ = nullptr;
    }
    return *this;
}

bool DirectXPipeline::create() {
    if (!create_root_signature()) {
        return false;
    }
    
    if (!create_pipeline_state()) {
        return false;
    }
    
    return true;
}

bool DirectXPipeline::create_root_signature() {
    // Define root parameters
    std::vector<D3D12_ROOT_PARAMETER> root_params;
    
    // Root constants (for dimensions, parameters)
    if (num_root_constants_ > 0) {
        D3D12_ROOT_PARAMETER constants_param = {};
        constants_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        constants_param.Constants.ShaderRegister = 0;  // b0
        constants_param.Constants.RegisterSpace = 0;
        constants_param.Constants.Num32BitValues = num_root_constants_;
        constants_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root_params.push_back(constants_param);
    }
    
    // Reserve space for descriptor ranges (ensure stable addresses)
    descriptor_ranges_.clear();
    descriptor_ranges_.reserve(2);
    
    // UAV descriptor table (outputs)
    if (num_uavs_ > 0) {
        D3D12_DESCRIPTOR_RANGE uav_range = {};
        uav_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        uav_range.NumDescriptors = num_uavs_;
        uav_range.BaseShaderRegister = 0;  // u0, u1, ...
        uav_range.RegisterSpace = 0;
        uav_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        
        descriptor_ranges_.push_back(uav_range);
        
        D3D12_ROOT_PARAMETER uav_param = {};
        uav_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        uav_param.DescriptorTable.NumDescriptorRanges = 1;
        uav_param.DescriptorTable.pDescriptorRanges = &descriptor_ranges_[descriptor_ranges_.size() - 1];
        uav_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root_params.push_back(uav_param);
    }
    
    // SRV descriptor table (inputs)
    if (num_srvs_ > 0) {
        D3D12_DESCRIPTOR_RANGE srv_range = {};
        srv_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srv_range.NumDescriptors = num_srvs_;
        srv_range.BaseShaderRegister = 0;  // t0, t1, ...
        srv_range.RegisterSpace = 0;
        srv_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        
        descriptor_ranges_.push_back(srv_range);
        
        D3D12_ROOT_PARAMETER srv_param = {};
        srv_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        srv_param.DescriptorTable.NumDescriptorRanges = 1;
        srv_param.DescriptorTable.pDescriptorRanges = &descriptor_ranges_[descriptor_ranges_.size() - 1];
        srv_param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        root_params.push_back(srv_param);
    }
    
    // Root signature description
    D3D12_ROOT_SIGNATURE_DESC root_sig_desc = {};
    root_sig_desc.NumParameters = static_cast<UINT>(root_params.size());
    root_sig_desc.pParameters = root_params.empty() ? nullptr : root_params.data();
    root_sig_desc.NumStaticSamplers = 0;
    root_sig_desc.pStaticSamplers = nullptr;
    root_sig_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    
    // Serialize root signature
    ComPtr<ID3DBlob> signature_blob;
    ComPtr<ID3DBlob> error_blob;
    
    HRESULT hr = D3D12SerializeRootSignature(
        &root_sig_desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signature_blob,
        &error_blob
    );
    
    if (FAILED(hr)) {
        if (error_blob) {
            std::cerr << "DirectXPipeline: Root signature serialization failed: "
                     << static_cast<const char*>(error_blob->GetBufferPointer()) << "\n";
        }
        return false;
    }
    
    // Create root signature
    hr = context_->device()->CreateRootSignature(
        0,
        signature_blob->GetBufferPointer(),
        signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&root_signature_)
    );
    
    if (FAILED(hr)) {
        std::cerr << "DirectXPipeline: Failed to create root signature\n";
        return false;
    }
    
    return true;
}

bool DirectXPipeline::create_pipeline_state() {
    // Compute pipeline state description
    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.pRootSignature = root_signature_.Get();
    pso_desc.CS = shader_->get_bytecode();
    pso_desc.NodeMask = 0;
    pso_desc.CachedPSO.pCachedBlob = nullptr;
    pso_desc.CachedPSO.CachedBlobSizeInBytes = 0;
    pso_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    
    HRESULT hr = context_->device()->CreateComputePipelineState(
        &pso_desc,
        IID_PPV_ARGS(&pipeline_state_)
    );
    
    if (FAILED(hr)) {
        std::cerr << "DirectXPipeline: Failed to create pipeline state\n";
        return false;
    }
    
    std::cout << "DirectXPipeline: Created compute pipeline\n";
    return true;
}

void DirectXPipeline::set_root_constants(const void* data, uint32_t num_values) {
    if (num_values > num_root_constants_) {
        throw std::runtime_error("DirectXPipeline: Too many root constants");
    }
    // Ensure root signature is set before setting root constants
    context_->command_list()->SetComputeRootSignature(root_signature_.Get());

    // Root constants are at index 0
    context_->command_list()->SetComputeRoot32BitConstants(
        0,  // Root parameter index
        num_values,
        data,
        0   // Dest offset in values
    );
}

void DirectXPipeline::bind_uav_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor) {
    // UAV table is after root constants (index 1)
    uint32_t root_index = (num_root_constants_ > 0) ? 1 : 0;
    // Ensure root signature is set before binding descriptor tables
    context_->command_list()->SetComputeRootSignature(root_signature_.Get());
    context_->command_list()->SetComputeRootDescriptorTable(root_index, base_descriptor);
}

void DirectXPipeline::bind_srv_table(uint32_t table_index, D3D12_GPU_DESCRIPTOR_HANDLE base_descriptor) {
    // SRV table is after root constants and UAV table
    uint32_t root_index = 0;
    if (num_root_constants_ > 0) {
      root_index++;
    }
    if (num_uavs_ > 0) {
      root_index++;
    }
    // Ensure root signature is set before binding descriptor tables
    context_->command_list()->SetComputeRootSignature(root_signature_.Get());
    context_->command_list()->SetComputeRootDescriptorTable(root_index, base_descriptor);
}

void DirectXPipeline::dispatch(uint32_t thread_groups_x, 
                               uint32_t thread_groups_y, 
                               uint32_t thread_groups_z) {
    // Set pipeline state and root signature
    context_->command_list()->SetPipelineState(pipeline_state_.Get());
    context_->command_list()->SetComputeRootSignature(root_signature_.Get());
    
    // Dispatch compute shader
    context_->command_list()->Dispatch(thread_groups_x, thread_groups_y, thread_groups_z);
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
