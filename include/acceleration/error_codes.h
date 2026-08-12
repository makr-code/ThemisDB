/**
 * @file error_codes.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=1, Unimpl=1, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace acceleration {

/**
 * @brief Structured error codes for acceleration backends.
 * 
 * Defines a canonical set of error codes across all acceleration backends
 * (CUDA, HIP, OpenCL, Vulkan) to enable uniform error handling and reporting.
 * 
 * Error codes are organized by category:
 * - 0: Success
 * - 100-199: Initialization errors (device discovery, driver, context creation)
 * - 200-299: Resource management errors (memory allocation, buffer creation)
 * - 300-399: Runtime/execution errors (kernel launch, synchronization, timeouts)
 * - 400-499: Configuration errors (invalid parameters, feature support)
 * - 500-599: Kernel/shader errors (compilation, linking, argument validation)
 * - 600-699: Validation errors (input shape, dtype, batch size constraints)
 * - 900-999: Unknown/generic errors (internal errors, not implemented)
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
 * @brief Convert an acceleration error code to its symbolic name.
 *
 * Provides a human-readable string representation of an AccelerationErrorCode
 * enum value (e.g., "Success", "NoDevicesFound", "OutOfDeviceMemory").
 * Useful for logging, debugging, and user-facing error messages.
 *
 * @param code The error code to convert.
 * @return A null-terminated C string with the error code's symbolic name.
 *         Returns "UnknownErrorCode" for unrecognized values.
 *
 * @note The returned pointer is valid for the lifetime of the program and
 *       points to a string literal.
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
 * @brief Check if an error code represents a success state.
 *
 * @param code The error code to check.
 * @return true if the code is AccelerationErrorCode::Success, false otherwise.
 */
inline bool isSuccess(AccelerationErrorCode code) {
    return code == AccelerationErrorCode::Success;
}

/**
 * @brief Check if an error code is an initialization error (100-199).
 *
 * Initialization errors occur during device discovery, driver loading,
 * context creation, or device enumeration.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [100, 200), false otherwise.
 */
inline bool isInitializationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 100 && c < 200;
}

/**
 * @brief Check if an error code is a resource error (200-299).
 *
 * Resource errors occur during memory allocation, deallocation, or buffer
 * management operations.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [200, 300), false otherwise.
 */
inline bool isResourceError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 200 && c < 300;
}

/**
 * @brief Check if an error code is a runtime error (300-399).
 *
 * Runtime errors occur during kernel execution, device synchronization,
 * or timeout conditions.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [300, 400), false otherwise.
 */
inline bool isRuntimeError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 300 && c < 400;
}

/**
 * @brief Check if an error code is a configuration error (400-499).
 *
 * Configuration errors occur when parameters are invalid, features are not
 * supported, or backend initialization state is incorrect.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [400, 500), false otherwise.
 */
inline bool isConfigurationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 400 && c < 500;
}

/**
 * @brief Check if an error code is a kernel/shader error (500-599).
 *
 * Kernel errors occur during shader/kernel compilation, linking, or when
 * kernel arguments are invalid or not found.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [500, 600), false otherwise.
 */
inline bool isKernelError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 500 && c < 600;
}

/**
 * @brief Check if an error code is a validation error (600-699).
 *
 * Validation errors occur when input tensor shapes, data types, batch sizes,
 * or numeric ranges are invalid or violate constraints.
 *
 * @param code The error code to check.
 * @return true if the code is in the range [600, 700), false otherwise.
 */
inline bool isValidationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 600 && c < 700;
}

} // namespace acceleration
} // namespace themis
