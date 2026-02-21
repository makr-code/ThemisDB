/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            error_codes.h                                      ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     304                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace acceleration {

/**
 * Structured error codes for acceleration backends
 * 
 * Error codes are organized by category:
 * - 0: Success
 * - 100-199: Initialization errors
 * - 200-299: Resource management errors
 * - 300-399: Runtime/execution errors
 * - 400-499: Configuration errors
 * - 500-599: Kernel/shader errors
 * - 900-999: Unknown/generic errors
 */
enum class AccelerationErrorCode : uint32_t {
    // ========================================================================
    // Success
    // ========================================================================
    Success = 0,
    
    // ========================================================================
    // Initialization Errors (100-199)
    // ========================================================================
    
    /// No GPU devices found on the system
    NoDevicesFound = 101,
    
    /// Required driver not installed or not accessible
    DriverNotInstalled = 102,
    
    /// Device exists but not supported (too old, incompatible)
    DeviceNotSupported = 103,
    
    /// Failed to create GPU context
    ContextCreationFailed = 104,
    
    /// Failed to create command queue/stream
    QueueCreationFailed = 105,
    
    /// Failed to set device as active
    DeviceSetFailed = 106,
    
    /// Device properties query failed
    DevicePropertiesQueryFailed = 107,
    
    /// Runtime version incompatible
    RuntimeVersionIncompatible = 108,
    
    /// Platform not available (OpenCL)
    PlatformNotAvailable = 109,
    
    // ========================================================================
    // Resource Management Errors (200-299)
    // ========================================================================
    
    /// Out of device memory
    OutOfDeviceMemory = 201,
    
    /// Out of host memory
    OutOfHostMemory = 202,
    
    /// Memory allocation failed (unspecified reason)
    AllocationFailed = 203,
    
    /// Memory copy failed (host to device or device to host)
    MemoryCopyFailed = 204,
    
    /// Buffer creation failed
    BufferCreationFailed = 205,
    
    /// Invalid memory access
    InvalidMemoryAccess = 206,
    
    // ========================================================================
    // Runtime/Execution Errors (300-399)
    // ========================================================================
    
    /// Kernel/shader launch failed
    KernelLaunchFailed = 301,
    
    /// Kernel execution error
    KernelExecutionFailed = 302,
    
    /// Command queue synchronization failed
    SynchronizationFailed = 303,
    
    /// Timeout waiting for operation
    OperationTimeout = 304,
    
    /// Device lost or reset
    DeviceLost = 305,
    
    // ========================================================================
    // Kernel/Shader Compilation Errors (500-599)
    // ========================================================================
    
    /// Kernel/shader compilation failed
    KernelCompilationFailed = 501,
    
    /// Kernel/shader not found
    KernelNotFound = 502,
    
    /// Invalid kernel arguments
    InvalidKernelArguments = 503,
    
    /// Shader program linking failed
    ProgramLinkingFailed = 504,
    
    // ========================================================================
    // Configuration Errors (400-499)
    // ========================================================================
    
    /// Invalid backend configuration
    InvalidConfiguration = 401,
    
    /// Requested feature not supported
    FeatureNotSupported = 402,
    
    /// Invalid parameter value
    InvalidParameter = 403,
    
    /// Backend not initialized
    BackendNotInitialized = 404,
    
    /// Backend already initialized
    BackendAlreadyInitialized = 405,
    
    // ========================================================================
    // Generic/Unknown Errors (900-999)
    // ========================================================================
    
    /// Unknown error occurred
    UnknownError = 900,
    
    /// Internal error (bug in backend code)
    InternalError = 901,
    
    /// Operation not implemented
    NotImplemented = 902
};

/**
 * Convert error code to human-readable string
 */
inline const char* errorCodeToString(AccelerationErrorCode code) {
    switch (code) {
        case AccelerationErrorCode::Success:
            return "Success";
            
        // Initialization errors
        case AccelerationErrorCode::NoDevicesFound:
            return "No devices found";
        case AccelerationErrorCode::DriverNotInstalled:
            return "Driver not installed";
        case AccelerationErrorCode::DeviceNotSupported:
            return "Device not supported";
        case AccelerationErrorCode::ContextCreationFailed:
            return "Context creation failed";
        case AccelerationErrorCode::QueueCreationFailed:
            return "Queue creation failed";
        case AccelerationErrorCode::DeviceSetFailed:
            return "Device set failed";
        case AccelerationErrorCode::DevicePropertiesQueryFailed:
            return "Device properties query failed";
        case AccelerationErrorCode::RuntimeVersionIncompatible:
            return "Runtime version incompatible";
        case AccelerationErrorCode::PlatformNotAvailable:
            return "Platform not available";
            
        // Resource errors
        case AccelerationErrorCode::OutOfDeviceMemory:
            return "Out of device memory";
        case AccelerationErrorCode::OutOfHostMemory:
            return "Out of host memory";
        case AccelerationErrorCode::AllocationFailed:
            return "Allocation failed";
        case AccelerationErrorCode::MemoryCopyFailed:
            return "Memory copy failed";
        case AccelerationErrorCode::BufferCreationFailed:
            return "Buffer creation failed";
        case AccelerationErrorCode::InvalidMemoryAccess:
            return "Invalid memory access";
            
        // Runtime errors
        case AccelerationErrorCode::KernelLaunchFailed:
            return "Kernel launch failed";
        case AccelerationErrorCode::KernelExecutionFailed:
            return "Kernel execution failed";
        case AccelerationErrorCode::SynchronizationFailed:
            return "Synchronization failed";
        case AccelerationErrorCode::OperationTimeout:
            return "Operation timeout";
        case AccelerationErrorCode::DeviceLost:
            return "Device lost";
            
        // Kernel compilation errors
        case AccelerationErrorCode::KernelCompilationFailed:
            return "Kernel compilation failed";
        case AccelerationErrorCode::KernelNotFound:
            return "Kernel not found";
        case AccelerationErrorCode::InvalidKernelArguments:
            return "Invalid kernel arguments";
        case AccelerationErrorCode::ProgramLinkingFailed:
            return "Program linking failed";
            
        // Configuration errors
        case AccelerationErrorCode::InvalidConfiguration:
            return "Invalid configuration";
        case AccelerationErrorCode::FeatureNotSupported:
            return "Feature not supported";
        case AccelerationErrorCode::InvalidParameter:
            return "Invalid parameter";
        case AccelerationErrorCode::BackendNotInitialized:
            return "Backend not initialized";
        case AccelerationErrorCode::BackendAlreadyInitialized:
            return "Backend already initialized";
            
        // Generic errors
        case AccelerationErrorCode::UnknownError:
            return "Unknown error";
        case AccelerationErrorCode::InternalError:
            return "Internal error";
        case AccelerationErrorCode::NotImplemented:
            return "Not implemented";
            
        default:
            return "Unknown error code";
    }
}

/**
 * Check if error code represents a success state
 */
inline bool isSuccess(AccelerationErrorCode code) {
    return code == AccelerationErrorCode::Success;
}

/**
 * Check if error code is an initialization error
 */
inline bool isInitializationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 100 && c < 200;
}

/**
 * Check if error code is a resource error
 */
inline bool isResourceError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 200 && c < 300;
}

/**
 * Check if error code is a runtime error
 */
inline bool isRuntimeError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 300 && c < 400;
}

/**
 * Check if error code is a configuration error
 */
inline bool isConfigurationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 400 && c < 500;
}

/**
 * Check if error code is a kernel/shader error
 */
inline bool isKernelError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 500 && c < 600;
}

} // namespace acceleration
} // namespace themis
