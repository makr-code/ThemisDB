/**
 * @file test_timeseries_retention.cpp
 * @brief Unit tests for RetentionManager – time-series data retention policies
 */

#include <gtest/gtest.h>
#include "timeseries/timeseries.h"
#include "timeseries/retention.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <filesystem>
#include <limits>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

namespace themis {
namespace {

static std::string makeTempPath(const std::string& tag) {
    namespace fs = std::filesystem;
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_retention_" + tag + "_" + std::to_string(ns))).string();
}

/// Current time in milliseconds
static int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

struct RetentionFixture : ::testing::Test {
    std::string db_path = {};
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;

    void SetUp() override {
        db_path = makeTempPath("test");
        RocksDBWrapper::Config cfg;
        cfg.db_path       = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;
        TSStore::Config ts_cfg;
        ts_cfg.compression = TSStore::CompressionType::None;
        store = std::make_unique<TSStore>(db->getRawDB(), nullptr, ts_cfg);
    }

    void TearDown() override {
        store.reset();
        db.reset();
        std::filesystem::remove_all(db_path);
    }

    void insertAt(const std::string& metric, int64_t ts_ms, double value = 1.0) {
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = "host";
        p.timestamp_ms = ts_ms;
        p.value        = value;
        ASSERT_TRUE(store->putDataPoint(p).has_value());
    }

    size_t countAll(const std::string& metric) {
        TSStore::QueryOptions q;
        q.metric            = metric;
        q.from_timestamp_ms = 0;
        q.to_timestamp_ms   = std::numeric_limits<int64_t>::max();
        q.limit             = 1000000;
        auto r = store->query(q);
        return r.has_value() ? r->size() : 0;
    }
};

// ─── apply(): data deletion ───────────────────────────────────────────────────

TEST_F(RetentionFixture, ApplyDeletesOldData) {
    int64_t now = nowMs();
    insertAt("logs", now - 20LL * 86400000);  // 20 days old → should be deleted
    insertAt("logs", now - 3LL  * 86400000);  // 3 days old  → retained

    RetentionPolicy policy;
    policy.per_metric["logs"] = std::chrono::days(7);
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();

    EXPECT_EQ(deleted, 1u);
    EXPECT_EQ(countAll("logs"), 1u);
}

TEST_F(RetentionFixture, ApplyKeepsRecentData) {
    int64_t now = nowMs();
    insertAt("metrics", now - 3600000);  // 1 hour ago → within 30-day window

    RetentionPolicy policy;
    policy.per_metric["metrics"] = std::chrono::days(30);
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();

    EXPECT_EQ(deleted, 0u);
    EXPECT_EQ(countAll("metrics"), 1u);
}

TEST_F(RetentionFixture, ApplyOnEmptyPolicyDoesNothing) {
    int64_t now = nowMs();
    insertAt("data", now - 1000000);

    RetentionPolicy policy; // empty policy – no metrics registered
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();

    EXPECT_EQ(deleted, 0u);
    EXPECT_EQ(countAll("data"), 1u);
}

TEST_F(RetentionFixture, ApplyWithNullStoreReturnsZero) {
    RetentionPolicy policy;
    policy.per_metric["x"] = 1s;
    RetentionManager mgr(nullptr, policy);
    EXPECT_EQ(mgr.apply(), 0u);
}

// ─── Multiple metrics with different windows ───────────────────────────────────

TEST_F(RetentionFixture, DifferentRetentionPerMetric) {
    int64_t now = nowMs();
    insertAt("short_ret", now - 3LL  * 86400000);   // 3 days, window=1 day  → deleted
    insertAt("long_ret",  now - 3LL  * 86400000);   // 3 days, window=7 days → retained
    insertAt("no_policy", now - 100LL * 86400000);  // 100 days, no policy   → retained

    RetentionPolicy policy;
    policy.per_metric["short_ret"] = std::chrono::days(1);
    policy.per_metric["long_ret"]  = std::chrono::days(7);

    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();

    EXPECT_EQ(deleted, 1u);
    EXPECT_EQ(countAll("short_ret"), 0u);  // deleted
    EXPECT_EQ(countAll("long_ret"),  1u);  // retained
    EXPECT_EQ(countAll("no_policy"), 1u);  // no policy, not touched
}

// ─── RetentionStats counters ───────────────────────────────────────────────────

TEST_F(RetentionFixture, StatsApplyCountIncrements) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_EQ(mgr.getStats().apply_count.load(), 0u);

    mgr.apply();
    EXPECT_EQ(mgr.getStats().apply_count.load(), 1u);

    mgr.apply();
    EXPECT_EQ(mgr.getStats().apply_count.load(), 2u);
}

TEST_F(RetentionFixture, StatsTotalDeletedAccumulates) {
    int64_t now = nowMs();
    insertAt("metric_a", now - 10LL * 86400000);  // 10 days old
    insertAt("metric_b", now - 10LL * 86400000);  // 10 days old

    RetentionPolicy policy;
    policy.per_metric["metric_a"] = std::chrono::days(5);
    policy.per_metric["metric_b"] = std::chrono::days(5);
    RetentionManager mgr(store.get(), policy);
    mgr.apply();

    EXPECT_GE(mgr.getStats().total_deleted.load(), 2u);
    EXPECT_GT(mgr.getStats().total_space_reclaimed_est.load(), 0u);
}

// ─── Audit log ───────────────────────────────────────────────────────────────

TEST_F(RetentionFixture, AuditLogRecordsApplyEntry) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.apply();

    auto log = mgr.getAuditLog();
    ASSERT_FALSE(log.empty());
    // The "apply" entry is always added
    bool found_apply = false;
    for (const auto& e : log) {
        if (e.action == "apply") { found_apply = true; break; }
    }
    EXPECT_TRUE(found_apply);
}

