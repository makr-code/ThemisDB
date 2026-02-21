/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_statistics_collector.cpp                      ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     254                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
        if (db_) db_->close();
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
