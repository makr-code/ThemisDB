/**
 * @file test_search_analytics.cpp
 * @brief Unit tests for SearchAnalytics (v1.5.0)
 */

#include <gtest/gtest.h>
#include "search/search_analytics.h"
#include <string>
#include <thread>
#include <vector>

using namespace themis;

// ============================================================================
// Config validation
// ============================================================================

TEST(SearchAnalyticsConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(SearchAnalytics{});
}

TEST(SearchAnalyticsConfig, ZeroMaxEventsThrows) {
    SearchAnalytics::Config cfg;
    cfg.max_events = 0;
    EXPECT_THROW(SearchAnalytics{cfg}, std::invalid_argument);
}

TEST(SearchAnalyticsConfig, ConfigRoundtrip) {
    SearchAnalytics::Config cfg;
    cfg.max_events = 500;
    SearchAnalytics sa{cfg};
    EXPECT_EQ(sa.getConfig().max_events, 500u);
}

// ============================================================================
// record() basic behaviour
// ============================================================================

TEST(SearchAnalyticsRecord, EventCountIncrements) {
    SearchAnalytics sa;
    EXPECT_EQ(sa.eventCount(), 0u);
    sa.record("query1", 5, 10.0);
    EXPECT_EQ(sa.eventCount(), 1u);
    sa.record("query2", 0, 3.5);
    EXPECT_EQ(sa.eventCount(), 2u);
}

TEST(SearchAnalyticsRecord, ZeroResultFlagged) {
    SearchAnalytics sa;
    sa.record("no results", 0, 5.0);
    auto zero = sa.getZeroResultQueries(10);
    ASSERT_EQ(zero.size(), 1u);
    EXPECT_EQ(zero[0].query, "no results");
    EXPECT_TRUE(zero[0].is_zero_result);
}

TEST(SearchAnalyticsRecord, NonZeroResultNotFlagged) {
    SearchAnalytics sa;
    sa.record("has results", 10, 5.0);
    auto zero = sa.getZeroResultQueries(10);
    EXPECT_TRUE(zero.empty());
}

TEST(SearchAnalyticsRecord, LatencyPreserved) {
    SearchAnalytics sa;
    sa.record("q", 3, 42.7);
    auto events = sa.getRecentEvents(1);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_DOUBLE_EQ(events[0].latency_ms, 42.7);
}

// ============================================================================
// Max events cap
// ============================================================================

TEST(SearchAnalyticsMaxEvents, OldestEvictedWhenFull) {
    SearchAnalytics::Config cfg;
    cfg.max_events = 3;
    SearchAnalytics sa{cfg};
    sa.record("first",  1, 1.0);
    sa.record("second", 2, 2.0);
    sa.record("third",  3, 3.0);
    sa.record("fourth", 4, 4.0); // evicts "first"
    EXPECT_EQ(sa.eventCount(), 3u);
    auto events = sa.getRecentEvents(10);
    // "first" should be gone; "fourth" should be present
    for (const auto& ev : events) {
        EXPECT_NE(ev.query, "first");
    }
    bool found_fourth = false;
    for (const auto& ev : events) {
        if (ev.query == "fourth") { found_fourth = true; break; }
    }
    EXPECT_TRUE(found_fourth);
}

// ============================================================================
// getRecentEvents
// ============================================================================

TEST(SearchAnalyticsRecent, ReturnsUpToLimit) {
    SearchAnalytics sa;
    for (int i = 0; i < 20; ++i) {
        sa.record("q" + std::to_string(i), i, static_cast<double>(i));
    }
    auto events = sa.getRecentEvents(5);
    EXPECT_EQ(events.size(), 5u);
}

TEST(SearchAnalyticsRecent, MostRecentFirst) {
    SearchAnalytics sa;
    sa.record("older", 1, 1.0);
    sa.record("newer", 2, 2.0);
    auto events = sa.getRecentEvents(2);
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[0].query, "newer");
    EXPECT_EQ(events[1].query, "older");
}

TEST(SearchAnalyticsRecent, EmptyReturnsEmpty) {
    SearchAnalytics sa;
    EXPECT_TRUE(sa.getRecentEvents(10).empty());
}

// ============================================================================
// getZeroResultQueries
// ============================================================================

TEST(SearchAnalyticsZeroResult, LimitHonored) {
    SearchAnalytics sa;
    for (int i = 0; i < 10; ++i) {
        sa.record("zero_" + std::to_string(i), 0, 1.0);
    }
    auto zero = sa.getZeroResultQueries(3);
    EXPECT_EQ(zero.size(), 3u);
}

TEST(SearchAnalyticsZeroResult, MixedQueriesOnlyZeroReturned) {
    SearchAnalytics sa;
    sa.record("no",  0, 1.0);
    sa.record("yes", 5, 1.0);
    sa.record("no2", 0, 1.0);
    auto zero = sa.getZeroResultQueries(10);
    EXPECT_EQ(zero.size(), 2u);
    for (const auto& ev : zero) {
        EXPECT_EQ(ev.result_count, 0u);
    }
}

// ============================================================================
// computeMetrics
// ============================================================================

TEST(SearchAnalyticsMetrics, EmptyMetrics) {
    SearchAnalytics sa;
    auto m = sa.computeMetrics();
    EXPECT_EQ(m.total_queries, 0u);
    EXPECT_DOUBLE_EQ(m.avg_latency_ms, 0.0);
}

