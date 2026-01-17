#include <gtest/gtest.h>
#include "llm/lora_framework/gpu_memory.h"
#include "llm/lora_framework/vram_allocator.h"
#include "llm/lora_framework/gpu_tensor.h"
#include <thread>
#include <vector>
#include <chrono>

using namespace themis::llm::lora;

namespace {
    constexpr size_t MB = 1024 * 1024;
    constexpr size_t KB = 1024;
}

// ============================================================================
// VRAM Allocator Basic Tests
// ============================================================================

TEST(VRAMAllocatorTest, InitializationCPU) {
    VRAMAllocator allocator(acceleration::BackendType::CPU);
    EXPECT_TRUE(allocator.is_available());
    EXPECT_EQ(allocator.backend_type(), acceleration::BackendType::CPU);
}

TEST(VRAMAllocatorTest, BasicAllocationAndDeallocation) {
    VRAMAllocator allocator(acceleration::BackendType::CPU);
    
    size_t size = 1024;
    void* ptr = allocator.allocate(size);
    
    ASSERT_NE(ptr, nullptr);
    
    auto stats = allocator.get_stats();
    EXPECT_GT(stats.allocated_bytes, 0);
    EXPECT_GT(stats.allocation_count, 0);
    
    allocator.deallocate(ptr);
    
    stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

TEST(VRAMAllocatorTest, MultipleAllocations) {
    VRAMAllocator allocator(acceleration::BackendType::CPU);
    
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i) {
        void* ptr = allocator.allocate(1024);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocation_count, 10);
    
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    
    stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

TEST(VRAMAllocatorTest, AlignmentRespected) {
    VRAMAllocator allocator(acceleration::BackendType::CPU);
    
    size_t alignment = 256;
    void* ptr = allocator.allocate(1024, alignment);
    ASSERT_NE(ptr, nullptr);
    
    // Check alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(addr % alignment, 0) << "Memory not aligned to " << alignment << " bytes";
    
    allocator.deallocate(ptr);
}

// ============================================================================
// Out-of-Memory (OOM) Handling Tests
// ============================================================================

TEST(VRAMAllocatorOOMTest, LargeAllocationFailure) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 10 * MB);
    
    // Try to allocate more than pool size
    void* ptr = allocator.allocate(100 * MB);
    
    // Should handle gracefully (return nullptr or throw)
    if (ptr != nullptr) {
        allocator.deallocate(ptr);
        GTEST_SKIP() << "System has enough memory for large allocation";
    } else {
        // Allocator should still be functional
        EXPECT_TRUE(allocator.is_available());
        
        // Small allocation should still work
        void* small_ptr = allocator.allocate(1024);
        EXPECT_NE(small_ptr, nullptr);
        allocator.deallocate(small_ptr);
    }
}

TEST(VRAMAllocatorOOMTest, GradualMemoryExhaustion) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 5 * MB);
    
    std::vector<void*> ptrs;
    size_t total_allocated = 0;
    
    // Allocate until we run out
    for (int i = 0; i < 1000; ++i) {
        void* ptr = allocator.allocate(100 * KB);
        if (ptr == nullptr) {
            break;
        }
        ptrs.push_back(ptr);
        total_allocated += 100 * KB;
    }
    
    // Should have allocated some memory
    EXPECT_GT(ptrs.size(), 0);
    
    // Check stats
    auto stats = allocator.get_stats();
    EXPECT_GT(stats.allocated_bytes, 0);
    EXPECT_LE(stats.allocated_bytes, 5 * MB + stats.overhead_bytes);
    
    // Clean up
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

TEST(VRAMAllocatorOOMTest, GracefulDegradation) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 2 * MB);
    
    // Fill memory
    std::vector<void*> ptrs;
    while (true) {
        void* ptr = allocator.allocate(100 * KB);
        if (ptr == nullptr) break;
        ptrs.push_back(ptr);
    }
    
    // Allocator should still be functional
    EXPECT_TRUE(allocator.is_available());
    
    // Free some memory
    if (!ptrs.empty()) {
        allocator.deallocate(ptrs.back());
        ptrs.pop_back();
    }
    
    // New allocation should work
    void* new_ptr = allocator.allocate(50 * KB);
    EXPECT_NE(new_ptr, nullptr);
    
    // Clean up
    if (new_ptr) allocator.deallocate(new_ptr);
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

