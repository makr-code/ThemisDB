// Test for huge pages support
// Tests memory allocation with huge pages optimization

#include <performance/huge_pages.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstring>
#include <iostream>

using namespace themis::memory;

// Test constants
namespace {
    constexpr size_t PAGE_SIZE_2MB = 2 * 1024 * 1024;
    constexpr size_t PAGE_SIZE_1GB = 1024 * 1024 * 1024;
    constexpr size_t ONE_TB = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    constexpr int CONCURRENT_TEST_TOKEN_COUNT = 10;
}

TEST(HugePagesTest, AvailabilityCheck) {
    // Test if huge pages are available
    bool available = huge_pages_available();
    
    #ifdef THEMIS_USE_HUGE_PAGES
    // If compiled with huge pages, we should get a status
    std::cout << "Huge pages availability: " << (available ? "YES" : "NO") << std::endl;
    #else
    // If not compiled with huge pages, should return false
    EXPECT_FALSE(available);
    #endif
}

TEST(HugePagesTest, PageSizeQuery) {
    size_t page_size = get_huge_page_size();
    
    #ifdef THEMIS_USE_HUGE_PAGES
    // Should return a valid huge page size (typically 2MB)
    std::cout << "Huge page size: " << page_size << " bytes ("
              << (page_size / (1024 * 1024)) << " MB)" << std::endl;
    
    // Common huge page sizes
    bool valid_size = (page_size == 0 ||  // Not available
                      page_size == PAGE_SIZE_2MB ||
                      page_size == PAGE_SIZE_1GB);
    EXPECT_TRUE(valid_size);
    #else
    // Without compile-time support, should return 0
    EXPECT_EQ(page_size, 0);
    #endif
}

TEST(HugePagesTest, EnabledCheck) {
    bool enabled = is_huge_pages_enabled();
    
    #ifdef THEMIS_USE_HUGE_PAGES
    EXPECT_TRUE(enabled);
    #else
    EXPECT_FALSE(enabled);
    #endif
}

TEST(HugePagesTest, StatusString) {
    std::string status = huge_pages_status();
    ASSERT_FALSE(status.empty());
    
    std::cout << "Huge pages status: " << status << std::endl;
    
    // Should contain one of the expected status strings
    bool valid_status = (status.find("disabled") != std::string::npos ||
                        status.find("enabled") != std::string::npos ||
                        status.find("unavailable") != std::string::npos);
    EXPECT_TRUE(valid_status);
}

TEST(HugePagesTest, AllocationAttempt) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Try to allocate 4MB (2 huge pages)
    size_t size = 4 * 1024 * 1024;
    void* ptr = allocate_huge_pages(size);
    
    if (ptr == nullptr) {
        // This is OK - huge pages might not be configured
        std::cout << "Warning: Huge pages allocation failed (system not configured?)" << std::endl;
        GTEST_SKIP() << "Huge pages allocation failed - system configuration needed";
    }
    
    // If allocation succeeded, test we can use it
    ASSERT_NE(ptr, nullptr);
    
    // Write some data
    std::memset(ptr, 0x42, size);
    
    // Verify we can read it
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    EXPECT_EQ(bytes[0], 0x42);
    EXPECT_EQ(bytes[size - 1], 0x42);
    
    // Free
    deallocate_huge_pages(ptr, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, LargeAllocation) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Try to allocate 64MB (32 huge pages)
    size_t size = 64 * 1024 * 1024;
    void* ptr = allocate_huge_pages(size);
    
    if (ptr == nullptr) {
        std::cout << "Warning: Large huge pages allocation failed" << std::endl;
        GTEST_SKIP() << "Large allocation failed - not enough huge pages?";
    }
    
    ASSERT_NE(ptr, nullptr);
    
    // Write pattern at start and end
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    bytes[0] = 0xAA;
    bytes[size / 2] = 0xBB;
    bytes[size - 1] = 0xCC;
    
    EXPECT_EQ(bytes[0], 0xAA);
    EXPECT_EQ(bytes[size / 2], 0xBB);
    EXPECT_EQ(bytes[size - 1], 0xCC);
    
    deallocate_huge_pages(ptr, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, NullDeallocation) {
    // Deallocating null should be safe
    EXPECT_NO_THROW(deallocate_huge_pages(nullptr, 1024));
}

