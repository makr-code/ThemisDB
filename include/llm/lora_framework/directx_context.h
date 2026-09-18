/**
 * @file directx_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef _WIN32

#include <d3d12.h>
#include <dxgi1_6.h>

// WRL (Windows Runtime Library) ComPtr support
// Prefer the SDK-provided <wrl/client.h> when available; otherwise provide
// a minimal, local ComPtr fallback so builds on trimmed SDKs succeed.
#if defined(__has_include)
  #if __has_include(<wrl/client.h>)
    #include <wrl/client.h>
    using Microsoft::WRL::ComPtr;
  #else
    // Minimal ComPtr fallback when WRL isn't present in the include paths
    namespace Microsoft {
      namespace WRL {
        template <typename T>
        class ComPtr {
        private:
          T* ptr = nullptr;
        public:
          ComPtr() = default;
          ~ComPtr() { if (ptr) ptr->Release(); }
          ComPtr(T* p) : ptr(p) { if (ptr) ptr->AddRef(); }
          ComPtr(const ComPtr& other) { ptr = other.ptr; if (ptr) ptr->AddRef(); }
          ComPtr& operator=(const ComPtr& other) {
            if (this == &other) {
              return *this;
            }
            if (ptr) {
              ptr->Release();
            }
            ptr = other.ptr;
            if (ptr) {
              ptr->AddRef();
            }
            return *this;
          }
          T* Get() const { return ptr; }
          /**
           * @brief Get Address Of.
           * @return Pointer to the result.
           * @details Implements GetAddressOf without additional internal calls.
           */
          T** GetAddressOf() { return &ptr; }
          /**
           * @brief Release And Get Address Of.
           * @return Pointer to the result.
           * @details Calls: Reset().
           */
          T** ReleaseAndGetAddressOf() { Reset(); return &ptr; }
          T** operator&() { return GetAddressOf(); }
          bool operator==(std::nullptr_t) const { return ptr == nullptr; }
          bool operator!=(std::nullptr_t) const { return ptr != nullptr; }
          T* operator->() const { return ptr; }
          T& operator*() const { return *ptr; }
          explicit operator bool() const { return ptr != nullptr; }
          /**
           * @brief Reset.
           * @details Calls: Release().
           */
          void Reset() { if (ptr) { ptr->Release(); ptr = nullptr; } }
          ComPtr& operator=(T* p) {
            if (ptr) {
              ptr->Release();
            }
            ptr = p;
            if (ptr) {
              ptr->AddRef();
            }
            return *this;
          }
        };
      }
    }
    using Microsoft::WRL::ComPtr;
  #endif
#else
  // No __has_include support — attempt to include the SDK header and fall back
  #include <wrl/client.h>
  using Microsoft::WRL::ComPtr;
#endif

#include <cstdint>
#include <memory>
#include <string>

namespace themis {
namespace lora {
namespace directx {

class DirectXContext {
public:
    explicit DirectXContext(int adapter_id = 0);
    
    ~DirectXContext();
    
    // Disable copy, allow move
    DirectXContext(const DirectXContext&) = delete;
    DirectXContext& operator=(const DirectXContext&) = delete;
    DirectXContext(DirectXContext&&) noexcept;
    DirectXContext& operator=(DirectXContext&&) noexcept;
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     */
    bool initialize();
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
    
    bool is_initialized() const { return initialized_; }
    
    ID3D12Device* device() const { return device_.Get(); }
    
    ID3D12CommandQueue* command_queue() const { return command_queue_.Get(); }
    
    ID3D12CommandAllocator* command_allocator() const { return command_allocator_.Get(); }
    
    ID3D12GraphicsCommandList* command_list() const { return command_list_.Get(); }
    
    ID3D12Fence* fence() const { return fence_.Get(); }
    
    uint64_t fence_value() const { return fence_value_; }
    
    int adapter_id() const { return adapter_id_; }
    
    bool wait_for_gpu(uint32_t timeout_ms = 30000);
    
    /**
     * @brief Reset command list.
     */
    void reset_command_list();
    
    void execute_command_list(uint32_t timeout_ms = 30000);
    
    const std::string& get_gpu_description() const { return gpu_description_; }

private:
    /**
     * @brief Create device.
     * @return True when the operation succeeds.
     */
    bool create_device();
    /**
     * @brief Create command queue.
     * @return True when the operation succeeds.
     */
    bool create_command_queue();
    /**
     * @brief Create command allocator.
     * @return True when the operation succeeds.
     */
    bool create_command_allocator();
    /**
     * @brief Create command list.
     * @return True when the operation succeeds.
     */
    bool create_command_list();
    /**
     * @brief Create fence.
     * @return True when the operation succeeds.
     */
    bool create_fence();
    /**
     * @brief Enable debug layer.
     */
    void enable_debug_layer();
    
    int adapter_id_ = 0;
    bool initialized_ = false;
    std::string gpu_description_;
    
    // D3D12 objects
    ComPtr<IDXGIFactory4> dxgi_factory_;
    ComPtr<IDXGIAdapter1> adapter_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12InfoQueue> info_queue_;
    ComPtr<ID3D12CommandQueue> command_queue_;
    ComPtr<ID3D12CommandAllocator> command_allocator_;
    ComPtr<ID3D12GraphicsCommandList> command_list_;
    ComPtr<ID3D12Fence> fence_;
    
    // Synchronization
    uint64_t fence_value_ = 0;
    void* fence_event_;  // HANDLE on Windows
    // Whether the command list is currently in recording state
    bool command_list_recording_ = false;
};

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
