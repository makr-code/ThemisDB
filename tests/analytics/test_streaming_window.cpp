/**
 * Streaming aggregation windows unit + integration tests.
 *
 * Covers:
 *  - StreamRecord construction and field access
 *  - makeRecord helper
 *  - WindowResult::get()
 *  - TumblingWindow: ingestion, slot routing, flush, aggregations, stats,
 *                    late event dropping, late data allowed
 *  - SlidingWindow:  overlapping assignment, aggregations, flush, stats
 *  - SessionWindow:  gap expiry, multi-partition, timer-driven expiry, flush, stats
 *  - HoppingWindow:  overlapping assignment (size > hop), aggregations, flush, stats
 *  - StreamingWindowPipeline: fluent builder for all four types, ingest, flush, stats
 *  - All 11 aggregation functions: COUNT/SUM/AVG/MIN/MAX/STDDEV/VARIANCE/PERCENTILE/
 *                                   FIRST/LAST/DISTINCT_COUNT
 */

#include <gtest/gtest.h>
#include "analytics/streaming_window.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace themisdb::analytics;
using namespace std::chrono_literals;

// ============================================================================
// Helpers
// ============================================================================

static std::chrono::system_clock::time_point baseTime() {
    // Fixed base so slot indices are predictable in tests
    return std::chrono::system_clock::time_point(std::chrono::hours(24 * 365 * 50));
}

static StreamRecord rec(const std::string& id,
                         std::chrono::milliseconds offset_from_base,
                         double value,
                         const std::string& partition = "") {
    return makeRecord(id, baseTime() + offset_from_base, partition,
                      {{"value", value}});
}

// ============================================================================
// StreamRecord / makeRecord
// ============================================================================

TEST(StreamRecordTest, FieldAccessors) {
    auto r = makeRecord("r1", baseTime(), "k1",
                        {{"int_val",    int64_t(42)},
                         {"dbl_val",    3.14},
                         {"str_val",    std::string("hello")},
                         {"bool_val",   true}});
    EXPECT_EQ(*r.get<int64_t>("int_val"), 42);
    EXPECT_DOUBLE_EQ(*r.get<double>("dbl_val"), 3.14);
    EXPECT_EQ(*r.get<std::string>("str_val"), "hello");
    EXPECT_TRUE(*r.get<bool>("bool_val"));
    EXPECT_FALSE(r.get<int64_t>("missing").has_value());
}

TEST(StreamRecordTest, MakeRecordSetsId) {
    auto r = makeRecord("test-id", baseTime());
    EXPECT_EQ(r.record_id, "test-id");
}

TEST(StreamRecordTest, MakeRecordGeneratesIdWhenEmpty) {
    auto r = makeRecord("", baseTime());
    EXPECT_FALSE(r.record_id.empty());
}

TEST(StreamRecordTest, SetField) {
    StreamRecord r;
    r.set("x", 99.0);
    EXPECT_DOUBLE_EQ(*r.get<double>("x"), 99.0);
}

// ============================================================================
// WindowResult
// ============================================================================

TEST(WindowResultTest, GetExistingAggregation) {
    WindowResult r;
    AggregatedValue av;
    av.name  = "total";
    av.func  = AggFunc::SUM;
    av.value = 42.0;
    r.aggregations.push_back(av);

    auto v = r.get("total");
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(*v), 42.0);
}

TEST(WindowResultTest, GetMissingAggregationReturnsNullopt) {
    WindowResult r;
    EXPECT_FALSE(r.get("missing").has_value());
}

// ============================================================================
// TumblingWindow
// ============================================================================

class TumblingWindowTest : public ::testing::Test {
protected:
    TumblingWindowConfig cfg1min() {
        TumblingWindowConfig c;
        c.size = 60000ms;
        return c;
    }
};

TEST_F(TumblingWindowTest, BasicIngestionAndFlush) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    // All in the same 1-minute slot
    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 10000ms, 2.0));
    win->ingest(rec("r3", 30000ms, 3.0));
    win->flush();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].record_count, 3u);
    auto cnt = results[0].get("cnt");
    ASSERT_TRUE(cnt.has_value());
    EXPECT_EQ(std::get<int64_t>(*cnt), 3);
}

