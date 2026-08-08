/**
 * @file test_query_optimizer_thread_safety.cpp
 * @brief Thread-safety hardening tests for Query Optimizer parallel plan optimizer
 *
 * Tests concurrent scenarios for:
 *  - PlanCache parallel get/put operations (GAP-5)
 *  - QueryOptimizer concurrent cost model updates (GAP-1, GAP-2)
 *  - Thread-safe counter updates (GAP-4)
 *  - Deadline propagation in federated contexts (GAP-5)
 *
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @see include/query/plan_cache.h
 * @see include/query/query_optimizer.h
 * @see QUERY_OPTIMIZER_THREAD_SAFETY_GAPS.md
 */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <cmath>

#include "query/plan_cache.h"
#include "query/query_optimizer.h"
#include "storage/base_entity.h"

namespace themis {
namespace query {
namespace test {

// ============================================================================
// Helpers and Fixtures
// ============================================================================

/// Create a minimal plan for testing.
static QueryOptimizer::Plan makeTestPlan() {
    QueryOptimizer::Plan p;
    p.nlp_complexity = 0.5;
    return p;
}

/// Create test statistics.
static PlanCache::Statistics makeTestStats(const std::string& table, size_t rows) {
    return PlanCache::Statistics({{table, rows}});
}

/// Create a small cache config for testing.
static PlanCache::Config smallCacheConfig() {
    PlanCache::Config cfg;
    cfg.max_entries = 100;
    cfg.max_plan_age = std::chrono::seconds{3600};
    cfg.statistics_drift_factor = 10.0;
    return cfg;
}

// ============================================================================
// Test Fixture
// ============================================================================

class QueryOptimizerThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        cache_ = std::make_unique<PlanCache>(smallCacheConfig());
    }

