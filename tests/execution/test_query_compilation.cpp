#include <gtest/gtest.h>

#include "execution/query_scheduler.h"

#include <chrono>
#include <utility>
#include <vector>

namespace themis::execution {

TEST(QueryCompilation, EnqueueCreatesUniqueIds) {
    QueryScheduler scheduler;

    const auto id1 = scheduler.enqueue([] {}, SLAPriority::HIGH, 25, "compile-q1");
    const auto id2 = scheduler.enqueue([] {}, SLAPriority::MEDIUM, 50, "compile-q2");

    ASSERT_NE(id1, 0u);
    ASSERT_NE(id2, 0u);
    EXPECT_NE(id1, id2);
    EXPECT_EQ(scheduler.size(), 2u);

    scheduler.shutdown();
}

TEST(QueryCompilation, DequeueRespectsEarliestDeadlineFirst) {
    QueryScheduler scheduler;

    ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::LOW, 250, "late"), 0u);
    ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::HIGH, 10, "early"), 0u);
    ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 100, "mid"), 0u);

    QueryEntry first;
    ASSERT_TRUE(scheduler.dequeue(first, std::chrono::milliseconds(200)));
    EXPECT_EQ(first.name, "early");

    QueryEntry second;
    ASSERT_TRUE(scheduler.dequeue(second, std::chrono::milliseconds(200)));
    EXPECT_EQ(second.name, "mid");

    scheduler.shutdown();
}

TEST(QueryCompilation, SameDeadlineUsesStableOrdering) {
    QueryScheduler scheduler;

    ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 75, "first"), 0u);
    ASSERT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 75, "second"), 0u);

    QueryEntry first;
    ASSERT_TRUE(scheduler.dequeue(first, std::chrono::milliseconds(200)));
    EXPECT_EQ(first.name, "first");

    QueryEntry second;
    ASSERT_TRUE(scheduler.dequeue(second, std::chrono::milliseconds(200)));
    EXPECT_EQ(second.name, "second");

    scheduler.shutdown();
}

TEST(QueryCompilation, ShutdownRejectsNewWork) {
    QueryScheduler scheduler;
    scheduler.shutdown();

    EXPECT_EQ(scheduler.enqueue([] {}, SLAPriority::HIGH, 10, "after-shutdown"), 0u);
    EXPECT_TRUE(scheduler.is_shutdown());
    EXPECT_EQ(scheduler.size(), 0u);
}

TEST(QueryCompilation, EmptyQueueTimesOutCleanly) {
    QueryScheduler scheduler;

    QueryEntry out;
    EXPECT_FALSE(scheduler.dequeue(out, std::chrono::milliseconds(25)));
    EXPECT_EQ(out.id, 0u);
    EXPECT_EQ(scheduler.size(), 0u);

    scheduler.shutdown();
}

TEST(QueryCompilation, MetricsTrackSuccessfulEnqueueAndCompletion) {
    QueryScheduler scheduler;

    const auto id = scheduler.enqueue([] {}, SLAPriority::HIGH, 100, "tracked");
    ASSERT_NE(id, 0u);

    QueryEntry out;
    ASSERT_TRUE(scheduler.dequeue(out, std::chrono::milliseconds(200)));
    EXPECT_EQ(out.id, id);

    scheduler.reportCompletion(id);
    const auto metrics = scheduler.metrics();

    EXPECT_EQ(metrics.total_enqueued, 1u);
    EXPECT_EQ(metrics.total_dequeued, 1u);
    EXPECT_EQ(metrics.completed_total, 1u);
    EXPECT_EQ(metrics.completed_in_sla, 1u);
    EXPECT_NEAR(metrics.sla_compliance_pct, 100.0, 0.1);

    scheduler.shutdown();
}

TEST(QueryCompilation, LowPriorityLoadshedAtThreshold) {
    QueryScheduler::Config cfg;
    cfg.max_queue_depth = 10;
    cfg.shed_threshold = 3;
    QueryScheduler scheduler(cfg);

    for (int i = 0; i < 3; ++i) {
        EXPECT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 500, "filler"), 0u);
    }

    EXPECT_EQ(scheduler.enqueue([] {}, SLAPriority::LOW, 500, "shed-me"), 0u);
    EXPECT_GE(scheduler.metrics().total_shed, 1u);

    scheduler.shutdown();
}

TEST(QueryCompilation, QueueDepthReflectsCurrentWorkload) {
    QueryScheduler scheduler;

    EXPECT_EQ(scheduler.size(), 0u);
    EXPECT_NE(scheduler.enqueue([] {}, SLAPriority::MEDIUM, 40, "a"), 0u);
    EXPECT_NE(scheduler.enqueue([] {}, SLAPriority::HIGH, 50, "b"), 0u);
    EXPECT_EQ(scheduler.size(), 2u);

    QueryEntry out;
    ASSERT_TRUE(scheduler.dequeue(out, std::chrono::milliseconds(200)));
    EXPECT_EQ(scheduler.size(), 1u);

    scheduler.shutdown();
}

}  // namespace themis::execution
