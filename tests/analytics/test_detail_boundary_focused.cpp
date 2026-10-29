#include <gtest/gtest.h>

#include "analytics/detail/ring_buffer.h"
#include "analytics/detail/stats.h"

#include <memory>
#include <span>
#include <vector>

namespace {

using themis::analytics::detail::computePercentile;
using themis::analytics::detail::EventRingBuffer;

TEST(AnalyticsRingBoundaryTest, RoundsCapacityUpAndRetainsFifoAcrossWraparound) {
    EventRingBuffer<int> queue(3);
    ASSERT_EQ(queue.capacity(), 4u);

    for (int cycle = 0; cycle < 20; ++cycle) {
        SCOPED_TRACE(cycle);
        for (int i = 0; i < 4; ++i) {
            ASSERT_TRUE(queue.push(cycle * 4 + i));
        }
        EXPECT_FALSE(queue.push(-1));
        EXPECT_EQ(queue.size_approx(), 4u);
        for (int i = 0; i < 4; ++i) {
            int value = -1;
            ASSERT_TRUE(queue.pop(value));
            EXPECT_EQ(value, cycle * 4 + i);
        }
        int unchanged = 99;
        EXPECT_FALSE(queue.pop(unchanged));
        EXPECT_EQ(unchanged, 99);
        EXPECT_TRUE(queue.empty());
    }
}

TEST(AnalyticsRingBoundaryTest, MinimumCapacitySupportsMoveOnlyOwnership) {
    EventRingBuffer<std::unique_ptr<int>> queue(0);
    ASSERT_EQ(queue.capacity(), 2u);
    auto item = std::make_unique<int>(42);
    ASSERT_TRUE(queue.push(std::move(item)));
    EXPECT_EQ(item, nullptr);
    std::unique_ptr<int> result;
    ASSERT_TRUE(queue.pop(result));
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(*result, 42);
    EXPECT_FALSE(queue.pop(result));
    EXPECT_EQ(*result, 42);
}

TEST(AnalyticsPercentileBoundaryTest, InterpolatesBothOverloadsWithoutMutatingInput) {
    const std::vector<double> original{30.0, -10.0, 10.0, 0.0};
    auto values = original;
    const auto view = std::span<const double>(values);
    EXPECT_DOUBLE_EQ(computePercentile(values, 25.0), -2.5);
    EXPECT_DOUBLE_EQ(computePercentile(view, 25.0), -2.5);
    EXPECT_DOUBLE_EQ(computePercentile(values, 50.0), 5.0);
    EXPECT_DOUBLE_EQ(computePercentile(view, 50.0), 5.0);
    EXPECT_EQ(values, original);
}

TEST(AnalyticsPercentileBoundaryTest, EmptySingletonAndClampedEndpointsAgree) {
    for (const auto& values :
         {std::vector<double>{}, std::vector<double>{7.0},
          std::vector<double>{9.0, -3.0}}) {
        const auto view = std::span<const double>(values);
        const double lower = values.empty() ? 0.0 : values.back();
        const double upper = values.empty() ? 0.0 : values.front();
        EXPECT_DOUBLE_EQ(computePercentile(values, -1.0), lower);
        EXPECT_DOUBLE_EQ(computePercentile(view, -1.0), lower);
        EXPECT_DOUBLE_EQ(computePercentile(values, 101.0), upper);
        EXPECT_DOUBLE_EQ(computePercentile(view, 101.0), upper);
    }
}

} // namespace