    std::unique_ptr<PlanCache> cache_;
};

// ============================================================================
// GAP-4: Concurrent Counter Updates
// ============================================================================

/**
 * @test ConcurrentCounterUpdates_HighContention_CountersAccurate
 * @brief Multiple threads increment cache statistics simultaneously;
 *        final counters match total operations (no lost increments).
 *
 * Thread Safety: Tests that atomic counter increments in PlanCache::get()
 * and PlanCache::put() don't lose updates under high contention.
 */
TEST_F(QueryOptimizerThreadSafetyTest, ConcurrentCounterUpdates_HighContention_CountersAccurate) {
    const int num_threads = 10;
    const int ops_per_thread = 100;
    const int total_queries = num_threads * ops_per_thread;

    auto test_thread = [this, ops_per_thread](int thread_id) {
        for (int i = 0; i < ops_per_thread; ++i) {
            // Generate unique query to avoid cache hits
            std::string query = "SELECT * FROM table WHERE id = " + 
                              std::to_string(thread_id * ops_per_thread + i);
            
            // Half do put, half do get (to exercise both code paths)
            if ((thread_id + i) % 2 == 0) {
                cache_->put(query, makeTestPlan(), makeTestStats("table", 1000));
            } else {
                cache_->get(query, makeTestStats("table", 1000));
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(test_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Verify counters are consistent
    auto stats = cache_->getStats();
    uint64_t total_ops = stats.hits.load(std::memory_order_acquire) + 
                         stats.misses.load(std::memory_order_acquire);
    
    // All operations should be counted (some as hits, some as misses)
    EXPECT_EQ(total_ops, total_queries);
    
    // Since we're generating unique queries, all should be misses initially
    EXPECT_EQ(stats.misses.load(std::memory_order_acquire), total_queries);
    EXPECT_EQ(stats.hits.load(std::memory_order_acquire), 0);
}

/**
 * @test ConcurrentPutAndGet_MixedWorkload_NoCrash
 * @brief Multiple threads concurrently put and get plans without crashes
 *        or data races (verified by ThreadSanitizer).
 */
TEST_F(QueryOptimizerThreadSafetyTest, ConcurrentPutAndGet_MixedWorkload_NoCrash) {
    const int num_threads = 8;
    const int iterations = 50;
    
    std::atomic<int> errors{0};

    auto thread_func = [this, iterations, &errors](int thread_id) {
        try {
            for (int i = 0; i < iterations; ++i) {
                std::string query = "SELECT * FROM users WHERE id = @id_" + 
                                  std::to_string(thread_id);
                
                // Alternate put and get
                if (i % 2 == 0) {
                    cache_->put(query, makeTestPlan(), makeTestStats("users", 5000));
                } else {
                    auto result = cache_->get(query, makeTestStats("users", 5000));
                    // On even iterations (after put), should hit
                    if ((i + 1) % 4 == 0) {
                        EXPECT_TRUE(result.has_value());
                    }
                }
            }
        } catch (const std::exception& e) {
            ++errors;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_func, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors, 0) << "No exceptions should be thrown during concurrent operations";
}

// ============================================================================
// GAP-5: Deadline Propagation
// ============================================================================

/**
 * @test PlanCacheGet_DeadlineExceeded_FailsFast
 * @brief When deadline is in the past, get() fails immediately without
 *        acquiring lock or waiting.
 *
 * Thread Safety: Tests deadline-aware fast-path (GAP-5) to prevent cascading
 * timeouts in federated queries.
 */
TEST_F(QueryOptimizerThreadSafetyTest, PlanCacheGet_DeadlineExceeded_FailsFast) {
    // Put a plan first
    std::string query = "SELECT * FROM test";
    cache_->put(query, makeTestPlan(), makeTestStats("test", 100));

    // Create a deadline in the past
    auto past = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);

    // get() should fail immediately due to deadline
    auto result = cache_->get(query, makeTestStats("test", 100), "", past);
    
    EXPECT_FALSE(result.has_value()) << "get() should return nullopt when deadline exceeded";
}

/**
 * @test PlanCachePut_DeadlineExceeded_SkipsCache
 * @brief When deadline is exceeded, put() returns immediately without
 *        caching the plan.
 */
TEST_F(QueryOptimizerThreadSafetyTest, PlanCachePut_DeadlineExceeded_SkipsCache) {
    // Create a deadline in the past
    auto past = std::chrono::steady_clock::now() - std::chrono::milliseconds(100);

    std::string query = "SELECT * FROM items WHERE id = @id";
    
    // put() should return immediately due to deadline
    cache_->put(query, makeTestPlan(), makeTestStats("items", 500), {}, {}, "", past);

    // Verify the plan was NOT cached
    auto result = cache_->get(query);
    EXPECT_FALSE(result.has_value()) << "plan should not be cached when deadline exceeded";
}

/**
 * @test PlanCacheGet_DeadlineInFuture_Succeeds
 * @brief When deadline is in the future, get() succeeds normally.
 */
TEST_F(QueryOptimizerThreadSafetyTest, PlanCacheGet_DeadlineInFuture_Succeeds) {
    std::string query = "SELECT * FROM products";
    cache_->put(query, makeTestPlan(), makeTestStats("products", 2000));

    // Create a deadline far in the future
    auto future = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    // get() should succeed
    auto result = cache_->get(query, makeTestStats("products", 2000), "", future);
    
    EXPECT_TRUE(result.has_value()) << "get() should succeed with future deadline";
}

// ============================================================================
// GAP-1 & GAP-2: Concurrent Optimizer State Updates
// ============================================================================

/**
 * @brief Mock secondary index manager for testing.
 */
class MockSecondaryIndexManager : public SecondaryIndexManager {
public:
    size_t estimateCountEqual(const std::string&, const std::string&,
                             const std::string&, size_t, bool*) const override {
        return 100;  // Dummy estimate
    }
};

/**
 * @test QueryOptimizerCostModel_ConcurrentSetGet_NoRaceCondition
 * @brief Multiple threads call setAdvisorCostConstants() and advisorCostConstants()
 *        concurrently without data races.
 *
 * Thread Safety: Tests protection of advisor_cost_model_ member (GAP-2).
 */
TEST_F(QueryOptimizerThreadSafetyTest, QueryOptimizerCostModel_ConcurrentSetGet_NoRaceCondition) {
    MockSecondaryIndexManager idx_mgr;
    QueryOptimizer optimizer(idx_mgr);

    const int num_threads = 4;
    const int iterations = 100;
    
    std::atomic<int> errors{0};

    auto thread_func = [&optimizer, iterations, &errors](int thread_id) {
        try {
            for (int i = 0; i < iterations; ++i) {
                OptimizerCostModel::CostConstants constants;
                constants.cpu_cost_per_row = 0.1 * (thread_id + 1);
                constants.io_cost_per_kb = 0.05 * (thread_id + 1);
                
                if (i % 2 == 0) {
                    // Writers set constants
                    optimizer.setAdvisorCostConstants(constants);
                } else {
                    // Readers get constants
                    auto read_constants = optimizer.advisorCostConstants();
                    EXPECT_GT(read_constants.cpu_cost_per_row, 0.0);
                }
            }
        } catch (const std::exception& e) {
            ++errors;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(thread_func, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(errors, 0) << "No exceptions during concurrent cost model updates";
}

// ============================================================================
// Multi-threaded Stress Tests
// ============================================================================

/**
 * @test ConcurrentCacheOperations_Stress_NoDeadlock
 * @brief Stress test with many concurrent operations; should complete
 *        without deadlock or hanging.
 */
TEST_F(QueryOptimizerThreadSafetyTest, ConcurrentCacheOperations_Stress_NoDeadlock) {
    const int num_threads = 16;
    const int ops_per_thread = 200;
    
    std::atomic<int> operations_completed{0};

    auto stress_thread = [this, ops_per_thread, &operations_completed](int thread_id) {
        for (int i = 0; i < ops_per_thread; ++i) {
            std::string query = "SELECT * FROM t" + std::to_string(thread_id % 5) + 
                              " WHERE id = " + std::to_string(i);
            
            int op = (thread_id + i) % 5;
            if (op == 0) {
                cache_->put(query, makeTestPlan(), makeTestStats("t" + std::to_string(thread_id % 5), 1000));
            } else if (op == 1) {
                cache_->get(query);
            } else if (op == 2) {
                cache_->invalidateTable("t" + std::to_string(thread_id % 5));
            } else if (op == 3) {
                cache_->evictExpired();
            } else {
                cache_->getStats();
            }
            ++operations_completed;
        }
    };

    std::vector<std::thread> threads;
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(stress_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    
    EXPECT_EQ(operations_completed, num_threads * ops_per_thread);
    
    // Should complete within reasonable time (not deadlocked)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(ms, 30000) << "Stress test should complete in < 30s";
    
    THEMIS_INFO("Stress test: {} ops in {}ms", operations_completed, ms);
}

/**
 * @test StatsDriftDetection_ConcurrentUpdates_Accurate
 * @brief Statistics drift counter should be accurate even under concurrent
 *        get/put operations that trigger drift detection.
 */
TEST_F(QueryOptimizerThreadSafetyTest, StatsDriftDetection_ConcurrentUpdates_Accurate) {
    const int num_threads = 8;
    const int iterations = 50;
    
    std::string query = "SELECT * FROM data";

    // Initial put with 1000 rows
    cache_->put(query, makeTestPlan(), makeTestStats("data", 1000));

    auto stats_drift_thread = [this, query, iterations](int thread_id) {
        for (int i = 0; i < iterations; ++i) {
            // Simulate cardinality change: multiply by 20 to exceed 10x drift
            size_t new_cardinality = 1000 + (20000 * (thread_id + 1));
            
            auto result = cache_->get(query, makeTestStats("data", new_cardinality));
            
            // Should miss due to statistics drift (cardinality changed > 10x)
            if (i > 0) {
                EXPECT_FALSE(result.has_value()) << "Should miss due to stats drift";
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(stats_drift_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto stats = cache_->getStats();
    uint64_t stat_drifts = stats.stat_drifts.load(std::memory_order_acquire);
    
    // Should have many stat drift detections
    EXPECT_GT(stat_drifts, 0) << "Should detect statistics drift";
}

/**
 * @test CacheMemoryTracking_ConcurrentPutEvict_Consistent
 * @brief Cache memory counter should remain consistent even with concurrent
 *        put and evict operations.
 */
TEST_F(QueryOptimizerThreadSafetyTest, CacheMemoryTracking_ConcurrentPutEvict_Consistent) {
    // Create small cache to trigger evictions
    PlanCache::Config small_cfg;
    small_cfg.max_entries = 50;
    small_cfg.max_memory_bytes = 1024 * 1024;  // 1MB limit
    
    PlanCache small_cache(small_cfg);

    const int num_threads = 4;
    const int puts_per_thread = 200;

    auto put_thread = [&small_cache, puts_per_thread](int thread_id) {
        for (int i = 0; i < puts_per_thread; ++i) {
            std::string query = "SELECT * FROM large_" + std::to_string(thread_id * puts_per_thread + i);
            small_cache.put(query, makeTestPlan(), makeTestStats("large", 100000));
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(put_thread, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto stats = small_cache.getStats();
    size_t current_size = stats.current_size.load(std::memory_order_acquire);
    size_t current_memory = stats.current_memory_bytes.load(std::memory_order_acquire);
    
    // Cache size should not exceed configured max
    EXPECT_LE(current_size, small_cfg.max_entries);
    
    // Memory should not vastly exceed limit (some overage OK due to atomic operations)
    EXPECT_LE(current_memory, small_cfg.max_memory_bytes * 1.1);
}

} // namespace test
} // namespace query
} // namespace themis
