// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
//
// Tests for SchemaConsistencyChecker

#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <thread>

#include "metadata/schema_consistency_checker.h"
#include "metadata/schema_manager.h"
#include "metadata/statistics_collector.h"
#include "metadata/schema_constraints.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"

using namespace themis;

static std::string makeTempDbPath(const std::string& prefix) {
    namespace fs = std::filesystem;
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return (fs::temp_directory_path() / (prefix + std::to_string(now))).string();
}

class SchemaConsistencyCheckerTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path       = makeTempDbPath("test_consistency_");
        cfg.enable_blobdb = false;

        db_      = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open()) << "Failed to open test database";
        idx_mgr_ = std::make_unique<SecondaryIndexManager>(*db_);
        schema_  = std::make_unique<SchemaManager>(*db_, idx_mgr_.get());
        stats_   = std::make_unique<StatisticsCollector>(*db_);
        constraints_ = std::make_unique<SchemaConstraints>();
    }

    void TearDown() override {
        if (db_) db_->close();
    }

    void registerTable(const std::string& name,
                       const std::vector<std::string>& cols = {"id", "value"}) {
        SchemaManager::TableSchema ts;
        ts.name = name;
        ts.type = "relational";
        for (const auto& c : cols) {
            SchemaManager::PropertyInfo p; p.name = c; p.type = "string";
            ts.properties.push_back(p);
        }
        schema_->setTableSchema(name, ts);
    }

    void insertRow(const std::string& table, const std::string& id) {
        nlohmann::json doc;
        doc["id"] = id;
        db_->put(table + ":" + id, doc.dump());
    }

    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_mgr_;
    std::unique_ptr<SchemaManager>         schema_;
    std::unique_ptr<StatisticsCollector>   stats_;
    std::unique_ptr<SchemaConstraints>     constraints_;
};

// ============================================================================
// Basic construction
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, ConstructWithNullOptionals) {
    SchemaConsistencyChecker checker(*db_, *schema_);
    SUCCEED();
}

TEST_F(SchemaConsistencyCheckerTest, ConstructWithAllComponents) {
    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), constraints_.get());
    SUCCEED();
}

// ============================================================================
// runCheck – empty database / empty schema
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, EmptyDatabaseNoIssues) {
    SchemaConsistencyChecker checker(*db_, *schema_);
    auto issues = checker.runCheck();
    // Without stats/constraints there should be no issues on an empty database
    EXPECT_TRUE(issues.empty());
}

// ============================================================================
// runCheck – orphan keys
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, UnregisteredPrefixIsDiscoveredAsTable) {
    // Prefixes that appear in user keys are discovered as tables by SchemaManager,
    // so they are not considered orphaned by the consistency checker.
    db_->put("orphan_table:row1", R"({"id":"row1"})");

    SchemaConsistencyChecker checker(*db_, *schema_);
    auto issues = checker.runCheck();

    bool found_orphan = false;
    for (const auto& issue : issues) {
        if (issue.issue_type == "orphan_key" && issue.table_name == "orphan_table") {
            found_orphan = true;
        }
    }
    EXPECT_FALSE(found_orphan);
}

TEST_F(SchemaConsistencyCheckerTest, RegisteredTableKeyNotOrphan) {
    registerTable("users");
    insertRow("users", "alice");

    SchemaConsistencyChecker checker(*db_, *schema_);
    auto issues = checker.runCheck();

    for (const auto& issue : issues) {
        EXPECT_NE(issue.table_name, "users")
            << "Registered table 'users' should not be flagged as orphan";
    }
}

TEST_F(SchemaConsistencyCheckerTest, SystemKeyPrefixesNotOrphan) {
    // System keys like "stats:", "config:", "audit:" should not be flagged
    db_->put("stats:some_data", R"({})");
    db_->put("config:settings", R"({})");
    db_->put("audit:event1", R"({})");

    SchemaConsistencyChecker checker(*db_, *schema_);
    auto issues = checker.runCheck();

    for (const auto& issue : issues) {
        if (issue.issue_type == "orphan_key") {
            EXPECT_NE(issue.table_name, "stats");
            EXPECT_NE(issue.table_name, "config");
            EXPECT_NE(issue.table_name, "audit");
        }
    }
}

// ============================================================================
// runCheck – stale statistics
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, NoStatsReportedAsStale) {
    registerTable("events");
    // Don't collect any stats → should be reported as stale

    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), nullptr);
    auto issues = checker.runCheck();

    bool found_stale = false;
    for (const auto& issue : issues) {
        if (issue.issue_type == "stale_stats" && issue.table_name == "events") {
            found_stale = true;
        }
    }
    EXPECT_TRUE(found_stale);
}

