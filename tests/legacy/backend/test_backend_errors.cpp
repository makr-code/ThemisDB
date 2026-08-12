// Test: Backend Error Integration Tests
// Validates that backends correctly use structured error codes
// Tests getLastError() and error context integration

#include <gtest/gtest.h>
#include "acceleration/compute_backend.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include "acceleration/cpu_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif

#ifdef THEMIS_ENABLE_OPENCL
#include "acceleration/opencl_backend.h"
#endif

#ifdef THEMIS_ENABLE_METAL
#include "acceleration/metal_backend.h"
#endif

#include <memory>
#include <iostream>

using namespace themis::acceleration;

// Test that CPU backend initializes successfully
TEST(CPUBackend, InitializationSuccess) {
    CPUVectorBackend backend;
    bool result = backend.initialize();
    
    // CPU should always succeed
    EXPECT_TRUE(result);
    
    // On success, last error should be Success
    auto error = backend.getLastError();
    EXPECT_TRUE(error.isSuccess());
    EXPECT_EQ(error.code, AccelerationErrorCode::Success);
}

#ifdef THEMIS_ENABLE_CUDA
// Test CUDA backend error handling
TEST(CUDABackend, ErrorContext) {
    CUDAVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        // If initialization failed, should have error context
        auto error = backend.getLastError();
        
        // Error should not be Success
        EXPECT_FALSE(error.isSuccess());
        EXPECT_NE(error.code, AccelerationErrorCode::Success);
        
        // Should have backend name
        EXPECT_EQ(error.backendName, "CUDA");
        
        // Should have error message
        EXPECT_FALSE(error.message.empty());
        
        // Should have troubleshooting hint
        EXPECT_FALSE(error.troubleshootingHint.empty());
        
        // Error should be categorized (likely initialization error)
        if (isInitializationError(error.code)) {
            std::cout << "CUDA initialization error: " << error.format() << std::endl;
        }
        
        // Test formatted output
        std::string formatted = error.format();
        EXPECT_FALSE(formatted.empty());
        EXPECT_NE(formatted.find("CUDA"), std::string::npos);
        
    } else {
        // If initialization succeeded, error should be Success
        auto error = backend.getLastError();
        EXPECT_TRUE(error.isSuccess());
        EXPECT_EQ(error.code, AccelerationErrorCode::Success);
        
        std::cout << "CUDA backend initialized successfully" << std::endl;
    }
}

// Test CUDA backend provides meaningful errors
TEST(CUDABackend, MeaningfulErrors) {
    CUDAVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        auto error = backend.getLastError();
        
        // Common error scenarios
        if (error.code == AccelerationErrorCode::NoDevicesFound) {
            EXPECT_NE(error.message.find("device"), std::string::npos);
            EXPECT_FALSE(error.troubleshootingHint.empty());
        }
        else if (error.code == AccelerationErrorCode::DriverNotInstalled) {
            EXPECT_FALSE(error.troubleshootingHint.empty());
        }
        else if (error.code == AccelerationErrorCode::FeatureNotSupported) {
            // CUDA not compiled in
            EXPECT_NE(error.message.find("compiled"), std::string::npos);
        }
    }
}
#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_HIP
// Test HIP backend error handling
TEST(HIPBackend, ErrorContext) {
    HIPVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        // If initialization failed, should have error context
        auto error = backend.getLastError();
        
        // Error should not be Success
        EXPECT_FALSE(error.isSuccess());
        EXPECT_NE(error.code, AccelerationErrorCode::Success);
        
        // Should have backend name
        EXPECT_EQ(error.backendName, "HIP");
        
        // Should have error message
        EXPECT_FALSE(error.message.empty());
        
        // Should have troubleshooting hint
        EXPECT_FALSE(error.troubleshootingHint.empty());
        
        std::cout << "HIP initialization error: " << error.format() << std::endl;
        
    } else {
        // If initialization succeeded, error should be Success
        auto error = backend.getLastError();
        EXPECT_TRUE(error.isSuccess());
        EXPECT_EQ(error.code, AccelerationErrorCode::Success);
        
        std::cout << "HIP backend initialized successfully" << std::endl;
    }
}

// Test HIP backend handles double initialization
TEST(HIPBackend, DoubleInitialization) {
    HIPVectorBackend backend;
    bool first = backend.initialize();
    
    if (first) {
        // Try to initialize again
        bool second = backend.initialize();
        
        // Should handle gracefully (may succeed or return error)
        auto error = backend.getLastError();
        
        if (!second) {
            // If it fails, should have appropriate error
            EXPECT_EQ(error.code, AccelerationErrorCode::BackendAlreadyInitialized);
        }
    }
}
#endif // THEMIS_ENABLE_HIP

