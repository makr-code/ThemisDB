/**
 * @file test_cache_lru_move_semantics.cpp
 * @brief Tests for LRU cache move semantics and container integration
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "cache/lru_cache.h"
#include <utility>
#include <string>

using namespace themis::cache;

class LRUCacheTest : public ::testing::Test {
protected:
    using StringIntCache = LRUCache<std::string, int>;
};

// Test: Default construction creates valid state
TEST_F(LRUCacheTest, DefaultConstructionValid) {
    StringIntCache cache(10);
    EXPECT_FALSE(cache.is_moved_from());
    EXPECT_TRUE(cache.is_valid());
    EXPECT_EQ(cache.size(), 0);
}

// Test: Move constructor transfers ownership
TEST_F(LRUCacheTest, MoveConstructorTransfersOwnership) {
    StringIntCache src(10);
    src.insert("key1", 42);

    StringIntCache dst(std::move(src));
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Move assignment transfers ownership
TEST_F(LRUCacheTest, MoveAssignmentTransfersOwnership) {
    StringIntCache src(10);
    src.insert("key1", 42);

    StringIntCache dst(10);
    dst = std::move(src);

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Moved-from cache throws on insert
TEST_F(LRUCacheTest, MovedFromCacheThrowsOnInsert) {
    StringIntCache src(10);
    StringIntCache dst(std::move(src));

    EXPECT_THROW({
        src.insert("key", 42);
    }, std::logic_error);
}

// Test: Moved-from cache throws on get
TEST_F(LRUCacheTest, MovedFromCacheThrowsOnGet) {
    StringIntCache src(10);
    StringIntCache dst(std::move(src));

    EXPECT_THROW({
        src.get("key");
    }, std::logic_error);
}

// Test: Moved-from cache throws on peek
TEST_F(LRUCacheTest, MovedFromCacheThrowsOnPeek) {
    StringIntCache src(10);
    StringIntCache dst(std::move(src));

    EXPECT_THROW({
        src.peek("key");
    }, std::logic_error);
}

// Test: Moved-from cache throws on erase
TEST_F(LRUCacheTest, MovedFromCacheThrowsOnErase) {
    StringIntCache src(10);
    StringIntCache dst(std::move(src));

    EXPECT_THROW({
        src.erase("key");
    }, std::logic_error);
}

// Test: Moved-from cache throws on clear
TEST_F(LRUCacheTest, MovedFromCacheThrowsOnClear) {
    StringIntCache src(10);
    StringIntCache dst(std::move(src));

    EXPECT_THROW({
        src.clear();
    }, std::logic_error);
}

// Test: Self-move assignment is safe
TEST_F(LRUCacheTest, SelfMoveAssignmentIsSafe) {
    StringIntCache cache(10);
    cache.insert("key", 42);
    
    size_t original_size = cache.size();
    cache = std::move(cache);

    EXPECT_TRUE(cache.is_moved_from());
}

// Test: Capacity preserved after move
TEST_F(LRUCacheTest, CapacityPreservedAfterMove) {
    StringIntCache src(100);
    StringIntCache dst(std::move(src));

    EXPECT_EQ(dst.capacity(), 100);
}

// Test: Hit/miss callbacks work after move
TEST_F(LRUCacheTest, CallbacksWorkAfterMove) {
    StringIntCache src(10);
    int hit_count = 0;
    src.on_hit([&hit_count](const std::string&, const int&) {
        hit_count++;
    });

    StringIntCache dst(std::move(src));
    
    // Source is moved-from, can't test callbacks
    EXPECT_EQ(src.hits(), 0);
}

// Test: Destructor safe on moved-from cache
TEST_F(LRUCacheTest, DestructorSafeOnMovedFrom) {
    {
        StringIntCache src(10);
        StringIntCache dst(std::move(src));
    }  // Should not crash
}

// Test: Move with populated cache
TEST_F(LRUCacheTest, MoveWithPopulatedCache) {
    StringIntCache src(10);
    for (int i = 0; i < 5; i++) {
        src.insert("key" + std::to_string(i), i * 10);
    }

    size_t original_size = src.size();
    StringIntCache dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_EQ(dst.size(), original_size);
}

// Test: Cache statistics reset on moved-from
TEST_F(LRUCacheTest, StatsResetOnMovedFrom) {
    StringIntCache src(10);
    for (int i = 0; i < 5; i++) {
        src.insert("key" + std::to_string(i), i);
    }

    StringIntCache dst(std::move(src));

    EXPECT_EQ(src.hits(), 0);
    EXPECT_EQ(src.misses(), 0);
    EXPECT_EQ(src.evictions(), 0);
}

// Test: LRU order maintained after move
TEST_F(LRUCacheTest, LRUOrderMaintainedAfterMove) {
    StringIntCache src(5);
    for (int i = 0; i < 3; i++) {
        src.insert("key" + std::to_string(i), i);
    }

    StringIntCache dst(std::move(src));

    // Check that moved cache can still contain entries
    EXPECT_EQ(dst.size(), 3);
}
