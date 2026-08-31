/**
 * @file test_utils_thread_pool_stress_saturation.cpp
 * @brief Focused stress tests for ThreadPoolManager saturation and priorities.
 */

#include <gtest/gtest.h>

#include "utils/thread_pool_manager.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace themis {
namespace utils {

class ThreadPoolStressTest : public ::testing::Test {};

TEST_F(ThreadPoolStressTest, QueueSaturation_RapidSubmission) {
    ThreadPoolManager::Config config;
    config.cpu_pool.min_threads = 1;
    config.cpu_pool.max_threads = 1;
    config.cpu_pool.queue_size = 1;
    config.cpu_pool.name = "stress-cpu";

    ThreadPoolManager manager(config);

    std::atomic<int> completed{0};
    std::atomic<int> accepted{0};
    std::atomic<int> rejected{0};

    for (int i = 0; i < 200; ++i) {
        auto ok = manager.submitTask(
            ThreadPoolManager::PoolType::CPU,
            [&completed]() {
                completed.fetch_add(1, std::memory_order_relaxed);
            },
            "stress-task",
            Task::Priority::NORMAL);

        if (ok) {
            ++accepted;
        } else {
            ++rejected;
        }
    }

    manager.shutdown();

    EXPECT_EQ(accepted + rejected, 200);
    EXPECT_GT(accepted.load(), 0);
}

TEST_F(ThreadPoolStressTest, PrioritySubmissionDoesNotThrow) {
    ThreadPoolManager::Config config;
    config.cpu_pool.min_threads = 2;
    config.cpu_pool.max_threads = 2;
    config.cpu_pool.queue_size = 32;
    config.cpu_pool.name = "priority-cpu";

    ThreadPoolManager manager(config);
    std::atomic<int> executed{0};

    for (int i = 0; i < 20; ++i) {
        EXPECT_TRUE(manager.submitTask(
            ThreadPoolManager::PoolType::CPU,
            [&executed]() {
                executed.fetch_add(1, std::memory_order_relaxed);
            },
            "priority-task",
            (i % 3 == 0) ? Task::Priority::HIGH : Task::Priority::NORMAL));
    }

    manager.shutdown();
    EXPECT_GT(executed.load(), 0);
}

} // namespace utils
} // namespace themis
