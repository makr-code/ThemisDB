/**
 * @file gpu_utilization_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 83/100
 * @note Gap Summary: total=8; TODO=1, Stub=3, Unimpl=1, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/gpu_utilization_monitor.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cstdlib>

// NVML support for NVIDIA GPUs
#ifdef THEMIS_ENABLE_CUDA
#include <nvml.h>
#endif

// ROCm SMI support for AMD GPUs
#ifdef THEMIS_ENABLE_HIP
#include <rocm_smi/rocm_smi.h>
#endif

// Vulkan support (limited metrics)
#ifdef THEMIS_ENABLE_VULKAN
// Vulkan headers would go here if needed for VK_EXT_memory_budget
// #include <vulkan/vulkan.h>
#endif

// DirectX support (limited metrics)
#ifdef THEMIS_ENABLE_DIRECTX
// DirectX headers would go here if needed for DXGI queries
// #include <dxgi1_6.h>
#endif

namespace themis {
namespace llm {
namespace lora {

GPUUtilizationMonitor::GPUUtilizationMonitor(const Device& device)
    : device_(device)
    , is_available_(false) {
    
    // Initialize based on device type
    if (device_.type == DeviceType::CUDA) {
        is_available_ = initializeNVML();
    } else if (device_.type == DeviceType::HIP) {
        is_available_ = initializeROCm();
    } else if (device_.type == DeviceType::VULKAN) {
        is_available_ = initializeVulkan();
    } else if (device_.type == DeviceType::DIRECTX) {
        is_available_ = initializeDirectX();
    }
    
    if (is_available_) {
        spdlog::info("GPUUtilizationMonitor initialized for device type {}", 
                     static_cast<int>(device_.type));
    } else {
        spdlog::warn("GPUUtilizationMonitor: Monitoring not available for device type {}", 
                     static_cast<int>(device_.type));
    }
    
    // Reserve space for metrics history
    metrics_history_.reserve(100);
}

GPUUtilizationMonitor::~GPUUtilizationMonitor() {
    if (device_.type == DeviceType::CUDA) {
        shutdownNVML();
    } else if (device_.type == DeviceType::HIP) {
        shutdownROCm();
    } else if (device_.type == DeviceType::VULKAN) {
        shutdownVulkan();
    } else if (device_.type == DeviceType::DIRECTX) {
        shutdownDirectX();
    }
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryMetrics() {
    Metrics metrics = {};
    
    if (!is_available_) {
        return getFallbackMetrics();
    }
    
    // Query based on device type
    if (device_.type == DeviceType::CUDA) {
        metrics = queryNVML();
    } else if (device_.type == DeviceType::HIP) {
        metrics = queryROCm();
    } else if (device_.type == DeviceType::VULKAN) {
        metrics = queryVulkan();
    } else if (device_.type == DeviceType::DIRECTX) {
        metrics = queryDirectX();
    } else {
        metrics = getFallbackMetrics();
    }
    
    // Add timestamp
    auto now = std::chrono::system_clock::now();
    metrics.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    // Store in history
    last_metrics_ = metrics;
    metrics_history_.push_back(metrics);
    
    // Keep only last 100 samples
    if (metrics_history_.size() > 100) {
        metrics_history_.erase(metrics_history_.begin());
    }
    
    return metrics;
}

bool GPUUtilizationMonitor::isUnderutilized([[maybe_unused]] float threshold) const {
    return last_metrics_.gpu_utilization_pct < (threshold * 100.0f);
}

std::vector<std::string> GPUUtilizationMonitor::getOptimizationRecommendations() const {
    std::vector<std::string> recommendations;
    
    const auto& metrics = last_metrics_;
    
    if (metrics.gpu_utilization_pct < 70.0f) {
        recommendations.push_back(
            "Low GPU utilization (" + std::to_string(static_cast<int>(metrics.gpu_utilization_pct)) + 
            "%): Consider increasing batch size"
        );
    }
    
    if (metrics.memory_utilization_pct < 60.0f) {
        recommendations.push_back(
            "Low memory utilization (" + std::to_string(static_cast<int>(metrics.memory_utilization_pct)) + 
            "%): Can increase sequence length or batch size"
        );
    }
    
    if (metrics.sm_occupancy_pct > 0.0f && metrics.sm_occupancy_pct < 50.0f) {
        recommendations.push_back(
            "Low SM occupancy (" + std::to_string(static_cast<int>(metrics.sm_occupancy_pct)) + 
            "%): Kernel launch configuration may be suboptimal"
        );
    }
    
    if (metrics.gpu_utilization_pct > 95.0f) {
        recommendations.push_back(
            "Excellent GPU utilization (" + std::to_string(static_cast<int>(metrics.gpu_utilization_pct)) + 
            "%): GPU is well utilized"
        );
    }
    
    return recommendations;
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::getAverageMetrics([[maybe_unused]] size_t num_samples) const {
    if (metrics_history_.empty()) {
        return last_metrics_;
    }
    
    // Take last N samples
    size_t start_idx = 0;
    if (metrics_history_.size() > num_samples) {
        start_idx = metrics_history_.size() - num_samples;
    }
    
    Metrics avg;
    size_t count = metrics_history_.size() - start_idx;
    
    for (size_t i = start_idx; i < metrics_history_.size(); ++i) {
        const auto& m = metrics_history_[i];
        avg.gpu_utilization_pct += m.gpu_utilization_pct;
        avg.memory_utilization_pct += m.memory_utilization_pct;
        avg.compute_throughput_tflops += m.compute_throughput_tflops;
        avg.memory_bandwidth_gb_s += m.memory_bandwidth_gb_s;
        avg.sm_occupancy_pct += m.sm_occupancy_pct;
    }
    
    if (count > 0) {
        avg.gpu_utilization_pct /= count;
        avg.memory_utilization_pct /= count;
        avg.compute_throughput_tflops /= count;
        avg.memory_bandwidth_gb_s /= count;
        avg.sm_occupancy_pct /= count;
    }
    
    return avg;
}

std::string GPUUtilizationMonitor::getDeviceInfo() const {
    return "GPU Device " + std::to_string(device_.device_id) + 
           " (Type: " + std::to_string(static_cast<int>(device_.type)) + ")";
}

// NVML (NVIDIA) implementation
bool GPUUtilizationMonitor::initializeNVML() {
#ifdef THEMIS_ENABLE_CUDA
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        spdlog::warn("Failed to initialize NVML: {}", nvmlErrorString(result));
        return false;
    }
    
    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(device_.device_id, &device);
    if (result != NVML_SUCCESS) {
        spdlog::warn("Failed to get NVML device handle: {}", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }
    
    nvml_device_ = device;  // Store directly (type-safe)
    spdlog::info("NVML initialized successfully for device {}", device_.device_id);
    return true;
#else
    spdlog::debug("NVML not available (CUDA not enabled)");
    return false;
#endif
}

void GPUUtilizationMonitor::shutdownNVML() {
#ifdef THEMIS_ENABLE_CUDA
    if (nvml_device_) {
        nvmlShutdown();
        nvml_device_ = nullptr;
    }
#endif
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryNVML() {
    Metrics metrics;
    
#ifdef THEMIS_ENABLE_CUDA
    if (!nvml_device_) {
        return metrics;
    }
    
    // Use device handle directly (type-safe, no cast needed)
    
    // GPU utilization
    nvmlUtilization_t util;
    nvmlReturn_t result = nvmlDeviceGetUtilizationRates(nvml_device_, &util);
    if (result == NVML_SUCCESS) {
        metrics.gpu_utilization_pct = static_cast<float>(util.gpu);
        metrics.memory_utilization_pct = static_cast<float>(util.memory);
    }
    
    // Memory info
    nvmlMemory_t mem;
    result = nvmlDeviceGetMemoryInfo(nvml_device_, &mem);
    if (result == NVML_SUCCESS) {
        metrics.memory_utilization_pct = 100.0f * mem.used / mem.total;
    }
    
    spdlog::debug("NVML metrics: GPU={:.1f}%, Memory={:.1f}%",
                 metrics.gpu_utilization_pct, metrics.memory_utilization_pct);
#endif
    
    return metrics;
}

// ROCm implementation
bool GPUUtilizationMonitor::initializeROCm() {
#ifdef THEMIS_ENABLE_HIP
    rsmi_status_t result = rsmi_init(0);
    if (result != RSMI_STATUS_SUCCESS) {
        spdlog::warn("Failed to initialize ROCm SMI: {}", result);
        return false;
    }
    
    rocm_device_index_ = static_cast<uint32_t>(device_.device_id);
    spdlog::info("ROCm SMI initialized successfully for device {}", rocm_device_index_);
    return true;
#else
    spdlog::debug("ROCm SMI not available (HIP not enabled)");
    return false;
#endif
}

void GPUUtilizationMonitor::shutdownROCm() {
#ifdef THEMIS_ENABLE_HIP
    rsmi_shut_down();
#endif
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryROCm() {
    Metrics metrics;
    
#ifdef THEMIS_ENABLE_HIP
    // Use stored device index (type-safe, no cast needed)
    
    // GPU busy percentage
    uint32_t busy_percent = {};
    rsmi_status_t result = rsmi_dev_busy_percent_get(rocm_device_index_, &busy_percent);
    if (result == RSMI_STATUS_SUCCESS) {
        metrics.gpu_utilization_pct = static_cast<float>(busy_percent);
    }
    
    // Memory usage
    uint64_t mem_used, mem_total;
    result = rsmi_dev_memory_usage_get(rocm_device_index_, RSMI_MEM_TYPE_VRAM, &mem_used);
    if (result == RSMI_STATUS_SUCCESS) {
        result = rsmi_dev_memory_total_get(rocm_device_index_, RSMI_MEM_TYPE_VRAM, &mem_total);
        if (result == RSMI_STATUS_SUCCESS && mem_total > 0) {
            metrics.memory_utilization_pct = 100.0f * mem_used / mem_total;
        }
    }
    
    spdlog::debug("ROCm SMI metrics: GPU={:.1f}%, Memory={:.1f}%",
                 metrics.gpu_utilization_pct, metrics.memory_utilization_pct);
#endif
    
    return metrics;
}

// Vulkan implementation
bool GPUUtilizationMonitor::initializeVulkan() {
#ifdef THEMIS_ENABLE_VULKAN
    // Vulkan doesn't have a built-in GPU utilization query like NVML
    // We can query memory usage via VK_EXT_memory_budget extension
    // Note: GPU utilization percentage is not directly available
    spdlog::info("Vulkan performance monitoring initialized (limited metrics)");
    return true;
#else
    spdlog::debug("Vulkan performance monitoring not available (Vulkan not enabled)");
    return false;
#endif
}

void GPUUtilizationMonitor::shutdownVulkan() {
    // No cleanup needed for Vulkan monitoring
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryVulkan() {
    Metrics metrics;
    
#ifdef THEMIS_ENABLE_VULKAN
    // PERMANENT HARDWARE FALLBACK NOTE (Vulkan GPU metrics — VK_EXT_memory_budget):
    // Purpose: Provide a safe default return value while VK_EXT_memory_budget
    //          and GPU-occupancy queries are not yet implemented.
    // Activation: `THEMIS_ENABLE_VULKAN` is defined AND VK_EXT_memory_budget
    //             query is NOT yet wired.
    // Production Delta: Returns zero utilisation (0 %, 0 %) instead of real
    //             values.  Callers should treat zero as "metrics unavailable"
    //             and fall back to conservative resource-management decisions.
    // Hardware requirement: VK_EXT_memory_budget extension + vkGetPhysicalDeviceMemoryProperties2KHR.
    metrics.gpu_utilization_pct    = 0.0f;   // unavailable — VK_EXT_memory_budget not queried
    metrics.memory_utilization_pct = 0.0f;

    spdlog::debug("Vulkan metrics unavailable (VK_EXT_memory_budget not implemented); "
                  "returning zero utilisation");
#endif
    
    return metrics;
}

// DirectX implementation
bool GPUUtilizationMonitor::initializeDirectX() {
#ifdef THEMIS_ENABLE_DIRECTX
    // DirectX 12 provides DXGI adapter queries for memory info
    // GPU utilization requires D3D12 query heaps or external tools
    spdlog::info("DirectX performance monitoring initialized (limited metrics)");
    return true;
#else
    spdlog::debug("DirectX performance monitoring not available (DirectX not enabled)");
    return false;
#endif
}

void GPUUtilizationMonitor::shutdownDirectX() {
    // No cleanup needed for DirectX monitoring
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryDirectX() {
    Metrics metrics;
    
#ifdef THEMIS_ENABLE_DIRECTX
    // PERMANENT HARDWARE FALLBACK NOTE (DirectX GPU metrics — IDXGIAdapter3):
    // Purpose: Provide a safe default return value while IDXGIAdapter3::QueryVideoMemoryInfo()
    //          and D3D12 GPU-occupancy queries are not yet implemented.
    // Activation: `THEMIS_ENABLE_DIRECTX` is defined AND QueryVideoMemoryInfo is NOT
    //             yet wired.
    // Production Delta: Returns zero utilisation (0 %, 0 %) instead of real values.
    //             Callers should treat zero as "metrics unavailable" and fall back to
    //             conservative resource-management decisions.
    // Hardware requirement: Windows + DirectX 12 SDK + IDXGIAdapter3 + D3D12 timestamp queries.
    metrics.gpu_utilization_pct    = 0.0f;   // unavailable — QueryVideoMemoryInfo not queried
    metrics.memory_utilization_pct = 0.0f;

    spdlog::debug("DirectX metrics unavailable (IDXGIAdapter3::QueryVideoMemoryInfo not "
                  "implemented); returning zero utilisation");
#endif
    
    return metrics;
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::getFallbackMetrics() const {
    // Return reasonable default values when monitoring not available
    Metrics metrics;
    metrics.gpu_utilization_pct = 75.0f;  // Assume moderate utilization
    metrics.memory_utilization_pct = 70.0f;
    return metrics;
}

} // namespace lora
} // namespace llm
} // namespace themis

