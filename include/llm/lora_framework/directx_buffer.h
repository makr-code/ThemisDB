/**
 * @file directx_buffer.h
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
#include <cstddef>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

class DirectXBuffer {
public:
    DirectXBuffer(DirectXContext* context, size_t size, 
                  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    ~DirectXBuffer() noexcept;
    
    // Disable copy, allow move
    DirectXBuffer(const DirectXBuffer&) = delete;
    DirectXBuffer& operator=(const DirectXBuffer&) = delete;
    DirectXBuffer(DirectXBuffer&&) noexcept;
    DirectXBuffer& operator=(DirectXBuffer&&) noexcept;
    
    /**
     * @brief Upload.
     * @param[in] data Input parameter.
     * @param[in] size Input parameter.
     */
    void upload(const void* data, size_t size);
    
    /**
     * @brief Download.
     * @param[in,out] data Input/output parameter.
     * @param[in] size Input parameter.
     */
    void download(void* data, size_t size);
    
    ID3D12Resource* resource() const { return resource_.Get(); }
    
    size_t size() const { return size_; }
    
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { 
        return resource_ ? resource_->GetGPUVirtualAddress() : 0; 
    }
    
    /**
     * @brief Transition state.
     * @param[in] new_state Input parameter.
     */
    void transition_state(D3D12_RESOURCE_STATES new_state);
    
    D3D12_RESOURCE_STATES current_state() const { return current_state_; }

private:
    /**
     * @brief Create default buffer.
     * @return True when the operation succeeds.
     */
    bool create_default_buffer();
    /**
     * @brief Create upload buffer.
     * @return True when the operation succeeds.
     */
    bool create_upload_buffer();
    /**
     * @brief Create readback buffer.
     * @return True when the operation succeeds.
     */
    bool create_readback_buffer();
    
    DirectXContext* context_;
    size_t size_ = 0;
    D3D12_RESOURCE_FLAGS flags_;
    D3D12_RESOURCE_STATES current_state_;
    
    // GPU buffer (default heap)
    ComPtr<ID3D12Resource> resource_;
    
    // Staging buffers
    ComPtr<ID3D12Resource> upload_buffer_;
    ComPtr<ID3D12Resource> readback_buffer_;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
