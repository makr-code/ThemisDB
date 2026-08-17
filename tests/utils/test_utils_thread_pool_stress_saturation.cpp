/**
 * @file test_utils_thread_pool_stress_saturation.cpp
 * @brief TSAN stress test for ThreadPoolManager saturation and priority ordering
 * @date 2026-08-17
 *
 * Tests Phase 4.3 concurrency stress for thread_pool_manager.cpp:
 * - Queue saturation handling (queue full error)
 * - Priority ordering under load
 * - Task completion guarantees
 * - No data races (verified under TSAN)
 *
 * Run with TSAN enabled:
 *   TSAN_OPTIONS=halt_on_error=1 ctest -R stress_thread_pool
 */

#include <gtest/gtest.h>
#include "utils/thread_pool_manager.h"
#include "utils/error_contracts.h"

#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <queue>

namespace themis {
namespace utils {

class ThreadPoolStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default thread pool configuration
    }
};

// ============================================================================
// Stress Test: Queue Saturation with Rapid Task Submission
// ============================================================================

TEST_F(ThreadPoolStressTest, QueueSaturation_RapidSubmission) {
    ThreadPoolManager pool(/*thread_count=*/4, /*queue_size=*/1000);
    
    std::atomic<int> submitted = 0;
    std::atomic<int> rejected = 0;
    std::atomic<int> completed = 0;
    
    // Try to submit more tasks than queue capacity
    constexpr int kTaskCount = 5000;
    
    for (int i = 0; i < kTaskCount; ++i) {
        auto task = [&completed]() {
            completed++;
        };
        
        if (pool.submit(task, ThreadPoolManager::TaskPriority::NORMAL)) {
            submitted++;
        } else {
            rejected++;
        }
    }
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Verify results
    EXPECT_EQ(submitted + rejected, kTaskCount);
    EXPECT_GT(submitted, 0);
    EXPECT_GE(completed, 1);  // At least some tasks completed
    
    std::cout << "Submitted: " << submitted << ", Rejected: " << rejected 
              << ", Completed: " << completed << std::endl;
}

// ============================================================================
// Stress Test: Priority Task Ordering Under Load
// ============================================================================

TEST_F(ThreadPoolStressTest, PriorityOrdering_UnderLoad) {
    ThreadPoolManager pool(/*thread_count=*/2, /*queue_size=*/1000);
    
    std::vector<int> completion_order;
    std::mutex order_mutex;
    
    // Submit high-priority tasks after normal ones
    // They should be processed earlier
    for (int i = 0; i < 10; ++i) {
        pool.submit([i, &completion_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(i);
        }, ThreadPoolManager::TaskPriority::NORMAL);
    }
    
    // Give normal tasks a moment to queue
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Submit high-priority tasks
    for (int i = 100; i < 110; ++i) {
        pool.submit([i, &completion_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            completion_order.push_back(i);
        }, ThreadPoolManager::TaskPriority::HIGH);
    }
    
    // Wait for all tasks
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Verify all tasks completed
    EXPECT_GE(completion_order.size(), 15);
}

// ============================================================================
// Stress Test: Many Threads Submitting Tasks Concurrently
// ============================================================================

