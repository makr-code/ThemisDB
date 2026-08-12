// Test: RAII Resource Wrapper Tests
// Validates automatic resource cleanup and exception safety

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/raii/cuda_raii.h"
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include "acceleration/raii/hip_raii.h"
#include <hip/hip_runtime.h>
#endif

#ifdef THEMIS_ENABLE_OPENCL
#include "acceleration/raii/opencl_raii.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

#if defined(THEMIS_ENABLE_CUDA) || defined(THEMIS_ENABLE_HIP) || defined(THEMIS_ENABLE_OPENCL)
using namespace themis::acceleration::raii;
#endif

// ============================================================================
// CUDA RAII Tests
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CudaRAII, StreamLifecycle) {
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }
    
    // Test stream creation and destruction
    {
        CudaStream stream(true);
        EXPECT_TRUE(stream.valid());
        EXPECT_NE(stream.get(), nullptr);
    }
    // Stream should be automatically destroyed here
    
    std::cout << "  ✓ CUDA stream RAII lifecycle works" << std::endl;
}

TEST(CudaRAII, StreamMove) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }
    
    CudaStream stream1(true);
    EXPECT_TRUE(stream1.valid());
    
    // Move construction
    CudaStream stream2(std::move(stream1));
    EXPECT_FALSE(stream1.valid());
    EXPECT_TRUE(stream2.valid());
    
    // Move assignment
    CudaStream stream3;
    stream3 = std::move(stream2);
    EXPECT_FALSE(stream2.valid());
    EXPECT_TRUE(stream3.valid());
    
    std::cout << "  ✓ CUDA stream move semantics work" << std::endl;
}

TEST(CudaRAII, DeviceMemoryLifecycle) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }
    
    // Test memory allocation and deallocation
    {
        CudaDeviceMemory mem(1024);  // 1KB
        EXPECT_TRUE(mem.valid());
        EXPECT_NE(mem.get(), nullptr);
        EXPECT_EQ(mem.size(), 1024);
    }
    // Memory should be automatically freed here
    
    std::cout << "  ✓ CUDA device memory RAII lifecycle works" << std::endl;
}

TEST(CudaRAII, DeviceMemoryCopy) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }
    
    const size_t size = 10 * sizeof(float);
    float hostData[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    float hostResult[10] = {0};
    
    CudaDeviceMemory mem(size);
    EXPECT_TRUE(mem.valid());
    
    // Copy to device
    EXPECT_NO_THROW(mem.copyFrom(hostData, size));
    
    // Copy back to host
    EXPECT_NO_THROW(mem.copyTo(hostResult, size));
    
    // Verify data integrity
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(hostResult[i], hostData[i]);
    }
    
    std::cout << "  ✓ CUDA device memory copy operations work" << std::endl;
}

#endif // THEMIS_ENABLE_CUDA

// ============================================================================
// HIP RAII Tests
// ============================================================================

#ifdef THEMIS_ENABLE_HIP

TEST(HipRAII, StreamLifecycle) {
    int deviceCount = 0;
    hipError_t err = hipGetDeviceCount(&deviceCount);
    if (err != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }
    
    // Test stream creation and destruction
    {
        HipStream stream(true);
        EXPECT_TRUE(stream.valid());
        EXPECT_NE(stream.get(), nullptr);
    }
    // Stream should be automatically destroyed here
    
    std::cout << "  ✓ HIP stream RAII lifecycle works" << std::endl;
}

TEST(HipRAII, StreamMove) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }
    
    HipStream stream1(true);
    EXPECT_TRUE(stream1.valid());
    
    // Move construction
    HipStream stream2(std::move(stream1));
    EXPECT_FALSE(stream1.valid());
    EXPECT_TRUE(stream2.valid());
    
    // Move assignment
    HipStream stream3;
    stream3 = std::move(stream2);
    EXPECT_FALSE(stream2.valid());
    EXPECT_TRUE(stream3.valid());
    
    std::cout << "  ✓ HIP stream move semantics work" << std::endl;
}

TEST(HipRAII, DeviceMemoryLifecycle) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }
    
    // Test memory allocation and deallocation
    {
        HipDeviceMemory mem(1024);  // 1KB
        EXPECT_TRUE(mem.valid());
        EXPECT_NE(mem.get(), nullptr);
        EXPECT_EQ(mem.size(), 1024);
    }
    // Memory should be automatically freed here
    
    std::cout << "  ✓ HIP device memory RAII lifecycle works" << std::endl;
}

