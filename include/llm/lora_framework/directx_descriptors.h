/**
 * @file directx_descriptors.h
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
#include <d3d12.h>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief DirectX 12 descriptor heap manager
 * 
 * Manages descriptor heaps (CBV/SRV/UAV) for compute shaders.
 */
class DirectXDescriptors {
public:
    /**
     * @brief Create descriptor heap manager
     * @param context DirectX context
     * @param max_descriptors Maximum number of descriptors
     */
    DirectXDescriptors(DirectXContext* context, uint32_t max_descriptors = 256);
    
    ~DirectXDescriptors();
    
    // Disable copy, allow move
    DirectXDescriptors(const DirectXDescriptors&) = delete;
    DirectXDescriptors& operator=(const DirectXDescriptors&) = delete;
    DirectXDescriptors(DirectXDescriptors&&) noexcept;
    DirectXDescriptors& operator=(DirectXDescriptors&&) noexcept;
    
    /**
     * @brief Initialize descriptor heap
     */
    bool initialize();
    
    /**
     * @brief Create UAV (Unordered Access View) descriptor
     * @param resource Resource to create view for
     * @param num_elements Number of elements in buffer
     * @param element_size Size of each element in bytes
     * @return Descriptor index
     */
    uint32_t create_uav(ID3D12Resource* resource, uint32_t num_elements, uint32_t element_size);
    
    /**
     * @brief Create SRV (Shader Resource View) descriptor
     * @param resource Resource to create view for
     * @param num_elements Number of elements in buffer
     * @param element_size Size of each element in bytes
     * @return Descriptor index
     */
    uint32_t create_srv(ID3D12Resource* resource, uint32_t num_elements, uint32_t element_size);
    
    /**
     * @brief Get CPU descriptor handle
     * @param index Descriptor index
     */
    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(uint32_t index) const;
    
    /**
     * @brief Get GPU descriptor handle
     * @param index Descriptor index
     */
    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(uint32_t index) const;
    
    /**
     * @brief Get descriptor heap
     */
    ID3D12DescriptorHeap* heap() const { return descriptor_heap_.Get(); }
    
    /**
     * @brief Reset all descriptors (for reuse)
     */
    void reset();

private:
    DirectXContext* context_;
    uint32_t max_descriptors_ = 0;
    uint32_t current_descriptor_ = 0;
    uint32_t descriptor_increment_size_ = 0;
    
    ComPtr<ID3D12DescriptorHeap> descriptor_heap_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_heap_start_;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_heap_start_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
