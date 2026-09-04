// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>

#include "metadata/statistics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;

// Helper to create a unique temporary database path
static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class StatisticsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_stats_");
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
    }

    void TearDown() override {
        if (db_) {
          db_->close();
        }
    }

    // Insert a row into table `table_name` with key `row_id`
    void insertRow(const std::string& table_name,
                   const std::string& row_id,
                   BaseEntity::FieldMap fields)
    {
        BaseEntity entity = BaseEntity::fromFields(row_id, fields);
        db_->put(table_name + ":" + row_id, entity.serialize());
    }

    std::unique_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// Empty database
// ============================================================================

TEST_F(StatisticsCollectorTest, CollectStatsEmptyTable) {
    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("nodata");

    // Empty table: collection should succeed but report 0 rows
    ASSERT_TRUE(result.ok) << "collectStats should succeed even for empty table";
    EXPECT_EQ(result.value.table_name, "nodata");
    EXPECT_EQ(result.value.row_count, 0u);
    EXPECT_EQ(result.value.column_stats.size(), 0u);
}

TEST_F(StatisticsCollectorTest, CollectStatsEmptyTableName) {
    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

// ============================================================================
// Basic stats collection
// ============================================================================

TEST_F(StatisticsCollectorTest, CollectStatsRowCount) {
    for (int i = 0; i < 5; ++i) {
        insertRow("items", "item" + std::to_string(i), {
            {"value", int64_t(i * 10)}
        });
    }

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("items");

    ASSERT_TRUE(result.ok);
    EXPECT_EQ(result.value.row_count, 5u);
    EXPECT_EQ(result.value.table_name, "items");
}

TEST_F(StatisticsCollectorTest, CollectStatsColumnStats) {
    insertRow("users", "u1", {{"age", int64_t(20)}, {"name", std::string("Alice")}});
    insertRow("users", "u2", {{"age", int64_t(30)}, {"name", std::string("Bob")}});
    insertRow("users", "u3", {{"age", int64_t(20)}, {"name", std::string("Carol")}});

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("users", 100);

    ASSERT_TRUE(result.ok);
    ASSERT_GT(result.value.column_stats.count("age"), 0u);
    ASSERT_GT(result.value.column_stats.count("name"), 0u);

    const ColumnStats& age_stats = result.value.column_stats.at("age");
    EXPECT_EQ(age_stats.column_name, "age");
    EXPECT_GE(age_stats.total_count, 3u);
    // 2 distinct values: 20 and 30
    EXPECT_EQ(age_stats.distinct_count, 2u);
    EXPECT_EQ(age_stats.null_count, 0u);
    EXPECT_DOUBLE_EQ(age_stats.null_fraction, 0.0);
    // Selectivity = 1 / distinct_count
    EXPECT_NEAR(age_stats.selectivity, 0.5, 1e-6);

    const ColumnStats& name_stats = result.value.column_stats.at("name");
    EXPECT_EQ(name_stats.distinct_count, 3u);
}

// ============================================================================
// Histogram
// ============================================================================

TEST_F(StatisticsCollectorTest, HistogramBuiltForNumericColumn) {
    for (int i = 0; i < 10; ++i) {
        insertRow("nums", "n" + std::to_string(i), {{"val", double(i)}});
    }

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("nums", 100);

    ASSERT_TRUE(result.ok);
    ASSERT_GT(result.value.column_stats.count("val"), 0u);

    const ColumnStats& cs = result.value.column_stats.at("val");
    EXPECT_TRUE(cs.histogram.has_value())   << "Histogram should be built for numeric column";
    EXPECT_FALSE(cs.histogram->empty());
    EXPECT_TRUE(cs.min_value.has_value());
    EXPECT_TRUE(cs.max_value.has_value());
    EXPECT_NEAR(*cs.min_value, 0.0, 1e-6);
    EXPECT_NEAR(*cs.max_value, 9.0, 1e-6);
}

// ============================================================================
// getStats – cache and persistence
// ============================================================================

TEST_F(StatisticsCollectorTest, GetStatsCacheHit) {
    insertRow("cache_test", "r1", {{"x", int64_t(1)}});

    StatisticsCollector sc(*db_);
    auto r1 = sc.collectStats("cache_test");
    ASSERT_TRUE(r1.ok);

    // Second call should return from in-memory cache
    auto r2 = sc.getStats("cache_test");
    ASSERT_TRUE(r2.ok);
    EXPECT_EQ(r2.value.table_name, "cache_test");
    EXPECT_EQ(r2.value.row_count, r1.value.row_count);
}

TEST_F(StatisticsCollectorTest, GetStatsNotFound) {
    StatisticsCollector sc(*db_);
    auto result = sc.getStats("nonexistent_table_xyz");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, GetStatsEmptyName) {
    StatisticsCollector sc(*db_);
    auto result = sc.getStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

// ============================================================================
// updateStats
// ============================================================================

TEST_F(StatisticsCollectorTest, UpdateStats) {
    insertRow("orders", "o1", {{"amount", double(9.99)}});

    StatisticsCollector sc(*db_);
    auto result = sc.updateStats("orders");
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.value);

    // Verify stats are now retrievable
    auto get_result = sc.getStats("orders");
    ASSERT_TRUE(get_result.ok);
    EXPECT_EQ(get_result.value.table_name, "orders");
}

// ============================================================================
// clearStats
// ============================================================================

TEST_F(StatisticsCollectorTest, ClearStats) {
    insertRow("products", "p1", {{"price", double(5.0)}});

    StatisticsCollector sc(*db_);
    ASSERT_TRUE(sc.collectStats("products").ok);

    auto clear_result = sc.clearStats("products");
    EXPECT_TRUE(clear_result.ok);

    // After clearing, getStats should fail (not in cache, not in DB)
    auto get_result = sc.getStats("products");
    EXPECT_FALSE(get_result.ok);
}

TEST_F(StatisticsCollectorTest, ClearStatsEmptyName) {
    StatisticsCollector sc(*db_);
    auto result = sc.clearStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

// ============================================================================
// JSON serialisation
// ============================================================================

TEST_F(StatisticsCollectorTest, ToJSON) {
    insertRow("json_test", "j1", {{"score", int64_t(42)}});

    StatisticsCollector sc(*db_);
    ASSERT_TRUE(sc.collectStats("json_test").ok);

    auto j = sc.toJSON();
    EXPECT_TRUE(j.contains("json_test"));
    EXPECT_TRUE(j["json_test"].contains("row_count"));
    EXPECT_TRUE(j["json_test"].contains("table_name"));
}

TEST_F(StatisticsCollectorTest, TableStatsToJSON) {
    insertRow("stats_json", "s1", {{"v", int64_t(1)}});

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("stats_json");
    ASSERT_TRUE(result.ok);

    auto j = result.value.toJSON();
    EXPECT_EQ(j["table_name"], "stats_json");
    EXPECT_TRUE(j.contains("row_count"));
    EXPECT_TRUE(j.contains("column_stats"));
    EXPECT_TRUE(j.contains("last_updated"));
}

TEST_F(StatisticsCollectorTest, HistogramBucketToJSON) {
    HistogramBucket b;
    b.lower_bound = 0.0;
    b.upper_bound = 10.0;
    b.frequency   = 5;

    auto j = b.toJSON();
    EXPECT_EQ(j["lower_bound"], 0.0);
    EXPECT_EQ(j["upper_bound"], 10.0);
    EXPECT_EQ(j["frequency"],   5u);
}

// ============================================================================
// Index statistics export
// ============================================================================

TEST_F(StatisticsCollectorTest, ImportIndexStatsBasic) {
    StatisticsCollector sc(*db_);

    IndexStats s1;
    s1.table                = "users";
    s1.column               = "email";
    s1.type                 = "regular";
    s1.entry_count          = 42;
    s1.estimated_size_bytes = 4200;
    s1.unique               = true;
    s1.additional_info      = "unique";

    auto result = sc.importIndexStats("users", {s1});
    ASSERT_TRUE(result.ok);
    EXPECT_TRUE(result.value);
}

TEST_F(StatisticsCollectorTest, GetIndexStatsAfterImport) {
    StatisticsCollector sc(*db_);

    IndexStats s1;
    s1.table       = "products";
    s1.column      = "price";
    s1.type        = "range";
    s1.entry_count = 100;
    s1.unique      = false;
    s1.additional_info = "sorted";

    IndexStats s2;
    s2.table       = "products";
    s2.column      = "name";
    s2.type        = "regular";
    s2.entry_count = 100;

    ASSERT_TRUE(sc.importIndexStats("products", {s1, s2}).ok);

    auto result = sc.getIndexStats("products");
    ASSERT_TRUE(result.ok);
    ASSERT_EQ(result.value.size(), 2u);

    const auto& r0 = result.value[0];
    EXPECT_EQ(r0.table,  "products");
    EXPECT_EQ(r0.column, "price");
    EXPECT_EQ(r0.type,   "range");
    EXPECT_EQ(r0.entry_count, 100u);
    EXPECT_FALSE(r0.unique);
    EXPECT_EQ(r0.additional_info, "sorted");
}

TEST_F(StatisticsCollectorTest, GetIndexStatsMissingTable) {
    StatisticsCollector sc(*db_);
    auto result = sc.getIndexStats("nonexistent_xyz");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, GetIndexStatsEmptyName) {
    StatisticsCollector sc(*db_);
    auto result = sc.getIndexStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, ImportIndexStatsEmptyName) {
    StatisticsCollector sc(*db_);
    auto result = sc.importIndexStats("", {});
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, ClearIndexStats) {
    StatisticsCollector sc(*db_);

    IndexStats s;
    s.table        = "orders";
    s.column       = "status";
    s.type         = "regular";
    s.entry_count  = 10;

    ASSERT_TRUE(sc.importIndexStats("orders", {s}).ok);
    ASSERT_TRUE(sc.getIndexStats("orders").ok);

    auto clear_result = sc.clearIndexStats("orders");
    EXPECT_TRUE(clear_result.ok);

    auto get_result = sc.getIndexStats("orders");
    EXPECT_FALSE(get_result.ok);
    EXPECT_EQ(get_result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, ClearIndexStatsEmptyName) {
    StatisticsCollector sc(*db_);
    auto result = sc.clearIndexStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
}

TEST_F(StatisticsCollectorTest, IndexStatsPersistenceAcrossInstances) {
    // Import via first instance
    {
        StatisticsCollector sc(*db_);
        IndexStats s;
        s.table        = "sessions";
        s.column       = "user_id";
        s.type         = "regular";
        s.entry_count  = 25;
        s.unique       = false;
        ASSERT_TRUE(sc.importIndexStats("sessions", {s}).ok);
    }

    // Load via second instance (hits RocksDB, not in-memory cache)
    {
        StatisticsCollector sc2(*db_);
        auto result = sc2.getIndexStats("sessions");
        ASSERT_TRUE(result.ok);
        ASSERT_EQ(result.value.size(), 1u);
        EXPECT_EQ(result.value[0].column, "user_id");
        EXPECT_EQ(result.value[0].entry_count, 25u);
    }
}

TEST_F(StatisticsCollectorTest, IndexStatsToJSON) {
    StatisticsCollector sc(*db_);

    IndexStats s;
    s.table                = "items";
    s.column               = "category";
    s.type                 = "regular";
    s.entry_count          = 50;
    s.estimated_size_bytes = 5000;
    s.unique               = false;
    s.additional_info      = "";

    auto j = s.toJSON();
    EXPECT_EQ(j["table"],                "items");
    EXPECT_EQ(j["column"],               "category");
    EXPECT_EQ(j["type"],                 "regular");
    EXPECT_EQ(j["entry_count"],          50u);
    EXPECT_EQ(j["estimated_size_bytes"], 5000u);
    EXPECT_EQ(j["unique"],               false);
    EXPECT_TRUE(j.contains("last_updated"));
}

TEST_F(StatisticsCollectorTest, ImportIndexStatsEmptyVector) {
    StatisticsCollector sc(*db_);
    // Importing an empty list is valid – replaces any existing stats
    auto result = sc.importIndexStats("mytable", {});
    EXPECT_TRUE(result.ok);

    auto get_result = sc.getIndexStats("mytable");
    ASSERT_TRUE(get_result.ok);
    EXPECT_TRUE(get_result.value.empty());
}

TEST_F(StatisticsCollectorTest, ToJSONIncludesIndexStats) {
    StatisticsCollector sc(*db_);

    IndexStats s;
    s.table        = "orders";
    s.column       = "amount";
    s.type         = "range";
    s.entry_count  = 77;
    s.unique       = false;

    ASSERT_TRUE(sc.importIndexStats("orders", {s}).ok);

    auto j = sc.toJSON();
    ASSERT_TRUE(j.contains("index_stats"))  << "toJSON() must include 'index_stats' key when index stats are cached";
    ASSERT_TRUE(j["index_stats"].contains("orders"));
    ASSERT_FALSE(j["index_stats"]["orders"].empty());
    EXPECT_EQ(j["index_stats"]["orders"][0]["column"], "amount");
    EXPECT_EQ(j["index_stats"]["orders"][0]["entry_count"], 77u);
}

// ============================================================================
// Histogram persistence (loadStats must round-trip histogram buckets)
// ============================================================================

TEST_F(StatisticsCollectorTest, HistogramPersistedAndRestoredAcrossInstances) {
    // Insert numeric data so a histogram is built
    for (int i = 0; i < 20; ++i) {
        insertRow("hist_persist", "r" + std::to_string(i), {{"score", double(i)}});
    }

    // Collect and persist via first instance
    {
        StatisticsCollector sc(*db_);
        auto result = sc.collectStats("hist_persist", 100);
        ASSERT_TRUE(result.ok);
        ASSERT_GT(result.value.column_stats.count("score"), 0u);
        EXPECT_TRUE(result.value.column_stats.at("score").histogram.has_value())
            << "Histogram should be built during collection";
    }

    // Load via second instance (hits RocksDB, not in-memory cache)
    {
        StatisticsCollector sc2(*db_);
        auto result = sc2.getStats("hist_persist");
        ASSERT_TRUE(result.ok);
        ASSERT_GT(result.value.column_stats.count("score"), 0u);
        const ColumnStats& cs = result.value.column_stats.at("score");
        EXPECT_TRUE(cs.histogram.has_value())
            << "Histogram must survive persistence round-trip";
        EXPECT_FALSE(cs.histogram->empty())
            << "Loaded histogram must not be empty";
        // Verify bucket contents are sane
        for (const auto& bucket : *cs.histogram) {
            EXPECT_LE(bucket.lower_bound, bucket.upper_bound);
            EXPECT_GT(bucket.frequency, 0u);
        }
    }
}

// ============================================================================
// Equi-height histogram: approximately equal frequency per bucket
// ============================================================================

TEST_F(StatisticsCollectorTest, EquiHeightHistogramEqualFrequencies) {
    // Insert 20 uniformly distributed values
    for (int i = 0; i < 20; ++i) {
        insertRow("equi", "e" + std::to_string(i), {{"val", double(i)}});
    }

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("equi", 100);
    ASSERT_TRUE(result.ok);
    ASSERT_GT(result.value.column_stats.count("val"), 0u);

    const ColumnStats& cs = result.value.column_stats.at("val");
    ASSERT_TRUE(cs.histogram.has_value());
    const auto& buckets = *cs.histogram;
    ASSERT_FALSE(buckets.empty());

    // Total frequency must equal the sample count
    size_t total = 0;
    for (const auto& b : buckets) {
        total += b.frequency;
    }
    EXPECT_EQ(total, 20u);

    // For equi-height each bucket should have roughly equal frequency
    // With 20 uniform values the max/min frequency ratio should be <= 2.
    size_t max_freq = 0, min_freq = SIZE_MAX;
    for (const auto& b : buckets) {
        max_freq = std::max(max_freq, b.frequency);
        min_freq = std::min(min_freq, b.frequency);
    }
    EXPECT_LE(static_cast<double>(max_freq) / static_cast<double>(min_freq), 2.0)
        << "Equi-height histogram frequencies should be approximately equal";
}

TEST_F(StatisticsCollectorTest, EquiHeightHistogramSkewedData) {
    // Insert skewed data: 15 identical zeros, then 5 distinct values 1..5
    for (int i = 0; i < 15; ++i) {
        insertRow("skew", "s0_" + std::to_string(i), {{"v", double(0)}});
    }
    for (int i = 1; i <= 5; ++i) {
        insertRow("skew", "s" + std::to_string(i), {{"v", double(i)}});
    }

    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("skew", 100);
    ASSERT_TRUE(result.ok);
    ASSERT_GT(result.value.column_stats.count("v"), 0u);

    const ColumnStats& cs = result.value.column_stats.at("v");
    ASSERT_TRUE(cs.histogram.has_value());
    const auto& buckets = *cs.histogram;

    // Total frequency = 20
    size_t total = 0;
    for (const auto& b : buckets) {
      total += b.frequency;
    }
    EXPECT_EQ(total, 20u);

    // Equi-height cannot split identical values: all 15 zeros must end up in a
    // single bucket.  The maximum bucket frequency should therefore be exactly 15.
    // (An equi-width histogram over [0,5] with 20 buckets would also yield one
    // large bucket but with a different bucket width; here we verify the
    // grouping-of-equals property of the equi-height implementation.)
    size_t max_freq = 0;
    for (const auto& b : buckets) {
      max_freq = std::max(max_freq, b.frequency);
    }
    EXPECT_EQ(max_freq, 15u)
        << "All 15 identical zeros must be grouped into a single bucket";
}

// ============================================================================
// Range selectivity estimation via histogram
// ============================================================================

TEST_F(StatisticsCollectorTest, RangeSelectivityFullRange) {
    for (int i = 0; i < 10; ++i) {
        insertRow("sel_full", "r" + std::to_string(i), {{"v", double(i)}});
    }
    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("sel_full", 100);
    ASSERT_TRUE(result.ok);
    const ColumnStats& cs = result.value.column_stats.at("v");

    // Full range should return ~1.0
    double sel = cs.estimateRangeSelectivity(-1.0, 100.0);
    EXPECT_NEAR(sel, 1.0, 1e-6);
}

TEST_F(StatisticsCollectorTest, RangeSelectivityEmpty) {
    for (int i = 0; i < 10; ++i) {
        insertRow("sel_empty", "r" + std::to_string(i), {{"v", double(i)}});
    }
    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("sel_empty", 100);
    ASSERT_TRUE(result.ok);
    const ColumnStats& cs = result.value.column_stats.at("v");

    // Range completely outside data range should return 0
    double sel = cs.estimateRangeSelectivity(100.0, 200.0);
    EXPECT_NEAR(sel, 0.0, 1e-6);
}

TEST_F(StatisticsCollectorTest, RangeSelectivityPartial) {
    // 10 uniformly-spaced values 0..9; the lower half [0,5) covers 5 of 10 values.
    // With equi-height histograms, each bucket covers ~equal number of values, so
    // the estimate for [0,5) should be close to 50% (±15% tolerance for bucket
    // boundary effects when num_buckets > num_values/2).
    for (int i = 0; i < 10; ++i) {
        insertRow("sel_partial", "r" + std::to_string(i), {{"v", double(i)}});
    }
    StatisticsCollector sc(*db_);
    auto result = sc.collectStats("sel_partial", 100);
    ASSERT_TRUE(result.ok);
    const ColumnStats& cs = result.value.column_stats.at("v");

    double sel = cs.estimateRangeSelectivity(0.0, 5.0);
    EXPECT_NEAR(sel, 0.5, 0.15)
        << "Half-range selectivity should be ~50%; got " << sel;
}

// ============================================================================
// Metrics hook callbacks
// ============================================================================

/// Test double that records every callback invocation.
struct RecordingHook : public StatisticsCollector::IMetricsHook {
    std::atomic<int> collect_ok{0};
    std::atomic<int> collect_fail{0};
    std::atomic<int> cache_hit{0};
    std::atomic<int> cache_miss{0};
    std::atomic<int> error_count{0};

    void onCollect(std::string_view, double, size_t, bool success) override {
        if (success) {
          ++collect_ok; else ++collect_fail;
        }
    }
    void onCacheHit(std::string_view) override  { ++cache_hit;    }
    void onCacheMiss(std::string_view) override { ++cache_miss;   }
    void onError(std::string_view, int) override { ++error_count; }
};

TEST_F(StatisticsCollectorTest, MetricsHook_OnCollect_CalledAfterCollectStats) {
    insertRow("mh_collect", "r1", {{"x", int64_t(1)}});

    StatisticsCollector sc(*db_);
    RecordingHook hook;
    sc.setMetricsHook(&hook);

    auto result = sc.collectStats("mh_collect");
    ASSERT_TRUE(result.ok);

    EXPECT_EQ(hook.collect_ok.load(),   1);
    EXPECT_EQ(hook.collect_fail.load(), 0);

    sc.setMetricsHook(nullptr);
}

TEST_F(StatisticsCollectorTest, MetricsHook_OnError_CalledForEmptyTableName) {
    StatisticsCollector sc(*db_);
    RecordingHook hook;
    sc.setMetricsHook(&hook);

    auto result = sc.collectStats("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, StatsErrorCode::TABLE_NOT_FOUND);
    EXPECT_GE(hook.error_count.load(), 1);

    sc.setMetricsHook(nullptr);
}

TEST_F(StatisticsCollectorTest, MetricsHook_OnCacheHit_CalledOnSecondGetStats) {
    insertRow("mh_hit", "r1", {{"y", int64_t(42)}});

    StatisticsCollector sc(*db_);
    ASSERT_TRUE(sc.collectStats("mh_hit").ok);

    RecordingHook hook;
    sc.setMetricsHook(&hook);

    // First call: in-memory cache is already populated → cache hit
    auto r1 = sc.getStats("mh_hit");
    ASSERT_TRUE(r1.ok);
    EXPECT_EQ(hook.cache_hit.load(), 1);
    EXPECT_EQ(hook.cache_miss.load(), 0);

    // Second call: still a cache hit
    auto r2 = sc.getStats("mh_hit");
    ASSERT_TRUE(r2.ok);
    EXPECT_EQ(hook.cache_hit.load(), 2);

    sc.setMetricsHook(nullptr);
}

TEST_F(StatisticsCollectorTest, MetricsHook_OnCacheMiss_CalledWhenCacheEmpty) {
    insertRow("mh_miss", "r1", {{"z", int64_t(7)}});

    {
        // Persist stats with one collector instance
        StatisticsCollector sc(*db_);
        ASSERT_TRUE(sc.collectStats("mh_miss").ok);
    }

    // New collector instance: in-memory cache is empty → cache miss, loads from RocksDB
    StatisticsCollector sc2(*db_);
    RecordingHook hook;
    sc2.setMetricsHook(&hook);

    auto r = sc2.getStats("mh_miss");
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(hook.cache_miss.load(), 1);
    EXPECT_EQ(hook.cache_hit.load(),  0);

    sc2.setMetricsHook(nullptr);
}
