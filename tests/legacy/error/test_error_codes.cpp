// Test: Error Code System Tests
// Validates the structured error code system for acceleration backends
// Tests error code categorization, formatting, and helper functions

#include <gtest/gtest.h>
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"

#include <string>
#include <chrono>
#include <algorithm>

using namespace themis::acceleration;

// Test error code string conversion
TEST(ErrorCodes, ToString) {
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::Success), "Success");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::NoDevicesFound), "NoDevicesFound");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::DriverNotInstalled), "DriverNotInstalled");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::OutOfDeviceMemory), "OutOfDeviceMemory");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelLaunchFailed), "KernelLaunchFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::KernelCompilationFailed), "KernelCompilationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidConfiguration), "InvalidConfiguration");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::UnknownError), "UnknownError");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InputValidationFailed), "InputValidationFailed");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidInputShape), "InvalidInputShape");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InvalidInputDtype), "InvalidInputDtype");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::BatchSizeExceeded), "BatchSizeExceeded");
    EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::InputRangeViolation), "InputRangeViolation");
}

// Test initialization error categorization
TEST(ErrorCodes, InitializationErrorCategory) {
    // Should be initialization errors (100-199)
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::DriverNotInstalled));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::DeviceNotSupported));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::ContextCreationFailed));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::QueueCreationFailed));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::DeviceSetFailed));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::DevicePropertiesQueryFailed));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::RuntimeVersionIncompatible));
    EXPECT_TRUE(isInitializationError(AccelerationErrorCode::PlatformNotAvailable));
    
    // Should NOT be initialization errors
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::KernelLaunchFailed));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::InvalidConfiguration));
    EXPECT_FALSE(isInitializationError(AccelerationErrorCode::UnknownError));
}

// Test resource error categorization
TEST(ErrorCodes, ResourceErrorCategory) {
    // Should be resource errors (200-299)
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::OutOfHostMemory));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::AllocationFailed));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::MemoryCopyFailed));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::BufferCreationFailed));
    EXPECT_TRUE(isResourceError(AccelerationErrorCode::InvalidMemoryAccess));
    
    // Should NOT be resource errors
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::KernelLaunchFailed));
    EXPECT_FALSE(isResourceError(AccelerationErrorCode::InvalidConfiguration));
}

// Test runtime error categorization
TEST(ErrorCodes, RuntimeErrorCategory) {
    // Should be runtime errors (300-399)
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::KernelLaunchFailed));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::KernelExecutionFailed));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::SynchronizationFailed));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::OperationTimeout));
    EXPECT_TRUE(isRuntimeError(AccelerationErrorCode::DeviceLost));
    
    // Should NOT be runtime errors
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isRuntimeError(AccelerationErrorCode::InvalidConfiguration));
}

// Test configuration error categorization
TEST(ErrorCodes, ConfigurationErrorCategory) {
    // Should be configuration errors (400-499)
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::InvalidConfiguration));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::FeatureNotSupported));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::InvalidParameter));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::BackendNotInitialized));
    EXPECT_TRUE(isConfigurationError(AccelerationErrorCode::BackendAlreadyInitialized));
    
    // Should NOT be configuration errors
    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isConfigurationError(AccelerationErrorCode::KernelLaunchFailed));
}

// Test kernel error categorization
TEST(ErrorCodes, KernelErrorCategory) {
    // Should be kernel errors (500-599)
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::KernelCompilationFailed));
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::KernelNotFound));
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::InvalidKernelArguments));
    EXPECT_TRUE(isKernelError(AccelerationErrorCode::ProgramLinkingFailed));
    
    // Should NOT be kernel errors
    EXPECT_FALSE(isKernelError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isKernelError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isKernelError(AccelerationErrorCode::OutOfDeviceMemory));
    EXPECT_FALSE(isKernelError(AccelerationErrorCode::KernelLaunchFailed)); // Runtime, not kernel error
}