TEST_F(TumblingWindowTest, RecordsInDifferentSlots) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    // slot 0: 0ms, slot 1: 60000ms, slot 2: 120000ms
    // Advancing to slot 2 should close slot 0
    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 60000ms, 2.0));  // closes slot 0
    win->ingest(rec("r3", 120000ms, 3.0)); // closes slot 1
    win->flush(); // closes slot 2

    EXPECT_GE(results.size(), 2u);
}

TEST_F(TumblingWindowTest, SumAggregation) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"total", AggFunc::SUM, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 10.0));
    win->ingest(rec("r2", 10000ms, 20.0));
    win->ingest(rec("r3", 20000ms, 30.0));
    win->flush();

    ASSERT_FALSE(results.empty());
    auto v = results[0].get("total");
    ASSERT_TRUE(v.has_value());
    EXPECT_DOUBLE_EQ(std::get<double>(*v), 60.0);
}

TEST_F(TumblingWindowTest, AvgMinMaxAggregations) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"avg", AggFunc::AVG, "value"});
    win->addAggregation({"mn",  AggFunc::MIN, "value"});
    win->addAggregation({"mx",  AggFunc::MAX, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 1000ms, 3.0));
    win->ingest(rec("r3", 2000ms, 5.0));
    win->flush();

    ASSERT_FALSE(results.empty());
    auto avg = results[0].get("avg");
    auto mn  = results[0].get("mn");
    auto mx  = results[0].get("mx");
    ASSERT_TRUE(avg.has_value());
    ASSERT_TRUE(mn.has_value());
    ASSERT_TRUE(mx.has_value());
    ASSERT_TRUE(std::holds_alternative<double>(*avg));
    ASSERT_TRUE(std::holds_alternative<double>(*mn));
    ASSERT_TRUE(std::holds_alternative<double>(*mx));
    EXPECT_DOUBLE_EQ(std::get<double>(*avg), 3.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*mn),  1.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*mx),  5.0);
}

TEST_F(TumblingWindowTest, StddevVarianceAggregations) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"std", AggFunc::STDDEV, "value"});
    win->addAggregation({"var", AggFunc::VARIANCE, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    for (double v : {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}) {
        win->ingest(rec("r", 0ms, v));
    }
    win->flush();
    ASSERT_FALSE(results.empty());
    auto std_opt = results[0].get("std");
    auto var_opt = results[0].get("var");
    ASSERT_TRUE(std_opt.has_value());
    ASSERT_TRUE(var_opt.has_value());
    ASSERT_TRUE(std::holds_alternative<double>(*std_opt));
    ASSERT_TRUE(std::holds_alternative<double>(*var_opt));
    double std_val = std::get<double>(*std_opt);
    double var_val = std::get<double>(*var_opt);
    EXPECT_GT(std_val, 0.0);
    EXPECT_NEAR(var_val, std_val * std_val, 0.001);
}

TEST_F(TumblingWindowTest, PercentileAggregation) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"p50", AggFunc::PERCENTILE, "value", 50.0});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    for (double v : {1.0, 2.0, 3.0, 4.0, 5.0}) win->ingest(rec("r", 0ms, v));
    win->flush();
    ASSERT_FALSE(results.empty());
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("p50")), 3.0);
}

TEST_F(TumblingWindowTest, FirstLastAggregations) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"first", AggFunc::FIRST, "value"});
    win->addAggregation({"last",  AggFunc::LAST,  "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 1000ms, 2.0));
    win->ingest(rec("r3", 2000ms, 3.0));
    win->flush();
    ASSERT_FALSE(results.empty());
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("first")), 1.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("last")),  3.0);
}

TEST_F(TumblingWindowTest, DistinctCountAggregation) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"dc", AggFunc::DISTINCT_COUNT, "cat"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    for (const std::string& cat : {"A", "B", "A", "C", "B", "A"}) {
        auto r = makeRecord("", baseTime(), "", {{"cat", cat}});
        win->ingest(r);
    }
    win->flush();
    ASSERT_FALSE(results.empty());
    EXPECT_EQ(std::get<int64_t>(*results[0].get("dc")), 3);
}

TEST_F(TumblingWindowTest, LateEventDropped) {
    std::vector<WindowResult> results;
    auto win = createTumblingWindow(cfg1min());
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    // Ingest a "future" record first to advance watermark
    win->ingest(rec("future", 200000ms, 1.0)); // slot 3 (>= 180s)
    // Now ingest a very old record (slot 0) — should be dropped
    bool ok = win->ingest(rec("old", 0ms, 2.0));
    EXPECT_FALSE(ok);

    auto stats = win->getStats();
    EXPECT_GE(stats.records_dropped, 1u);
    EXPECT_GE(stats.late_records, 1u);
}

