/**
 * @file gpu_memory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_memory.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

// Backend-specific detection
#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#include <vector>
#endif

#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
#include <dxgi1_4.h>
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#endif

namespace themis {
namespace llm {
namespace lora {

// ============================================================================
// GPUMemoryManager Implementation
// ============================================================================

GPUMemoryManager::GPUMemoryManager() {
    // Auto-select best backend
    default_device_ = auto_select_device();
    initialize_allocators(device_to_backend(default_device_.type));
}

GPUMemoryManager::GPUMemoryManager(acceleration::BackendType backend) {
    switch (backend) {
        case acceleration::BackendType::CUDA:
            default_device_ = Device::cuda();
            break;
        case acceleration::BackendType::HIP:
            default_device_ = Device::hip();
            break;
        case acceleration::BackendType::VULKAN:
            default_device_ = Device::vulkan();
            break;
        case acceleration::BackendType::DIRECTX:
            default_device_ = Device::directx();
            break;
        case acceleration::BackendType::CPU:
            default_device_ = Device::cpu();
            break;
        // Handle other backend types
        case acceleration::BackendType::ZLUDA:
        [[fallthrough]];
        case acceleration::BackendType::ROCM:
        [[fallthrough]];
        case acceleration::BackendType::OPENGL:
        [[fallthrough]];
        case acceleration::BackendType::METAL:
        [[fallthrough]];
        case acceleration::BackendType::ONEAPI:
        [[fallthrough]];
        case acceleration::BackendType::OPENCL:
        [[fallthrough]];
        case acceleration::BackendType::WEBGPU:
        [[fallthrough]];
        case acceleration::BackendType::AUTO:
        [[fallthrough]];
        case acceleration::BackendType::MULTI_GPU:
        [[fallthrough]];
        case acceleration::BackendType::NPU_APPLE:
        [[fallthrough]];
        case acceleration::BackendType::NPU_INTEL:
        [[fallthrough]];
        case acceleration::BackendType::NPU_QUALCOMM:
        [[fallthrough]];
        case acceleration::BackendType::NPU_ARM:
        [[fallthrough]];
        case acceleration::BackendType::NNAPI:
            // Fallback to CPU for unsupported backends
            default_device_ = Device::cpu();
            break;
    }
    
    initialize_allocators(backend);
}

GPUMemoryManager::~GPUMemoryManager() = default;

GPUMemoryManager::GPUMemoryManager(GPUMemoryManager&& other) noexcept
    : default_device_(other.default_device_)
    , cpu_allocator_(std::move(other.cpu_allocator_))
    , cuda_allocator_(std::move(other.cuda_allocator_))
    , hip_allocator_(std::move(other.hip_allocator_))
    , vulkan_allocator_(std::move(other.vulkan_allocator_))
    , directx_allocator_(std::move(other.directx_allocator_)) {
}

GPUMemoryManager& GPUMemoryManager::operator=(GPUMemoryManager&& other) noexcept {
    if (this != &other) {
        default_device_ = other.default_device_;
        cpu_allocator_ = std::move(other.cpu_allocator_);
        cuda_allocator_ = std::move(other.cuda_allocator_);
        hip_allocator_ = std::move(other.hip_allocator_);
        vulkan_allocator_ = std::move(other.vulkan_allocator_);
        directx_allocator_ = std::move(other.directx_allocator_);
    }
    return *this;
}

VRAMAllocator* GPUMemoryManager::get_allocator(const Device& device) {
    switch (device.type) {
        case DeviceType::CPU:
            if (!cpu_allocator_) {
                cpu_allocator_ = std::make_unique<VRAMAllocator>(
                    acceleration::BackendType::CPU);
            }
            return cpu_allocator_.get();
            
        case DeviceType::CUDA:
            if (!cuda_allocator_) {
                cuda_allocator_ = std::make_unique<VRAMAllocator>(
                    acceleration::BackendType::CUDA);
            }
            return cuda_allocator_.get();
            
        case DeviceType::HIP:
            if (!hip_allocator_) {
                hip_allocator_ = std::make_unique<VRAMAllocator>(
                    acceleration::BackendType::HIP);
            }
            return hip_allocator_.get();
            
        case DeviceType::VULKAN:
            if (!vulkan_allocator_) {
                vulkan_allocator_ = std::make_unique<VRAMAllocator>(
                    acceleration::BackendType::VULKAN);
            }
            return vulkan_allocator_.get();
            
        case DeviceType::DIRECTX:
            if (!directx_allocator_) {
                directx_allocator_ = std::make_unique<VRAMAllocator>(
                    acceleration::BackendType::DIRECTX);
            }
            return directx_allocator_.get();
            
        default:
            return nullptr;
    }
}

bool GPUMemoryManager::is_device_available(const Device& device) const {
    auto backends = detect_backends();
    auto backend = device_to_backend(device.type);
    
    for (const auto& info : backends) {
        if (info.type == backend && info.available) {
            return true;
        }
    }
    
    return device.type == DeviceType::CPU; // CPU always available
}

std::vector<Device> GPUMemoryManager::available_devices() const {
    std::vector<Device> devices;
    
    // CPU always available
    devices.push_back(Device::cpu());
    
    auto backends = detect_backends();
    for (const auto& info : backends) {
        if (info.available) {
            switch (info.type) {
                case acceleration::BackendType::CUDA:
                    devices.push_back(Device::cuda());
                    break;
                case acceleration::BackendType::HIP:
                    devices.push_back(Device::hip());
                    break;
                case acceleration::BackendType::VULKAN:
                    devices.push_back(Device::vulkan());
                    break;
                case acceleration::BackendType::DIRECTX:
                    devices.push_back(Device::directx());
                    break;
                default:
                    break;
            }
        }
    }
    
    return devices;
}

Device GPUMemoryManager::auto_select_device() {
    // Priority: Vulkan → CUDA → HIP → DirectX → CPU
    auto backends = detect_backends();
    
    // Try Vulkan first (cross-platform)
    for (const auto& info : backends) {
        if (info.type == acceleration::BackendType::VULKAN && info.available) {
            return Device::vulkan();
        }
    }
    
    // Try CUDA (NVIDIA)
    for (const auto& info : backends) {
        if (info.type == acceleration::BackendType::CUDA && info.available) {
            return Device::cuda();
        }
    }
    
    // Try HIP (AMD)
    for (const auto& info : backends) {
        if (info.type == acceleration::BackendType::HIP && info.available) {
            return Device::hip();
        }
    }
    
    // Try DirectX (Windows)
    for (const auto& info : backends) {
        if (info.type == acceleration::BackendType::DIRECTX && info.available) {
            return Device::directx();
        }
    }
    
    // Fallback to CPU
    return Device::cpu();
}

std::vector<GPUMemoryManager::BackendInfo> GPUMemoryManager::detect_backends() {
    std::vector<BackendInfo> backends;
    
    // Detect CUDA
#ifdef THEMIS_ENABLE_CUDA
    {
        BackendInfo info;
        info.type = acceleration::BackendType::CUDA;
        
        int device_count = 0;
        cudaError_t err = cudaGetDeviceCount(&device_count);
        if (err == cudaSuccess && device_count > 0) {
            info.available = true;
            
            // Query device 0 properties
            cudaDeviceProp prop{};
            cudaError_t prop_err = cudaGetDeviceProperties(&prop, 0);
            if (prop_err != cudaSuccess) {
                spdlog::warn("GPUMemoryManager: cudaGetDeviceProperties failed: {}",
                             cudaGetErrorString(prop_err));
            } else {
                info.device_name = prop.name;
                info.vram_bytes = prop.totalGlobalMem;
                info.compute_units = prop.multiProcessorCount;
            }
            
            int runtime_version = 0;
            // REL-18a: check cudaRuntimeGetVersion return value
            {
                cudaError_t ver_err = cudaRuntimeGetVersion(&runtime_version);
                if (ver_err != cudaSuccess) {
                    spdlog::warn("GPUMemoryManager: cudaRuntimeGetVersion failed: {}",
                                 cudaGetErrorString(ver_err));
                    runtime_version = 0;
                }
            }
            info.version = std::to_string(runtime_version / 1000) + "." +
                          std::to_string((runtime_version % 100) / 10);
        }
        
        backends.push_back(info);
    }
#endif
    
    // Detect HIP
#ifdef THEMIS_ENABLE_HIP
    {
        BackendInfo info;
        info.type = acceleration::BackendType::HIP;
        
        int device_count = 0;
        hipError_t err = hipGetDeviceCount(&device_count);
        if (err == hipSuccess && device_count > 0) {
            info.available = true;
            
            hipDeviceProp_t prop{};
            // REL-18b: zero-init prop and check hipGetDeviceProperties return value
            {
                hipError_t prop_err = hipGetDeviceProperties(&prop, 0);
                if (prop_err != hipSuccess) {
                    spdlog::warn("GPUMemoryManager: hipGetDeviceProperties failed: {}",
                                 hipGetErrorString(prop_err));
                } else {
                    info.device_name = prop.name;
                    info.vram_bytes = prop.totalGlobalMem;
                    info.compute_units = prop.multiProcessorCount;
                }
            }

            int runtime_version = 0;
            // REL-18c: check hipRuntimeGetVersion return value
            {
                hipError_t ver_err = hipRuntimeGetVersion(&runtime_version);
                if (ver_err != hipSuccess) {
                    spdlog::warn("GPUMemoryManager: hipRuntimeGetVersion failed: {}",
                                 hipGetErrorString(ver_err));
                    runtime_version = 0;
                }
            }
            info.version = std::to_string(runtime_version / 1000) + "." +
                          std::to_string((runtime_version % 100) / 10);
        }
        
        backends.push_back(info);
    }
#endif
    
    // Detect Vulkan
#ifdef THEMIS_ENABLE_VULKAN
    {
        BackendInfo info;
        info.type = acceleration::BackendType::VULKAN;

        VkApplicationInfo appInfo{};
        appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instCI{};
        instCI.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instCI.pApplicationInfo = &appInfo;

        VkInstance instance = VK_NULL_HANDLE;
        VkResult create_result = vkCreateInstance(&instCI, nullptr, &instance);
        if (create_result == VK_SUCCESS) {
            uint32_t deviceCount = 0;
            VkResult enumerate_count_result =
                vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (enumerate_count_result != VK_SUCCESS) {
                spdlog::warn("GPUMemoryManager: vkEnumeratePhysicalDevices(count) failed: {}",
                             static_cast<int>(enumerate_count_result));
            } else if (deviceCount > 0) {
                std::vector<VkPhysicalDevice> physDevices(deviceCount);
                VkResult enumerate_fill_result =
                    vkEnumeratePhysicalDevices(instance, &deviceCount, physDevices.data());
                if (enumerate_fill_result == VK_SUCCESS || enumerate_fill_result == VK_INCOMPLETE) {
                    if (deviceCount == 0) {
                        spdlog::warn("GPUMemoryManager: vkEnumeratePhysicalDevices(fill) returned no devices");
                    } else {
                    // Use first physical device's properties
                    VkPhysicalDeviceProperties props{};
                    vkGetPhysicalDeviceProperties(physDevices[0], &props);

                    info.available   = true;
                    info.device_name = props.deviceName;
                    info.version     = std::to_string(VK_VERSION_MAJOR(props.apiVersion)) + "." +
                                       std::to_string(VK_VERSION_MINOR(props.apiVersion)) + "." +
                                       std::to_string(VK_VERSION_PATCH(props.apiVersion));

                    // Report device-local heap size as VRAM
                    VkPhysicalDeviceMemoryProperties memProps{};
                    vkGetPhysicalDeviceMemoryProperties(physDevices[0], &memProps);
                    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
                        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                            info.vram_bytes = memProps.memoryHeaps[i].size;
                            break;
                        }
                    }
                    }
                } else {
                    spdlog::warn("GPUMemoryManager: vkEnumeratePhysicalDevices(fill) failed: {}",
                                 static_cast<int>(enumerate_fill_result));
                }
            } else {
                spdlog::debug("GPUMemoryManager: Vulkan backend available but no physical device found");
            }
            vkDestroyInstance(instance, nullptr);
        } else {
            spdlog::warn("GPUMemoryManager: vkCreateInstance probe failed: {}",
                         static_cast<int>(create_result));
        }

        backends.push_back(info);
    }
#else
    {
        BackendInfo info;
        info.type      = acceleration::BackendType::VULKAN;
        info.available = false;
        backends.push_back(info);
    }
#endif
    
    // Detect DirectX
#if defined(_WIN32) && defined(THEMIS_ENABLE_DIRECTX)
    {
        BackendInfo info;
        info.type = acceleration::BackendType::DIRECTX;

        IDXGIFactory1* factory = nullptr;
        if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                         reinterpret_cast<void**>(&factory)))) {
            IDXGIAdapter1* adapter = nullptr;
            for (UINT i = 0;
                 factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND;
                 ++i) {
                DXGI_ADAPTER_DESC1 desc{};
                if (SUCCEEDED(adapter->GetDesc1(&desc)) &&
                    !(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
                    // Verify D3D12 support (nullptr device — no object created)
                    if (SUCCEEDED(D3D12CreateDevice(adapter,
                                                     D3D_FEATURE_LEVEL_11_0,
                                                     __uuidof(ID3D12Device),
                                                     nullptr))) {
                        info.available = true;
                        // Convert UTF-16 device name to UTF-8 using WideCharToMultiByte
                        int len = WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                                                       nullptr, 0, nullptr, nullptr);
                        if (len > 0) {
                            info.device_name.resize(static_cast<size_t>(len) - 1);
                            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1,
                                                &info.device_name[0], len, nullptr, nullptr);
                        }
                        info.vram_bytes  = static_cast<size_t>(
                            desc.DedicatedVideoMemory);
                        adapter->Release();
                        break;
                    }
                }
                adapter->Release();
            }
            factory->Release();
        }

        backends.push_back(info);
    }
#else
    {
        BackendInfo info;
        info.type      = acceleration::BackendType::DIRECTX;
        info.available = false;
        backends.push_back(info);
    }
#endif
    
    // CPU always available
    {
        BackendInfo info;
        info.type = acceleration::BackendType::CPU;
        info.available = true;
        info.device_name = "CPU";
        backends.push_back(info);
    }
    
    return backends;
}

VRAMAllocator::Stats GPUMemoryManager::get_stats(const Device& device) const {
    switch (device.type) {
        case DeviceType::CPU:
            return cpu_allocator_ ? cpu_allocator_->get_stats() : VRAMAllocator::Stats{};
        case DeviceType::CUDA:
            return cuda_allocator_ ? cuda_allocator_->get_stats() : VRAMAllocator::Stats{};
        case DeviceType::HIP:
            return hip_allocator_ ? hip_allocator_->get_stats() : VRAMAllocator::Stats{};
        case DeviceType::VULKAN:
            return vulkan_allocator_ ? vulkan_allocator_->get_stats() : VRAMAllocator::Stats{};
        case DeviceType::DIRECTX:
            return directx_allocator_ ? directx_allocator_->get_stats() : VRAMAllocator::Stats{};
        default:
            return VRAMAllocator::Stats{};
    }
}

void GPUMemoryManager::initialize_allocators(acceleration::BackendType preferred_backend) {
    // Always create CPU allocator as fallback
    cpu_allocator_ = std::make_unique<VRAMAllocator>(acceleration::BackendType::CPU);
    
    // Create preferred backend allocator
    switch (preferred_backend) {
        case acceleration::BackendType::CUDA:
            cuda_allocator_ = std::make_unique<VRAMAllocator>(acceleration::BackendType::CUDA);
            if (!cuda_allocator_->is_available()) {
                cuda_allocator_.reset();
            }
            break;
            
        case acceleration::BackendType::HIP:
            hip_allocator_ = std::make_unique<VRAMAllocator>(acceleration::BackendType::HIP);
            if (!hip_allocator_->is_available()) {
                hip_allocator_.reset();
            }
            break;
            
        case acceleration::BackendType::VULKAN:
            vulkan_allocator_ = std::make_unique<VRAMAllocator>(acceleration::BackendType::VULKAN);
            if (!vulkan_allocator_->is_available()) {
                vulkan_allocator_.reset();
            }
            break;
            
        case acceleration::BackendType::DIRECTX:
            directx_allocator_ = std::make_unique<VRAMAllocator>(acceleration::BackendType::DIRECTX);
            if (!directx_allocator_->is_available()) {
                directx_allocator_.reset();
            }
            break;
            
        default:
            break;
    }
}

acceleration::BackendType GPUMemoryManager::device_to_backend(DeviceType type) {
    switch (type) {
        case DeviceType::CPU: return acceleration::BackendType::CPU;
        case DeviceType::CUDA: return acceleration::BackendType::CUDA;
        case DeviceType::HIP: return acceleration::BackendType::HIP;
        case DeviceType::VULKAN: return acceleration::BackendType::VULKAN;
        case DeviceType::DIRECTX: return acceleration::BackendType::DIRECTX;
        default: return acceleration::BackendType::CPU;
    }
}

} // namespace lora
} // namespace llm
} // namespace themis