// Test validation error categorization
TEST(ErrorCodes, ValidationErrorCategory) {
    // Should be validation errors (600-699)
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InputValidationFailed));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InvalidInputShape));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InvalidInputDtype));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::BatchSizeExceeded));
    EXPECT_TRUE(isValidationError(AccelerationErrorCode::InputRangeViolation));
    
    // Should NOT be validation errors
    EXPECT_FALSE(isValidationError(AccelerationErrorCode::Success));
    EXPECT_FALSE(isValidationError(AccelerationErrorCode::NoDevicesFound));
    EXPECT_FALSE(isValidationError(AccelerationErrorCode::KernelLaunchFailed));
    EXPECT_FALSE(isValidationError(AccelerationErrorCode::InvalidConfiguration));
}

// Test ErrorContext basic properties
TEST(ErrorContext, BasicProperties) {
    ErrorContext error(
        AccelerationErrorCode::NoDevicesFound,
        "CUDA",
        "No compatible GPU devices found",
        "Check: 1) GPU installed, 2) Driver installed, 3) GPU supported"
    );
    
    EXPECT_EQ(error.code, AccelerationErrorCode::NoDevicesFound);
    EXPECT_EQ(error.backendName, "CUDA");
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.troubleshootingHint.empty());
    EXPECT_FALSE(error.isSuccess());
}

// Test ErrorContext success state
TEST(ErrorContext, SuccessState) {
    ErrorContext success(
        AccelerationErrorCode::Success,
        "CUDA",
        ""
    );
    
    EXPECT_TRUE(success.isSuccess());
    EXPECT_EQ(success.code, AccelerationErrorCode::Success);
}

// Test ErrorContext formatting
TEST(ErrorContext, Formatting) {
    ErrorContext error(
        AccelerationErrorCode::OutOfDeviceMemory,
        "CUDA",
        "Failed to allocate 2GB on GPU",
        "Reduce batch size or use GPU with more VRAM"
    );
    
    std::string formatted = error.format();
    
    // Should contain backend name
    EXPECT_NE(formatted.find("CUDA"), std::string::npos);
    
    // Should contain error code number
    EXPECT_NE(formatted.find("201"), std::string::npos);
    
    // Should contain message
    EXPECT_NE(formatted.find("2GB"), std::string::npos);
    
    // Should contain hint
    EXPECT_NE(formatted.find("batch size"), std::string::npos);
}

// Test ErrorContext category
TEST(ErrorContext, Category) {
    ErrorContext initError(AccelerationErrorCode::NoDevicesFound, "CUDA", "");
    EXPECT_EQ(initError.getCategory(), "Initialization");
    
    ErrorContext resourceError(AccelerationErrorCode::OutOfDeviceMemory, "CUDA", "");
    EXPECT_EQ(resourceError.getCategory(), "Resource");
    
    ErrorContext runtimeError(AccelerationErrorCode::KernelLaunchFailed, "CUDA", "");
    EXPECT_EQ(runtimeError.getCategory(), "Runtime");
    
    ErrorContext configError(AccelerationErrorCode::InvalidConfiguration, "CUDA", "");
    EXPECT_EQ(configError.getCategory(), "Configuration");
    
    ErrorContext kernelError(AccelerationErrorCode::KernelCompilationFailed, "CUDA", "");
    EXPECT_EQ(kernelError.getCategory(), "Kernel");
    
    ErrorContext validationError(AccelerationErrorCode::InputValidationFailed, "CUDA", "");
    EXPECT_EQ(validationError.getCategory(), "Validation");
    
    ErrorContext success(AccelerationErrorCode::Success, "CUDA", "");
    EXPECT_EQ(success.getCategory(), "Success");
}

