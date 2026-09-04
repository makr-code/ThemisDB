// Test for performance allocator
// Tests memory allocation with optional jemalloc or mimalloc optimization

#include <performance/allocator.h>
#include <gtest/gtest.h>
#include <vector>
#include <cstring>
#include <chrono>

#include "../test_performance_helpers.h"

using namespace themis::memory;

TEST(PerformanceAllocatorTest, BasicAllocation) {
    // Test basic allocation and deallocation
    void* ptr = allocate(1024);
    ASSERT_NE(ptr, nullptr);
    
    // Write some data
    std::memset(ptr, 0x42, 1024);
    
    // Verify we can read it back
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    EXPECT_EQ(bytes[0], 0x42);
    EXPECT_EQ(bytes[1023], 0x42);
    
    deallocate(ptr);
}

TEST(PerformanceAllocatorTest, MultipleAllocations) {
    // Test multiple allocations
    std::vector<void*> ptrs = {};

    for (int i = 0; i < 100; ++i) {
        void* ptr = allocate(128);
        ASSERT_NE(ptr, nullptr);
        ptrs.push_back(ptr);
    }
    
    // Free all
    for (void* ptr : ptrs) {
        deallocate(ptr);
    }
}

TEST(PerformanceAllocatorTest, AlignedAllocation) {
    // Test aligned allocation
    void* ptr = allocate_aligned(1024, 64);
    ASSERT_NE(ptr, nullptr);
    
    // Check alignment
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    EXPECT_EQ(addr % 64, 0) << "Pointer is not 64-byte aligned";
    
    deallocate_aligned(ptr, 64);
}

TEST(PerformanceAllocatorTest, LargeAllocation) {
    // Test large allocation (10MB)
    size_t size = 10 * 1024 * 1024;
    void* ptr = allocate(size);
    ASSERT_NE(ptr, nullptr);
    
    // Write pattern
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    bytes[0] = 0xAA;
    bytes[size - 1] = 0xBB;
    
    EXPECT_EQ(bytes[0], 0xAA);
    EXPECT_EQ(bytes[size - 1], 0xBB);
    
    deallocate(ptr);
}

TEST(PerformanceAllocatorTest, NullDeallocation) {
    // Deallocating null should be safe
    EXPECT_NO_THROW(deallocate(nullptr));
    EXPECT_NO_THROW(deallocate_aligned(nullptr, 64));
}

TEST(PerformanceAllocatorTest, AllocatorInfo) {
    // Test allocator name
    const char* name = allocator_name();
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(std::strlen(name) > 0);
    
    #ifdef THEMIS_ENABLE_JEMALLOC
    EXPECT_STREQ(name, "jemalloc");
    EXPECT_TRUE(is_jemalloc_enabled());
    EXPECT_FALSE(is_mimalloc_enabled());
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    EXPECT_STREQ(name, "mimalloc");
    EXPECT_TRUE(is_mimalloc_enabled());
    EXPECT_FALSE(is_jemalloc_enabled());
    #else
    EXPECT_STREQ(name, "system");
    EXPECT_FALSE(is_mimalloc_enabled());
    EXPECT_FALSE(is_jemalloc_enabled());
    #endif
}

TEST(PerformanceAllocatorTest, PerformanceBenchmark) {
    // Simple performance test (not a real benchmark, just for smoke testing)
    const int iterations = 1000;
    const size_t alloc_size = 256;

    auto samples = themis::test::sampleLatencyMs([&]() {
        for (int i = 0; i < iterations; ++i) {
            void* ptr = allocate(alloc_size);
            ASSERT_NE(ptr, nullptr);
            deallocate(ptr);
        }
    });

    const auto p95_ms = themis::test::percentileValue(samples, 95);
    // Historical CI budget: 100ms for 1000 allocations/deallocations.
    EXPECT_LT(p95_ms, 100.0) << "Allocations took too long: p95=" << p95_ms << "ms";
    
    #ifdef THEMIS_ENABLE_JEMALLOC
    std::cout << "Jemalloc performance p95: " << p95_ms << "ms for "
              << iterations << " allocations" << std::endl;
    #elif defined(THEMIS_ENABLE_MIMALLOC)
    std::cout << "Mimalloc performance p95: " << p95_ms << "ms for "
              << iterations << " allocations" << std::endl;
    #endif
}
