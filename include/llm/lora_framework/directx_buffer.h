/**
 * @file directx_buffer.h
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
#include <cstddef>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief DirectX 12 buffer wrapper for GPU memory management
 * 
 * Manages ID3D12Resource objects with support for:
 * - Upload heaps (CPU→GPU)
 * - Default heaps (GPU-only, fastest)
 * - Readback heaps (GPU→CPU)
 */
class DirectXBuffer {
public:
    /**
     * @brief Create DirectX buffer
     * @param context DirectX context
     * @param size Buffer size in bytes
     * @param usage Buffer usage flags
     */
    DirectXBuffer(DirectXContext* context, size_t size, 
                  D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    
    ~DirectXBuffer();
    
    // Disable copy, allow move
    DirectXBuffer(const DirectXBuffer&) = delete;
    DirectXBuffer& operator=(const DirectXBuffer&) = delete;
    DirectXBuffer(DirectXBuffer&&) noexcept;
    DirectXBuffer& operator=(DirectXBuffer&&) noexcept;
    
    /**
     * @brief Upload data from CPU to GPU
     * @param data Source CPU data
     * @param size Size in bytes
     */
    void upload(const void* data, size_t size);
    
    /**
     * @brief Download data from GPU to CPU
     * @param data Destination CPU buffer
     * @param size Size in bytes
     */
    void download(void* data, size_t size);
    
    /**
     * @brief Get underlying D3D12 resource (default heap)
     */
    ID3D12Resource* resource() const { return resource_.Get(); }
    
    /**
     * @brief Get buffer size in bytes
     */
    size_t size() const { return size_; }
    
    /**
     * @brief Get GPU virtual address
     */
    D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { 
        return resource_ ? resource_->GetGPUVirtualAddress() : 0; 
    }
    
    /**
     * @brief Transition resource state
     */
    void transition_state(D3D12_RESOURCE_STATES new_state);
    
    /**
     * @brief Get current resource state
     */
    D3D12_RESOURCE_STATES current_state() const { return current_state_; }

private:
    bool create_default_buffer();
    bool create_upload_buffer();
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
