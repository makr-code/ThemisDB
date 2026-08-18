/**
 * @file gpu_memory_error_codes.h
 * @brief GPU Memory Manager error codes [7300-7399]
 * @version 1.0.0
 * @date 2026-08-17
 * 
 * Batch 5 Phase 1B: CRITICAL Gap Fixes for GPU Memory Manager
 * 
 * Error codes are in the [7300-7399] range as per architecture guidelines.
 * Each code provides specific context for allocation, cleanup, and device failures.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief GPU Memory Manager error codes.
 * 
 * Error codes in the [7300-7399] range.
 * Organized by severity and failure category:
 * - [7300-7309]: Critical allocation failures
 * - [7310-7319]: Device/GPU management failures  
 * - [7320-7329]: Memory cleanup and RAII failures
 * - [7330-7339]: Device fallback and recovery failures
 * - [7340-7349]: Temperature/health monitoring failures
 * - [7350-7399]: Reserved for future use
 */
enum class GPUMemoryErrorCode : int32_t {
    // Allocation failures [7300-7309]
    /// GPU memory allocation failed (out of VRAM)
    GPU_ALLOCATION_OOM = 7300,
    
    /// GPU allocation size overflow detected
    GPU_ALLOCATION_OVERFLOW = 7301,
    
    /// GPU allocation pre-check failed (device validation)
    GPU_ALLOCATION_PRECHECK_FAILED = 7302,
    
    /// GPU allocation failed and CPU fallback not available
    GPU_ALLOCATION_NO_FALLBACK = 7303,
    
    /// CPU memory allocation failed (pinned)
    CPU_PINNED_ALLOCATION_FAILED = 7304,
    
    /// CPU memory allocation failed (regular)
    CPU_ALLOCATION_FAILED = 7305,
    
    /// Memory allocation exceeds hard limit
    ALLOCATION_EXCEEDS_LIMIT = 7306,
    
    /// Memory fragmentation critical
    FRAGMENTATION_CRITICAL = 7307,
    
    // Device/GPU failures [7310-7319]
    /// GPU device not available
    GPU_DEVICE_UNAVAILABLE = 7310,
    
    /// GPU device set failed
    GPU_DEVICE_SET_FAILED = 7311,
    
    /// GPU device query failed
    GPU_DEVICE_QUERY_FAILED = 7312,
    
    /// GPU device reset during operation
    GPU_DEVICE_RESET = 7313,
    
    /// GPU device health check failed
    GPU_DEVICE_UNHEALTHY = 7314,
    
    /// Invalid GPU device ID
    INVALID_GPU_DEVICE_ID = 7315,
    
    /// Multi-GPU configuration invalid
    MULTIGPU_CONFIG_INVALID = 7316,
    
    // Cleanup and RAII [7320-7329]
    /// CUDA free() failed during cleanup
    CUDA_FREE_FAILED = 7320,
    
    /// CUDA pinned free failed
    CUDA_PINNED_FREE_FAILED = 7321,
    
    /// Secure clear failed during cleanup
    SECURE_CLEAR_FAILED = 7322,
    
    /// Cleanup error but memory was freed
    CLEANUP_ERROR_PARTIAL = 7323,
    
    /// Memory holder double-free detected
    DOUBLE_FREE_DETECTED = 7324,
    
    /// Cleanup timeout (cleanup took too long)
    CLEANUP_TIMEOUT = 7325,
    
    // Fallback and recovery [7330-7339]
    /// GPU allocation failed, falling back to pinned CPU
    FALLBACK_TO_PINNED_CPU = 7330,
    
    /// Pinned CPU allocation failed, falling back to regular CPU
    FALLBACK_TO_CPU = 7331,
    
    /// CPU fallback also failed
    FALLBACK_EXHAUSTED = 7332,
    
    /// Device fallback triggered
    DEVICE_FALLBACK_TRIGGERED = 7333,
    
    /// No healthy GPU device available
    NO_HEALTHY_GPU_AVAILABLE = 7334,
    
    // Temperature/health monitoring [7340-7349]
    /// Temperature query failed
    TEMPERATURE_QUERY_FAILED = 7340,
    
    /// GPU thermal throttling detected
    THERMAL_THROTTLING = 7341,
    
    /// GPU temperature critical
    TEMPERATURE_CRITICAL = 7342,
    
    /// GPU health monitoring disabled
    HEALTH_MONITORING_DISABLED = 7343,
    
    /// GPU peer access failed
    PEER_ACCESS_FAILED = 7344,
};

/**
 * @brief Convert error code to human-readable string
 * @param code The error code
 * @return String description of the error
 */
