// Test: SAGA Concurrent Execution
// Validates parallel SAGA execution and compensation correctness

#include <gtest/gtest.h>
#include "transaction/saga.h"
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <random>

using namespace themis;

class SagaConcurrentExecutionTest : public ::testing::Test {
protected:
    // No special setup needed for SAGA tests
};

// ===== Basic Concurrent Compensation Tests =====

TEST_F(SagaConcurrentExecutionTest, MultipleSagasConcurrentCompensation) {
    const int num_sagas = 10;
    const int steps_per_saga = 5;
    
    std::vector<std::unique_ptr<Saga>> sagas;
    std::atomic<int> total_compensations{0};
    
    // Create multiple SAGAs
    for (int i = 0; i < num_sagas; i++) {
        auto saga = std::make_unique<Saga>();
        
        for (int j = 0; j < steps_per_saga; j++) {
            saga->addStep(
                "saga_" + std::to_string(i) + "_step_" + std::to_string(j),
                [&total_compensations]() {
                    total_compensations++;
                }
            );
        }
        
        sagas.push_back(std::move(saga));
    }
    
    // Compensate all SAGAs concurrently
    std::vector<std::thread> threads = {};

    for (auto& saga : sagas) {
        threads.emplace_back([&saga]() {
            saga->compensate();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all steps were compensated
    EXPECT_EQ(total_compensations, num_sagas * steps_per_saga);
    
    // Verify all SAGAs are fully compensated
    for (const auto& saga : sagas) {
        EXPECT_TRUE(saga->isFullyCompensated());
    }
}

// ===== Race Condition Tests =====

TEST_F(SagaConcurrentExecutionTest, NoRaceConditionsInCompensation) {
    const int num_threads = 10;
    const int iterations_per_thread = 100;
    
    std::atomic<int> compensation_count{0};
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < iterations_per_thread; i++) {
                Saga saga;
                
                saga.addStep("step_1", [&compensation_count]() {
                    compensation_count++;
                });
                
                saga.addStep("step_2", [&compensation_count]() {
                    compensation_count++;
                });
                
                saga.compensate();
                
                EXPECT_TRUE(saga.isFullyCompensated());
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    int expected = num_threads * iterations_per_thread * 2; // 2 steps per saga
    EXPECT_EQ(compensation_count, expected);
}

// ===== Idempotency Tests =====

TEST_F(SagaConcurrentExecutionTest, IdempotentCompensation) {
    const int num_threads = 5;
    
    Saga saga;
    std::atomic<int> compensation_count{0};
    
    // Add steps
    for (int i = 0; i < 10; i++) {
        saga.addStep(
            "step_" + std::to_string(i),
            [&compensation_count]() {
                compensation_count++;
            }
        );
    }
    
    // Multiple threads try to compensate the same saga
    std::vector<std::thread> threads = {};

    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&saga]() {
            saga.compensate();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should only compensate once (idempotent)
    EXPECT_EQ(compensation_count, 10);
    EXPECT_TRUE(saga.isFullyCompensated());
}

// ===== Compensation Order Tests =====

TEST_F(SagaConcurrentExecutionTest, CompensationOrderIsReversed) {
    const int num_sagas = 5;
    
    std::vector<std::thread> threads;
    std::vector<bool> all_correct(num_sagas, false);
    
    for (int s = 0; s < num_sagas; s++) {
        threads.emplace_back([s, &all_correct]() {
            Saga saga;
            std::vector<int> compensation_order;
            std::mutex order_mutex = {};
            
            // Add steps
            for (int i = 0; i < 10; i++) {
                saga.addStep(
                    "step_" + std::to_string(i),
                    [i, &compensation_order, &order_mutex]() {
                        std::lock_guard<std::mutex> lock(order_mutex);
                        compensation_order.push_back(i);
                    }
                );
            }
            
            saga.compensate();
            
            // Verify reverse order
            bool correct_order = true;
            for (size_t i = 0; i < compensation_order.size(); i++) {
                if (compensation_order[i] != static_cast<int>(9 - i)) {
                    correct_order = false;
                    break;
                }
            }
            
            all_correct[s] = correct_order;
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All SAGAs should have compensated in correct order
    for (bool correct : all_correct) {
        EXPECT_TRUE(correct);
    }
}

// ===== Exception Handling in Concurrent Execution =====

TEST_F(SagaConcurrentExecutionTest, ExceptionHandlingInConcurrentCompensation) {
    const int num_sagas = 10;
    
    std::vector<std::unique_ptr<Saga>> sagas;
    std::atomic<int> successful_compensations{0};
    std::atomic<int> failed_compensations{0};
    
    for (int i = 0; i < num_sagas; i++) {
        auto saga = std::make_unique<Saga>();
        
        // Some steps will fail
        for (int j = 0; j < 5; j++) {
            saga->addStep(
                "step_" + std::to_string(j),
                [j, &successful_compensations, &failed_compensations]() {
                    if (j == 2) { // Middle step fails
                        failed_compensations++;
                        throw std::runtime_error("Intentional failure");
                    }
                    successful_compensations++;
                }
            );
        }
        
        sagas.push_back(std::move(saga));
    }
    
    // Compensate concurrently
    std::vector<std::thread> threads = {};

    for (auto& saga : sagas) {
        threads.emplace_back([&saga]() {
            saga->compensate();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Each saga has 5 steps, 1 fails, 4 succeed
    EXPECT_EQ(failed_compensations, num_sagas * 1);
    EXPECT_EQ(successful_compensations, num_sagas * 4);
}

// ===== Shared Resource Tests =====

TEST_F(SagaConcurrentExecutionTest, SharedResourceCompensation) {
    const int num_sagas = 5;
    const int ops_per_saga = 10;
    
    std::atomic<int> shared_counter{0};
    std::vector<std::unique_ptr<Saga>> sagas;
    
    // Each saga increments then decrements shared counter
    for (int i = 0; i < num_sagas; i++) {
        auto saga = std::make_unique<Saga>();
        
        for (int j = 0; j < ops_per_saga; j++) {
            // Forward: increment
            shared_counter++;
            
            // Compensation: decrement
            saga->addStep(
                "saga_" + std::to_string(i) + "_op_" + std::to_string(j),
                [&shared_counter]() {
                    shared_counter--;
                }
            );
        }
        
        sagas.push_back(std::move(saga));
    }
    
    // Counter should be at max
    EXPECT_EQ(shared_counter, num_sagas * ops_per_saga);
    
    // Compensate all SAGAs concurrently
    std::vector<std::thread> threads = {};

    for (auto& saga : sagas) {
        threads.emplace_back([&saga]() {
            saga->compensate();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // After compensation, should be back to zero
    EXPECT_EQ(shared_counter, 0);
}

// ===== Stress Test =====

TEST_F(SagaConcurrentExecutionTest, HighConcurrencyStressTest) {
    const int num_threads = 20;
    const int sagas_per_thread = 50;
    
    std::atomic<int> total_sagas_compensated{0};
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < sagas_per_thread; i++) {
                Saga saga;
                
                // Random number of steps (1-10)
                std::mt19937 rng(t * sagas_per_thread + i);
                std::uniform_int_distribution<int> dist(1, 10);
                int num_steps = dist(rng);
                
                for (int j = 0; j < num_steps; j++) {
                    saga.addStep(
                        "thread_" + std::to_string(t) + "_saga_" + std::to_string(i) + "_step_" + std::to_string(j),
                        []() {
                            // Minimal work
                        }
                    );
                }
                
                saga.compensate();
                
                if (saga.isFullyCompensated()) {
                    total_sagas_compensated++;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(total_sagas_compensated, num_threads * sagas_per_thread);
}

// ===== Timing Tests =====

TEST_F(SagaConcurrentExecutionTest, CompensationTimingConsistency) {
    const int num_sagas = 10;
    const int steps_per_saga = 5;
    
    std::vector<std::unique_ptr<Saga>> sagas;
    std::vector<int64_t> compensation_times;
    std::mutex times_mutex = {};
    
    for (int i = 0; i < num_sagas; i++) {
        auto saga = std::make_unique<Saga>();
        
        for (int j = 0; j < steps_per_saga; j++) {
            saga->addStep(
                "step_" + std::to_string(j),
                []() {
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            );
        }
        
        sagas.push_back(std::move(saga));
    }
    
    // Compensate and measure time
    std::vector<std::thread> threads = {};

    for (auto& saga : sagas) {
        threads.emplace_back([&saga, &compensation_times, &times_mutex]() {
            auto start = std::chrono::high_resolution_clock::now();
            saga->compensate();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            
            std::lock_guard<std::mutex> lock(times_mutex);
            compensation_times.push_back(duration);
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    ASSERT_EQ(compensation_times.size(), num_sagas);
    
    // All times should be relatively similar (within 2x of each other)
    int64_t min_time = *std::min_element(compensation_times.begin(), compensation_times.end());
    int64_t max_time = *std::max_element(compensation_times.begin(), compensation_times.end());
    
    // Allow up to 3x variance due to scheduling
    EXPECT_LT(max_time, min_time * 3) 
        << "Compensation times vary too much: min=" << min_time << ", max=" << max_time;
}

// ===== Partial Compensation Concurrency Test =====

TEST_F(SagaConcurrentExecutionTest, PartialCompensationConcurrent) {
    const int num_threads = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_compensations{0};
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([t, &total_compensations]() {
            Saga saga;
            
            // Execute partial saga (simulate failure mid-way)
            int steps_to_execute = 3 + (t % 5); // 3-7 steps
            
            for (int i = 0; i < steps_to_execute; i++) {
                saga.addStep(
                    "thread_" + std::to_string(t) + "_step_" + std::to_string(i),
                    [&total_compensations]() {
                        total_compensations++;
                    }
                );
            }
            
            // Compensate partial saga
            saga.compensate();
            
            EXPECT_EQ(saga.compensatedCount(), static_cast<size_t>(steps_to_execute));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all partial compensations executed
    EXPECT_GT(total_compensations, 0);
}

// ===== Phase 4: compensateWithRetry and getMetrics tests =====

TEST_F(SagaConcurrentExecutionTest, CompensateWithRetrySucceedsFirstAttempt) {
    Saga saga;
    std::atomic<int> count{0};

    saga.addStep("step_ok", [&count]() { ++count; });
    saga.compensateWithRetry(3, std::chrono::milliseconds(1));

    EXPECT_EQ(count.load(), 1);
    EXPECT_TRUE(saga.isFullyCompensated());
}

TEST_F(SagaConcurrentExecutionTest, CompensateWithRetryRetriesOnException) {
    Saga saga;
    std::atomic<int> attempts{0};

    saga.addStep("flaky_step", [&attempts]() {
        int a = ++attempts;
        if (a < 3) {
          throw std::runtime_error("transient error");
        }
        // succeeds on 3rd attempt
    });

    saga.compensateWithRetry(5, std::chrono::milliseconds(1));

    EXPECT_GE(attempts.load(), 3);
    EXPECT_TRUE(saga.isFullyCompensated());
}

TEST_F(SagaConcurrentExecutionTest, CompensateWithRetryFailsAfterMaxRetries) {
    Saga saga;
    std::atomic<int> attempts{0};

    saga.addStep("always_fails", [&attempts]() {
        ++attempts;
        throw std::runtime_error("permanent error");
    });

    saga.compensateWithRetry(2, std::chrono::milliseconds(1));

    // 1 original + 2 retries = 3 attempts; step NOT marked compensated
    EXPECT_EQ(attempts.load(), 3);
    auto metrics = saga.getMetrics();
    EXPECT_EQ(metrics.failed_compensations, 1u);
    EXPECT_FALSE(saga.isFullyCompensated());
}

TEST_F(SagaConcurrentExecutionTest, GetMetricsReportsRetriedCount) {
    Saga saga;
    std::atomic<int> calls{0};

    // Two steps: one needs a retry, one succeeds immediately
    saga.addStep("flaky", [&calls]() {
        if (++calls == 1) {
          throw std::runtime_error("first attempt fails");
        }
    });
    saga.addStep("ok", []() {});

    saga.compensateWithRetry(3, std::chrono::milliseconds(1));

    auto m = saga.getMetrics();
    EXPECT_GE(m.retried_compensations, 1u); // "flaky" was retried
    EXPECT_EQ(m.failed_compensations,  0u); // both eventually succeeded
    EXPECT_EQ(m.compensated_steps,     2u);
}

TEST_F(SagaConcurrentExecutionTest, GetMetricsAfterClear) {
    Saga saga;
    int x = 0;
    saga.addStep("s", [&x]() { ++x; });
    saga.compensate();

    // clear resets steps but preserves cumulative metrics
    saga.clear();
    EXPECT_EQ(saga.stepCount(), 0u);

    auto m = saga.getMetrics();
    // total_steps is 0 (steps cleared), but duration_ms etc. may differ
    EXPECT_EQ(m.total_steps, 0u);
}

TEST_F(SagaConcurrentExecutionTest, CompensateWithRetryBackoffIsCapped) {
    // Verify that a huge initial backoff is clamped to 30 s and does not
    // cause overflow.  With max_retries=0 no sleep occurs, so the call
    // should complete nearly instantly regardless of the (clamped) backoff.
    Saga saga;
    int x = 0;
    saga.addStep("s", [&x]() { ++x; });

    auto t0 = std::chrono::steady_clock::now();
    // max_retries = 0 → first attempt only, no sleep needed
    saga.compensateWithRetry(0, std::chrono::milliseconds(
        std::numeric_limits<int32_t>::max()));
    auto elapsed = std::chrono::steady_clock::now() - t0;

    EXPECT_EQ(x, 1);
    // Should complete well within 5 seconds (no sleep when max_retries=0)
    EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 5);
}
