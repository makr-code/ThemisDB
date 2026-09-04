#include <gtest/gtest.h>
#include "llm/model_metadata_cache.h"
#include "llm/lora_metadata_cache.h"
#include "llm/paged_block_manager.h"
#include "utils/type_conversion.h"
#include <thread>
#include <vector>

using namespace themis::llm;
using themis::utils::conversion::safe_size_to_int;

// ============================================================================
// ModelMetadataCache Tests
// ============================================================================

TEST(ModelMetadataCacheTest, PutAndGet) {
    ModelMetadataCache cache;
    
    ModelMetadata meta;
    meta.model_id = "test-model";
    meta.path = "/models/test.gguf";
    meta.size_bytes = 1000000;
    meta.n_layers = 32;
    meta.n_ctx = 4096;
    
    cache.put("test-model", meta);
    
    auto retrieved = cache.get("test-model");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->model_id, "test-model");
    EXPECT_EQ(retrieved->size_bytes, 1000000);
}

TEST(ModelMetadataCacheTest, Touch) {
    ModelMetadataCache cache;
    
    ModelMetadata meta;
    meta.model_id = "test-model";
    meta.path = "/models/test.gguf";
    meta.access_count = 0;
    
    cache.put("test-model", meta);
    cache.touch("test-model");
    
    auto retrieved = cache.get("test-model");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->access_count, 1);
}

TEST(ModelMetadataCacheTest, GetStats) {
    ModelMetadataCache cache;
    
    ModelMetadata meta1;
    meta1.model_id = "model1";
    meta1.size_bytes = 1000000;
    meta1.is_pinned = true;
    
    ModelMetadata meta2;
    meta2.model_id = "model2";
    meta2.size_bytes = 2000000;
    meta2.is_pinned = false;
    
    cache.put("model1", meta1);
    cache.put("model2", meta2);
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.total_entries, 2);
    EXPECT_EQ(stats.pinned_entries, 1);
    EXPECT_EQ(stats.total_size_bytes, 3000000);
}

TEST(ModelMetadataCacheTest, ConcurrentAccess) {
    ModelMetadataCache cache;
    
    // Pre-populate cache
    for (int i = 0; i < 10; i++) {
        ModelMetadata meta;
        meta.model_id = "model" + std::to_string(i);
        meta.size_bytes = 1000000 * i;
        cache.put(meta.model_id, meta);
    }
    
    // Concurrent reads (lock-free)
    std::vector<std::thread> threads = {};

    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&cache]() {
            for (int i = 0; i < 100; i++) {
                auto meta = cache.get("model" + std::to_string(i % 10));
                EXPECT_TRUE(meta.has_value());
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    EXPECT_EQ(cache.size(), 10);
}

// ============================================================================
// LoRAMetadataCache Tests
// ============================================================================

TEST(LoRAMetadataCacheTest, PutAndGet) {
    LoRAMetadataCache cache;
    
    LoRAMetadata meta;
    meta.lora_id = "test-lora";
    meta.path = "/loras/test.bin";
    meta.base_model_id = "base-model";
    meta.size_bytes = 50000;
    meta.rank = 16;
    
    cache.put("test-lora", meta);
    
    auto retrieved = cache.get("test-lora");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->lora_id, "test-lora");
    EXPECT_EQ(retrieved->rank, 16);
}

TEST(LoRAMetadataCacheTest, MarkLoadedUnloaded) {
    LoRAMetadataCache cache;
    
    LoRAMetadata meta;
    meta.lora_id = "test-lora";
    meta.is_loaded = false;
    meta.slot_id = -1;
    
    cache.put("test-lora", meta);
    cache.markLoaded("test-lora", 5);
    
    auto retrieved = cache.get("test-lora");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_TRUE(retrieved->is_loaded);
    EXPECT_EQ(retrieved->slot_id, 5);
    
    cache.markUnloaded("test-lora");
    retrieved = cache.get("test-lora");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_FALSE(retrieved->is_loaded);
    EXPECT_EQ(retrieved->slot_id, -1);
}

TEST(LoRAMetadataCacheTest, GetStats) {
    LoRAMetadataCache cache;
    
    LoRAMetadata meta1;
    meta1.lora_id = "lora1";
    meta1.size_bytes = 10000;
    meta1.is_loaded = true;
    
    LoRAMetadata meta2;
    meta2.lora_id = "lora2";
    meta2.size_bytes = 20000;
    meta2.is_loaded = false;
    
    cache.put("lora1", meta1);
    cache.put("lora2", meta2);
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.total_entries, 2);
    EXPECT_EQ(stats.loaded_entries, 1);
    EXPECT_EQ(stats.total_size_bytes, 30000);
}

// ============================================================================
// PagedBlockManager Tests
// ============================================================================

TEST(PagedBlockManagerTest, Initialization) {
    PagedBlockManager::Config config;
    config.max_blocks = 100;
    config.block_size_tokens = 128;
    
    PagedBlockManager mgr(config);
    
    auto stats = mgr.getStats();
    EXPECT_EQ(stats.num_blocks, 100);
    EXPECT_EQ(stats.num_free_blocks, 100);
    EXPECT_EQ(stats.num_allocated_blocks, 0);
}

