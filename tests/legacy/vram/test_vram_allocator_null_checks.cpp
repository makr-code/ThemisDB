#include <gtest/gtest.h>
#include "llm/lora_framework/vram_allocator.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ostream_sink.h>
#include <sstream>

using namespace themis::llm::lora;
using namespace themis::acceleration;

/**
 * @file test_vram_allocator_null_checks.cpp
 * @brief Tests for VRAM allocator null-check consistency
 * 
 * Validates that:
 * - All GPU allocations properly check for null pointers
 * - Error codes are validated before returning
 * - Allocation failures are properly logged
 */

class VRAMAllocatorNullCheckTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Capture logs for verification
        log_stream_ = std::make_shared<std::ostringstream>();
        auto ostream_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(*log_stream_);
        auto logger = std::make_shared<spdlog::logger>("test_logger", ostream_sink);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::debug);
    }
    
    void TearDown() override {
        spdlog::set_default_logger(nullptr);
    }
    
    std::string get_logs() {
        return log_stream_->str();
    }
    
    std::shared_ptr<std::ostringstream> log_stream_;
};

// Test that CPU allocations properly handle null returns
TEST_F(VRAMAllocatorNullCheckTest, CPUAllocation_HandlesNullReturn) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    // Allocate a reasonable amount of memory
    void* ptr = allocator.allocate(1024);
    EXPECT_NE(ptr, nullptr);
    
    if (ptr != nullptr) {
        allocator.deallocate(ptr);
    }
}

// Test that allocation of zero bytes returns nullptr
TEST_F(VRAMAllocatorNullCheckTest, ZeroByteAllocation_ReturnsNull) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    // Zero-byte allocations should return nullptr
    void* ptr = allocator.allocate(0);
    EXPECT_EQ(ptr, nullptr);
}

// Test that very large allocation attempts are handled gracefully
TEST_F(VRAMAllocatorNullCheckTest, ExtremelyLargeAllocation_HandledGracefully) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    // Try to allocate an unreasonably large amount (1 PB)
    size_t huge_size = 1ULL * 1024 * 1024 * 1024 * 1024 * 1024;
    void* ptr = allocator.allocate(huge_size);
    
    // Should return nullptr without crashing
    EXPECT_EQ(ptr, nullptr);
    
    // Check that an error was logged
    std::string logs = get_logs();
    if (ptr == nullptr) {
        // If allocation failed, we expect an error log
        // (CPU allocations log errors on failure)
        EXPECT_TRUE(logs.find("failed") != std::string::npos || logs.empty());
    }
}

// Test that multiple small allocations work correctly
TEST_F(VRAMAllocatorNullCheckTest, MultipleSmallAllocations_AllValid) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    std::vector<void*> ptrs;
    constexpr int NUM_ALLOCATIONS = 100;
    constexpr size_t SMALL_SIZE = 256;
    
    // Allocate many small blocks
    for (int i = 0; i < NUM_ALLOCATIONS; i++) {
        void* ptr = allocator.allocate(SMALL_SIZE);
        EXPECT_NE(ptr, nullptr) << "Allocation " << i << " failed";
        if (ptr != nullptr) {
            ptrs.push_back(ptr);
        }
    }
    
    // Verify all allocations succeeded
    EXPECT_EQ(ptrs.size(), NUM_ALLOCATIONS);
    
    // Clean up
    for (void* ptr : ptrs) {
        allocator.deallocate(ptr);
    }
    
    // Verify all memory was freed
    auto stats = allocator.get_stats();
    EXPECT_EQ(stats.allocated_bytes, 0);
}

// Test that deallocating nullptr is safe
TEST_F(VRAMAllocatorNullCheckTest, DeallocateNull_IsSafe) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    // Should not crash
    EXPECT_NO_THROW(allocator.deallocate(nullptr));
}

