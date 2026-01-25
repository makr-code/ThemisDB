#include <gtest/gtest.h>
#include "utils/safe_cast.h"
#include <cmath>
#include <limits>

using namespace themis::utils;

// ============================================================================
// Basic safe_cast Tests
// ============================================================================

TEST(SafeCastTest, FloatToUInt32) {
    float f = 3.14f;
    uint32_t bits = safe_cast<uint32_t>(f);
    
    // Verify round-trip conversion preserves the value
    float f_back = safe_cast<float>(bits);
    EXPECT_EQ(f, f_back);
}

TEST(SafeCastTest, DoubleToUInt64) {
    double d = 3.141592653589793;
    uint64_t bits = safe_cast<uint64_t>(d);
    
    // Verify round-trip conversion preserves the value
    double d_back = safe_cast<double>(bits);
    EXPECT_EQ(d, d_back);
}

TEST(SafeCastTest, Int32ToFloat) {
    int32_t i = 0x40490FDB;  // Bit pattern for ~3.14159...
    float f = safe_cast<float>(i);
    
    // Verify we can cast back
    int32_t i_back = safe_cast<int32_t>(f);
    EXPECT_EQ(i, i_back);
}

// ============================================================================
// FloatBits Helper Tests
// ============================================================================

TEST(FloatBitsTest, FloatToU32AndBack) {
    float original = 42.5f;
    uint32_t bits = FloatBits::to_u32(original);
    float result = FloatBits::from_u32(bits);
    
    EXPECT_EQ(original, result);
}

TEST(FloatBitsTest, DoubleToU64AndBack) {
    double original = 123.456789;
    uint64_t bits = FloatBits::to_u64(original);
    double result = FloatBits::from_u64(bits);
    
    EXPECT_EQ(original, result);
}

// ============================================================================
// Special Values Tests (NaN, Infinity)
// ============================================================================

TEST(FloatBitsTest, PreservesPositiveInfinity) {
    float inf = std::numeric_limits<float>::infinity();
    uint32_t bits = FloatBits::to_u32(inf);
    float result = FloatBits::from_u32(bits);
    
    EXPECT_TRUE(std::isinf(result));
    EXPECT_GT(result, 0);
}

TEST(FloatBitsTest, PreservesNegativeInfinity) {
    float neg_inf = -std::numeric_limits<float>::infinity();
    uint32_t bits = FloatBits::to_u32(neg_inf);
    float result = FloatBits::from_u32(bits);
    
    EXPECT_TRUE(std::isinf(result));
    EXPECT_LT(result, 0);
}

TEST(FloatBitsTest, PreservesNaN) {
    float nan = std::numeric_limits<float>::quiet_NaN();
    uint32_t bits = FloatBits::to_u32(nan);
    float result = FloatBits::from_u32(bits);
    
    // NaN != NaN, so we check with isnan
    EXPECT_TRUE(std::isnan(result));
}

TEST(FloatBitsTest, PreservesDoubleInfinity) {
    double inf = std::numeric_limits<double>::infinity();
    uint64_t bits = FloatBits::to_u64(inf);
    double result = FloatBits::from_u64(bits);
    
    EXPECT_TRUE(std::isinf(result));
    EXPECT_GT(result, 0);
}

TEST(FloatBitsTest, PreservesDoubleNaN) {
    double nan = std::numeric_limits<double>::quiet_NaN();
    uint64_t bits = FloatBits::to_u64(nan);
    double result = FloatBits::from_u64(bits);
    
    EXPECT_TRUE(std::isnan(result));
}

// ============================================================================
// Zero and Negative Zero Tests
// ============================================================================

TEST(FloatBitsTest, PreservesZero) {
    float zero = 0.0f;
    uint32_t bits = FloatBits::to_u32(zero);
    EXPECT_EQ(bits, 0u);
    
    float result = FloatBits::from_u32(bits);
    EXPECT_EQ(result, 0.0f);
}

TEST(FloatBitsTest, PreservesNegativeZero) {
    float neg_zero = -0.0f;
    uint32_t bits = FloatBits::to_u32(neg_zero);
    
    // Negative zero has bit pattern 0x80000000
    EXPECT_EQ(bits, 0x80000000u);
    
    float result = FloatBits::from_u32(bits);
    // Use copysign to check if it's negative zero
    EXPECT_EQ(std::copysign(1.0f, result), -1.0f);
}

// ============================================================================
// Edge Cases Tests
// ============================================================================

TEST(FloatBitsTest, MinMaxFloatValues) {
    // Test smallest positive normalized float
    float min_float = std::numeric_limits<float>::min();
    uint32_t bits_min = FloatBits::to_u32(min_float);
    float result_min = FloatBits::from_u32(bits_min);
    EXPECT_EQ(min_float, result_min);
    
    // Test largest float
    float max_float = std::numeric_limits<float>::max();
    uint32_t bits_max = FloatBits::to_u32(max_float);
    float result_max = FloatBits::from_u32(bits_max);
    EXPECT_EQ(max_float, result_max);
}

TEST(FloatBitsTest, MinMaxDoubleValues) {
    // Test smallest positive normalized double
    double min_double = std::numeric_limits<double>::min();
    uint64_t bits_min = FloatBits::to_u64(min_double);
    double result_min = FloatBits::from_u64(bits_min);
    EXPECT_EQ(min_double, result_min);
    
    // Test largest double
    double max_double = std::numeric_limits<double>::max();
    uint64_t bits_max = FloatBits::to_u64(max_double);
    double result_max = FloatBits::from_u64(bits_max);
    EXPECT_EQ(max_double, result_max);
}

// ============================================================================
// Bit Pattern Tests
// ============================================================================

TEST(FloatBitsTest, KnownBitPattern) {
    // 3.14159... in IEEE 754 is approximately 0x40490FDB
    uint32_t expected_bits = 0x40490FDB;
    float f = FloatBits::from_u32(expected_bits);
    
    // Should be approximately pi
    EXPECT_NEAR(f, 3.14159f, 0.00001f);
    
    // Convert back and verify bit pattern is preserved
    uint32_t bits = FloatBits::to_u32(f);
    EXPECT_EQ(bits, expected_bits);
}

TEST(SafeCastTest, NoOptimizationIssues) {
    // This test ensures the compiler doesn't optimize away the memcpy
    volatile float f = 1.0f;
    uint32_t bits = safe_cast<uint32_t>(f);
    volatile float f_back = safe_cast<float>(bits);
    
    // The volatile qualifier prevents optimization
    EXPECT_EQ(f, f_back);
}
