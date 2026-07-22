/*
 * ThemisDB | File: test_thread_pool_manager.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


/**
 * @file test_thread_pool_manager.cpp
 * @brief Tests for ThreadPoolManager
 * 
 * Tests the thread pool infrastructure:
 * - Task submission and execution
 * - Queue management and overflow handling
 * - Multiple pool types (IO, CPU, Blocking)
 * - Statistics and metrics
 * - Graceful shutdown
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <future>
#include <memory>

#include "utils/thread_pool_manager.h"

using namespace std::chrono_literals;

namespace themis {
namespace utils {
namespace test {

/**
 * @brief Test basic task submission and execution
 */
TEST(ThreadPoolTest, SubmitAndExecute) {
    ThreadPool::Config config;
    config.name = "test_pool";
    config.min_threads = 2;
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<bool> executed{false};
    auto task = std::make_shared<Task>(
        [&executed]() { executed = true; },
        Task::Priority::NORMAL,
        "test_task"
    );
    
    EXPECT_TRUE(pool->submit(task));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(executed);
}

/**
 * @brief Test queue full scenario
 */
TEST(ThreadPoolTest, QueueFull) {
    ThreadPool::Config config;
    config.queue_size = 1;  // Only 1 task can be queued
    config.min_threads = 1;
    config.max_threads = 1;  // Prevent dynamic thread growth
    config.name = "queue_full_test";
    
    auto pool = std::make_unique<ThreadPool>(config);

    std::promise<void> blocking_started;
    auto blocking_started_future = blocking_started.get_future();
    std::promise<void> release_blocking_task;
    auto release_blocking_task_future = release_blocking_task.get_future().share();

    auto blocking_task = std::make_shared<Task>(
        [&blocking_started, release_blocking_task_future]() {
            blocking_started.set_value();
            release_blocking_task_future.wait();
        },
        Task::Priority::NORMAL,
        "blocking_task"
    );
    ASSERT_TRUE(pool->submit(blocking_task));
    ASSERT_EQ(blocking_started_future.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    auto queued_task = std::make_shared<Task>(
        []() {},
        Task::Priority::NORMAL,
        "queued_task"
    );
    ASSERT_TRUE(pool->submit(queued_task));
    
    // Now: 1 task executing, 1 task in queue (queue full)
    // Next should fail (queue full, worker busy)
    auto task = std::make_shared<Task>([]() {}, Task::Priority::NORMAL, "overflow_task");
    bool result = pool->submit(task, std::chrono::milliseconds(50));  // Short timeout
    EXPECT_FALSE(result);

    release_blocking_task.set_value();
}

/**
 * @brief Test statistics collection
 */
TEST(ThreadPoolTest, Statistics) {
    ThreadPool::Config config;
    config.name = "stats_test";
    config.min_threads = 2;
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<int> counter{0};
    
    // Submit multiple tasks
    for (int i = 0; i < 5; i++) {
        auto task = std::make_shared<Task>(
            [&counter]() { 
                counter++; 
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            },
            Task::Priority::NORMAL,
            "stats_task"
        );
        pool->submit(task);
    }
    
    // Wait for tasks to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto stats = pool->getStatistics();
    EXPECT_EQ(stats.total_executed, 5);
    EXPECT_EQ(stats.total_failed, 0);
    EXPECT_EQ(counter, 5);
}

/**
 * @brief Test graceful shutdown
 */
TEST(ThreadPoolTest, GracefulShutdown) {
    ThreadPool::Config config;
    config.name = "shutdown_test";
    config.min_threads = 2;
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<int> counter{0};
    
    // Submit tasks
    for (int i = 0; i < 3; i++) {
        auto task = std::make_shared<Task>(
            [&counter]() { 
                counter++; 
            },
            Task::Priority::NORMAL,
            "shutdown_task"
        );
        pool->submit(task);
    }
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Shutdown
    pool->shutdown();
    
    EXPECT_FALSE(pool->isRunning());
    EXPECT_EQ(counter, 3);
}

/**
 * @brief Test ThreadPoolManager with multiple pool types
 */
TEST(ThreadPoolManagerTest, MultiplePoolTypes) {
    ThreadPoolManager::Config config;
    auto manager = std::make_unique<ThreadPoolManager>(config);
    
    std::atomic<int> io_count{0};
    std::atomic<int> cpu_count{0};
    std::atomic<int> blocking_count{0};
    
    // Submit to different pools
    manager->submitTask(
        ThreadPoolManager::PoolType::IO,
        [&io_count]() { io_count++; },
        "io_task"
    );
    manager->submitTask(
        ThreadPoolManager::PoolType::CPU,
        [&cpu_count]() { cpu_count++; },
        "cpu_task"
    );
    manager->submitTask(
        ThreadPoolManager::PoolType::BLOCKING,
        [&blocking_count]() { blocking_count++; },
        "blocking_task"
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_EQ(io_count, 1);
    EXPECT_EQ(cpu_count, 1);
    EXPECT_EQ(blocking_count, 1);
}

/**
 * @brief Test ThreadPoolManager statistics
 */
TEST(ThreadPoolManagerTest, GlobalStatistics) {
    ThreadPoolManager::Config config;
    config.enable_metrics = false;  // Disable background metrics for test
    auto manager = std::make_unique<ThreadPoolManager>(config);
    
    // Submit tasks to different pools
    for (int i = 0; i < 3; i++) {
        manager->submitTask(
            ThreadPoolManager::PoolType::IO,
            []() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); },
            "io_task"
        );
    }
    
    for (int i = 0; i < 2; i++) {
        manager->submitTask(
            ThreadPoolManager::PoolType::CPU,
            []() { std::this_thread::sleep_for(std::chrono::milliseconds(10)); },
            "cpu_task"
        );
    }
    
    // Wait for tasks
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.io_stats.total_executed, 3);
    EXPECT_EQ(stats.cpu_stats.total_executed, 2);
}

