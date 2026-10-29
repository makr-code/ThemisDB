#include <gtest/gtest.h>

#include "ingestion/bounded_queue.h"

#include <chrono>

namespace {

using namespace themis::ingestion;

TEST(IngestionQueueBoundaryTest, ExactMemoryLimitRejectsNextItemWithoutChangingContents) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 10;
    config.max_memory_bytes = 2 * sizeof(int);
    BoundedQueue<int> queue(config);
    ASSERT_TRUE(queue.enqueue(11, false).success);
    ASSERT_TRUE(queue.enqueue(22, false).success);
    EXPECT_EQ(queue.currentMemoryBytes(), config.max_memory_bytes);

    const auto rejected = queue.enqueue(33, false);
    EXPECT_FALSE(rejected.success);
    EXPECT_TRUE(rejected.is_memory_limit);
    EXPECT_FALSE(rejected.is_saturation);
    EXPECT_FALSE(rejected.error_message.empty());
    EXPECT_EQ(queue.size(), 2u);
    EXPECT_EQ(queue.tryDequeue(), 11);
    EXPECT_EQ(queue.tryDequeue(), 22);
    EXPECT_EQ(queue.currentMemoryBytes(), 0u);
    EXPECT_EQ(queue.getState(), QueueSaturationState::NORMAL);
    ASSERT_TRUE(queue.enqueue(44, false).success);
    EXPECT_EQ(queue.tryDequeue(), 44);
    const auto stats = queue.getStats();
    EXPECT_EQ(stats.enqueue_attempts, 4u);
    EXPECT_EQ(stats.enqueue_successes, 3u);
    EXPECT_EQ(stats.enqueue_memory_blocks, 1u);
    EXPECT_EQ(stats.dequeue_count, 3u);
}

TEST(IngestionQueueBoundaryTest, ClearResetsOccupancyButPreservesHistoricalPeaks) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 2;
    config.max_memory_bytes = 0;
    BoundedQueue<int> queue(config);
    ASSERT_TRUE(queue.enqueue(1, false).success);
    ASSERT_TRUE(queue.enqueue(2, false).success);
    ASSERT_FALSE(queue.enqueue(3, false).success);

    queue.clear();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.currentMemoryBytes(), 0u);
    EXPECT_EQ(queue.getState(), QueueSaturationState::NORMAL);
    const auto stats = queue.getStats();
    EXPECT_EQ(stats.current_item_count, 0u);
    EXPECT_EQ(stats.current_memory_bytes, 0u);
    EXPECT_EQ(stats.max_item_count_observed, 2u);
    EXPECT_EQ(stats.max_memory_observed, 2 * sizeof(int));
    EXPECT_EQ(stats.enqueue_saturation_blocks, 1u);
    ASSERT_TRUE(queue.enqueue(4, false).success);
    EXPECT_EQ(queue.tryDequeue(), 4);
}

TEST(IngestionQueueBoundaryTest, ZeroTimeoutDoesNotEnqueueIntoFullQueue) {
    QueueResourceLimitConfig config;
    config.max_queue_size = 1;
    config.max_memory_bytes = 0;
    config.blocking_enqueue_timeout_ms = std::chrono::milliseconds(0);
    BoundedQueue<int> queue(config);
    ASSERT_TRUE(queue.enqueue(1, false).success);
    const auto rejected = queue.enqueue(2, true);
    EXPECT_FALSE(rejected.success);
    EXPECT_TRUE(rejected.is_timeout);
    EXPECT_FALSE(rejected.error_message.empty());
    EXPECT_EQ(queue.tryDequeue(), 1);
    EXPECT_FALSE(queue.tryDequeue().has_value());
}

} // namespace
