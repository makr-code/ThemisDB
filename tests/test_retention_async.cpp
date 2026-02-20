// Phase 6: Retention & Cleanup – Async Background Cleanup Tests

#include <gtest/gtest.h>
#include "timeseries/retention.h"
#include "timeseries/tsstore.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <chrono>
#include <memory>
#include <thread>
#include <limits>

using namespace themis;
namespace fs = std::filesystem;

static std::string makeRetTempPath(const std::string& tag) {
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / ("themis_ret_" + tag + "_" + std::to_string(ns))).string();
}

struct RetentionFixture : ::testing::Test {
    std::string db_path;
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<TSStore> store;
    int64_t base_ms{1700000000000LL};

    void SetUp() override {
        db_path = makeRetTempPath("ret");
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path;
        cfg.enable_blobdb = false;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open()) << "RocksDB open failed at " << db_path;
        store = std::make_unique<TSStore>(db->getRawDB());
    }

    void TearDown() override {
        store.reset();
        db.reset();
        fs::remove_all(db_path);
    }

    void insertOldPoint(const std::string& metric, const std::string& entity,
                        int64_t age_ms, double value = 1.0) {
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        TSStore::DataPoint p;
        p.metric       = metric;
        p.entity       = entity;
        p.timestamp_ms = now_ms - age_ms;
        p.value        = value;
        ASSERT_TRUE(store->putDataPoint(p).ok);
    }

    size_t countPoints(const std::string& metric, const std::string& entity) {
        TSStore::QueryOptions q;
        q.metric           = metric;
        q.entity           = entity;
        q.from_timestamp_ms = 0;
        q.to_timestamp_ms  = std::numeric_limits<int64_t>::max();
        q.limit            = 1000000;
        auto r = store->query(q);
        return r.has_value() ? r->size() : 0;
    }
};

// ===== Construction & Policy =====

TEST_F(RetentionFixture, ConstructsWithEmptyPolicy) {
    RetentionPolicy policy;
    EXPECT_NO_THROW({ RetentionManager mgr(store.get(), policy); });
}

TEST_F(RetentionFixture, ApplyWithNoDataDoesNotCrash) {
    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::seconds(86400);
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();
    EXPECT_EQ(deleted, 0u);
}

TEST_F(RetentionFixture, ApplyDeletesOldPoints) {
    // Insert a point 10 days ago
    insertOldPoint("cpu", "s1", 10LL * 86400 * 1000);  // 10 days old
    ASSERT_EQ(countPoints("cpu", "s1"), 1u);

    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::days(7);  // 7-day retention
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();
    EXPECT_GE(deleted, 1u);
    EXPECT_EQ(countPoints("cpu", "s1"), 0u);
}

TEST_F(RetentionFixture, ApplyKeepsRecentPoints) {
    // Insert a point 1 hour ago
    insertOldPoint("mem", "s2", 3600000LL);  // 1 hour old
    ASSERT_EQ(countPoints("mem", "s2"), 1u);

    RetentionPolicy policy;
    policy.per_metric["mem"] = std::chrono::days(30);  // 30-day retention
    RetentionManager mgr(store.get(), policy);
    size_t deleted = mgr.apply();
    EXPECT_EQ(deleted, 0u);
    EXPECT_EQ(countPoints("mem", "s2"), 1u);
}

TEST_F(RetentionFixture, ApplyOnlyAffectsConfiguredMetrics) {
    insertOldPoint("cpu", "s3", 10LL * 86400 * 1000);  // old cpu point
    insertOldPoint("mem", "s3", 10LL * 86400 * 1000);  // old mem point

    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::days(1);   // cpu: 1-day retention
    // mem: not configured → not deleted
    RetentionManager mgr(store.get(), policy);
    mgr.apply();

    EXPECT_EQ(countPoints("cpu", "s3"), 0u);    // deleted
    EXPECT_EQ(countPoints("mem", "s3"), 1u);    // kept
}

TEST_F(RetentionFixture, ApplyCountInStats) {
    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    mgr.apply();
    EXPECT_EQ(mgr.getStats().apply_count.load(), 2u);
}

TEST_F(RetentionFixture, SetPolicyTakesEffect) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    RetentionPolicy new_policy;
    new_policy.per_metric["disk"] = std::chrono::days(7);
    mgr.setPolicy(new_policy);
    ASSERT_TRUE(mgr.getPolicy().per_metric.count("disk") > 0);
}

// ===== Async background cleanup =====

TEST_F(RetentionFixture, AsyncStartStop) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_FALSE(mgr.isAsyncRunning());
    mgr.startAsync(std::chrono::seconds(3600));
    EXPECT_TRUE(mgr.isAsyncRunning());
    mgr.stopAsync();
    EXPECT_FALSE(mgr.isAsyncRunning());
}

TEST_F(RetentionFixture, AsyncDoubleStartNoCrash) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.startAsync(std::chrono::seconds(3600));
    mgr.startAsync(std::chrono::seconds(3600));  // second call ignored
    EXPECT_TRUE(mgr.isAsyncRunning());
    mgr.stopAsync();
}

TEST_F(RetentionFixture, AsyncDoubleStopNoCrash) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.stopAsync();  // stop without starting
    mgr.stopAsync();  // again, no crash
}

TEST_F(RetentionFixture, AsyncCycleCountsAfterSleep) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    // Use a very short interval for testing
    mgr.startAsync(std::chrono::milliseconds(50));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stopAsync();
    // Should have run at least 1 background cycle
    EXPECT_GE(mgr.getStats().async_cycle_count.load(), 1u);
}

TEST_F(RetentionFixture, AsyncDeletesOldDataInBackground) {
    // Insert old data
    insertOldPoint("net", "s5", 10LL * 86400 * 1000);  // 10 days old
    ASSERT_EQ(countPoints("net", "s5"), 1u);

    RetentionPolicy policy;
    policy.per_metric["net"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), policy);
    // Start async with short interval
    mgr.startAsync(std::chrono::milliseconds(50));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stopAsync();

    EXPECT_EQ(countPoints("net", "s5"), 0u);
}
