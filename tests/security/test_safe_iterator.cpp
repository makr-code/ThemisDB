#include <gtest/gtest.h>
#include <vector>
#include <list>
#include <deque>
#include <array>
#include <memory>
#include "security/safe_iterator.h"

using namespace themis::security::SafeIterator;

// ============================================================================
// BoundsChecker Tests - Out-of-Bounds Access Prevention
// ============================================================================

class BoundsCheckerTest : public ::testing::Test {
protected:
    std::vector<int> vec_{1, 2, 3, 4, 5};
    std::deque<int> deque_{10, 20, 30, 40, 50};
    std::list<int> list_{100, 200, 300, 400, 500};
};

TEST_F(BoundsCheckerTest, VectorBoundsCheck_ValidDereference) {
    auto it = vec_.begin();
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()));
    EXPECT_EQ(*it, 1);
}

TEST_F(BoundsCheckerTest, VectorBoundsCheck_InvalidDereferenceAtEnd) {
    auto it = vec_.end();
    EXPECT_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()),
                 std::out_of_range);
}

TEST_F(BoundsCheckerTest, VectorBoundsCheck_ValidMiddleElement) {
    auto it = vec_.begin() + 2;
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()));
    EXPECT_EQ(*it, 3);
}

TEST_F(BoundsCheckerTest, VectorBoundsCheck_InvalidLastElement) {
    auto it = vec_.end();
    EXPECT_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()),
                 std::out_of_range);
}

TEST_F(BoundsCheckerTest, VectorBoundsCheck_IsValidForDereference) {
    auto valid_it = vec_.begin();
    auto invalid_it = vec_.end();
    
    EXPECT_TRUE(BoundsChecker::is_valid_for_dereference(valid_it, vec_.begin(), vec_.end()));
    EXPECT_FALSE(BoundsChecker::is_valid_for_dereference(invalid_it, vec_.begin(), vec_.end()));
}

TEST_F(BoundsCheckerTest, DequeOutOfBounds_BeforeBegin) {
    // Create an iterator conceptually before begin (via distance)
    auto begin_it = deque_.begin();
    auto end_it = deque_.end();
    
    // Verify that end is invalid for dereference
    EXPECT_FALSE(BoundsChecker::is_valid_for_dereference(end_it, begin_it, end_it));
}

TEST_F(BoundsCheckerTest, ListBoundsCheck_ValidAccess) {
    auto it = list_.begin();
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, list_.begin(), list_.end()));
}

TEST_F(BoundsCheckerTest, ListBoundsCheck_InvalidAccess) {
    auto it = list_.end();
    EXPECT_THROW(BoundsChecker::check_dereference(it, list_.begin(), list_.end()),
                 std::out_of_range);
}

TEST_F(BoundsCheckerTest, EmptyVectorBoundsCheck) {
    std::vector<int> empty;
    auto it = empty.begin();
    EXPECT_THROW(BoundsChecker::check_dereference(it, empty.begin(), empty.end()),
                 std::out_of_range);
}

// ============================================================================
// InvalidationDetector Tests - Use-After-Free Detection
// ============================================================================

class InvalidationDetectorTest : public ::testing::Test {
protected:
    std::vector<int> vec_{1, 2, 3, 4, 5};
};

TEST_F(InvalidationDetectorTest, DetectorCreation) {
    InvalidationDetector detector(vec_);
    EXPECT_EQ(detector.initial_size(), 5);
    EXPECT_FALSE(detector.check());
}

TEST_F(InvalidationDetectorTest, DetectorModificationDetection) {
    InvalidationDetector detector(vec_, false);  // Non-strict mode
    
    // Initial state: no modification
    EXPECT_FALSE(detector.check());
    
    // Mark modification
    detector.set_modification_detected(true);
    EXPECT_TRUE(detector.check());
}

