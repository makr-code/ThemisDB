/**
 * @file test_streaming_join.cpp
 * @brief Focused unit tests for analytics::HashJoin and analytics::IntervalJoin.
 *
 * Test IDs: SJ-01 … SJ-15
 */

#include <gtest/gtest.h>
#include "analytics/streaming_join.h"
#include "analytics/columnar_execution.h"

#include <memory>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a ColumnBatch with int64 "id" and string "name" columns.
static ColumnBatch makeIdNameBatch(const std::vector<std::pair<int64_t, std::string>>& rows) {
    auto id_col   = std::make_shared<Column>("id",   ColumnType::Int64);
    auto name_col = std::make_shared<Column>("name", ColumnType::String);
    for (const auto& [id, name] : rows) {
        id_col->appendInt64(id);
        name_col->appendString(name);
    }
    ColumnBatch batch(rows.size());
    batch.addColumn(id_col);
    batch.addColumn(name_col);
    return batch;
}

/// Build a ColumnBatch with int64 "id" and int64 "score" columns.
static ColumnBatch makeIdScoreBatch(const std::vector<std::pair<int64_t, int64_t>>& rows) {
    auto id_col    = std::make_shared<Column>("id",    ColumnType::Int64);
    auto score_col = std::make_shared<Column>("score", ColumnType::Int64);
    for (const auto& [id, score] : rows) {
        id_col->appendInt64(id);
        score_col->appendInt64(score);
    }
    ColumnBatch batch(rows.size());
    batch.addColumn(id_col);
    batch.addColumn(score_col);
    return batch;
}

/// Build a ColumnBatch with int64 "id", int64 "ts", and string "val" columns.
static ColumnBatch makeTimestampBatch(
    const std::vector<std::tuple<int64_t, int64_t, std::string>>& rows)
{
    auto id_col  = std::make_shared<Column>("id",  ColumnType::Int64);
    auto ts_col  = std::make_shared<Column>("ts",  ColumnType::Int64);
    auto val_col = std::make_shared<Column>("val", ColumnType::String);
    for (const auto& [id, ts, val] : rows) {
        id_col->appendInt64(id);
        ts_col->appendInt64(ts);
        val_col->appendString(val);
    }
    ColumnBatch batch(rows.size());
    batch.addColumn(id_col);
    batch.addColumn(ts_col);
    batch.addColumn(val_col);
    return batch;
}

// ===========================================================================
// SJ-01  HashJoin: empty probe returns empty result
// ===========================================================================
TEST(StreamingJoinTest, SJ01_HashJoinEmptyProbe) {
    HashJoin join(HashJoin::Config{.join_keys = {"id"}});
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}, {2, "bob"}}));

    ColumnBatch empty_probe(0);
    auto id_col = std::make_shared<Column>("id", ColumnType::Int64);
    empty_probe.addColumn(id_col);

    auto result = join.probe(empty_probe);
    EXPECT_EQ(result.rowCount(), 0u);
}

// ===========================================================================
// SJ-02  HashJoin: inner join basic correctness
// ===========================================================================
TEST(StreamingJoinTest, SJ02_HashJoinInnerBasic) {
    HashJoin join(HashJoin::Config{
        .join_keys = {"id"},
        .join_type = JoinType::Inner,
    });
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}, {2, "bob"}, {3, "charlie"}}));

    auto probe = makeIdScoreBatch({{1, 100}, {2, 200}, {4, 400}});
    auto result = join.probe(probe);

    // id=4 has no match → dropped in inner join
    EXPECT_EQ(result.rowCount(), 2u);

    auto id_out = result.getColumn("id");
    ASSERT_NE(id_out, nullptr);
    EXPECT_EQ(id_out->int64Data()[0], 1);
    EXPECT_EQ(id_out->int64Data()[1], 2);

    auto name_out = result.getColumn("name");
    ASSERT_NE(name_out, nullptr);
    EXPECT_EQ(name_out->stringData()[0], "alice");
    EXPECT_EQ(name_out->stringData()[1], "bob");
}

// ===========================================================================
// SJ-03  HashJoin: left-outer join preserves unmatched probe rows
// ===========================================================================
TEST(StreamingJoinTest, SJ03_HashJoinLeftOuter) {
    HashJoin join(HashJoin::Config{
        .join_keys = {"id"},
        .join_type = JoinType::LeftOuter,
    });
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}}));

    auto probe = makeIdScoreBatch({{1, 100}, {99, 999}});
    auto result = join.probe(probe);

    EXPECT_EQ(result.rowCount(), 2u); // both probe rows kept

    auto name_out = result.getColumn("name");
    ASSERT_NE(name_out, nullptr);
    EXPECT_FALSE(name_out->isNull(0)); // matched
    EXPECT_TRUE(name_out->isNull(1));  // unmatched → null
}

