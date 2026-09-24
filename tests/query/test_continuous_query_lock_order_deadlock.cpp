/**
 * @file test_continuous_query_lock_order_deadlock.cpp
 * @brief Regression tests for lock-order deadlock fixes in continuous query engine
 *
 * Validates that the WAVE3B-FIX for blocking_no_timeout deadlock scenarios are properly fixed:
 *   - Nested lock acquisition (inject_mutex_ -> registry_mutex_) is eliminated
 *   - Destructor safely handles timeout in stopLoop()
 *   - Concurrent injection and evaluation don't cause deadlocks
 *
 * Issue: makr-code/ThemisDB#6592
 * Comments: [WAVE3B-FIX: lock-order deadlock in tickOnce()]
 *           [WAVE3B-FIX: lock-order deadlock risk in destructor]
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <optional>

#include "query/continuous_query_engine.h"
#include "query/window_spec.h"
#include "utils/logger.h"

namespace themis {
namespace query {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class ContinuousQueryLockOrderDeadlockTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logger for test output
        THEMIS_INFO("ContinuousQueryLockOrderDeadlockTest: Starting test");
    }

    void TearDown() override {
        THEMIS_INFO("ContinuousQueryLockOrderDeadlockTest: Test complete");
    }

    /**
     * Create a basic continuous query specification for testing
     */
    ContinuousQuerySpec makeTestSpec(const std::string& name) {
        ContinuousQuerySpec spec;
        spec.name = name;
        spec.source_collection = "test_collection";
        spec.aql = "FOR doc IN " + name + " RETURN doc";
        spec.window = WindowSpec::TumblingWindow(std::chrono::seconds(1));
        spec.max_window_tuples = 10000;
        spec.max_window_bytes = 10 * 1024 * 1024;  // 10 MB
        spec.result_mode = ResultMode::DELTA;
        return spec;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test Cases: Concurrent Injection and Evaluation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryLockOrderDeadlockTest, ConcurrentInjectionAndEvaluation) {
    /**
     * Test: Multiple threads simultaneously injecting tuples while the engine
     *       is running its evaluation loop.
     *
     * Validates: The fix for [WAVE3B-FIX: lock-order deadlock in tickOnce()]
     * ensures that inject_mutex_ and registry_mutex_ are never held simultaneously.
     *
     * Scenario:
     *   - Create a continuous query engine and register a query
     *   - Spawn multiple injector threads that rapidly inject tuples
     *   - Let the engine run for a duration while injections occur
     *   - Verify no deadlock or race condition occurs
     *   - Destroy the engine (destructor must not deadlock)
     */
    auto engine = makeContinuousQueryEngine(std::chrono::milliseconds(100));
    ASSERT_TRUE(engine);

    // Register a query
    auto spec = makeTestSpec("concurrent_injection_test");
    auto reg_result = engine->registerQuery(spec);
    ASSERT_TRUE(reg_result);

    const std::string query_name = *reg_result;
    THEMIS_INFO("Registered query: {}", query_name);

    // Subscribe to results
    auto stream_result = engine->subscribe(query_name, ResultMode::DELTA);
    ASSERT_TRUE(stream_result);

    // Start injector threads
    std::atomic<bool> stop_injection{false};
    std::atomic<size_t> injection_count{0};
    std::vector<std::thread> injector_threads;

    const size_t num_injectors = 4;
    const size_t injections_per_thread = 100;

    for (size_t t = 0; t < num_injectors; ++t) {
        injector_threads.emplace_back([&, t]() {
            for (size_t i = 0; i < injections_per_thread && !stop_injection.load(); ++i) {
                std::string payload = "{\"id\": " + std::to_string(t * 1000 + i) + "}";
                engine->injectTuple("test_collection", payload, i * 100);
                injection_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // Let injections run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    stop_injection.store(true);

    // Wait for all injectors to complete
    for (auto& t : injector_threads) {
        t.join();
    }

    THEMIS_INFO("Completed {} tuple injections", injection_count.load());
    EXPECT_GT(injection_count.load(), 0) << "Should have injected some tuples";

    // Engine destruction must not deadlock
    // (destructor calls stopLoop and may encounter timeout,
    //  which should be handled gracefully)
    THEMIS_INFO("Destroying engine (destructor must not deadlock)");
    engine.reset();
    THEMIS_INFO("Engine destroyed successfully");
}

TEST_F(ContinuousQueryLockOrderDeadlockTest, RapidRegisterDropWithInjection) {
    /**
     * Test: Rapid query registration and dropping while concurrent injections occur.
     *
     * Validates: Lock ordering is consistent across registerQuery, dropQuery,
     * injectTuple, and tickOnce operations.
     *
     * Scenario:
     *   - Spawn injector threads
     *   - Simultaneously register and drop queries from main thread
     *   - Verify no deadlock or race condition
     */
    auto engine = makeContinuousQueryEngine(std::chrono::milliseconds(50));
    ASSERT_TRUE(engine);

    std::atomic<bool> stop{false};
    std::atomic<size_t> query_ops{0};
    std::atomic<size_t> injection_count{0};

    // Injector thread
    auto injector = std::thread([&]() {
        for (size_t i = 0; !stop.load(); ++i) {
            engine->injectTuple("test_collection",
                               "{\"data\": \"" + std::to_string(i) + "\"}",
                               i * 100);
            injection_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // Rapid register/drop operations
    for (size_t i = 0; i < 20; ++i) {
        auto spec = makeTestSpec("rapid_" + std::to_string(i));
        auto reg = engine->registerQuery(spec);
        ASSERT_TRUE(reg);
        query_ops++;

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        auto drop = engine->dropQuery(spec.name);
        ASSERT_TRUE(drop);
        query_ops++;
    }

    stop.store(true);
    injector.join();

    THEMIS_INFO("Completed {} query operations and {} injections",
                query_ops.load(), injection_count.load());
    EXPECT_GT(injection_count.load(), 0);

    // Destruction should not deadlock
    engine.reset();
}

TEST_F(ContinuousQueryLockOrderDeadlockTest, DestructorWithPendingInjections) {
    /**
     * Test: Destructor is called while there are pending injections in the queue.
     *
     * Validates: [WAVE3B-FIX: lock-order deadlock risk in destructor]
     * ensures destructor safely handles the case where stopLoop() may timeout.
     *
     * Scenario:
     *   - Register a query
     *   - Inject many tuples
     *   - Immediately destroy the engine without waiting for processing
     *   - Verify destructor completes without deadlock
     */
    {
        auto engine = makeContinuousQueryEngine(std::chrono::milliseconds(1000));
        ASSERT_TRUE(engine);

        auto spec = makeTestSpec("pending_injections_test");
        auto reg = engine->registerQuery(spec);
        ASSERT_TRUE(reg);

        // Inject tuples rapidly
        for (size_t i = 0; i < 50; ++i) {
            std::string payload = "{\"batch\": " + std::to_string(i) + "}";
            engine->injectTuple("test_collection", payload, i);
        }

        THEMIS_INFO("Injected 50 tuples, now destroying engine immediately");
        // Destructor is called here while loop is processing
    }
    THEMIS_INFO("Engine destroyed successfully");
}

TEST_F(ContinuousQueryLockOrderDeadlockTest, ConcurrentSubscribers) {
    /**
     * Test: Multiple subscribers concurrently receiving results while injections occur.
     *
     * Validates: Subscription result queue thread-safety alongside injection
     * and evaluation with proper lock ordering.
     *
     * Scenario:
     *   - Register a query with multiple subscribers
     *   - Spawn injector threads and result consumer threads
     *   - Verify all operations complete without deadlock
     */
    auto engine = makeContinuousQueryEngine(std::chrono::milliseconds(50));
    ASSERT_TRUE(engine);

    auto spec = makeTestSpec("multi_subscriber_test");
    auto reg = engine->registerQuery(spec);
    ASSERT_TRUE(reg);
    const std::string query_name = *reg;

    // Create multiple subscribers
    const size_t num_subscribers = 3;
    std::vector<ContinuousQueryEngine::ResultStreamPtr> streams;
    for (size_t i = 0; i < num_subscribers; ++i) {
        auto stream_result = engine->subscribe(query_name, ResultMode::DELTA);
        ASSERT_TRUE(stream_result);
        streams.push_back(*stream_result);
    }

    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // Injector threads
    for (size_t i = 0; i < 2; ++i) {
        threads.emplace_back([&]() {
            for (size_t j = 0; !stop.load(); ++j) {
                engine->injectTuple("test_collection",
                                   "{\"value\": " + std::to_string(j) + "}",
                                   j * 100);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }

    // Let everything run for a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop.store(true);

    for (auto& t : threads) {
        t.join();
    }

    THEMIS_INFO("Completed concurrent subscription test with {} subscribers", num_subscribers);
    engine.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Stress Test: High-Concurrency Scenario
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ContinuousQueryLockOrderDeadlockTest, HighConcurrencyStressTest) {
    /**
     * Test: Extreme concurrent scenario to stress-test lock ordering.
     *
     * This test creates a high-concurrency environment to increase the likelihood
     * of deadlock if the lock ordering is incorrect.
     */
    auto engine = makeContinuousQueryEngine(std::chrono::milliseconds(25));
    ASSERT_TRUE(engine);

    std::atomic<bool> stop{false};
    std::atomic<size_t> total_ops{0};

    auto spec = makeTestSpec("stress_test");
    auto reg = engine->registerQuery(spec);
    ASSERT_TRUE(reg);

    // Multiple concurrent threads doing different operations
    std::vector<std::thread> worker_threads;
    const size_t num_workers = 8;

    for (size_t w = 0; w < num_workers; ++w) {
        worker_threads.emplace_back([&, w]() {
            for (size_t i = 0; !stop.load(); ++i) {
                if (w % 3 == 0) {
                    // Injector thread
                    engine->injectTuple("test_collection",
                                       "{\"worker\": " + std::to_string(w) + "}",
                                       i);
                } else if (w % 3 == 1) {
                    // Register/drop thread
                    std::string query_name = "query_" + std::to_string(w) + "_" + std::to_string(i);
                    auto spec2 = makeTestSpec(query_name);
                    if (auto reg2 = engine->registerQuery(spec2)) {
                        total_ops++;
                        if (i % 5 == 0) {
                            engine->dropQuery(query_name);
                            total_ops++;
                        }
                    }
                } else {
                    // Subscribe thread
                    if (auto stream = engine->subscribe("stress_test", ResultMode::DELTA)) {
                        total_ops++;
                    }
                }
                total_ops++;
            }
        });
    }

    // Run stress test for 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    stop.store(true);

    for (auto& t : worker_threads) {
        t.join();
    }

    THEMIS_INFO("Stress test completed: {} total operations", total_ops.load());
    EXPECT_GT(total_ops.load(), 0);

    // Destruction must complete without timeout or deadlock
    engine.reset();
}

}  // namespace
}  // namespace query
}  // namespace themis
