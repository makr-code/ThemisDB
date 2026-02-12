#include <gtest/gtest.h>
#include "utils/safe_arithmetic.h"
#include <limits>
#include <vector>

using namespace themis::utils;

// ============================================================================
// safe_add Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeAddPositiveOffset) {
    size_t base = 10;
    int offset = 5;
    auto result = safe_add(base, offset);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 15u);
}

TEST(SafeArithmeticTest, SafeAddNegativeOffset) {
    size_t base = 10;
    int offset = -5;
    auto result = safe_add(base, offset);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 5u);
}

TEST(SafeArithmeticTest, SafeAddUnderflowDetection) {
    size_t base = 3;
    int offset = -10;
    auto result = safe_add(base, offset);
    
    ASSERT_FALSE(result.has_value());  // Should detect underflow
}

TEST(SafeArithmeticTest, SafeAddOverflowDetection) {
    size_t base = SIZE_MAX - 10;
    int offset = 20;
    auto result = safe_add(base, offset);
    
    ASSERT_FALSE(result.has_value());  // Should detect overflow
}

TEST(SafeArithmeticTest, SafeAddZeroOffset) {
    size_t base = 42;
    int offset = 0;
    auto result = safe_add(base, offset);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 42u);
}

TEST(SafeArithmeticTest, SafeAddEdgeCaseZeroBase) {
    size_t base = 0;
    int offset = -1;
    auto result = safe_add(base, offset);
    
    ASSERT_FALSE(result.has_value());  // Underflow
}

// ============================================================================
// safe_sub Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeSubNormal) {
    size_t a = 10;
    size_t b = 5;
    auto result = safe_sub(a, b);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 5u);
}

TEST(SafeArithmeticTest, SafeSubUnderflow) {
    size_t a = 5;
    size_t b = 10;
    auto result = safe_sub(a, b);
    
    ASSERT_FALSE(result.has_value());  // Would underflow
}

TEST(SafeArithmeticTest, SafeSubEqual) {
    size_t a = 10;
    size_t b = 10;
    auto result = safe_sub(a, b);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);
}

// ============================================================================
// safe_int_to_size Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeIntToSizePositive) {
    int value = 100;
    auto result = safe_int_to_size(value);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 100u);
}

TEST(SafeArithmeticTest, SafeIntToSizeNegative) {
    int value = -1;
    auto result = safe_int_to_size(value);
    
    ASSERT_FALSE(result.has_value());  // Negative not allowed
}

TEST(SafeArithmeticTest, SafeIntToSizeZero) {
    int value = 0;
    auto result = safe_int_to_size(value);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);
}

// ============================================================================
// safe_int64_to_size Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeInt64ToSizePositive) {
    int64_t value = 1000;
    auto result = safe_int64_to_size(value);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 1000u);
}

TEST(SafeArithmeticTest, SafeInt64ToSizeNegative) {
    int64_t value = -100;
    auto result = safe_int64_to_size(value);
    
    ASSERT_FALSE(result.has_value());
}

// ============================================================================
// in_range Tests
// ============================================================================

TEST(SafeArithmeticTest, InRangeValid) {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    int idx = 2;
    
    EXPECT_TRUE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeNegative) {
    std::vector<int> vec = {1, 2, 3};
    int idx = -1;
    
    EXPECT_FALSE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeTooLarge) {
    std::vector<int> vec = {1, 2, 3};
    int idx = 10;
    
    EXPECT_FALSE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeZero) {
    std::vector<int> vec = {1, 2, 3};
    int idx = 0;
    
    EXPECT_TRUE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeLastElement) {
    std::vector<int> vec = {1, 2, 3};
    int idx = 2;
    
    EXPECT_TRUE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeJustOutOfBounds) {
    std::vector<int> vec = {1, 2, 3};
    int idx = 3;
    
    EXPECT_FALSE(in_range(idx, vec.size()));  // Size is 3, valid indices are 0-2
}

TEST(SafeArithmeticTest, InRangeInt64Valid) {
    std::vector<int> vec = {1, 2, 3};
    int64_t idx = 1;
    
    EXPECT_TRUE(in_range(idx, vec.size()));
}

TEST(SafeArithmeticTest, InRangeInt64Negative) {
    std::vector<int> vec = {1, 2, 3};
    int64_t idx = -5;
    
    EXPECT_FALSE(in_range(idx, vec.size()));
}

// ============================================================================
// safe_size_to_int Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeSizeToIntNormal) {
    size_t value = 100;
    auto result = safe_size_to_int(value);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, 100);
}

TEST(SafeArithmeticTest, SafeSizeToIntMaxInt) {
    size_t value = INT_MAX;
    auto result = safe_size_to_int(value);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, INT_MAX);
}

TEST(SafeArithmeticTest, SafeSizeToIntOverflow) {
    size_t value = static_cast<size_t>(INT_MAX) + 1;
    auto result = safe_size_to_int(value);
    
    ASSERT_FALSE(result.has_value());  // Overflow
}