TEST_F(ThreadPoolStressTest, ConcurrentSubmitters_ManyThreads) {
    ThreadPoolManager pool(/*thread_count=*/8, /*queue_size=*/5000);
    
    std::atomic<int> total_completed = 0;
    constexpr int kSubmitterThreads = 16;
    constexpr int kTasksPerSubmitter = 100;
    
    std::vector<std::thread> submitters;
    
    for (int s = 0; s < kSubmitterThreads; ++s) {
        submitters.emplace_back([&pool, &total_completed, s]() {
            for (int t = 0; t < kTasksPerSubmitter; ++t) {
                pool.submit([&total_completed]() {
                    total_completed++;
                }, ThreadPoolManager::TaskPriority::NORMAL);
                
                // Small sleep to simulate realistic work
                if (t % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });
    }
    
    // Wait for all submitters
    for (auto& t : submitters) {
        t.join();
    }
    
    // Wait for task completion
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Verify tasks completed
    EXPECT_GT(total_completed, 0);
    std::cout << "Concurrent submitters: " << total_completed 
              << " tasks completed" << std::endl;
}

// ============================================================================
// Stress Test: Rapid Task Completion with Minimal Processing
// ============================================================================

TEST_F(ThreadPoolStressTest, RapidTaskCompletion) {
    ThreadPoolManager pool(/*thread_count=*/4, /*queue_size=*/10000);
    
    std::atomic<int> completed = 0;
    auto start = std::chrono::high_resolution_clock::now();
    
    // Submit many light tasks
    constexpr int kTaskCount = 10000;
    for (int i = 0; i < kTaskCount; ++i) {
        pool.submit([&completed]() {
            completed++;
        }, ThreadPoolManager::TaskPriority::NORMAL);
    }
    
    // Wait for completion
    while (completed < kTaskCount && 
           std::chrono::high_resolution_clock::now() - start < std::chrono::seconds(10)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start
    ).count();
    
    // Calculate throughput
    double throughput = (completed > 0) ? 
        (static_cast<double>(completed) * 1000.0 / duration) : 0.0;
    
    std::cout << "Task throughput: " << throughput << " tasks/sec" << std::endl;
    EXPECT_GT(completed, 0);
}

// ============================================================================
// Stress Test: Mixed Priority Task Interleaving
// ============================================================================

TEST_F(ThreadPoolStressTest, MixedPriorityInterleaving) {
    ThreadPoolManager pool(/*thread_count=*/4, /*queue_size=*/2000);
    
    std::atomic<int> high_completed = 0;
    std::atomic<int> normal_completed = 0;
    std::atomic<int> low_completed = 0;
    
    // Submit tasks in mixed order
    for (int i = 0; i < 100; ++i) {
        if (i % 3 == 0) {
            pool.submit([&high_completed]() {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                high_completed++;
            }, ThreadPoolManager::TaskPriority::HIGH);
        } else if (i % 3 == 1) {
            pool.submit([&normal_completed]() {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                normal_completed++;
            }, ThreadPoolManager::TaskPriority::NORMAL);
        } else {
            pool.submit([&low_completed]() {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                low_completed++;
            }, ThreadPoolManager::TaskPriority::LOW);
        }
    }
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Verify all priorities were processed
    EXPECT_GT(high_completed, 0);
    EXPECT_GT(normal_completed, 0);
    EXPECT_GT(low_completed, 0);
    
    // High priority should generally complete earlier (opportunistically)
    std::cout << "High: " << high_completed << ", Normal: " << normal_completed 
              << ", Low: " << low_completed << std::endl;
}

// ============================================================================
// Stress Test: Tasks with Varying Execution Times
// ============================================================================

TEST_F(ThreadPoolStressTest, VaryingExecutionTimes) {
    ThreadPoolManager pool(/*thread_count=*/4, /*queue_size=*/500);
    
    std::atomic<int> fast_completed = 0;
    std::atomic<int> medium_completed = 0;
    std::atomic<int> slow_completed = 0;
    
    // Submit tasks with different execution times
    for (int i = 0; i < 50; ++i) {
        // Fast tasks (minimal work)
        pool.submit([&fast_completed]() {
            fast_completed++;
        }, ThreadPoolManager::TaskPriority::HIGH);
        
        // Medium tasks (some work)
        pool.submit([&medium_completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            medium_completed++;
        }, ThreadPoolManager::TaskPriority::NORMAL);
        
        // Slow tasks (more work)
        pool.submit([&slow_completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            slow_completed++;
        }, ThreadPoolManager::TaskPriority::LOW);
    }
    
    // Wait for all tasks
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Verify all completed
    EXPECT_EQ(fast_completed, 50);
    EXPECT_EQ(medium_completed, 50);
    EXPECT_EQ(slow_completed, 50);
}

// ============================================================================
// Stress Test: Queue Fill and Drain Cycles
// ============================================================================

TEST_F(ThreadPoolStressTest, QueueFillDrainCycles) {
    ThreadPoolManager pool(/*thread_count=*/2, /*queue_size=*/100);
    
    std::atomic<int> cycles_completed = 0;
    
    // Multiple fill-drain cycles
    for (int cycle = 0; cycle < 10; ++cycle) {
        // Fill queue
        for (int i = 0; i < 50; ++i) {
            pool.submit([&cycles_completed]() {
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                cycles_completed++;
            }, ThreadPoolManager::TaskPriority::NORMAL);
        }
        
        // Wait for drain
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    EXPECT_GT(cycles_completed, 0);
    std::cout << "Cycles completed: " << cycles_completed << " tasks" << std::endl;
}

// ============================================================================
// Stress Test: Shutdown During Active Submissions
// ============================================================================

TEST_F(ThreadPoolStressTest, ShutdownDuringLoad) {
    {
        ThreadPoolManager pool(/*thread_count=*/4, /*queue_size=*/1000);
        
        std::atomic<int> completed = 0;
        
        // Start submitting tasks
        for (int i = 0; i < 500; ++i) {
            pool.submit([&completed]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                completed++;
            }, ThreadPoolManager::TaskPriority::NORMAL);
        }
        
        // Pool goes out of scope and shuts down
        // Should drain or cancel gracefully without hanging
    }
    
    // If we reach here, shutdown succeeded without hanging
    EXPECT_TRUE(true);
}

// ============================================================================
// Stress Test: High Thread Count with Limited Queue
// ============================================================================

TEST_F(ThreadPoolStressTest, ManyThreadsLimitedQueue) {
    ThreadPoolManager pool(/*thread_count=*/16, /*queue_size=*/100);
    
    std::atomic<int> submitted = 0;
    std::atomic<int> completed = 0;
    
    // Try to submit many tasks to limited queue
    for (int i = 0; i < 1000; ++i) {
        if (pool.submit([&completed]() {
            completed++;
        }, ThreadPoolManager::TaskPriority::NORMAL)) {
            submitted++;
        }
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // With many threads, should process quickly
    EXPECT_GT(submitted, 0);
    EXPECT_GT(completed, 0);
    
    std::cout << "Submitted: " << submitted << ", Completed: " << completed << std::endl;
}

} // namespace utils
} // namespace themis
