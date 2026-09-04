// ThemisDB Alignment Helpers Test Suite
// Tests compile-time and runtime alignment verification

#include <gtest/gtest.h>
#include "performance/alignment_helpers.h"
#include "utils/unaligned_access.h"
#include <vector>
#include <cstdint>

using namespace themis::performance;
using namespace themis::utils;

// ============================================================================
// Compile-time Alignment Checks
// ============================================================================

// Test structures with various alignments
struct alignas(16) Aligned16 {
    uint64_t a = 0;
    uint64_t b;
};

struct alignas(32) Aligned32 {
    uint64_t data[4];
};

struct alignas(64) Aligned64 {
    uint64_t data[8];
};

struct alignas(8) Aligned8 {
    uint64_t value = 0;
};

// These assertions should compile successfully
THEMIS_STATIC_ASSERT_ALIGNED(Aligned16, 16);
THEMIS_STATIC_ASSERT_ALIGNED(Aligned32, 32);
THEMIS_STATIC_ASSERT_ALIGNED(Aligned64, 64);
THEMIS_STATIC_ASSERT_ALIGNED(Aligned8, 8);

// Test size assertions
THEMIS_STATIC_ASSERT_SIZE(Aligned16, 16);
THEMIS_STATIC_ASSERT_SIZE(Aligned8, 8);

// Test minimum alignment
THEMIS_STATIC_ASSERT_MIN_ALIGNED(Aligned64, 32);  // 64 >= 32
THEMIS_STATIC_ASSERT_MIN_ALIGNED(Aligned32, 16);  // 32 >= 16

TEST(AlignmentHelpersTest, CompileTimeAlignmentCheck) {
    // Test check_alignment template function
    EXPECT_TRUE((check_alignment<Aligned16, 16>()));
    EXPECT_FALSE((check_alignment<Aligned16, 32>()));
    
    EXPECT_TRUE((check_alignment<Aligned32, 32>()));
    EXPECT_FALSE((check_alignment<Aligned32, 16>()));
    
    EXPECT_TRUE((check_alignment<Aligned64, 64>()));
    EXPECT_FALSE((check_alignment<Aligned64, 32>()));
}

TEST(AlignmentHelpersTest, CompileTimeMinAlignmentCheck) {
    // Test check_min_alignment template function
    EXPECT_TRUE((check_min_alignment<Aligned64, 32>()));
    EXPECT_TRUE((check_min_alignment<Aligned64, 64>()));
    EXPECT_FALSE((check_min_alignment<Aligned64, 128>()));
    
    EXPECT_TRUE((check_min_alignment<Aligned16, 8>()));
    EXPECT_TRUE((check_min_alignment<Aligned16, 16>()));
    EXPECT_FALSE((check_min_alignment<Aligned16, 32>()));
}

// ============================================================================
// Runtime Pointer Alignment Checks
// ============================================================================

TEST(AlignmentHelpersTest, IsAlignedWithAlignedPointer) {
    alignas(64) char buffer[128];
    void* ptr = buffer;
    
    // Buffer is 64-byte aligned
    EXPECT_TRUE(is_aligned<64>(ptr));
    EXPECT_TRUE(is_aligned<32>(ptr));
    EXPECT_TRUE(is_aligned<16>(ptr));
    EXPECT_TRUE(is_aligned<8>(ptr));
    EXPECT_TRUE(is_aligned<4>(ptr));
    EXPECT_TRUE(is_aligned<2>(ptr));
    EXPECT_TRUE(is_aligned<1>(ptr));
}

TEST(AlignmentHelpersTest, IsAlignedWithUnalignedPointer) {
    alignas(64) char buffer[128];
    
    // Test misaligned pointers
    EXPECT_FALSE(is_aligned<64>(buffer + 1));
    EXPECT_FALSE(is_aligned<32>(buffer + 1));
    EXPECT_FALSE(is_aligned<16>(buffer + 1));
    
    // 16-byte offset from 64-byte aligned is 16-byte aligned
    EXPECT_TRUE(is_aligned<16>(buffer + 16));
    EXPECT_FALSE(is_aligned<32>(buffer + 16));
}

