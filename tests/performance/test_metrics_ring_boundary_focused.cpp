#include <gtest/gtest.h>

#include "performance/lockfree_metrics_buffer.h"

#include <memory>

namespace {

using themis::performance::LockFreeRingBuffer;

TEST(MetricsRingBoundaryTest, ReusesWrappedSlotsAndCountsOnlyRejectedPushes) {
    LockFreeRingBuffer<int, 4> queue;
    for (int cycle = 0; cycle < 20; ++cycle) {
        SCOPED_TRACE(cycle);
        for (int i = 0; i < 3; ++i) {
            ASSERT_TRUE(queue.tryPush(cycle * 3 + i));
        }
        EXPECT_EQ(queue.size(), 3u);
        EXPECT_FALSE(queue.tryPush(-1));
        EXPECT_EQ(queue.dropped_count(), static_cast<unsigned>(cycle + 1));
        for (int i = 0; i < 3; ++i) {
            int value = -1;
            ASSERT_TRUE(queue.tryPop(value));
            EXPECT_EQ(value, cycle * 3 + i);
        }
        int unchanged = 42;
        EXPECT_FALSE(queue.tryPop(unchanged));
        EXPECT_EQ(unchanged, 42);
        EXPECT_TRUE(queue.empty());
    }
    queue.reset_dropped_count();
    EXPECT_EQ(queue.dropped_count(), 0u);
}

TEST(MetricsRingBoundaryTest, RejectedMoveDoesNotConsumeOwnershipOrOverwriteQueuedItem) {
    LockFreeRingBuffer<std::unique_ptr<int>, 2> queue;
    ASSERT_TRUE(queue.tryPush(std::make_unique<int>(1)));
    auto rejected = std::make_unique<int>(2);
    EXPECT_FALSE(queue.tryPush(std::move(rejected)));
    ASSERT_NE(rejected, nullptr);
    EXPECT_EQ(*rejected, 2);

    std::unique_ptr<int> result;
    ASSERT_TRUE(queue.tryPop(result));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 1);
    ASSERT_TRUE(queue.tryPush(std::move(rejected)));
    EXPECT_EQ(rejected, nullptr);
    ASSERT_TRUE(queue.tryPop(result));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 2);
    EXPECT_EQ(queue.dropped_count(), 1u);
}

} // namespace