/**
 * @brief Test task priority (basic)
 */
TEST(ThreadPoolTest, TaskPriority) {
    ThreadPool::Config config;
    config.name = "priority_test";
    config.min_threads = 1;  // Single thread to ensure sequential execution
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<int> counter{0};
    std::vector<int> execution_order;
    std::mutex order_mutex;
    
    // Submit high priority task
    auto high_task = std::make_shared<Task>(
        [&counter, &execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(1);
            counter++;
        },
        Task::Priority::HIGH,
        "high_priority_task"
    );
    
    // Submit normal priority task
    auto normal_task = std::make_shared<Task>(
        [&counter, &execution_order, &order_mutex]() {
            std::lock_guard<std::mutex> lock(order_mutex);
            execution_order.push_back(2);
            counter++;
        },
        Task::Priority::NORMAL,
        "normal_priority_task"
    );
    
    pool->submit(high_task);
    pool->submit(normal_task);
    
    // Wait for completion
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_EQ(counter, 2);
    EXPECT_EQ(execution_order.size(), 2);
}

/**
 * @brief Test exception handling in tasks
 */
TEST(ThreadPoolTest, ExceptionHandling) {
    ThreadPool::Config config;
    config.name = "exception_test";
    config.min_threads = 2;
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<bool> executed_after_exception{false};
    
    // Submit task that throws
    auto throwing_task = std::make_shared<Task>(
        []() { throw std::runtime_error("Test exception"); },
        Task::Priority::NORMAL,
        "throwing_task"
    );
    
    // Submit normal task
    auto normal_task = std::make_shared<Task>(
        [&executed_after_exception]() { executed_after_exception = true; },
        Task::Priority::NORMAL,
        "normal_task"
    );
    
    pool->submit(throwing_task);
    pool->submit(normal_task);
    
    // Wait for both tasks to complete (longer wait to ensure exception handling)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Normal task should have executed despite exception in first task
    EXPECT_TRUE(executed_after_exception);
    
    auto stats = pool->getStatistics();
    EXPECT_EQ(stats.total_failed, 1);
}

/**
 * @brief Test waitAll functionality
 */
TEST(ThreadPoolTest, WaitAll) {
    ThreadPool::Config config;
    config.name = "wait_test";
    config.min_threads = 2;
    
    auto pool = std::make_unique<ThreadPool>(config);
    
    std::atomic<int> counter{0};
    
    // Submit tasks
    for (int i = 0; i < 5; i++) {
        auto task = std::make_shared<Task>(
            [&counter]() { 
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                counter++;
            },
            Task::Priority::NORMAL,
            "wait_task"
        );
        pool->submit(task);
    }
    
    // Wait for all tasks
    bool completed = pool->waitAll(std::chrono::seconds(2));
    
    EXPECT_TRUE(completed);
    EXPECT_EQ(counter, 5);
}

/**
 * @brief Test global singleton
 */
TEST(ThreadPoolManagerTest, GlobalSingleton) {
    auto& manager1 = getThreadPoolManager();
    auto& manager2 = getThreadPoolManager();
    
    // Should be the same instance
    EXPECT_EQ(&manager1, &manager2);
    
    // Should be able to submit tasks
    std::atomic<bool> executed{false};
    manager1.submitTask(
        ThreadPoolManager::PoolType::IO,
        [&executed]() { executed = true; },
        "singleton_test"
    );
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(executed);
}

} // namespace test
} // namespace utils
} // namespace themis
