/**
 * @file error_codes.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <cstdint>

namespace themis {
namespace acceleration {

enum class AccelerationErrorCode : uint32_t {
    // ========================================================================
    // Success
    // ========================================================================
    Success = 0,
    
    // ========================================================================
    // Initialization Errors (100-199)
    // ========================================================================
    
    NoDevicesFound = 101,
    
    DriverNotInstalled = 102,
    
    DeviceNotSupported = 103,
    
    ContextCreationFailed = 104,
    
    QueueCreationFailed = 105,
    
    DeviceSetFailed = 106,
    
    DevicePropertiesQueryFailed = 107,
    
    RuntimeVersionIncompatible = 108,
    
    PlatformNotAvailable = 109,
    
    // ========================================================================
    // Resource Management Errors (200-299)
    // ========================================================================
    
    OutOfDeviceMemory = 201,
    
    OutOfHostMemory = 202,
    
    AllocationFailed = 203,
    
    MemoryCopyFailed = 204,
    
    BufferCreationFailed = 205,
    
    InvalidMemoryAccess = 206,
    
    // ========================================================================
    // Runtime/Execution Errors (300-399)
    // ========================================================================
    
    KernelLaunchFailed = 301,
    
    KernelExecutionFailed = 302,
    
    SynchronizationFailed = 303,
    
    OperationTimeout = 304,
    
    DeviceLost = 305,
    
    // ========================================================================
    // Kernel/Shader Compilation Errors (500-599)
    // ========================================================================
    
    KernelCompilationFailed = 501,
    
    KernelNotFound = 502,
    
    InvalidKernelArguments = 503,
    
    ProgramLinkingFailed = 504,
    
    // ========================================================================
    // Validation Errors (600-699)
    // ========================================================================
    
    InputValidationFailed = 601,
    
    InvalidInputShape = 602,
    
    InvalidInputDtype = 603,
    
    BatchSizeExceeded = 604,
    
    InputRangeViolation = 605,

    // ========================================================================
    // Configuration Errors (400-499)
    // ========================================================================
    
    InvalidConfiguration = 401,
    
    FeatureNotSupported = 402,
    
    InvalidParameter = 403,
    
    BackendNotInitialized = 404,
    
    BackendAlreadyInitialized = 405,
    
    // ========================================================================
    // Generic/Unknown Errors (900-999)
    // ========================================================================
    
    UnknownError = 900,
    
    InternalError = 901,
    
    NotImplemented = 902
};

/**
 * @brief Error Code To String.
 * @param[in] code Input parameter.
 * @return Pointer to the result.
 * @details Implements errorCodeToString without additional internal calls.
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
 * @brief Is Success.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isSuccess without additional internal calls.
 */
inline bool isSuccess(AccelerationErrorCode code) {
    return code == AccelerationErrorCode::Success;
}

/**
 * @brief Is Initialization Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isInitializationError without additional internal calls.
 */
inline bool isInitializationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 100 && c < 200;
}

/**
 * @brief Is Resource Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isResourceError without additional internal calls.
 */
inline bool isResourceError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 200 && c < 300;
}

/**
 * @brief Is Runtime Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isRuntimeError without additional internal calls.
 */
inline bool isRuntimeError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 300 && c < 400;
}

/**
 * @brief Is Configuration Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isConfigurationError without additional internal calls.
 */
inline bool isConfigurationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 400 && c < 500;
}

/**
 * @brief Is Kernel Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isKernelError without additional internal calls.
 */
inline bool isKernelError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 500 && c < 600;
}

/**
 * @brief Is Validation Error.
 * @param[in] code Input parameter.
 * @return True when the operation succeeds.
 * @details Implements isValidationError without additional internal calls.
 */
inline bool isValidationError(AccelerationErrorCode code) {
    uint32_t c = static_cast<uint32_t>(code);
    return c >= 600 && c < 700;
}

} // namespace acceleration
} // namespace themis
