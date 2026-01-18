#include <gtest/gtest.h>
#include "security/vram_secure_clear.h"
#include <cstring>
#include <vector>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

using namespace themis::security;

/**
 * @brief Test CPU secure clear with multi-pass overwrite
 */
TEST(VRAMSecureClearTest, CPUSecureClear) {
    const size_t size = 1024;
    std::vector<uint8_t> buffer(size);
    
    // Fill with sensitive data
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    // Verify data is present
    EXPECT_NE(buffer[0], 0xAA);
    
    // Secure clear with 3 passes
    VRAMSecureClear::Config config;
    config.num_passes = 3;
    config.audit_log = false;
    
    VRAMSecureClear::secureClearCPU(buffer.data(), size, config);
    
    // Verify data is cleared (should contain last pattern 0xAA)
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(buffer[i], 0xAA) << "Byte " << i << " not cleared";
    }
}

/**
 * @brief Test CPU secure clear with single pass
 */
TEST(VRAMSecureClearTest, CPUSinglePass) {
    const size_t size = 512;
    std::vector<uint8_t> buffer(size, 0x42);
    
    VRAMSecureClear::Config config;
    config.num_passes = 1;
    config.audit_log = false;
    
    VRAMSecureClear::secureClearCPU(buffer.data(), size, config);
    
    // Should contain pattern 0x00 (first pattern)
    for (size_t i = 0; i < size; ++i) {
        EXPECT_EQ(buffer[i], 0x00);
    }
}

/**
 * @brief Test CPU secure clear with null pointer
 */
TEST(VRAMSecureClearTest, CPUNullPointer) {
    VRAMSecureClear::Config config;
    config.audit_log = false;
    
    // Should not crash
    EXPECT_NO_THROW(VRAMSecureClear::secureClearCPU(nullptr, 0, config));
    EXPECT_NO_THROW(VRAMSecureClear::secureClearCPU(nullptr, 1024, config));
}

#ifdef THEMIS_ENABLE_CUDA
/**
 * @brief Test CUDA secure clear with multi-pass overwrite
 */
TEST(VRAMSecureClearTest, CUDASecureClear) {
    // Check if CUDA is available
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "CUDA not available, skipping test";
    }
    
    const size_t size = 4096;
    void* d_ptr = nullptr;
    
    // Allocate device memory
    err = cudaMalloc(&d_ptr, size);
    ASSERT_EQ(err, cudaSuccess) << "cudaMalloc failed";
    
    // Fill with test pattern
    err = cudaMemset(d_ptr, 0x42, size);
    ASSERT_EQ(err, cudaSuccess) << "cudaMemset failed";
    
    // Secure clear with 3 passes
    VRAMSecureClear::Config config;
    config.num_passes = 3;
    config.verify_clear = true;
    config.audit_log = false;
    
    bool success = VRAMSecureClear::secureClearCUDA(d_ptr, size, config);
    EXPECT_TRUE(success) << "Secure clear failed";
    
    // Verify by reading back
    std::vector<uint8_t> host_buffer(size);
    err = cudaMemcpy(host_buffer.data(), d_ptr, size, cudaMemcpyDeviceToHost);
    ASSERT_EQ(err, cudaSuccess) << "cudaMemcpy failed";
    
    // Should contain last pattern 0xAA
    for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {
        EXPECT_EQ(host_buffer[i], 0xAA) << "GPU byte " << i << " not cleared";
    }
    
    // Free device memory
    cudaFree(d_ptr);
}

/**
 * @brief Test CUDA secure clear with verification enabled
 */
TEST(VRAMSecureClearTest, CUDAWithVerification) {
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "CUDA not available, skipping test";
    }
    
    const size_t size = 2048;
    void* d_ptr = nullptr;
    
    err = cudaMalloc(&d_ptr, size);
    ASSERT_EQ(err, cudaSuccess);
    
    VRAMSecureClear::Config config;
    config.num_passes = 2;
    config.verify_clear = true;
    config.audit_log = false;
    
    bool success = VRAMSecureClear::secureClearCUDA(d_ptr, size, config);
    EXPECT_TRUE(success);
    
    cudaFree(d_ptr);
}

