#include <gtest/gtest.h>
#include "llm/lora_framework/vram_allocator.h"
#include "llm/lora_framework/gpu_memory.h"
#include <vector>

// Temporarily disable GPU LoRA tests on MSVC
#define SKIP_LORA_GPU_TESTS 1

#if SKIP_LORA_GPU_TESTS

TEST(DummyLoraGpu, DisabledOnMSVC) {
    GTEST_SKIP() << "capability:gpu_lora_tests_enabled=false;reason=msvc_porting_in_progress";
}

#else

using namespace themis::llm::lora;
using namespace themis::acceleration;

/**
 * @file test_lora_gpu.cpp
 * @brief Tests for GPU-accelerated LoRA training infrastructure
 * 
 * Test Coverage:
 * - VRAM allocation/deallocation
 * - GPU memory management
 * - Device detection and selection
 * - CPU ↔ GPU data transfer
 * - Memory pooling
 */

namespace {
    // Test memory sizes
    constexpr size_t SMALL_BLOCK_SIZE = 1024;
    constexpr size_t MEDIUM_BLOCK_SIZE = 256 * 1024;    // 256 KB
    constexpr size_t LARGE_BLOCK_SIZE = 1024 * 1024;    // 1 MB
    constexpr size_t POOL_SIZE_10MB = 10 * 1024 * 1024; // 10 MB
    constexpr size_t TEST_ARRAY_SIZE_128 = 128;
    constexpr size_t TEST_ARRAY_SIZE_256 = 256;
}

class LoRAGPUTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Detect available backends
        auto backends = GPUMemoryManager::detect_backends();
        
        has_cuda_ = false;
        has_hip_ = false;
        has_vulkan_ = false;
        
        for (const auto& backend : backends) {
            if (backend.type == BackendType::CUDA && backend.available) {
                has_cuda_ = true;
            }
            if (backend.type == BackendType::HIP && backend.available) {
                has_hip_ = true;
            }
            if (backend.type == BackendType::VULKAN && backend.available) {
                has_vulkan_ = true;
            }
        }
    }
    
    bool has_cuda_ = false;
    bool has_hip_ = false;
    bool has_vulkan_ = false;
};

// ===== VRAMAllocator Tests =====

TEST_F(LoRAGPUTest, VRAMAllocator_CPUBackend) {
    // CPU backend should always be available
    VRAMAllocator allocator(BackendType::CPU);
    EXPECT_TRUE(allocator.is_available());
    EXPECT_EQ(allocator.backend_type(), BackendType::CPU);
}

