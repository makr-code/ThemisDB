/**
 * @file test_cache_eviction_policy_move_semantics.cpp
 * @brief Tests for polymorphic eviction policy move semantics
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "cache/cache_eviction_policy.h"
#include <utility>
#include <memory>

using namespace themis::cache;

class EvictionPolicyTest : public ::testing::Test {
protected:
    std::vector<CacheKeyDescriptor> create_test_candidates() {
        return {
            {"key1", 5, 100, 1000},
            {"key2", 3, 50, 2000},
            {"key3", 8, 200, 500}
        };
    }
};

// Test: LRU policy basic functionality
TEST_F(EvictionPolicyTest, LRUPolicyBasic) {
    LRUEvictionPolicy policy;
    EXPECT_FALSE(policy.is_moved_from());
    EXPECT_STREQ(policy.policy_name(), "LRU");
}

// Test: LRU policy move constructor
TEST_F(EvictionPolicyTest, LRUPolicyMoveConstructor) {
    LRUEvictionPolicy src;
    LRUEvictionPolicy dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: LRU policy move assignment
TEST_F(EvictionPolicyTest, LRUPolicyMoveAssignment) {
    LRUEvictionPolicy src;
    LRUEvictionPolicy dst;

    dst = std::move(src);
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: LRU policy throws on moved-from record_hit
TEST_F(EvictionPolicyTest, LRUPolicyThrowsOnMovedFromRecordHit) {
    LRUEvictionPolicy src;
    LRUEvictionPolicy dst(std::move(src));

    EXPECT_THROW({
        src.record_hit("key");
    }, std::logic_error);
}

// Test: LRU policy throws on moved-from choose_victim
TEST_F(EvictionPolicyTest, LRUPolicyThrowsOnMovedFromChooseVictim) {
    LRUEvictionPolicy src;
    LRUEvictionPolicy dst(std::move(src));

    auto candidates = create_test_candidates();
    EXPECT_THROW({
        src.choose_victim(candidates);
    }, std::logic_error);
}

// Test: LFU policy basic functionality
TEST_F(EvictionPolicyTest, LFUPolicyBasic) {
    LFUEvictionPolicy policy(0.5);
    EXPECT_FALSE(policy.is_moved_from());
    EXPECT_STREQ(policy.policy_name(), "LFU");
}

// Test: LFU policy move constructor
TEST_F(EvictionPolicyTest, LFUPolicyMoveConstructor) {
    LFUEvictionPolicy src(0.5);
    LFUEvictionPolicy dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: LFU policy move preserves configuration
TEST_F(EvictionPolicyTest, LFUPolicyMovePreservesConfig) {
    LFUEvictionPolicy src(0.75);
    LFUEvictionPolicy dst(std::move(src));

    auto cloned = dst.clone();
    EXPECT_STREQ(cloned->policy_name(), "LFU");
}

// Test: FIFO policy basic functionality
TEST_F(EvictionPolicyTest, FIFOPolicyBasic) {
    FIFOEvictionPolicy policy;
    EXPECT_FALSE(policy.is_moved_from());
    EXPECT_STREQ(policy.policy_name(), "FIFO");
}

// Test: FIFO policy move constructor
TEST_F(EvictionPolicyTest, FIFOPolicyMoveConstructor) {
    FIFOEvictionPolicy src;
    FIFOEvictionPolicy dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: ARC policy basic functionality
TEST_F(EvictionPolicyTest, ARCPolicyBasic) {
    ARCEvictionPolicy policy;
    EXPECT_FALSE(policy.is_moved_from());
    EXPECT_STREQ(policy.policy_name(), "ARC");
}

// Test: ARC policy move constructor
TEST_F(EvictionPolicyTest, ARCPolicyMoveConstructor) {
    ARCEvictionPolicy src;
    ARCEvictionPolicy dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: ARC policy move assignment
TEST_F(EvictionPolicyTest, ARCPolicyMoveAssignment) {
    ARCEvictionPolicy src;
    ARCEvictionPolicy dst;

    dst = std::move(src);
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Policy factory creates policies
TEST_F(EvictionPolicyTest, PolicyFactoryCreatesLRU) {
    auto policy = EvictionPolicyFactory::create("LRU");
    EXPECT_NE(policy, nullptr);
    EXPECT_STREQ(policy->policy_name(), "LRU");
}

// Test: Policy factory creates LFU
TEST_F(EvictionPolicyTest, PolicyFactoryCreatesLFU) {
    auto policy = EvictionPolicyFactory::create("LFU");
    EXPECT_NE(policy, nullptr);
    EXPECT_STREQ(policy->policy_name(), "LFU");
}

// Test: Policy factory creates FIFO
TEST_F(EvictionPolicyTest, PolicyFactoryCreatesFIFO) {
    auto policy = EvictionPolicyFactory::create("FIFO");
    EXPECT_NE(policy, nullptr);
    EXPECT_STREQ(policy->policy_name(), "FIFO");
}

// Test: Policy factory creates ARC
TEST_F(EvictionPolicyTest, PolicyFactoryCreatesARC) {
    auto policy = EvictionPolicyFactory::create("ARC");
    EXPECT_NE(policy, nullptr);
    EXPECT_STREQ(policy->policy_name(), "ARC");
}

// Test: Policy factory throws on invalid name
TEST_F(EvictionPolicyTest, PolicyFactoryThrowsOnInvalid) {
    EXPECT_THROW({
        EvictionPolicyFactory::create("INVALID");
    }, std::invalid_argument);
}

// Test: Policy factory case-insensitive
TEST_F(EvictionPolicyTest, PolicyFactoryCaseInsensitive) {
    auto policy_upper = EvictionPolicyFactory::create("LRU");
    auto policy_lower = EvictionPolicyFactory::create("lru");

    EXPECT_STREQ(policy_upper->policy_name(), policy_lower->policy_name());
}

// Test: Polymorphic move with unique_ptr
TEST_F(EvictionPolicyTest, PolymorphicMoveWithUniquePtr) {
    std::unique_ptr<CacheEvictionPolicy> src = std::make_unique<LRUEvictionPolicy>();
    std::unique_ptr<CacheEvictionPolicy> dst = std::move(src);

    EXPECT_TRUE(src == nullptr);
    EXPECT_NE(dst, nullptr);
    EXPECT_STREQ(dst->policy_name(), "LRU");
}

// Test: Destructor safe on moved-from policies
TEST_F(EvictionPolicyTest, DestructorSafeOnMovedFrom) {
    {
        LRUEvictionPolicy src;
        LRUEvictionPolicy dst(std::move(src));
    }  // Should not crash
}

// Test: Clone functionality
TEST_F(EvictionPolicyTest, CloneFunctionality) {
    LRUEvictionPolicy src;
    auto cloned = src.clone();

    EXPECT_NE(cloned, nullptr);
    EXPECT_STREQ(cloned->policy_name(), "LRU");
    EXPECT_FALSE(cloned->is_moved_from());
}
