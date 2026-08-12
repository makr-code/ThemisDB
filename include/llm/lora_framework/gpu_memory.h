/**
 * @file gpu_memory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/vram_allocator.h"
#include "acceleration/compute_backend.h"
#include <memory>
#include <string>
#include <vector>

namespace themis {
namespace llm {
namespace lora {

/**
 * @brief Device location for tensors
 */
enum class DeviceType {
    CPU,        // CPU memory
    CUDA,       // NVIDIA GPU (CUDA)
    HIP,        // AMD GPU (HIP)
    VULKAN,     // Vulkan compute
    DIRECTX     // DirectX compute (Windows)
};

/**
 * @brief Device descriptor
 */
struct Device {
    DeviceType type = DeviceType::CPU;
    int device_id = 0;  // For multi-GPU systems
    
    // Factory methods
    static Device cpu() { return Device{DeviceType::CPU, 0}; }
    static Device cuda(int id = 0) { return Device{DeviceType::CUDA, id}; }
    static Device hip(int id = 0) { return Device{DeviceType::HIP, id}; }
    static Device vulkan(int id = 0) { return Device{DeviceType::VULKAN, id}; }
    static Device directx(int id = 0) { return Device{DeviceType::DIRECTX, id}; }
    
    bool operator==(const Device& other) const {
        return type == other.type && device_id == other.device_id;
    }
    
    bool operator!=(const Device& other) const {
        return !(*this == other);
    }
    
    std::string to_string() const;
};

/**
 * @brief GPU Memory Manager
 * 
 * High-level GPU memory management for LoRA training:
 * - Automatic backend selection
 * - Device migration (CPU ↔ GPU)
 * - Memory pooling across devices
 * - Unified memory support (where available)
 */
class GPUMemoryManager {
public:
    /**
     * @brief Initialize GPU memory manager with auto-selected backend
     * 
     * Priority: Vulkan → CUDA → HIP → DirectX → CPU
     */
    GPUMemoryManager();
    
    /**
     * @brief Initialize with specific backend
     */
    explicit GPUMemoryManager(acceleration::BackendType backend);
    
    ~GPUMemoryManager();
    
    // Disable copy, allow move
    GPUMemoryManager(const GPUMemoryManager&) = delete;
    GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
    GPUMemoryManager(GPUMemoryManager&&) noexcept;
    GPUMemoryManager& operator=(GPUMemoryManager&&) noexcept;
    
    /**
     * @brief Get allocator for specific device
     */
    VRAMAllocator* get_allocator(const Device& device);
    
    /**
     * @brief Get default device (best available GPU or CPU)
     */
    Device default_device() const { return default_device_; }
    
    /**
     * @brief Check if device is available
     */
    bool is_device_available(const Device& device) const;
    
    /**
     * @brief Get available devices
     */
    std::vector<Device> available_devices() const;
    
    /**
     * @brief Auto-select best available device
     * Priority: Vulkan → CUDA → HIP → DirectX → CPU
     */
    static Device auto_select_device();
    
    /**
     * @brief Detect backend capabilities
     */
    struct BackendInfo {
        acceleration::BackendType type;
        bool available = false;
        size_t vram_bytes = 0;
        int compute_units = 0;
        std::string device_name;
        std::string version;
    };
    
    static std::vector<BackendInfo> detect_backends();
    
    /**
     * @brief Get memory statistics for device
     */
    VRAMAllocator::Stats get_stats(const Device& device) const;

private:
    Device default_device_;
    
    // Allocators for different backends
    std::unique_ptr<VRAMAllocator> cpu_allocator_;
    std::unique_ptr<VRAMAllocator> cuda_allocator_;
    std::unique_ptr<VRAMAllocator> hip_allocator_;
    std::unique_ptr<VRAMAllocator> vulkan_allocator_;
    std::unique_ptr<VRAMAllocator> directx_allocator_;
    
    // Helper to initialize allocators
    void initialize_allocators(acceleration::BackendType preferred_backend);
    
    // Convert DeviceType to BackendType
    static acceleration::BackendType device_to_backend(DeviceType type);
};

/**
 * @brief Convert device type to string
 */
inline std::string device_type_to_string(DeviceType type) {
    switch (type) {
        case DeviceType::CPU: return "CPU";
        case DeviceType::CUDA: return "CUDA";
        case DeviceType::HIP: return "HIP";
        case DeviceType::VULKAN: return "Vulkan";
        case DeviceType::DIRECTX: return "DirectX";
        default: return "Unknown";
    }
}

inline std::string Device::to_string() const {
    return device_type_to_string(type) + ":" + std::to_string(device_id);
}

} // namespace lora
} // namespace llm
} // namespace themis