#ifdef THEMIS_ENABLE_OPENCL
// Test OpenCL backend error handling
TEST(OpenCLBackend, ErrorContext) {
    OpenCLVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        // If initialization failed, should have error context
        auto error = backend.getLastError();
        
        // Error should not be Success
        EXPECT_FALSE(error.isSuccess());
        EXPECT_NE(error.code, AccelerationErrorCode::Success);
        
        // Should have backend name
        EXPECT_EQ(error.backendName, "OpenCL");
        
        // Should have error message
        EXPECT_FALSE(error.message.empty());
        
        // Should have troubleshooting hint
        EXPECT_FALSE(error.troubleshootingHint.empty());
        
        std::cout << "OpenCL initialization error: " << error.format() << std::endl;
        
        // OpenCL-specific errors
        if (error.code == AccelerationErrorCode::PlatformNotAvailable) {
            EXPECT_NE(error.message.find("platform"), std::string::npos);
        }
        else if (error.code == AccelerationErrorCode::KernelCompilationFailed) {
            // Should include build log
            EXPECT_FALSE(error.message.empty());
        }
        
    } else {
        // If initialization succeeded, error should be Success
        auto error = backend.getLastError();
        EXPECT_TRUE(error.isSuccess());
        EXPECT_EQ(error.code, AccelerationErrorCode::Success);
        
        std::cout << "OpenCL backend initialized successfully" << std::endl;
    }
}

// Test OpenCL kernel compilation errors
TEST(OpenCLBackend, KernelCompilationError) {
    OpenCLVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        auto error = backend.getLastError();
        
        // Check if it's a kernel compilation error
        if (error.code == AccelerationErrorCode::KernelCompilationFailed) {
            // Should have build log in message
            EXPECT_FALSE(error.message.empty());
            
            // Should mention the kernel or compilation
            EXPECT_TRUE(
                error.message.find("kernel") != std::string::npos ||
                error.message.find("compil") != std::string::npos ||
                error.message.find("build") != std::string::npos
            );
        }
    }
}
#endif // THEMIS_ENABLE_OPENCL

#ifdef THEMIS_ENABLE_METAL
// Test Metal backend error handling
TEST(MetalBackend, ErrorContext) {
    MetalVectorBackend backend;
    bool result = backend.initialize();
    
    if (!result) {
        // If initialization failed, should have error context
        auto error = backend.getLastError();
        
        // Error should not be Success
        EXPECT_FALSE(error.isSuccess());
        EXPECT_NE(error.code, AccelerationErrorCode::Success);
        
        // Should have backend name
        EXPECT_EQ(error.backendName, "Metal");
        
        // Should have error message
        EXPECT_FALSE(error.message.empty());
        
        // Should have troubleshooting hint
        EXPECT_FALSE(error.troubleshootingHint.empty());
        
        std::cout << "Metal initialization error: " << error.format() << std::endl;
        
    } else {
        // If initialization succeeded, error should be Success
        auto error = backend.getLastError();
        EXPECT_TRUE(error.isSuccess());
        EXPECT_EQ(error.code, AccelerationErrorCode::Success);
        
        std::cout << "Metal backend initialized successfully" << std::endl;
    }
}
#endif // THEMIS_ENABLE_METAL

// Test error context comparison
TEST(ErrorContext, Comparison) {
    ErrorContext error1(AccelerationErrorCode::NoDevicesFound, "CUDA", "Message 1");
    ErrorContext error2(AccelerationErrorCode::NoDevicesFound, "CUDA", "Message 1");
    ErrorContext error3(AccelerationErrorCode::DriverNotInstalled, "CUDA", "Message 2");
    
    // Same error codes should have same code
    EXPECT_EQ(error1.code, error2.code);
    
    // Different error codes should differ
    EXPECT_NE(error1.code, error3.code);
}

// Test that all integrated backends use error codes
TEST(AllBackends, ErrorCodeIntegration) {
    std::vector<std::string> backends_tested;
    std::vector<std::string> backends_succeeded;
    std::vector<std::string> backends_failed;
    
    // Test CPU
    {
        CPUVectorBackend backend;
        bool result = backend.initialize();
        backends_tested.push_back("CPU");
        if (result) {
            backends_succeeded.push_back("CPU");
            auto error = backend.getLastError();
            EXPECT_TRUE(error.isSuccess());
        } else {
            backends_failed.push_back("CPU");
        }
    }
    
#ifdef THEMIS_ENABLE_CUDA
    {
        CUDAVectorBackend backend;
        bool result = backend.initialize();
        backends_tested.push_back("CUDA");
        if (result) {
            backends_succeeded.push_back("CUDA");
        } else {
            backends_failed.push_back("CUDA");
            auto error = backend.getLastError();
            EXPECT_FALSE(error.isSuccess());
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_HIP
    {
        HIPVectorBackend backend;
        bool result = backend.initialize();
        backends_tested.push_back("HIP");
        if (result) {
            backends_succeeded.push_back("HIP");
        } else {
            backends_failed.push_back("HIP");
            auto error = backend.getLastError();
            EXPECT_FALSE(error.isSuccess());
        }
    }
#endif
    
#ifdef THEMIS_ENABLE_OPENCL
    {
        OpenCLVectorBackend backend;
        bool result = backend.initialize();
        backends_tested.push_back("OpenCL");
        if (result) {
            backends_succeeded.push_back("OpenCL");
        } else {
            backends_failed.push_back("OpenCL");
            auto error = backend.getLastError();
            EXPECT_FALSE(error.isSuccess());
        }
    }
#endif
    
    // Report summary
    std::cout << "\n=== Backend Error Integration Summary ===" << std::endl;
    std::cout << "Backends tested: " << backends_tested.size() << std::endl;
    std::cout << "Succeeded: " << backends_succeeded.size() << " - ";
    for (const auto& name : backends_succeeded) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Failed: " << backends_failed.size() << " - ";
    for (const auto& name : backends_failed) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // At least one backend should be tested
    EXPECT_GE(backends_tested.size(), 1);
}
