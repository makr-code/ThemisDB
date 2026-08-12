#include <gtest/gtest.h>
#include "query/cte_cache.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <thread>
#include <atomic>

using namespace themis::query;
using json = nlohmann::json;

class CTECacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any previous test artifacts
        if (std::filesystem::exists("./test_cte_spill")) {
            std::filesystem::remove_all("./test_cte_spill");
        }
    }
    
    void TearDown() override {
        // Clean up test artifacts
        if (std::filesystem::exists("./test_cte_spill")) {
            std::filesystem::remove_all("./test_cte_spill");
        }
    }
    
    // Helper to create test data
    std::vector<json> createTestData(size_t count) {
        std::vector<json> data;
        for (size_t i = 0; i < count; ++i) {
            data.push_back({
                {"id", i},
                {"name", "user_" + std::to_string(i)},
                {"value", i * 100}
            });
        }
        return data;
    }
};

// ============================================================================
// Phase 4.3: Basic CTECache Tests
// ============================================================================

TEST_F(CTECacheTest, BasicStoreAndGet) {
    CTECache::Config config;
    config.max_memory_bytes = 10 * 1024 * 1024; // 10MB
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    auto data = createTestData(100);
    ASSERT_TRUE(cache.store("test_cte", data));
    
    auto retrieved = cache.get("test_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 100);
    EXPECT_EQ((*retrieved)[0]["id"], 0);
    EXPECT_EQ((*retrieved)[99]["id"], 99);
}

TEST_F(CTECacheTest, MultipleCTEs) {
    CTECache::Config config;
    config.max_memory_bytes = 10 * 1024 * 1024;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    ASSERT_TRUE(cache.store("cte1", createTestData(50)));
    ASSERT_TRUE(cache.store("cte2", createTestData(75)));
    ASSERT_TRUE(cache.store("cte3", createTestData(100)));
    
    EXPECT_TRUE(cache.contains("cte1"));
    EXPECT_TRUE(cache.contains("cte2"));
    EXPECT_TRUE(cache.contains("cte3"));
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.total_ctes, 3);
}

TEST_F(CTECacheTest, RemoveCTE) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    auto data = createTestData(50);
    ASSERT_TRUE(cache.store("temp_cte", data));
    EXPECT_TRUE(cache.contains("temp_cte"));
    
    cache.remove("temp_cte");
    EXPECT_FALSE(cache.contains("temp_cte"));
    
    auto retrieved = cache.get("temp_cte");
    EXPECT_FALSE(retrieved.has_value());
}

// ============================================================================
// Phase 4.3: Spill-to-Disk Tests
// ============================================================================

TEST_F(CTECacheTest, AutomaticSpillToDisk) {
    CTECache::Config config;
    config.max_memory_bytes = 50 * 1024; // Very small: 50KB to force spill
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    // Store a large CTE that exceeds memory limit
    auto large_data = createTestData(1000); // Should be >50KB
    ASSERT_TRUE(cache.store("large_cte", large_data));
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.spill_operations, 0); // Should have spilled
    
    // Verify we can still retrieve it
    auto retrieved = cache.get("large_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 1000);
    EXPECT_EQ((*retrieved)[0]["id"], 0);
    EXPECT_EQ((*retrieved)[999]["id"], 999);
    
    auto post_stats = cache.getStats();
    EXPECT_GT(post_stats.disk_reads, 0); // Should have read from disk
}

TEST_F(CTECacheTest, MultipleSpills) {
    CTECache::Config config;
    config.max_memory_bytes = 100 * 1024; // 100KB
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    // Store multiple large CTEs to trigger multiple spills
    ASSERT_TRUE(cache.store("cte1", createTestData(500)));
    ASSERT_TRUE(cache.store("cte2", createTestData(500)));
    ASSERT_TRUE(cache.store("cte3", createTestData(500)));
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.spill_operations, 0);
    
    // All CTEs should still be retrievable
    EXPECT_TRUE(cache.contains("cte1"));
    EXPECT_TRUE(cache.contains("cte2"));
    EXPECT_TRUE(cache.contains("cte3"));
    
    auto cte1 = cache.get("cte1");
    auto cte2 = cache.get("cte2");
    auto cte3 = cache.get("cte3");
    
    ASSERT_TRUE(cte1.has_value());
    ASSERT_TRUE(cte2.has_value());
    ASSERT_TRUE(cte3.has_value());
    
    EXPECT_EQ(cte1->size(), 500);
    EXPECT_EQ(cte2->size(), 500);
    EXPECT_EQ(cte3->size(), 500);
}

