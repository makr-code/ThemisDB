/**
 * @file test_query_deadlock_concurrent_timeout.cpp
 * @brief Regression tests for query module deadlock prevention via timeout-aware thread joins.
 * 
 * These tests verify that watchdog threads and parallel executors handle timeout
 * scenarios without deadlock, even when worker threads or watchdog threads are slow
 * to complete.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "query/parallel_executor.h"
#include "query/query_canceller.h"
#include "storage/base_entity.h"
#include "utils/expected.h"

using namespace themis;
using namespace themis::query;

// ════════════════════════════════════════════════════════════════════════════
// Helper: Create a test entity
// ════════════════════════════════════════════════════════════════════════════

static BaseEntity createTestEntity(const std::string& id, int value) {
    BaseEntity entity;
    entity.setId(id);
    entity.setAttribute("value", value);
    return entity;
}

// ════════════════════════════════════════════════════════════════════════════
// Parallel Executor Timeout Tests
// ════════════════════════════════════════════════════════════════════════════

class ParallelExecutorDeadlockTest : public ::testing::Test {
protected:
    ParallelExecutor::ParallelConfig defaultConfig() {
        ParallelExecutor::ParallelConfig cfg;
        cfg.enable_parallel_scan = true;
        cfg.enable_parallel_join = true;
        cfg.enable_parallel_aggregate = true;
        cfg.max_threads = 4;
        cfg.morsel_size = 100;
        return cfg;
    }
};

/**
 * @test ParallelExecutorDeadlockTest::ParallelScanWithQuickCompletion
 * @brief Verify parallel scan completes without deadlock when all tasks finish promptly.
 */
TEST_F(ParallelExecutorDeadlockTest, ParallelScanWithQuickCompletion) {
    ParallelExecutor executor(defaultConfig());
    
    // Create a moderately-sized input: 1000 entities
    ParallelExecutor::Table input;
    input.reserve(1000);
    for (int i = 0; i < 1000; ++i) {
        input.push_back(createTestEntity("id_" + std::to_string(i), i));
    }
    
    // Filter: accept only even values
    auto filter = [](const BaseEntity& e) {
        auto val = e.getAttribute("value");
        return val && val->is_number() && (val->get<int>() % 2 == 0);
    };
    
    // Execute scan with 4 threads; should complete within 5 seconds
    auto start = std::chrono::steady_clock::now();
    auto result = executor.parallelScan(input, filter, 4);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    ASSERT_TRUE(result.has_value()) << "Parallel scan should succeed";
    EXPECT_EQ(result->size(), 500u) << "Should have 500 even values";
    EXPECT_LT(elapsed.count(), 5000) << "Should complete within 5 seconds (no timeout/deadlock)";
}

/**
 * @test ParallelExecutorDeadlockTest::ParallelScanWithSlowFilter
 * @brief Verify parallel scan completes even with slow filter functions.
 * This tests that watchdog timeout doesn't cause deadlock in the join operation.
 */
TEST_F(ParallelExecutorDeadlockTest, ParallelScanWithSlowFilter) {
    ParallelExecutor executor(defaultConfig());
    
    // Create input: 100 entities
    ParallelExecutor::Table input;
    input.reserve(100);
    for (int i = 0; i < 100; ++i) {
        input.push_back(createTestEntity("id_" + std::to_string(i), i));
    }
    
    // Slow filter: each evaluation sleeps for 1ms
    auto slow_filter = [](const BaseEntity& e) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto val = e.getAttribute("value");
        return val && val->is_number() && (val->get<int>() % 2 == 0);
    };
    
    // Execute scan with 2 threads; even though filter is slow, scan should
    // complete because watchdog doesn't deadlock on join.
    auto start = std::chrono::steady_clock::now();
    auto result = executor.parallelScan(input, slow_filter, 2);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    ASSERT_TRUE(result.has_value()) << "Parallel scan should succeed (no deadlock)";
    EXPECT_EQ(result->size(), 50u) << "Should have 50 even values";
    EXPECT_LT(elapsed.count(), 10000) << "Should complete within 10 seconds";
}

/**
 * @test ParallelExecutorDeadlockTest::ParallelScanWithEmptyInput
 * @brief Verify parallel scan handles empty input correctly.
 * Tests the null guard on sequential fallback path.
 */
TEST_F(ParallelExecutorDeadlockTest, ParallelScanWithEmptyInput) {
    ParallelExecutor executor(defaultConfig());
    
    ParallelExecutor::Table input;  // Empty input
    
    auto filter = [](const BaseEntity& e) {
        (void)e;
        return true;
    };
    
    auto result = executor.parallelScan(input, filter, 4);
    ASSERT_TRUE(result.has_value()) << "Should handle empty input gracefully";
    EXPECT_EQ(result->size(), 0u) << "Empty input should produce empty output";
}

/**
 * @test ParallelExecutorDeadlockTest::SequentialFallbackPath
 * @brief Verify sequential path is used when parallelism is disabled.
 */
TEST_F(ParallelExecutorDeadlockTest, SequentialFallbackPath) {
    ParallelExecutor::ParallelConfig cfg = defaultConfig();
    cfg.enable_parallel_scan = false;  // Force sequential path
    ParallelExecutor executor(cfg);
    
    ParallelExecutor::Table input;
    input.reserve(100);
    for (int i = 0; i < 100; ++i) {
        input.push_back(createTestEntity("id_" + std::to_string(i), i));
    }
    
    auto filter = [](const BaseEntity& e) {
        auto val = e.getAttribute("value");
        return val && val->is_number() && (val->get<int>() % 2 == 0);
    };
    
    auto result = executor.parallelScan(input, filter, 1);
    ASSERT_TRUE(result.has_value()) << "Sequential path should succeed";
    EXPECT_EQ(result->size(), 50u) << "Should have 50 even values";
}