// ============================================================================
// safe_less_than Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeLessThanPositiveTrue) {
    int signed_val = 5;
    size_t unsigned_val = 10;
    
    EXPECT_TRUE(safe_less_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeLessThanPositiveFalse) {
    int signed_val = 10;
    size_t unsigned_val = 5;
    
    EXPECT_FALSE(safe_less_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeLessThanNegative) {
    int signed_val = -1;
    size_t unsigned_val = 0;
    
    EXPECT_TRUE(safe_less_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeLessThanEqual) {
    int signed_val = 5;
    size_t unsigned_val = 5;
    
    EXPECT_FALSE(safe_less_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeLessThanInt64Negative) {
    int64_t signed_val = -100;
    size_t unsigned_val = 10;
    
    EXPECT_TRUE(safe_less_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeLessThanInt64Positive) {
    int64_t signed_val = 50;
    size_t unsigned_val = 100;
    
    EXPECT_TRUE(safe_less_than(signed_val, unsigned_val));
}

// ============================================================================
// safe_greater_than Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeGreaterThanPositiveTrue) {
    int signed_val = 10;
    size_t unsigned_val = 5;
    
    EXPECT_TRUE(safe_greater_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeGreaterThanPositiveFalse) {
    int signed_val = 5;
    size_t unsigned_val = 10;
    
    EXPECT_FALSE(safe_greater_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeGreaterThanNegative) {
    int signed_val = -1;
    size_t unsigned_val = 0;
    
    EXPECT_FALSE(safe_greater_than(signed_val, unsigned_val));
}

TEST(SafeArithmeticTest, SafeGreaterThanEqual) {
    int signed_val = 5;
    size_t unsigned_val = 5;
    
    EXPECT_FALSE(safe_greater_than(signed_val, unsigned_val));
}

// ============================================================================
// safe_iterate Tests
// ============================================================================

TEST(SafeArithmeticTest, SafeIterateValid) {
    std::vector<int> data = {10, 20, 30, 40, 50};
    std::vector<int> results;
    
    bool success = safe_iterate(data, 2, [&](size_t i) {
        results.push_back(data[i]);
    });
    
    EXPECT_TRUE(success);
    ASSERT_EQ(results.size(), 3u);
    EXPECT_EQ(results[0], 30);
    EXPECT_EQ(results[1], 40);
    EXPECT_EQ(results[2], 50);
}

TEST(SafeArithmeticTest, SafeIterateFromStart) {
    std::vector<int> data = {1, 2, 3};
    int sum = 0;
    
    bool success = safe_iterate(data, 0, [&](size_t i) {
        sum += data[i];
    });
    
    EXPECT_TRUE(success);
    EXPECT_EQ(sum, 6);
}

TEST(SafeArithmeticTest, SafeIterateNegativeIndex) {
    std::vector<int> data = {1, 2, 3};
    bool callback_called = false;
    
    bool success = safe_iterate(data, -1, [&](size_t) {
        callback_called = true;  // Should not be called
    });
    
    EXPECT_FALSE(success);
    EXPECT_FALSE(callback_called);  // Verify callback was never invoked
}

TEST(SafeArithmeticTest, SafeIterateOutOfBounds) {
    std::vector<int> data = {1, 2, 3};
    bool callback_called = false;
    
    bool success = safe_iterate(data, 10, [&](size_t) {
        callback_called = true;  // Should not be called
    });
    
    EXPECT_FALSE(success);
    EXPECT_FALSE(callback_called);  // Verify callback was never invoked
}

// ============================================================================
// Real-World Usage Tests
// ============================================================================

TEST(SafeArithmeticTest, RealWorldVectorIndexing) {
    // Simulate common pattern: int index with vector
    std::vector<int> data = {10, 20, 30, 40, 50};
    int user_index = 2;
    
    if (in_range(user_index, data.size())) {
        EXPECT_EQ(data[user_index], 30);
    } else {
        FAIL() << "Index should be valid";
    }
}

TEST(SafeArithmeticTest, RealWorldBufferAllocation) {
    // Simulate: get count from API, allocate buffer
    int count_from_api = 100;
    
    if (auto size = safe_int_to_size(count_from_api)) {
        std::vector<int> buffer(*size);
        EXPECT_EQ(buffer.size(), 100u);
    } else {
        FAIL() << "Conversion should succeed";
    }
}

TEST(SafeArithmeticTest, RealWorldNegativeErrorCode) {
    // Simulate: error code -1 should not create huge buffer
    int error_code = -1;
    
    auto size = safe_int_to_size(error_code);
    EXPECT_FALSE(size.has_value());  // Correctly rejects negative
}

TEST(SafeArithmeticTest, RealWorldLoopWithOffset) {
    // Simulate: loop starting from offset
    std::vector<int> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int offset = 3;
    int count = 4;
    
    std::vector<int> results;
    
    if (in_range(offset, data.size())) {
        size_t start = static_cast<size_t>(offset);
        size_t end = std::min(start + static_cast<size_t>(count), data.size());
        
        for (size_t i = start; i < end; ++i) {
            results.push_back(data[i]);
        }
    }
    
    ASSERT_EQ(results.size(), 4u);
    EXPECT_EQ(results[0], 4);
    EXPECT_EQ(results[1], 5);
    EXPECT_EQ(results[2], 6);
    EXPECT_EQ(results[3], 7);
}