/**
 * @brief Test CUDA secure clear with null pointer
 */
TEST(VRAMSecureClearTest, CUDANullPointer) {
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        GTEST_SKIP() << "CUDA not available, skipping test";
    }
    
    VRAMSecureClear::Config config;
    config.audit_log = false;
    
    // Should not crash
    EXPECT_TRUE(VRAMSecureClear::secureClearCUDA(nullptr, 0, config));
    EXPECT_TRUE(VRAMSecureClear::secureClearCUDA(nullptr, 1024, config));
}
#endif // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_HIP
/**
 * @brief Test HIP secure clear
 */
TEST(VRAMSecureClearTest, HIPSecureClear) {
    int deviceCount = 0;
    hipError_t err = hipGetDeviceCount(&deviceCount);
    if (err != hipSuccess || deviceCount == 0) {
        GTEST_SKIP() << "HIP not available, skipping test";
    }
    
    const size_t size = 4096;
    void* d_ptr = nullptr;
    
    err = hipMalloc(&d_ptr, size);
    ASSERT_EQ(err, hipSuccess);
    
    err = hipMemset(d_ptr, 0x42, size);
    ASSERT_EQ(err, hipSuccess);
    
    VRAMSecureClear::Config config;
    config.num_passes = 3;
    config.verify_clear = true;
    config.audit_log = false;
    
    bool success = VRAMSecureClear::secureClearHIP(d_ptr, size, config);
    EXPECT_TRUE(success);
    
    std::vector<uint8_t> host_buffer(size);
    err = hipMemcpy(host_buffer.data(), d_ptr, size, hipMemcpyDeviceToHost);
    ASSERT_EQ(err, hipSuccess);
    
    for (size_t i = 0; i < std::min(size, size_t(100)); ++i) {
        EXPECT_EQ(host_buffer[i], 0xAA);
    }
    
    hipFree(d_ptr);
}
#endif // THEMIS_ENABLE_HIP

/**
 * @brief Test that multi-pass uses different patterns
 */
TEST(VRAMSecureClearTest, MultiPassPatterns) {
    const size_t size = 256;
    std::vector<uint8_t> buffer1(size);
    std::vector<uint8_t> buffer2(size);
    std::vector<uint8_t> buffer3(size);
    
    // Fill all with same data
    std::memset(buffer1.data(), 0xFF, size);
    std::memset(buffer2.data(), 0xFF, size);
    std::memset(buffer3.data(), 0xFF, size);
    
    VRAMSecureClear::Config config;
    config.audit_log = false;
    
    // Test different number of passes
    config.num_passes = 1;
    VRAMSecureClear::secureClearCPU(buffer1.data(), size, config);
    EXPECT_EQ(buffer1[0], 0x00); // First pattern
    
    config.num_passes = 2;
    VRAMSecureClear::secureClearCPU(buffer2.data(), size, config);
    EXPECT_EQ(buffer2[0], 0xFF); // Second pattern
    
    config.num_passes = 3;
    VRAMSecureClear::secureClearCPU(buffer3.data(), size, config);
    EXPECT_EQ(buffer3[0], 0xAA); // Third pattern
}

/**
 * @brief Test secure clear with large memory region
 */
TEST(VRAMSecureClearTest, LargeMemoryRegion) {
    const size_t size = 10 * 1024 * 1024; // 10 MB
    std::vector<uint8_t> buffer(size);
    
    // Fill with test data
    for (size_t i = 0; i < size; i += 1024) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    VRAMSecureClear::Config config;
    config.num_passes = 3;
    config.audit_log = false;
    
    VRAMSecureClear::secureClearCPU(buffer.data(), size, config);
    
    // Spot check clearance
    EXPECT_EQ(buffer[0], 0xAA);
    EXPECT_EQ(buffer[size / 2], 0xAA);
    EXPECT_EQ(buffer[size - 1], 0xAA);
}
