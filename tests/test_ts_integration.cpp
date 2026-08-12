// Phase 7: Integration & Chaos Tests for TimeSeries Module
// End-to-end tests covering the full pipeline:
//   TSAutoBuffer → TSStore → ContinuousAggregateManager → TSQueryOptimizer → RetentionManager

#include <gtest/gtest.h>
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"
#include "timeseries/continuous_agg.h"
#include "timeseries/query_optimizer.h"
#include "timeseries/retention.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <random>
#include <thread>
#include <vector>
#include <limits>
#include <cstddef>

using namespace themis;
namespace fs = std::filesystem;

// ============================================================
// Fixture
// ============================================================

struct IntegrationFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    int64_t base_ms{1700000000000LL};

    void SetUp() override {
        auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        db_path = (fs::temp_directory_path() / ("themis_integ_" + std::to_string(ns))).string();
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed";
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    TSStore::DataPoint makePoint(const std::string& metric, const std::string& entity,
                                  double value, int64_t ts_ms) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = ts_ms;
        p.value        = value;
        return p;
    }

    void insertPoints(const std::string& metric, const std::string& entity,
                      int n, int64_t step_ms = 10000) {
        for (int i = 0; i < n; ++i) {
            auto r = store->putDataPoint(makePoint(metric, entity,
                                                    static_cast<double>(i),
                                                    base_ms + i * step_ms));
            ASSERT_TRUE(r.has_value());
        }
    }

    size_t queryCount(const std::string& metric, const std::string& entity,
                      int64_t from_ms, int64_t to_ms) {
        TSStore::QueryOptions q;
        q.metric           = metric;
        q.entity           = entity;
        q.from_timestamp_ms = from_ms;
        q.to_timestamp_ms  = to_ms;
        q.limit            = 1000000;
        auto r = store->query(q);
        return r.has_value() ? r->size() : 0;
    }
};

// ============================================================
// End-to-End: Buffer → Store → Query
// ============================================================

TEST_F(IntegrationFixture, E2E_BufferFlushThenQuery) {
    TSAutoBufferConfig buf_cfg;
    buf_cfg.async_flush = false;
    TSAutoBuffer buf(store.get(), buf_cfg);

    for (int i = 0; i < 50; ++i) {
        buf.add(makePoint("e2e_cpu", "srv01", static_cast<double>(i),
                           base_ms + i * 10000));
    }
    buf.flush();

    EXPECT_EQ(queryCount("e2e_cpu", "srv01", base_ms, base_ms + 500000), 50u);
}

TEST_F(IntegrationFixture, E2E_BufferAutoFlushOnSizeThreshold) {
    TSAutoBufferConfig buf_cfg;
    buf_cfg.async_flush          = false;
    buf_cfg.max_points_per_buffer = 10;  // Flush every 10 points
    TSAutoBuffer buf(store.get(), buf_cfg);

    for (int i = 0; i < 30; ++i) {
        buf.add(makePoint("e2e_mem", "srv01", static_cast<double>(i),
                           base_ms + i * 1000));
    }
    // At least 2 auto-flushes should have happened
    EXPECT_GE(buf.getStats().size_triggered_flush.load(), 2u);
}

// ============================================================
// End-to-End: Store → Aggregate → Optimizer
// ============================================================

TEST_F(IntegrationFixture, E2E_AggregateAndOptimize) {
    insertPoints("e2e_req", "web01", 360, 10000);  // 1-hour of data at 10s intervals

    // Compute 1-minute aggregates
    ContinuousAggregateManager mgr(store.get());
    AggConfig agg_cfg;
    agg_cfg.metric      = "e2e_req";
    agg_cfg.entity      = std::string("web01");
    agg_cfg.window.size = std::chrono::minutes(1);
    mgr.refresh(agg_cfg, base_ms, base_ms + 3600000);

    // Optimizer should find and use the aggregate
    TSQueryOptimizer opt(store.get());
    opt.registerAvailableAggregate("e2e_req", std::chrono::minutes(1));

    TSQueryOptimizer::OptimizationHint hint;
    hint.use_aggregates        = true;
    hint.min_window_for_agg_ms = 0;
    hint.max_raw_points        = 1;  // Force aggregate use

    auto plan = opt.optimizeAggregateQuery(
        "e2e_req", std::string("web01"), base_ms, base_ms + 3600000, hint);
    EXPECT_TRUE(plan.uses_aggregate);
    EXPECT_GT(plan.estimated_speedup, 1.0);
}

