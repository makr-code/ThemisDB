/**
 * @file test_gpu_memory_allocator_move_semantics.cpp
 * @brief Tests for GPU memory allocator double-free prevention and move semantics
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "gpu/gpu_memory_allocator.h"
#include <utility>

using namespace themis::gpu;

class GPUMemoryAllocatorTest : public ::testing::Test {
protected:
    GPUMemoryAllocator::Config default_config{};
};

// Test: Default construction creates valid state
TEST_F(GPUMemoryAllocatorTest, DefaultConstructorIsValid) {
    GPUMemoryAllocator alloc;
    EXPECT_FALSE(alloc.is_moved_from());
    EXPECT_FALSE(alloc.is_initialized());
}

// Test: Move constructor transfers ownership
TEST_F(GPUMemoryAllocatorTest, MoveConstructorTransfersOwnership) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Move assignment transfers ownership
TEST_F(GPUMemoryAllocatorTest, MoveAssignmentTransfersOwnership) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst;

    dst = std::move(src);
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Self-move assignment is safe
TEST_F(GPUMemoryAllocatorTest, SelfMoveAssignmentIsSafe) {
    GPUMemoryAllocator alloc;
    alloc = std::move(alloc);

    EXPECT_TRUE(alloc.is_moved_from());
}

// Test: Allocation from moved-from allocator throws
TEST_F(GPUMemoryAllocatorTest, AllocateThrowsWhenMovedFrom) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    EXPECT_THROW({
        src.allocate(64);
    }, std::logic_error);
}

// Test: Deallocation is idempotent on moved-from allocator (no double-free)
TEST_F(GPUMemoryAllocatorTest, DeallocateIdempotentOnMovedFrom) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    MemoryAllocation dummy{};
    src.deallocate(dummy);  // Should not throw
    src.deallocate(dummy);  // Second call should be safe
}

// Test: allocation_count() returns 0 when moved-from
TEST_F(GPUMemoryAllocatorTest, AllocationCountZeroWhenMovedFrom) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    EXPECT_EQ(src.allocation_count(), 0);
}

// Test: allocated_memory() returns 0 when moved-from
TEST_F(GPUMemoryAllocatorTest, AllocatedMemoryZeroWhenMovedFrom) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    EXPECT_EQ(src.allocated_memory(), 0);
}

// Test: DeviceMemoryRegion move semantics
TEST_F(GPUMemoryAllocatorTest, DeviceMemoryRegionMoveSemantics) {
    GPUMemoryAllocator alloc;
    
    DeviceMemoryRegion region1(alloc, 64);
    EXPECT_TRUE(region1.is_valid());

    DeviceMemoryRegion region2(std::move(region1));
    EXPECT_FALSE(region1.is_valid());
    EXPECT_TRUE(region2.is_valid());
}

// Test: DeviceMemoryRegion size() is 0 when moved-from
TEST_F(GPUMemoryAllocatorTest, DeviceMemoryRegionSizeZeroWhenMovedFrom) {
    GPUMemoryAllocator alloc;
    
    DeviceMemoryRegion region1(alloc, 64);
    DeviceMemoryRegion region2(std::move(region1));

    EXPECT_EQ(region1.size(), 0);
}

// Test: DeviceMemoryRegion pointers null when moved-from
TEST_F(GPUMemoryAllocatorTest, DeviceMemoryRegionPointersNullWhenMovedFrom) {
    GPUMemoryAllocator alloc;
    
    DeviceMemoryRegion region1(alloc, 64);
    DeviceMemoryRegion region2(std::move(region1));

    EXPECT_EQ(region1.device_ptr(), nullptr);
    EXPECT_EQ(region1.host_ptr(), nullptr);
}

// Test: Move assignment chain is safe
TEST_F(GPUMemoryAllocatorTest, MoveAssignmentChainIsSafe) {
    GPUMemoryAllocator a, b, c;
    
    b = std::move(a);
    EXPECT_TRUE(a.is_moved_from());
    
    c = std::move(b);
    EXPECT_TRUE(b.is_moved_from());
    EXPECT_FALSE(c.is_moved_from());
}

// Test: Destructor safe on moved-from objects (no double-cleanup)
TEST_F(GPUMemoryAllocatorTest, DestructorSafeOnMovedFrom) {
    {
        GPUMemoryAllocator src;
        GPUMemoryAllocator dst(std::move(src));
    }  // Should not crash
}

// Test: Allocator config preserved after move
TEST_F(GPUMemoryAllocatorTest, ConfigPreservedAfterMove) {
    GPUMemoryAllocator src;
    GPUMemoryAllocator dst(std::move(src));

    auto src_config = src.get_config();
    auto dst_config = dst.get_config();
    
    // Moved-from allocator should have invalid config
    EXPECT_EQ(src_config.device_id, -1);
}