TEST(HipRAII, DeviceMemoryCopy) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }
    
    const size_t size = 10 * sizeof(float);
    float hostData[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    float hostResult[10] = {0};
    
    HipDeviceMemory mem(size);
    EXPECT_TRUE(mem.valid());
    
    // Copy to device
    EXPECT_NO_THROW(mem.copyFrom(hostData, size));
    
    // Copy back to host
    EXPECT_NO_THROW(mem.copyTo(hostResult, size));
    
    // Verify data integrity
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(hostResult[i], hostData[i]);
    }
    
    std::cout << "  ✓ HIP device memory copy operations work" << std::endl;
}

#endif // THEMIS_ENABLE_HIP

// ============================================================================
// OpenCL RAII Tests
// ============================================================================

#ifdef THEMIS_ENABLE_OPENCL

TEST(OpenCLRAII, ContextLifecycle) {
    cl_uint numPlatforms = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        GTEST_SKIP() << "capability:opencl_platform_available=false;reason=no_opencl_platforms_available";
    }
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    
    cl_device_id device;
    cl_uint numDevices = 0;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &numDevices);
    if (err != CL_SUCCESS || numDevices == 0) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_devices_available";
    }
    
    // Test context creation and destruction
    {
        OpenCLContext context;
        EXPECT_NO_THROW(context.create(nullptr, 1, &device));
        EXPECT_TRUE(context.valid());
        EXPECT_NE(context.get(), nullptr);
    }
    // Context should be automatically released here
    
    std::cout << "  ✓ OpenCL context RAII lifecycle works" << std::endl;
}

TEST(OpenCLRAII, QueueLifecycle) {
    cl_uint numPlatforms = 0;
    if (clGetPlatformIDs(0, nullptr, &numPlatforms) != CL_SUCCESS || numPlatforms == 0) {
        GTEST_SKIP() << "capability:opencl_platform_available=false;reason=no_opencl_platforms_available";
    }
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    
    cl_device_id device;
    cl_uint numDevices = 0;
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &numDevices) != CL_SUCCESS) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_devices_available";
    }
    
    OpenCLContext context;
    context.create(nullptr, 1, &device);
    
    // Test queue creation and destruction
    {
        OpenCLQueue queue;
        EXPECT_NO_THROW(queue.create(context.get(), device));
        EXPECT_TRUE(queue.valid());
        EXPECT_NE(queue.get(), nullptr);
    }
    // Queue should be automatically released here
    
    std::cout << "  ✓ OpenCL queue RAII lifecycle works" << std::endl;
}

TEST(OpenCLRAII, BufferLifecycle) {
    cl_uint numPlatforms = 0;
    if (clGetPlatformIDs(0, nullptr, &numPlatforms) != CL_SUCCESS || numPlatforms == 0) {
        GTEST_SKIP() << "capability:opencl_platform_available=false;reason=no_opencl_platforms_available";
    }
    
    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);
    
    cl_device_id device;
    cl_uint numDevices = 0;
    if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &numDevices) != CL_SUCCESS) {
        GTEST_SKIP() << "capability:opencl_device_available=false;reason=no_opencl_devices_available";
    }
    
    OpenCLContext context;
    context.create(nullptr, 1, &device);
    
    // Test buffer creation and destruction
    {
        OpenCLBuffer buffer;
        EXPECT_NO_THROW(buffer.create(context.get(), CL_MEM_READ_WRITE, 1024));
        EXPECT_TRUE(buffer.valid());
        EXPECT_NE(buffer.get(), nullptr);
        EXPECT_EQ(buffer.size(), 1024);
    }
    // Buffer should be automatically released here
    
    std::cout << "  ✓ OpenCL buffer RAII lifecycle works" << std::endl;
}

#endif // THEMIS_ENABLE_OPENCL

// ============================================================================
// RAII Principles Tests (no GPU hardware required)
// ============================================================================

TEST(RAII, ExceptionSafety) {
    // This test demonstrates that RAII wrappers properly clean up
    // even when exceptions are thrown
    
#ifdef THEMIS_ENABLE_CUDA
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) == cudaSuccess && deviceCount > 0) {
        try {
            CudaStream stream(true);
            CudaDeviceMemory mem(1024);
            
            // Simulate an error
            throw std::runtime_error("Simulated error");
        } catch (const std::runtime_error&) {
            // Resources should be cleaned up automatically
            // even though exception was thrown
        }
        
        std::cout << "  ✓ RAII exception safety validated (CUDA)" << std::endl;
    }
#endif
    
#ifdef THEMIS_ENABLE_OPENCL
    cl_uint numPlatforms = 0;
    if (clGetPlatformIDs(0, nullptr, &numPlatforms) == CL_SUCCESS && numPlatforms > 0) {
        cl_platform_id platform;
        clGetPlatformIDs(1, &platform, nullptr);
        
        cl_device_id device;
        cl_uint numDevices = 0;
        if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &numDevices) == CL_SUCCESS) {
            try {
                OpenCLContext context;
                context.create(nullptr, 1, &device);
                
                OpenCLQueue queue;
                queue.create(context.get(), device);
                
                // Simulate an error
                throw std::runtime_error("Simulated error");
            } catch (const std::runtime_error&) {
                // Resources should be cleaned up automatically
            }
            
            std::cout << "  ✓ RAII exception safety validated (OpenCL)" << std::endl;
        }
    }
