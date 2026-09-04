// Test for memory pool allocator
// Tests all allocator types: Buddy, Slab, Stack, and Pool

#include <utils/memory/pool_allocator.h>
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <limits>

using namespace themis::memory;

// ============================================================================
// Buddy Allocator Tests
// ============================================================================

TEST(BuddyAllocatorTest, BasicAllocation) {
    BuddyAllocator allocator(1024 * 1024, 64);  // 1MB, 64B min block
    
    auto result = allocator.allocate(256);
    ASSERT_TRUE(result.has_value());
    void* ptr = *result;
    ASSERT_NE(ptr, nullptr);
    
    // Write and read data
    std::memset(ptr, 0x42, 256);
    EXPECT_EQ(static_cast<uint8_t*>(ptr)[0], 0x42);
    
    auto dealloc_result = allocator.deallocate(ptr);
    ASSERT_TRUE(dealloc_result.has_value());
}

TEST(BuddyAllocatorTest, MultipleAllocations) {
    BuddyAllocator allocator(64 * 1024, 64);  // 64KB
    
    std::vector<void*> ptrs = {};

    for (int i = 0; i < 10; ++i) {
        auto result = allocator.allocate(128);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
    }
    
    // Deallocate all
    for (void* ptr : ptrs) {
        auto result = allocator.deallocate(ptr);
        EXPECT_TRUE(result.has_value());
    }
}

TEST(BuddyAllocatorTest, VariableSizes) {
    BuddyAllocator allocator(1024 * 1024, 64);
    
    // Test different sizes
    std::vector<size_t> sizes = {64, 128, 256, 512, 1024, 2048, 4096};
    std::vector<void*> ptrs;
    
    for (size_t size : sizes) {
        auto result = allocator.allocate(size);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
        
        // Verify we can use the memory
        std::memset(*result, 0xFF, size);
    }
    
    for (void* ptr : ptrs) {
        EXPECT_TRUE(allocator.deallocate(ptr).has_value());
    }
}