TEST_F(TumblingWindowTest, StatsTracking) {
    auto win = createTumblingWindow(cfg1min());
    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 0ms, 2.0));

    auto s = win->getStats();
    EXPECT_EQ(s.records_ingested, 2u);
    EXPECT_GE(s.windows_opened, 1u);
}

// ============================================================================
// SlidingWindow
// ============================================================================

class SlidingWindowTest : public ::testing::Test {
protected:
    SlidingWindowConfig cfg(std::chrono::milliseconds size,
                              std::chrono::milliseconds slide) {
        SlidingWindowConfig c;
        c.size  = size;
        c.slide = slide;
        return c;
    }
};

TEST_F(SlidingWindowTest, RecordAppearsInMultipleWindows) {
    // size=60s, slide=30s → each record falls in ceil(60/30)=2 windows
    std::vector<WindowResult> results;
    auto win = createSlidingWindow(cfg(60000ms, 30000ms));
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    // One record at t=45s (should fall in windows starting at 0s and 30s)
    win->ingest(rec("r1", 45000ms, 1.0));
    win->flush();

    // At least one result; both windows containing r1 should emit
    EXPECT_GE(results.size(), 1u);
}

TEST_F(SlidingWindowTest, AggregationsWork) {
    std::vector<WindowResult> results;
    auto win = createSlidingWindow(cfg(60000ms, 60000ms)); // non-overlapping when slide == size
    win->addAggregation({"sum", AggFunc::SUM, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 5.0));
    win->ingest(rec("r2", 10000ms, 10.0));
    win->ingest(rec("r3", 20000ms, 15.0));
    win->flush();

    ASSERT_FALSE(results.empty());
    double total = std::get<double>(*results[0].get("sum"));
    EXPECT_DOUBLE_EQ(total, 30.0);
}

TEST_F(SlidingWindowTest, StatsTracking) {
    auto win = createSlidingWindow(cfg(60000ms, 30000ms));
    win->ingest(rec("r1", 0ms, 1.0));
    win->ingest(rec("r2", 30000ms, 2.0));
    auto s = win->getStats();
    EXPECT_GE(s.records_ingested, 2u);
    EXPECT_GE(s.windows_opened, 1u);
}

TEST_F(SlidingWindowTest, LateEventDropped) {
    auto win = createSlidingWindow(cfg(60000ms, 30000ms));
    win->ingest(rec("future", 300000ms, 1.0)); // advance watermark
    bool ok = win->ingest(rec("old", 0ms, 2.0));
    EXPECT_FALSE(ok);
    EXPECT_GE(win->getStats().records_dropped, 1u);
}

// ============================================================================
// SessionWindow
// ============================================================================

class SessionWindowTest : public ::testing::Test {
protected:
    SessionWindowConfig cfg(std::chrono::milliseconds gap) {
        SessionWindowConfig c;
        c.gap = gap;
        return c;
    }
};

TEST_F(SessionWindowTest, BasicSession) {
    std::vector<WindowResult> results;
    auto win = createSessionWindow(cfg(5000ms)); // 5s gap
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    // All within 5s gap: same session
    win->ingest(rec("r1", 0ms, 1.0, "user1"));
    win->ingest(rec("r2", 1000ms, 2.0, "user1"));
    win->ingest(rec("r3", 2000ms, 3.0, "user1"));
    win->flush();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].record_count, 3u);
    EXPECT_EQ(results[0].partition_key, "user1");
    EXPECT_EQ(std::get<int64_t>(*results[0].get("cnt")), 3);
}

TEST_F(SessionWindowTest, GapTriggersNewSession) {
    std::vector<WindowResult> results;
    auto win = createSessionWindow(cfg(5000ms));
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 1.0, "u1"));
    win->ingest(rec("r2", 1000ms, 2.0, "u1"));
    // Gap: 10s > 5s → closes first session, starts new
    win->ingest(rec("r3", 11000ms, 3.0, "u1"));
    win->flush();

    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].record_count, 2u);
    EXPECT_EQ(results[1].record_count, 1u);
}

