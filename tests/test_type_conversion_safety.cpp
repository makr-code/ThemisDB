/**
 * ThemisDB Type Conversion Safety Tests
 * 
 * Tests for signed/unsigned type conversion utilities
 * ensuring safe operations and proper error handling.
 * 
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "utils/type_conversion.h"
#include <limits>
#include <vector>

using namespace themis::utils;
using namespace themis::utils::conversion;

// ============================================================================
// Tests for safe_int_to_size()
// ============================================================================

TEST(TypeConversionSafety, SafeIntToSize_ValidPositive) {
    int value = 100;
    size_t result = safe_int_to_size(value);
    EXPECT_EQ(result, 100u);
}

TEST(TypeConversionSafety, SafeIntToSize_Zero) {
    int value = 0;
    size_t result = safe_int_to_size(value);
    EXPECT_EQ(result, 0u);
}

TEST(TypeConversionSafety, SafeIntToSize_NegativeThrows) {
    int value = -1;
    EXPECT_THROW(safe_int_to_size(value), ConversionException);
    
    int large_negative = -1000;
    EXPECT_THROW(safe_int_to_size(large_negative), ConversionException);
}

TEST(TypeConversionSafety, SafeIntToSize_LargePositive) {
    int value = std::numeric_limits<int>::max();
    size_t result = safe_int_to_size(value);
    EXPECT_EQ(result, static_cast<size_t>(std::numeric_limits<int>::max()));
}

// ============================================================================
// Tests for safe_int64_to_size()
// ============================================================================

TEST(TypeConversionSafety, SafeInt64ToSize_ValidPositive) {
    int64_t value = 1000000LL;
    size_t result = safe_int64_to_size(value);
    EXPECT_EQ(result, 1000000u);
}

TEST(TypeConversionSafety, SafeInt64ToSize_NegativeThrows) {
    int64_t value = -1LL;
    EXPECT_THROW(safe_int64_to_size(value), ConversionException);
}

// ============================================================================
// Tests for is_valid_index()
// ============================================================================

TEST(TypeConversionSafety, IsValidIndex_ValidPositive) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    EXPECT_TRUE(is_valid_index(0, vec.size()));
    EXPECT_TRUE(is_valid_index(2, vec.size()));
    EXPECT_TRUE(is_valid_index(4, vec.size()));
}

TEST(TypeConversionSafety, IsValidIndex_OutOfBounds) {
    std::vector<int> vec = {1, 2, 3};
    
    EXPECT_FALSE(is_valid_index(3, vec.size()));
    EXPECT_FALSE(is_valid_index(10, vec.size()));
}

TEST(TypeConversionSafety, IsValidIndex_NegativeIndex) {
    std::vector<int> vec = {1, 2, 3};
    
    // This is the critical test - negative indices should be invalid
    EXPECT_FALSE(is_valid_index(-1, vec.size()));
    EXPECT_FALSE(is_valid_index(-5, vec.size()));
}

TEST(TypeConversionSafety, IsValidIndex_EmptyContainer) {
    std::vector<int> vec;
    
    EXPECT_FALSE(is_valid_index(0, vec.size()));
    EXPECT_FALSE(is_valid_index(-1, vec.size()));
}

// ============================================================================
// Tests for safe_diff()
// ============================================================================

TEST(TypeConversionSafety, SafeDiff_PositiveDifference) {
    size_t a = 100;
    size_t b = 50;
    std::ptrdiff_t result = safe_diff(a, b);
    EXPECT_EQ(result, 50);
}

TEST(TypeConversionSafety, SafeDiff_NegativeDifference) {
    size_t a = 50;
    size_t b = 100;
    std::ptrdiff_t result = safe_diff(a, b);
    EXPECT_EQ(result, -50);
}

TEST(TypeConversionSafety, SafeDiff_ZeroDifference) {
    size_t a = 100;
    size_t b = 100;
    std::ptrdiff_t result = safe_diff(a, b);
    EXPECT_EQ(result, 0);
}

TEST(TypeConversionSafety, SafeDiff_LargeButValid) {
    size_t a = 1000000;
    size_t b = 500000;
    std::ptrdiff_t result = safe_diff(a, b);
    EXPECT_EQ(result, 500000);
}

// ============================================================================
// Integration Tests - Real-world scenarios
// ============================================================================

TEST(TypeConversionSafety, RealWorld_LoopWithNegativeCheck) {
    std::vector<int> data = {10, 20, 30, 40, 50};
    int index = -1;
    
    // Should not crash with negative index
    if (is_valid_index(index, data.size())) {
        // This should never execute
        FAIL() << "Negative index should not be valid";
    }
    
    // Now with valid index
    index = 2;
    if (is_valid_index(index, data.size())) {
        EXPECT_EQ(data[index], 30);
    } else {
        FAIL() << "Valid index should be accepted";
    }
}

TEST(TypeConversionSafety, RealWorld_ContainerSizeComparison) {
    std::vector<int> container = {1, 2, 3, 4, 5};
    int count = 3;
    
    // Safe conversion before comparison
    size_t count_size = safe_int_to_size(count);
    EXPECT_LT(count_size, container.size());
    
    // Process only 'count' items
    for (size_t i = 0; i < count_size && i < container.size(); ++i) {
        EXPECT_GT(container[i], 0);
    }
}

TEST(TypeConversionSafety, RealWorld_StringSubstringWithSafety) {
    std::string str = "Hello World";
    int start = 6;
    int length = 5;
    
    // Safe conversions
    size_t start_size = safe_int_to_size(start);
    size_t length_size = safe_int_to_size(length);
    
    if (start_size + length_size <= str.length()) {
        std::string substr = str.substr(start_size, length_size);
        EXPECT_EQ(substr, "World");
    }
}

TEST(TypeConversionSafety, RealWorld_ArrayAccessWithValidation) {
    std::vector<double> values = {1.1, 2.2, 3.3, 4.4, 5.5};
    
    // Simulate user input that could be negative
    int user_index = -1;
    
    // Validate before access
    if (is_valid_index(user_index, values.size())) {
        FAIL() << "Should not access with negative index";
    }
    
    user_index = 2;
    if (is_valid_index(user_index, values.size())) {
        EXPECT_DOUBLE_EQ(values[user_index], 3.3);
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(TypeConversionSafety, EdgeCase_MaxIntToSize) {
    int max_int = std::numeric_limits<int>::max();
    size_t result = safe_int_to_size(max_int);
    EXPECT_EQ(result, static_cast<size_t>(max_int));
}

TEST(TypeConversionSafety, EdgeCase_VeryLargeContainer) {
    // Simulate a very large container size
    size_t large_size = static_cast<size_t>(std::numeric_limits<int>::max()) + 100;
    
    // A negative index should still be invalid
    EXPECT_FALSE(is_valid_index(-1, large_size));
    
    // Max int should be valid if container is large enough
    EXPECT_TRUE(is_valid_index(std::numeric_limits<int>::max(), large_size));
}

TEST(TypeConversionSafety, EdgeCase_ZeroSizedContainer) {
    std::vector<int> empty_vec;
    
    // Any access to empty container should be invalid
    EXPECT_FALSE(is_valid_index(0, empty_vec.size()));
    EXPECT_FALSE(is_valid_index(-1, empty_vec.size()));
    EXPECT_FALSE(is_valid_index(1, empty_vec.size()));
}

// ============================================================================
// Existing conversion utilities tests (already in codebase)
// ============================================================================

TEST(TypeConversionSafety, ExistingSafeSize_ToInt32) {
    size_t small = 100;
    int32_t result = safe_size_to_int32(small);
    EXPECT_EQ(result, 100);
    
    // Overflow test
    size_t too_large = static_cast<size_t>(std::numeric_limits<int32_t>::max()) + 1;
    EXPECT_THROW(safe_size_to_int32(too_large), ConversionException);
}

TEST(TypeConversionSafety, ExistingSafeSize_ToInt) {
    size_t small = 1000;
    int result = safe_size_to_int(small);
    EXPECT_EQ(result, 1000);
    
    // Overflow test
    size_t too_large = static_cast<size_t>(std::numeric_limits<int>::max()) + 1;
    EXPECT_THROW(safe_size_to_int(too_large), ConversionException);
}