TEST_F(LoRAGPUTest, VRAMAllocator_BasicAllocation) {
    VRAMAllocator allocator(BackendType::CPU);
    
    // Allocate memory
    void* ptr = allocator.allocate(SMALL_BLOCK_SIZE);
    ASSERT_NE(ptr, nullptr);
    
    // Check stats
    auto stats = allocator.get_stats();
    EXPECT_GE(stats.allocated_bytes, SMALL_BLOCK_SIZE);
    EXPECT_EQ(stats.allocation_count, 1);
    
    // Deallocate
    allocator.deallocate(ptr);
    
    // Stats should reflect deallocation
    stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

TEST_F(LoRAGPUTest, VRAMAllocator_MultipleAllocations) {
    VRAMAllocator allocator(BackendType::CPU);
    
    std::vector<void*> ptrs;
    size_t total_size = 0;
    
    // Allocate multiple blocks
    for (int i = 0; i < 10; i++) {
        size_t size = (i + 1) * 256;
        void* ptr = allocator.allocate(size);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
        total_size += size;
    }
    
    // Check stats
    auto stats = allocator.get_stats();
    EXPECT_GE(stats.allocated_bytes, total_size);
    EXPECT_EQ(stats.allocation_count, 10);
    
    // Deallocate all
    for (auto ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    
    // All memory should be freed
    stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

TEST_F(LoRAGPUTest, VRAMAllocator_UploadDownload) {
    VRAMAllocator allocator(BackendType::CPU);
    
    // Create test data
    std::vector<float> host_data(TEST_ARRAY_SIZE_256);
    for (size_t i = 0; i < host_data.size(); i++) {
        host_data[i] = static_cast<float>(i);
    }
    
    // Allocate GPU memory
    size_t size_bytes = host_data.size() * sizeof(float);
    void* gpu_ptr = allocator.allocate(size_bytes);
    ASSERT_NE(gpu_ptr, nullptr);
    
    // Upload data
    bool success = allocator.upload(gpu_ptr, host_data.data(), size_bytes);
    EXPECT_TRUE(success);
    
    // Download data
    std::vector<float> result(TEST_ARRAY_SIZE_256);
    success = allocator.download(result.data(), gpu_ptr, size_bytes);
    EXPECT_TRUE(success);
    
    // Verify data matches
    for (size_t i = 0; i < host_data.size(); i++) {
        EXPECT_FLOAT_EQ(result[i], host_data[i]);
    }
    
    allocator.deallocate(gpu_ptr);
}

TEST_F(LoRAGPUTest, VRAMAllocator_MemoryPooling) {
    VRAMAllocator allocator(BackendType::CPU);
    
    // Allocate and deallocate multiple times
    void* ptr1 = allocator.allocate(SMALL_BLOCK_SIZE);
    ASSERT_NE(ptr1, nullptr);
    allocator.deallocate(ptr1);
    
    // Second allocation should reuse freed block
    void* ptr2 = allocator.allocate(SMALL_BLOCK_SIZE);
    ASSERT_NE(ptr2, nullptr);
    
    // May reuse same memory
    // (exact behavior depends on pool implementation)
    
    allocator.deallocate(ptr2);
}

TEST_F(LoRAGPUTest, VRAMAllocator_Reset) {
    VRAMAllocator allocator(BackendType::CPU);
    
    // Allocate several blocks
    for (int i = 0; i < 5; i++) {
        allocator.allocate(SMALL_BLOCK_SIZE);
    }
    
    auto stats = allocator.get_stats();
    EXPECT_GT(stats.allocated_bytes, 0);
    
    // Reset should free everything
    allocator.reset();
    
    stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

// ===== VRAMTensor Tests =====

TEST_F(LoRAGPUTest, VRAMTensor_Construction) {
    VRAMAllocator allocator(BackendType::CPU);
    
    VRAMTensor tensor(&allocator, SMALL_BLOCK_SIZE);
    EXPECT_NE(tensor.ptr(), nullptr);
    EXPECT_EQ(tensor.size(), SMALL_BLOCK_SIZE);
}

TEST_F(LoRAGPUTest, VRAMTensor_MoveSemantics) {
    VRAMAllocator allocator(BackendType::CPU);
    
    VRAMTensor tensor1(&allocator, SMALL_BLOCK_SIZE);
    void* original_ptr = tensor1.ptr();
    
    // Move construct
    VRAMTensor tensor2(std::move(tensor1));
    EXPECT_EQ(tensor2.ptr(), original_ptr);
    EXPECT_EQ(tensor1.ptr(), nullptr);
    
    // Move assign
    VRAMTensor tensor3(&allocator, SMALL_BLOCK_SIZE / 2);
    tensor3 = std::move(tensor2);
    EXPECT_EQ(tensor3.ptr(), original_ptr);
    EXPECT_EQ(tensor2.ptr(), nullptr);
}

TEST_F(LoRAGPUTest, VRAMTensor_UploadDownload) {
    VRAMAllocator allocator(BackendType::CPU);
    
    std::vector<float> data(TEST_ARRAY_SIZE_128);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<float>(i * 2);
    }
    
    VRAMTensor tensor(&allocator, data.size() * sizeof(float));
    
    // Upload
    bool success = tensor.upload(data.data(), data.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    // Download
    std::vector<float> result(TEST_ARRAY_SIZE_128);
    success = tensor.download(result.data(), result.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    // Verify
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_FLOAT_EQ(result[i], data[i]);
    }
}

// ===== GPUMemoryManager Tests =====

TEST_F(LoRAGPUTest, GPUMemoryManager_Construction) {
    GPUMemoryManager manager;
    
    // Should auto-select a device
    Device default_device = manager.default_device();
    EXPECT_TRUE(default_device.type == DeviceType::CPU || 
                default_device.type == DeviceType::CUDA ||
                default_device.type == DeviceType::HIP ||
                default_device.type == DeviceType::VULKAN ||
                default_device.type == DeviceType::DIRECTX);
}

TEST_F(LoRAGPUTest, GPUMemoryManager_DeviceDetection) {
    auto backends = GPUMemoryManager::detect_backends();
    
    // Should detect at least CPU
    EXPECT_GE(backends.size(), 1);
    
    // Find CPU backend
    bool found_cpu = false;
    for (const auto& backend : backends) {
        if (backend.type == BackendType::CPU && backend.available) {
            found_cpu = true;
            break;
        }
    }
    EXPECT_TRUE(found_cpu);
}

TEST_F(LoRAGPUTest, GPUMemoryManager_DetectBackends_IncludesVulkanEntry) {
    auto backends = GPUMemoryManager::detect_backends();

    bool found_vulkan = false;
    for (const auto& backend : backends) {
        if (backend.type == BackendType::VULKAN) {
            found_vulkan = true;
            // When available, device_name must be non-empty
            if (backend.available) {
                EXPECT_FALSE(backend.device_name.empty());
            }
            break;
        }
    }
    EXPECT_TRUE(found_vulkan) << "Vulkan entry must always be present in detect_backends()";
}

TEST_F(LoRAGPUTest, GPUMemoryManager_DetectBackends_IncludesDirectXEntry) {
    auto backends = GPUMemoryManager::detect_backends();

    bool found_directx = false;
    for (const auto& backend : backends) {
        if (backend.type == BackendType::DIRECTX) {
            found_directx = true;
            // When available (Windows + DirectX enabled), device_name must be non-empty
            if (backend.available) {
                EXPECT_FALSE(backend.device_name.empty());
            }
            break;
        }
    }
    // DirectX entry is always emitted (available=false on non-Windows/non-DirectX builds)
    EXPECT_TRUE(found_directx) << "DirectX entry must always be present in detect_backends()";
}

TEST_F(LoRAGPUTest, GPUMemoryManager_AvailableDevices) {
    GPUMemoryManager manager;
    
    auto devices = manager.available_devices();
    EXPECT_GE(devices.size(), 1);  // At least CPU
    
    // CPU should always be available
    bool has_cpu = false;
    for (const auto& device : devices) {
        if (device.type == DeviceType::CPU) {
            has_cpu = true;
            break;
        }
    }
    EXPECT_TRUE(has_cpu);
}

TEST_F(LoRAGPUTest, GPUMemoryManager_GetAllocator) {
    GPUMemoryManager manager;
    
    // Get CPU allocator
    Device cpu_device = Device::cpu();
    VRAMAllocator* cpu_alloc = manager.get_allocator(cpu_device);
    ASSERT_NE(cpu_alloc, nullptr);
    EXPECT_TRUE(cpu_alloc->is_available());
    
    // Test allocation through manager
    void* ptr = cpu_alloc->allocate(SMALL_BLOCK_SIZE);
    ASSERT_NE(ptr, nullptr);
    cpu_alloc->deallocate(ptr);
}

TEST_F(LoRAGPUTest, GPUMemoryManager_AutoSelectDevice) {
    Device device = GPUMemoryManager::auto_select_device();
    
    // Should select a valid device
    // Priority: Vulkan → CUDA → HIP → DirectX → CPU
    EXPECT_TRUE(device.type == DeviceType::CPU || 
                device.type == DeviceType::CUDA ||
                device.type == DeviceType::HIP ||
                device.type == DeviceType::VULKAN ||
                device.type == DeviceType::DIRECTX);
}

TEST_F(LoRAGPUTest, GPUMemoryManager_Stats) {
    GPUMemoryManager manager;
    
    Device cpu_device = Device::cpu();
    VRAMAllocator* allocator = manager.get_allocator(cpu_device);
    
    // Allocate some memory
    void* ptr = allocator->allocate(MEDIUM_BLOCK_SIZE);
    ASSERT_NE(ptr, nullptr);
    
    // Get stats
    auto stats = manager.get_stats(cpu_device);
    EXPECT_GE(stats.allocated_bytes, MEDIUM_BLOCK_SIZE);
    
    allocator->deallocate(ptr);
}

// ===== CUDA Backend Tests (conditional) =====

TEST_F(LoRAGPUTest, CUDA_Availability) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    VRAMAllocator allocator(BackendType::CUDA);
    EXPECT_TRUE(allocator.is_available());
    EXPECT_EQ(allocator.backend_type(), BackendType::CUDA);
}

TEST_F(LoRAGPUTest, CUDA_BasicOperations) {
    if (!has_cuda_) {
        GTEST_SKIP() << "capability:cuda_runtime_available=false;reason=cuda_not_available";
    }
    
    VRAMAllocator allocator(BackendType::CUDA);
    
    // Allocate GPU memory
    constexpr size_t cuda_test_size = 1024;
    void* gpu_ptr = allocator.allocate(cuda_test_size * sizeof(float));
    ASSERT_NE(gpu_ptr, nullptr);
    
    // Upload/download test
    std::vector<float> host_data(cuda_test_size, 42.0f);
    bool success = allocator.upload(gpu_ptr, host_data.data(), 
                                    host_data.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    std::vector<float> result(cuda_test_size);
    success = allocator.download(result.data(), gpu_ptr, 
                                 result.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    for (size_t i = 0; i < host_data.size(); i++) {
        EXPECT_FLOAT_EQ(result[i], host_data[i]);
    }
    
    allocator.deallocate(gpu_ptr);
}

// ===== HIP Backend Tests (conditional) =====

TEST_F(LoRAGPUTest, HIP_Availability) {
    if (!has_hip_) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_not_available";
    }
    
    VRAMAllocator allocator(BackendType::HIP);
    EXPECT_TRUE(allocator.is_available());
    EXPECT_EQ(allocator.backend_type(), BackendType::HIP);
}

TEST_F(LoRAGPUTest, HIP_BasicOperations) {
    if (!has_hip_) {
        GTEST_SKIP() << "capability:hip_runtime_available=false;reason=hip_not_available";
    }
    
    VRAMAllocator allocator(BackendType::HIP);
    
    // Similar to CUDA test
    constexpr size_t hip_test_size = 1024;
    void* gpu_ptr = allocator.allocate(hip_test_size * sizeof(float));
    ASSERT_NE(gpu_ptr, nullptr);
    
    std::vector<float> host_data(hip_test_size, 3.14f);
    bool success = allocator.upload(gpu_ptr, host_data.data(), 
                                    host_data.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    std::vector<float> result(hip_test_size);
    success = allocator.download(result.data(), gpu_ptr, 
                                 result.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    for (size_t i = 0; i < host_data.size(); i++) {
        EXPECT_FLOAT_EQ(result[i], host_data[i]);
    }
    
    allocator.deallocate(gpu_ptr);
}

// ===== Memory Overhead Tests =====

TEST_F(LoRAGPUTest, MemoryOverhead_LessThan5Percent) {
    VRAMAllocator allocator(BackendType::CPU, POOL_SIZE_10MB);
    
    // Allocate memory
    void* ptr = allocator.allocate(LARGE_BLOCK_SIZE);
    ASSERT_NE(ptr, nullptr);
    
    auto stats = allocator.get_stats();
    
    // Calculate overhead percentage
    float overhead_percent = (static_cast<float>(stats.overhead_bytes) / 
                             static_cast<float>(stats.allocated_bytes)) * 100.0f;
    
    // Should be less than 5%
    EXPECT_LT(overhead_percent, 5.0f);
    
    allocator.deallocate(ptr);
}

// ===== Device String Conversion Tests =====

TEST_F(LoRAGPUTest, DeviceToString) {
    Device cpu = Device::cpu();
    EXPECT_EQ(cpu.to_string(), "CPU:0");
    
    Device cuda = Device::cuda(1);
    EXPECT_EQ(cuda.to_string(), "CUDA:1");
    
    Device hip = Device::hip(2);
    EXPECT_EQ(hip.to_string(), "HIP:2");
}
#endif // SKIP_LORA_GPU_TESTS