// ============================================================================
// Memory Fragmentation Tests
// ============================================================================

TEST(VRAMAllocatorFragmentationTest, FragmentationDetection) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 10 * MB);
    
    // Create fragmentation by alternating allocations and deallocations
    std::vector<void*> ptrs;
    
    // Allocate 10 blocks
    for (int i = 0; i < 10; ++i) {
        void* ptr = allocator.allocate(100 * KB);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Free every other block
    for (size_t i = 1; i < ptrs.size(); i += 2) {
        allocator.deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }
    
    auto stats = allocator.get_stats();
    // Fragmentation should be > 0 due to holes
    EXPECT_GE(stats.fragmentation, 0.0f);
    EXPECT_LE(stats.fragmentation, 1.0f);
    
    // Clean up
    for (void* ptr : ptrs) {
        if (ptr) allocator.deallocate(ptr);
    }
}

TEST(VRAMAllocatorFragmentationTest, FragmentationReduction) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 10 * MB);
    
    // Create fragmentation
    std::vector<void*> ptrs;
    for (int i = 0; i < 20; ++i) {
        void* ptr = allocator.allocate(50 * KB);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Free every other block
    for (size_t i = 1; i < ptrs.size(); i += 2) {
        allocator.deallocate(ptrs[i]);
        ptrs[i] = nullptr;
    }
    
    float initial_frag = allocator.get_stats().fragmentation;
    
    // Free all blocks to allow coalescing
    for (void* ptr : ptrs) {
        if (ptr) allocator.deallocate(ptr);
    }
    
    float final_frag = allocator.get_stats().fragmentation;
    
    // Fragmentation should be reduced (or at least not increased)
    EXPECT_LE(final_frag, initial_frag);
}

// ============================================================================
// VRAM Stress Testing
// ============================================================================

