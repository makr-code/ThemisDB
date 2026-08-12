/**
 * Tests for TemporalAggregator
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "temporal/temporal_aggregator.h"
#include <thread>

using namespace themisdb::temporal;

// ============================================================================
// Helpers
// ============================================================================

// Insert a row with a numeric "value" field.
// Sleeps 1 ms between calls so that each insert gets a distinct sys_start
// timestamp (wall-clock milliseconds).
static void insertAt(SystemVersionedTable& t,
                     const std::string& key,
                     double value,
                     int extra_sleep_ms = 1) {
    std::this_thread::sleep_for(std::chrono::milliseconds(extra_sleep_ms));
    if (!t.insert(key, {{"value", value}})) {
        // Key already exists as current row; create a new version.
        t.update(key, {{"value", value}});
    }
}

class TemporalAggregatorTest : public ::testing::Test {
protected:
    TemporalAggregator agg;
};

// ============================================================================
// TUMBLING windows
// ============================================================================

TEST_F(TemporalAggregatorTest, Tumbling_COUNT_EmptyTable) {
    SystemVersionedTable t{"tbl", "n"};
    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10;
    spec.func           = AggregateFunc::COUNT;

    // No rows → all windows have count 0 (windows are still generated)
    Timestamp from = now();
    Timestamp to   = from + 30;
    auto results = agg.aggregate(t, spec, from, to);
    EXPECT_EQ(results.size(), 3u); // 3 windows of 10ms
    for (const auto& r : results) {
        EXPECT_EQ(r.record_count, 0u);
        EXPECT_EQ(r.value, 0.0);
    }
}

TEST_F(TemporalAggregatorTest, Tumbling_COUNT_RowsDistributedAcrossWindows) {
    // Insert at least 3 rows spread across 3 windows of 10ms each.
    // We use the actual wall clock and just check the total count == 3.
    SystemVersionedTable t{"tbl", "n"};
    Timestamp t0 = now();
    t.insert("k1", {{"value", 10}});
    t.insert("k2", {{"value", 20}});
    t.insert("k3", {{"value", 30}});
    Timestamp t1 = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 1000; // 1-second tumbling windows
    spec.func           = AggregateFunc::COUNT;

    auto results = agg.aggregate(t, spec, t0, t1);

    // Total count across all windows must be exactly 3
    size_t total = 0;
    for (const auto& r : results) {
        total += r.record_count;
    }
    EXPECT_EQ(total, 3u);
}

TEST_F(TemporalAggregatorTest, Tumbling_SUM_SingleWindow) {
    SystemVersionedTable t{"tbl", "n"};

    Timestamp base = now();
    t.insert("k1", {{"value", 10}});
    t.insert("k2", {{"value", 20}});
    t.insert("k3", {{"value", 30}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10'000; // 10-second window – all rows fit in one window
    spec.func           = AggregateFunc::SUM;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, base, after);
    // Expect at least one window with sum = 60
    double total_sum = 0.0;
    for (const auto& r : results) {
        total_sum += r.value;
    }
    EXPECT_DOUBLE_EQ(total_sum, 60.0);
}

TEST_F(TemporalAggregatorTest, Tumbling_AVG_SingleWindow) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"value", 10.0}});
    t.insert("k2", {{"value", 20.0}});
    t.insert("k3", {{"value", 30.0}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10'000;
    spec.func           = AggregateFunc::AVG;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, base, after);

    // Collect all rows in a single effective window
    double total_sum   = 0.0;
    size_t total_count = 0;
    for (const auto& r : results) {
        total_sum   += r.value * r.record_count;
        total_count += r.record_count;
    }
    if (total_count > 0) {
        EXPECT_NEAR(total_sum / total_count, 20.0, 1e-9);
    }
}

TEST_F(TemporalAggregatorTest, Tumbling_MIN_MAX) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"value", 5.0}});
    t.insert("k2", {{"value", 15.0}});
    t.insert("k3", {{"value", 25.0}});
    Timestamp after = now() + 1;

    auto runAgg = [&](AggregateFunc func) {
        AggregationSpec spec;
        spec.window_type    = WindowType::TUMBLING;
        spec.window_size_ms = 10'000;
        spec.func           = func;
        spec.measure_field  = "value";
        return agg.aggregate(t, spec, base, after);
    };

    // MIN across all non-empty windows should include 5.0
    auto min_results = runAgg(AggregateFunc::MIN);
    double global_min = std::numeric_limits<double>::max();
    for (const auto& r : min_results) {
        if (r.record_count > 0) {
            global_min = std::min(global_min, r.value);
        }
    }
    EXPECT_DOUBLE_EQ(global_min, 5.0);

    // MAX across all non-empty windows should include 25.0
    auto max_results = runAgg(AggregateFunc::MAX);
    double global_max = std::numeric_limits<double>::lowest();
    for (const auto& r : max_results) {
        if (r.record_count > 0) {
            global_max = std::max(global_max, r.value);
        }
    }
    EXPECT_DOUBLE_EQ(global_max, 25.0);
}

TEST_F(TemporalAggregatorTest, Tumbling_NoMeasureFieldForSUM_ValueIsZero) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"label", "a"}});
    t.insert("k2", {{"label", "b"}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10'000;
    spec.func           = AggregateFunc::SUM;
    spec.measure_field  = "missing_field";

    auto results = agg.aggregate(t, spec, base, after);
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.value, 0.0);
    }
}

TEST_F(TemporalAggregatorTest, Tumbling_ResultWindowsAreContiguous) {
    SystemVersionedTable t{"tbl", "n"};
    t.insert("k1", {{"value", 1}});

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 5;
    spec.func           = AggregateFunc::COUNT;

    Timestamp from = 100;
    Timestamp to   = 120;
    auto results = agg.aggregate(t, spec, from, to);

    ASSERT_EQ(results.size(), 4u); // [100,105), [105,110), [110,115), [115,120)
    EXPECT_EQ(results[0].window_start, 100);
    EXPECT_EQ(results[0].window_end,   105);
    EXPECT_EQ(results[1].window_start, 105);
    EXPECT_EQ(results[3].window_end,   120);

    // Windows must be contiguous
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_EQ(results[i].window_start, results[i - 1].window_end);
    }
}

// ============================================================================
// SLIDING windows
// ============================================================================

TEST_F(TemporalAggregatorTest, Sliding_COUNT_RowInMultipleWindows) {
    // Use a known timestamp range so we can predict window assignment.
    // Window size = 10, slide = 5 → windows overlap by 5ms
    // A row at t=102 falls in window [100,110) and window [100,110) slide by 5 → [105,115)?
    // Let's verify the total row count sums to more than the raw row count.
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"value", 1}});
    t.insert("k2", {{"value", 2}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type       = WindowType::SLIDING;
    spec.window_size_ms    = 10;
    spec.slide_interval_ms = 5;
    spec.func              = AggregateFunc::COUNT;

    auto results = agg.aggregate(t, spec, base, after);
    // Each row may appear in multiple windows → total count ≥ 2
    size_t total = 0;
    for (const auto& r : results) {
        total += r.record_count;
    }
    EXPECT_GE(total, 2u);
}

TEST_F(TemporalAggregatorTest, Sliding_DefaultSlide_EqualsTumbling) {
    // When slide_interval_ms == 0 (default) the behaviour should match TUMBLING.
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"value", 1}});
    t.insert("k2", {{"value", 2}});
    t.insert("k3", {{"value", 3}});
    Timestamp after = now() + 1;

    AggregationSpec spec_tumbling;
    spec_tumbling.window_type    = WindowType::TUMBLING;
    spec_tumbling.window_size_ms = 5000;
    spec_tumbling.func           = AggregateFunc::COUNT;

    AggregationSpec spec_sliding;
    spec_sliding.window_type       = WindowType::SLIDING;
    spec_sliding.window_size_ms    = 5000;
    spec_sliding.slide_interval_ms = 0; // defaults to window_size
    spec_sliding.func              = AggregateFunc::COUNT;

    auto r_t = agg.aggregate(t, spec_tumbling, base, after);
    auto r_s = agg.aggregate(t, spec_sliding,  base, after);

    // Same number of windows and same total count
    EXPECT_EQ(r_t.size(), r_s.size());
    size_t sum_t = 0, sum_s = 0;
    for (const auto& r : r_t) sum_t += r.record_count;
    for (const auto& r : r_s) sum_s += r.record_count;
    EXPECT_EQ(sum_t, sum_s);
}

// ============================================================================
// SESSION windows
// ============================================================================

TEST_F(TemporalAggregatorTest, Session_COUNT_TwoSessions) {
    // Create two groups of events separated by a large gap.
    // The aggregator uses sys_time.start timestamps, which are real wall-clock
    // values in milliseconds.  We need a gap > gap_duration_ms.
    // We sleep to create real gap in sys_start.
    SystemVersionedTable t{"tbl", "n"};
    t.insert("k1", {{"value", 1}});
    t.insert("k2", {{"value", 2}});
    // Gap: sleep for more than the gap_duration_ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    t.insert("k3", {{"value", 3}});
    t.insert("k4", {{"value", 4}});

    AggregationSpec spec;
    spec.window_type    = WindowType::SESSION;
    spec.gap_duration_ms = 20; // 20ms gap closes a session
    spec.func           = AggregateFunc::COUNT;

    auto results = agg.aggregate(t, spec, kMinTimestamp, kMaxTimestamp);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_EQ(results[0].record_count, 2u);
    EXPECT_EQ(results[1].record_count, 2u);
}

TEST_F(TemporalAggregatorTest, Session_SUM_MergeLargeGap) {
    SystemVersionedTable t{"tbl", "n"};
    t.insert("k1", {{"value", 10.0}});
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    t.insert("k2", {{"value", 20.0}});

    AggregationSpec spec;
    spec.window_type    = WindowType::SESSION;
    spec.gap_duration_ms = 30;
    spec.func           = AggregateFunc::SUM;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, kMinTimestamp, kMaxTimestamp);
    ASSERT_EQ(results.size(), 2u);
    EXPECT_DOUBLE_EQ(results[0].value, 10.0);
    EXPECT_DOUBLE_EQ(results[1].value, 20.0);
}

TEST_F(TemporalAggregatorTest, Session_EmptyTable_ReturnsEmpty) {
    SystemVersionedTable t{"tbl", "n"};
    AggregationSpec spec;
    spec.window_type    = WindowType::SESSION;
    spec.gap_duration_ms = 100;
    spec.func           = AggregateFunc::COUNT;

    auto results = agg.aggregate(t, spec, kMinTimestamp, kMaxTimestamp);
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// toJson sanity check
// ============================================================================

TEST_F(TemporalAggregatorTest, AggregateResult_ToJson) {
    AggregateResult r;
    r.window_start  = 1000;
    r.window_end    = 2000;
    r.value         = 42.0;
    r.record_count  = 5;

    auto j = r.toJson();
    EXPECT_EQ(j["window_start"], 1000);
    EXPECT_EQ(j["window_end"],   2000);
    EXPECT_DOUBLE_EQ(j["value"].get<double>(), 42.0);
    EXPECT_EQ(j["record_count"], 5);
}

TEST_F(TemporalAggregatorTest, AggregateResult_ToJson_WithGroupValues) {
    AggregateResult r;
    r.window_start  = 1000;
    r.window_end    = 2000;
    r.value         = 7.0;
    r.record_count  = 1;
    r.group_values  = {{"region", "us-east"}, {"product", "widget"}};

    auto j = r.toJson();
    EXPECT_TRUE(j.contains("group_values"));
    EXPECT_EQ(j["group_values"]["region"], "us-east");
    EXPECT_EQ(j["group_values"]["product"], "widget");
}

// ============================================================================
// GROUP BY aggregations
// ============================================================================

TEST_F(TemporalAggregatorTest, GroupBy_COUNT_TwoGroups) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"region", "us"}, {"value", 10}});
    t.insert("k2", {{"region", "eu"}, {"value", 20}});
    t.insert("k3", {{"region", "us"}, {"value", 30}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10'000;
    spec.func           = AggregateFunc::COUNT;
    spec.group_by_fields = {"region"};

    auto groups = agg.aggregateByGroup(t, spec, base, after);

    // Expect exactly two groups
    ASSERT_EQ(groups.size(), 2u);

    size_t us_count = 0, eu_count = 0;
    for (auto& [key, windows] : groups) {
        for (const auto& w : windows) {
            if (w.group_values.count("region") &&
                w.group_values.at("region") == "us") {
                us_count += w.record_count;
            } else if (w.group_values.count("region") &&
                       w.group_values.at("region") == "eu") {
                eu_count += w.record_count;
            }
        }
    }
    EXPECT_EQ(us_count, 2u);
    EXPECT_EQ(eu_count, 1u);
}

TEST_F(TemporalAggregatorTest, GroupBy_SUM_GroupValuesPopulated) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"cat", "A"}, {"price", 5.0}});
    t.insert("k2", {{"cat", "B"}, {"price", 15.0}});
    t.insert("k3", {{"cat", "A"}, {"price", 25.0}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type     = WindowType::TUMBLING;
    spec.window_size_ms  = 10'000;
    spec.func            = AggregateFunc::SUM;
    spec.measure_field   = "price";
    spec.group_by_fields = {"cat"};

    auto groups = agg.aggregateByGroup(t, spec, base, after);
    ASSERT_EQ(groups.size(), 2u);

    double sum_a = 0.0;
    for (auto& [key, windows] : groups) {
        if (!windows.empty() &&
            windows[0].group_values.count("cat") &&
            windows[0].group_values.at("cat") == "A") {
            for (const auto& w : windows) sum_a += w.value;
        }
    }
    EXPECT_DOUBLE_EQ(sum_a, 30.0);
}

TEST_F(TemporalAggregatorTest, GroupBy_EmptyGroupByFields_ReturnsUnnamedGroup) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"value", 1}});
    Timestamp after = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 10'000;
    spec.func           = AggregateFunc::COUNT;
    // group_by_fields intentionally empty

    auto groups = agg.aggregateByGroup(t, spec, base, after);
    ASSERT_EQ(groups.size(), 1u);
    // The single group key is the empty string
    EXPECT_TRUE(groups.count(""));
}

TEST_F(TemporalAggregatorTest, GroupBy_InvalidRange_ReturnsEmpty) {
    SystemVersionedTable t{"tbl", "n"};
    t.insert("k1", {{"region", "us"}});

    AggregationSpec spec;
    spec.window_type     = WindowType::TUMBLING;
    spec.window_size_ms  = 1000;
    spec.func            = AggregateFunc::COUNT;
    spec.group_by_fields = {"region"};

    Timestamp ts = now();
    auto groups = agg.aggregateByGroup(t, spec, ts, ts); // from == to
    EXPECT_TRUE(groups.empty());

    groups = agg.aggregateByGroup(t, spec, ts + 1000, ts); // from > to (1000ms = spec.window_size_ms)
    EXPECT_TRUE(groups.empty());
}

// ============================================================================
// Snapshot aggregations
// ============================================================================

TEST_F(TemporalAggregatorTest, Snapshots_COUNT_StableRow) {
    // Insert a single row that is current across the whole query range.
    SystemVersionedTable t{"tbl", "n"};
    t.insert("k1", {{"value", 1}});

    // Query 5 snapshots of 1-second intervals far in the future
    // so the row is still current at every tick.
    Timestamp from = now() + 100'000; // 100 seconds in the future
    Timestamp to   = from + 5000;     // 5 seconds range

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 1000;
    spec.func           = AggregateFunc::COUNT;

    auto results = agg.aggregateSnapshots(t, spec, from, to);
    ASSERT_EQ(results.size(), 5u);
    for (const auto& r : results) {
        EXPECT_EQ(r.record_count, 1u); // row visible at every snapshot
    }
}

TEST_F(TemporalAggregatorTest, Snapshots_SUM_VisibleAtSnapshotTime) {
    SystemVersionedTable t{"tbl", "n"};
    // Insert a row with sys_start < from so it is visible throughout
    Timestamp now_ts = now();
    t.insert("k1", {{"value", 7.0}});

    Timestamp from = now_ts + 100'000;
    Timestamp to   = from + 3000;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 1000;
    spec.func           = AggregateFunc::SUM;
    spec.measure_field  = "value";

    auto results = agg.aggregateSnapshots(t, spec, from, to);
    ASSERT_EQ(results.size(), 3u);
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.value, 7.0);
    }
}

TEST_F(TemporalAggregatorTest, Snapshots_EmptyTable_ReturnsEmpty) {
    SystemVersionedTable t{"tbl", "n"};

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 1000;
    spec.func           = AggregateFunc::COUNT;

    Timestamp from = now();
    Timestamp to   = from + 3000;

    auto results = agg.aggregateSnapshots(t, spec, from, to);
    // No versions in the table → empty result vector.
    EXPECT_TRUE(results.empty());
}

// ============================================================================
// Trend analysis
// ============================================================================

TEST_F(TemporalAggregatorTest, AnalyzeTrend_IncreasingValues) {
    // Insert rows with monotonically increasing values spread over time
    // so the trend slope should be positive.
    // Sleep between inserts to guarantee distinct millisecond sys_start values
    // (matching the convention used by Session window tests in this file).
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"v", 10.0}});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    t.insert("k2", {{"v", 20.0}});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    t.insert("k3", {{"v", 30.0}});
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    t.insert("k4", {{"v", 40.0}});
    Timestamp after = now() + 1;

    auto trend = agg.analyzeTrend(t, "v", base, after);

    EXPECT_GT(trend.slope, 0.0);  // increasing values → positive slope
    EXPECT_GE(trend.sample_count, 1u);
    EXPECT_GE(trend.r_squared, 0.0);
    EXPECT_LE(trend.r_squared, 1.0);
    EXPECT_EQ(trend.period_start, base);
    EXPECT_EQ(trend.period_end,   after);
}

TEST_F(TemporalAggregatorTest, AnalyzeTrend_EmptyTable_ReturnsZeroTrend) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    auto trend = agg.analyzeTrend(t, "v", base, base + 1000);

    EXPECT_EQ(trend.slope,        0.0);
    EXPECT_EQ(trend.intercept,    0.0);
    EXPECT_EQ(trend.r_squared,    0.0);
    EXPECT_EQ(trend.sample_count, 0u);
}

TEST_F(TemporalAggregatorTest, AnalyzeTrend_ToJson) {
    // Only checks that the returned JSON object contains the required keys;
    // no timing-dependent assertions needed here.
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("k1", {{"v", 5.0}});
    t.insert("k2", {{"v", 10.0}});
    Timestamp after = now() + 1;

    auto trend = agg.analyzeTrend(t, "v", base, after);
    auto j = trend.toJson();

    EXPECT_TRUE(j.contains("slope"));
    EXPECT_TRUE(j.contains("intercept"));
    EXPECT_TRUE(j.contains("r_squared"));
    EXPECT_TRUE(j.contains("sample_count"));
    EXPECT_TRUE(j.contains("period_start"));
    EXPECT_TRUE(j.contains("period_end"));
}

// ── FIRST_VALUE / LAST_VALUE tests (FLV-01..05) ──────────────────────────────

// Inserts rows in a known order using wall-clock time (1 ms sleep between
// each insert so that sys_start timestamps are strictly increasing).

TEST_F(TemporalAggregatorTest, FLV_01_FirstValue_Tumbling_ReturnsEarliestInWindow) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();

    // Insert in ascending sys_start order: 10.0 first, 20.0, 30.0 last.
    insertAt(t, "k1", 10.0);  // earliest
    insertAt(t, "k1", 20.0);
    insertAt(t, "k1", 30.0);  // latest

    Timestamp end = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = end - base + 10;
    spec.func           = AggregateFunc::FIRST_VALUE;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, base, end);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_DOUBLE_EQ(results[0].value, 10.0);
}

TEST_F(TemporalAggregatorTest, FLV_02_LastValue_Tumbling_ReturnsLatestInWindow) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();

    insertAt(t, "k1", 10.0);
    insertAt(t, "k1", 20.0);
    insertAt(t, "k1", 30.0);  // latest

    Timestamp end = now() + 1;

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = end - base + 10;
    spec.func           = AggregateFunc::LAST_VALUE;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, base, end);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_DOUBLE_EQ(results[0].value, 30.0);
}

TEST_F(TemporalAggregatorTest, FLV_03_FirstLast_SingleRow_ReturnSameValue) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();
    t.insert("solo", {{"value", 42.0}});
    Timestamp end = now() + 1;

    for (auto func : {AggregateFunc::FIRST_VALUE, AggregateFunc::LAST_VALUE}) {
        AggregationSpec spec;
        spec.window_type    = WindowType::TUMBLING;
        spec.window_size_ms = end - base + 10;
        spec.func           = func;
        spec.measure_field  = "value";

        auto results = agg.aggregate(t, spec, base, end);
        ASSERT_GE(results.size(), 1u);
        EXPECT_DOUBLE_EQ(results[0].value, 42.0);
    }
}

TEST_F(TemporalAggregatorTest, FLV_04_FirstValue_EmptyWindow_ResultIsEmpty) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp future_base = now() + 100000;  // far in the future — no rows

    AggregationSpec spec;
    spec.window_type    = WindowType::TUMBLING;
    spec.window_size_ms = 100;
    spec.func           = AggregateFunc::FIRST_VALUE;
    spec.measure_field  = "value";

    auto results = agg.aggregate(t, spec, future_base, future_base + 100);
    // No rows → either empty result list or value=0.0
    for (const auto& r : results) {
        EXPECT_DOUBLE_EQ(r.value, 0.0);
    }
}

TEST_F(TemporalAggregatorTest, FLV_05_FirstValue_Differs_From_LastValue) {
    SystemVersionedTable t{"tbl", "n"};
    Timestamp base = now();

    insertAt(t, "k1", 100.0);  // earliest
    insertAt(t, "k1", 200.0);  // latest

    Timestamp end = now() + 1;
    int64_t win = end - base + 10;

    AggregationSpec spec_first;
    spec_first.window_type    = WindowType::TUMBLING;
    spec_first.window_size_ms = win;
    spec_first.func           = AggregateFunc::FIRST_VALUE;
    spec_first.measure_field  = "value";

    AggregationSpec spec_last = spec_first;
    spec_last.func = AggregateFunc::LAST_VALUE;

    auto first_res = agg.aggregate(t, spec_first, base, end);
    auto last_res  = agg.aggregate(t, spec_last,  base, end);

    ASSERT_EQ(first_res.size(), 1u);
    ASSERT_EQ(last_res.size(),  1u);
    EXPECT_DOUBLE_EQ(first_res[0].value, 100.0);
    EXPECT_DOUBLE_EQ(last_res[0].value,  200.0);
}
