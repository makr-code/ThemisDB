#include "llm/lora_framework/gpu_utilization_monitor.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>
#include <numeric>

// NVML support for NVIDIA GPUs
#ifdef THEMIS_ENABLE_CUDA
#include <nvml.h>
#endif

// ROCm SMI support for AMD GPUs
#ifdef THEMIS_ENABLE_HIP
#include <rocm_smi/rocm_smi.h>
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
    Metrics metrics;
    
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

bool GPUUtilizationMonitor::isUnderutilized(float threshold) const {
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

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::getAverageMetrics(size_t num_samples) const {
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
    
    nvml_device_ = static_cast<void*>(device);
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
    
    nvmlDevice_t device = static_cast<nvmlDevice_t>(nvml_device_);
    
    // GPU utilization
    nvmlUtilization_t util;
    nvmlReturn_t result = nvmlDeviceGetUtilizationRates(device, &util);
    if (result == NVML_SUCCESS) {
        metrics.gpu_utilization_pct = static_cast<float>(util.gpu);
        metrics.memory_utilization_pct = static_cast<float>(util.memory);
    }
    
    // Memory info
    nvmlMemory_t mem;
    result = nvmlDeviceGetMemoryInfo(device, &mem);
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
    
    spdlog::info("ROCm SMI initialized successfully");
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
    uint32_t dv_ind = static_cast<uint32_t>(device_.device_id);
    
    // GPU busy percentage
    uint32_t busy_percent;
    rsmi_status_t result = rsmi_dev_busy_percent_get(dv_ind, &busy_percent);
    if (result == RSMI_STATUS_SUCCESS) {
        metrics.gpu_utilization_pct = static_cast<float>(busy_percent);
    }
    
    // Memory usage
    uint64_t mem_used, mem_total;
    result = rsmi_dev_memory_usage_get(dv_ind, RSMI_MEM_TYPE_VRAM, &mem_used);
    if (result == RSMI_STATUS_SUCCESS) {
        result = rsmi_dev_memory_total_get(dv_ind, RSMI_MEM_TYPE_VRAM, &mem_total);
        if (result == RSMI_STATUS_SUCCESS && mem_total > 0) {
            metrics.memory_utilization_pct = 100.0f * mem_used / mem_total;
        }
    }
    
    spdlog::debug("ROCm SMI metrics: GPU={:.1f}%, Memory={:.1f}%",
                 metrics.gpu_utilization_pct, metrics.memory_utilization_pct);
#endif
    
    return metrics;
}

// Vulkan implementation (stub - would need VK_EXT_performance_query)
bool GPUUtilizationMonitor::initializeVulkan() {
    spdlog::debug("Vulkan performance monitoring not yet implemented");
    return false;
}

void GPUUtilizationMonitor::shutdownVulkan() {
    // No-op
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryVulkan() {
    return getFallbackMetrics();
}

// DirectX implementation (stub - would need D3D12 performance counters)
bool GPUUtilizationMonitor::initializeDirectX() {
    spdlog::debug("DirectX performance monitoring not yet implemented");
    return false;
}

void GPUUtilizationMonitor::shutdownDirectX() {
    // No-op
}

GPUUtilizationMonitor::Metrics GPUUtilizationMonitor::queryDirectX() {
    return getFallbackMetrics();
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
