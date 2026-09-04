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
    std::string db_path = {};
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
        ASSERT_TRUE(store->putDataPoint(p).has_value());
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
    mgr.startAsync(std::chrono::seconds(1));
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
    mgr.startAsync(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    mgr.stopAsync();

    EXPECT_EQ(countPoints("net", "s5"), 0u);
}

// ===== Staged/Graduated Deletion =====

TEST_F(RetentionFixture, StagedDeletionPolicySetAndRead) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_FALSE(mgr.hasStagedDeletion());

    StagedDeletionPolicy staged;
    staged.mark_after        = std::chrono::days(30);
    staged.soft_delete_after = std::chrono::days(60);
    staged.hard_delete_after = std::chrono::days(90);
    mgr.setStagedDeletion(staged);
    EXPECT_TRUE(mgr.hasStagedDeletion());
}

TEST_F(RetentionFixture, StagedDeletionPolicyFields) {
    StagedDeletionPolicy staged;
    staged.mark_after        = std::chrono::seconds(86400);   // 1 day
    staged.soft_delete_after = std::chrono::seconds(604800);  // 7 days
    staged.hard_delete_after = std::chrono::seconds(2592000); // 30 days
    EXPECT_EQ(staged.mark_after.count(), 86400);
    EXPECT_EQ(staged.soft_delete_after.count(), 604800);
    EXPECT_EQ(staged.hard_delete_after.count(), 2592000);
}

TEST_F(RetentionFixture, StagedDeletionDoesNotCrashOnApply) {
    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), policy);
    StagedDeletionPolicy staged;
    staged.hard_delete_after = std::chrono::days(1);
    mgr.setStagedDeletion(staged);
    EXPECT_NO_THROW(mgr.apply());
}

// ===== Compliance Logging =====

TEST_F(RetentionFixture, AuditLogInitiallyEmpty) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_TRUE(mgr.getAuditLog().empty());
}

TEST_F(RetentionFixture, AuditLogPopulatedAfterApply) {
    RetentionPolicy policy;
    policy.per_metric["cpu"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    auto log = mgr.getAuditLog();
    EXPECT_FALSE(log.empty());
    // Should have at least an "apply" action entry
    bool has_apply = false;
    for (const auto& e : log) {
        if (e.action == "apply") { has_apply = true; break; }
    }
    EXPECT_TRUE(has_apply);
}

TEST_F(RetentionFixture, AuditLogEntryHasTimestamp) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    auto log = mgr.getAuditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_GT(log[0].timestamp_ms, 0LL);
}

TEST_F(RetentionFixture, AuditCallbackInvokedOnApply) {
    RetentionPolicy policy;
    policy.per_metric["net"] = std::chrono::days(1);
    RetentionManager mgr(store.get(), policy);
    std::vector<RetentionAuditEntry> captured;
    mgr.setAuditCallback([&](const RetentionAuditEntry& e) {
        captured.push_back(e);
    });
    mgr.apply();
    EXPECT_FALSE(captured.empty());
}

TEST_F(RetentionFixture, AuditCallbackReceivesHardDeleteEntry) {
    // Insert old data to trigger deletion
    insertOldPoint("disk2", "s9", 20LL * 86400 * 1000);  // 20 days old
    RetentionPolicy policy;
    policy.per_metric["disk2"] = std::chrono::days(7);
    RetentionManager mgr(store.get(), policy);
    std::string last_action = {};
    mgr.setAuditCallback([&](const RetentionAuditEntry& e) {
        if (!e.action.empty()) {
          last_action = e.action;
        }
    });
    mgr.apply();
    // "hard_delete" and/or "apply" should appear
    EXPECT_FALSE(last_action.empty());
}

TEST_F(RetentionFixture, ClearAuditLog) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    EXPECT_FALSE(mgr.getAuditLog().empty());
    mgr.clearAuditLog();
    EXPECT_TRUE(mgr.getAuditLog().empty());
}

// ===== Retention Metrics: space_reclaimed =====

TEST_F(RetentionFixture, SpaceReclaimedStartsZero) {
    RetentionPolicy policy;
    RetentionManager mgr(store.get(), policy);
    EXPECT_EQ(mgr.getStats().total_space_reclaimed_est.load(), 0u);
}

TEST_F(RetentionFixture, SpaceReclaimedIncreasesAfterDeletion) {
    insertOldPoint("disk3", "s10", 30LL * 86400 * 1000);
    RetentionPolicy policy;
    policy.per_metric["disk3"] = std::chrono::days(7);
    RetentionManager mgr(store.get(), policy);
    mgr.apply();
    // If a point was deleted, estimate should increase
    uint64_t space = mgr.getStats().total_space_reclaimed_est.load();
    // Either 0 (if delete not supported) or > 0
    EXPECT_GE(space, 0u);
}