// Test ErrorContextHelpers::createNoDevicesError
TEST(ErrorContextHelpers, NoDevicesError) {
    auto error = ErrorContextHelpers::createNoDevicesError("CUDA");
    
    EXPECT_EQ(error.code, AccelerationErrorCode::NoDevicesFound);
    EXPECT_EQ(error.backendName, "CUDA");
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.troubleshootingHint.empty());
    EXPECT_FALSE(error.isSuccess());
    
    // Should mention "devices" in message
    EXPECT_NE(error.message.find("device"), std::string::npos);
}

// Test ErrorContextHelpers::createDriverError
TEST(ErrorContextHelpers, DriverError) {
    auto error = ErrorContextHelpers::createDriverError("HIP");
    
    EXPECT_EQ(error.code, AccelerationErrorCode::DriverNotInstalled);
    EXPECT_EQ(error.backendName, "HIP");
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.troubleshootingHint.empty());
    
    // Should mention driver installation/access
    EXPECT_NE(error.message.find("driver"), std::string::npos);
}

// Test ErrorContextHelpers::createContextError
TEST(ErrorContextHelpers, ContextError) {
    auto error = ErrorContextHelpers::createContextError("OpenCL", "clCreateContext failed");
    
    EXPECT_EQ(error.code, AccelerationErrorCode::ContextCreationFailed);
    EXPECT_EQ(error.backendName, "OpenCL");
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.troubleshootingHint.empty());
}

// Test ErrorContextHelpers::createMemoryError
TEST(ErrorContextHelpers, MemoryError) {
    auto error = ErrorContextHelpers::createMemoryError("CUDA", 2048 * 1024 * 1024ULL);
    
    EXPECT_EQ(error.code, AccelerationErrorCode::OutOfDeviceMemory);
    EXPECT_EQ(error.backendName, "CUDA");
    EXPECT_FALSE(error.message.empty());
    EXPECT_FALSE(error.troubleshootingHint.empty());
    
    // Should mention requested memory size (converted to MB)
    EXPECT_NE(error.message.find("2048"), std::string::npos);
}

// Test ErrorContextHelpers::createKernelCompilationError
TEST(ErrorContextHelpers, KernelCompilationError) {
    std::string buildLog = "error: unknown type name 'foo'";
    auto error = ErrorContextHelpers::createKernelCompilationError(
        "OpenCL", "distance_kernel", buildLog
    );
    
    EXPECT_EQ(error.code, AccelerationErrorCode::KernelCompilationFailed);
    EXPECT_EQ(error.backendName, "OpenCL");
    EXPECT_FALSE(error.message.empty());
    
    // Should include kernel name
    EXPECT_NE(error.message.find("distance_kernel"), std::string::npos);
    
    // Should include build log
    EXPECT_NE(error.message.find("foo"), std::string::npos);
}

// Test ErrorContext with optional system info
TEST(ErrorContext, WithSystemInfo) {
    ErrorContext error(
        AccelerationErrorCode::DeviceNotSupported,
        "CUDA",
        "GPU compute capability too old",
        "Upgrade to GPU with compute capability 6.0+",
        "Device: GTX 960, Compute: 5.2"
    );
    
    EXPECT_TRUE(error.systemInfo.has_value());
    EXPECT_EQ(*error.systemInfo, "Device: GTX 960, Compute: 5.2");
    
    std::string formatted = error.format();
    EXPECT_NE(formatted.find("GTX 960"), std::string::npos);
}

// Test ErrorContext timestamp
TEST(ErrorContext, Timestamp) {
    auto before = std::chrono::system_clock::now();
    
    ErrorContext error(
        AccelerationErrorCode::NoDevicesFound,
        "CUDA",
        "Test message"
    );
    
    auto after = std::chrono::system_clock::now();
    
    // Timestamp should be between before and after
    EXPECT_GE(error.timestamp, before);
    EXPECT_LE(error.timestamp, after);
}