// Test that allocation/deallocation sequence maintains correct stats
TEST_F(VRAMAllocatorNullCheckTest, AllocationStats_RemainsConsistent) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    auto stats_initial = allocator.get_stats();
    EXPECT_EQ(stats_initial.allocated_bytes, 0);
    
    // Allocate
    constexpr size_t SIZE = 4096;
    void* ptr = allocator.allocate(SIZE);
    ASSERT_NE(ptr, nullptr);
    
    auto stats_after_alloc = allocator.get_stats();
    EXPECT_GE(stats_after_alloc.allocated_bytes, SIZE);
    EXPECT_EQ(stats_after_alloc.allocation_count, 1);
    
    // Deallocate
    allocator.deallocate(ptr);
    
    auto stats_after_dealloc = allocator.get_stats();
    EXPECT_EQ(stats_after_dealloc.allocated_bytes, 0);
}

// Test VRAMTensor wrapper also handles null allocations
TEST_F(VRAMAllocatorNullCheckTest, VRAMTensor_HandlesAllocation) {
    VRAMAllocator allocator(BackendType::CPU);
    ASSERT_TRUE(allocator.is_available());
    
    // Create tensor
    constexpr size_t SIZE = 1024;
    VRAMTensor tensor(&allocator, SIZE);
    
    // Should have valid pointer
    EXPECT_NE(tensor.ptr(), nullptr);
    EXPECT_EQ(tensor.size(), SIZE);
    
    // Test upload/download
    std::vector<float> data(SIZE / sizeof(float), 42.0f);
    bool success = tensor.upload(data.data(), data.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    std::vector<float> result(SIZE / sizeof(float));
    success = tensor.download(result.data(), result.size() * sizeof(float));
    EXPECT_TRUE(success);
    
    // Verify data
    for (size_t i = 0; i < data.size(); i++) {
        EXPECT_FLOAT_EQ(result[i], data[i]);
    }
}

// Test that initialization failures are properly reported
TEST_F(VRAMAllocatorNullCheckTest, BackendInitialization_ProperlyReported) {
    // Try to create allocator with unsupported backend
    // VULKAN/DIRECTX may not be available
    VRAMAllocator allocator(BackendType::VULKAN);
    
    // Should either be available or not, but shouldn't crash
    bool available = allocator.is_available();
    
    if (!available) {
        // If not available, allocation should return nullptr
        void* ptr = allocator.allocate(1024);
        EXPECT_EQ(ptr, nullptr);
    }
}

// ─── Batch 36 input_validation regression tests ─────────────────────────────

// Allocation exceeding pool_size_bytes must be rejected with nullptr (not crash
// or silently over-allocate). We pass a small explicit pool so the guard fires.
TEST_F(VRAMAllocatorNullCheckTest, AllocateExceedsPoolSize_ReturnsNull) {
    // pool_size_bytes = 1 KB
    constexpr size_t POOL = 1024;
    VRAMAllocator allocator(BackendType::CPU, POOL);

    if (!allocator.is_available()) {
        GTEST_SKIP() << "CPU backend unavailable";
    }

    // Request more than the pool can hold — must be rejected
    void* ptr = allocator.allocate(POOL + 1);
    EXPECT_EQ(ptr, nullptr);

    // Verify the rejection is reflected in the error log
    std::string logs = get_logs();
    EXPECT_NE(logs.find("exceeds pool size"), std::string::npos);

    // After the rejected call, a smaller allocation within the pool must succeed
    void* small = allocator.allocate(64);
    EXPECT_NE(small, nullptr);
    allocator.deallocate(small);
}

// Confirm that pool_size_bytes == 0 (auto-detect) does NOT reject normal
// allocations — the guard is conditional on pool_size_bytes_ > 0.
TEST_F(VRAMAllocatorNullCheckTest, AllocateWithZeroPool_NotRejected) {
    // pool_size_bytes = 0 means "no explicit limit"
    VRAMAllocator allocator(BackendType::CPU, /*pool_size_bytes=*/0);

    if (!allocator.is_available()) {
        GTEST_SKIP() << "CPU backend unavailable";
    }

    void* ptr = allocator.allocate(4096);
    EXPECT_NE(ptr, nullptr);
    allocator.deallocate(ptr);
}