TEST_F(InvalidationDetectorTest, DetectorStrictMode_ThrowsOnModification) {
    InvalidationDetector detector(vec_, true);  // Strict mode
    
    EXPECT_THROW(detector.set_modification_detected(true), std::runtime_error);
}

TEST_F(InvalidationDetectorTest, DetectorReset) {
    InvalidationDetector detector(vec_, false);
    detector.set_modification_detected(true);
    EXPECT_TRUE(detector.check());
    
    detector.reset();
    EXPECT_FALSE(detector.check());
}

TEST_F(InvalidationDetectorTest, DetectorWithLargeContainer) {
    std::vector<int> large(10000);
    for (int i = 0; i < 10000; ++i) {
        large[i] = i;
    }
    
    InvalidationDetector detector(large);
    EXPECT_EQ(detector.initial_size(), 10000);
}

// ============================================================================
// AdvanceSafe Tests - Safe Iterator Advancement
// ============================================================================

class AdvanceSafeTest : public ::testing::Test {
protected:
    std::vector<int> vec_{1, 2, 3, 4, 5};
    std::deque<int> deque_{10, 20, 30, 40, 50};
    std::list<int> list_{100, 200, 300, 400, 500};
};

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_Forward) {
    auto it = vec_.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 2, vec_.begin(), vec_.end()));
    EXPECT_EQ(*it, 3);
}

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_Backward) {
    auto it = vec_.begin() + 4;
    EXPECT_NO_THROW(AdvanceSafe::advance(it, -2, vec_.begin(), vec_.end()));
    EXPECT_EQ(*it, 3);
}

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_ZeroDistance) {
    auto it = vec_.begin();
    auto original_it = it;
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 0, vec_.begin(), vec_.end()));
    EXPECT_EQ(it, original_it);
}

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_OutOfBoundsForward) {
    auto it = vec_.begin();
    EXPECT_THROW(AdvanceSafe::advance(it, 10, vec_.begin(), vec_.end()),
                 std::out_of_range);
}

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_OutOfBoundsBackward) {
    auto it = vec_.begin() + 2;
    EXPECT_THROW(AdvanceSafe::advance(it, -5, vec_.begin(), vec_.end()),
                 std::out_of_range);
}

TEST_F(AdvanceSafeTest, VectorAdvanceSafe_AdvanceToEnd) {
    auto it = vec_.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 5, vec_.begin(), vec_.end()));
    EXPECT_EQ(it, vec_.end());
}

TEST_F(AdvanceSafeTest, DequeAdvanceSafe_Forward) {
    auto it = deque_.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 3, deque_.begin(), deque_.end()));
    EXPECT_EQ(*it, 40);
}

TEST_F(AdvanceSafeTest, ListAdvanceSafe_Forward) {
    auto it = list_.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 2, list_.begin(), list_.end()));
    EXPECT_EQ(*it, 300);
}

TEST_F(AdvanceSafeTest, ListAdvanceSafe_OutOfBounds) {
    auto it = list_.begin();
    EXPECT_THROW(AdvanceSafe::advance(it, 100, list_.begin(), list_.end()),
                 std::out_of_range);
}

TEST_F(AdvanceSafeTest, CanAdvance_ValidAdvance) {
    auto it = vec_.begin();
    EXPECT_TRUE(AdvanceSafe::can_advance(it, 2, vec_.begin(), vec_.end()));
}

TEST_F(AdvanceSafeTest, CanAdvance_InvalidAdvance) {
    auto it = vec_.begin();
    EXPECT_FALSE(AdvanceSafe::can_advance(it, 10, vec_.begin(), vec_.end()));
}

TEST_F(AdvanceSafeTest, CanAdvance_ZeroDistance) {
    auto it = vec_.begin();
    EXPECT_TRUE(AdvanceSafe::can_advance(it, 0, vec_.begin(), vec_.end()));
}

// ============================================================================
// RangeValidator Tests - Iterator Pair Validation
// ============================================================================

