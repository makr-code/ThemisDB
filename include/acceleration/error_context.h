/**
 * @file error_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/error_codes.h"
#include <string>
#include <optional>
#include <chrono>

namespace themis {
namespace acceleration {

/**
 * Rich error context with diagnostic information
 * 
 * Provides structured error information including:
 * - Error code (categorized)
 * - Backend name
 * - Detailed message
 * - Troubleshooting hint
 * - Optional system information
 * - Timestamp
 */
struct ErrorContext {
    /// Structured error code
    AccelerationErrorCode code;
    
    /// Backend name (e.g., "CUDA", "HIP", "OpenCL")
    std::string backendName;
    
    /// Detailed error message
    std::string message;
    
    /// Actionable troubleshooting hint
    std::string troubleshootingHint;
    
    /// Optional system information (driver version, device name, etc.)
    std::optional<std::string> systemInfo;
    
    /// Timestamp when error occurred (for debugging/logging)
    std::chrono::system_clock::time_point timestamp;
    
    /// Default constructor
    ErrorContext()
        : code(AccelerationErrorCode::UnknownError)
        , timestamp(std::chrono::system_clock::now()) {}
    
    /// Constructor with basic information
    ErrorContext(AccelerationErrorCode errorCode,
                 std::string backend,
                 std::string msg)
        : code(errorCode)
        , backendName(std::move(backend))
        , message(std::move(msg))
        , timestamp(std::chrono::system_clock::now()) {}
    
    /// Constructor with troubleshooting hint
    ErrorContext(AccelerationErrorCode errorCode,
                 std::string backend,
                 std::string msg,
                 std::string hint)
        : code(errorCode)
        , backendName(std::move(backend))
        , message(std::move(msg))
        , troubleshootingHint(std::move(hint))
        , timestamp(std::chrono::system_clock::now()) {}
    
    /// Full constructor
    ErrorContext(AccelerationErrorCode errorCode,
                 std::string backend,
                 std::string msg,
                 std::string hint,
                 std::string sysInfo)
        : code(errorCode)
        , backendName(std::move(backend))
        , message(std::move(msg))
        , troubleshootingHint(std::move(hint))
        , systemInfo(std::move(sysInfo))
        , timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * Format error for display/logging
     */
    std::string format() const {
        std::string result = {};
        result += "[" + backendName + "] ";
        result += errorCodeToString(code);
        result += " (" + std::to_string(static_cast<uint32_t>(code)) + ")";
        
        if (!message.empty()) {
            result += "\n  Message: " + message;
        }
        
        if (!troubleshootingHint.empty()) {
            result += "\n  Hint: " + troubleshootingHint;
        }
        
        if (systemInfo.has_value() && !systemInfo->empty()) {
            result += "\n  System: " + *systemInfo;
        }
        
        return result;
    }
    
    /**
     * Check if this represents a success state
     */
    bool isSuccess() const {
        return code == AccelerationErrorCode::Success;
    }
    
    /**
     * Get error category as string
     */
    std::string getCategory() const {
        if (code == AccelerationErrorCode::Success) {
          return "Success";
        }
        if (isInitializationError(code)) {
          return "Initialization";
        }
        if (isResourceError(code)) {
          return "Resource";
        }
        if (isRuntimeError(code)) {
          return "Runtime";
        }
        if (isConfigurationError(code)) {
          return "Configuration";
        }
        if (isKernelError(code)) {
          return "Kernel";
        }
        if (isValidationError(code)) {
          return "Validation";
        }
        return "Unknown";
    }
};

/**
 * Helper function to create error context with common troubleshooting hints
 */
namespace ErrorContextHelpers {