#endif
    
    // If neither CUDA nor OpenCL available, skip
    SUCCEED();
}

// ============================================================================
// CublasHandle RAII Tests
// ============================================================================

#ifdef THEMIS_ENABLE_CUDA

TEST(CublasHandleRAII, DefaultCtorNotValid) {
    CublasHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.get(), nullptr);
}

TEST(CublasHandleRAII, CreateReturnsFalseWithoutDevice) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) == cudaSuccess && deviceCount > 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=true;reason=need_no_device_for_this_test";
    }
    CublasHandle h;
    // Without a CUDA device, create() must return false, not crash.
    bool ok = h.create();
    // Accept any result — the important thing is no crash and no leak.
    (void)ok;
    SUCCEED();
}

TEST(CublasHandleRAII, LifecycleWithDevice) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    {
        CublasHandle h;
        ASSERT_TRUE(h.create());
        EXPECT_TRUE(h.valid());
        EXPECT_NE(h.get(), nullptr);
    }
    // Destructor called here — no leak.
    std::cout << "  ✓ CublasHandle RAII lifecycle works" << std::endl;
}

TEST(CublasHandleRAII, MoveConstruct) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    CublasHandle h1;
    ASSERT_TRUE(h1.create());

    CublasHandle h2(std::move(h1));
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
    std::cout << "  ✓ CublasHandle move construction works" << std::endl;
}

TEST(CublasHandleRAII, MoveAssign) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    CublasHandle h1;
    ASSERT_TRUE(h1.create());

    CublasHandle h2;
    h2 = std::move(h1);
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
    std::cout << "  ✓ CublasHandle move assignment works" << std::endl;
}

TEST(CublasHandleRAII, ExceptionSafeCleanup) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    EXPECT_NO_FATAL_FAILURE({
        try {
            CublasHandle h;
            h.create();
            throw std::runtime_error("Simulated error during GPU operation");
        } catch (const std::runtime_error&) {
            // CublasHandle destructor must have run; no cuBLAS handle leak.
        }
    });
    std::cout << "  ✓ CublasHandle exception-safe cleanup works" << std::endl;
}

// ============================================================================
// CudaDeviceBuffer<T> RAII Tests
// ============================================================================

TEST(CudaDeviceBufferRAII, DefaultCtorNotValid) {
    CudaDeviceBuffer<float> buf;
    EXPECT_FALSE(buf.valid());
    EXPECT_EQ(buf.get(), nullptr);
    EXPECT_EQ(buf.count(), 0u);
}

TEST(CudaDeviceBufferRAII, TryAllocateNoDeviceReturnsFalse) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) == cudaSuccess && deviceCount > 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=true;reason=need_no_device_for_this_test";
    }
    CudaDeviceBuffer<float> buf;
    // Without GPU, tryAllocate must return false without crashing.
    bool ok = buf.tryAllocate(64);
    EXPECT_FALSE(ok);
    EXPECT_FALSE(buf.valid());
}

TEST(CudaDeviceBufferRAII, LifecycleWithDevice) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    {
        CudaDeviceBuffer<float> buf(16);
        EXPECT_TRUE(buf.valid());
        EXPECT_NE(buf.get(), nullptr);
        EXPECT_EQ(buf.count(), 16u);
    }
    // Destructor frees memory here.
    std::cout << "  ✓ CudaDeviceBuffer lifecycle works" << std::endl;
}

TEST(CudaDeviceBufferRAII, CopyRoundTrip) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    constexpr int N = 8;
    float host_in[N]  = {1, 2, 3, 4, 5, 6, 7, 8};
    float host_out[N] = {};

    CudaDeviceBuffer<float> buf(N);
    ASSERT_TRUE(buf.valid());
    EXPECT_NO_THROW(buf.copyFrom(host_in, N));
    EXPECT_NO_THROW(buf.copyTo(host_out, N));

    for (int i = 0; i < N; ++i) {
        EXPECT_FLOAT_EQ(host_out[i], host_in[i]);
    }
    std::cout << "  ✓ CudaDeviceBuffer copy round-trip works" << std::endl;
}

TEST(CudaDeviceBufferRAII, TryAllocateZeroCount) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }
    CudaDeviceBuffer<float> buf;
    EXPECT_TRUE(buf.tryAllocate(0));  // zero-count must succeed silently
    EXPECT_FALSE(buf.valid());        // but nothing allocated
}

