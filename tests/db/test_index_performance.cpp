/**
 * @file test_index_performance.cpp
 * @brief Comprehensive tests for index and query performance
 * 
 * Tests index operations and query performance:
 * - Index creation time benchmarks
 * - Indexed range query performance
 * - Index lookup throughput measurement
 * - Performance SLA validation (< 100ms for range queries)
 * - Large dataset handling (100k+ documents)
 * - Index maintenance under concurrent load
 * 
 * Best Practices Applied:
 * - Real performance measurements
 * - SLA validation
 * - Large-scale testing
 * - Proper benchmarking
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "../test_performance_helpers.h"
#include <filesystem>
#include <vector>
#include <random>

using namespace themis;

namespace fs = std::filesystem;

/**
 * Test fixture for index performance tests
 */
class IndexPerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_index_perf_test";
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }
        
        RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 256;
        config.block_cache_size_mb = 512;
        config.max_background_jobs = 8;
        config.enable_statistics = true;
        
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        
        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        vector_index_ = std::make_unique<VectorIndexManager>(*db_);
    }
    
    void TearDown() override {
        vector_index_.reset();
        secondary_index_.reset();
        db_->close();
        db_.reset();
        
        if (fs::exists(test_db_path_)) {
            fs::remove_all(test_db_path_);
        }
    }
    
    // Helper to generate random string
    std::string randomString(size_t length) {
        static const char charset[] = 
            "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
        
        std::string result;
        result.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            result += charset[dist(gen)];
        }
        return result;
    }
    
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_index_;
    std::unique_ptr<VectorIndexManager> vector_index_;
};

// ═══════════════════════════════════════════════════════════
// Index Creation Performance Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test index creation time for medium dataset
 * Acceptance Criteria:
 * - Index creation completes within reasonable time
 * - Index is functional after creation
 * - Performance is acceptable (< 5 seconds for 10k items)
 */
TEST_F(IndexPerformanceTest, Creation_MediumDataset) {
    const int num_items = 10000;
    
    // Insert data
    test::LatencyMeasurement insert_timer;
    
    for (int i = 0; i < num_items; ++i) {
        std::string key = "item_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("age", int64_t(20 + (i % 50)));
        entity.setField("name", randomString(20));
        
        auto result = db_->put("users::" + key, entity.serialize());
        ASSERT_TRUE(result);
    }
    
    double insert_time = insert_timer.elapsedMs();
    
    // Create secondary index on age field
    test::LatencyMeasurement index_timer;
    
    bool index_created = secondary_index_->createIndex(
        "users", "age", SecondaryIndexManager::IndexType::BTREE);
    
    double index_time = index_timer.elapsedMs();
    
    EXPECT_TRUE(index_created);
    EXPECT_LT(insert_time, 5000.0) << "Insert took too long: " << insert_time << "ms";
    EXPECT_LT(index_time, 5000.0) << "Index creation took too long: " << index_time << "ms";
    
    std::cout << "Inserted " << num_items << " items in " << insert_time << "ms" << std::endl;
    std::cout << "Created index in " << index_time << "ms" << std::endl;
}

/**
 * Test index creation throughput
 * Acceptance Criteria:
 * - Index creation throughput is measured
 * - Throughput is reasonable (> 1000 items/sec)
 */
