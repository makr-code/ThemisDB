#include <gtest/gtest.h>
#include "llm/block_table.h"
#include "llm/paged_kv_cache.h"
#include "llm/paged_block_manager.h"
#include <memory>
#include <chrono>

using namespace themis::llm;

class BlockTableTest : public ::testing::Test {
protected:
    void SetUp() override {
        PagedBlockManager::Config config;
        config.total_blocks = 1000;
        block_manager = std::make_shared<PagedBlockManager>(config);
    }

    std::shared_ptr<PagedBlockManager> block_manager;
};

TEST_F(BlockTableTest, AllocateBlocks) {
    BlockTable::Config config;
    BlockTable table(block_manager, 1, config);
    
    auto blocks = table.allocateBlocks(10);
    EXPECT_EQ(blocks.size(), 10);
    
    auto mapping = table.getBlockMapping();
    EXPECT_EQ(mapping.size(), 10);
}

TEST_F(BlockTableTest, ReleaseBlocks) {
    BlockTable::Config config;
    BlockTable table(block_manager, 1, config);
    
    table.allocateBlocks(10);
    table.releaseBlocks();
    
    auto mapping = table.getBlockMapping();
    EXPECT_EQ(mapping.size(), 0);
}

TEST_F(BlockTableTest, SharePrefix) {
    BlockTable::Config config;
    config.enable_cow = true;
    
    BlockTable table(block_manager, 1, config);
    table.allocateBlocks(10);
    
    table.sharePrefix(0, 5 * config.block_size);
    
    auto stats = table.getStats();
    EXPECT_GT(stats.num_shared_blocks, 0);
}

TEST_F(BlockTableTest, GetStats) {
    BlockTable::Config config;
    BlockTable table(block_manager, 1, config);
    table.allocateBlocks(10);
    
    auto stats = table.getStats();
    EXPECT_EQ(stats.num_blocks, 10);
}

class PagedKVCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        PagedBlockManager::Config bm_config;
        bm_config.total_blocks = 1000;
        block_manager = std::make_shared<PagedBlockManager>(bm_config);
        
        PagedKVCache::Config cache_config;
        cache_config.num_blocks = 1000;
        cache = std::make_unique<PagedKVCache>(cache_config, block_manager);
    }

    std::shared_ptr<PagedBlockManager> block_manager;
    std::unique_ptr<PagedKVCache> cache;
};

TEST_F(PagedKVCacheTest, StoreRetrieve) {
    uint64_t seq_id = 1;
    size_t layer_id = 0;
    
    // Create dummy KV data (2 * num_kv_heads * head_dim * num_tokens)
    std::vector<float> kv_data(2 * 8 * 128 * 16, 1.0f);  // 16 tokens
    
    cache->store(seq_id, layer_id, kv_data);
    
    auto retrieved = cache->retrieve(seq_id, layer_id);
    EXPECT_EQ(retrieved.size(), kv_data.size());
}

TEST_F(PagedKVCacheTest, PrefixSharing) {
    uint64_t parent_seq = 1;
    uint64_t child_seq = 2;
    
    // Store data for parent
    std::vector<float> kv_data(2 * 8 * 128 * 32, 1.0f);  // 32 tokens
    cache->store(parent_seq, 0, kv_data);
    
    // Share prefix with child
    cache->sharePrefix(child_seq, parent_seq, 16);  // Share 16 tokens
    
    auto child_table = cache->getBlockTable(child_seq);
    ASSERT_NE(child_table, nullptr);
    
    auto stats = child_table->getStats();
    EXPECT_GT(stats.sharing_ratio, 0.0);
}

TEST_F(PagedKVCacheTest, RemoveSequence) {
    uint64_t seq_id = 1;
    std::vector<float> kv_data(2 * 8 * 128 * 16, 1.0f);
    
    cache->store(seq_id, 0, kv_data);
    cache->removeSequence(seq_id);
    
    auto retrieved = cache->retrieve(seq_id, 0);
    EXPECT_EQ(retrieved.size(), 0);
}

TEST_F(PagedKVCacheTest, GetStats) {
    // Store multiple sequences
    for (uint64_t seq = 1; seq <= 5; ++seq) {
        std::vector<float> kv_data(2 * 8 * 128 * 16, 1.0f);
        cache->store(seq, 0, kv_data);
    }
    
    auto stats = cache->getStats();
    EXPECT_EQ(stats.num_sequences, 5);
    EXPECT_GT(stats.blocks_used, 0);
}

TEST_F(PagedKVCacheTest, MemoryEfficiency) {
    // Test that block-based storage is more efficient than naive storage
    
    // Store 100 sequences with varying lengths
    for (uint64_t seq = 1; seq <= 100; ++seq) {
        size_t num_tokens = 10 + (seq % 20);  // 10-30 tokens
        std::vector<float> kv_data(2 * 8 * 128 * num_tokens, 1.0f);
        cache->store(seq, 0, kv_data);
    }
    
    auto stats = cache->getStats();
    
    // With 16 tokens per block, we should have low fragmentation
    EXPECT_LT(stats.fragmentation_rate, 0.1);  // <10% fragmentation
}

// Benchmark: Block allocation latency
TEST(PagedKVCacheBenchmark, BlockAllocationLatency) {
    PagedBlockManager::Config bm_config;
    bm_config.total_blocks = 10000;
    auto block_manager = std::make_shared<PagedBlockManager>(bm_config);
    
    BlockTable::Config config;
    BlockTable table(block_manager, 1, config);
    
    auto start = std::chrono::high_resolution_clock::now();
    table.allocateBlocks(1000);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Should allocate 1000 blocks in < 1ms
    EXPECT_LT(duration.count(), 1000);
}

// Benchmark: Prefix sharing overhead
TEST(PagedKVCacheBenchmark, PrefixSharingOverhead) {
    PagedBlockManager::Config bm_config;
    bm_config.total_blocks = 10000;
    auto block_manager = std::make_shared<PagedBlockManager>(bm_config);
    
    PagedKVCache::Config cache_config;
    cache_config.num_blocks = 10000;
    cache_config.enable_prefix_caching = true;
    PagedKVCache cache(cache_config, block_manager);
    
    // Store parent sequence
    std::vector<float> kv_data(2 * 8 * 128 * 512, 1.0f);  // 512 tokens
    cache.store(1, 0, kv_data);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create 100 child sequences sharing prefix
    for (uint64_t child = 2; child <= 101; ++child) {
        cache.sharePrefix(child, 1, 256);  // Share 256 tokens
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Sharing 100 prefixes should be < 100μs (< 1μs per share)
    EXPECT_LT(duration.count(), 100);
}

// No custom main; gtest_main provides the entry point
