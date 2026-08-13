#include "sharding/paged_kv_cache.h"

#include <gtest/gtest.h>

namespace themisdb::sharding {
namespace {

KVCacheConfig makeConfig() {
    KVCacheConfig config;
    config.block_size = 4;
    config.max_blocks_per_request = 8;
    config.max_total_blocks = 16;
    config.max_cache_memory_bytes = 1U << 20;
    config.preallocate_blocks = false;
    config.eviction_threshold = 0.95;
    return config;
}

TEST(PagedKVCacheFocusedTest, WriteReadRoundTripPersistsPayload) {
    PagedKVCache cache(makeConfig());
    ASSERT_TRUE(cache.initialize());
    ASSERT_TRUE(cache.reserveRequest(11, std::vector<int>{10, 20, 30, 40}));

    auto block_id = cache.allocateBlock(11, 4);
    ASSERT_TRUE(block_id.has_value());

    const std::vector<float> keys{1.0f, 2.0f, 3.0f, 4.0f};
    const std::vector<float> values{5.0f, 6.0f, 7.0f, 8.0f};
    ASSERT_TRUE(cache.writeBlock(*block_id, 0, keys, values, 4));

    std::vector<float> read_keys;
    std::vector<float> read_values;
    ASSERT_TRUE(cache.readBlock(*block_id, 0, 4, read_keys, read_values));
    EXPECT_EQ(read_keys, keys);
    EXPECT_EQ(read_values, values);
}

TEST(PagedKVCacheFocusedTest, FindSharedPrefixReturnsReusableFullBlock) {
    PagedKVCache cache(makeConfig());
    ASSERT_TRUE(cache.initialize());

    const std::vector<int> source_tokens{1, 2, 3, 4, 5, 6, 7, 8};
    const std::vector<int> target_tokens{1, 2, 3, 4, 99, 100};

    ASSERT_TRUE(cache.reserveRequest(21, source_tokens));
    auto first_block = cache.allocateBlock(21, 4);
    auto second_block = cache.allocateBlock(21, 4);
    ASSERT_TRUE(first_block.has_value());
    ASSERT_TRUE(second_block.has_value());

    ASSERT_TRUE(cache.reserveRequest(22, target_tokens));
    auto shared_block = cache.findSharedPrefix(22, target_tokens);
    ASSERT_TRUE(shared_block.has_value());
    EXPECT_EQ(*shared_block, *first_block);
}

TEST(PagedKVCacheFocusedTest, SharedBlockSurvivesSourceRequestCleanup) {
    PagedKVCache cache(makeConfig());
    ASSERT_TRUE(cache.initialize());

    const std::vector<int> tokens{7, 8, 9, 10};
    ASSERT_TRUE(cache.reserveRequest(31, tokens));
    ASSERT_TRUE(cache.reserveRequest(32, tokens));

    auto block_id = cache.allocateBlock(31, 4);
    ASSERT_TRUE(block_id.has_value());

    const std::vector<float> keys{11.0f, 12.0f, 13.0f, 14.0f};
    const std::vector<float> values{21.0f, 22.0f, 23.0f, 24.0f};
    ASSERT_TRUE(cache.writeBlock(*block_id, 0, keys, values, 4));
    ASSERT_TRUE(cache.sharePrefixBlock(31, 32, *block_id));

    cache.freeRequest(31);

    auto target_blocks = cache.getRequestBlocks(32);
    ASSERT_EQ(target_blocks.size(), 1U);
    EXPECT_EQ(target_blocks.front(), *block_id);

    std::vector<float> read_keys;
    std::vector<float> read_values;
    ASSERT_TRUE(cache.readBlock(*block_id, 0, 4, read_keys, read_values));
    EXPECT_EQ(read_keys, keys);
    EXPECT_EQ(read_values, values);

    cache.freeRequest(32);
    EXPECT_FALSE(cache.getBlock(*block_id).has_value());
    EXPECT_TRUE(cache.getRequestBlocks(32).empty());
}

}  // namespace
}  // namespace themisdb::sharding