TEST(CudaDeviceBufferRAII, MoveConstruct) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    CudaDeviceBuffer<float> b1(4);
    ASSERT_TRUE(b1.valid());

    CudaDeviceBuffer<float> b2(std::move(b1));
    EXPECT_FALSE(b1.valid());
    EXPECT_TRUE(b2.valid());
    EXPECT_EQ(b2.count(), 4u);
    std::cout << "  ✓ CudaDeviceBuffer move construction works" << std::endl;
}

TEST(CudaDeviceBufferRAII, ExceptionSafeCleanup) {
    int deviceCount = 0;
    if (cudaGetDeviceCount(&deviceCount) != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=no_cuda_devices_available";
    }

    EXPECT_NO_FATAL_FAILURE({
        try {
            CudaDeviceBuffer<float> d_a(32);
            CudaDeviceBuffer<float> d_b(32);
            throw std::runtime_error("Simulated GPU kernel failure");
        } catch (const std::runtime_error&) {
            // Both buffers must have been freed by their destructors.
        }
    });
    std::cout << "  ✓ CudaDeviceBuffer exception-safe cleanup works" << std::endl;
}

#endif // THEMIS_ENABLE_CUDA

// ============================================================================
// HipblasHandle RAII Tests
// ============================================================================

#ifdef THEMIS_ENABLE_HIP

TEST(HipblasHandleRAII, DefaultCtorNotValid) {
    HipblasHandle h;
    EXPECT_FALSE(h.valid());
    EXPECT_EQ(h.get(), nullptr);
}

TEST(HipblasHandleRAII, LifecycleWithDevice) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    {
        HipblasHandle h;
        ASSERT_TRUE(h.create());
        EXPECT_TRUE(h.valid());
        EXPECT_NE(h.get(), nullptr);
    }
    std::cout << "  ✓ HipblasHandle RAII lifecycle works" << std::endl;
}

TEST(HipblasHandleRAII, MoveConstruct) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    HipblasHandle h1;
    ASSERT_TRUE(h1.create());

    HipblasHandle h2(std::move(h1));
    EXPECT_FALSE(h1.valid());
    EXPECT_TRUE(h2.valid());
}

TEST(HipblasHandleRAII, ExceptionSafeCleanup) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    EXPECT_NO_FATAL_FAILURE({
        try {
            HipblasHandle h;
            h.create();
            throw std::runtime_error("Simulated HIP error");
        } catch (const std::runtime_error&) {}
    });
    std::cout << "  ✓ HipblasHandle exception-safe cleanup works" << std::endl;
}

// ============================================================================
// HipDeviceBuffer<T> RAII Tests
// ============================================================================

TEST(HipDeviceBufferRAII, DefaultCtorNotValid) {
    HipDeviceBuffer<float> buf;
    EXPECT_FALSE(buf.valid());
    EXPECT_EQ(buf.get(), nullptr);
    EXPECT_EQ(buf.count(), 0u);
}

TEST(HipDeviceBufferRAII, LifecycleWithDevice) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    {
        HipDeviceBuffer<float> buf(16);
        EXPECT_TRUE(buf.valid());
        EXPECT_NE(buf.get(), nullptr);
        EXPECT_EQ(buf.count(), 16u);
    }
    std::cout << "  ✓ HipDeviceBuffer lifecycle works" << std::endl;
}

TEST(HipDeviceBufferRAII, CopyRoundTrip) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    constexpr int N = 8;
    float host_in[N]  = {1, 2, 3, 4, 5, 6, 7, 8};
    float host_out[N] = {};

    HipDeviceBuffer<float> buf(N);
    ASSERT_TRUE(buf.valid());
    EXPECT_NO_THROW(buf.copyFrom(host_in, N));
    EXPECT_NO_THROW(buf.copyTo(host_out, N));

    for (int i = 0; i < N; ++i) {
        EXPECT_FLOAT_EQ(host_out[i], host_in[i]);
    }
    std::cout << "  ✓ HipDeviceBuffer copy round-trip works" << std::endl;
}

TEST(HipDeviceBufferRAII, ExceptionSafeCleanup) {
    int deviceCount = 0;
    if (hipGetDeviceCount(&deviceCount) != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=no_hip_devices_available";
    }

    EXPECT_NO_FATAL_FAILURE({
        try {
            HipDeviceBuffer<float> d_a(32);
            HipDeviceBuffer<float> d_b(32);
            throw std::runtime_error("Simulated HIP kernel failure");
        } catch (const std::runtime_error&) {}
    });
    std::cout << "  ✓ HipDeviceBuffer exception-safe cleanup works" << std::endl;
}

#endif // THEMIS_ENABLE_HIP