TEST_F(SchemaConsistencyCheckerTest, FreshStatsNotReportedAsStale) {
    registerTable("metrics");
    insertRow("metrics", "m1");
    stats_->collectStats("metrics");

    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), nullptr);
    checker.setMaxStatsAge(std::chrono::hours(24));
    auto issues = checker.runCheck();

    for (const auto& issue : issues) {
        if (issue.issue_type == "stale_stats") {
            EXPECT_NE(issue.table_name, "metrics");
        }
    }
}

// ============================================================================
// runCheck – missing constraints
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, TableWithNoConstraintsReported) {
    registerTable("orders");
    // No constraints added → should be reported

    SchemaConsistencyChecker checker(*db_, *schema_, nullptr, constraints_.get());
    auto issues = checker.runCheck();

    bool found = false;
    for (const auto& issue : issues) {
        if (issue.issue_type == "missing_constraint" && issue.table_name == "orders") {
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(SchemaConsistencyCheckerTest, TableWithConstraintsNotReported) {
    registerTable("invoices");
    constraints_->addConstraint("invoices", "id",
        ColumnConstraint::makeNotNull("invoices_id_nn"));

    SchemaConsistencyChecker checker(*db_, *schema_, nullptr, constraints_.get());
    auto issues = checker.runCheck();

    for (const auto& issue : issues) {
        if (issue.issue_type == "missing_constraint") {
            EXPECT_NE(issue.table_name, "invoices");
        }
    }
}

// ============================================================================
// getLastCheckResults / lastResultsToJSON
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, GetLastCheckResultsBeforeRun) {
    SchemaConsistencyChecker checker(*db_, *schema_);
    auto results = checker.getLastCheckResults();
    EXPECT_TRUE(results.empty());
}

TEST_F(SchemaConsistencyCheckerTest, GetLastCheckResultsAfterRun) {
    registerTable("logs");
    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), constraints_.get());
    checker.runCheck();
    auto results = checker.getLastCheckResults();
    // At least "stale_stats" and "missing_constraint" for "logs"
    EXPECT_FALSE(results.empty());
}

TEST_F(SchemaConsistencyCheckerTest, LastResultsToJSONIsArray) {
    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), constraints_.get());
    registerTable("payments");
    checker.runCheck();
    auto j = checker.lastResultsToJSON();
    EXPECT_TRUE(j.is_array());
    EXPECT_FALSE(j.empty());
    for (const auto& item : j) {
        EXPECT_TRUE(item.contains("issue_type"));
        EXPECT_TRUE(item.contains("table_name"));
        EXPECT_TRUE(item.contains("detail"));
    }
}

// ============================================================================
// ConsistencyIssue::toJSON
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, IssueToJSON) {
    ConsistencyIssue issue;
    issue.issue_type  = "orphan_key";
    issue.table_name  = "ghost";
    issue.column_name = "";
    issue.detail      = "Keys with prefix 'ghost' found";

    auto j = issue.toJSON();
    EXPECT_EQ(j["issue_type"].get<std::string>(), "orphan_key");
    EXPECT_EQ(j["table_name"].get<std::string>(), "ghost");
    EXPECT_EQ(j["detail"].get<std::string>(), "Keys with prefix 'ghost' found");
}

// ============================================================================
// Background checking
// ============================================================================

TEST_F(SchemaConsistencyCheckerTest, BackgroundCheckRunsAndStops) {
    registerTable("tasks");

    SchemaConsistencyChecker checker(*db_, *schema_, stats_.get(), constraints_.get());
    checker.startBackgroundCheck(std::chrono::seconds(1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    checker.stopBackgroundCheck();

    auto results = checker.getLastCheckResults();
    EXPECT_FALSE(results.empty());
}

TEST_F(SchemaConsistencyCheckerTest, ZeroIntervalDisablesBackgroundCheck) {
    SchemaConsistencyChecker checker(*db_, *schema_);
    checker.startBackgroundCheck(std::chrono::seconds(0));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    // No crash; last results still empty
    EXPECT_TRUE(checker.getLastCheckResults().empty());
}

TEST_F(SchemaConsistencyCheckerTest, DestructorStopsBackgroundThread) {
    {
        SchemaConsistencyChecker checker(*db_, *schema_);
        checker.startBackgroundCheck(std::chrono::seconds(60));
        // Destructor called here – must not hang
    }
    SUCCEED();
}

TEST_F(SchemaConsistencyCheckerTest, StopBackgroundCheckIdempotent) {
    SchemaConsistencyChecker checker(*db_, *schema_);
    checker.startBackgroundCheck(std::chrono::seconds(60));
    checker.stopBackgroundCheck();
    checker.stopBackgroundCheck();  // second call must not crash
    SUCCEED();
}