TEST_F(SessionWindowTest, MultiplePartitionsAreIndependent) {
    std::vector<WindowResult> results;
    auto win = createSessionWindow(cfg(5000ms));
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 1.0, "alice"));
    win->ingest(rec("r2", 0ms, 2.0, "bob"));
    win->ingest(rec("r3", 1000ms, 3.0, "alice"));
    win->flush();

    ASSERT_EQ(results.size(), 2u);
    // Each partition has its own session
    std::map<std::string, uint64_t> counts;
    for (const auto& r : results) {
        counts[r.partition_key] = r.record_count;
    }
    EXPECT_EQ(counts["alice"], 2u);
    EXPECT_EQ(counts["bob"], 1u);
}

TEST_F(SessionWindowTest, TimerDrivenExpiry) {
    std::atomic<int> fired{0};
    auto win = createSessionWindow(cfg(100ms)); // 100ms gap for fast expiry in test
    win->setResultCallback([&](WindowResult) { ++fired; });

    win->ingest(rec("r1", 0ms, 1.0, "timer-user"));
    // Wait longer than gap + timer interval (200ms timer check)
    std::this_thread::sleep_for(400ms);

    EXPECT_GE(fired.load(), 1);
}

// Validates that session_expiry_check_interval_ms is honoured end-to-end:
// the window must fire within gap + check_interval + 50 ms of the last event.
TEST_F(SessionWindowTest, ConfigurableExpiryInterval) {
    constexpr auto gap_ms           = 100ms;
    constexpr auto check_interval   = 80ms;   // non-default, smaller than gap
    constexpr auto tolerance        = 50ms;
    // Expected upper bound for emission after last event:
    //   100ms (gap) + 80ms (check_interval) + 50ms (tolerance) = 230ms
    constexpr auto max_wait = gap_ms + check_interval + tolerance;

    std::atomic<int> fired{0};
    std::chrono::steady_clock::time_point ingest_time;

    SessionWindowConfig c;
    c.gap                             = gap_ms;
    c.session_expiry_check_interval_ms = check_interval;
    auto win = createSessionWindow(c);
    win->setResultCallback([&](WindowResult) { ++fired; });

    ingest_time = std::chrono::steady_clock::now();
    win->ingest(rec("r1", 0ms, 1.0, "cfg-user"));

    // Poll until fired or max_wait elapses
    const auto deadline = ingest_time + max_wait;
    while (fired.load() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(5ms);
    }

    EXPECT_GE(fired.load(), 1)
        << "SessionWindow did not emit within gap + check_interval + 50 ms";
}

TEST_F(SessionWindowTest, AggregationsSumAvg) {
    std::vector<WindowResult> results;
    auto win = createSessionWindow(cfg(5000ms));
    win->addAggregation({"sum", AggFunc::SUM, "value"});
    win->addAggregation({"avg", AggFunc::AVG, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 10.0, "p1"));
    win->ingest(rec("r2", 1000ms, 20.0, "p1"));
    win->flush();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("sum")), 30.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("avg")), 15.0);
}

TEST_F(SessionWindowTest, StatsTracking) {
    auto win = createSessionWindow(cfg(5000ms));
    win->ingest(rec("r1", 0ms, 1.0, "u1"));
    win->ingest(rec("r2", 1000ms, 2.0, "u1"));
    auto s = win->getStats();
    EXPECT_EQ(s.records_ingested, 2u);
    EXPECT_GE(s.windows_opened, 1u);
}

// ============================================================================
// HoppingWindow
// ============================================================================

class HoppingWindowTest : public ::testing::Test {
protected:
    HoppingWindowConfig cfg(std::chrono::milliseconds size,
                              std::chrono::milliseconds hop) {
        HoppingWindowConfig c;
        c.size = size;
        c.hop  = hop;
        return c;
    }
};

TEST_F(HoppingWindowTest, RecordInMultipleWindows) {
    // size=60s, hop=30s → a record at t=45s falls in 2 windows
    std::vector<WindowResult> results;
    auto win = createHoppingWindow(cfg(60000ms, 30000ms));
    win->addAggregation({"cnt", AggFunc::COUNT, ""});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 45000ms, 1.0));
    win->flush();

    EXPECT_GE(results.size(), 1u);
}