// ===========================================================================
// SJ-04  HashJoin: multiple matches per key (fan-out)
// ===========================================================================
TEST(StreamingJoinTest, SJ04_HashJoinFanOut) {
    HashJoin join(HashJoin::Config{.join_keys = {"id"}, .join_type = JoinType::Inner});
    // Build side has two rows with id=1.
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}, {1, "alex"}}));

    auto probe = makeIdScoreBatch({{1, 42}});
    auto result = join.probe(probe);

    EXPECT_EQ(result.rowCount(), 2u); // fan-out: one probe row × two build rows

    auto name_out = result.getColumn("name");
    ASSERT_NE(name_out, nullptr);
    // Both "alice" and "alex" should appear.
    auto& names = name_out->stringData();
    EXPECT_TRUE(std::find(names.begin(), names.end(), "alice") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "alex")  != names.end());
}

// ===========================================================================
// SJ-05  HashJoin: reset clears build side
// ===========================================================================
TEST(StreamingJoinTest, SJ05_HashJoinReset) {
    HashJoin join(HashJoin::Config{.join_keys = {"id"}, .join_type = JoinType::Inner});
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}}));
    EXPECT_EQ(join.buildSideSize(), 1u);

    join.reset();
    EXPECT_EQ(join.buildSideSize(), 0u);

    // After reset, probe should find nothing (build is empty → no inner matches).
    // Re-add build.
    join.addBuildBatch(makeIdNameBatch({{2, "bob"}}));
    auto probe = makeIdScoreBatch({{1, 10}});
    auto result = join.probe(probe);
    EXPECT_EQ(result.rowCount(), 0u); // id=1 not in build
}

// ===========================================================================
// SJ-06  HashJoin: build accumulation across multiple batches
// ===========================================================================
TEST(StreamingJoinTest, SJ06_HashJoinMultiBuildBatch) {
    HashJoin join(HashJoin::Config{.join_keys = {"id"}, .join_type = JoinType::Inner});
    join.addBuildBatch(makeIdNameBatch({{1, "alice"}}));
    join.addBuildBatch(makeIdNameBatch({{2, "bob"}}));
    join.addBuildBatch(makeIdNameBatch({{3, "charlie"}}));

    EXPECT_EQ(join.buildSideSize(), 3u);

    auto probe = makeIdScoreBatch({{1, 10}, {2, 20}, {3, 30}});
    auto result = join.probe(probe);
    EXPECT_EQ(result.rowCount(), 3u);
}

// ===========================================================================
// SJ-07  HashJoin: max_build_rows limit respected
// ===========================================================================
TEST(StreamingJoinTest, SJ07_HashJoinMaxBuildRows) {
    HashJoin join(HashJoin::Config{
        .join_keys     = {"id"},
        .max_build_rows = 2,
    });
    EXPECT_TRUE(join.addBuildBatch(makeIdNameBatch({{1, "a"}, {2, "b"}})));
    // Adding another row should fail.
    bool ok = join.addBuildBatch(makeIdNameBatch({{3, "c"}}));
    EXPECT_FALSE(ok);
    EXPECT_EQ(join.buildSideSize(), 2u);
}

// ===========================================================================
// SJ-08  HashJoin: no-key (empty join_keys) throws
// ===========================================================================
TEST(StreamingJoinTest, SJ08_HashJoinEmptyKeysThrows) {
    EXPECT_THROW(HashJoin join(HashJoin::Config{.join_keys = {}}),
                 std::invalid_argument);
}

// ===========================================================================
// SJ-09  IntervalJoin: basic time-interval inner join
// ===========================================================================
TEST(StreamingJoinTest, SJ09_IntervalJoinBasicInner) {
    IntervalJoin join(IntervalJoin::Config{
        .join_keys   = {"id"},
        .time_column = "ts",
        .before_ms   = 1000,
        .after_ms    = 1000,
        .join_type   = JoinType::Inner,
    });

    // Build side: id=1 at ts=100, id=2 at ts=200.
    join.addBuildBatch(makeTimestampBatch({{1, 100, "build_A"}, {2, 200, "build_B"}}));

    // Probe: id=1 at ts=100 (match), id=2 at ts=5000 (no match — too far).
    auto probe = makeTimestampBatch({{1, 100, "probe_A"}, {2, 5000, "probe_B"}});
    auto result = join.probe(probe);

    EXPECT_EQ(result.rowCount(), 1u); // only id=1 matches
    auto id_out = result.getColumn("id");
    ASSERT_NE(id_out, nullptr);
    EXPECT_EQ(id_out->int64Data()[0], 1);
}

// ===========================================================================
// SJ-10  IntervalJoin: left-outer preserves unmatched probe
// ===========================================================================
TEST(StreamingJoinTest, SJ10_IntervalJoinLeftOuter) {
    IntervalJoin join(IntervalJoin::Config{
        .join_keys   = {"id"},
        .time_column = "ts",
        .before_ms   = 500,
        .after_ms    = 500,
        .join_type   = JoinType::LeftOuter,
    });

    join.addBuildBatch(makeTimestampBatch({{1, 100, "B"}}));
    auto probe = makeTimestampBatch({{1, 100, "P"}, {2, 100, "Q"}});
    auto result = join.probe(probe);

    EXPECT_EQ(result.rowCount(), 2u); // both probe rows emitted
}