/**
 * @test ParallelExecutorDeadlockTest::ConcurrentScansNoDeadlock
 * @brief Verify multiple concurrent parallel scans don't deadlock each other.
 * This is a stress test to catch lock-order or nested blocking issues.
 */
TEST_F(ParallelExecutorDeadlockTest, ConcurrentScansNoDeadlock) {
    // Create input for multiple threads
    ParallelExecutor::Table input;
    input.reserve(500);
    for (int i = 0; i < 500; ++i) {
        input.push_back(createTestEntity("id_" + std::to_string(i), i));
    }
    
    auto filter = [](const BaseEntity& e) {
        auto val = e.getAttribute("value");
        return val && val->is_number() && (val->get<int>() % 3 == 0);
    };
    
    // Run 4 concurrent scans in separate threads
    std::vector<std::thread> threads;
    std::vector<std::atomic<bool>> results(4, false);
    
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t]() {
            ParallelExecutor executor(defaultConfig());
            auto result = executor.parallelScan(input, filter, 2);
            results[t].store(result.has_value() && result->size() > 0, std::memory_order_release);
        });
    }
    
    auto start = std::chrono::steady_clock::now();
    for (auto& thread : threads) {
        thread.join();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    // All scans should succeed
    for (int t = 0; t < 4; ++t) {
        EXPECT_TRUE(results[t].load(std::memory_order_acquire)) << "Concurrent scan " << t << " should succeed";
    }
    EXPECT_LT(elapsed.count(), 30000) << "All 4 concurrent scans should complete within 30 seconds";
}

/**
 * @test ParallelExecutorDeadlockTest::ParallelHashJoinNoDeadlock
 * @brief Verify parallel hash join doesn't deadlock on completion.
 */
TEST_F(ParallelExecutorDeadlockTest, ParallelHashJoinNoDeadlock) {
    ParallelExecutor executor(defaultConfig());
    
    // Create left table: 200 entities
    ParallelExecutor::Table left;
    left.reserve(200);
    for (int i = 0; i < 200; ++i) {
        BaseEntity e;
        e.setId("left_" + std::to_string(i));
        e.setAttribute("key", i % 50);
        e.setAttribute("value", i);
        left.push_back(e);
    }
    
    // Create right table: 300 entities
    ParallelExecutor::Table right;
    right.reserve(300);
    for (int i = 0; i < 300; ++i) {
        BaseEntity e;
        e.setId("right_" + std::to_string(i));
        e.setAttribute("key", i % 50);
        e.setAttribute("value", i + 1000);
        right.push_back(e);
    }
    
    ParallelExecutor::JoinSpec spec;
    spec.left_key = "key";
    spec.right_key = "key";
    
    auto start = std::chrono::steady_clock::now();
    auto result = executor.parallelHashJoin(left, right, spec, 4);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    ASSERT_TRUE(result.has_value()) << "Parallel hash join should succeed";
    EXPECT_GT(result->size(), 0u) << "Should produce join results";
    EXPECT_LT(elapsed.count(), 10000) << "Should complete within 10 seconds (no deadlock)";
}

/**
 * @test ParallelExecutorDeadlockTest::ParallelAggregateNoDeadlock
 * @brief Verify parallel aggregate doesn't deadlock on completion.
 */
TEST_F(ParallelExecutorDeadlockTest, ParallelAggregateNoDeadlock) {
    ParallelExecutor executor(defaultConfig());
    
    // Create input: 400 entities
    ParallelExecutor::Table input;
    input.reserve(400);
    for (int i = 0; i < 400; ++i) {
        BaseEntity e;
        e.setId("id_" + std::to_string(i));
        e.setAttribute("group", i % 5);
        e.setAttribute("value", static_cast<double>(i));
        input.push_back(e);
    }
    
    ParallelExecutor::AggregateSpec spec;
    spec.field = "value";
    spec.function = ParallelExecutor::AggregateFunction::Sum;
    spec.group_by = {"group"};
    
    auto start = std::chrono::steady_clock::now();
    auto result = executor.parallelAggregate(input, spec, 4);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    ASSERT_TRUE(result.has_value()) << "Parallel aggregate should succeed";
    EXPECT_EQ(result->size(), 5u) << "Should have 5 groups";
    EXPECT_LT(elapsed.count(), 10000) << "Should complete within 10 seconds (no deadlock)";
}

// ════════════════════════════════════════════════════════════════════════════
// Query Cancellation Timeout Tests
// ════════════════════════════════════════════════════════════════════════════

/**
 * @test QueryCancellerTimeoutTest::CancelledQueryDoesNotDeadlock
 * @brief Verify that query cancellation doesn't cause deadlock in concurrent scenarios.
 */
TEST(QueryCancellerTimeoutTest, CancelledQueryDoesNotDeadlock) {
    QueryCanceller canceller;
    
    // Register multiple queries
    std::vector<std::shared_ptr<QueryCancellationToken>> tokens;
    for (int i = 0; i < 10; ++i) {
        auto token = canceller.registerQuery("query_" + std::to_string(i));
        tokens.push_back(token);
    }
    
    // Cancel all queries from multiple threads
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                canceller.cancel("query_" + std::to_string(j));
                std::this_thread::yield();
            }
        });
    }
    
    auto start = std::chrono::steady_clock::now();
    for (auto& thread : threads) {
        thread.join();
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    
    EXPECT_LT(elapsed.count(), 5000) << "Concurrent cancellations should complete quickly";
    
    // Verify all tokens are cancelled
    for (const auto& token : tokens) {
        EXPECT_TRUE(token->isCancelled()) << "All tokens should be cancelled";
    }
}
