/**
 * @file directx_context.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/directx_context.h"

#ifdef _WIN32

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>

// Link required D3D12 libraries
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace themis {
namespace lora {
namespace directx {

DirectXContext::DirectXContext(int adapter_id)
    : adapter_id_(adapter_id)
    , initialized_(false)
    , fence_value_(0)
    , fence_event_(nullptr) {
}

DirectXContext::~DirectXContext() {
    cleanup();
}

DirectXContext::DirectXContext(DirectXContext&& other) noexcept
    : adapter_id_(other.adapter_id_)
    , initialized_(other.initialized_)
    , gpu_description_(std::move(other.gpu_description_))
    , dxgi_factory_(std::move(other.dxgi_factory_))
    , adapter_(std::move(other.adapter_))
    , device_(std::move(other.device_))
    , command_queue_(std::move(other.command_queue_))
    , command_allocator_(std::move(other.command_allocator_))
    , command_list_(std::move(other.command_list_))
    , fence_(std::move(other.fence_))
    , fence_value_(other.fence_value_)
    , fence_event_(other.fence_event_) {
    
    other.initialized_ = false;
    other.fence_event_ = nullptr;
}

DirectXContext& DirectXContext::operator=(DirectXContext&& other) noexcept {
    if (this != &other) {
        cleanup();
        
        adapter_id_ = other.adapter_id_;
        initialized_ = other.initialized_;
        gpu_description_ = std::move(other.gpu_description_);
        dxgi_factory_ = std::move(other.dxgi_factory_);
        adapter_ = std::move(other.adapter_);
        device_ = std::move(other.device_);
        command_queue_ = std::move(other.command_queue_);
        command_allocator_ = std::move(other.command_allocator_);
        command_list_ = std::move(other.command_list_);
        fence_ = std::move(other.fence_);
        fence_value_ = other.fence_value_;
        fence_event_ = other.fence_event_;
        
        other.initialized_ = false;
        other.fence_event_ = nullptr;
    }
    return *this;
}

bool DirectXContext::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Enable debug layer in debug builds or when explicitly requested via env
    #ifdef _DEBUG
    enable_debug_layer();
    #else
    const char* dbg_env = std::getenv("THEMIS_ENABLE_D3D_DEBUG");
    if (dbg_env && std::string(dbg_env) == "1") {
        enable_debug_layer();
    }
    #endif
    
    // Create DXGI factory
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_factory_));
    if (FAILED(hr)) {
        std::cerr << "Failed to create DXGI factory\n";
        return false;
    }
    
    // Create device and other resources
    if (!create_device()) {
      return false;
    }
    if (!create_command_queue()) {
      return false;
    }
    if (!create_command_allocator()) {
      return false;
    }
    if (!create_command_list()) {
      return false;
    }
    if (!create_fence()) {
      return false;
    }
    
    initialized_ = true;
    return true;
}

void DirectXContext::cleanup() {
    if (!initialized_) {
        return;
    }
    
    // Wait for GPU to finish
    wait_for_gpu();
    
    // Close fence event
    if (fence_event_) {
        CloseHandle(fence_event_);
        fence_event_ = nullptr;
    }
    
    // Release COM objects (handled by ComPtr)
    fence_.Reset();
    command_list_.Reset();
    command_allocator_.Reset();
    command_queue_.Reset();
    device_.Reset();
    adapter_.Reset();
    dxgi_factory_.Reset();
    
    initialized_ = false;
}

bool DirectXContext::create_device() {
    // Enumerate adapters
    ComPtr<IDXGIAdapter1> temp_adapter;
    DXGI_ADAPTER_DESC1 desc;
    
    for (UINT i = 0; dxgi_factory_->EnumAdapters1(i, &temp_adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        temp_adapter->GetDesc1(&desc);
        
        // Skip software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }
        
        // Try to create device
        if (static_cast<int>(i) == adapter_id_) {
            HRESULT hr = D3D12CreateDevice(
                temp_adapter.Get(),
                D3D_FEATURE_LEVEL_12_0,
                IID_PPV_ARGS(&device_)
            );
            
            if (SUCCEEDED(hr)) {
                adapter_ = temp_adapter;
                
                // Convert wide string to regular string
                std::wstring w_desc(desc.Description);
                gpu_description_ = std::string(w_desc.begin(), w_desc.end());
                
                std::cout << "DirectX 12: Using adapter " << adapter_id_ 
                         << ": " << gpu_description_ << "\n";
                // If debug layer enabled, acquire the InfoQueue for message capture
                ComPtr<ID3D12InfoQueue> iq;
                if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&iq)))) {
                    info_queue_ = iq;
                    // Break on errors and corruption
                    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                    info_queue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                }
                return true;
            }
        }
    }
    
    std::cerr << "Failed to create D3D12 device for adapter " << adapter_id_ << "\n";
    return false;
}

bool DirectXContext::create_command_queue() {
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.NodeMask = 0;
    
    HRESULT hr = device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_));
    if (FAILED(hr)) {
        std::cerr << "Failed to create command queue\n";
        return false;
    }
    
    return true;
}

bool DirectXContext::create_command_allocator() {
    HRESULT hr = device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        IID_PPV_ARGS(&command_allocator_)
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create command allocator\n";
        return false;
    }
    
    return true;
}

bool DirectXContext::create_command_list() {
    HRESULT hr = device_->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_COMPUTE,
        command_allocator_.Get(),
        nullptr,
        IID_PPV_ARGS(&command_list_)
    );
    
    if (FAILED(hr)) {
        std::cerr << "Failed to create command list\n";
        return false;
    }
    
    // Command lists are created in recording state; close it to start idle.
    command_list_->Close();
    command_list_recording_ = false;

    return true;
}

bool DirectXContext::create_fence() {
    HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    if (FAILED(hr)) {
        std::cerr << "Failed to create fence\n";
        return false;
    }
    
    fence_value_ = 1;
    
    // Create event for fence signaling
    fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!fence_event_) {
        std::cerr << "Failed to create fence event\n";
        return false;
    }
    
    return true;
}

void DirectXContext::enable_debug_layer() {
    ComPtr<ID3D12Debug> debug_controller;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
        debug_controller->EnableDebugLayer();
        std::cout << "DirectX 12 debug layer enabled\n";
    }
}

bool DirectXContext::wait_for_gpu(uint32_t timeout_ms) {
    if (!fence_ || !fence_event_) {
        return false;
    }
    
    // Signal fence with current value
    const uint64_t fence_to_wait = fence_value_;
    HRESULT hr = command_queue_->Signal(fence_.Get(), fence_to_wait);
    if (FAILED(hr)) {
        std::cerr << "Failed to signal fence\n";
        return false;
    }
    fence_value_++;
    
    // Wait for fence to reach the signaled value
    if (fence_->GetCompletedValue() < fence_to_wait) {
        hr = fence_->SetEventOnCompletion(fence_to_wait, fence_event_);
        if (FAILED(hr)) {
            std::cerr << "Failed to set fence event\n";
            return false;
        }
        const DWORD wait_result = WaitForSingleObject(fence_event_, timeout_ms);
        if (wait_result == WAIT_TIMEOUT) {
            std::cerr << "DirectX wait_for_gpu timed out after " << timeout_ms << " ms\n";
            return false;
        }
        if (wait_result != WAIT_OBJECT_0) {
            std::cerr << "DirectX wait_for_gpu failed with wait code " << wait_result << "\n";
            return false;
        }
    }
    return true;
}

void DirectXContext::reset_command_list() {
    // If a command list is already recording, execute it first to free the allocator
    if (command_list_recording_) {
        execute_command_list();
    }

    // Reset allocator
    HRESULT hr = command_allocator_->Reset();
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to reset command allocator");
    }

    // Reset command list
    hr = command_list_->Reset(command_allocator_.Get(), nullptr);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to reset command list");
    }
    command_list_recording_ = true;
}

void DirectXContext::execute_command_list([[maybe_unused]] uint32_t timeout_ms) {
    // Close command list
    HRESULT hr = command_list_->Close();
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to close command list");
    }
    command_list_recording_ = false;
    
    // Execute command list
    ID3D12CommandList* cmd_lists[] = {command_list_.Get()};
    command_queue_->ExecuteCommandLists(1, cmd_lists);
    
    // Wait for completion
    if (!wait_for_gpu(timeout_ms)) {
        throw std::runtime_error("DirectX command execution timed out or failed while waiting for GPU");
    }

    // Dump any stored debug messages
    if (info_queue_) {
        UINT64 num = info_queue_->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 i = 0; i < num; ++i) {
            SIZE_T msgLen = 0;
            info_queue_->GetMessage(i, nullptr, &msgLen);
            std::vector<char> buffer(msgLen);
            D3D12_MESSAGE* msg = reinterpret_cast<D3D12_MESSAGE*>(buffer.data());
            info_queue_->GetMessage(i, msg, &msgLen);
            std::cerr << "D3D12 INFOQUEUE: " << msg->pDescription << "\n";
        }
        info_queue_->ClearStoredMessages();
    }
}

} // namespace directx
} // namespace lora
} // namespace themis

#endif // _WIN32
