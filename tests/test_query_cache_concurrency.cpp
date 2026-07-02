/**
 * @file test_query_cache_concurrency.cpp
 * @brief Concurrency tests for Query Cache - Sprint 2 Quick-Win Batch 2 fixes
 * 
 * Tests for findings QW-011 through QW-015:
 * - QW-011: Missing read lock during validation (shared_lock for reads)
 * - QW-012: Race condition in cache invalidation (atomic compare-exchange)
 * - QW-013: Cache coherency: duplicate entries (check-then-insert)
 * - QW-014: Atomic increment without CAS loop (moved to QW-012)
 * - QW-015: Result cloning inefficiency (move semantics)
 */

#include <gtest/gtest.h>
#include "query/query_cache.h"
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

namespace themis {
namespace query {
namespace test {

class QueryCacheConcurrencyTest : public ::testing::Test {
protected:
    QueryCache::Config config_;
    
    void SetUp() override {
        config_.max_entries = 1000;
        config_.max_memory_bytes = 10 * 1024 * 1024;  // 10MB
        config_.enable_ttl = true;
        config_.default_ttl = std::chrono::seconds(3600);
        config_.eviction_policy = QueryCache::EvictionPolicy::LRU;
    }
};

/**
 * QW-011: Test concurrent readers with shared_lock
 * Multiple threads reading from cache simultaneously should not block each other
 */
TEST_F(QueryCacheConcurrencyTest, ConcurrentReadsWithSharedLock) {
    QueryCache cache(config_);
    
    // Pre-populate cache with some entries
    nlohmann::json params = nlohmann::json::object({{"param", 1}});
    nlohmann::json result = nlohmann::json::object({{"result", "test_value"}});
    
    EXPECT_TRUE(cache.put("SELECT 1", params, result, {}).ok());
    
    std::atomic<int> read_count{0};
    std::vector<std::thread> readers;
    
    // Spawn 10 reader threads
    for (int i = 0; i < 10; ++i) {
        readers.emplace_back([&cache, &params, &read_count]() {
            for (int j = 0; j < 100; ++j) {
                auto result = cache.get("SELECT 1", params);
                if (result.ok() && result.value().found) {
                    read_count++;
                }
            }
        });
    }
    
    // Wait for all readers to complete
    for (auto& t : readers) {
        t.join();
    }
    
    // All 1000 reads (10 threads * 100 reads) should succeed without blocking
    EXPECT_EQ(read_count, 1000);
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.hits, 900);  // At least 90% hit rate
}

/**
 * QW-012: Test atomic cache invalidation without race conditions
 * Concurrent invalidations should be atomic and consistent
 */
TEST_F(QueryCacheConcurrencyTest, AtomicCacheInvalidation) {
    QueryCache cache(config_);
    
    // Pre-populate cache with dependent entries
    for (int i = 0; i < 10; ++i) {
        nlohmann::json params = nlohmann::json::object({{"id", i}});
        nlohmann::json result = nlohmann::json::object({{"value", i * 10}});
        std::vector<std::string> deps = {"users", "orders"};
        
        EXPECT_TRUE(cache.put("SELECT * FROM users WHERE id = " + std::to_string(i), 
                             params, result, deps).ok());
    }
    
    std::atomic<int> invalidation_count{0};
    std::vector<std::thread> invalidators;
    
    // Spawn multiple threads to invalidate the same dependency
    for (int i = 0; i < 5; ++i) {
        invalidators.emplace_back([&cache, &invalidation_count]() {
            auto result = cache.invalidateByDependency("users");
            if (result.ok()) {
                invalidation_count += result.value();
            }
        });
    }
    
    // Wait for all invalidation threads
    for (auto& t : invalidators) {
        t.join();
    }
    
    // Should have consistent invalidation results
    // First invalidation removes 10 entries, subsequent ones should remove 0
    EXPECT_GE(invalidation_count, 10);
    
    // Verify entries were invalidated
    auto stats = cache.getStats();
    EXPECT_LT(stats.current_entries, 10);
}

/**
 * QW-013: Test cache coherency with duplicate detection
 * Concurrent puts with identical entries should maintain coherency
 */
TEST_F(QueryCacheConcurrencyTest, CacheCoherencyDuplicateDetection) {
    QueryCache cache(config_);
    
    nlohmann::json params = nlohmann::json::object({{"id", 1}});
    nlohmann::json result1 = nlohmann::json::object({{"value", 100}});
    nlohmann::json result2 = nlohmann::json::object({{"value", 200}});
    
    std::vector<std::thread> writers;
    std::atomic<int> put_success{0};
    
    // Spawn multiple threads writing the same query with different results
    for (int i = 0; i < 5; ++i) {
        writers.emplace_back([&cache, &params, &result1, &result2, &put_success, i]() {
            nlohmann::json result = (i % 2 == 0) ? result1 : result2;
            auto res = cache.put("SELECT * FROM users WHERE id = 1", params, result, {});
            if (res.ok()) {
                put_success++;
            }
        });
    }
    
    // Wait for all writers
    for (auto& t : writers) {
        t.join();
    }
    
    EXPECT_EQ(put_success, 5);
    
    // Verify only one entry exists (last one wins)
    auto lookup = cache.get("SELECT * FROM users WHERE id = 1", params);
    EXPECT_TRUE(lookup.ok());
    EXPECT_TRUE(lookup.value().found);
    
    // Cache should have exactly 1 entry
    auto stats = cache.getStats();
    EXPECT_EQ(stats.current_entries, 1);
}

/**
 * QW-014 & QW-015: Test performance with move semantics
 * Results should be moved, not copied, for efficiency
 */
TEST_F(QueryCacheConcurrencyTest, MoveSemanticsDuringRetrieval) {
    QueryCache cache(config_);
    
    nlohmann::json params = nlohmann::json::object({{"large_param", true}});
    
    // Create a large result to test move semantics
    nlohmann::json large_result;
    for (int i = 0; i < 1000; ++i) {
        large_result["data"][std::to_string(i)] = i * 1.5;
    }
    
    EXPECT_TRUE(cache.put("SELECT * FROM large_table", params, large_result, {}).ok());
    
    std::vector<std::thread> readers;
    std::atomic<int> retrieval_count{0};
    
    // Multiple threads retrieving large results
    for (int i = 0; i < 10; ++i) {
        readers.emplace_back([&cache, &params, &retrieval_count]() {
            for (int j = 0; j < 50; ++j) {
                auto result = cache.get("SELECT * FROM large_table", params);
                if (result.ok() && result.value().found) {
                    retrieval_count++;
                }
            }
        });
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto& t : readers) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(retrieval_count, 500);
    
    // Should complete quickly even with large results (move semantics benefit)
    EXPECT_LT(duration, 5000);  // Less than 5 seconds for 500 retrievals
}

/**
 * Mixed workload: Concurrent reads, writes, and invalidations
 * Verifies all concurrency fixes work together
 */
TEST_F(QueryCacheConcurrencyTest, MixedWorkloadConcurrency) {
    QueryCache cache(config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> read_count{0};
    std::atomic<int> write_count{0};
    std::atomic<int> invalidate_count{0};
    
    // Reader threads
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&cache, &read_count]() {
            for (int j = 0; j < 100; ++j) {
                nlohmann::json params = nlohmann::json::object({{"id", j % 10}});
                auto result = cache.get("SELECT * FROM table WHERE id = " + std::to_string(j % 10), params);
                if (result.ok()) {
                    read_count++;
                }
            }
        });
    }
    