TEST(AlignmentHelpersTest, IsAlignedEdgeCases) {
    alignas(16) char buffer[64];
    
    // 8-byte offset
    EXPECT_TRUE(is_aligned<8>(buffer + 8));
    EXPECT_FALSE(is_aligned<16>(buffer + 8));
    
    // 4-byte offset
    EXPECT_TRUE(is_aligned<4>(buffer + 4));
    EXPECT_FALSE(is_aligned<8>(buffer + 4));
}

// ============================================================================
// Pointer Alignment Functions
// ============================================================================

TEST(AlignmentHelpersTest, AlignUp) {
    alignas(64) char buffer[128];
    
    // Already aligned
    EXPECT_EQ(align_up<16>(buffer), buffer);
    EXPECT_EQ(align_up<32>(buffer), buffer);
    EXPECT_EQ(align_up<64>(buffer), buffer);
    
    // Align up misaligned pointers
    void* ptr1 = buffer + 1;
    void* aligned1 = align_up<16>(ptr1);
    EXPECT_TRUE(is_aligned<16>(aligned1));
    EXPECT_GE(aligned1, ptr1);
    EXPECT_EQ(aligned1, buffer + 16);
    
    void* ptr5 = buffer + 5;
    void* aligned5 = align_up<8>(ptr5);
    EXPECT_TRUE(is_aligned<8>(aligned5));
    EXPECT_GE(aligned5, ptr5);
    EXPECT_EQ(aligned5, buffer + 8);
}

TEST(AlignmentHelpersTest, AlignUpConst) {
    alignas(64) const char buffer[128] = {};
    
    const void* ptr1 = buffer + 1;
    const void* aligned1 = align_up<16>(ptr1);
    EXPECT_TRUE(is_aligned<16>(aligned1));
    EXPECT_GE(aligned1, ptr1);
}

TEST(AlignmentHelpersTest, AlignDown) {
    alignas(64) char buffer[128];
    
    // Already aligned
    EXPECT_EQ(align_down<16>(buffer), buffer);
    EXPECT_EQ(align_down<32>(buffer), buffer);
    EXPECT_EQ(align_down<64>(buffer), buffer);
    
    // Align down misaligned pointers
    void* ptr17 = buffer + 17;
    void* aligned17 = align_down<16>(ptr17);
    EXPECT_TRUE(is_aligned<16>(aligned17));
    EXPECT_LE(aligned17, ptr17);
    EXPECT_EQ(aligned17, buffer + 16);
    
    void* ptr50 = buffer + 50;
    void* aligned50 = align_down<32>(ptr50);
    EXPECT_TRUE(is_aligned<32>(aligned50));
    EXPECT_LE(aligned50, ptr50);
    EXPECT_EQ(aligned50, buffer + 32);
}

TEST(AlignmentHelpersTest, AlignDownConst) {
    alignas(64) const char buffer[128] = {};
    
    const void* ptr17 = buffer + 17;
    const void* aligned17 = align_down<16>(ptr17);
    EXPECT_TRUE(is_aligned<16>(aligned17));
    EXPECT_LE(aligned17, ptr17);
}

// ============================================================================
// Padding Calculation
// ============================================================================

TEST(AlignmentHelpersTest, PaddingForAlignment) {
    // Already aligned
    EXPECT_EQ((padding_for_alignment<16>(0)), 0);
    EXPECT_EQ((padding_for_alignment<16>(16)), 0);
    EXPECT_EQ((padding_for_alignment<16>(32)), 0);
    
    // Needs padding
    EXPECT_EQ((padding_for_alignment<16>(1)), 15);
    EXPECT_EQ((padding_for_alignment<16>(15)), 1);
    EXPECT_EQ((padding_for_alignment<16>(17)), 15);
    
    EXPECT_EQ((padding_for_alignment<8>(1)), 7);
    EXPECT_EQ((padding_for_alignment<8>(5)), 3);
    EXPECT_EQ((padding_for_alignment<8>(9)), 7);
}

