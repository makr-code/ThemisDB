/**
 * Tests for ContinuousAggMaterializationEngine
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "timeseries/continuous_agg.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <memory>
#include <algorithm>

using namespace themis;
namespace fs = std::filesystem;

// ============================================================================
// Shared test fixture
// ============================================================================

static std::string makeMaterialTempPath() {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() /
            ("themis_mat_" + std::to_string(ns))).string();
}

struct MaterializationFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore>        store;
    // base timestamp: 2023-11-15 00:00:00 UTC in ms
    int64_t base_ms{1700000000000LL};

    void SetUp() override {
        db_path = makeMaterialTempPath();
        RocksDBWrapper::Config cfg;
        cfg.db_path      = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed: " << db_path;
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    // Insert n evenly-spaced data points starting at base_ms
    void insertPoints(const std::string& metric, const std::string& entity,
                      int n, double start_val = 1.0, double step_val = 1.0,
                      int64_t step_ms = 10000LL) {
        for (int i = 0; i < n; ++i) {
            TSStore::DataPoint p;
            p.metric       = metric;
            p.entity       = entity;
            p.timestamp_ms = base_ms + i * step_ms;
            p.value        = start_val + i * step_val;
            ASSERT_TRUE(store->putDataPoint(p).has_value());
        }
    }

    // Build a standard AggConfig
    static AggConfig makeConfig(const std::string& metric,
                                const std::string& entity,
                                std::chrono::milliseconds window) {
        AggConfig cfg;
        cfg.metric       = metric;
        cfg.entity       = entity;
        cfg.window.size  = window;
        return cfg;
    }
};

// ============================================================================
// createAggregate / listAggregates / getAggregate
// ============================================================================

TEST_F(MaterializationFixture, CreateAggregate_Success_ReturnsTrueAndListed) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "cpu_1min";
    def.config = makeConfig("cpu", "server1", std::chrono::minutes(1));

    EXPECT_TRUE(engine.createAggregate(def));
    auto names = engine.listAggregates();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "cpu_1min");
}

TEST_F(MaterializationFixture, CreateAggregate_DuplicateName_ReturnsFalse) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "cpu_1min";
    def.config = makeConfig("cpu", "s1", std::chrono::minutes(1));

    EXPECT_TRUE(engine.createAggregate(def));
    EXPECT_FALSE(engine.createAggregate(def)); // duplicate
    EXPECT_EQ(engine.listAggregates().size(), 1u);
}

TEST_F(MaterializationFixture, GetAggregate_ExistingName_ReturnsDefinition) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "mem_5min";
    def.config = makeConfig("mem", "host1", std::chrono::minutes(5));

    engine.createAggregate(def);

    auto got = engine.getAggregate("mem_5min");
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->name, "mem_5min");
    EXPECT_EQ(got->config.metric, "mem");
    EXPECT_FALSE(got->agg_id.empty()); // auto-populated
}

TEST_F(MaterializationFixture, GetAggregate_UnknownName_ReturnsNullopt) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_FALSE(engine.getAggregate("nonexistent").has_value());
}

TEST_F(MaterializationFixture, ListAggregates_PreservesInsertionOrder) {
    ContinuousAggMaterializationEngine engine(store.get());

    for (const auto& name : {"a", "b", "c", "d"}) {
        ContinuousAggDefinition d;
        d.name   = name;
        d.config = makeConfig(name, "e", std::chrono::minutes(1));
        engine.createAggregate(d);
    }

    auto names = engine.listAggregates();
    ASSERT_EQ(names.size(), 4u);
    EXPECT_EQ(names[0], "a");
    EXPECT_EQ(names[3], "d");
}

// ============================================================================
// dropAggregate
// ============================================================================

TEST_F(MaterializationFixture, DropAggregate_ExistingName_ReturnsTrueAndRemoved) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "to_drop";
    def.config = makeConfig("m", "e", std::chrono::minutes(1));
    engine.createAggregate(def);

    EXPECT_TRUE(engine.dropAggregate("to_drop"));
    EXPECT_EQ(engine.listAggregates().size(), 0u);
    EXPECT_FALSE(engine.getAggregate("to_drop").has_value());
}

TEST_F(MaterializationFixture, DropAggregate_UnknownName_ReturnsFalse) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_FALSE(engine.dropAggregate("no_such_agg"));
}

TEST_F(MaterializationFixture, DropAggregate_ThenRecreate_Succeeds) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "tmp";
    def.config = makeConfig("m", "e", std::chrono::minutes(1));
    ASSERT_TRUE(engine.createAggregate(def));
    ASSERT_TRUE(engine.dropAggregate("tmp"));
    EXPECT_TRUE(engine.createAggregate(def)); // re-create must succeed
}

// ============================================================================
// refreshAggregate + queryMaterialized
// ============================================================================

TEST_F(MaterializationFixture, RefreshAggregate_ProducesDataPoints) {
    // Insert 6 points over 60 seconds (one per 10 s)
    // with a 60-second window → 1 aggregate window
    insertPoints("cpu", "srv", 6, 10.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "cpu_1min";
    def.config = makeConfig("cpu", "srv", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("cpu_1min", to_ms);

    auto pts = engine.queryMaterialized("cpu_1min", base_ms, to_ms + 1);
    EXPECT_GE(pts.size(), 1u);
}

TEST_F(MaterializationFixture, RefreshAggregate_WatermarkAdvances) {
    insertPoints("temp", "node", 12, 20.0, 0.5, 5000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "temp_1min";
    def.config = makeConfig("temp", "node", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("temp_1min", to_ms);

    auto s = engine.getAggregateStatus("temp_1min");
    ASSERT_TRUE(s.has_value());
    EXPECT_GE(s->watermark_ms, to_ms);
}

TEST_F(MaterializationFixture, RefreshAggregate_AlreadyUpToDate_ReturnsZero) {
    insertPoints("m", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "m_1min";
    def.config = makeConfig("m", "e", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("m_1min", to_ms);
    // Second call with the same to_ms must be a no-op
    size_t written = engine.refreshAggregate("m_1min", to_ms);
    EXPECT_EQ(written, 0u);
}

TEST_F(MaterializationFixture, RefreshAggregate_UnknownAggregate_ReturnsZero) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_EQ(engine.refreshAggregate("no_such", base_ms + 60000LL), 0u);
}

TEST_F(MaterializationFixture, RefreshAggregate_InactiveAggregate_Skipped) {
    insertPoints("m", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "inactive_agg";
    def.config = makeConfig("m", "e", std::chrono::milliseconds(60000));
    def.status = ContinuousAggStatus::INACTIVE;
    engine.createAggregate(def);

    size_t written = engine.refreshAggregate("inactive_agg", base_ms + 60000LL);
    EXPECT_EQ(written, 0u);
}

// ============================================================================
// refreshAll
// ============================================================================

TEST_F(MaterializationFixture, RefreshAll_MultipleAggregates_RefreshesBoth) {
    insertPoints("cpu",  "s1", 6, 1.0, 1.0, 10000LL);
    insertPoints("mem",  "s1", 6, 100.0, 5.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition d1;
    d1.name   = "cpu_1min";
    d1.config = makeConfig("cpu", "s1", std::chrono::milliseconds(60000));
    engine.createAggregate(d1);

    ContinuousAggDefinition d2;
    d2.name   = "mem_1min";
    d2.config = makeConfig("mem", "s1", std::chrono::milliseconds(60000));
    engine.createAggregate(d2);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAll(to_ms);

    EXPECT_GE(engine.queryMaterialized("cpu_1min", base_ms, to_ms + 1).size(), 1u);
    EXPECT_GE(engine.queryMaterialized("mem_1min", base_ms, to_ms + 1).size(), 1u);
}

TEST_F(MaterializationFixture, RefreshAll_SkipsAutoRefreshFalse) {
    insertPoints("m", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name         = "no_auto";
    def.config       = makeConfig("m", "e", std::chrono::milliseconds(60000));
    def.auto_refresh = false;
    engine.createAggregate(def);

    size_t total = engine.refreshAll(base_ms + 60000LL);
    EXPECT_EQ(total, 0u);

    // No materialized data should exist for this aggregate
    EXPECT_TRUE(engine.queryMaterialized("no_auto", base_ms,
                                          base_ms + 60001LL).empty());
}

TEST_F(MaterializationFixture, RefreshAll_EmptyRegistry_ReturnsZero) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_EQ(engine.refreshAll(base_ms + 60000LL), 0u);
}

// ============================================================================
// queryMaterialized
// ============================================================================

TEST_F(MaterializationFixture, QueryMaterialized_BeforeRefresh_ReturnsEmpty) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "empty_agg";
    def.config = makeConfig("m", "e", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    EXPECT_TRUE(engine.queryMaterialized("empty_agg", base_ms, base_ms + 60000LL).empty());
}

TEST_F(MaterializationFixture, QueryMaterialized_UnknownAggregate_ReturnsEmpty) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_TRUE(engine.queryMaterialized("no_such", base_ms, base_ms + 60000LL).empty());
}

TEST_F(MaterializationFixture, QueryMaterialized_OutsideRefreshedRange_ReturnsEmpty) {
    insertPoints("cpu", "s", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "cpu_1min";
    def.config = makeConfig("cpu", "s", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("cpu_1min", to_ms);

    // Query for a time range entirely before the data
    auto pts = engine.queryMaterialized("cpu_1min", 0LL, base_ms - 1LL);
    EXPECT_TRUE(pts.empty());
}

TEST_F(MaterializationFixture, QueryMaterialized_AggregateValuesAreSane) {
    // Insert 6 points: values 1,2,3,4,5,6 over 60s
    insertPoints("v", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "v_1min";
    def.config = makeConfig("v", "e", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("v_1min", to_ms);

    auto pts = engine.queryMaterialized("v_1min", base_ms, to_ms + 1);
    ASSERT_GE(pts.size(), 1u);

    // The aggregate point stores avg as its value; avg of 1..6 = 3.5
    double avg = pts[0].value;
    EXPECT_NEAR(avg, 3.5, 0.01);

    // Metadata should contain min, max, sum, count
    EXPECT_TRUE(pts[0].metadata.count("min") > 0 || pts[0].metadata.count("sum") > 0);
}

// ============================================================================
// getAggregateStatus / getAllStatus
// ============================================================================

TEST_F(MaterializationFixture, GetAggregateStatus_UnknownName_ReturnsNullopt) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_FALSE(engine.getAggregateStatus("no_such").has_value());
}

TEST_F(MaterializationFixture, GetAggregateStatus_BeforeRefresh_WatermarkZero) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "fresh";
    def.config = makeConfig("x", "y", std::chrono::minutes(1));
    engine.createAggregate(def);

    auto s = engine.getAggregateStatus("fresh");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->watermark_ms, 0);
    EXPECT_EQ(s->name, "fresh");
    EXPECT_FALSE(s->derived_metric.empty());
}

TEST_F(MaterializationFixture, GetAggregateStatus_AfterRefresh_WatermarkUpdated) {
    insertPoints("p", "q", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "p_1min";
    def.config = makeConfig("p", "q", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAggregate("p_1min", to_ms);

    auto s = engine.getAggregateStatus("p_1min");
    ASSERT_TRUE(s.has_value());
    EXPECT_GE(s->watermark_ms, to_ms);
}

TEST_F(MaterializationFixture, GetAllStatus_ReturnsStatusForEveryAggregate) {
    ContinuousAggMaterializationEngine engine(store.get());

    for (const auto& name : {"agg1", "agg2", "agg3"}) {
        ContinuousAggDefinition d;
        d.name   = name;
        d.config = makeConfig(name, "e", std::chrono::minutes(1));
        engine.createAggregate(d);
    }

    auto all = engine.getAllStatus();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0].name, "agg1");
    EXPECT_EQ(all[2].name, "agg3");
}

TEST_F(MaterializationFixture, GetAllStatus_EmptyRegistry_ReturnsEmptyVector) {
    ContinuousAggMaterializationEngine engine(store.get());
    EXPECT_TRUE(engine.getAllStatus().empty());
}

// ============================================================================
// agg_id derivation
// ============================================================================

TEST_F(MaterializationFixture, AggId_ContainsNameAndDerivedMetric) {
    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition def;
    def.name   = "cpu_5min";
    def.config = makeConfig("cpu_usage", "host", std::chrono::minutes(5));
    engine.createAggregate(def);

    auto got = engine.getAggregate("cpu_5min");
    ASSERT_TRUE(got.has_value());
    // agg_id must contain the aggregate name
    EXPECT_NE(got->agg_id.find("cpu_5min"), std::string::npos);
    // agg_id must reference the derived metric name
    const std::string derived = ContinuousAggregateManager::derivedMetricName(
        "cpu_usage", std::chrono::minutes(5));
    EXPECT_NE(got->agg_id.find(derived), std::string::npos);
}

// ============================================================================
// ContinuousAggStatus
// ============================================================================

TEST_F(MaterializationFixture, InactiveAggregate_SkippedByRefreshAll) {
    insertPoints("z", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());

    ContinuousAggDefinition active;
    active.name   = "z_active";
    active.config = makeConfig("z", "e", std::chrono::milliseconds(60000));
    active.status = ContinuousAggStatus::ACTIVE;
    engine.createAggregate(active);

    ContinuousAggDefinition inactive;
    inactive.name   = "z_inactive";
    inactive.config = makeConfig("z", "e", std::chrono::milliseconds(60000));
    inactive.status = ContinuousAggStatus::INACTIVE;
    engine.createAggregate(inactive);

    int64_t to_ms = base_ms + 60000LL;
    engine.refreshAll(to_ms);

    // Active aggregate must have been materialized
    EXPECT_GE(engine.queryMaterialized("z_active", base_ms, to_ms + 1).size(), 1u);

    // Inactive aggregate must have no watermark advance
    auto s = engine.getAggregateStatus("z_inactive");
    ASSERT_TRUE(s.has_value());
    EXPECT_EQ(s->watermark_ms, 0);
}

// ============================================================================
// Incremental refresh — second pass only processes new data
// ============================================================================

TEST_F(MaterializationFixture, IncrementalRefresh_SecondPassOnlyProcessesNewData) {
    // Phase 1: insert 6 points over minute 1
    insertPoints("inc", "e", 6, 1.0, 1.0, 10000LL);

    ContinuousAggMaterializationEngine engine(store.get());
    ContinuousAggDefinition def;
    def.name   = "inc_1min";
    def.config = makeConfig("inc", "e", std::chrono::milliseconds(60000));
    engine.createAggregate(def);

    int64_t to1 = base_ms + 60000LL;
    engine.refreshAggregate("inc_1min", to1);

    auto pts1 = engine.queryMaterialized("inc_1min", base_ms, to1 + 1);
    EXPECT_GE(pts1.size(), 1u);

    // Phase 2: insert 6 more points in minute 2
    for (int i = 0; i < 6; ++i) {
        TSStore::DataPoint p;
        p.metric       = "inc";
        p.entity       = "e";
        p.timestamp_ms = base_ms + 60000LL + i * 10000LL;
        p.value        = 10.0 + i;
        ASSERT_TRUE(store->putDataPoint(p).has_value());
    }

    int64_t to2 = base_ms + 120000LL;
    size_t new_windows = engine.refreshAggregate("inc_1min", to2);
    EXPECT_GE(new_windows, 1u); // at least one new window processed

    auto pts2 = engine.queryMaterialized("inc_1min", base_ms + 60000LL, to2 + 1);
    EXPECT_GE(pts2.size(), 1u);
}