    /**
     * @brief TBD: Describe createNoDevicesError.
     * @param[in] backendName Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createNoDevicesError(const std::string& backendName) {
        return ErrorContext(
            AccelerationErrorCode::NoDevicesFound,
            backendName,
            "No compatible GPU devices found on this system",
            "Check: 1) GPU is installed, 2) Driver is installed, 3) GPU is supported by " + backendName
        );
    }
    
    /**
     * @brief TBD: Describe createDriverError.
     * @param[in] backendName Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createDriverError(const std::string& backendName) {
        return ErrorContext(
            AccelerationErrorCode::DriverNotInstalled,
            backendName,
            "GPU driver not found or not accessible",
            "Install the latest " + backendName + " driver for your GPU"
        );
    }
    
    /**
     * @brief TBD: Describe createContextError.
     * @param[in] backendName Input parameter.
     * @param[in] details Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createContextError(const std::string& backendName, const std::string& details) {
        return ErrorContext(
            AccelerationErrorCode::ContextCreationFailed,
            backendName,
            "Failed to create GPU context: " + details,
            "Check: 1) GPU is not in use by another process, 2) Sufficient system resources"
        );
    }
    
    /**
     * @brief TBD: Describe createQueueError.
     * @param[in] backendName Input parameter.
     * @param[in] details Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createQueueError(const std::string& backendName, const std::string& details) {
        return ErrorContext(
            AccelerationErrorCode::QueueCreationFailed,
            backendName,
            "Failed to create command queue/stream: " + details,
            "Check: 1) GPU context is valid, 2) Sufficient GPU resources available"
        );
    }
    
    /**
     * @brief TBD: Describe createMemoryError.
     * @param[in] backendName Input parameter.
     * @param[in] requestedBytes Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext(), std::to_string().
     */
    inline ErrorContext createMemoryError(const std::string& backendName, size_t requestedBytes) {
        return ErrorContext(
            AccelerationErrorCode::OutOfDeviceMemory,
            backendName,
            "Out of GPU memory (requested " + std::to_string(requestedBytes / (1024*1024)) + " MB)",
            "Reduce data size or free GPU memory by closing other applications"
        );
    }
    
    /**
     * @brief TBD: Describe createKernelCompilationError.
     * @param[in] backendName Input parameter.
     * @param[in] kernelName Input parameter.
     * @param[in] buildLog Input parameter.
     * @return Return value.
     * @details Calls: empty(), ErrorContext().
     */
    inline ErrorContext createKernelCompilationError(const std::string& backendName, 
                                                      const std::string& kernelName,
                                                      const std::string& buildLog) {
        std::string msg = "Kernel '" + kernelName + "' compilation failed";
        if (!buildLog.empty()) {
            msg += "\nBuild log:\n" + buildLog;
        }
        
        return ErrorContext(
            AccelerationErrorCode::KernelCompilationFailed,
            backendName,
            msg,
            "Check kernel source code for syntax errors or unsupported features"
        );
    }
    
    /**
     * @brief TBD: Describe createKernelLaunchError.
     * @param[in] backendName Input parameter.
     * @param[in] kernelName Input parameter.
     * @param[in] details Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createKernelLaunchError(const std::string& backendName,
                                                 const std::string& kernelName,
                                                 const std::string& details) {
        return ErrorContext(
            AccelerationErrorCode::KernelLaunchFailed,
            backendName,
            "Failed to launch kernel '" + kernelName + "': " + details,
            "Check: 1) Kernel arguments are valid, 2) Work group size is appropriate"
        );
    }

    /**
     * @brief TBD: Describe createValidationError.
     * @param[in] backendName Input parameter.
     * @param[in] code Input parameter.
     * @param[in] details Input parameter.
     * @return Return value.
     * @details Calls: ErrorContext().
     */
    inline ErrorContext createValidationError(const std::string& backendName,
                                              AccelerationErrorCode code,
                                              const std::string& details) {
        return ErrorContext(
            code,
            backendName,
            "Validation failed: " + details,
            "Check input shapes, data types, value ranges and batch sizes before dispatch"
        );
    }

} // namespace ErrorContextHelpers

} // namespace acceleration
} // namespace themis