TEST(HugePagesTest, SetupInstructions) {
    std::string instructions = get_huge_pages_setup_instructions(1024);
    
    ASSERT_FALSE(instructions.empty());
    std::cout << "\n" << instructions << std::endl;
    
    // Should mention configuration steps
    bool has_info = (instructions.find("enable") != std::string::npos ||
                    instructions.find("Enable") != std::string::npos);
    EXPECT_TRUE(has_info);
}

TEST(HugePagesTest, MultipleAllocations) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Try multiple smaller allocations
    std::vector<void*> ptrs;
    size_t alloc_size = 4 * 1024 * 1024; // 4MB each
    
    for (int i = 0; i < 5; ++i) {
        void* ptr = allocate_huge_pages(alloc_size);
        if (ptr) {
            ptrs.push_back(ptr);
        } else {
            // Some allocations might fail - that's OK
            break;
        }
    }
    
    if (ptrs.empty()) {
        GTEST_SKIP() << "No huge pages allocations succeeded";
    }
    
    std::cout << "Successfully allocated " << ptrs.size() 
              << " huge page regions" << std::endl;
    
    // Free all
    for (void* ptr : ptrs) {
        deallocate_huge_pages(ptr, alloc_size);
    }
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

// ============================================================================
// Allocation Tests (New)
// ============================================================================

TEST(HugePagesTest, Allocate2MBHugePages) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Allocate exactly 2MB (standard huge page size)
    size_t size_2mb = PAGE_SIZE_2MB;
    void* ptr = allocate_huge_pages(size_2mb);
    
    if (ptr == nullptr) {
        std::cout << "Warning: 2MB huge page allocation failed" << std::endl;
        GTEST_SKIP() << "2MB allocation failed - system not configured?";
    }
    
    ASSERT_NE(ptr, nullptr);
    
    // Write and verify pattern across the entire 2MB
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    for (size_t i = 0; i < size_2mb; i += 4096) {
        bytes[i] = static_cast<unsigned char>(i % 256);
    }
    
    for (size_t i = 0; i < size_2mb; i += 4096) {
        EXPECT_EQ(bytes[i], static_cast<unsigned char>(i % 256));
    }
    
    deallocate_huge_pages(ptr, size_2mb);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, Allocate1GBHugePages) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Try to allocate 1GB huge page (may not be supported on all systems)
    size_t size_1gb = PAGE_SIZE_1GB;
    void* ptr = allocate_huge_pages(size_1gb);
    
    if (ptr == nullptr) {
        // 1GB huge pages require special CPU and kernel support
        std::cout << "Note: 1GB huge pages not available (this is normal)" << std::endl;
        GTEST_SKIP() << "1GB huge pages not supported on this system";
    }
    
    // If allocation succeeded, verify we can use it
    ASSERT_NE(ptr, nullptr);
    
    // Write pattern at key locations
    unsigned char* bytes = static_cast<unsigned char*>(ptr);
    bytes[0] = 0xAA;
    bytes[size_1gb / 4] = 0xBB;
    bytes[size_1gb / 2] = 0xCC;
    bytes[size_1gb - 1] = 0xDD;
    
    EXPECT_EQ(bytes[0], 0xAA);
    EXPECT_EQ(bytes[size_1gb / 4], 0xBB);
    EXPECT_EQ(bytes[size_1gb / 2], 0xCC);
    EXPECT_EQ(bytes[size_1gb - 1], 0xDD);
    
    deallocate_huge_pages(ptr, size_1gb);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, HugePagesWithFallback) {
    #ifdef THEMIS_USE_HUGE_PAGES
    // This tests that the system gracefully handles huge pages allocation
    // even when huge pages might not be available (fallback behavior)
    
    size_t size = 8 * 1024 * 1024; // 8MB
    void* ptr = allocate_huge_pages(size);
    
    if (ptr != nullptr) {
        // Allocation succeeded (either huge pages or fallback)
        // Verify memory is usable
        std::memset(ptr, 0x55, size);
        unsigned char* bytes = static_cast<unsigned char*>(ptr);
        EXPECT_EQ(bytes[0], 0x55);
        EXPECT_EQ(bytes[size - 1], 0x55);
        
        deallocate_huge_pages(ptr, size);
    } else {
        // If allocation completely failed, that's also a valid result
        std::cout << "Note: Huge pages allocation with fallback failed" << std::endl;
    }
    
    #else
    // Without huge pages support, allocation should return nullptr
    void* ptr = allocate_huge_pages(1024);
    EXPECT_EQ(ptr, nullptr);
    #endif
}

