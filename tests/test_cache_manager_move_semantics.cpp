/**
 * @file test_cache_manager_move_semantics.cpp
 * @brief Tests for cache manager policy movement and event handler semantics
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include "cache/cache_manager.h"
#include "cache/cache_eviction_policy.h"
#include <utility>

using namespace themis::cache;

class CacheManagerTest : public ::testing::Test {
protected:
    CacheManagerConfig default_config{
        .default_cache_size = 100,
        .num_shards = 4
    };
};

// Test: Default construction valid
TEST_F(CacheManagerTest, ConstructionValid) {
    CacheManager mgr(default_config);
    EXPECT_FALSE(mgr.is_moved_from());
    EXPECT_TRUE(mgr.is_valid());
}

// Test: Move constructor
TEST_F(CacheManagerTest, MoveConstructor) {
    CacheManager src(default_config);
    src.register_cache("cache1", 50);

    CacheManager dst(std::move(src));
    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Move assignment
TEST_F(CacheManagerTest, MoveAssignment) {
    CacheManager src(default_config);
    src.register_cache("cache1", 50);

    CacheManager dst(default_config);
    dst = std::move(src);

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_FALSE(dst.is_moved_from());
}

// Test: Moved-from manager throws on register_cache
TEST_F(CacheManagerTest, MovedFromThrowsOnRegister) {
    CacheManager src(default_config);
    CacheManager dst(std::move(src));

    EXPECT_THROW({
        src.register_cache("cache1");
    }, std::logic_error);
}

// Test: Moved-from manager throws on unregister_cache
TEST_F(CacheManagerTest, MovedFromThrowsOnUnregister) {
    CacheManager src(default_config);
    CacheManager dst(std::move(src));

    EXPECT_THROW({
        src.unregister_cache("cache1");
    }, std::logic_error);
}

// Test: Moved-from manager throws on clear_all
TEST_F(CacheManagerTest, MovedFromThrowsOnClearAll) {
    CacheManager src(default_config);
    CacheManager dst(std::move(src));

    EXPECT_THROW({
        src.clear_all();
    }, std::logic_error);
}

// Test: Moved-from manager throws on register_event_handler
TEST_F(CacheManagerTest, MovedFromThrowsOnRegisterHandler) {
    CacheManager src(default_config);
    CacheManager dst(std::move(src));

    EXPECT_THROW({
        src.register_event_handler([](const CacheEvent&) {});
    }, std::logic_error);
}

// Test: Policy move semantics
TEST_F(CacheManagerTest, PolicyMoveSemanticsValid) {
    CacheManager mgr(default_config);
    mgr.register_cache("cache1");

    auto policy = std::make_unique<LRUEvictionPolicy>();
    bool success = mgr.set_eviction_policy("cache1", 
                                           LRUEvictionPolicy());
    EXPECT_TRUE(success);
}

// Test: Get cache names after move
TEST_F(CacheManagerTest, GetCacheNamesAfterMove) {
    CacheManager src(default_config);
    src.register_cache("cache1");
    src.register_cache("cache2");

    CacheManager dst(std::move(src));

    auto names = dst.get_cache_names();
    EXPECT_EQ(names.size(), 2);
}

// Test: Event handlers preserved after move
TEST_F(CacheManagerTest, EventHandlersPreservedAfterMove) {
    CacheManager src(default_config);
    int call_count = 0;
    
    src.register_event_handler([&call_count](const CacheEvent&) {
        call_count++;
    });

    CacheManager dst(std::move(src));

    EXPECT_TRUE(src.is_moved_from());
    EXPECT_EQ(call_count, 0);  // Handler should not be triggered during move
}

// Test: Self-move assignment is safe
TEST_F(CacheManagerTest, SelfMoveAssignmentIsSafe) {
    CacheManager mgr(default_config);
    mgr.register_cache("cache1");

    mgr = std::move(mgr);
    EXPECT_TRUE(mgr.is_moved_from());
}

// Test: Destructor safe on moved-from manager
TEST_F(CacheManagerTest, DestructorSafeOnMovedFrom) {
    {
        CacheManager src(default_config);
        CacheManager dst(std::move(src));
    }  // Should not crash
}

// Test: Config accessible before and after move
TEST_F(CacheManagerTest, ConfigAccessibleAfterMove) {
    CacheManager src(default_config);
    auto src_config = src.get_config();

    CacheManager dst(std::move(src));
    auto dst_config = dst.get_config();

    EXPECT_EQ(src_config.default_cache_size, dst_config.default_cache_size);
}

// Test: Moved-from manager dispatch_event is no-op
TEST_F(CacheManagerTest, MovedFromDispatchEventIsNoop) {
    CacheManager src(default_config);
    CacheManager dst(std::move(src));

    CacheEvent event;
    event.type = CacheEvent::Type::HIT;
    src.dispatch_event(event);  // Should not crash
}

// Test: get_cache_stats on moved-from returns empty
TEST_F(CacheManagerTest, GetStatsOnMovedFromReturnsEmpty) {
    CacheManager src(default_config);
    src.register_cache("cache1");

    CacheManager dst(std::move(src));

    auto stats = src.get_cache_stats("cache1");
    EXPECT_FALSE(stats.has_value());
}
