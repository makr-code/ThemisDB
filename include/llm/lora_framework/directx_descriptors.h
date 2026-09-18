/**
 * @file directx_descriptors.h
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
#include <d3d12.h>
#include <cstdint>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

class DirectXDescriptors {
public:
    DirectXDescriptors(DirectXContext* context, uint32_t max_descriptors = 256);
    
    ~DirectXDescriptors() noexcept;
    
    // Disable copy, allow move
    DirectXDescriptors(const DirectXDescriptors&) = delete;
    DirectXDescriptors& operator=(const DirectXDescriptors&) = delete;
    DirectXDescriptors(DirectXDescriptors&&) noexcept;
    DirectXDescriptors& operator=(DirectXDescriptors&&) noexcept;
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Create uav.
     * @param[in,out] resource Input/output parameter.
     * @param[in] num_elements Input parameter.
     * @param[in] element_size Input parameter.
     * @return Return value.
     */
    uint32_t create_uav(ID3D12Resource* resource, uint32_t num_elements, uint32_t element_size);
    
    /**
     * @brief Create srv.
     * @param[in,out] resource Input/output parameter.
     * @param[in] num_elements Input parameter.
     * @param[in] element_size Input parameter.
     * @return Return value.
     */
    uint32_t create_srv(ID3D12Resource* resource, uint32_t num_elements, uint32_t element_size);
    
    /**
     * @brief Get cpu handle.
     * @param[in] index Input parameter.
     * @return Return value.
     */
    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(uint32_t index) const;
    
    /**
     * @brief Get gpu handle.
     * @param[in] index Input parameter.
     * @return Return value.
     */
    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(uint32_t index) const;
    
    ID3D12DescriptorHeap* heap() const { return descriptor_heap_.Get(); }
    
    /**
     * @brief Reset the modification detection flag.
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
