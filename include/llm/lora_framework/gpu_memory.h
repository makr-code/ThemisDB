/**
 * @file gpu_memory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class DeviceType {
    CPU,        // CPU memory
    CUDA,       // NVIDIA GPU (CUDA)
    HIP,        // AMD GPU (HIP)
    VULKAN,     // Vulkan compute
    DIRECTX     // DirectX compute (Windows)
};

struct Device {
    DeviceType type = DeviceType::CPU;
    int device_id = 0;  // For multi-GPU systems
    
    // Factory methods
    /**
     * @brief Cpu.
     * @return Return value.
     * @details Implements cpu without additional internal calls.
     */
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
    
    /**
     * @brief To string.
     * @return Return value.
     */
    std::string to_string() const;
};

class GPUMemoryManager {
public:
    GPUMemoryManager();
    
    /**
     * @brief GPUMemory Manager.
     * @param[in] backend Input parameter.
     * @return Return value.
     */
    explicit GPUMemoryManager(acceleration::BackendType backend);
    
    ~GPUMemoryManager();
    
    // Disable copy, allow move
    GPUMemoryManager(const GPUMemoryManager&) = delete;
    GPUMemoryManager& operator=(const GPUMemoryManager&) = delete;
    GPUMemoryManager(GPUMemoryManager&&) noexcept;
    GPUMemoryManager& operator=(GPUMemoryManager&&) noexcept;
    
    /**
     * @brief Get allocator.
     * @param[in] device Input parameter.
     * @return Pointer to the result.
     */
    VRAMAllocator* get_allocator(const Device& device);
    
    Device default_device() const { return default_device_; }
    
    /**
     * @brief Is device available.
     * @param[in] device Input parameter.
     * @return True when the operation succeeds.
     */
    bool is_device_available(const Device& device) const;
    
    /**
     * @brief Available devices.
     * @return Return value.
     */
    std::vector<Device> available_devices() const;
    
    /**
     * @brief Auto select device.
     * @return Return value.
     */
    static Device auto_select_device();
    
    struct BackendInfo {
        acceleration::BackendType type;
        bool available = false;
        size_t vram_bytes = 0;
        int compute_units = 0;
        std::string device_name;
        std::string version;
    };
    
    /**
     * @brief Detect backends.
     * @return Return value.
     */
    static std::vector<BackendInfo> detect_backends();
    
    /**
     * @brief Get stats.
     * @param[in] device Input parameter.
     * @return Return value.
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
    
    /**
     * @brief Helper to initialize allocators
     * @param[in] preferred_backend Input parameter.
     */
    void initialize_allocators(acceleration::BackendType preferred_backend);
    
    /**
     * @brief Convert DeviceType to BackendType
     * @param[in] type Input parameter.
     * @return Return value.
     */
    static acceleration::BackendType device_to_backend(DeviceType type);
};

/**
 * @brief Device type to string.
 * @param[in] type Input parameter.
 * @return Return value.
 * @details Implements device_type_to_string without additional internal calls.
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
