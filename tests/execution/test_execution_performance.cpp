#include <gtest/gtest.h>

#include "execution/query_scheduler.h"

#include <chrono>
#include <thread>

namespace themis::execution {

TEST(ExecutionPerformance, ThroughputRemainsHealthyForSmallBatch) {
    QueryScheduler scheduler;
    constexpr int kOps = 80;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kOps; ++i) {
        EXPECT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 100, "perf"), 0u);
    }

    for (int i = 0; i < kOps; ++i) {
        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    EXPECT_LT(elapsed_ms.count(), 1000);

    scheduler.shutdown();
}

TEST(ExecutionPerformance, HighPriorityLatencyStaysWithinBound) {
    QueryScheduler scheduler;
    const auto start = std::chrono::steady_clock::now();

    const auto id = scheduler.enqueue([] {}, SLAPriority::HIGH, 20, "latency");
    ASSERT_NE(id, 0u);

    QueryEntry entry;
    ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    EXPECT_LT(elapsed_ms.count(), 500);

    scheduler.shutdown();
}

TEST(ExecutionPerformance, QueueStaysStableDuringBurst) {
    QueryScheduler scheduler;
    constexpr int kBurst = 200;

    for (int i = 0; i < kBurst; ++i) {
        EXPECT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 250, "burst"), 0u);
    }

    EXPECT_EQ(scheduler.size(), static_cast<std::size_t>(kBurst));

    QueryEntry entry;
    for (int i = 0; i < kBurst; ++i) {
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    }

    EXPECT_EQ(scheduler.size(), 0u);
    scheduler.shutdown();
}

TEST(ExecutionPerformance, CompletionMetricsRemainStableAcrossLoops) {
    QueryScheduler scheduler;

    for (int i = 0; i < 40; ++i) {
        const auto id = scheduler.enqueue([] {}, SLAPriority::HIGH, 500, "loop");
        ASSERT_NE(id, 0u);

        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
        scheduler.reportCompletion(id);
    }

    const auto metrics = scheduler.metrics();
    EXPECT_EQ(metrics.total_enqueued, 40u);
    EXPECT_EQ(metrics.total_dequeued, 40u);
    EXPECT_EQ(metrics.completed_total, 40u);
    EXPECT_GE(metrics.sla_compliance_pct, 95.0);

    scheduler.shutdown();
}

TEST(ExecutionPerformance, ManySmallOperationsDoNotCorruptMetrics) {
    QueryScheduler scheduler;

    for (int i = 0; i < 120; ++i) {
        const auto id = scheduler.enqueue([] {}, SLAPriority::MEDIUM, 150, "small");
        ASSERT_NE(id, 0u);
        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
        scheduler.reportCompletion(id);
    }

    const auto metrics = scheduler.metrics();
    EXPECT_EQ(metrics.total_enqueued, 120u);
    EXPECT_EQ(metrics.total_dequeued, 120u);
    EXPECT_EQ(metrics.completed_total, 120u);
    EXPECT_GT(metrics.avg_enqueue_us, 0.0);
    EXPECT_GT(metrics.avg_dequeue_us, 0.0);

    scheduler.shutdown();
}

TEST(ExecutionPerformance, RepeatedLifecycleKeepsQueueBounded) {
    QueryScheduler scheduler;
    constexpr int kRounds = 25;

    for (int round = 0; round < kRounds; ++round) {
        for (int i = 0; i < 10; ++i) {
            ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::LOW, 300, "life"), 0u);
        }
        for (int i = 0; i < 10; ++i) {
            QueryEntry entry;
            ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
        }
        EXPECT_EQ(scheduler.size(), 0u);
    }

    scheduler.shutdown();
}

TEST(ExecutionPerformance, ConcurrentProducersStillDrainAllWork) {
    QueryScheduler scheduler;
    constexpr int kProducers = 4;
    constexpr int kItemsPerProducer = 30;
    std::atomic<int> completed{0};

    std::vector<std::thread> producers;
    producers.reserve(kProducers);

    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&] {
            for (int i = 0; i < kItemsPerProducer; ++i) {
                const auto id = scheduler.enqueue([&completed] { completed.fetch_add(1, std::memory_order_relaxed); },
                                                  SLAPriority::MEDIUM,
                                                  200,
                                                  "parallel-perf");
                EXPECT_NE(id, 0u);
            }
        });
    }

    for (auto& producer : producers) {
        producer.join();
    }

    for (int i = 0; i < kProducers * kItemsPerProducer; ++i) {
        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(250)));
        entry.execute();
    }

    EXPECT_EQ(completed.load(std::memory_order_relaxed), kProducers * kItemsPerProducer);
    scheduler.shutdown();
}

}  // namespace themis::execution
