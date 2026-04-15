/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            error_codes.h                                      ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
    // Validation Errors (600-699)
    // ========================================================================
    
    /// Generic input validation failure
    InputValidationFailed = 601,
    
    /// Invalid tensor/array shape (wrong rank, zero dimension, etc.)
    InvalidInputShape = 602,
    
    /// Unsupported data type or precision mode
    InvalidInputDtype = 603,
    
    /// Batch size exceeds backend or device limit
    BatchSizeExceeded = 604,
    
    /// Input values out of valid numeric range (NaN, Inf, overflow)
    InputRangeViolation = 605,

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
 * Convert error code to its symbolic name (enum identifier string)
 */
inline const char* errorCodeToString(AccelerationErrorCode code) {
    switch (code) {
        case AccelerationErrorCode::Success:
            return "Success";
            
        // Initialization errors
        case AccelerationErrorCode::NoDevicesFound:
            return "NoDevicesFound";
        case AccelerationErrorCode::DriverNotInstalled:
            return "DriverNotInstalled";
        case AccelerationErrorCode::DeviceNotSupported:
            return "DeviceNotSupported";
        case AccelerationErrorCode::ContextCreationFailed:
            return "ContextCreationFailed";
        case AccelerationErrorCode::QueueCreationFailed:
            return "QueueCreationFailed";
        case AccelerationErrorCode::DeviceSetFailed:
            return "DeviceSetFailed";
        case AccelerationErrorCode::DevicePropertiesQueryFailed:
            return "DevicePropertiesQueryFailed";
        case AccelerationErrorCode::RuntimeVersionIncompatible:
            return "RuntimeVersionIncompatible";
        case AccelerationErrorCode::PlatformNotAvailable:
            return "PlatformNotAvailable";
            
        // Resource errors
        case AccelerationErrorCode::OutOfDeviceMemory:
            return "OutOfDeviceMemory";
        case AccelerationErrorCode::OutOfHostMemory:
            return "OutOfHostMemory";
        case AccelerationErrorCode::AllocationFailed:
            return "AllocationFailed";
        case AccelerationErrorCode::MemoryCopyFailed:
            return "MemoryCopyFailed";
        case AccelerationErrorCode::BufferCreationFailed:
            return "BufferCreationFailed";
        case AccelerationErrorCode::InvalidMemoryAccess:
            return "InvalidMemoryAccess";
            
        // Runtime errors
        case AccelerationErrorCode::KernelLaunchFailed:
            return "KernelLaunchFailed";
        case AccelerationErrorCode::KernelExecutionFailed:
            return "KernelExecutionFailed";
        case AccelerationErrorCode::SynchronizationFailed:
            return "SynchronizationFailed";
        case AccelerationErrorCode::OperationTimeout:
            return "OperationTimeout";
        case AccelerationErrorCode::DeviceLost:
            return "DeviceLost";
            
        // Kernel compilation errors
        case AccelerationErrorCode::KernelCompilationFailed:
            return "KernelCompilationFailed";
        case AccelerationErrorCode::KernelNotFound:
            return "KernelNotFound";
        case AccelerationErrorCode::InvalidKernelArguments:
            return "InvalidKernelArguments";
        case AccelerationErrorCode::ProgramLinkingFailed:
            return "ProgramLinkingFailed";
            
        // Validation errors
        case AccelerationErrorCode::InputValidationFailed:
            return "InputValidationFailed";
        case AccelerationErrorCode::InvalidInputShape:
            return "InvalidInputShape";
        case AccelerationErrorCode::InvalidInputDtype:
            return "InvalidInputDtype";
        case AccelerationErrorCode::BatchSizeExceeded:
            return "BatchSizeExceeded";
        case AccelerationErrorCode::InputRangeViolation:
            return "InputRangeViolation";
            
        // Configuration errors
        case AccelerationErrorCode::InvalidConfiguration:
            return "InvalidConfiguration";
        case AccelerationErrorCode::FeatureNotSupported:
            return "FeatureNotSupported";
        case AccelerationErrorCode::InvalidParameter:
            return "InvalidParameter";
        case AccelerationErrorCode::BackendNotInitialized:
            return "BackendNotInitialized";
        case AccelerationErrorCode::BackendAlreadyInitialized:
            return "BackendAlreadyInitialized";
            
        // Generic errors
        case AccelerationErrorCode::UnknownError:
            return "UnknownError";
        case AccelerationErrorCode::InternalError:
            return "InternalError";
        case AccelerationErrorCode::NotImplemented:
            return "NotImplemented";
            
        default:
            return "UnknownErrorCode";
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

/**
 * Check if error code is a validation error
 */
inline bool isValidationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 600 && c < 700;
}

} // namespace acceleration
} // namespace themis