TEST(BuddyAllocatorTest, Exhaustion) {
    BuddyAllocator allocator(4096, 64);  // Small pool
    
    std::vector<void*> ptrs;
    
    // Allocate until exhausted
    while (true) {
        auto result = allocator.allocate(64);
        if (!result.has_value()) {
            break;
        }
        ptrs.push_back(*result);
    }
    
    EXPECT_GT(ptrs.size(), 0);
    EXPECT_GT(allocator.getStats().allocation_failures.load(), 0);
    
    // Clean up
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

TEST(BuddyAllocatorTest, Statistics) {
    BuddyAllocator allocator(64 * 1024, 64);
    
    auto result1 = allocator.allocate(128);
    ASSERT_TRUE(result1.has_value());
    
    const auto& stats = allocator.getStats();
    EXPECT_EQ(stats.total_allocations.load(), 1);
    EXPECT_GT(stats.bytes_allocated.load(), 0);
    
    allocator.deallocate(*result1);
    EXPECT_EQ(stats.total_deallocations.load(), 1);
}

TEST(BuddyAllocatorTest, Reset) {
    BuddyAllocator allocator(64 * 1024, 64);
    
    auto result = allocator.allocate(256);
    ASSERT_TRUE(result.has_value());
    
    auto reset_result = allocator.reset();
    ASSERT_TRUE(reset_result.has_value());
    
    const auto& stats = allocator.getStats();
    EXPECT_EQ(stats.total_allocations.load(), 0);
    EXPECT_EQ(stats.bytes_allocated.load(), 0);
}

TEST(BuddyAllocatorTest, CacheLineAlignment) {
    BuddyAllocator allocator(1024 * 1024, 64);
    
    auto result = allocator.allocate(256, AllocationHint::CACHE_LINE_ALIGNED);
    ASSERT_TRUE(result.has_value());
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(*result);
    EXPECT_EQ(addr % CACHE_LINE_SIZE, 0) << "Not cache line aligned";
    
    allocator.deallocate(*result);
}

// ============================================================================
// Slab Allocator Tests
// ============================================================================

TEST(SlabAllocatorTest, BasicAllocation) {
    SlabAllocator allocator(128, 64);  // 128-byte objects, 64 per slab
    
    auto result = allocator.allocate(128);
    ASSERT_TRUE(result.has_value());
    void* ptr = *result;
    ASSERT_NE(ptr, nullptr);
    
    std::memset(ptr, 0x55, 128);
    EXPECT_EQ(static_cast<uint8_t*>(ptr)[0], 0x55);
    
    auto dealloc_result = allocator.deallocate(ptr);
    ASSERT_TRUE(dealloc_result.has_value());
}

TEST(SlabAllocatorTest, MultipleAllocations) {
    SlabAllocator allocator(64, 32);
    
    std::vector<void*> ptrs = {};

    for (int i = 0; i < 100; ++i) {
        auto result = allocator.allocate(64);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
    }
    
    EXPECT_GT(allocator.getSlabCount(), 1);  // Should have multiple slabs
    
    for (void* ptr : ptrs) {
        EXPECT_TRUE(allocator.deallocate(ptr).has_value());
    }
}

TEST(SlabAllocatorTest, SizeValidation) {
    SlabAllocator allocator(128, 64);
    
    // Request larger than slab size should fail
    auto result = allocator.allocate(256);
    EXPECT_FALSE(result.has_value());
    
    // Zero size should fail
    result = allocator.allocate(0);
    EXPECT_FALSE(result.has_value());
}

TEST(SlabAllocatorTest, Utilization) {
    SlabAllocator allocator(64, 10);
    
    EXPECT_EQ(allocator.getUtilization(), 0.0);
    
    std::vector<void*> ptrs = {};

    for (int i = 0; i < 5; ++i) {
        auto result = allocator.allocate(64);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
    }
    
    EXPECT_GT(allocator.getUtilization(), 0.0);
    EXPECT_LE(allocator.getUtilization(), 1.0);
    
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

TEST(SlabAllocatorTest, MaxSlabs) {
    SlabAllocator allocator(64, 10, 2);  // Max 2 slabs
    
    std::vector<void*> ptrs;
    
    // Fill up to max slabs
    for (int i = 0; i < 20; ++i) {
        auto result = allocator.allocate(64);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
    }
    
    // Should fail after max slabs reached
    auto result = allocator.allocate(64);
    EXPECT_FALSE(result.has_value());
    
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

TEST(SlabAllocatorTest, Reset) {
    SlabAllocator allocator(128, 32);
    
    auto result = allocator.allocate(128);
    ASSERT_TRUE(result.has_value());
    
    EXPECT_GT(allocator.getSlabCount(), 0);
    
    auto reset_result = allocator.reset();
    ASSERT_TRUE(reset_result.has_value());
    
    EXPECT_EQ(allocator.getSlabCount(), 0);
}

TEST(SlabAllocatorTest, OverflowProtection) {
    // Test that creating a slab with sizes that would overflow throws an exception
    // SIZE_MAX / 2 + 1 objects of size SIZE_MAX / 2 + 1 would overflow
    size_t large_size = SIZE_MAX / 2 + 1;
    
    // This should throw std::overflow_error during construction
    EXPECT_THROW({
        SlabAllocator allocator(large_size, large_size);
    }, std::overflow_error);
    
    // Also test with even larger values
    EXPECT_THROW({
        SlabAllocator allocator(SIZE_MAX, 2);
    }, std::overflow_error);
    
    // Test edge case: Values that don't overflow but are at the boundary
    // Use sqrt(SIZE_MAX) as a safe large value that won't overflow when squared
    size_t sqrt_max = 1ULL << (sizeof(size_t) * 4);  // Approximately sqrt(SIZE_MAX)
    EXPECT_NO_THROW({
        SlabAllocator allocator(sqrt_max, 1);
    });
    
    // Test that overflow is detected when multiplying two moderately large values
    size_t half_sqrt_max = sqrt_max / 2;
    EXPECT_THROW({
        SlabAllocator allocator(sqrt_max + 1, sqrt_max + 1);
    }, std::overflow_error);
}

// ============================================================================
// Stack Allocator Tests
// ============================================================================

TEST(StackAllocatorTest, BasicAllocation) {
    StackAllocator allocator(4096);
    
    auto result = allocator.allocate(256);
    ASSERT_TRUE(result.has_value());
    void* ptr = *result;
    ASSERT_NE(ptr, nullptr);
    
    std::memset(ptr, 0xAA, 256);
    EXPECT_EQ(static_cast<uint8_t*>(ptr)[0], 0xAA);
    
    EXPECT_GT(allocator.getCurrentOffset(), 0);
}

TEST(StackAllocatorTest, LIFODeallocation) {
    StackAllocator allocator(4096);
    
    auto result1 = allocator.allocate(128);
    auto result2 = allocator.allocate(256);
    auto result3 = allocator.allocate(512);
    
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    
    // Must deallocate in LIFO order
    EXPECT_TRUE(allocator.deallocate(*result3).has_value());
    EXPECT_TRUE(allocator.deallocate(*result2).has_value());
    EXPECT_TRUE(allocator.deallocate(*result1).has_value());
}

TEST(StackAllocatorTest, NonLIFOFails) {
    StackAllocator allocator(4096);
    
    auto result1 = allocator.allocate(128);
    auto result2 = allocator.allocate(256);
    
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    
    // Try to deallocate out of order
    auto dealloc_result = allocator.deallocate(*result1);
    EXPECT_FALSE(dealloc_result.has_value());
}

TEST(StackAllocatorTest, SaveRestore) {
    StackAllocator allocator(4096);
    
    auto result1 = allocator.allocate(128);
    ASSERT_TRUE(result1.has_value());
    
    size_t pos = allocator.savePosition();
    
    auto result2 = allocator.allocate(256);
    auto result3 = allocator.allocate(512);
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    
    // Restore to saved position (frees result2 and result3)
    auto restore_result = allocator.restorePosition(pos);
    EXPECT_TRUE(restore_result.has_value());
    
    EXPECT_EQ(allocator.getCurrentOffset(), pos);
}

TEST(StackAllocatorTest, Exhaustion) {
    StackAllocator allocator(1024);
    
    std::vector<void*> ptrs;
    
    while (true) {
        auto result = allocator.allocate(128);
        if (!result.has_value()) {
            break;
        }
        ptrs.push_back(*result);
    }
    
    EXPECT_GT(ptrs.size(), 0);
    EXPECT_EQ(allocator.getAvailableSpace(), 0);
}

TEST(StackAllocatorTest, Reset) {
    StackAllocator allocator(4096);
    
    allocator.allocate(256);
    allocator.allocate(512);
    
    EXPECT_GT(allocator.getCurrentOffset(), 0);
    
    auto reset_result = allocator.reset();
    ASSERT_TRUE(reset_result.has_value());
    
    EXPECT_EQ(allocator.getCurrentOffset(), 0);
    EXPECT_EQ(allocator.getAvailableSpace(), 4096);
}

// ============================================================================
// Pool Allocator Tests
// ============================================================================

TEST(PoolAllocatorTest, BasicAllocation) {
    PoolAllocator pool;
    
    auto result = pool.allocate(256);
    ASSERT_TRUE(result.has_value());
    void* ptr = *result;
    ASSERT_NE(ptr, nullptr);
    
    std::memset(ptr, 0xBB, 256);
    EXPECT_EQ(static_cast<uint8_t*>(ptr)[0], 0xBB);
    
    auto dealloc_result = pool.deallocate(ptr);
    ASSERT_TRUE(dealloc_result.has_value());
}

TEST(PoolAllocatorTest, MixedSizes) {
    PoolAllocator pool;
    
    std::vector<std::pair<void*, size_t>> allocations;
    std::vector<size_t> sizes = {64, 128, 256, 512, 1024, 2048};
    
    for (size_t size : sizes) {
        auto result = pool.allocate(size);
        ASSERT_TRUE(result.has_value());
        allocations.push_back({*result, size});
    }
    
    for (auto [ptr, size] : allocations) {
        EXPECT_TRUE(pool.deallocate(ptr).has_value());
    }
}

TEST(PoolAllocatorTest, ShortLivedHint) {
    PoolAllocator pool;
    
    // Allocate with SHORT_LIVED hint (should use stack allocator)
    auto result = pool.allocate(256, AllocationHint::SHORT_LIVED);
    ASSERT_TRUE(result.has_value());
    
    const auto& stack_stats = pool.getStackStats();
    EXPECT_GT(stack_stats.total_allocations.load(), 0);
    
    pool.deallocate(*result);
}

TEST(PoolAllocatorTest, CombinedStatistics) {
    PoolAllocator pool;
    
    // Allocate from different pools
    auto result1 = pool.allocate(64);   // Slab
    auto result2 = pool.allocate(5000); // Buddy
    auto result3 = pool.allocate(128, AllocationHint::SHORT_LIVED);  // Stack
    
    ASSERT_TRUE(result1.has_value());
    ASSERT_TRUE(result2.has_value());
    ASSERT_TRUE(result3.has_value());
    
    auto stats = pool.getCombinedStats();
    EXPECT_GE(stats.total_allocations.load(), 3);
    
    pool.deallocate(*result1);
    pool.deallocate(*result2);
    pool.deallocate(*result3);
}

TEST(PoolAllocatorTest, CustomConfiguration) {
    PoolAllocator::Config config;
    config.buddy_pool_size = 256 * 1024;
    config.slab_sizes = {32, 64, 128};
    config.stack_capacity = 512;
    
    PoolAllocator pool(config);
    
    auto result = pool.allocate(32);
    ASSERT_TRUE(result.has_value());
    
    pool.deallocate(*result);
}

TEST(PoolAllocatorTest, Reset) {
    PoolAllocator pool;
    
    pool.allocate(256);
    pool.allocate(512);
    pool.allocate(1024);
    
    auto stats_before = pool.getCombinedStats();
    EXPECT_GT(stats_before.total_allocations.load(), 0);
    
    auto reset_result = pool.reset();
    ASSERT_TRUE(reset_result.has_value());
}

// ============================================================================
// Concurrent Access Tests
// ============================================================================

TEST(PoolAllocatorTest, ConcurrentAllocations) {
    PoolAllocator pool;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};
    
    auto worker = [&]() {
        for (int i = 0; i < 100; ++i) {
            auto result = pool.allocate(128);
            if (result.has_value()) {
                success_count++;
                // Use the memory
                std::memset(*result, 0xCC, 128);
                pool.deallocate(*result);
            } else {
                failure_count++;
            }
        }
    };
    
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(success_count.load(), 0);
}

TEST(BuddyAllocatorTest, ConcurrentAccess) {
    BuddyAllocator allocator(1024 * 1024, 64);
    std::atomic<int> success_count{0};
    
    auto worker = [&]() {
        for (int i = 0; i < 50; ++i) {
            auto result = allocator.allocate(256);
            if (result.has_value()) {
                success_count++;
                allocator.deallocate(*result);
            }
        }
    };
    
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(success_count.load(), 0);
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(PoolAllocatorTest, AllocationLatency) {
    PoolAllocator pool;
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = pool.allocate(128);
        ASSERT_TRUE(result.has_value());
        pool.deallocate(*result);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_latency = static_cast<double>(duration.count()) / iterations;
    
    // Target: <1µs per allocation (though this is just a sanity check)
    EXPECT_LT(avg_latency, 10.0) << "Average latency: " << avg_latency << "µs";
}

TEST(SlabAllocatorTest, AllocationLatency) {
    SlabAllocator allocator(128, 256);
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<void*> ptrs = {};

    for (int i = 0; i < iterations; ++i) {
        auto result = allocator.allocate(128);
        ASSERT_TRUE(result.has_value());
        ptrs.push_back(*result);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_latency = static_cast<double>(duration.count()) / iterations;
    
    // Slab should be very fast (O(1))
    EXPECT_LT(avg_latency, 5.0) << "Average latency: " << avg_latency << "µs";
    
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
}

TEST(StackAllocatorTest, AllocationLatency) {
    StackAllocator allocator(1024 * 1024);
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto result = allocator.allocate(128);
        ASSERT_TRUE(result.has_value());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double avg_latency = static_cast<double>(duration.count()) / iterations;
    
    // Stack should be fastest (O(1) pointer bump)
    EXPECT_LT(avg_latency, 2.0) << "Average latency: " << avg_latency << "µs";
}
