#include <gtest/gtest.h>

#include "config/lru_cache.h"

#include <string>

namespace {

using themis::config::LRUCacheWithTTL;

TEST(ConfigLruBoundaryTest, UpdatingFullCachePromotesEntryWithoutEviction) {
    LRUCacheWithTTL<std::string, int> cache(2, 3600);
    cache.put("a", 1);
    cache.put("b", 2);
    cache.put("a", 10);
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_EQ(cache.stats().evictions, 0u);

    cache.put("c", 3);
    EXPECT_FALSE(cache.get("b").has_value());
    ASSERT_TRUE(cache.get("a").has_value());
    EXPECT_EQ(*cache.get("a"), 10);
    EXPECT_EQ(cache.stats().evictions, 1u);
}

TEST(ConfigLruBoundaryTest, RemovesInterleavedExpiredEntriesWithoutDroppingLiveEntries) {
    LRUCacheWithTTL<std::string, int> cache(6, 3600);
    cache.put("expired-old", 1, 0);
    cache.put("live-old", 2);
    cache.put("expired-middle", 3, 0);
    cache.put("live-new", 4);
    cache.put("expired-new", 5, 0);

    cache.removeExpired();
    EXPECT_EQ(cache.size(), 2u);
    EXPECT_EQ(cache.stats().expirations, 3u);
    EXPECT_EQ(cache.get("live-old"), 2);
    EXPECT_EQ(cache.get("live-new"), 4);
    cache.removeExpired();
    EXPECT_EQ(cache.stats().expirations, 3u);
}

TEST(ConfigLruBoundaryTest, ReplacingExpiredEntryRefreshesTtlAndDoesNotEvict) {
    LRUCacheWithTTL<std::string, int> cache(1, 3600);
    cache.put("key", 1, 0);
    cache.put("key", 2);
    cache.removeExpired();
    EXPECT_EQ(cache.get("key"), 2);
    EXPECT_EQ(cache.stats().evictions, 0u);
    EXPECT_EQ(cache.stats().expirations, 0u);
}

} // namespace