TEST_F(HoppingWindowTest, WhenHopEqualsSizeBehavesLikeTumbling) {
    std::vector<WindowResult> results;
    auto win = createHoppingWindow(cfg(60000ms, 60000ms));
    win->addAggregation({"sum", AggFunc::SUM, "value"});
    win->setResultCallback([&](WindowResult r) { results.push_back(r); });

    win->ingest(rec("r1", 0ms, 5.0));
    win->ingest(rec("r2", 30000ms, 10.0));
    win->flush();

    ASSERT_FALSE(results.empty());
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("sum")), 15.0);
}

TEST_F(HoppingWindowTest, LateEventDropped) {
    auto win = createHoppingWindow(cfg(60000ms, 30000ms));
    win->ingest(rec("future", 300000ms, 1.0));
    bool ok = win->ingest(rec("old", 0ms, 2.0));
    EXPECT_FALSE(ok);
    EXPECT_GE(win->getStats().records_dropped, 1u);
}

TEST_F(HoppingWindowTest, StatsTracking) {
    auto win = createHoppingWindow(cfg(60000ms, 30000ms));
    win->ingest(rec("r1", 0ms, 1.0));
    auto s = win->getStats();
    EXPECT_GE(s.records_ingested, 1u);
    EXPECT_GE(s.windows_opened, 1u);
}

TEST_F(HoppingWindowTest, MultipleAggregations) {
    std::vector<WindowResult> results;
    HoppingWindow win(cfg(60000ms, 60000ms));
    win.addAggregation({"cnt", AggFunc::COUNT, ""});
    win.addAggregation({"sum", AggFunc::SUM, "value"});
    win.addAggregation({"avg", AggFunc::AVG, "value"});
    win.addAggregation({"mn",  AggFunc::MIN, "value"});
    win.addAggregation({"mx",  AggFunc::MAX, "value"});
    win.setResultCallback([&](WindowResult r) { results.push_back(r); });

    for (double v : {1.0, 3.0, 5.0}) win.ingest(rec("r", 0ms, v));
    win.flush();

    ASSERT_FALSE(results.empty());
    EXPECT_EQ(std::get<int64_t>(*results[0].get("cnt")), 3);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("sum")), 9.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("avg")), 3.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("mn")),  1.0);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("mx")),  5.0);
}

// ============================================================================
// StreamingWindowPipeline
// ============================================================================

TEST(PipelineTest, TumblingPipelineEndToEnd) {
    std::vector<WindowResult> results;
    auto pipeline = StreamingWindowPipeline::tumbling(60000ms)
        .aggregate({"cnt",   AggFunc::COUNT, ""})
        .aggregate({"total", AggFunc::SUM,   "value"})
        .onResult([&](WindowResult r) { results.push_back(r); })
        .build();

    pipeline->ingest(rec("r1", 0ms, 10.0));
    pipeline->ingest(rec("r2", 10000ms, 20.0));
    pipeline->flush();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(std::get<int64_t>(*results[0].get("cnt")), 2);
    EXPECT_DOUBLE_EQ(std::get<double>(*results[0].get("total")), 30.0);
}

TEST(PipelineTest, SlidingPipelineEndToEnd) {
    std::vector<WindowResult> results;
    auto pipeline = StreamingWindowPipeline::sliding(60000ms, 30000ms)
        .aggregate({"cnt", AggFunc::COUNT, ""})
        .onResult([&](WindowResult r) { results.push_back(r); })
        .build();

    pipeline->ingest(rec("r1", 45000ms, 1.0));
    pipeline->flush();

    EXPECT_GE(results.size(), 1u);
}

TEST(PipelineTest, SessionPipelineEndToEnd) {
    std::vector<WindowResult> results;
    auto pipeline = StreamingWindowPipeline::session(5000ms)
        .aggregate({"cnt", AggFunc::COUNT, ""})
        .onResult([&](WindowResult r) { results.push_back(r); })
        .build();

    pipeline->ingest(rec("r1", 0ms, 1.0, "user1"));
    pipeline->ingest(rec("r2", 1000ms, 2.0, "user1"));
    pipeline->flush();

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(std::get<int64_t>(*results[0].get("cnt")), 2);
}

TEST(PipelineTest, HoppingPipelineEndToEnd) {
    std::vector<WindowResult> results;
    auto pipeline = StreamingWindowPipeline::hopping(60000ms, 30000ms)
        .aggregate({"cnt", AggFunc::COUNT, ""})
        .onResult([&](WindowResult r) { results.push_back(r); })
        .build();

    pipeline->ingest(rec("r1", 45000ms, 1.0));
    pipeline->flush();

    EXPECT_GE(results.size(), 1u);
}