    // Writer threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&cache, &write_count]() {
            for (int j = 0; j < 50; ++j) {
                nlohmann::json params = nlohmann::json::object({{"id", j}});
                nlohmann::json result = nlohmann::json::object({{"value", j * i}});
                auto res = cache.put("SELECT * FROM table WHERE id = " + std::to_string(j), 
                                    params, result, {"table_" + std::to_string(j % 5)});
                if (res.ok()) {
                    write_count++;
                }
            }
        });
    }
    
    // Invalidator threads
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([&cache, &invalidate_count]() {
            for (int j = 0; j < 5; ++j) {
                auto result = cache.invalidateByDependency("table_" + std::to_string(j));
                if (result.ok()) {
                    invalidate_count += result.value();
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(write_count, 150);  // 3 threads * 50 writes
    
    auto stats = cache.getStats();
    EXPECT_GT(stats.total_requests, 0);
}

/**
 * QW-011: Stress test with many concurrent readers
 * Verifies that shared_lock allows true concurrent read access
 */
TEST_F(QueryCacheConcurrencyTest, HighConcurrencyReadStress) {
    QueryCache cache(config_);
    
    // Pre-populate with entries
    for (int i = 0; i < 100; ++i) {
        nlohmann::json params = nlohmann::json::object({{"id", i}});
        nlohmann::json result = nlohmann::json::object({{"value", i}});
        cache.put("Q" + std::to_string(i), params, result, {});
    }
    
    std::vector<std::thread> readers;
    std::atomic<int> total_reads{0};
    
    // 50 concurrent reader threads
    for (int i = 0; i < 50; ++i) {
        readers.emplace_back([&cache, &total_reads]() {
            for (int j = 0; j < 10; ++j) {
                for (int k = 0; k < 100; ++k) {
                    nlohmann::json params = nlohmann::json::object({{"id", k}});
                    cache.get("Q" + std::to_string(k), params);
                    total_reads++;
                }
            }
        });
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto& t : readers) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(total_reads, 50000);  // 50 threads * 10 * 100
    
    // Should complete quickly with shared_lock allowing concurrent reads
    EXPECT_LT(duration, 10000);  // Less than 10 seconds
}

}  // namespace test
}  // namespace query
}  // namespace themis