TEST(SearchAnalyticsMetrics, TotalQueriesCorrect) {
    SearchAnalytics sa;
    sa.record("a", 1, 10.0);
    sa.record("b", 2, 20.0);
    sa.record("c", 0, 5.0);
    auto m = sa.computeMetrics();
    EXPECT_EQ(m.total_queries, 3u);
    EXPECT_EQ(m.zero_result_queries, 1u);
}

TEST(SearchAnalyticsMetrics, AvgLatencyCorrect) {
    SearchAnalytics sa;
    sa.record("a", 1, 10.0);
    sa.record("b", 2, 30.0);
    auto m = sa.computeMetrics();
    EXPECT_DOUBLE_EQ(m.avg_latency_ms, 20.0);
}

TEST(SearchAnalyticsMetrics, ZeroResultRateCorrect) {
    SearchAnalytics sa;
    sa.record("a", 0, 1.0);
    sa.record("b", 5, 1.0);
    auto m = sa.computeMetrics();
    EXPECT_DOUBLE_EQ(m.zero_result_rate, 0.5);
}

TEST(SearchAnalyticsMetrics, TopQueriesTracked) {
    SearchAnalytics sa;
    for (int i = 0; i < 5; ++i) {
      sa.record("popular", 3, 5.0);
    }
    sa.record("rare", 1, 5.0);
    auto m = sa.computeMetrics();
    ASSERT_FALSE(m.top_queries.empty());
    EXPECT_EQ(m.top_queries.at("popular"), 5u);
}

TEST(SearchAnalyticsMetrics, PercentilesInNonDecreasingOrder) {
    SearchAnalytics sa;
    for (int i = 1; i <= 100; ++i) {
        sa.record("q", 1, static_cast<double>(i));
    }
    auto m = sa.computeMetrics();
    EXPECT_LE(m.avg_latency_ms, m.p95_latency_ms);
    EXPECT_LE(m.p95_latency_ms, m.p99_latency_ms);
}

// ============================================================================
// getTopQueries
// ============================================================================

TEST(SearchAnalyticsTopQueries, EmptyReturnsEmpty) {
    SearchAnalytics sa;
    EXPECT_TRUE(sa.getTopQueries(10).empty());
}

TEST(SearchAnalyticsTopQueries, LimitZeroReturnsEmpty) {
    SearchAnalytics sa;
    sa.record("q", 1, 1.0);
    EXPECT_TRUE(sa.getTopQueries(0).empty());
}

TEST(SearchAnalyticsTopQueries, SortedByDescendingFrequency) {
    SearchAnalytics sa;
    for (int i = 0; i < 5; ++i) {
      sa.record("popular", 1, 1.0);
    }
    for (int i = 0; i < 2; ++i) {
      sa.record("medium",  1, 1.0);
    }
    sa.record("rare", 1, 1.0);
    auto top = sa.getTopQueries(10);
    ASSERT_EQ(top.size(), 3u);
    EXPECT_EQ(top[0].first,  "popular");
    EXPECT_EQ(top[0].second, 5u);
    EXPECT_EQ(top[1].first,  "medium");
    EXPECT_EQ(top[1].second, 2u);
    EXPECT_EQ(top[2].first,  "rare");
    EXPECT_EQ(top[2].second, 1u);
}

TEST(SearchAnalyticsTopQueries, LimitHonored) {
    SearchAnalytics sa;
    // Record 10 distinct queries each once; alphabetical tiebreak means q0 < q1 < ... < q9
    for (int i = 0; i < 10; ++i) {
      sa.record("q" + std::to_string(i), 1, 1.0);
    }
    auto top = sa.getTopQueries(3);
    ASSERT_EQ(top.size(), 3u);
    // All have equal frequency so alphabetical tiebreak applies: q0, q1, q2
    EXPECT_EQ(top[0].first, "q0");
    EXPECT_EQ(top[1].first, "q1");
    EXPECT_EQ(top[2].first, "q2");
}

TEST(SearchAnalyticsTopQueries, LimitLargerThanDistinctQueries) {
    SearchAnalytics sa;
    sa.record("a", 1, 1.0);
    sa.record("b", 1, 1.0);
    auto top = sa.getTopQueries(100);
    EXPECT_EQ(top.size(), 2u);
}

TEST(SearchAnalyticsTopQueries, CountsAreAccurate) {
    SearchAnalytics sa;
    for (int i = 0; i < 7; ++i) {
      sa.record("x", 1, 1.0);
    }
    auto top = sa.getTopQueries(1);
    ASSERT_EQ(top.size(), 1u);
    EXPECT_EQ(top[0].first,  "x");
    EXPECT_EQ(top[0].second, 7u);
}

// ============================================================================
// clear()
// ============================================================================

TEST(SearchAnalyticsClear, ClearResetsCount) {
    SearchAnalytics sa;
    sa.record("q", 1, 1.0);
    sa.record("q", 2, 2.0);
    sa.clear();
    EXPECT_EQ(sa.eventCount(), 0u);
    EXPECT_TRUE(sa.getRecentEvents(10).empty());
}

// ============================================================================
// Thread safety smoke test
// ============================================================================

TEST(SearchAnalyticsThreadSafety, ConcurrentRecordDoesNotCrash) {
    SearchAnalytics sa;
    constexpr int kThreads = 4;
    constexpr int kEventsPerThread = 50;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&sa, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                sa.record("t" + std::to_string(t) + "_q" + std::to_string(i),
                          i % 3, static_cast<double>(i));
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // No crash; event count is within the allowed range
    EXPECT_LE(sa.eventCount(), sa.getConfig().max_events);
}
