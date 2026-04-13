/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            directx_context.h                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:23:06                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
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
     */
    void wait_for_gpu();
    
    /**
     * @brief Reset command list for new recording
     */
    void reset_command_list();
    
    /**
     * @brief Execute command list and wait for completion
     */
    void execute_command_list();
    
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
    
    int adapter_id_;
    bool initialized_;
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
    uint64_t fence_value_;
    void* fence_event_;  // HANDLE on Windows
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
