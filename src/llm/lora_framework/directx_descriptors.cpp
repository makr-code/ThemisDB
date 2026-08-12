/**
 * @file directx_descriptors.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/directx_descriptors.h"

#ifdef _WIN32

#include <stdexcept>
#include <iostream>

namespace themis {
namespace lora {
namespace directx {

DirectXDescriptors::DirectXDescriptors(DirectXContext* context, uint32_t max_descriptors)
    : context_(context)
    , max_descriptors_(max_descriptors)
    , current_descriptor_(0)
    , descriptor_increment_size_(0) {
    
    if (!context_ || !context_->is_initialized()) {
        throw std::runtime_error("DirectXDescriptors: Invalid context");
    }
}

DirectXDescriptors::~DirectXDescriptors() {
    // COM pointers automatically release
}

DirectXDescriptors::DirectXDescriptors(DirectXDescriptors&& other) noexcept
    : context_(other.context_)
    , max_descriptors_(other.max_descriptors_)
    , current_descriptor_(other.current_descriptor_)
    , descriptor_increment_size_(other.descriptor_increment_size_)
    , descriptor_heap_(std::move(other.descriptor_heap_))
    , cpu_heap_start_(other.cpu_heap_start_)
    , gpu_heap_start_(other.gpu_heap_start_) {
    
    other.context_ = nullptr;
}

DirectXDescriptors& DirectXDescriptors::operator=(DirectXDescriptors&& other) noexcept {
    if (this != &other) {
        context_ = other.context_;
        max_descriptors_ = other.max_descriptors_;
        current_descriptor_ = other.current_descriptor_;
        descriptor_increment_size_ = other.descriptor_increment_size_;
        descriptor_heap_ = std::move(other.descriptor_heap_);
        cpu_heap_start_ = other.cpu_heap_start_;
        gpu_heap_start_ = other.gpu_heap_start_;
        
        other.context_ = nullptr;
    }
    return *this;
}

bool DirectXDescriptors::initialize() {
    // Create CBV/SRV/UAV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.NumDescriptors = max_descriptors_;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NodeMask = 0;
    
    HRESULT hr = context_->device()->CreateDescriptorHeap(
        &heap_desc,
        IID_PPV_ARGS(&descriptor_heap_)
    );
    
    if (FAILED(hr)) {
        std::cerr << "DirectXDescriptors: Failed to create descriptor heap\n";
        return false;
    }
    
    // Get descriptor increment size
    descriptor_increment_size_ = context_->device()->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
    
    // Get heap start handles
    cpu_heap_start_ = descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
    gpu_heap_start_ = descriptor_heap_->GetGPUDescriptorHandleForHeapStart();
    
    return true;
}

uint32_t DirectXDescriptors::create_uav(ID3D12Resource* resource, 
                                        uint32_t num_elements, 
                                        uint32_t element_size) {
    if (current_descriptor_ >= max_descriptors_) {
        throw std::runtime_error("DirectXDescriptors: Descriptor heap full");
    }
    
    uint32_t descriptor_index = current_descriptor_++;
    
    // Create UAV description
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
    uav_desc.Format = DXGI_FORMAT_UNKNOWN;
    uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uav_desc.Buffer.FirstElement = 0;
    uav_desc.Buffer.NumElements = num_elements;
    uav_desc.Buffer.StructureByteStride = element_size;
    uav_desc.Buffer.CounterOffsetInBytes = 0;
    uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    
    // Create UAV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_handle(descriptor_index);
    context_->device()->CreateUnorderedAccessView(resource, nullptr, &uav_desc, handle);
    
    return descriptor_index;
}

uint32_t DirectXDescriptors::create_srv(ID3D12Resource* resource, 
                                        uint32_t num_elements, 
                                        uint32_t element_size) {
    if (current_descriptor_ >= max_descriptors_) {
        throw std::runtime_error("DirectXDescriptors: Descriptor heap full");
    }
    
    uint32_t descriptor_index = current_descriptor_++;
    
    // Create SRV description
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Format = DXGI_FORMAT_UNKNOWN;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Buffer.FirstElement = 0;
    srv_desc.Buffer.NumElements = num_elements;
    srv_desc.Buffer.StructureByteStride = element_size;
    srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    
    // Create SRV
    D3D12_CPU_DESCRIPTOR_HANDLE handle = get_cpu_handle(descriptor_index);
    context_->device()->CreateShaderResourceView(resource, &srv_desc, handle);
    
    return descriptor_index;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_cpu_handle(uint32_t index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = cpu_heap_start_;
    handle.ptr += index * descriptor_increment_size_;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXDescriptors::get_gpu_handle(uint32_t index) const {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = gpu_heap_start_;
    handle.ptr += index * descriptor_increment_size_;
    return handle;
}

void DirectXDescriptors::reset() {
    current_descriptor_ = 0;
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