TEST(PagedBlockManagerTest, AllocateAndFree) {
    PagedBlockManager::Config config;
    config.max_blocks = 100;
    
    PagedBlockManager mgr(config);
    
    // Allocate blocks
    auto block_ids = mgr.allocateBlocks(10);
    EXPECT_EQ(block_ids.size(), 10);
    EXPECT_EQ(mgr.getNumFreeBlocks(), 90);
    
    // Free blocks
    mgr.freeBlocks(block_ids);
    EXPECT_EQ(mgr.getNumFreeBlocks(), 100);
}

TEST(PagedBlockManagerTest, AllocationFailure) {
    PagedBlockManager::Config config;
    config.max_blocks = 10;
    
    PagedBlockManager mgr(config);
    
    // Try to allocate more than available
    auto block_ids = mgr.allocateBlocks(20);
    EXPECT_TRUE(block_ids.empty());
    EXPECT_EQ(mgr.getNumFreeBlocks(), 10);
}

TEST(PagedBlockManagerTest, GetBlock) {
    PagedBlockManager::Config config;
    config.max_blocks = 100;
    config.block_size_tokens = 128;
    
    PagedBlockManager mgr(config);
    
    auto block_ids = mgr.allocateBlocks(1);
    ASSERT_FALSE(block_ids.empty());
    
    // Test callback pattern (withBlock)
    bool callback_executed = false;
    mgr.withBlock(block_ids[0], [&](const PagedBlockManager::Block& block) {
        callback_executed = true;
        EXPECT_EQ(block.block_id, block_ids[0]);
        EXPECT_FALSE(block.is_free);
    });
    EXPECT_TRUE(callback_executed);
    
    // Test reference wrapper pattern (getBlockRef)
    auto block_ref = mgr.getBlockRef(block_ids[0]);
    ASSERT_TRUE(block_ref.has_value());
    const PagedBlockManager::Block& block = block_ref->get();
    EXPECT_EQ(block.block_id, block_ids[0]);
    EXPECT_FALSE(block.is_free);
}

TEST(PagedBlockManagerTest, ConcurrentAllocation) {
    PagedBlockManager::Config config;
    config.max_blocks = 1000;
    
    PagedBlockManager mgr(config);
    
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> all_allocated(10);
    
    // Allocate from multiple threads
    for (int t = 0; t < 10; t++) {
        threads.emplace_back([&mgr, &all_allocated, t]() {
            all_allocated[t] = mgr.allocateBlocks(50);
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    // Verify all allocations succeeded
    int total_allocated = 0;
    for (const auto& ids : all_allocated) {
        total_allocated += safe_size_to_int(ids.size());
    }
    
    EXPECT_EQ(total_allocated, 500);
    EXPECT_EQ(mgr.getNumFreeBlocks(), 500);
}

TEST(PagedBlockManagerTest, Reset) {
    PagedBlockManager::Config config;
    config.max_blocks = 100;
    
    PagedBlockManager mgr(config);
    
    // Allocate some blocks
    auto block_ids = mgr.allocateBlocks(50);
    EXPECT_EQ(mgr.getNumFreeBlocks(), 50);
    
    // Reset
    mgr.reset();
    EXPECT_EQ(mgr.getNumFreeBlocks(), 100);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(LLMCachingIntegrationTest, ModelAndLoRAMetadata) {
    ModelMetadataCache model_cache;
    LoRAMetadataCache lora_cache;
    
    // Add model
    ModelMetadata model_meta;
    model_meta.model_id = "base-model";
    model_meta.size_bytes = 6400000000;
    model_cache.put("base-model", model_meta);
    
    // Add LoRAs for this model
    for (int i = 0; i < 5; i++) {
        LoRAMetadata lora_meta;
        lora_meta.lora_id = "lora" + std::to_string(i);
        lora_meta.base_model_id = "base-model";
        lora_meta.size_bytes = 10000000;
        lora_cache.put(lora_meta.lora_id, lora_meta);
    }
    
    // Verify
    auto model = model_cache.get("base-model");
    ASSERT_TRUE(model.has_value());
    
    auto lora_stats = lora_cache.getStats();
    EXPECT_EQ(lora_stats.total_entries, 5);
}

TEST(LLMCachingIntegrationTest, HighConcurrency) {
    ModelMetadataCache model_cache;
    PagedBlockManager mgr({.max_blocks = 10000});
    
    std::vector<std::thread> threads;
    
    // Mix of operations
    for (int t = 0; t < 20; t++) {
        threads.emplace_back([&, t]() {
            // Add models
            for (int i = 0; i < 10; i++) {
                ModelMetadata meta;
                meta.model_id = "model_" + std::to_string(t) + "_" + std::to_string(i);
                meta.size_bytes = 1000000;
                model_cache.put(meta.model_id, meta);
            }
            
            // Allocate blocks
            auto blocks = mgr.allocateBlocks(10);
            
            // Read models
            for (int i = 0; i < 10; i++) {
                auto meta = model_cache.get("model_" + std::to_string(t) + "_" + std::to_string(i));
                EXPECT_TRUE(meta.has_value());
            }
            
            // Free blocks
            if (!blocks.empty()) {
                mgr.freeBlocks(blocks);
            }
        });
    }
    
    for (auto& th : threads) {
        th.join();
    }
    
    EXPECT_EQ(model_cache.size(), 200);
    EXPECT_GT(mgr.getNumFreeBlocks(), 9000);
}

// No custom main; gtest_main provides the entry point
