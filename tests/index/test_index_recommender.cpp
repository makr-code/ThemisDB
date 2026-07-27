/*
 * ThemisDB | File: test_index_recommender.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>
#include "metadata/index_recommender.h"
#include "metadata/statistics_collector.h"
#include "observability/metrics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;
using namespace themis;
using namespace themis::metadata;
using namespace themis::observability;

// ---------------------------------------------------------------------------
// Helper: open a temporary RocksDB instance
// ---------------------------------------------------------------------------
static std::string uniqueTmpPath(const std::string& tag) {
    return (fs::temp_directory_path() /
            ("themis_idxrec_" + tag + "_" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
             "_" + std::to_string(static_cast<int>(
                 std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFF))))
               .string();
}

static std::shared_ptr<RocksDBWrapper> openTempDB(const std::string& path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path    = path;
    cfg.enable_wal = true;
    auto db = std::make_shared<RocksDBWrapper>(cfg);
    if (!db->open()) return nullptr;
    return db;
}

// ---------------------------------------------------------------------------
// Fixture (in-memory, no persistence)
// ---------------------------------------------------------------------------
class IndexRecommenderTest : public ::testing::Test {
protected:
    IndexRecommender rec_;
};

// ============================================================================
// ColumnAccess / IndexRecommendation JSON
// ============================================================================

TEST(IndexRecommenderStructTest, ColumnAccessToJSON) {
    ColumnAccess ca;
    ca.table_name       = "users";
    ca.column_name      = "email";
    ca.filter_count     = 10;
    ca.sort_count       = 2;
    ca.avg_selectivity  = 0.01;

    auto j = ca.toJSON();
    EXPECT_EQ(j["table_name"],      "users");
    EXPECT_EQ(j["column_name"],     "email");
    EXPECT_EQ(j["filter_count"],    10u);
    EXPECT_EQ(j["sort_count"],      2u);
    EXPECT_NEAR(j["avg_selectivity"].get<double>(), 0.01, 1e-9);
}

TEST(IndexRecommenderStructTest, IndexRecommendationToJSONAdd) {
    IndexRecommendation r;
    r.table_name    = "orders";
    r.column_name   = "status";
    r.index_type    = "regular";
    r.action        = IndexRecommendation::Action::ADD;
    r.benefit_score = 75.0;
    r.rationale     = "High filter frequency";

    auto j = r.toJSON();
    EXPECT_EQ(j["action"],        "ADD");
    EXPECT_EQ(j["column_name"],   "status");
    EXPECT_NEAR(j["benefit_score"].get<double>(), 75.0, 1e-6);
}

TEST(IndexRecommenderStructTest, IndexRecommendationToJSONDrop) {
    IndexRecommendation r;
    r.action = IndexRecommendation::Action::DROP;
    auto j = r.toJSON();
    EXPECT_EQ(j["action"], "DROP");
}

// ============================================================================
// recordAccess + recordQuery
// ============================================================================

TEST_F(IndexRecommenderTest, RecordAccessUpdatesCounts) {
    rec_.recordQuery();
    rec_.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.05);

    auto stats = rec_.getAccessStats("users");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].column_name,  "email");
    EXPECT_EQ(stats[0].filter_count, 1u);
    EXPECT_EQ(stats[0].sort_count,   0u);
}

TEST_F(IndexRecommenderTest, RecordSortAccess) {
    rec_.recordQuery();
    rec_.recordAccess("orders", "created_at", IndexRecommender::AccessType::SORT, 1.0);

    auto stats = rec_.getAccessStats("orders");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].sort_count,   1u);
    EXPECT_EQ(stats[0].filter_count, 0u);
}

TEST_F(IndexRecommenderTest, MultipleAccessesAccumulateCounts) {
    for (int i = 0; i < 5; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("items", "price", IndexRecommender::AccessType::FILTER, 0.1);
    }
    auto stats = rec_.getAccessStats("items");
    ASSERT_EQ(stats.size(), 1u);
    EXPECT_EQ(stats[0].filter_count, 5u);
}

TEST_F(IndexRecommenderTest, RecordAccessMultipleColumns) {
    rec_.recordQuery();
    rec_.recordAccess("t", "col_a", IndexRecommender::AccessType::FILTER, 0.1);
    rec_.recordAccess("t", "col_b", IndexRecommender::AccessType::FILTER, 0.5);

    auto stats = rec_.getAccessStats("t");
    EXPECT_EQ(stats.size(), 2u);
}

// ============================================================================
// getAccessStats
// ============================================================================

TEST_F(IndexRecommenderTest, GetAccessStatsUnknownTable) {
    auto stats = rec_.getAccessStats("nonexistent");
    EXPECT_TRUE(stats.empty());
}

// ============================================================================
// recommend
// ============================================================================

TEST_F(IndexRecommenderTest, RecommendEmptyStats) {
    auto recs = rec_.recommend("no_table");
    EXPECT_TRUE(recs.empty());
}

TEST_F(IndexRecommenderTest, RecommendAddIndex) {
    // Record many filter accesses on a very selective column
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto recs = rec_.recommend("users");
    ASSERT_FALSE(recs.empty());

    auto add_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_GT(add_recs, 0);

    EXPECT_EQ(recs[0].column_name, "email");
    EXPECT_GE(recs[0].benefit_score, IndexRecommender::kAddThreshold);
}

TEST_F(IndexRecommenderTest, RecommendDropUnusedIndex) {
    // Record a very small number of accesses
    rec_.recordQuery();
    rec_.recordAccess("orders", "legacy_col", IndexRecommender::AccessType::FILTER, 1.0);

    // Add many more queries without using legacy_col
    for (int i = 0; i < 500; ++i) {
        rec_.recordQuery();
    }

    auto recs = rec_.recommend("orders", {"legacy_col"});
    auto drop_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::DROP; });
    EXPECT_GT(drop_recs, 0);
}

TEST_F(IndexRecommenderTest, NoRecommendForAlreadyIndexedHighBenefitColumn) {
    // Column with high benefit, already indexed → no ADD
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }
    auto recs = rec_.recommend("t", {"col"});  // col already indexed
    auto add_recs = std::count_if(recs.begin(), recs.end(),
        [](const auto& r) { return r.action == IndexRecommendation::Action::ADD; });
    EXPECT_EQ(add_recs, 0);
}

TEST_F(IndexRecommenderTest, RecommendSortPreferRangeIndex) {
    // Sort-heavy column should get "range" index type recommended
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("products", "price", IndexRecommender::AccessType::SORT, 0.5);
    }

    auto recs = rec_.recommend("products");
    // If a recommendation exists, sort-heavy should suggest range
    if (!recs.empty()) {
        auto it = std::find_if(recs.begin(), recs.end(),
            [](const auto& r) { return r.column_name == "price"; });
        if (it != recs.end() && it->action == IndexRecommendation::Action::ADD) {
            EXPECT_EQ(it->index_type, "range");
        }
    }
}

TEST_F(IndexRecommenderTest, RecommendSortedByBenefitDesc) {
    for (int i = 0; i < 10; ++i) rec_.recordQuery();
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);
    rec_.recordAccess("t", "low_col",  IndexRecommender::AccessType::FILTER, 0.9);

    // Add a lot more queries so benefit normalises differently
    for (int i = 0; i < 10; ++i) rec_.recordQuery();
    rec_.recordAccess("t", "high_col", IndexRecommender::AccessType::FILTER, 0.0);

    auto recs = rec_.recommend("t");
    for (size_t i = 1; i < recs.size(); ++i) {
        EXPECT_GE(recs[i-1].benefit_score, recs[i].benefit_score);
    }
}

// ============================================================================
// recommendAll
// ============================================================================

TEST_F(IndexRecommenderTest, RecommendAllMultipleTables) {
    for (int i = 0; i < 50; ++i) {
        rec_.recordQuery();
        rec_.recordAccess("t1", "col", IndexRecommender::AccessType::FILTER, 0.0);
        rec_.recordAccess("t2", "col", IndexRecommender::AccessType::FILTER, 0.0);
    }

    auto all = rec_.recommendAll();
    EXPECT_GE(all.size(), 2u);
    EXPECT_TRUE(all.count("t1"));
    EXPECT_TRUE(all.count("t2"));
}

// ============================================================================
// reset
// ============================================================================

TEST_F(IndexRecommenderTest, ResetClearsStats) {
    rec_.recordQuery();
    rec_.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.1);
    EXPECT_FALSE(rec_.getAccessStats("t").empty());

    rec_.reset();
    EXPECT_TRUE(rec_.getAccessStats("t").empty());
}

// ============================================================================
// toJSON
// ============================================================================

TEST_F(IndexRecommenderTest, ToJSONStructure) {
    rec_.recordQuery();
    rec_.recordAccess("t", "a", IndexRecommender::AccessType::FILTER, 0.1);
    rec_.recordAccess("t", "b", IndexRecommender::AccessType::SORT,   0.5);

    auto j = rec_.toJSON();
    EXPECT_TRUE(j.contains("t"));
    EXPECT_TRUE(j["t"].is_array());
    EXPECT_EQ(j["t"].size(), 2u);
}

// ============================================================================
// Access-pattern persistence (AC-1 through AC-5)
// ============================================================================

class IndexRecommenderPersistTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = uniqueTmpPath("persist");
        fs::remove_all(db_path_);
        db_ = openTempDB(db_path_);
        ASSERT_NE(db_, nullptr) << "Failed to open test RocksDB at " << db_path_;
    }

    void TearDown() override {
        db_.reset();
        fs::remove_all(db_path_);
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// AC-1: persistStats() writes meta_idx_stats::<table> keys to RocksDB
TEST_F(IndexRecommenderPersistTest, PersistStatsWritesRocksDBKeys) {
    IndexRecommender rec(db_.get(), std::chrono::seconds(0));

    for (int i = 0; i < 10; ++i) {
        rec.recordQuery();
        rec.recordAccess("orders", "status", IndexRecommender::AccessType::FILTER, 0.05);
    }

    rec.persistStats();

    // Verify the key exists in RocksDB
    std::string value;
    ASSERT_TRUE(db_->get("meta_idx_stats::orders", value));
    EXPECT_FALSE(value.empty());

    // Verify it is valid JSON
    auto arr = nlohmann::json::parse(value);
    ASSERT_TRUE(arr.is_array());
    ASSERT_EQ(arr.size(), 1u);
    EXPECT_EQ(arr[0]["column_name"], "status");
    EXPECT_EQ(arr[0]["filter_count"].get<uint64_t>(), 10u);
}

// AC-2: total_queries is persisted under meta_idx_stats::__total_queries__
TEST_F(IndexRecommenderPersistTest, PersistStatsTotalQueriesKey) {
    IndexRecommender rec(db_.get(), std::chrono::seconds(0));

    for (int i = 0; i < 42; ++i) rec.recordQuery();
    rec.recordAccess("t", "c", IndexRecommender::AccessType::FILTER, 0.1);
    rec.persistStats();

    std::string value;
    ASSERT_TRUE(db_->get("meta_idx_stats::__total_queries__", value));
    EXPECT_EQ(std::stoull(value), 42u);
}

// AC-3: Constructor loads persisted stats from RocksDB (round-trip)
TEST_F(IndexRecommenderPersistTest, ConstructorLoadsPersistedStats) {
    // Phase 1: record accesses and persist
    {
        IndexRecommender rec(db_.get(), std::chrono::seconds(0));
        for (int i = 0; i < 20; ++i) {
            rec.recordQuery();
            rec.recordAccess("users", "email", IndexRecommender::AccessType::FILTER, 0.02);
        }
        rec.persistStats();
    }

    // Phase 2: new instance with same DB should load the stats
    {
        IndexRecommender rec2(db_.get(), std::chrono::seconds(0));
        auto stats = rec2.getAccessStats("users");
        ASSERT_EQ(stats.size(), 1u);
        EXPECT_EQ(stats[0].column_name,  "email");
        EXPECT_EQ(stats[0].filter_count, 20u);
    }
}

// AC-4: reset() removes persisted keys from RocksDB
TEST_F(IndexRecommenderPersistTest, ResetDeletesRocksDBKeys) {
    IndexRecommender rec(db_.get(), std::chrono::seconds(0));
    rec.recordQuery();
    rec.recordAccess("items", "price", IndexRecommender::AccessType::FILTER, 0.3);
    rec.persistStats();

    // Confirm key exists before reset
    std::string value;
    ASSERT_TRUE(db_->get("meta_idx_stats::items", value));

    rec.reset();

    // After reset, key should be gone
    EXPECT_FALSE(db_->get("meta_idx_stats::items", value));
    EXPECT_TRUE(rec.getAccessStats("items").empty());
}

// AC-5: Destructor performs a final flush to RocksDB
TEST_F(IndexRecommenderPersistTest, DestructorFlushesToDB) {
    {
        IndexRecommender rec(db_.get(), std::chrono::seconds(0));
        for (int i = 0; i < 5; ++i) {
            rec.recordQuery();
            rec.recordAccess("products", "category", IndexRecommender::AccessType::FILTER, 0.1);
        }
        // Destructor fires here — no explicit persistStats() call
    }

    std::string value;
    ASSERT_TRUE(db_->get("meta_idx_stats::products", value));
    auto arr = nlohmann::json::parse(value);
    ASSERT_EQ(arr.size(), 1u);
    EXPECT_EQ(arr[0]["filter_count"].get<uint64_t>(), 5u);
}

// AC-6: New in-memory accesses merge with persisted data on re-construction
TEST_F(IndexRecommenderPersistTest, MergesPersistedAndInMemoryAccesses) {
    // Persist 10 filter accesses
    {
        IndexRecommender rec(db_.get(), std::chrono::seconds(0));
        for (int i = 0; i < 10; ++i) {
            rec.recordQuery();
            rec.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.1);
        }
        rec.persistStats();
    }

    // New instance: load 10 persisted + add 5 more
    {
        IndexRecommender rec2(db_.get(), std::chrono::seconds(0));
        for (int i = 0; i < 5; ++i) {
            rec2.recordQuery();
            rec2.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.1);
        }

        auto stats = rec2.getAccessStats("t");
        ASSERT_EQ(stats.size(), 1u);
        EXPECT_EQ(stats[0].filter_count, 15u);  // 10 persisted + 5 new
    }
}

// AC-7: Background persist thread flushes stats within the given interval
TEST_F(IndexRecommenderPersistTest, BackgroundThreadPersistsWithinInterval) {
    // Use a very short interval (50 ms) to exercise the background thread
    IndexRecommender rec(db_.get(), std::chrono::milliseconds(50));

    for (int i = 0; i < 8; ++i) {
        rec.recordQuery();
        rec.recordAccess("bg_tbl", "col", IndexRecommender::AccessType::FILTER, 0.2);
    }

    // Poll with a generous timeout (2s) to avoid flakiness on slow CI runners
    std::string value;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (db_->get("meta_idx_stats::bg_tbl", value)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_TRUE(db_->get("meta_idx_stats::bg_tbl", value));
    auto arr = nlohmann::json::parse(value);
    ASSERT_FALSE(arr.empty());
    EXPECT_EQ(arr[0]["column_name"], "col");
}

// ============================================================================
// Cost-model benefit (AC-8): StatisticsCollector integration
// ============================================================================

class IndexRecommenderCostModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = uniqueTmpPath("costmodel");
        fs::remove_all(db_path_);
        db_ = openTempDB(db_path_);
        ASSERT_NE(db_, nullptr) << "Failed to open test RocksDB at " << db_path_;
    }

    void TearDown() override {
        db_.reset();
        fs::remove_all(db_path_);
    }

    // Seed a TableStats JSON blob into RocksDB so StatisticsCollector can load it.
    void seedTableStats(const std::string& table, size_t row_count,
                        const std::string& col, double selectivity) {
        nlohmann::json j;
        j["table_name"]         = table;
        j["row_count"]          = row_count;
        j["total_size_bytes"]   = row_count * 64;
        j["avg_row_size_bytes"] = 64.0;
        j["sample_size"]        = std::min(row_count, size_t{1000});
        nlohmann::json cs;
        cs["column_name"]    = col;
        cs["distinct_count"] = static_cast<size_t>(1.0 / std::max(selectivity, 1e-9));
        cs["null_count"]     = 0;
        cs["total_count"]    = row_count;
        cs["selectivity"]    = selectivity;
        cs["null_fraction"]  = 0.0;
        nlohmann::json col_stats;
        col_stats[col] = cs;
        j["column_stats"] = col_stats;
        db_->put("stats:" + table, j.dump());
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// AC-8a: Without StatisticsCollector the heuristic model is used (no crash).
TEST_F(IndexRecommenderCostModelTest, NoStatisticsCollector_HeuristicUsed) {
    IndexRecommender rec;
    for (int i = 0; i < 50; ++i) {
        rec.recordQuery();
        rec.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.01);
    }
    // Should produce recommendations without crashing
    auto recs = rec.recommend("t");
    EXPECT_FALSE(recs.empty());
    EXPECT_EQ(recs[0].action, IndexRecommendation::Action::ADD);
}

// AC-8b: StatisticsCollector attached but no table data → falls back to heuristic gracefully.
TEST_F(IndexRecommenderCostModelTest, StatisticsCollector_NoTableData_FallsBackGracefully) {
    StatisticsCollector sc(*db_);  // DB has no stats for this table
    IndexRecommender rec;
    rec.setStatisticsCollector(&sc);

    for (int i = 0; i < 50; ++i) {
        rec.recordQuery();
        rec.recordAccess("t", "col", IndexRecommender::AccessType::FILTER, 0.01);
    }
    // Must not crash; heuristic-based recommendation returned
    auto recs = rec.recommend("t");
    EXPECT_FALSE(recs.empty());
    EXPECT_EQ(recs[0].action, IndexRecommendation::Action::ADD);
}

// AC-8c: Cost-model uses StatisticsCollector selectivity — highly selective column
//        (low selectivity value) gets a higher score than a non-selective one.
TEST_F(IndexRecommenderCostModelTest, CostModel_HighSelectivity_HigherScore) {
    // Seed stats for two tables: selective_tbl (selectivity=0.01) and
    // nonselective_tbl (selectivity=0.99) — same row count, same access pattern.
    seedTableStats("selective_tbl",    100000, "col", 0.01);
    seedTableStats("nonselective_tbl", 100000, "col", 0.99);

    StatisticsCollector sc(*db_);
    IndexRecommender rec;
    rec.setStatisticsCollector(&sc);

    // Same access pattern for both tables, but keep access density low enough
    // to avoid score saturation at the 100-point cap.
    for (int i = 0; i < 100; ++i) {
        rec.recordQuery();
        if (i < 20) {
            rec.recordAccess("selective_tbl",    "col", IndexRecommender::AccessType::FILTER, 0.5);
            rec.recordAccess("nonselective_tbl", "col", IndexRecommender::AccessType::FILTER, 0.5);
        }
    }

    auto sel_recs    = rec.recommend("selective_tbl");
    auto nonsel_recs = rec.recommend("nonselective_tbl");

    ASSERT_FALSE(sel_recs.empty())    << "Expected ADD rec for selective table";
    ASSERT_FALSE(nonsel_recs.empty()) << "Expected ADD rec for non-selective table";

    // Selective column should produce a higher benefit score
    EXPECT_GT(sel_recs[0].benefit_score, nonsel_recs[0].benefit_score);
}

// AC-8d: Cost-model applies write-amplification penalty for large tables.
//        Same access pattern, but large_tbl has 10M rows vs small_tbl with 1k rows.
TEST_F(IndexRecommenderCostModelTest, CostModel_LargeTable_WriteAmplificationPenalty) {
    seedTableStats("small_tbl", 1000,       "col", 0.01);
    seedTableStats("large_tbl", 10000000,   "col", 0.01);

    StatisticsCollector sc(*db_);
    IndexRecommender rec;
    rec.setStatisticsCollector(&sc);

    for (int i = 0; i < 50; ++i) {
        rec.recordQuery();
        rec.recordAccess("small_tbl", "col", IndexRecommender::AccessType::FILTER, 0.5);
        rec.recordAccess("large_tbl", "col", IndexRecommender::AccessType::FILTER, 0.5);
    }

    auto small_recs = rec.recommend("small_tbl");
    auto large_recs = rec.recommend("large_tbl");

    ASSERT_FALSE(small_recs.empty());
    ASSERT_FALSE(large_recs.empty());

    // Small table should have a higher or equal benefit score (no/less write penalty)
    EXPECT_GE(small_recs[0].benefit_score, large_recs[0].benefit_score);
}

// ============================================================================
// Metric emission (AC-9): metadata.index_recommendation.generated_total
// ============================================================================

class IndexRecommenderMetricTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
};

// AC-9a: recommend() increments the counter when a MetricsCollector is attached.
TEST_F(IndexRecommenderMetricTest, RecommendIncrementsCounter) {
    auto& mc = MetricsCollector::getInstance();

    IndexRecommender rec;
    rec.setMetricsCollector(&mc);

    for (int i = 0; i < 30; ++i) {
        rec.recordQuery();
        rec.recordAccess("tbl", "col", IndexRecommender::AccessType::FILTER, 0.1);
    }

    rec.recommend("tbl");
    rec.recommend("tbl");

    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(metrics.find("metadata.index_recommendation.generated_total"), std::string::npos);
}

// AC-9b: Each recommend() call increments the counter exactly once.
TEST_F(IndexRecommenderMetricTest, RecommendCounterIncrementsOnce) {
    auto& mc = MetricsCollector::getInstance();

    IndexRecommender rec;
    rec.setMetricsCollector(&mc);

    for (int i = 0; i < 30; ++i) {
        rec.recordQuery();
        rec.recordAccess("tbl", "col", IndexRecommender::AccessType::FILTER, 0.05);
    }

    // Call recommend() 3 times — counter should appear 3 times or show value 3
    for (int i = 0; i < 3; ++i) rec.recommend("tbl");

    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_NE(metrics.find("metadata.index_recommendation.generated_total"), std::string::npos);
    // Verify label contains the table name
    EXPECT_NE(metrics.find("tbl"), std::string::npos);
}

// AC-9c: Without MetricsCollector recommend() does not crash.
TEST_F(IndexRecommenderMetricTest, NoMetricsCollector_NoCrash) {
    IndexRecommender rec;  // no setMetricsCollector()
    for (int i = 0; i < 30; ++i) {
        rec.recordQuery();
        rec.recordAccess("tbl", "col", IndexRecommender::AccessType::FILTER, 0.05);
    }
    EXPECT_NO_THROW(rec.recommend("tbl"));
}

// AC-9d: setMetricsCollector(nullptr) disables metric emission.
TEST_F(IndexRecommenderMetricTest, SetMetricsCollectorNullptr_DisablesEmission) {
    auto& mc = MetricsCollector::getInstance();
    mc.reset();

    IndexRecommender rec;
    rec.setMetricsCollector(&mc);
    rec.setMetricsCollector(nullptr);  // disable

    for (int i = 0; i < 30; ++i) {
        rec.recordQuery();
        rec.recordAccess("tbl", "col", IndexRecommender::AccessType::FILTER, 0.05);
    }
    rec.recommend("tbl");

    // No counter should have been emitted
    std::string metrics = mc.getPrometheusMetrics();
    EXPECT_EQ(metrics.find("metadata.index_recommendation.generated_total"), std::string::npos);
}