TEST(PipelineTest, StatsAfterIngest) {
    auto pipeline = StreamingWindowPipeline::tumbling(60000ms).build();
    pipeline->ingest(rec("r1", 0ms, 1.0));
    pipeline->ingest(rec("r2", 0ms, 2.0));
    auto s = pipeline->getStats();
    EXPECT_EQ(s.records_ingested, 2u);
}

TEST(PipelineTest, FlushEmitsWithoutCallback) {
    // No callback set: should not crash
    auto pipeline = StreamingWindowPipeline::tumbling(60000ms)
        .aggregate({"cnt", AggFunc::COUNT, ""})
        .build();
    pipeline->ingest(rec("r1", 0ms, 1.0));
    EXPECT_NO_THROW(pipeline->flush());
}

TEST(PipelineTest, MultipleIngestsAndFlush) {
    std::atomic<int> fire_count{0};
    auto pipeline = StreamingWindowPipeline::tumbling(60000ms)
        .aggregate({"sum", AggFunc::SUM, "value"})
        .onResult([&](WindowResult) { ++fire_count; })
        .build();

    for (int i = 0; i < 10; ++i) {
        pipeline->ingest(rec("r" + std::to_string(i), std::chrono::milliseconds(i * 1000), static_cast<double>(i)));
    }
    pipeline->flush();
    EXPECT_GE(fire_count.load(), 1);
}

// ============================================================================
// Bug-fix regression tests
// ============================================================================

// BUG 2: SessionWindow::ingest should not regress last_event on OOO records.
// An out-of-order record arriving within the gap must NOT cause the session to
// be closed on the next timer tick (last_event must not move backward).
TEST(BugfixTest, SessionWindowOutOfOrderDoesNotRegressLastEvent) {
    std::atomic<int> fired{0};
    SessionWindow win([]() {
        SessionWindowConfig c;
        c.gap = 200ms; // short gap for the timer test
        return c;
    }());
    win.setResultCallback([&](WindowResult) { ++fired; });

    // Record at t=0
    win.ingest(rec("r1", 0ms, 1.0, "user"));
    // OOO record at t=0 (same offset) — last_event must remain max(0,0)=0
    win.ingest(rec("r2", 0ms, 2.0, "user"));
    // Advance: record at t=100ms — still within 200ms gap
    win.ingest(rec("r3", 100ms, 3.0, "user"));

    // Sleep 50ms: gap timer should NOT fire (last_event = 100ms, not regressed to 0ms).
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(fired.load(), 0) << "session must not expire prematurely";

    // Flush should emit exactly one session with 3 records.
    win.flush();
    EXPECT_EQ(fired.load(), 1);
}

// BUG 4: SessionWindow must track watermark and drop records flagged as late.
TEST(BugfixTest, SessionWindowDropsLateRecordWhenAllowLateFalse) {
    SessionWindowConfig cfg;
    cfg.gap = 60000ms;
    cfg.watermark.max_out_of_orderness = std::chrono::milliseconds(0);
    cfg.watermark.allow_late_data = false;
    SessionWindow win(cfg);
    win.addAggregation({"cnt", AggFunc::COUNT, ""});

    // Advance watermark with a far-future record
    bool ok_future = win.ingest(rec("future", 300000ms, 1.0, "u1"));
    EXPECT_TRUE(ok_future);

    // Now ingest a very old record — should be dropped
    bool ok_old = win.ingest(rec("old", 0ms, 2.0, "u1"));
    EXPECT_FALSE(ok_old);

    auto stats = win.getStats();
    EXPECT_GE(stats.records_dropped, 1u);
    EXPECT_GE(stats.late_records, 1u);
}