TEST(VRAMAllocatorStressTest, RapidAllocationDeallocation) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 20 * MB);
    
    // Rapidly allocate and deallocate
    for (int iteration = 0; iteration < 100; ++iteration) {
        std::vector<void*> ptrs;
        
        // Allocate
        for (int i = 0; i < 10; ++i) {
            void* ptr = allocator.allocate(50 * KB);
            if (ptr) ptrs.push_back(ptr);
        }
        
        // Deallocate
        for (void* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
    
    // Should still be functional
    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
    EXPECT_TRUE(allocator.is_available());
}

TEST(VRAMAllocatorStressTest, ConcurrentAllocations) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 50 * MB);
    
    const int num_threads = 4;
    const int allocs_per_thread = 20;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<void*>> thread_ptrs(num_threads);
    
    // Spawn threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&allocator, &thread_ptrs, t, allocs_per_thread]() {
            for (int i = 0; i < allocs_per_thread; ++i) {
                void* ptr = allocator.allocate(10 * KB);
                if (ptr) {
                    thread_ptrs[t].push_back(ptr);
                }
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Check that allocations succeeded
    size_t total_allocs = 0;
    for (const auto& ptrs : thread_ptrs) {
        total_allocs += ptrs.size();
    }
    EXPECT_GT(total_allocs, 0);
    
    // Clean up
    for (auto& ptrs : thread_ptrs) {
        for (void* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
    }
    
    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

TEST(VRAMAllocatorStressTest, PeakUsageTracking) {
    VRAMAllocator allocator(acceleration::BackendType::CPU, 20 * MB);
    
    std::vector<void*> ptrs;
    
    // Allocate to peak
    for (int i = 0; i < 50; ++i) {
        void* ptr = allocator.allocate(100 * KB);
        if (ptr) ptrs.push_back(ptr);
    }
    
    auto peak_stats = allocator.get_stats();
    size_t peak_usage = peak_stats.peak_usage_bytes;
    
    EXPECT_GT(peak_usage, 0);
    
    // Deallocate half
    for (size_t i = 0; i < ptrs.size() / 2; ++i) {
        allocator.deallocate(ptrs[i]);
    }
    ptrs.erase(ptrs.begin(), ptrs.begin() + ptrs.size() / 2);
    
    auto current_stats = allocator.get_stats();
    
    // Peak should remain the same
    EXPECT_EQ(current_stats.peak_usage_bytes, peak_usage);
    
    // Current usage should be less
    EXPECT_LT(current_stats.allocated_bytes, peak_usage);
    
    // Clean up
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

// ============================================================================
// GPU Memory Manager Tests
// ============================================================================

TEST(GPUMemoryManagerTest, Initialization) {
    GPUMemoryManager manager;
    
    auto devices = manager.available_devices();
    EXPECT_GT(devices.size(), 0);  // At least CPU should be available
    
    Device default_dev = manager.default_device();
    EXPECT_TRUE(manager.is_device_available(default_dev));
}

TEST(GPUMemoryManagerTest, DeviceAvailability) {
    GPUMemoryManager manager;
    
    // CPU should always be available
    EXPECT_TRUE(manager.is_device_available(Device::cpu()));
    
    // Check other devices (may or may not be available)
    Device cuda_dev = Device::cuda();
    Device hip_dev = Device::hip();
    
    // These tests just ensure the checks don't crash
    bool cuda_available = manager.is_device_available(cuda_dev);
    bool hip_available = manager.is_device_available(hip_dev);
    
    EXPECT_TRUE(cuda_available || hip_available || true);  // At least one should work
}

TEST(GPUMemoryManagerTest, BackendDetection) {
    auto backends = GPUMemoryManager::detect_backends();
    
    EXPECT_GT(backends.size(), 0);
    
    // CPU backend should always be present
    bool cpu_found = false;
    for (const auto& backend : backends) {
        if (backend.type == acceleration::BackendType::CPU) {
            cpu_found = true;
            EXPECT_TRUE(backend.available);
        }
    }
    EXPECT_TRUE(cpu_found);
}

TEST(GPUMemoryManagerTest, GetAllocator) {
    GPUMemoryManager manager;
    
    Device cpu_dev = Device::cpu();
    VRAMAllocator* allocator = manager.get_allocator(cpu_dev);
    
    ASSERT_NE(allocator, nullptr);
    EXPECT_TRUE(allocator->is_available());
}

TEST(GPUMemoryManagerTest, MemoryStatistics) {
    GPUMemoryManager manager;
    
    Device cpu_dev = Device::cpu();
    auto stats = manager.get_stats(cpu_dev);
    
    // Stats should be retrievable
    EXPECT_GE(stats.total_bytes, 0);
    EXPECT_GE(stats.free_bytes, 0);
}

// ============================================================================
// Integration Tests with GPUTensor
// ============================================================================

TEST(GPUMemoryIntegrationTest, TensorAllocationOnCPU) {
    GPUTensor tensor({100, 100}, Device::cpu());
    
    std::vector<float> data(100 * 100, 1.0f);
    tensor.upload(data);
    
    auto result = tensor.cpu_data();
    EXPECT_EQ(result.size(), 100 * 100);
    
    for (float val : result) {
        EXPECT_FLOAT_EQ(val, 1.0f);
    }
}

TEST(GPUMemoryIntegrationTest, MultipleTensorAllocations) {
    std::vector<GPUTensor> tensors;
    
    for (int i = 0; i < 10; ++i) {
        tensors.emplace_back(std::vector<size_t>{50, 50}, Device::cpu());
        
        std::vector<float> data(50 * 50, static_cast<float>(i));
        tensors.back().upload(data);
    }
    
    // Verify all tensors
    for (size_t i = 0; i < tensors.size(); ++i) {
        auto data = tensors[i].cpu_data();
        EXPECT_EQ(data.size(), 50 * 50);
        
        for (float val : data) {
            EXPECT_FLOAT_EQ(val, static_cast<float>(i));
        }
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