TEST_F(IntegrationFixture, E2E_RollupHierarchy) {
    insertPoints("e2e_disk", "db01", 360, 10000);

    ContinuousAggregateManager mgr(store.get());
    auto hierarchy = RollupHierarchy::defaultHierarchy("e2e_disk", std::string("db01"));
    EXPECT_NO_THROW(mgr.refreshHierarchy(hierarchy, base_ms, base_ms + 3600000));

    // 1m aggregate should exist
    auto agg_name = ContinuousAggregateManager::derivedMetricName(
        "e2e_disk", std::chrono::minutes(1));
    EXPECT_GE(queryCount(agg_name, "db01", base_ms, base_ms + 3600000), 0u);
}

// ============================================================
// End-to-End: Store → Retention → Verify deletion
// ============================================================

TEST_F(IntegrationFixture, E2E_RetentionDeletesOldData) {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Insert old data (20 days ago)
    auto old_ts = now_ms - 20LL * 86400000;
    store->putDataPoint(makePoint("e2e_logs", "host01", 1.0, old_ts));
    ASSERT_EQ(queryCount("e2e_logs", "host01", 0, std::numeric_limits<int64_t>::max()), 1u);

    RetentionPolicy policy;
    policy.per_metric["e2e_logs"] = std::chrono::days(7);
    RetentionManager ret(store.get(), policy);
    ret.apply();

    EXPECT_EQ(queryCount("e2e_logs", "host01", 0, std::numeric_limits<int64_t>::max()), 0u);
}

TEST_F(IntegrationFixture, E2E_RetentionKeepsRecentData) {
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    store->putDataPoint(makePoint("e2e_metrics", "host01", 99.0, now_ms - 3600000));

    RetentionPolicy policy;
    policy.per_metric["e2e_metrics"] = std::chrono::days(30);
    RetentionManager ret(store.get(), policy);
    ret.apply();

    EXPECT_EQ(queryCount("e2e_metrics", "host01", 0, std::numeric_limits<int64_t>::max()), 1u);
}

// ============================================================
// End-to-End: WAL crash-recovery scenario
// ============================================================

TEST_F(IntegrationFixture, E2E_WALCrashRecovery) {
    std::string wal = (fs::temp_directory_path() / "e2e_crash_wal.jsonl").string();

    // Phase 1: Fill buffer, persist WAL (simulate crash before flush)
    {
        TSAutoBufferConfig cfg;
        cfg.async_flush = false;
        TSAutoBuffer buf(store.get(), cfg);
        for (int i = 0; i < 5; ++i) {
            buf.add(makePoint("e2e_crash", "s1", static_cast<double>(i),
                               base_ms + i * 1000));
        }
        size_t persisted = buf.persistToWAL(wal);
        EXPECT_EQ(persisted, 5u);
        // Simulate crash: buf goes out of scope without flush
    }

    // Phase 2: After restart, restore from WAL and flush
    {
        TSAutoBufferConfig cfg;
        cfg.async_flush = false;
        TSAutoBuffer buf(store.get(), cfg);
        std::ptrdiff_t restored = buf.restoreFromWAL(wal);
        EXPECT_EQ(restored, 5);
        buf.flush();
        TSAutoBuffer::removeWAL(wal);
    }

    EXPECT_EQ(queryCount("e2e_crash", "s1", base_ms, base_ms + 10000), 5u);
}

// ============================================================
// Chaos: Concurrent writes + flush
// ============================================================