inline std::string gpuMemoryErrorToString(GPUMemoryErrorCode code) {
    switch (code) {
        case GPUMemoryErrorCode::GPU_ALLOCATION_OOM:
            return "GPU_ALLOCATION_OOM: GPU ran out of device memory";
        case GPUMemoryErrorCode::GPU_ALLOCATION_OVERFLOW:
            return "GPU_ALLOCATION_OVERFLOW: Allocation size overflow detected";
        case GPUMemoryErrorCode::GPU_ALLOCATION_PRECHECK_FAILED:
            return "GPU_ALLOCATION_PRECHECK_FAILED: Device validation failed before allocation";
        case GPUMemoryErrorCode::GPU_ALLOCATION_NO_FALLBACK:
            return "GPU_ALLOCATION_NO_FALLBACK: GPU allocation failed and CPU fallback not available";
        case GPUMemoryErrorCode::CPU_PINNED_ALLOCATION_FAILED:
            return "CPU_PINNED_ALLOCATION_FAILED: Pinned host memory allocation failed";
        case GPUMemoryErrorCode::CPU_ALLOCATION_FAILED:
            return "CPU_ALLOCATION_FAILED: Regular host memory allocation failed";
        case GPUMemoryErrorCode::ALLOCATION_EXCEEDS_LIMIT:
            return "ALLOCATION_EXCEEDS_LIMIT: Allocation exceeds configured hard limit";
        case GPUMemoryErrorCode::FRAGMENTATION_CRITICAL:
            return "FRAGMENTATION_CRITICAL: Memory fragmentation at critical levels";
        case GPUMemoryErrorCode::GPU_DEVICE_UNAVAILABLE:
            return "GPU_DEVICE_UNAVAILABLE: GPU device not accessible";
        case GPUMemoryErrorCode::GPU_DEVICE_SET_FAILED:
            return "GPU_DEVICE_SET_FAILED: Failed to set active GPU device";
        case GPUMemoryErrorCode::GPU_DEVICE_QUERY_FAILED:
            return "GPU_DEVICE_QUERY_FAILED: GPU device property query failed";
        case GPUMemoryErrorCode::GPU_DEVICE_RESET:
            return "GPU_DEVICE_RESET: GPU device was reset during operation";
        case GPUMemoryErrorCode::GPU_DEVICE_UNHEALTHY:
            return "GPU_DEVICE_UNHEALTHY: GPU device health check failed";
        case GPUMemoryErrorCode::INVALID_GPU_DEVICE_ID:
            return "INVALID_GPU_DEVICE_ID: GPU device ID is invalid";
        case GPUMemoryErrorCode::MULTIGPU_CONFIG_INVALID:
            return "MULTIGPU_CONFIG_INVALID: Multi-GPU configuration is invalid";
        case GPUMemoryErrorCode::CUDA_FREE_FAILED:
            return "CUDA_FREE_FAILED: cudaFree() failed during cleanup";
        case GPUMemoryErrorCode::CUDA_PINNED_FREE_FAILED:
            return "CUDA_PINNED_FREE_FAILED: cudaFreeHost() failed during cleanup";
        case GPUMemoryErrorCode::SECURE_CLEAR_FAILED:
            return "SECURE_CLEAR_FAILED: Secure memory clear failed";
        case GPUMemoryErrorCode::CLEANUP_ERROR_PARTIAL:
            return "CLEANUP_ERROR_PARTIAL: Cleanup encountered error but memory was freed";
        case GPUMemoryErrorCode::DOUBLE_FREE_DETECTED:
            return "DOUBLE_FREE_DETECTED: Attempted to free already freed memory";
        case GPUMemoryErrorCode::CLEANUP_TIMEOUT:
            return "CLEANUP_TIMEOUT: Memory cleanup exceeded timeout limit";
        case GPUMemoryErrorCode::FALLBACK_TO_PINNED_CPU:
            return "FALLBACK_TO_PINNED_CPU: GPU allocation failed, using pinned CPU memory";
        case GPUMemoryErrorCode::FALLBACK_TO_CPU:
            return "FALLBACK_TO_CPU: GPU allocation failed, using regular CPU memory";
        case GPUMemoryErrorCode::FALLBACK_EXHAUSTED:
            return "FALLBACK_EXHAUSTED: All fallback strategies exhausted";
        case GPUMemoryErrorCode::DEVICE_FALLBACK_TRIGGERED:
            return "DEVICE_FALLBACK_TRIGGERED: GPU unavailable, using CPU fallback";
        case GPUMemoryErrorCode::NO_HEALTHY_GPU_AVAILABLE:
            return "NO_HEALTHY_GPU_AVAILABLE: No healthy GPU device available";
        case GPUMemoryErrorCode::TEMPERATURE_QUERY_FAILED:
            return "TEMPERATURE_QUERY_FAILED: GPU temperature query failed";
        case GPUMemoryErrorCode::THERMAL_THROTTLING:
            return "THERMAL_THROTTLING: GPU thermal throttling detected";
        case GPUMemoryErrorCode::TEMPERATURE_CRITICAL:
            return "TEMPERATURE_CRITICAL: GPU temperature at critical level";
        case GPUMemoryErrorCode::HEALTH_MONITORING_DISABLED:
            return "HEALTH_MONITORING_DISABLED: GPU health monitoring is disabled";
        case GPUMemoryErrorCode::PEER_ACCESS_FAILED:
            return "PEER_ACCESS_FAILED: GPU peer access setup failed";
        default:
            return "UNKNOWN_ERROR: Unknown GPU memory error code";
    }
}

} // namespace llm
} // namespace themis