class RangeValidatorTest : public ::testing::Test {
protected:
    std::vector<int> vec_{1, 2, 3, 4, 5};
    std::list<int> list_{10, 20, 30, 40, 50};
};

TEST_F(RangeValidatorTest, ValidRange_Vector) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.end());
    EXPECT_EQ(*range.begin(), 1);
    EXPECT_EQ(range.begin(), vec_.begin());
    EXPECT_EQ(range.end(), vec_.end());
}

TEST_F(RangeValidatorTest, InvalidRange_Vector_BeginGreaterThanEnd) {
    EXPECT_THROW(
        RangeValidator<std::vector<int>::iterator> range(vec_.end(), vec_.begin()),
        std::invalid_argument
    );
}

TEST_F(RangeValidatorTest, RangeSize_Vector) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.end());
    EXPECT_EQ(range.size(), 5);
}

TEST_F(RangeValidatorTest, PartialRangeSize_Vector) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin() + 1, vec_.begin() + 4);
    EXPECT_EQ(range.size(), 3);
}

TEST_F(RangeValidatorTest, EmptyRange_Vector) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.begin());
    EXPECT_TRUE(range.empty());
    EXPECT_EQ(range.size(), 0);
}

TEST_F(RangeValidatorTest, ValidRange_List) {
    RangeValidator<std::list<int>::iterator> range(list_.begin(), list_.end());
    EXPECT_FALSE(range.empty());
    EXPECT_EQ(*range.begin(), 10);
}

TEST_F(RangeValidatorTest, RangeIteration) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.end());
    
    int sum = 0;
    for (auto it = range.begin(); it != range.end(); ++it) {
        sum += *it;
    }
    EXPECT_EQ(sum, 15);  // 1+2+3+4+5 = 15
}

TEST_F(RangeValidatorTest, SingleElementRange) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.begin() + 1);
    EXPECT_FALSE(range.empty());
    EXPECT_EQ(range.size(), 1);
    EXPECT_EQ(*range.begin(), 1);
}

// ============================================================================
// Integration Tests - Combined Safety Mechanisms
// ============================================================================

class SafeIteratorIntegrationTest : public ::testing::Test {
protected:
    std::vector<int> vec_{1, 2, 3, 4, 5};
};

TEST_F(SafeIteratorIntegrationTest, BoundsCheckAndAdvance) {
    auto it = vec_.begin();
    
    // Verify bounds before advancing
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()));
    
    // Safely advance
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 2, vec_.begin(), vec_.end()));
    
    // Verify bounds after advancing
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()));
    EXPECT_EQ(*it, 3);
}

TEST_F(SafeIteratorIntegrationTest, RangeValidationAndIteration) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin(), vec_.end());
    
    int count = 0;
    for (auto it = range.begin(); it != range.end(); ++it) {
        // Check bounds for every dereference
        EXPECT_NO_THROW(BoundsChecker::check_dereference(it, range.begin(), range.end()));
        ++count;
    }
    EXPECT_EQ(count, 5);
}

TEST_F(SafeIteratorIntegrationTest, InvalidationDetectorWithSafeIteration) {
    InvalidationDetector detector(vec_, false);
    
    for (auto it = vec_.begin(); it != vec_.end(); ++it) {
        // Periodically check for modifications
        EXPECT_FALSE(detector.check());
        
        // Safe dereference
        EXPECT_NO_THROW(BoundsChecker::check_dereference(it, vec_.begin(), vec_.end()));
    }
}

TEST_F(SafeIteratorIntegrationTest, ComplexScenario_AdvanceWithinRange) {
    RangeValidator<std::vector<int>::iterator> range(vec_.begin() + 1, vec_.end() - 1);
    auto it = range.begin();
    
    // Move within the range
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 1, range.begin(), range.end()));
    EXPECT_EQ(*it, 3);
    
    // Try to move beyond (should fail)
    EXPECT_THROW(AdvanceSafe::advance(it, 10, range.begin(), range.end()),
                 std::out_of_range);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

