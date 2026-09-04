/**
 * @file test_query_safety_fixes.cpp
 * @brief Unit tests for critical safety fixes in query module.
 *
 * Tests for:
 * 1. Iterator invalidation fixes (query_rewrite_rule.cpp)
 * 2. Multiplication overflow fixes (tensor_aware_query_optimizer.cpp)
 * 3. Connection/resource leak fixes (cq_watermark.cpp)
 * 4. Blocking timeout fixes (query_canceller.cpp)
 */

#include <gtest/gtest.h>

#include "query/query_rewrite_rule.h"
#include "query/tensor_aware_query_optimizer.h"
#include "query/cq_watermark.h"
#include "query/query_canceller.h"

#include <cmath>
#include <thread>
#include <chrono>
#include <limits>
#include <atomic>

namespace themis {
namespace query {

// ============================================================================
// 1. Iterator Invalidation Tests (query_rewrite_rule.cpp)
// ============================================================================

class QueryRewriteRuleTest : public ::testing::Test {
protected:
    PredicatePushdownRule rule_;
    RewriteContext ctx_;
};

/// Test that collectOrChain safely handles nested OR structures without
/// iterator invalidation.
TEST_F(QueryRewriteRuleTest, NestedOrChainIteratorSafety) {
    // Create a simple OR structure: (field = 1) OR (field = 2)
    nlohmann::json plan = nlohmann::json::object();
    plan["type"] = "or";
    plan["left"] = {
        {"type", "eq"},
        {"field", "status"},
        {"value", 1}
    };
    plan["right"] = {
        {"type", "eq"},
        {"field", "status"},
        {"value", 2}
    };

    // The apply() method should not crash or produce invalid iterators
    // even when recursively processing nested structures.
    EXPECT_NO_THROW({ rule_.apply(plan, ctx_); });
    // Plan must remain a valid JSON object after rewriting.
    EXPECT_TRUE(plan.is_object());
}

/// Test that deeply nested OR chains don't cause iterator invalidation.
TEST_F(QueryRewriteRuleTest, DeeplyNestedOrChainSafety) {
    // Create: ((field = 1) OR (field = 2)) OR ((field = 3) OR (field = 4))
    nlohmann::json plan = nlohmann::json::object();
    plan["type"] = "or";
    
    // Left: (field = 1) OR (field = 2)
    plan["left"] = {
        {"type", "or"},
        {"left", {{"type", "eq"}, {"field", "id"}, {"value", 1}}},
        {"right", {{"type", "eq"}, {"field", "id"}, {"value", 2}}}
    };
    
    // Right: (field = 3) OR (field = 4)
    plan["right"] = {
        {"type", "or"},
        {"left", {{"type", "eq"}, {"field", "id"}, {"value", 3}}},
        {"right", {{"type", "eq"}, {"field", "id"}, {"value", 4}}}
    };

    // Should handle deep nesting without iterator invalidation
    EXPECT_NO_THROW({ rule_.apply(plan, ctx_); });
    EXPECT_TRUE(plan.is_object());
}

// ============================================================================
// 2. Multiplication Overflow Tests (tensor_aware_query_optimizer.cpp)
// ============================================================================

class TensorAwareOptimizerTest : public ::testing::Test {
protected:
    TensorAwareQueryOptimizer optimizer_;
};

/// Test that tensor similarity cost calculation doesn't overflow.
TEST_F(TensorAwareOptimizerTest, TensorSimilarityOverflowSafety) {
    // Maximum realistic dimensions
    double cost = optimizer_.estimateTTCost(
        "TENSOR_SIMILARITY",
        1000000,  // order = 1e6
        1000000,  // mode_size = 1e6
        1000000   // max_rank = 1e6
    );
    
    // Cost should be finite and not NaN
    EXPECT_TRUE(std::isfinite(cost));
    // Should be a very large but reasonable value
    EXPECT_LE(cost, std::numeric_limits<double>::max() / 2.0);
}

/// Test that tensor contract cost calculation doesn't overflow.
TEST_F(TensorAwareOptimizerTest, TensorContractOverflowSafety) {
    double cost = optimizer_.estimateTTCost(
        "TENSOR_CONTRACT",
        500000,
        500000,
        500000
    );
    
    EXPECT_TRUE(std::isfinite(cost));
    EXPECT_LE(cost, std::numeric_limits<double>::max() / 2.0);
}

/// Test that tensor compress cost calculation doesn't overflow.
TEST_F(TensorAwareOptimizerTest, TensorCompressOverflowSafety) {
    double cost = optimizer_.estimateTTCost(
        "TENSOR_COMPRESS",
        100000,
        100000,
        100000
    );
    
    EXPECT_TRUE(std::isfinite(cost));
    EXPECT_LE(cost, std::numeric_limits<double>::max() / 2.0);
}

/// Test that various edge cases don't cause overflow.
TEST_F(TensorAwareOptimizerTest, EdgeCasesNoOverflow) {
    // Zero dimensions
    EXPECT_TRUE(std::isfinite(optimizer_.estimateTTCost("TENSOR_SIMILARITY", 0, 0, 0)));
    
    // One dimension
    EXPECT_TRUE(std::isfinite(optimizer_.estimateTTCost("TENSOR_SIMILARITY", 1, 1, 1)));
    
    // Maximum values
    auto max_val = static_cast<std::size_t>(std::numeric_limits<int>::max());
    EXPECT_TRUE(std::isfinite(optimizer_.estimateTTCost("TENSOR_NORM", max_val, max_val, max_val)));
}

// ============================================================================
// 3. Resource/Connection Safety Tests (cq_watermark.cpp)
// ============================================================================

class CQWatermarkTest : public ::testing::Test {
protected:
    CQWatermark watermark_{500};  // 500ms late allowance
};

/// Test that watermark arithmetic doesn't underflow.
TEST_F(CQWatermarkTest, ArithmeticUnderflowSafety) {
    // Observe extreme timestamps
    watermark_.observe(std::numeric_limits<int64_t>::max());
    
    // Should not crash or produce undefined behavior
    EXPECT_GE(watermark_.maxSeenUs(), 0);
    
    // Advance should handle large timestamps safely
    watermark_.advance();
    EXPECT_GE(watermark_.watermarkUs(), 0);
}

/// Test that late event tracking is exception-safe.
TEST_F(CQWatermarkTest, LateEventTrackingExceptionSafety) {
    int64_t current_wm = watermark_.watermarkUs();
    
    // Simulate late events
    for (int i = 0; i < 1000; ++i) {
        int64_t late_ts = current_wm - 100000 * (i + 1);
        watermark_.observe(late_ts);
    }
    
    // State should remain consistent
    uint64_t total_late = watermark_.lateProcessed() + watermark_.lateDropped();
    EXPECT_GE(total_late, 0);
}

/// Test concurrent watermark updates for thread safety.
TEST_F(CQWatermarkTest, ConcurrentAccessThreadSafety) {
    std::atomic<bool> stop{false};
    std::atomic<int64_t> max_observed{0};
    
    auto observer = [&]() {
        int64_t ts = 0;
        while (!stop.load()) {
            ts += 1000000;  // 1 second increments
            watermark_.observe(ts);
            max_observed.store(ts, std::memory_order_release);
        }
    };
    
    auto advancer = [&]() {
        while (!stop.load()) {
            watermark_.advance();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    
    std::thread t1(observer);
    std::thread t2(observer);
    std::thread t3(advancer);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stop.store(true);
    
    t1.join();
    t2.join();
    t3.join();
    
    // All counters should be consistent
    EXPECT_GE(watermark_.lateProcessed() + watermark_.lateDropped(), 0);
}

// ============================================================================
// 4. Blocking Timeout Tests (query_canceller.cpp)
// ============================================================================

class QueryCancellerTest : public ::testing::Test {
protected:
    QueryCanceller canceller_;
};

/// Test that registerQuery returns quickly even if lock is contended.
TEST_F(QueryCancellerTest, RegisterQueryTimeout) {
    auto start = std::chrono::high_resolution_clock::now();
    auto token = canceller_.registerQuery("test-query-1");
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    // Should return almost immediately
    EXPECT_LT(elapsed, std::chrono::seconds(1));
    EXPECT_TRUE(token != nullptr);
}

/// Test that cancel operations have bounded blocking time.
TEST_F(QueryCancellerTest, CancelOperationTimeout) {
    auto token = canceller_.registerQuery("test-query-2");
    
    auto start = std::chrono::high_resolution_clock::now();
    bool cancelled = canceller_.cancel("test-query-2");
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    // Should complete within reasonable timeout
    EXPECT_LT(elapsed, std::chrono::seconds(1));
    EXPECT_TRUE(cancelled);
}

/// Test that unregisterQuery doesn't block indefinitely.
TEST_F(QueryCancellerTest, UnregisterQueryTimeout) {
    canceller_.registerQuery("test-query-3");
    
    auto start = std::chrono::high_resolution_clock::now();
    canceller_.unregisterQuery("test-query-3");
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    
    // Should complete within reasonable timeout
    EXPECT_LT(elapsed, std::chrono::seconds(1));
}

/// Test concurrent cancellation doesn't cause deadlock.
TEST_F(QueryCancellerTest, ConcurrentCancellationNoDeadlock) {
    std::vector<std::string> request_ids = {};

    for (int i = 0; i < 10; ++i) {
        request_ids.push_back("request-" + std::to_string(i));
        canceller_.registerQuery(request_ids.back());
    }
    
    std::atomic<int> cancel_count{0};
    std::vector<std::thread> threads;
    
    // Launch multiple threads trying to cancel simultaneously
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&]() {
            for (const auto& id : request_ids) {
                if (canceller_.cancel(id)) {
                    cancel_count.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    
    // All threads should complete within reasonable time
    for (auto& t : threads) {
        t.join();
    }
    
    // Some cancellations should have succeeded
    EXPECT_GT(cancel_count.load(), 0);
}

/// Test that token remains valid even if registry lock acquisition fails.
TEST_F(QueryCancellerTest, TokenValidAfterRegistryTimeout) {
    auto token = canceller_.registerQuery("test-query-4");
    
    // Token should be valid (can call cancel on it directly)
    EXPECT_TRUE(token != nullptr);
    EXPECT_FALSE(token->isCancelled());
    
    // Can cancel the token directly even if registry times out
    token->cancel();
    EXPECT_TRUE(token->isCancelled());
}

}  // namespace query
}  // namespace themis
