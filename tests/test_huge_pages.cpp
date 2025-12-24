// Test for huge pages support
// Tests memory allocation with huge pages optimization

#include <performance/huge_pages.h>
#include <gtest/gtest.h>
#include <cstring>
#include <iostream>

using namespace themis::memory;

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
                      page_size == 2 * 1024 * 1024 ||  // 2MB
                      page_size == 1024 * 1024 * 1024); // 1GB
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