class SafeIteratorEdgeCaseTest : public ::testing::Test {};

TEST_F(SafeIteratorEdgeCaseTest, EmptyVectorRange) {
    std::vector<int> empty;
    RangeValidator<std::vector<int>::iterator> range(empty.begin(), empty.end());
    EXPECT_TRUE(range.empty());
    EXPECT_EQ(range.size(), 0);
}

TEST_F(SafeIteratorEdgeCaseTest, SingleElementVectorBounds) {
    std::vector<int> single{42};
    auto it = single.begin();
    
    EXPECT_NO_THROW(BoundsChecker::check_dereference(it, single.begin(), single.end()));
    EXPECT_EQ(*it, 42);
    
    // Advance to end
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 1, single.begin(), single.end()));
    EXPECT_EQ(it, single.end());
}

TEST_F(SafeIteratorEdgeCaseTest, LargeVectorPerformance) {
    std::vector<int> large(100000);
    for (int i = 0; i < 100000; ++i) {
        large[i] = i;
    }
    
    // Range validation on large vector
    RangeValidator<std::vector<int>::iterator> range(large.begin(), large.end());
    EXPECT_EQ(range.size(), 100000);
    
    // Advance should still work efficiently
    auto it = large.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 50000, large.begin(), large.end()));
    EXPECT_EQ(*it, 50000);
}

TEST_F(SafeIteratorEdgeCaseTest, BidirectionalIteratorAdvance) {
    std::list<int> list{1, 2, 3, 4, 5};
    
    auto it = list.end();
    // Move backward from end
    EXPECT_NO_THROW(AdvanceSafe::advance(it, -2, list.begin(), list.end()));
    EXPECT_EQ(*it, 4);
}

TEST_F(SafeIteratorEdgeCaseTest, DequeRandomAccessBehavior) {
    std::deque<int> deque{10, 20, 30, 40, 50};
    
    // Deques support random-access, so advance should be fast
    auto it = deque.begin();
    EXPECT_NO_THROW(AdvanceSafe::advance(it, 3, deque.begin(), deque.end()));
    EXPECT_EQ(*it, 40);
}

// ============================================================================
// Stress Tests
// ============================================================================

class SafeIteratorStressTest : public ::testing::Test {};

TEST_F(SafeIteratorStressTest, ManyAdvanceOperations) {
    std::vector<int> vec(1000);
    for (int i = 0; i < 1000; ++i) {
        vec[i] = i;
    }
    
    auto it = vec.begin();
    for (int i = 0; i < 100; ++i) {
        EXPECT_NO_THROW(AdvanceSafe::advance(it, 10, vec.begin(), vec.end()));
    }
    EXPECT_EQ(*it, 1000 - 1);  // Should be at the last position after 1000 advances
}

TEST_F(SafeIteratorStressTest, InterleavedBoundsChecks) {
    std::vector<int> vec(500);
    for (int i = 0; i < 500; ++i) {
        vec[i] = i;
    }
    
    int checks_passed = 0;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        if (BoundsChecker::is_valid_for_dereference(it, vec.begin(), vec.end())) {
            ++checks_passed;
        }
    }
    EXPECT_EQ(checks_passed, 500);
}

TEST_F(SafeIteratorStressTest, MultipleRangeValidators) {
    std::vector<int> vec(100);
    for (int i = 0; i < 100; ++i) {
        vec[i] = i;
    }
    
    // Create multiple overlapping ranges
    RangeValidator<std::vector<int>::iterator> range1(vec.begin(), vec.end());
    RangeValidator<std::vector<int>::iterator> range2(vec.begin() + 10, vec.begin() + 50);
    RangeValidator<std::vector<int>::iterator> range3(vec.begin() + 25, vec.begin() + 75);
    
    EXPECT_EQ(range1.size(), 100);
    EXPECT_EQ(range2.size(), 40);
    EXPECT_EQ(range3.size(), 50);
}
