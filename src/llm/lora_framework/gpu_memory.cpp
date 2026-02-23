/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_memory.cpp                                     ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     390                                            ║
    • Open Issues:     TODOs: 2, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_framework/gpu_memory.h"
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
        case acceleration::BackendType::ROCM:
        case acceleration::BackendType::OPENGL:
        case acceleration::BackendType::METAL:
        case acceleration::BackendType::ONEAPI:
        case acceleration::BackendType::OPENCL:
        case acceleration::BackendType::WEBGPU:
        case acceleration::BackendType::AUTO:
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
            cudaDeviceProp prop;
            cudaGetDeviceProperties(&prop, 0);
            
            info.device_name = prop.name;
            info.vram_bytes = prop.totalGlobalMem;
            info.compute_units = prop.multiProcessorCount;
            
            int runtime_version;
            cudaRuntimeGetVersion(&runtime_version);
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
            
            hipDeviceProp_t prop;
            hipGetDeviceProperties(&prop, 0);
            
            info.device_name = prop.name;
            info.vram_bytes = prop.totalGlobalMem;
            info.compute_units = prop.multiProcessorCount;
            
            int runtime_version;
            hipRuntimeGetVersion(&runtime_version);
            info.version = std::to_string(runtime_version / 1000) + "." + 
                          std::to_string((runtime_version % 100) / 10);
        }
        
        backends.push_back(info);
    }
#endif
    
    // Detect Vulkan — enumerate physical devices and pick the best compute-capable one.
    // Prefers non-NVIDIA discrete GPUs so that Vulkan serves as the primary fallback
    // for AMD, Intel, ARM, and other non-CUDA hardware.
    {
        BackendInfo info;
        info.type = acceleration::BackendType::VULKAN;

#ifdef THEMIS_ENABLE_VULKAN
        VkApplicationInfo appInfo{};
        appInfo.sType      = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_0;
        VkInstanceCreateInfo ci{};
        ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &appInfo;

        VkInstance vkInst = VK_NULL_HANDLE;
        if (vkCreateInstance(&ci, nullptr, &vkInst) == VK_SUCCESS) {
            uint32_t devCount = 0;
            vkEnumeratePhysicalDevices(vkInst, &devCount, nullptr);
            if (devCount > 0) {
                std::vector<VkPhysicalDevice> vkDevs(devCount);
                vkEnumeratePhysicalDevices(vkInst, &devCount, vkDevs.data());

                // Select best device: non-NVIDIA discrete > NVIDIA discrete > integrated > other
                VkPhysicalDevice chosen = VK_NULL_HANDLE;
                VkPhysicalDeviceProperties chosenProps{};
                int bestScore = -1;
                for (const auto& d : vkDevs) {
                    VkPhysicalDeviceProperties p;
                    vkGetPhysicalDeviceProperties(d, &p);

                    // Check for at least one compute queue family
                    uint32_t qfCount = 0;
                    vkGetPhysicalDeviceQueueFamilyProperties(d, &qfCount, nullptr);
                    std::vector<VkQueueFamilyProperties> qfProps(qfCount);
                    vkGetPhysicalDeviceQueueFamilyProperties(d, &qfCount, qfProps.data());
                    bool hasCompute = false;
                    for (uint32_t qi = 0; qi < qfCount; ++qi) {
                        if (qfProps[qi].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                            hasCompute = true;
                            break;
                        }
                    }
                    if (!hasCompute) continue;

                    int score = 0;
                    if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                        score = (p.vendorID != vendor_id::NVIDIA) ? 30 : 20;
                    } else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                        score = (p.vendorID != vendor_id::NVIDIA) ? 12 : 10;
                    } else if (p.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
                        score = 5;
                    } else {
                        score = 1;
                    }

                    if (score > bestScore) {
                        bestScore   = score;
                        chosen      = d;
                        chosenProps = p;
                    }
                }

                if (chosen != VK_NULL_HANDLE) {
                    info.available   = true;
                    info.device_name = std::string(chosenProps.deviceName);

                    // Report device-local memory
                    VkPhysicalDeviceMemoryProperties memProps{};
                    vkGetPhysicalDeviceMemoryProperties(chosen, &memProps);
                    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
                        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                            info.vram_bytes = memProps.memoryHeaps[i].size;
                            break;
                        }
                    }
                }
            }
            vkDestroyInstance(vkInst, nullptr);
        }
#endif // THEMIS_ENABLE_VULKAN

        backends.push_back(info);
    }
    
    // Detect DirectX
    {
        BackendInfo info;
        info.type = acceleration::BackendType::DIRECTX;
        // TODO: Implement DirectX detection via D3D12
        // For now, mark as potentially available on Windows
#ifdef _WIN32
        info.available = false;  // Will be implemented in DirectX phase
#else
        info.available = false;
#endif
        backends.push_back(info);
    }
    
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