TEST_F(RetentionFixture, AuditLogRecordsHardDeleteEntry) {
    int64_t now = nowMs();
    insertAt("audit_m", now - 10LL * 86400000);

    RetentionPolicy policy;
    policy.per_metric["audit_m"] = std::chrono::days(5);
    RetentionManager mgr(store.get(), policy);
    mgr.apply();

    auto log = mgr.getAuditLog();
    bool found_delete = false;
    for (const auto& e : log) {
        if (e.action == "hard_delete" && e.metric == "audit_m") {
            found_delete = true;
            EXPECT_GT(e.records_affected, 0u);
            break;
        }
    }
    EXPECT_TRUE(found_delete) << "Expected hard_delete audit entry for 'audit_m'";
}

TEST_F(RetentionFixture, ClearAuditLogEmptiesLog) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    ASSERT_FALSE(mgr.getAuditLog().empty());

    mgr.clearAuditLog();
    EXPECT_TRUE(mgr.getAuditLog().empty());
}

// ─── Audit callback ───────────────────────────────────────────────────────────

TEST_F(RetentionFixture, AuditCallbackIsInvoked) {
    int64_t now = nowMs();
    insertAt("cb_m", now - 10LL * 86400000);

    RetentionPolicy policy;
    policy.per_metric["cb_m"] = std::chrono::days(5);
    RetentionManager mgr(store.get(), policy);

    std::vector<RetentionAuditEntry> calls;
    mgr.setAuditCallback([&](const RetentionAuditEntry& e) {
        calls.push_back(e);
    });
    mgr.apply();

    EXPECT_FALSE(calls.empty());
    bool found = false;
    for (const auto& e : calls) {
        if (e.action == "hard_delete") { found = true; break; }
    }
    EXPECT_TRUE(found);
}

// ─── setPolicy / getPolicy ────────────────────────────────────────────────────

TEST_F(RetentionFixture, SetPolicyUpdatesPolicy) {
    RetentionPolicy p1;
    p1.per_metric["m"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), p1);
    EXPECT_EQ(mgr.getPolicy().per_metric.count("m"), 1u);

    RetentionPolicy p2;
    p2.per_metric["n"] = std::chrono::days(7);
    mgr.setPolicy(p2);
    EXPECT_EQ(mgr.getPolicy().per_metric.count("m"), 0u);
    EXPECT_EQ(mgr.getPolicy().per_metric.count("n"), 1u);
}

// ─── StagedDeletion flag ──────────────────────────────────────────────────────

TEST_F(RetentionFixture, StagedDeletionFlagToggle) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_FALSE(mgr.hasStagedDeletion());

    StagedDeletionPolicy staged;
    staged.hard_delete_after = std::chrono::days(30);
    mgr.setStagedDeletion(staged);
    EXPECT_TRUE(mgr.hasStagedDeletion());
}

// ─── Async background thread ──────────────────────────────────────────────────

TEST_F(RetentionFixture, AsyncStartStop) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_FALSE(mgr.isAsyncRunning());

    mgr.startAsync(10h);
    EXPECT_TRUE(mgr.isAsyncRunning());

    mgr.stopAsync();
    EXPECT_FALSE(mgr.isAsyncRunning());
}

TEST_F(RetentionFixture, AsyncDoubleStartIsNoop) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.startAsync(10h);
    EXPECT_TRUE(mgr.isAsyncRunning());

    // Second start should be a no-op (still running)
    mgr.startAsync(10h);
    EXPECT_TRUE(mgr.isAsyncRunning());

    mgr.stopAsync();
    EXPECT_FALSE(mgr.isAsyncRunning());
}

TEST_F(RetentionFixture, AsyncRunsApplyInBackground) {
    int64_t now = nowMs();
    insertAt("async_m", now - 10LL * 86400000);

    RetentionPolicy policy;
    policy.per_metric["async_m"] = std::chrono::days(5);
    RetentionManager mgr(store.get(), policy);

    // Start with a very short interval so it fires quickly
    mgr.startAsync(std::chrono::seconds(1));
    // Give the background thread time to run at least one cycle
    std::this_thread::sleep_for(std::chrono::seconds(2));
    mgr.stopAsync();

    // The async cycle counter should have incremented
    EXPECT_GE(mgr.getStats().async_cycle_count.load(), 1u);
}

TEST(TimeSeriesStoreRegression, RangeQueryUsesDefaultColumnFamilyWhenNullptrWasPassed) {
    const auto db_path = makeTempPath("simple_store");

    RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    cfg.enable_blobdb = false;

    auto db = std::make_unique<RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open()) << "Failed to open RocksDB at " << db_path;

    TimeSeriesStore store(db->getRawDB(), nullptr);

    const std::string metric = "temperature";
    const std::string entity = "sensor_1";
    const int64_t base_ts = 1700000000000LL;

    for (int i = 0; i < 10; ++i) {
        TimeSeriesStore::DataPoint point;
        point.timestamp_ms = base_ts + (i * 1000);
        point.value = 20.0 + static_cast<double>(i);
        ASSERT_TRUE(store.put(metric, entity, point));
    }

    TimeSeriesStore::RangeQuery query;
    query.from_ms = base_ts;
    query.to_ms = base_ts + (60 * 1000);

    const auto points = store.query(metric, entity, query);
    ASSERT_EQ(points.size(), 10u);
    EXPECT_DOUBLE_EQ(points.front().value, 20.0);
    EXPECT_DOUBLE_EQ(points.back().value, 29.0);

    const auto latest = store.getLatest(metric, entity);
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ(latest->timestamp_ms, base_ts + (9 * 1000));

    db.reset();
    std::filesystem::remove_all(db_path);
}

} // namespace
} // namespace themis