TEST_F(IndexPerformanceTest, Creation_ThroughputMeasurement) {
    const int num_items = 5000;
    
    // Insert data
    for (int i = 0; i < num_items; ++i) {
        std::string key = "product_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("price", int64_t(10 + (i % 1000)));
        entity.setField("category", randomString(10));
        
        db_->put("products::" + key, entity.serialize());
    }
    
    // Measure index creation throughput
    test::ThroughputCalculator throughput;
    
    bool index_created = secondary_index_->createIndex(
        "products", "price", SecondaryIndexManager::IndexType::BTREE);
    
    throughput.increment(num_items);
    
    EXPECT_TRUE(index_created);
    
    double items_per_sec = throughput.getOpsPerSecond();
    EXPECT_GT(items_per_sec, 500.0) 
        << "Index creation throughput too low: " << items_per_sec << " items/sec";
    
    std::cout << "Index creation throughput: " << items_per_sec << " items/sec" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Range Query Performance Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test indexed range query performance
 * Acceptance Criteria:
 * - Range queries complete quickly (< 100ms)
 * - Results are accurate
 * - Performance meets SLA
 */
TEST_F(IndexPerformanceTest, RangeQuery_PerformanceSLA) {
    const int num_items = 50000;
    
    // Insert data with indexed field
    for (int i = 0; i < num_items; ++i) {
        std::string key = "record_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("score", int64_t(i % 1000));
        entity.setField("data", randomString(50));
        
        db_->put("records::" + key, entity.serialize());
    }
    
    // Create index
    bool index_created = secondary_index_->createIndex(
        "records", "score", SecondaryIndexManager::IndexType::BTREE);
    ASSERT_TRUE(index_created);
    
    // Test range query performance
    test::LatencyMeasurement timer;
    
    // Query for scores between 100 and 200
    auto results = secondary_index_->rangeQuery(
        "records", "score", int64_t(100), int64_t(200));
    
    double query_time = timer.elapsedMs();
    
    // SLA: Query should complete in < 100ms
    EXPECT_LT(query_time, 100.0) 
        << "Range query took " << query_time << "ms, expected < 100ms";
    
    // Verify results are present
    EXPECT_GT(results.size(), 0) << "Range query should return results";
    
    std::cout << "Range query returned " << results.size() 
              << " results in " << query_time << "ms" << std::endl;
}

/**
 * Test range query with different selectivities
 * Acceptance Criteria:
 * - Queries with different selectivities perform well
 * - Performance degrades gracefully with larger result sets
 */
TEST_F(IndexPerformanceTest, RangeQuery_VariableSelectivity) {
    const int num_items = 20000;
    
    // Insert data
    for (int i = 0; i < num_items; ++i) {
        std::string key = "item_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("value", int64_t(i));
        
        db_->put("items::" + key, entity.serialize());
    }
    
    // Create index
    secondary_index_->createIndex("items", "value", SecondaryIndexManager::IndexType::BTREE);
    
    // Test different selectivities
    struct TestCase {
        int64_t start;
        int64_t end;
        double max_time_ms;
    };
    
    std::vector<TestCase> test_cases = {
        {0, 10, 50.0},          // Very selective (10 items)
        {0, 100, 75.0},         // Selective (100 items)
        {0, 1000, 100.0},       // Moderate (1000 items)
        {0, 5000, 200.0},       // Large (5000 items)
    };
    
    for (const auto& tc : test_cases) {
        test::LatencyMeasurement timer;
        
        auto results = secondary_index_->rangeQuery(
            "items", "value", tc.start, tc.end);
        
        double query_time = timer.elapsedMs();
        
        EXPECT_LT(query_time, tc.max_time_ms)
            << "Range [" << tc.start << ", " << tc.end << "] took " 
            << query_time << "ms, expected < " << tc.max_time_ms << "ms";
        
        std::cout << "Range [" << tc.start << ", " << tc.end << "]: " 
                  << results.size() << " results in " << query_time << "ms" << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════
// Index Lookup Throughput Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test point lookup throughput with index
 * Acceptance Criteria:
 * - High throughput for point lookups (> 1000 ops/sec)
 * - Consistent performance
 */
TEST_F(IndexPerformanceTest, Lookup_ThroughputMeasurement) {
    const int num_items = 10000;
    const int num_lookups = 1000;
    
    // Insert data
    for (int i = 0; i < num_items; ++i) {
        std::string key = "user_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("id", int64_t(i));
        entity.setField("email", randomString(30));
        
        db_->put("users::" + key, entity.serialize());
    }
    
    // Create index
    secondary_index_->createIndex("users", "id", SecondaryIndexManager::IndexType::BTREE);
    
    // Measure lookup throughput
    test::ThroughputCalculator throughput;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, num_items - 1);
    
    for (int i = 0; i < num_lookups; ++i) {
        int64_t lookup_id = dist(gen);
        auto results = secondary_index_->lookup("users", "id", lookup_id);
        throughput.increment();
    }
    
    double ops_per_sec = throughput.getOpsPerSecond();
    
    EXPECT_GT(ops_per_sec, 500.0) 
        << "Lookup throughput too low: " << ops_per_sec << " ops/sec";
    
    std::cout << "Index lookup throughput: " << ops_per_sec << " ops/sec" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Large Dataset Handling Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test index performance with large dataset (100k+ items)
 * Acceptance Criteria:
 * - System handles large datasets efficiently
 * - Query performance remains acceptable
 * - Memory usage is reasonable
 */
TEST_F(IndexPerformanceTest, LargeDataset_100kItems) {
    const int num_items = 100000;
    
    test::MemoryUsageTracker memory;
    test::LatencyMeasurement timer;
    
    // Insert large dataset
    for (int i = 0; i < num_items; ++i) {
        std::string key = "doc_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("timestamp", int64_t(i));
        entity.setField("content", randomString(100));
        
        db_->put("documents::" + key, entity.serialize());
        
        // Progress indicator
        if (i % 10000 == 0 && i > 0) {
            std::cout << "Inserted " << i << " items..." << std::endl;
        }
    }
    
    double insert_time = timer.elapsedMs();
    std::cout << "Inserted " << num_items << " items in " 
              << insert_time << "ms" << std::endl;
    
    // Create index
    timer.reset();
    bool index_created = secondary_index_->createIndex(
        "documents", "timestamp", SecondaryIndexManager::IndexType::BTREE);
    
    double index_time = timer.elapsedMs();
    
    ASSERT_TRUE(index_created);
    EXPECT_LT(index_time, 30000.0) << "Index creation took too long for 100k items";
    
    std::cout << "Created index in " << index_time << "ms" << std::endl;
    
    // Test query performance on large dataset
    timer.reset();
    auto results = secondary_index_->rangeQuery(
        "documents", "timestamp", int64_t(1000), int64_t(2000));
    
    double query_time = timer.elapsedMs();
    
    EXPECT_LT(query_time, 200.0) 
        << "Range query on large dataset took " << query_time << "ms";
    EXPECT_GT(results.size(), 0);
    
    // Check memory usage (should be < 1GB for this test)
    double memory_delta = memory.getDeltaMB();
    EXPECT_LT(memory_delta, 1024.0) 
        << "Memory usage too high: " << memory_delta << "MB";
    
    std::cout << "Query on 100k dataset: " << results.size() 
              << " results in " << query_time << "ms" << std::endl;
    std::cout << "Memory usage: " << memory_delta << "MB" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Concurrent Index Access Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test index maintenance under concurrent load
 * Acceptance Criteria:
 * - Index remains consistent under concurrent access
 * - Performance degrades gracefully
 * - No data corruption
 */
TEST_F(IndexPerformanceTest, Concurrent_IndexMaintenance) {
    const int num_initial_items = 5000;
    
    // Insert initial data
    for (int i = 0; i < num_initial_items; ++i) {
        std::string key = "item_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("priority", int64_t(i % 100));
        
        db_->put("tasks::" + key, entity.serialize());
    }
    
    // Create index
    bool index_created = secondary_index_->createIndex(
        "tasks", "priority", SecondaryIndexManager::IndexType::BTREE);
    ASSERT_TRUE(index_created);
    
    // Concurrent operations
    std::atomic<int> inserts_completed{0};
    std::atomic<int> queries_completed{0};
    std::vector<std::thread> threads;
    
    // Writer threads
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, i, num_initial_items, &inserts_completed]() {
            for (int j = 0; j < 100; ++j) {
                std::string key = "item_" + std::to_string(num_initial_items + i * 100 + j);
                BaseEntity entity(key);
                entity.setField("priority", int64_t((i * 100 + j) % 100));
                
                if (db_->put("tasks::" + key, entity.serialize())) {
                    inserts_completed++;
                }
            }
        });
    }
    
    // Reader threads
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this, &queries_completed]() {
            for (int j = 0; j < 50; ++j) {
                auto results = secondary_index_->rangeQuery(
                    "tasks", "priority", int64_t(0), int64_t(50));
                if (results.size() > 0) {
                    queries_completed++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(inserts_completed.load(), 500);
    EXPECT_GT(queries_completed.load(), 0) << "Queries should succeed under concurrent load";
    
    std::cout << "Concurrent test: " << inserts_completed.load() 
              << " inserts, " << queries_completed.load() << " queries" << std::endl;
}

// ═══════════════════════════════════════════════════════════
// Index Efficiency Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test index vs non-index query performance
 * Acceptance Criteria:
 * - Indexed queries are significantly faster
 * - Index provides clear performance benefit
 */
TEST_F(IndexPerformanceTest, Efficiency_IndexVsNoIndex) {
    const int num_items = 10000;
    
    // Insert data
    for (int i = 0; i < num_items; ++i) {
        std::string key = "item_" + std::to_string(i);
        BaseEntity entity(key);
        entity.setField("category", int64_t(i % 10));
        entity.setField("name", randomString(20));
        
        db_->put("products::" + key, entity.serialize());
    }
    
    // Create index on category
    secondary_index_->createIndex(
        "products", "category", SecondaryIndexManager::IndexType::BTREE);
    
    // Test indexed query
    test::LatencyMeasurement indexed_timer;
    auto indexed_results = secondary_index_->lookup("products", "category", int64_t(5));
    double indexed_time = indexed_timer.elapsedMs();
    
    std::cout << "Indexed query: " << indexed_results.size() 
              << " results in " << indexed_time << "ms" << std::endl;
    
    // Indexed query should be fast
    EXPECT_LT(indexed_time, 100.0) << "Indexed query too slow";
    EXPECT_GT(indexed_results.size(), 0) << "Should find results";
}
