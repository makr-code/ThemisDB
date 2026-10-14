#include <gtest/gtest.h>

#include "execution/query_scheduler.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace themis::execution {

TEST(ExecutionEngine, ExecutesDequeuedCallback) {
    QueryScheduler scheduler;
    std::atomic<int> value{0};

    const auto id = scheduler.enqueue([&value] { value.fetch_add(5, std::memory_order_relaxed); },
                                      SLAPriority::HIGH,
                                      50,
                                      "exec-callback");
    ASSERT_NE(id, 0u);

    QueryEntry entry;
    ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    entry.execute();

    EXPECT_EQ(value.load(std::memory_order_relaxed), 5);
    scheduler.shutdown();
}

TEST(ExecutionEngine, MixedPriorityExecutionRespectsDeadlineOrder) {
    QueryScheduler scheduler;
    std::vector<std::string> order;

    ASSERT_NE(scheduler.enqueue([&order] { order.emplace_back("late"); }, SLAPriority::LOW, 250, "late"), 0u);
    ASSERT_NE(scheduler.enqueue([&order] { order.emplace_back("early"); }, SLAPriority::HIGH, 10, "early"), 0u);
    ASSERT_NE(scheduler.enqueue([&order] { order.emplace_back("mid"); }, SLAPriority::MEDIUM, 100, "mid"), 0u);

    for (int i = 0; i < 3; ++i) {
        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
        entry.execute();
    }

    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "early");
    EXPECT_EQ(order[1], "mid");
    EXPECT_EQ(order[2], "late");

    scheduler.shutdown();
}

TEST(ExecutionEngine, CompletionBeforeDeadlineCountsInSla) {
    QueryScheduler scheduler;

    const auto id = scheduler.enqueue([] {}, SLAPriority::HIGH, 200, "sla-ok");
    ASSERT_NE(id, 0u);

    QueryEntry entry;
    ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    scheduler.reportCompletion(id, std::chrono::steady_clock::now() + std::chrono::milliseconds(10));

    const auto metrics = scheduler.metrics();
    EXPECT_EQ(metrics.completed_total, 1u);
    EXPECT_EQ(metrics.completed_in_sla, 1u);
    EXPECT_NEAR(metrics.sla_compliance_pct, 100.0, 0.1);

    scheduler.shutdown();
}

TEST(ExecutionEngine, CompletionAfterDeadlineCountsAsMiss) {
    QueryScheduler scheduler;

    const auto id = scheduler.enqueue([] {}, SLAPriority::MEDIUM, 5, "sla-miss");
    ASSERT_NE(id, 0u);

    QueryEntry entry;
    ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    scheduler.reportCompletion(id, std::chrono::steady_clock::now() + std::chrono::milliseconds(100));

    const auto metrics = scheduler.metrics();
    EXPECT_EQ(metrics.completed_total, 1u);
    EXPECT_EQ(metrics.completed_in_sla, 0u);
    EXPECT_NEAR(metrics.sla_compliance_pct, 0.0, 0.1);

    scheduler.shutdown();
}

TEST(ExecutionEngine, UnknownCompletionIdIsIgnored) {
    QueryScheduler scheduler;
    scheduler.reportCompletion(424242u);

    const auto metrics = scheduler.metrics();
    EXPECT_EQ(metrics.completed_total, 1u);
    EXPECT_EQ(metrics.completed_in_sla, 0u);

    scheduler.shutdown();
}

TEST(ExecutionEngine, ShutdownIsIdempotentAndNonBlocking) {
    QueryScheduler scheduler;
    scheduler.shutdown();
    scheduler.shutdown();

    EXPECT_TRUE(scheduler.is_shutdown());
    EXPECT_EQ(scheduler.size(), 0u);
    EXPECT_EQ(scheduler.enqueue([] {}, SLAPriority::HIGH, 10, "still-blocked"), 0u);
}

TEST(ExecutionEngine, DequeueWaitsUntilTimeoutWhenQueueEmpty) {
    QueryScheduler scheduler;

    QueryEntry entry;
    const auto started = std::chrono::steady_clock::now();
    EXPECT_FALSE(scheduler.dequeue(entry, std::chrono::milliseconds(25)));
    const auto elapsed = std::chrono::steady_clock::now() - started;

    EXPECT_GE(elapsed, std::chrono::milliseconds(20));
    EXPECT_EQ(entry.id, 0u);

    scheduler.shutdown();
}

TEST(ExecutionEngine, ConcurrentEnqueueDequeueProducesConsistentCounts) {
    QueryScheduler scheduler;
    constexpr int kItems = 64;

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};

    std::thread producer([&] {
        for (int i = 0; i < kItems; ++i) {
            const auto id = scheduler.enqueue([&consumed] { consumed.fetch_add(1, std::memory_order_relaxed); },
                                              (i % 3 == 0) ? SLAPriority::HIGH : SLAPriority::MEDIUM,
                                              100,
                                              "parallel");
            if (id != 0u) {
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });

    std::thread consumer([&] {
        for (int i = 0; i < kItems; ++i) {
            QueryEntry entry;
            if (scheduler.dequeue(entry, std::chrono::milliseconds(250))) {
                entry.execute();
            }
        }
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(produced.load(std::memory_order_relaxed), kItems);
    EXPECT_EQ(consumed.load(std::memory_order_relaxed), kItems);

    scheduler.shutdown();
}

TEST(ExecutionEngine, DequeueAfterShutdownUnblocksWaitingConsumers) {
    QueryScheduler scheduler;
    std::atomic<bool> woke{false};

    std::thread waiter([&] {
        QueryEntry entry;
        woke.store(scheduler.dequeue(entry, std::chrono::seconds(1)), std::memory_order_relaxed);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    scheduler.shutdown();
    waiter.join();

    EXPECT_FALSE(woke.load(std::memory_order_relaxed));
}

TEST(ExecutionEngine, MetricsAverageLatencyIsComputed) {
    QueryScheduler scheduler;

    for (int i = 0; i < 5; ++i) {
        const auto id = scheduler.enqueue([] {}, SLAPriority::MEDIUM, 100, "latency");
        ASSERT_NE(id, 0u);

        QueryEntry entry;
        ASSERT_TRUE(scheduler.dequeue(entry, std::chrono::milliseconds(200)));
    }

    const auto metrics = scheduler.metrics();
    EXPECT_GE(metrics.total_enqueued, 5u);
    EXPECT_GE(metrics.total_dequeued, 5u);
    EXPECT_GT(metrics.avg_enqueue_us, 0.0);
    EXPECT_GT(metrics.avg_dequeue_us, 0.0);

    scheduler.shutdown();
}

}  // namespace themis::execution