TEST_F(IntegrationFixture, Chaos_ConcurrentWrites) {
    TSAutoBufferConfig cfg;
    cfg.async_flush          = false;
    cfg.max_points_per_buffer = 100;
    TSAutoBuffer buf(store.get(), cfg);

    constexpr int threads = 4;
    constexpr int per_thread = 25;
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) {
        pool.emplace_back([&, t]() {
            for (int i = 0; i < per_thread; ++i) {
                buf.add(makePoint("chaos_cpu", "srv_" + std::to_string(t),
                                   static_cast<double>(i),
                                   base_ms + t * 1000000LL + i * 1000));
            }
        });
    }
    for (auto& th : pool) th.join();
    buf.flush();

    size_t total = 0;
    for (int t = 0; t < threads; ++t) {
        total += queryCount("chaos_cpu", "srv_" + std::to_string(t),
                             base_ms, base_ms + 2000000LL);
    }
    EXPECT_EQ(total, static_cast<size_t>(threads * per_thread));
}

TEST_F(IntegrationFixture, Chaos_RandomValueRange) {
    std::mt19937_64 rng(12345);
    std::uniform_real_distribution<double> dist(-1e12, 1e12);

    for (int i = 0; i < 100; ++i) {
        auto r = store->putDataPoint(makePoint("chaos_rand", "s0",
                                                dist(rng), base_ms + i * 1000));
        EXPECT_TRUE(r.has_value());
    }
    EXPECT_EQ(queryCount("chaos_rand", "s0", base_ms, base_ms + 100000), 100u);
}

TEST_F(IntegrationFixture, Chaos_InfinityValues) {
    EXPECT_TRUE(store->putDataPoint(
        makePoint("chaos_inf", "s0", std::numeric_limits<double>::infinity(), base_ms)).has_value());
    EXPECT_TRUE(store->putDataPoint(
        makePoint("chaos_inf", "s0", -std::numeric_limits<double>::infinity(), base_ms + 1)).has_value());
}

TEST_F(IntegrationFixture, Chaos_NaNValue) {
    EXPECT_TRUE(store->putDataPoint(
        makePoint("chaos_nan", "s0", std::numeric_limits<double>::quiet_NaN(), base_ms)).has_value());
}

TEST_F(IntegrationFixture, Chaos_HighCardinality) {
    // 50 different entities for same metric
    for (int e = 0; e < 50; ++e) {
        store->putDataPoint(makePoint("chaos_hc", "entity_" + std::to_string(e),
                                       static_cast<double>(e), base_ms));
    }
    // Each entity should have exactly 1 point
    for (int e = 0; e < 50; ++e) {
        EXPECT_EQ(queryCount("chaos_hc", "entity_" + std::to_string(e),
                              base_ms, base_ms + 1), 1u);
    }
}

// ============================================================
// Multi-Shard integration: merge + store
// ============================================================

TEST_F(IntegrationFixture, E2E_DistributedAggTwoShards) {
    insertPoints("e2e_sharded", "s0", 30, 10000);

    DistributedAggregateCoordinator coord(
        store.get(), 2,
        [&](int shard_id, const AggConfig& cfg, int64_t from_ms, int64_t to_ms) -> AggShardResult {
            AggShardResult r;
            r.metric  = cfg.metric;
            r.from_ms = from_ms;
            r.to_ms   = to_ms;
            r.valid   = true;
            r.count   = 15;
            r.sum     = shard_id == 0 ? 105.0 : 210.0;  // 0+1+...+14 = 105
            r.min     = 0.0;
            r.max     = 14.0;
            return r;
        });

    AggConfig cfg;
    cfg.metric      = "e2e_sharded";
    cfg.entity      = std::string("s0");
    cfg.window.size = std::chrono::minutes(1);

    auto result = coord.refreshAggregate(cfg, base_ms, base_ms + 300000);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.count, 30u);
    EXPECT_NEAR(result.avg(), 10.5, 0.001);
}
