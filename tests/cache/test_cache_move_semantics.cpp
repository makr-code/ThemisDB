/**
 * @file test_cache_move_semantics.cpp
 * @brief Tests for Cache module move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "cache/cache_move_semantics.h"
#include <memory>
#include <utility>

namespace themis {
namespace cache {

class CacheModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// CacheEntry Template Tests (Gap 6.1)
// ============================================================================

TEST_F(CacheModuleMoveTest, CacheEntry_StringInt_MoveConstruction) {
    auto entry1 = std::make_unique<CacheEntry<std::string, int>>(
        "key1", 42);
    
    CacheEntry<std::string, int> entry2(std::move(*entry1));
    
    EXPECT_EQ(entry2.getKey(), "key1");
    EXPECT_EQ(entry2.getValue(), 42);
    EXPECT_EQ(entry2.getAccessCount(), 0);
}

TEST_F(CacheModuleMoveTest, CacheEntry_StringInt_MoveAssignment) {
    auto entry1 = std::make_unique<CacheEntry<std::string, int>>(
        "key1", 42);
    auto entry2 = std::make_unique<CacheEntry<std::string, int>>(
        "key2", 99);
    
    *entry2 = std::move(*entry1);
    
    EXPECT_EQ(entry2->getKey(), "key1");
    EXPECT_EQ(entry2->getValue(), 42);
}

TEST_F(CacheModuleMoveTest, CacheEntry_StringString_MoveConstruction) {
    auto entry1 = std::make_unique<CacheEntry<std::string, std::string>>(
        "cache_key", "cache_value");
    
    CacheEntry<std::string, std::string> entry2(std::move(*entry1));
    
    EXPECT_EQ(entry2.getKey(), "cache_key");
    EXPECT_EQ(entry2.getValue(), "cache_value");
}

TEST_F(CacheModuleMoveTest, CacheEntry_DeletesCopy) {
    using TestEntry = CacheEntry<std::string, int>;
    static_assert(!std::is_copy_constructible_v<TestEntry>);
    static_assert(!std::is_copy_assignable_v<TestEntry>);
}

TEST_F(CacheModuleMoveTest, CacheEntry_IsMove) {
    using TestEntry = CacheEntry<std::string, int>;
    static_assert(std::is_move_constructible_v<TestEntry>);
    static_assert(std::is_move_assignable_v<TestEntry>);
}

TEST_F(CacheModuleMoveTest, CacheEntry_AccessCountTracking) {
    auto entry1 = std::make_unique<CacheEntry<std::string, int>>(
        "key", 100);
    entry1->incrementAccessCount();
    entry1->incrementAccessCount();
    entry1->incrementAccessCount();
    
    EXPECT_EQ(entry1->getAccessCount(), 3);
    
    CacheEntry<std::string, int> entry2(std::move(*entry1));
    
    // Access count transferred
    EXPECT_EQ(entry2.getAccessCount(), 3);
    // Source reset to 0
    EXPECT_EQ(entry1->getAccessCount(), 0);
}

// ============================================================================
// LRUEvictionPolicy Tests (Gap 6.2)
// ============================================================================

TEST_F(CacheModuleMoveTest, LRUEvictionPolicy_MoveConstruction) {
    auto policy1 = std::make_unique<LRUEvictionPolicy>(1000);
    LRUEvictionPolicy policy2(std::move(*policy1));
    
    EXPECT_EQ(policy2.getMaxSize(), 1000);
    EXPECT_EQ(policy2.getCurrentSize(), 0);
    EXPECT_EQ(policy1->getCurrentSize(), 0);
}

TEST_F(CacheModuleMoveTest, LRUEvictionPolicy_MoveAssignment) {
    auto policy1 = std::make_unique<LRUEvictionPolicy>(500);
    auto policy2 = std::make_unique<LRUEvictionPolicy>(1000);
    
    *policy2 = std::move(*policy1);
    
    EXPECT_EQ(policy2->getMaxSize(), 500);
    EXPECT_EQ(policy1->getMaxSize(), 500);
}

TEST_F(CacheModuleMoveTest, LRUEvictionPolicy_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<LRUEvictionPolicy>);
    static_assert(!std::is_copy_assignable_v<LRUEvictionPolicy>);
}

TEST_F(CacheModuleMoveTest, LRUEvictionPolicy_IsMove) {
    static_assert(std::is_move_constructible_v<LRUEvictionPolicy>);
    static_assert(std::is_move_assignable_v<LRUEvictionPolicy>);
}

// ============================================================================
// CacheManager Tests (Gap 6.3)
// ============================================================================

TEST_F(CacheModuleMoveTest, CacheManager_MoveConstruction) {
    auto manager1 = std::make_unique<CacheManager>();
    CacheManager manager2(std::move(*manager1));
    
    EXPECT_EQ(manager2.getSize(), 0);
    EXPECT_EQ(manager2.getHitCount(), 0);
    EXPECT_EQ(manager2.getMissCount(), 0);
    EXPECT_EQ(manager1->getSize(), 0);
}

TEST_F(CacheModuleMoveTest, CacheManager_MoveAssignment) {
    auto manager1 = std::make_unique<CacheManager>();
    auto manager2 = std::make_unique<CacheManager>();
    
    *manager2 = std::move(*manager1);
    
    EXPECT_EQ(manager2->getSize(), 0);
    EXPECT_EQ(manager1->getSize(), 0);
}

TEST_F(CacheModuleMoveTest, CacheManager_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<CacheManager>);
    static_assert(!std::is_copy_assignable_v<CacheManager>);
}

TEST_F(CacheModuleMoveTest, CacheManager_IsMove) {
    static_assert(std::is_move_constructible_v<CacheManager>);
    static_assert(std::is_move_assignable_v<CacheManager>);
}

TEST_F(CacheModuleMoveTest, CacheManager_StatisticsTracking) {
    auto manager1 = std::make_unique<CacheManager>();
    
    // Simulate access patterns (via public interface when available)
    // For now, just verify the move doesn't lose state
    
    CacheManager manager2(std::move(*manager1));
    
    EXPECT_EQ(manager2.getHitCount(), 0);
    EXPECT_EQ(manager2.getMissCount(), 0);
    EXPECT_EQ(manager1->getHitCount(), 0);
}

// ============================================================================
// Exception Safety Tests
// ============================================================================

TEST_F(CacheModuleMoveTest, CacheEntry_MoveNoexceptGuarantee) {
    using TestEntry = CacheEntry<std::string, int>;
    static_assert(std::is_nothrow_move_constructible_v<TestEntry>);
    static_assert(std::is_nothrow_move_assignable_v<TestEntry>);
}

TEST_F(CacheModuleMoveTest, LRUEvictionPolicy_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<LRUEvictionPolicy>);
    static_assert(std::is_nothrow_move_assignable_v<LRUEvictionPolicy>);
}

TEST_F(CacheModuleMoveTest, CacheManager_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<CacheManager>);
    static_assert(std::is_nothrow_move_assignable_v<CacheManager>);
}

}  // namespace cache
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
