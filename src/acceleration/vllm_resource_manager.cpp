/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vllm_resource_manager.cpp                          ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/vllm_resource_manager.h"
#include "utils/logger.h"
#include <thread>
#include <algorithm>

#ifdef THEMIS_ENABLE_CUDA
    #include <cuda_runtime.h>
    #ifdef __linux__
        #include <nvml.h>
    #endif
#endif

namespace themis {
namespace acceleration {

VLLMResourceManager::VLLMResourceManager(const Config& config)
    : config_(config) {}

VLLMResourceManager::~VLLMResourceManager() {
    shutdown();
}

bool VLLMResourceManager::initialize() {
    if (initialized_) {
        THEMIS_WARN("VLLMResourceManager already initialized");
        return true;
    }
    
    // Detect system resources
    config_.total_cpu_cores = std::thread::hardware_concurrency();
    
    // Calculate ThemisDB allocation (remaining after vLLM)
    config_.themis_cpu_cores = config_.total_cpu_cores - config_.vllm_cpu_cores;
    config_.themis_ram_gb = config_.total_ram_gb - config_.vllm_ram_gb;
    
    THEMIS_INFO("VLLMResourceManager initialized:");
    THEMIS_INFO("  System: {} CPU cores, {} GB RAM", 
                config_.total_cpu_cores, config_.total_ram_gb);
    THEMIS_INFO("  vLLM reservation: {} cores, {} GB RAM", 
                config_.vllm_cpu_cores, config_.vllm_ram_gb);
    THEMIS_INFO("  ThemisDB allocation: {} cores, {} GB RAM", 
                config_.themis_cpu_cores, config_.themis_ram_gb);
    
    // Initialize NVML for GPU monitoring
#ifdef THEMIS_ENABLE_CUDA
    if (!initializeNVML()) {
        THEMIS_WARN("NVML initialization failed - GPU monitoring disabled");
    }
#else
    THEMIS_INFO("CUDA not enabled - GPU monitoring disabled");
#endif
    
    initialized_ = true;
    return true;
}

void VLLMResourceManager::shutdown() {
    if (!initialized_) return;
    
#ifdef THEMIS_ENABLE_CUDA
    shutdownNVML();
#endif
    
    initialized_ = false;
    THEMIS_INFO("VLLMResourceManager shutdown");
}

bool VLLMResourceManager::canUseGPU() {
#ifndef THEMIS_ENABLE_CUDA
    return false;  // CUDA not enabled
#else
    
    auto gpu_util = queryGPUUtilization();
    if (!gpu_util.has_value()) {
        // Can't query GPU - assume busy (safe fallback to CPU)
        return false;
    }
    
    // Only use GPU if vLLM is not heavily utilizing it (< 80%)
    bool can_use = gpu_util.value() < 80.0;
    
    if (!can_use) {
        THEMIS_DEBUG("GPU busy ({}% utilization) - using CPU fallback", gpu_util.value());
    }
    
    return can_use;
#endif
}

size_t VLLMResourceManager::getRecommendedThreadCount(const std::string& operation_type) const {
    if (!initialized_) {
        return std::thread::hardware_concurrency();
    }
    
    if (operation_type == "rocksdb") {
        return static_cast<size_t>(config_.themis_cpu_cores * config_.rocksdb_thread_ratio);
    } else if (operation_type == "tbb") {
        return static_cast<size_t>(config_.themis_cpu_cores * config_.tbb_thread_ratio);
    } else {
        // General purpose - use TBB allocation
        return static_cast<size_t>(config_.themis_cpu_cores * config_.tbb_thread_ratio);
    }
}

VLLMResourceManager::Stats VLLMResourceManager::getStats() const {
    Stats stats;
    
    if (!initialized_) {
        return stats;
    }
    
    // CPU stats (basic metrics - OS integration recommended for production)
    stats.active_threads = config_.themis_cpu_cores;
    stats.cpu_utilization = 0.0;  // Note: Implement OS-specific CPU monitoring for accurate metrics
    
    // RAM stats (basic metrics - OS integration recommended for production)
    stats.ram_used_mb = 0;  // Note: Implement OS-specific memory monitoring for accurate metrics
    stats.ram_utilization = 0.0;
    
#ifdef THEMIS_ENABLE_CUDA
    // GPU stats via NVML
    auto gpu_util = const_cast<VLLMResourceManager*>(this)->queryGPUUtilization();
    if (gpu_util.has_value()) {
        stats.gpu_available = true;
        stats.gpu_utilization = gpu_util.value();
        
        // Estimate vLLM usage (anything > 20% is likely vLLM)
        if (stats.gpu_utilization > 20.0) {
            stats.vllm_detected = true;
            stats.vllm_gpu_usage = stats.gpu_utilization;
        }
    }
#endif
    
    return stats;
}

void VLLMResourceManager::setConfig(const Config& config) {
    if (initialized_) {
        THEMIS_WARN("Cannot change config while initialized - call shutdown() first");
        return;
    }
    config_ = config;
}

bool VLLMResourceManager::initializeNVML() {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        THEMIS_ERROR("NVML initialization failed: {}", nvmlErrorString(result));
        return false;
    }
    
    // Get first GPU device
    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        THEMIS_ERROR("Failed to get NVML device handle: {}", nvmlErrorString(result));
        nvmlShutdown();
        return false;
    }
    
    nvml_device_ = static_cast<void*>(device);
    THEMIS_INFO("NVML initialized for GPU monitoring");
    return true;
#else
    return false;  // NVML not available
#endif
}

void VLLMResourceManager::shutdownNVML() {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    if (nvml_device_ != nullptr) {
        nvmlShutdown();
        nvml_device_ = nullptr;
        THEMIS_INFO("NVML shutdown");
    }
#endif
}

std::optional<double> VLLMResourceManager::queryGPUUtilization() {
#if defined(THEMIS_ENABLE_CUDA) && defined(__linux__)
    if (nvml_device_ == nullptr) {
        return std::nullopt;
    }
    
    nvmlUtilization_t utilization;
    nvmlDevice_t device = static_cast<nvmlDevice_t>(nvml_device_);
    nvmlReturn_t result = nvmlDeviceGetUtilizationRates(device, &utilization);
    
    if (result != NVML_SUCCESS) {
        THEMIS_WARN("Failed to query GPU utilization: {}", nvmlErrorString(result));
        return std::nullopt;
    }
    
    return static_cast<double>(utilization.gpu);
#else
    return std::nullopt;  // NVML not available
#endif
}

} // namespace acceleration
} // namespace themis