TEST_F(CTECacheTest, SpillFileCleanup) {
    {
        CTECache::Config config;
        config.max_memory_bytes = 10 * 1024; // 10KB to force spill
        config.spill_directory = "./test_cte_spill";
        config.auto_cleanup = true;
        
        CTECache cache(config);
        
        // Create spill files
        ASSERT_TRUE(cache.store("spill1", createTestData(200)));
        ASSERT_TRUE(cache.store("spill2", createTestData(200)));
        
        EXPECT_TRUE(std::filesystem::exists("./test_cte_spill"));
        
        // Cache destructor should clean up files
    }
    
    // After cache is destroyed, spill directory should be gone
    EXPECT_FALSE(std::filesystem::exists("./test_cte_spill"));
}

// ============================================================================
// Phase 4.3: Memory Management Tests
// ============================================================================

TEST_F(CTECacheTest, MemoryUsageTracking) {
    CTECache::Config config;
    config.max_memory_bytes = 10 * 1024 * 1024; // 10MB
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    EXPECT_EQ(cache.getCurrentMemoryUsage(), 0);
    
    auto data = createTestData(100);
    ASSERT_TRUE(cache.store("test_cte", data));
    
    EXPECT_GT(cache.getCurrentMemoryUsage(), 0);
    
    cache.remove("test_cte");
    EXPECT_EQ(cache.getCurrentMemoryUsage(), 0);
}

TEST_F(CTECacheTest, ClearCache) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    ASSERT_TRUE(cache.store("cte1", createTestData(50)));
    ASSERT_TRUE(cache.store("cte2", createTestData(50)));
    ASSERT_TRUE(cache.store("cte3", createTestData(50)));
    
    EXPECT_EQ(cache.size(), 3);
    EXPECT_GT(cache.getCurrentMemoryUsage(), 0);
    
    cache.clear();
    
    EXPECT_EQ(cache.size(), 0);
    EXPECT_EQ(cache.getCurrentMemoryUsage(), 0);
    EXPECT_FALSE(cache.contains("cte1"));
    EXPECT_FALSE(cache.contains("cte2"));
    EXPECT_FALSE(cache.contains("cte3"));
}

TEST_F(CTECacheTest, StatsAccumulation) {
    CTECache::Config config;
    config.max_memory_bytes = 50 * 1024; // Small to force spills
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    // Store and retrieve to accumulate stats
    ASSERT_TRUE(cache.store("cte1", createTestData(500)));
    ASSERT_TRUE(cache.store("cte2", createTestData(500)));
    
    auto stats1 = cache.getStats();
    
    // Trigger disk reads
    auto cte1 = cache.get("cte1");
    auto cte2 = cache.get("cte2");
    
    auto stats2 = cache.getStats();
    
    EXPECT_GE(stats2.disk_reads, stats1.disk_reads);
    EXPECT_EQ(stats2.total_ctes, 2);
    EXPECT_EQ(stats2.total_results, 1000);
}

// ============================================================================
// Phase 4.3: Edge Cases
// ============================================================================

TEST_F(CTECacheTest, EmptyResults) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    std::vector<json> empty_data;
    ASSERT_TRUE(cache.store("empty_cte", empty_data));
    
    auto retrieved = cache.get("empty_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 0);
}

TEST_F(CTECacheTest, NonExistentCTE) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    auto result = cache.get("nonexistent");
    EXPECT_FALSE(result.has_value());
    
    EXPECT_FALSE(cache.contains("nonexistent"));
    cache.remove("nonexistent");
}

TEST_F(CTECacheTest, OverwriteCTE) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    config.auto_cleanup = true;
    
    CTECache cache(config);
    
    auto data1 = createTestData(50);
    ASSERT_TRUE(cache.store("overwrite_cte", data1));
    
    auto data2 = createTestData(100);
    ASSERT_TRUE(cache.store("overwrite_cte", data2)); // Overwrite
    
    auto retrieved = cache.get("overwrite_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 100); // Should have new data
}

// ============================================================================
// CTE Cache Error Handling & Edge Case Tests
// ============================================================================

TEST_F(CTECacheTest, ErrorHandling_EmptyName_ReturnsFalse) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    auto data = createTestData(10);
    
    // Empty CTE name should be rejected
    EXPECT_FALSE(cache.store("", data));
    EXPECT_FALSE(cache.contains(""));
    
    auto retrieved = cache.get("");
    EXPECT_FALSE(retrieved.has_value());
}

TEST_F(CTECacheTest, ErrorHandling_EmptyData_StoresSuccessfully) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    std::vector<json> empty_data;
    
    // Empty data should be storable (represents empty result set)
    EXPECT_TRUE(cache.store("empty_cte", empty_data));
    EXPECT_TRUE(cache.contains("empty_cte"));
    
    auto retrieved = cache.get("empty_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 0);
}

