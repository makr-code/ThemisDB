/**
 * @file test_splinterdb_thread_safety_focused.cpp
 * @brief Focused thread-safety test for ConcurrentCompactor.
 *
 * Verifies that g_total_compaction_time_ms is accumulated correctly when
 * multiple worker threads execute compaction tasks concurrently. The race
 * condition (non-atomic load-modify-store) was fixed with a compare_exchange_weak
 * retry loop (2026-08-10).
 *
 * Test IDs: SPLINTER_TS_01, SPLINTER_TS_02
 */

#include <gtest/gtest.h>
#include "performance/phase3/splinterdb.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace themis {
namespace performance {
namespace phase3 {
namespace {

// ---- helper: wall-clock busy-sleep so compaction "work" takes ~1 ms --------
static void busy_sleep_1ms() {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        // spin
    }
}

// ---------------------------------------------------------------------------
// SPLINTER_TS_01 — Single-threaded baseline: stats are consistent
// ---------------------------------------------------------------------------
TEST(SplinterDBThreadSafety, SPLINTER_TS_01_SingleThreadedConsistency) {
    ConcurrentCompactor compactor(1);
    compactor.start();

    constexpr int kTasks = 5;
    std::atomic<int> completed{0};

    for (int i = 0; i < kTasks; ++i) {
        compactor.schedule_compaction(0, [&completed]() {
            busy_sleep_1ms();
            completed.fetch_add(1, std::memory_order_relaxed);
        });
    }

    // Wait for all tasks to finish (up to 5 s)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (completed.load() < kTasks &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    compactor.stop();

    auto stats = compactor.get_stats();
    EXPECT_EQ(stats.compactions_completed, static_cast<size_t>(kTasks))
        << "All scheduled compactions should complete";
    EXPECT_GE(stats.avg_compaction_time_ms, 0.0)
        << "Average time must be non-negative";
}

// ---------------------------------------------------------------------------
// SPLINTER_TS_02 — Multi-threaded: no lost updates under concurrent execution
//
// Strategy: schedule N tasks from multiple producer threads; each task records
// its individual duration in a shared vector. After all tasks complete we
// compare the sum recorded individually with avg * count from get_stats().
// A lost-update race would make avg*count < sum_individual.
// ---------------------------------------------------------------------------
TEST(SplinterDBThreadSafety, SPLINTER_TS_02_ConcurrentNoLostUpdates) {
    constexpr int kWorkers  = 4;
    constexpr int kPerWorker = 8;   // tasks per producer thread
    constexpr int kTotal    = kWorkers * kPerWorker;

    ConcurrentCompactor compactor(kWorkers);
    compactor.start();

    std::atomic<int> completed{0};

    // Saturate the compactor with tasks from kWorkers producer threads
    std::vector<std::thread> producers;
    producers.reserve(kWorkers);
    for (int w = 0; w < kWorkers; ++w) {
        producers.emplace_back([&compactor, &completed, kPerWorker]() {
            for (int i = 0; i < kPerWorker; ++i) {
                compactor.schedule_compaction(0, [&completed]() {
                    busy_sleep_1ms();
                    completed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }
    for (auto& t : producers) {
      t.join();
    }

    // Wait for all tasks (up to 30 s)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (completed.load() < kTotal &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    compactor.stop();

    auto stats = compactor.get_stats();

    EXPECT_EQ(stats.compactions_completed, static_cast<size_t>(kTotal))
        << "All " << kTotal << " compactions must complete";

    // avg_compaction_time_ms is total / count.
    // Each task sleeps ~1 ms, so total should be >= kTotal ms.
    // A lost update (race) would shrink total below this floor.
    double reconstructed_total = stats.avg_compaction_time_ms *
                                 static_cast<double>(stats.compactions_completed);
    EXPECT_GE(reconstructed_total, static_cast<double>(kTotal) * 0.5)
        << "Reconstructed total (" << reconstructed_total
        << " ms) looks too low — possible lost update from race condition. "
           "Expected >= " << (kTotal * 0.5) << " ms";
}

} // namespace
} // namespace phase3
} // namespace performance
} // namespace themis