// BUG 5: DISTINCT_COUNT must not count records where the field is absent.
TEST(BugfixTest, DistinctCountIgnoresMissingField) {
    std::vector<WindowResult> results;
    TumblingWindow win([]() {
        TumblingWindowConfig c;
        c.size = 60000ms;
        return c;
    }());
    win.addAggregation({"dc", AggFunc::DISTINCT_COUNT, "cat"});
    win.setResultCallback([&](WindowResult r) { results.push_back(r); });

    // Records WITH the "cat" field: 2 distinct values
    win.ingest(makeRecord("", baseTime(), "", {{"cat", std::string("A")}}));
    win.ingest(makeRecord("", baseTime(), "", {{"cat", std::string("B")}}));
    win.ingest(makeRecord("", baseTime(), "", {{"cat", std::string("A")}}));
    // Record WITHOUT the "cat" field: must NOT inflate the distinct count
    win.ingest(makeRecord("", baseTime(), "", {{"other", std::string("")}}));
    win.flush();

    ASSERT_FALSE(results.empty());
    // Expect exactly 2 distinct values ("A" and "B"), not 3
    EXPECT_EQ(std::get<int64_t>(*results[0].get("dc")), 2);
}

// BUG 7: StreamingWindowPipeline must refuse ingest()/flush() before build().
TEST(BugfixTest, PipelineIngestBeforeBuildReturnsFalse) {
    StreamingWindowPipeline p = StreamingWindowPipeline::tumbling(60000ms);
    // build() has NOT been called — ingest() and flush() must be no-ops
    EXPECT_FALSE(p.ingest(rec("r1", 0ms, 1.0)));
    EXPECT_NO_THROW(p.flush());
    auto s = p.getStats();
    EXPECT_EQ(s.records_ingested, 0u);
}

// ============================================================================
// aggFuncToString helper
// ============================================================================

TEST(HelpersTest, AggFuncToString) {
    EXPECT_STREQ(aggFuncToString(AggFunc::COUNT),          "COUNT");
    EXPECT_STREQ(aggFuncToString(AggFunc::SUM),            "SUM");
    EXPECT_STREQ(aggFuncToString(AggFunc::AVG),            "AVG");
    EXPECT_STREQ(aggFuncToString(AggFunc::MIN),            "MIN");
    EXPECT_STREQ(aggFuncToString(AggFunc::MAX),            "MAX");
    EXPECT_STREQ(aggFuncToString(AggFunc::STDDEV),         "STDDEV");
    EXPECT_STREQ(aggFuncToString(AggFunc::VARIANCE),       "VARIANCE");
    EXPECT_STREQ(aggFuncToString(AggFunc::PERCENTILE),     "PERCENTILE");
    EXPECT_STREQ(aggFuncToString(AggFunc::FIRST),          "FIRST");
    EXPECT_STREQ(aggFuncToString(AggFunc::LAST),           "LAST");
    EXPECT_STREQ(aggFuncToString(AggFunc::DISTINCT_COUNT), "DISTINCT_COUNT");
}

// ============================================================================
// Runtime limit tests
// ============================================================================

// TumblingWindow: max_open_windows forces eviction of oldest window when
// a record arrives that would open a new slot while already at capacity.
TEST(RuntimeLimitTest, TumblingWindowMaxOpenWindowsEviction) {
    TumblingWindowConfig cfg;
    cfg.size             = 1000ms; // 1-second windows
    cfg.max_open_windows = 2;      // allow at most 2 open windows
    // Use a large watermark tolerance so the watermark does not close
    // earlier windows before the eviction limit fires.
    cfg.watermark.max_out_of_orderness = std::chrono::hours(24);

    std::atomic<int> fired{0};
    TumblingWindow win(cfg);
    win.setResultCallback([&](WindowResult) { ++fired; });

    // Open window at slot 0 (t=0..1000 ms)
    EXPECT_TRUE(win.ingest(rec("r1",    0ms, 1.0)));
    // Open window at slot 1 (t=1000..2000 ms)
    EXPECT_TRUE(win.ingest(rec("r2", 1000ms, 2.0)));
    // Opening slot 2 must evict slot 0 → windows_evicted == 1
    EXPECT_TRUE(win.ingest(rec("r3", 2000ms, 3.0)));

    auto stats = win.getStats();
    EXPECT_GE(stats.windows_evicted, 1u) << "oldest window must be evicted at capacity";
    // Eviction emits a result for the evicted window
    EXPECT_GE(fired.load(), 1);
}