// ============================================================================
// Unaligned Memory Access
// ============================================================================

TEST(UnalignedAccessTest, ReadUnalignedUint32) {
    alignas(16) uint8_t buffer[16] = {
        0x01, 0x02, 0x03, 0x04,  // uint32: 0x04030201 (little-endian)
        0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C,
        0x0D, 0x0E, 0x0F, 0x10
    };
    
    // Read from aligned address
    uint32_t aligned_val = read_unaligned<uint32_t>(buffer);
    EXPECT_NE(aligned_val, 0);
    
    // Read from unaligned address (offset 1)
    uint32_t unaligned_val = read_unaligned<uint32_t>(buffer + 1);
    EXPECT_NE(unaligned_val, 0);
    
    // Values should be different due to different offsets
    EXPECT_NE(aligned_val, unaligned_val);
}

TEST(UnalignedAccessTest, WriteUnalignedUint32) {
    alignas(16) uint8_t buffer[16] = {0};
    
    uint32_t test_value = 0x12345678;
    
    // Write to unaligned address
    write_unaligned<uint32_t>(buffer + 1, test_value);
    
    // Read it back
    uint32_t read_value = read_unaligned<uint32_t>(buffer + 1);
    EXPECT_EQ(read_value, test_value);
}

TEST(UnalignedAccessTest, ReadWriteUnalignedUint64) {
    alignas(16) uint8_t buffer[32] = {0};
    
    uint64_t test_value = 0x123456789ABCDEF0ULL;
    
    // Write to unaligned, non-overlapping addresses
    write_unaligned<uint64_t>(buffer + 1, test_value);
    write_unaligned<uint64_t>(buffer + 10, test_value + 1);
    write_unaligned<uint64_t>(buffer + 18, test_value + 2);
    
    // Read back
    EXPECT_EQ(read_unaligned<uint64_t>(buffer + 1), test_value);
    EXPECT_EQ(read_unaligned<uint64_t>(buffer + 10), test_value + 1);
    EXPECT_EQ(read_unaligned<uint64_t>(buffer + 18), test_value + 2);
}

TEST(UnalignedAccessTest, ReadWriteUnalignedStruct) {
    struct TestStruct {
        uint32_t a = 0;
        uint32_t b;
        uint64_t c;
    };
    
    alignas(16) uint8_t buffer[32] = {0};
    
    TestStruct test_data{0x11111111, 0x22222222, 0x3333333333333333ULL};
    
    // Write to unaligned address
    write_unaligned<TestStruct>(buffer + 3, test_data);
    
    // Read back
    TestStruct read_data = read_unaligned<TestStruct>(buffer + 3);
    EXPECT_EQ(read_data.a, test_data.a);
    EXPECT_EQ(read_data.b, test_data.b);
    EXPECT_EQ(read_data.c, test_data.c);
}

// ============================================================================
// Checked Aligned Cast
// ============================================================================

TEST(UnalignedAccessTest, CheckedAlignedCastAligned) {
    alignas(16) Aligned16 aligned_obj;
    void* ptr = &aligned_obj;
    
    // Should succeed - properly aligned
    Aligned16* cast_ptr = checked_aligned_cast<Aligned16>(ptr);
    EXPECT_NE(cast_ptr, nullptr);
    EXPECT_EQ(cast_ptr, &aligned_obj);
}

TEST(UnalignedAccessTest, CheckedAlignedCastUnaligned) {
    alignas(16) char buffer[32];
    void* ptr = buffer + 1;  // Misaligned
    
    // Should fail - not 16-byte aligned
    Aligned16* cast_ptr = checked_aligned_cast<Aligned16>(ptr);
    EXPECT_EQ(cast_ptr, nullptr);
}

