// Unit tests for SplinterDB (Phase 3)
// Based on "SplinterDB: Closing the Bandwidth Gap for NVMe Key-Value Stores" (OSDI'20)

#include <gtest/gtest.h>
#include "performance/phase3/splinterdb.h"
#include <thread>
#include <atomic>
#include <chrono>

using namespace themis::performance::phase3;

// ==================== ConcurrentCompactor Tests ====================

TEST(ConcurrentCompactorTest, Construction) {
    ConcurrentCompactor compactor(4);
    auto stats = compactor.get_stats();
    
    EXPECT_EQ(stats.compactions_completed, 0u);
    EXPECT_EQ(stats.compactions_in_progress, 0u);
    EXPECT_EQ(stats.avg_compaction_time_ms, 0.0);
}

TEST(ConcurrentCompactorTest, StartStop) {
    ConcurrentCompactor compactor(2);
    
    // Should start successfully
    compactor.start();
    
    // Give threads time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Should stop successfully
    compactor.stop();
}

TEST(ConcurrentCompactorTest, SingleCompaction) {
    ConcurrentCompactor compactor(2);
    compactor.start();
    
    std::atomic<bool> compaction_executed{false};
    
    // Schedule a compaction task
    compactor.schedule_compaction(0, [&compaction_executed]() {
        compaction_executed.store(true, std::memory_order_relaxed);
    });
    
    // Wait for compaction to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(compaction_executed.load(std::memory_order_relaxed));
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, 1u);
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, MultipleCompactions) {
    ConcurrentCompactor compactor(4);
    compactor.start();
    
    const int num_compactions = 10;
    std::atomic<int> completed_count{0};
    
    // Schedule multiple compactions
    for (int i = 0; i < num_compactions; i++) {
        compactor.schedule_compaction(i % 3, [&completed_count]() {
            completed_count.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    // Wait for all compactions to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_EQ(completed_count.load(std::memory_order_relaxed), num_compactions);
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, static_cast<size_t>(num_compactions));
    EXPECT_EQ(stats.compactions_in_progress, 0u);
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, CompactionWithWork) {
    ConcurrentCompactor compactor(2);
    compactor.start();
    
    std::atomic<int> total_work{0};
    
    // Schedule compactions that do some work
    for (int i = 0; i < 5; i++) {
        compactor.schedule_compaction(0, [&total_work, i]() {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            total_work.fetch_add(i + 1, std::memory_order_relaxed);
        });
    }
    
    // Wait for compactions
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Sum of 1+2+3+4+5 = 15
    EXPECT_EQ(total_work.load(std::memory_order_relaxed), 15);
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, 5u);
    EXPECT_GT(stats.avg_compaction_time_ms, 0.0);
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, StatisticsTracking) {
    ConcurrentCompactor compactor(4);
    compactor.start();
    
    // Schedule compactions with varying durations
    for (int i = 0; i < 5; i++) {
        compactor.schedule_compaction(0, [i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(i * 5));
        });
    }
    
    // Wait for compactions
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, 5u);
    EXPECT_EQ(stats.compactions_in_progress, 0u);
    EXPECT_GT(stats.avg_compaction_time_ms, 0.0);
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, ConcurrentScheduling) {
    ConcurrentCompactor compactor(4);
    compactor.start();
    
    const int num_schedulers = 3;
    const int compactions_per_scheduler = 5;
    std::atomic<int> total_executed{0};
    
    std::vector<std::thread> schedulers;
    
    // Multiple threads scheduling compactions concurrently
    for (int t = 0; t < num_schedulers; t++) {
        schedulers.emplace_back([&compactor, &total_executed, compactions_per_scheduler]() {
            for (int i = 0; i < compactions_per_scheduler; i++) {
                compactor.schedule_compaction(i % 3, [&total_executed]() {
                    total_executed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }
    
    // Wait for schedulers
    for (auto& thread : schedulers) {
        thread.join();
    }
    
    // Wait for all compactions to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    EXPECT_EQ(total_executed.load(std::memory_order_relaxed), 
              num_schedulers * compactions_per_scheduler);
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, 
              static_cast<size_t>(num_schedulers * compactions_per_scheduler));
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, StopWhileCompacting) {
    ConcurrentCompactor compactor(2);
    compactor.start();
    
    std::atomic<int> started{0};
    std::atomic<int> completed{0};
    
    // Schedule long-running compactions
    for (int i = 0; i < 4; i++) {
        compactor.schedule_compaction(0, [&started, &completed]() {
            started.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    // Let some compactions start
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    // Stop (should wait for in-progress compactions)
    compactor.stop();
    
    // All started compactions should complete
    EXPECT_EQ(started.load(std::memory_order_relaxed), 
              completed.load(std::memory_order_relaxed));
}

TEST(ConcurrentCompactorTest, DifferentLevels) {
    ConcurrentCompactor compactor(4);
    compactor.start();
    
    std::atomic<int> level0_count{0};
    std::atomic<int> level1_count{0};
    std::atomic<int> level2_count{0};
    
    // Schedule compactions for different levels
    for (int i = 0; i < 3; i++) {
        compactor.schedule_compaction(0, [&level0_count]() {
            level0_count.fetch_add(1, std::memory_order_relaxed);
        });
        compactor.schedule_compaction(1, [&level1_count]() {
            level1_count.fetch_add(1, std::memory_order_relaxed);
        });
        compactor.schedule_compaction(2, [&level2_count]() {
            level2_count.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    // Wait for compactions
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_EQ(level0_count.load(std::memory_order_relaxed), 3);
    EXPECT_EQ(level1_count.load(std::memory_order_relaxed), 3);
    EXPECT_EQ(level2_count.load(std::memory_order_relaxed), 3);
    
    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, 9u);
    
    compactor.stop();
}

TEST(ConcurrentCompactorTest, RestartAfterStop) {
    ConcurrentCompactor compactor(2);
    
    // First run
    compactor.start();
    
    std::atomic<int> count1{0};
    compactor.schedule_compaction(0, [&count1]() {
        count1.fetch_add(1, std::memory_order_relaxed);
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count1.load(std::memory_order_relaxed), 1);
    
    compactor.stop();
    
    // Second run (restart)
    compactor.start();
    
    std::atomic<int> count2{0};
    compactor.schedule_compaction(0, [&count2]() {
        count2.fetch_add(1, std::memory_order_relaxed);
    });
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(count2.load(std::memory_order_relaxed), 1);
    
    compactor.stop();
}


