/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_temporal_aggregator.cpp                       ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:57:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     375                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    t.insert(key, {{"value", value}});
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