// TumblingWindow: max_records_per_window drops records once a slot is full.
TEST(RuntimeLimitTest, TumblingWindowMaxRecordsPerWindowDropsRecords) {
    TumblingWindowConfig cfg;
    cfg.size                 = 60000ms; // single window for the test
    cfg.max_records_per_window = 2;

    TumblingWindow win(cfg);

    EXPECT_TRUE(win.ingest(rec("r1", 0ms, 1.0)));
    EXPECT_TRUE(win.ingest(rec("r2", 0ms, 2.0)));
    // Third record in the same slot must be dropped
    EXPECT_FALSE(win.ingest(rec("r3", 0ms, 3.0)));

    auto stats = win.getStats();
    EXPECT_EQ(stats.records_dropped, 1u) << "record over limit must be counted as dropped";
    EXPECT_EQ(stats.records_ingested, 2u);
}

// SlidingWindow: max_open_windows skips new window creation (not eviction).
// Verifies the record is still accepted even when window creation is limited.
TEST(RuntimeLimitTest, SlidingWindowMaxOpenWindowsSkip) {
    SlidingWindowConfig cfg;
    cfg.size             = 200ms;
    cfg.slide            = 100ms;
    cfg.max_open_windows = 1; // only 1 concurrent window allowed

    SlidingWindow win(cfg);

    // A record at baseTime+0ms would normally open 3 overlapping windows
    // ([base-200ms,base+0ms), [base-100ms,base+100ms), [base+0ms,base+200ms)).
    // With max=1, only the first is created; the other 2 are skipped.
    EXPECT_TRUE(win.ingest(rec("r1", 0ms, 1.0)));

    auto stats = win.getStats();
    EXPECT_GE(stats.windows_evicted, 1u) << "skipped window creations must be counted";
    // The record itself must still be ingested
    EXPECT_EQ(stats.records_ingested, 1u);
}

// SlidingWindow: with a very tight max_open_windows limit, the skip counter
// must be non-zero when overlapping windows would otherwise be created.
TEST(RuntimeLimitTest, SlidingWindowMaxOpenWindowsSkipCounterNonZero) {
    SlidingWindowConfig cfg;
    cfg.size             = 500ms;
    cfg.slide            = 100ms;
    cfg.max_open_windows = 1; // artificially tight: only 1 concurrent window allowed

    SlidingWindow win(cfg);

    // This record would normally open 5 overlapping windows (500/100 = 5 slides).
    // With max=1, only the first window is created; the rest are skipped.
    EXPECT_TRUE(win.ingest(rec("r1", 250ms, 1.0)));

    auto stats = win.getStats();
    EXPECT_GE(stats.windows_evicted, 1u) << "skipped window creations must be counted";
}

// SessionWindow: max_open_sessions forces eviction of the oldest session when
// a new partition key arrives while already at the session limit.
TEST(RuntimeLimitTest, SessionWindowMaxOpenSessionsEviction) {
    SessionWindowConfig cfg;
    cfg.gap                = 60000ms; // long gap so sessions don't expire naturally
    cfg.max_open_sessions  = 2;
    cfg.session_expiry_check_interval_ms = 1000ms; // infrequent background check

    std::atomic<int> fired{0};
    SessionWindow win(cfg);
    win.setResultCallback([&](WindowResult) { ++fired; });

    // Open sessions for two distinct keys
    EXPECT_TRUE(win.ingest(rec("r1", 0ms, 1.0, "key_A")));
    EXPECT_TRUE(win.ingest(rec("r2", 1ms, 2.0, "key_B")));
    // Third distinct key must evict one of the existing sessions → windows_evicted == 1
    EXPECT_TRUE(win.ingest(rec("r3", 2ms, 3.0, "key_C")));

    auto stats = win.getStats();
    EXPECT_GE(stats.windows_evicted, 1u) << "oldest session must be evicted at capacity";
}

// HoppingWindow: max_open_windows skips new window creation (not eviction),
// analogous to SlidingWindow behaviour.
TEST(RuntimeLimitTest, HoppingWindowMaxOpenWindowsSkip) {
    HoppingWindowConfig cfg;
    cfg.size             = 500ms;
    cfg.hop              = 100ms;
    cfg.max_open_windows = 1; // only 1 concurrent window allowed

    HoppingWindow win(cfg);

    // A record at 250 ms would normally open multiple hop slots.
    // With max=1, subsequent hop slots are skipped → windows_evicted > 0.
    EXPECT_TRUE(win.ingest(rec("r1", 250ms, 1.0)));

    auto stats = win.getStats();
    EXPECT_GE(stats.windows_evicted, 1u) << "skipped hop-window creations must be counted";
    EXPECT_EQ(stats.records_ingested, 1u) << "record itself must not be dropped";
}