// ===========================================================================
// SJ-11  IntervalJoin: before/after asymmetry
// ===========================================================================
TEST(StreamingJoinTest, SJ11_IntervalJoinAsymmetric) {
    IntervalJoin join(IntervalJoin::Config{
        .join_keys   = {},  // no key: all build rows in window match
        .time_column = "ts",
        .before_ms   = 0,   // only build ts == probe ts or later
        .after_ms    = 100,
        .join_type   = JoinType::Inner,
    });

    // Build: ts=50 and ts=200.
    auto id_col  = std::make_shared<Column>("id", ColumnType::Int64);
    auto ts_col  = std::make_shared<Column>("ts", ColumnType::Int64);
    auto val_col = std::make_shared<Column>("val", ColumnType::String);
    id_col->appendInt64(1);  ts_col->appendInt64(50);  val_col->appendString("B50");
    id_col->appendInt64(2);  ts_col->appendInt64(200); val_col->appendString("B200");
    ColumnBatch build(2);
    build.addColumn(id_col); build.addColumn(ts_col); build.addColumn(val_col);
    join.addBuildBatch(build);

    // Probe at ts=100: window [100, 200]. Build ts=50 is BEFORE 100 → no match.
    // Build ts=200 is inside [100,200] → match.
    auto probe = makeTimestampBatch({{99, 100, "P"}});
    auto result = join.probe(probe);
    EXPECT_EQ(result.rowCount(), 1u);
}

// ===========================================================================
// SJ-12  IntervalJoin: reset clears build buffer
// ===========================================================================
TEST(StreamingJoinTest, SJ12_IntervalJoinReset) {
    IntervalJoin join(IntervalJoin::Config{
        .join_keys   = {"id"},
        .time_column = "ts",
        .before_ms   = 1000,
        .after_ms    = 1000,
    });
    join.addBuildBatch(makeTimestampBatch({{1, 100, "B"}}));
    EXPECT_EQ(join.buildSideSize(), 1u);
    join.reset();
    EXPECT_EQ(join.buildSideSize(), 0u);
}

// ===========================================================================
// SJ-13  IntervalJoin: build buffer pruning (old events discarded)
// ===========================================================================
TEST(StreamingJoinTest, SJ13_IntervalJoinPruning) {
    IntervalJoin join(IntervalJoin::Config{
        .join_keys   = {},
        .time_column = "ts",
        .before_ms   = 500,
        .after_ms    = 500,
        .slack_ms    = 0,
    });

    // Build: events from ts=0 to ts=100.
    for (int64_t t = 0; t <= 100; t += 10) {
        auto ts_col = std::make_shared<Column>("ts", ColumnType::Int64);
        ts_col->appendInt64(t);
        ColumnBatch b(1); b.addColumn(ts_col);
        join.addBuildBatch(b);
    }
    EXPECT_EQ(join.buildSideSize(), 11u);

    // Probe at ts=10000 (far future). After this probe the buffer should be
    // pruned to only events >= 10000 - 500 = 9500 → all old events removed.
    auto probe_ts_col = std::make_shared<Column>("ts", ColumnType::Int64);
    probe_ts_col->appendInt64(10000);
    ColumnBatch probe(1); probe.addColumn(probe_ts_col);
    join.probe(probe);

    EXPECT_EQ(join.buildSideSize(), 0u) << "All old build events should have been pruned";
}

// ===========================================================================
// SJ-14  IntervalJoin: missing time_column config throws
// ===========================================================================
TEST(StreamingJoinTest, SJ14_IntervalJoinMissingTimeColumnThrows) {
    EXPECT_THROW(
        IntervalJoin join(IntervalJoin::Config{.join_keys = {}, .time_column = ""}),
        std::invalid_argument
    );
}

// ===========================================================================
// SJ-15  HashJoin: project build_select and probe_select
// ===========================================================================
TEST(StreamingJoinTest, SJ15_HashJoinSelectProjection) {
    HashJoin join(HashJoin::Config{
        .join_keys    = {"id"},
        .join_type    = JoinType::Inner,
        .build_select = {"id", "name"},     // only "id" and "name" from build
        .probe_select = {"id", "score"},    // only "id" and "score" from probe
    });

    join.addBuildBatch(makeIdNameBatch({{1, "alice"}, {2, "bob"}}));
    auto probe = makeIdScoreBatch({{1, 99}, {2, 88}});
    auto result = join.probe(probe);

    EXPECT_EQ(result.rowCount(), 2u);
    // "score" should be present (from probe_select).
    EXPECT_NE(result.getColumn("score"), nullptr);
    // "name" should be present (from build_select, non-key).
    EXPECT_NE(result.getColumn("name"), nullptr);
    // "id" should appear once (not duplicated).
    size_t id_count = 0;
    for (size_t i = 0; i < result.columnCount(); ++i) {
        if (result.getColumnAt(i)->name() == "id") {
          ++id_count;
        }
    }
    EXPECT_EQ(id_count, 1u);
}
