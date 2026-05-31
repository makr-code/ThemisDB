/*
 * ThemisDB | File: directx_context.h | Version: 0.0.47 | Last Modified: 2026-05-27 04:01:17
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 153
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #572 Complete DirectX 12 Compute... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#ifdef _WIN32

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cstdint>
#include <memory>
#include <string>

using Microsoft::WRL::ComPtr;

namespace themis {
namespace lora {
namespace directx {

/**
 * @brief DirectX 12 context for compute operations
 * 
 * Manages D3D12 device, command queue, and synchronization primitives
 * for GPU-accelerated LoRA training on Windows.
 */
class DirectXContext {
public:
    /**
     * @brief Initialize DirectX 12 context
     * @param adapter_id GPU adapter ID (0 for default adapter)
     */
    explicit DirectXContext(int adapter_id = 0);
    
    ~DirectXContext();
    
    // Disable copy, allow move
    DirectXContext(const DirectXContext&) = delete;
    DirectXContext& operator=(const DirectXContext&) = delete;
    DirectXContext(DirectXContext&&) noexcept;
    DirectXContext& operator=(DirectXContext&&) noexcept;
    
    /**
     * @brief Initialize D3D12 device and resources
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Cleanup DirectX resources
     */
    void cleanup();
    
    /**
     * @brief Check if context is initialized
     */
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief Get D3D12 device
     */
    ID3D12Device* device() const { return device_.Get(); }
    
    /**
     * @brief Get compute command queue
     */
    ID3D12CommandQueue* command_queue() const { return command_queue_.Get(); }
    
    /**
     * @brief Get command allocator
     */
    ID3D12CommandAllocator* command_allocator() const { return command_allocator_.Get(); }
    
    /**
     * @brief Get command list
     */
    ID3D12GraphicsCommandList* command_list() const { return command_list_.Get(); }
    
    /**
     * @brief Get fence for GPU synchronization
     */
    ID3D12Fence* fence() const { return fence_.Get(); }
    
    /**
     * @brief Get current fence value
     */
    uint64_t fence_value() const { return fence_value_; }
    
    /**
     * @brief Get adapter ID
     */
    int adapter_id() const { return adapter_id_; }
    
    /**
     * @brief Wait for GPU to complete all pending work
     * @param timeout_ms Timeout in milliseconds
     * @return true if the GPU completed within timeout
     */
    bool wait_for_gpu(uint32_t timeout_ms = 30000);
    
    /**
     * @brief Reset command list for new recording
     */
    void reset_command_list();
    
    /**
     * @brief Execute command list and wait for completion
     * @param timeout_ms Timeout in milliseconds
     */
    void execute_command_list(uint32_t timeout_ms = 30000);
    
    /**
     * @brief Get GPU description string
     */
    const std::string& get_gpu_description() const { return gpu_description_; }

private:
    bool create_device();
    bool create_command_queue();
    bool create_command_allocator();
    bool create_command_list();
    bool create_fence();
    void enable_debug_layer();
    
    int adapter_id_ = 0;
    bool initialized_ = false;
    std::string gpu_description_;
    
    // D3D12 objects
    ComPtr<IDXGIFactory4> dxgi_factory_;
    ComPtr<IDXGIAdapter1> adapter_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> command_queue_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> command_list_;
    ComPtr<ID3D12Fence> fence_;
    
    // Synchronization
    uint64_t fence_value_ = 0;
    void* fence_event_;  // HANDLE on Windows
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