TEST(UnalignedAccessTest, CheckedAlignedCastConst) {
    alignas(32) const Aligned32 aligned_obj{};
    const void* ptr = &aligned_obj;
    
    // Should succeed - properly aligned
    const Aligned32* cast_ptr = checked_aligned_cast<Aligned32>(ptr);
    EXPECT_NE(cast_ptr, nullptr);
    EXPECT_EQ(cast_ptr, &aligned_obj);
}

TEST(UnalignedAccessTest, CheckedAlignedCastEdgeCases) {
    alignas(64) char buffer[128];
    
    // Test with different alignments
    EXPECT_NE(checked_aligned_cast<Aligned8>(buffer), nullptr);
    EXPECT_NE(checked_aligned_cast<Aligned16>(buffer), nullptr);
    EXPECT_NE(checked_aligned_cast<Aligned32>(buffer), nullptr);
    EXPECT_NE(checked_aligned_cast<Aligned64>(buffer), nullptr);
    
    // 32-byte offset from 64-byte aligned buffer
    void* ptr32 = buffer + 32;
    EXPECT_NE(checked_aligned_cast<Aligned8>(ptr32), nullptr);
    EXPECT_NE(checked_aligned_cast<Aligned16>(ptr32), nullptr);
    EXPECT_NE(checked_aligned_cast<Aligned32>(ptr32), nullptr);
    EXPECT_EQ(checked_aligned_cast<Aligned64>(ptr32), nullptr);  // Not 64-aligned
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(AlignmentHelpersTest, CacheLineAlignment) {
    // Simulate cache-line aligned structure
    struct alignas(64) CacheLineCounter {
        uint64_t counter = 0;
        char padding[56];
    };
    
    THEMIS_STATIC_ASSERT_ALIGNED(CacheLineCounter, 64);
    THEMIS_STATIC_ASSERT_SIZE(CacheLineCounter, 64);
    
    CacheLineCounter counter{};
    EXPECT_TRUE(is_aligned<64>(&counter));
}

TEST(AlignmentHelpersTest, SIMDAlignment) {
    // Simulate SIMD vector types
    struct alignas(16) Vec4f {
        float x, y, z, w;
    };
    
    THEMIS_STATIC_ASSERT_ALIGNED(Vec4f, 16);
    THEMIS_STATIC_ASSERT_SIZE(Vec4f, 16);
    
    Vec4f vec{1.0f, 2.0f, 3.0f, 4.0f};
    EXPECT_TRUE(is_aligned<16>(&vec));
}

TEST(AlignmentHelpersTest, AllocatedMemoryAlignment) {
    // Test alignment of dynamically allocated memory using RAII
    
    // Helper to manage aligned allocation/deallocation
    struct AlignedDeleter {
        size_t alignment = 0;
        void operator()(void* ptr) const {
            ::operator delete(ptr, std::align_val_t(alignment));
        }
    };
    
    // 16-byte aligned allocation
    {
        std::unique_ptr<void, AlignedDeleter> ptr16(
            ::operator new(64, std::align_val_t(16)),
            AlignedDeleter{16}
        );
        EXPECT_TRUE(is_aligned<16>(ptr16.get()));
    }
    
    // 32-byte aligned allocation
    {
        std::unique_ptr<void, AlignedDeleter> ptr32(
            ::operator new(64, std::align_val_t(32)),
            AlignedDeleter{32}
        );
        EXPECT_TRUE(is_aligned<32>(ptr32.get()));
    }
    
    // 64-byte aligned allocation
    {
        std::unique_ptr<void, AlignedDeleter> ptr64(
            ::operator new(128, std::align_val_t(64)),
            AlignedDeleter{64}
        );
        EXPECT_TRUE(is_aligned<64>(ptr64.get()));
    }
}