TEST(HugePagesTest, AllocationFailureHandling) {
    #ifdef THEMIS_USE_HUGE_PAGES
    // Test allocation of unreasonably large size
    void* ptr = allocate_huge_pages(ONE_TB);
    
    // Should gracefully fail and return nullptr
    EXPECT_EQ(ptr, nullptr);
    
    // Test zero-size allocation
    ptr = allocate_huge_pages(0);
    // Behavior for zero-size is implementation-defined
    if (ptr != nullptr) {
        deallocate_huge_pages(ptr, 0);
    }
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

// ============================================================================
// Performance Tests (New)
// ============================================================================

TEST(HugePagesTest, MemoryAccessPerformance) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    size_t size = 64 * 1024 * 1024; // 64MB
    void* ptr = allocate_huge_pages(size);
    
    if (ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate memory for performance test";
    }
    
    // Perform memory access pattern typical in database workloads
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile uint64_t* data = static_cast<uint64_t*>(ptr);
    size_t num_elements = size / sizeof(uint64_t);
    
    // Sequential write
    for (size_t i = 0; i < num_elements; i++) {
        data[i] = i;
    }
    
    // Random access pattern
    uint64_t sum = 0;
    for (size_t i = 0; i < num_elements; i += 1024) {
        sum += data[i];
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Memory access performance: " << duration.count() << " μs" << std::endl;
    std::cout << "Checksum: " << sum << std::endl;
    
    // Just verify it completed without crash
    EXPECT_GT(duration.count(), 0);
    
    deallocate_huge_pages(ptr, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, TLBHitRateImprovement) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Allocate memory with huge pages
    size_t size = 32 * 1024 * 1024; // 32MB
    void* ptr_huge = allocate_huge_pages(size);
    
    if (ptr_huge == nullptr) {
        GTEST_SKIP() << "Could not allocate huge pages for TLB test";
    }
    
    // Access pattern that would benefit from reduced TLB misses
    volatile char* data = static_cast<char*>(ptr_huge);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Touch one byte per page (4KB stride)
    for (size_t i = 0; i < size; i += 4096) {
        data[i] = 1;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_huge = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "TLB test with huge pages: " << duration_huge.count() << " ns" << std::endl;
    
    // The test itself just verifies the operation completes
    // Actual TLB hit rate would require hardware performance counters
    EXPECT_GT(duration_huge.count(), 0);
    
    deallocate_huge_pages(ptr_huge, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, QueryPerformanceImpact) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Simulate a query workload: scanning through data
    size_t size = 64 * 1024 * 1024; // 64MB buffer
    void* ptr = allocate_huge_pages(size);
    
    if (ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate memory for query test";
    }
    
    // Initialize with test data
    uint64_t* data = static_cast<uint64_t*>(ptr);
    size_t num_elements = size / sizeof(uint64_t);
    
    for (size_t i = 0; i < num_elements; i++) {
        data[i] = i % 1000;
    }
    
    // Simulate query: count elements matching a condition
    auto start = std::chrono::high_resolution_clock::now();
    
    size_t count = 0;
    for (size_t i = 0; i < num_elements; i++) {
        if (data[i] < 100) {
            count++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Query scan time: " << duration.count() << " μs" << std::endl;
    std::cout << "Matching elements: " << count << std::endl;
    
    EXPECT_GT(count, 0);
    EXPECT_GT(duration.count(), 0);
    
    deallocate_huge_pages(ptr, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, ThroughputImprovement) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Measure throughput with huge pages
    size_t size = 128 * 1024 * 1024; // 128MB
    void* ptr = allocate_huge_pages(size);
    
    if (ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate memory for throughput test";
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Write throughput test
    std::memset(ptr, 0xFF, size);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double throughput_mb_s = (size / (1024.0 * 1024.0)) / (duration.count() / 1000.0);
    
    std::cout << "Write throughput: " << throughput_mb_s << " MB/s" << std::endl;
    
    EXPECT_GT(throughput_mb_s, 0);
    
    deallocate_huge_pages(ptr, size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

// ============================================================================
// Memory Management Tests (New)
// ============================================================================

TEST(HugePagesTest, MemoryTracking) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Track multiple allocations
    struct Allocation {
        void* ptr;
        size_t size;
    };
    
    std::vector<Allocation> allocations;
    std::vector<size_t> sizes = {
        2 * 1024 * 1024,   // 2MB
        4 * 1024 * 1024,   // 4MB
        8 * 1024 * 1024,   // 8MB
    };
    
    size_t total_allocated = 0;
    
    for (auto size : sizes) {
        void* ptr = allocate_huge_pages(size);
        if (ptr != nullptr) {
            allocations.push_back({ptr, size});
            total_allocated += size;
        }
    }
    
    if (allocations.empty()) {
        GTEST_SKIP() << "No allocations succeeded";
    }
    
    std::cout << "Total allocated: " << (total_allocated / (1024 * 1024)) 
              << " MB across " << allocations.size() << " regions" << std::endl;
    
    EXPECT_GT(total_allocated, 0);
    
    // Clean up all allocations
    for (const auto& allocation : allocations) {
        deallocate_huge_pages(allocation.ptr, allocation.size);
    }
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, FragmentationHandling) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Test allocation/deallocation pattern that could cause fragmentation
    std::vector<void*> ptrs;
    size_t alloc_size = 2 * 1024 * 1024; // 2MB
    
    // Allocate several regions
    for (int i = 0; i < 5; i++) {
        void* ptr = allocate_huge_pages(alloc_size);
        if (ptr != nullptr) {
            ptrs.push_back(ptr);
        }
    }
    
    if (ptrs.size() < 3) {
        // Clean up and skip
        for (void* ptr : ptrs) {
            deallocate_huge_pages(ptr, alloc_size);
        }
        GTEST_SKIP() << "Not enough allocations succeeded";
    }
    
    // Free every other allocation (creates gaps)
    for (size_t i = 0; i < ptrs.size(); i += 2) {
        deallocate_huge_pages(ptrs[i], alloc_size);
        ptrs[i] = nullptr;
    }
    
    // Try to allocate in the gaps
    void* new_ptr = allocate_huge_pages(alloc_size);
    if (new_ptr != nullptr) {
        deallocate_huge_pages(new_ptr, alloc_size);
    }
    
    // Clean up remaining allocations
    for (void* ptr : ptrs) {
        if (ptr != nullptr) {
            deallocate_huge_pages(ptr, alloc_size);
        }
    }
    
    // Test just verifies no crashes occurred
    SUCCEED();
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, MemoryPressureScenarios) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Test allocation under memory pressure
    std::vector<void*> allocations;
    size_t alloc_size = 8 * 1024 * 1024; // 8MB each
    size_t max_attempts = 20;
    
    // Keep allocating until we fail
    for (size_t i = 0; i < max_attempts; i++) {
        void* ptr = allocate_huge_pages(alloc_size);
        if (ptr == nullptr) {
            // Expected to fail at some point
            break;
        }
        allocations.push_back(ptr);
    }
    
    std::cout << "Allocated " << allocations.size() 
              << " regions before memory pressure" << std::endl;
    
    // Clean up all allocations
    for (void* ptr : allocations) {
        deallocate_huge_pages(ptr, alloc_size);
    }
    
    // After cleanup, we should be able to allocate again
    void* ptr = allocate_huge_pages(alloc_size);
    if (ptr != nullptr) {
        deallocate_huge_pages(ptr, alloc_size);
    }
    
    SUCCEED();
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, HugePagesDeallocation) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Test various deallocation scenarios
    size_t size = 4 * 1024 * 1024;
    
    // Normal deallocation
    void* ptr1 = allocate_huge_pages(size);
    if (ptr1 != nullptr) {
        deallocate_huge_pages(ptr1, size);
    }
    
    // Double free should be safe (though not recommended)
    // Our implementation checks for nullptr
    deallocate_huge_pages(nullptr, size);
    
    // Allocate and deallocate multiple times
    for (int i = 0; i < 3; i++) {
        void* ptr = allocate_huge_pages(size);
        if (ptr != nullptr) {
            deallocate_huge_pages(ptr, size);
        }
    }
    
    SUCCEED();
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

// ============================================================================
// Configuration Tests (New)
// ============================================================================

TEST(HugePagesTest, ConfigurationValidation) {
    // Test configuration query functions
    bool enabled = is_huge_pages_enabled();
    
    #ifdef THEMIS_USE_HUGE_PAGES
    EXPECT_TRUE(enabled);
    #else
    EXPECT_FALSE(enabled);
    #endif
    
    // Test page size query
    constexpr size_t PAGE_SIZE_2MB = 2 * 1024 * 1024;
    constexpr size_t PAGE_SIZE_1GB = 1024 * 1024 * 1024;
    
    size_t page_size = get_huge_page_size();
    
    #ifdef THEMIS_USE_HUGE_PAGES
    // Should return a valid page size (2MB, 1GB, or 0 if unavailable)
    bool valid_size = (page_size == 0 || 
                      page_size == PAGE_SIZE_2MB ||
                      page_size == PAGE_SIZE_1GB);
    EXPECT_TRUE(valid_size);
    #else
    EXPECT_EQ(page_size, 0);
    #endif
    
    // Test status string
    std::string status = huge_pages_status();
    EXPECT_FALSE(status.empty());
    
    std::cout << "Configuration: " << status << std::endl;
}

TEST(HugePagesTest, RuntimeEnableDisable) {
    // Test runtime query of huge pages status
    // Note: Actual runtime enable/disable would require kernel support
    // This tests the query functions
    
    bool available = huge_pages_available();
    bool enabled = is_huge_pages_enabled();
    
    std::cout << "Runtime status:" << std::endl;
    std::cout << "  Enabled (compile): " << (enabled ? "yes" : "no") << std::endl;
    std::cout << "  Available (system): " << (available ? "yes" : "no") << std::endl;
    
    #ifdef THEMIS_USE_HUGE_PAGES
    // If compiled with support, enabled should be true
    EXPECT_TRUE(enabled);
    
    // Available depends on system configuration
    // Both true and false are valid
    #else
    // Without compile-time support, should be disabled and unavailable
    EXPECT_FALSE(enabled);
    EXPECT_FALSE(available);
    #endif
}

TEST(HugePagesTest, MultipleSizeConfiguration) {
    // Test support for multiple huge page sizes
    size_t page_size = get_huge_page_size();
    
    std::cout << "Configured page size: " << page_size << " bytes" << std::endl;
    
    #ifdef THEMIS_USE_HUGE_PAGES
    if (page_size > 0) {
        // Try allocation with the configured size
        void* ptr = allocate_huge_pages(page_size);
        
        if (ptr != nullptr) {
            // Verify usability
            std::memset(ptr, 0x99, std::min(page_size, size_t(1024)));
            deallocate_huge_pages(ptr, page_size);
        }
    }
    #endif
    
    // Test is informational; actual multi-size support is platform-dependent
    SUCCEED();
}

// ============================================================================
// Integration Tests (New)
// ============================================================================

TEST(HugePagesTest, HugePagesWithRocksDBCache) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Simulate RocksDB block cache size allocation
    size_t cache_size = 64 * 1024 * 1024; // 64MB cache
    void* cache_ptr = allocate_huge_pages(cache_size);
    
    if (cache_ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate cache memory";
    }
    
    // Simulate cache operations
    struct CacheBlock {
        uint64_t key;
        char data[4096];
    };
    
    CacheBlock* cache = static_cast<CacheBlock*>(cache_ptr);
    size_t num_blocks = cache_size / sizeof(CacheBlock);
    
    // Write some cache blocks
    for (size_t i = 0; i < std::min(num_blocks, size_t(100)); i++) {
        cache[i].key = i;
        std::memset(cache[i].data, static_cast<int>(i % 256), sizeof(cache[i].data));
    }
    
    // Verify
    EXPECT_EQ(cache[0].key, 0);
    EXPECT_EQ(cache[50].key, 50);
    
    deallocate_huge_pages(cache_ptr, cache_size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, HugePagesWithTransactionBuffer) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Simulate transaction buffer pool
    size_t buffer_size = 32 * 1024 * 1024; // 32MB buffer
    void* buffer_ptr = allocate_huge_pages(buffer_size);
    
    if (buffer_ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate buffer memory";
    }
    
    // Simulate transaction log writes
    char* buffer = static_cast<char*>(buffer_ptr);
    
    for (size_t i = 0; i < 1000; i++) {
        size_t offset = (i * 1024) % buffer_size;
        if (offset + 1024 <= buffer_size) {
            std::memset(buffer + offset, static_cast<int>(i % 256), 1024);
        }
    }
    
    // Verify some writes
    EXPECT_EQ(buffer[0], 0);
    EXPECT_EQ(buffer[1024], 1);
    
    deallocate_huge_pages(buffer_ptr, buffer_size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, HugePagesWithIndexStructures) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Simulate index structure (e.g., B-tree nodes)
    size_t index_size = 16 * 1024 * 1024; // 16MB for index
    void* index_ptr = allocate_huge_pages(index_size);
    
    if (index_ptr == nullptr) {
        GTEST_SKIP() << "Could not allocate index memory";
    }
    
    // Simulate B-tree node structure
    struct BTreeNode {
        uint64_t keys[64];
        uint64_t children[65];
        int num_keys;
    };
    
    BTreeNode* nodes = static_cast<BTreeNode*>(index_ptr);
    size_t num_nodes = index_size / sizeof(BTreeNode);
    
    // Initialize some nodes
    for (size_t i = 0; i < std::min(num_nodes, size_t(100)); i++) {
        nodes[i].num_keys = static_cast<int>(i % 64);
        for (int j = 0; j < nodes[i].num_keys; j++) {
            nodes[i].keys[j] = i * 100 + j;
        }
    }
    
    // Verify
    EXPECT_EQ(nodes[1].num_keys, 1);
    EXPECT_EQ(nodes[10].keys[0], 1000);
    
    deallocate_huge_pages(index_ptr, index_size);
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}

TEST(HugePagesTest, MultiNUMAHugePagesAllocation) {
    #ifdef THEMIS_USE_HUGE_PAGES
    if (!huge_pages_available()) {
        GTEST_SKIP() << "Huge pages not available on this system";
    }
    
    // Test allocation that could span multiple NUMA nodes
    // In practice, NUMA allocation would use numa_alloc_* APIs
    // This test simulates multi-region allocation
    
    std::vector<void*> numa_regions;
    size_t region_size = 8 * 1024 * 1024; // 8MB per region
    size_t num_regions = 4; // Simulate 4 NUMA nodes
    
    for (size_t i = 0; i < num_regions; i++) {
        void* ptr = allocate_huge_pages(region_size);
        if (ptr != nullptr) {
            numa_regions.push_back(ptr);
            
            // Touch memory in this region
            std::memset(ptr, static_cast<int>(i), region_size);
        }
    }
    
    if (numa_regions.empty()) {
        GTEST_SKIP() << "No allocations succeeded";
    }
    
    std::cout << "Allocated " << numa_regions.size() 
              << " NUMA-style regions" << std::endl;
    
    // Verify each region
    for (size_t i = 0; i < numa_regions.size(); i++) {
        unsigned char* bytes = static_cast<unsigned char*>(numa_regions[i]);
        EXPECT_EQ(bytes[0], static_cast<unsigned char>(i));
    }
    
    // Clean up
    for (void* ptr : numa_regions) {
        deallocate_huge_pages(ptr, region_size);
    }
    
    #else
    GTEST_SKIP() << "Huge pages not enabled at compile time";
    #endif
}