// Test error code uniqueness (all codes should be unique)
TEST(ErrorCodes, Uniqueness) {
    std::vector<uint32_t> codes = {
        static_cast<uint32_t>(AccelerationErrorCode::Success),
        static_cast<uint32_t>(AccelerationErrorCode::NoDevicesFound),
        static_cast<uint32_t>(AccelerationErrorCode::DriverNotInstalled),
        static_cast<uint32_t>(AccelerationErrorCode::DeviceNotSupported),
        static_cast<uint32_t>(AccelerationErrorCode::ContextCreationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::QueueCreationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::DeviceSetFailed),
        static_cast<uint32_t>(AccelerationErrorCode::DevicePropertiesQueryFailed),
        static_cast<uint32_t>(AccelerationErrorCode::RuntimeVersionIncompatible),
        static_cast<uint32_t>(AccelerationErrorCode::PlatformNotAvailable),
        static_cast<uint32_t>(AccelerationErrorCode::OutOfDeviceMemory),
        static_cast<uint32_t>(AccelerationErrorCode::OutOfHostMemory),
        static_cast<uint32_t>(AccelerationErrorCode::AllocationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::MemoryCopyFailed),
        static_cast<uint32_t>(AccelerationErrorCode::BufferCreationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidMemoryAccess),
        static_cast<uint32_t>(AccelerationErrorCode::KernelLaunchFailed),
        static_cast<uint32_t>(AccelerationErrorCode::KernelExecutionFailed),
        static_cast<uint32_t>(AccelerationErrorCode::SynchronizationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::OperationTimeout),
        static_cast<uint32_t>(AccelerationErrorCode::DeviceLost),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidConfiguration),
        static_cast<uint32_t>(AccelerationErrorCode::FeatureNotSupported),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidParameter),
        static_cast<uint32_t>(AccelerationErrorCode::BackendNotInitialized),
        static_cast<uint32_t>(AccelerationErrorCode::BackendAlreadyInitialized),
        static_cast<uint32_t>(AccelerationErrorCode::KernelCompilationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::KernelNotFound),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidKernelArguments),
        static_cast<uint32_t>(AccelerationErrorCode::ProgramLinkingFailed),
        static_cast<uint32_t>(AccelerationErrorCode::InputValidationFailed),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidInputShape),
        static_cast<uint32_t>(AccelerationErrorCode::InvalidInputDtype),
        static_cast<uint32_t>(AccelerationErrorCode::BatchSizeExceeded),
        static_cast<uint32_t>(AccelerationErrorCode::InputRangeViolation),
        static_cast<uint32_t>(AccelerationErrorCode::UnknownError),
        static_cast<uint32_t>(AccelerationErrorCode::InternalError),
        static_cast<uint32_t>(AccelerationErrorCode::NotImplemented),
    };
    
    // Check for duplicates
    std::sort(codes.begin(), codes.end());
    auto it = std::adjacent_find(codes.begin(), codes.end());
    EXPECT_EQ(it, codes.end()) << "Found duplicate error code: " << *it;
}

// Test error code ranges
TEST(ErrorCodes, Ranges) {
    // Initialization errors: 100-199
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::NoDevicesFound), 100);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::NoDevicesFound), 200);
    
    // Resource errors: 200-299
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::OutOfDeviceMemory), 200);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::OutOfDeviceMemory), 300);
    
    // Runtime errors: 300-399
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::KernelLaunchFailed), 300);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::KernelLaunchFailed), 400);
    
    // Configuration errors: 400-499
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::InvalidConfiguration), 400);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::InvalidConfiguration), 500);
    
    // Kernel errors: 500-599
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::KernelCompilationFailed), 500);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::KernelCompilationFailed), 600);
    
    // Validation errors: 600-699
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::InputValidationFailed), 600);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::InputValidationFailed), 700);
    
    // Generic errors: 900-999
    EXPECT_GE(static_cast<uint32_t>(AccelerationErrorCode::UnknownError), 900);
    EXPECT_LT(static_cast<uint32_t>(AccelerationErrorCode::UnknownError), 1000);
}