TEST_F(CTECacheTest, ErrorHandling_DuplicateStore_Overwrites) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Store first version
    auto data1 = createTestData(50);
    ASSERT_TRUE(cache.store("cte", data1));
    
    // Store second version (should overwrite)
    auto data2 = createTestData(100);
    ASSERT_TRUE(cache.store("cte", data2));
    
    // Should have the second version
    auto retrieved = cache.get("cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 100);
}

TEST_F(CTECacheTest, ErrorHandling_GetNonExistent_ReturnsEmpty) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Get non-existent CTE should return empty optional
    auto retrieved = cache.get("nonexistent");
    EXPECT_FALSE(retrieved.has_value());
    
    // contains should return false
    EXPECT_FALSE(cache.contains("nonexistent"));
}

TEST_F(CTECacheTest, ErrorHandling_RemoveNonExistent_NoError) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Remove non-existent CTE should not crash
    EXPECT_NO_THROW(cache.remove("nonexistent"));
    
    // Stats should be unchanged
    auto stats = cache.getStats();
    EXPECT_EQ(stats.total_ctes, 0);
}

TEST_F(CTECacheTest, EdgeCase_VeryLargeCTEName_HandlesCorrectly) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Very long CTE name (1000 characters)
    std::string long_name(1000, 'x');
    auto data = createTestData(10);
    
    EXPECT_TRUE(cache.store(long_name, data));
    EXPECT_TRUE(cache.contains(long_name));
    
    auto retrieved = cache.get(long_name);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 10);
}

TEST_F(CTECacheTest, EdgeCase_SpecialCharactersInName_HandlesCorrectly) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // CTE name with special characters
    std::string special_name = "cte_with-special.chars!@#$%";
    auto data = createTestData(10);
    
    EXPECT_TRUE(cache.store(special_name, data));
    EXPECT_TRUE(cache.contains(special_name));
    
    auto retrieved = cache.get(special_name);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 10);
}

TEST_F(CTECacheTest, EdgeCase_NullValuesInData_HandlesCorrectly) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Data with null values
    std::vector<json> data_with_nulls = {
        {{"id", 1}, {"name", nullptr}, {"value", 100}},
        {{"id", 2}, {"name", "test"}, {"value", nullptr}},
        {{"id", 3}, {"name", nullptr}, {"value", nullptr}}
    };
    
    EXPECT_TRUE(cache.store("null_cte", data_with_nulls));
    
    auto retrieved = cache.get("null_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 3);
    EXPECT_TRUE((*retrieved)[0]["name"].is_null());
}

TEST_F(CTECacheTest, EdgeCase_VeryComplexNestedJSON_HandlesCorrectly) {
    CTECache::Config config;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Complex nested JSON structure
    std::vector<json> complex_data = {
        {
            {"id", 1},
            {"metadata", {
                {"nested", {
                    {"deep", {
                        {"value", 42}
                    }}
                }},
                {"array", {1, 2, 3, 4, 5}}
            }}
        }
    };
    
    EXPECT_TRUE(cache.store("complex_cte", complex_data));
    
    auto retrieved = cache.get("complex_cte");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ((*retrieved)[0]["metadata"]["nested"]["deep"]["value"], 42);
}

TEST_F(CTECacheTest, Stress_ManySmallCTEs_HandlesCorrectly) {
    CTECache::Config config;
    config.max_memory_bytes = 50 * 1024 * 1024; // 50MB
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Store many small CTEs
    const int CTE_COUNT = 1000;
    for (int i = 0; i < CTE_COUNT; ++i) {
        std::string name = "small_cte_" + std::to_string(i);
        auto data = createTestData(10); // Small dataset
        ASSERT_TRUE(cache.store(name, data));
    }
    
    auto stats = cache.getStats();
    EXPECT_EQ(stats.total_ctes, CTE_COUNT);
    
    // Verify random access
    auto retrieved = cache.get("small_cte_500");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->size(), 10);
}

TEST_F(CTECacheTest, Concurrent_MultipleAccessors_ThreadSafe) {
    CTECache::Config config;
    config.max_memory_bytes = 50 * 1024 * 1024;
    config.spill_directory = "./test_cte_spill";
    CTECache cache(config);
    
    // Prepare some data
    for (int i = 0; i < 10; ++i) {
        cache.store("cte_" + std::to_string(i), createTestData(100));
    }
    
    // Multiple threads accessing cache concurrently
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&cache, &success_count, t]() {
            for (int i = 0; i < 100; ++i) {
                std::string name = "cte_" + std::to_string(i % 10);
                auto retrieved = cache.get(name);
                if (retrieved.has_value()) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have successful retrievals
    EXPECT_GT(success_count.load(), 0);
}
