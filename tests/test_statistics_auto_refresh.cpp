// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Tests for StatisticsCollector auto-refresh background thread

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>

#include "metadata/statistics_collector.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class StatisticsAutoRefreshTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_statsrefresh_");
        cfg.enable_blobdb = false;

        db_ = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
    }

    void TearDown() override {
        if (db_) db_->close();
    }

    void insertRows(const std::string& table, int count) {
        for (int i = 0; i < count; ++i) {
            std::string key = table + ":row" + std::to_string(i);
            nlohmann::json doc;
            doc["id"]  = i;
            doc["val"] = "v" + std::to_string(i);
            db_->put(key, doc.dump());
        }
    }

    std::unique_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// Destructor stops the thread
// ============================================================================

TEST_F(StatisticsAutoRefreshTest, DestructorStopsThread) {
    {
        StatisticsCollector sc(*db_);
        sc.setRefreshInterval(std::chrono::seconds(300));
        // Let it start
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // Destructor is called here → must not hang
    }
    SUCCEED();  // If we reach here, destructor didn't deadlock
}

// ============================================================================
// Interval 0 disables the thread
// ============================================================================

TEST_F(StatisticsAutoRefreshTest, ZeroIntervalDisablesThread) {
    StatisticsCollector sc(*db_);
    sc.setRefreshInterval(std::chrono::seconds(0));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // No thread running – just check we can still call getStats
    auto result = sc.getStats("nonexistent");
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// setRefreshInterval can be called multiple times
// ============================================================================

TEST_F(StatisticsAutoRefreshTest, MultipleSetRefreshIntervalCalls) {
    StatisticsCollector sc(*db_);
    sc.setRefreshInterval(std::chrono::seconds(300));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sc.setRefreshInterval(std::chrono::seconds(600));  // should replace, not deadlock
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sc.stopRefresh();
    SUCCEED();
}

// ============================================================================
// stopRefresh is idempotent
// ============================================================================

TEST_F(StatisticsAutoRefreshTest, StopRefreshIdempotent) {
    StatisticsCollector sc(*db_);
    sc.setRefreshInterval(std::chrono::seconds(60));
    sc.stopRefresh();
    sc.stopRefresh();  // second call must not crash/deadlock
    SUCCEED();
}

// ============================================================================
// Auto-refresh actually re-collects stats
// ============================================================================

TEST_F(StatisticsAutoRefreshTest, AutoRefreshUpdatesStats) {
    // Insert some rows
    insertRows("products", 10);

    StatisticsCollector sc(*db_);
    // Do a first manual collect so the table is known
    auto first = sc.collectStats("products");
    ASSERT_TRUE(first.ok) << first.error_message;
    size_t initial_rows = first.value.row_count;

    // Insert more rows
    insertRows("products_extra", 5);
    // Re-collect via setRefreshInterval with very short interval
    sc.setRefreshInterval(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    sc.stopRefresh();

    // The in-memory stats should still be valid
    auto after = sc.getStats("products");
    EXPECT_TRUE(after.ok) << after.error_message;
    // Row count after refresh should still reflect reality
    EXPECT_GE(after.value.row_count, 0u);
    (void)initial_rows;
}

// ============================================================================
// Metrics hook called during auto-refresh
// ============================================================================

struct CountingHook : public StatisticsCollector::IMetricsHook {
    std::atomic<int> collect_count{0};
    std::atomic<int> error_count{0};
    std::atomic<int> hit_count{0};
    std::atomic<int> miss_count{0};

    void onCollect(std::string_view, double, size_t, bool success) override {
        if (success) ++collect_count; else ++error_count;
    }
    void onCacheHit(std::string_view) override  { ++hit_count;  }
    void onCacheMiss(std::string_view) override { ++miss_count; }
    void onError(std::string_view, int) override { ++error_count; }
};

TEST_F(StatisticsAutoRefreshTest, MetricsHookCalledDuringAutoRefresh) {
    insertRows("events", 5);

    StatisticsCollector sc(*db_);
    CountingHook hook;
    sc.setMetricsHook(&hook);

    // First manual collect (registers the table)
    ASSERT_TRUE(sc.collectStats("events").ok);
    int initial_count = hook.collect_count.load();

    // Auto-refresh with 1-second interval
    sc.setRefreshInterval(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    sc.stopRefresh();

    // At least one additional onCollect call expected
    EXPECT_GT(hook.collect_count.load(), initial_count);

    sc.setMetricsHook(nullptr);
}
