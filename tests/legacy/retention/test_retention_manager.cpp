#include <gtest/gtest.h>
#include "utils/retention_manager.h"
#include <chrono>
#include <atomic>
#include <thread>

using namespace vcc;
using namespace std::chrono_literals;

// ============================================================================
// Policy management
// ============================================================================

TEST(RetentionPolicyMgmt, RegisterPolicy) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "gdpr_test";
    p.retention_period   = 30 * 24h;
    p.archive_after      = 7 * 24h;
    p.auto_purge_enabled = true;
    p.require_audit_trail = true;
    p.classification_level = "offen";

    EXPECT_TRUE(mgr.registerPolicy(p));
    auto policies = mgr.getPolicies();
    EXPECT_EQ(policies.size(), 1u);
    EXPECT_EQ(policies[0].name, "gdpr_test");
}

TEST(RetentionPolicyMgmt, RegisterDuplicateOverwrites) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "dup";
    p.retention_period = 10s;
    p.archive_after    = 5s;
    mgr.registerPolicy(p);

    p.retention_period = 20s;
    EXPECT_TRUE(mgr.registerPolicy(p));
    auto r = mgr.getPolicy("dup");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ((*r)->retention_period, 20s);
}

TEST(RetentionPolicyMgmt, RegisterEmptyNameFails) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "";
    p.retention_period = 10s;
    p.archive_after    = 5s;
    EXPECT_FALSE(mgr.registerPolicy(p));
}

TEST(RetentionPolicyMgmt, RemoveExistingPolicy) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "to_remove";
    p.retention_period = 10s;
    p.archive_after    = 5s;
    mgr.registerPolicy(p);
    EXPECT_TRUE(mgr.removePolicy("to_remove"));
    EXPECT_EQ(mgr.getPolicies().size(), 0u);
}

TEST(RetentionPolicyMgmt, RemoveNonExistentFails) {
    RetentionManager mgr;
    EXPECT_FALSE(mgr.removePolicy("ghost"));
}

TEST(RetentionPolicyMgmt, GetPolicyFound) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "found";
    p.retention_period = 5s;
    p.archive_after    = 2s;
    mgr.registerPolicy(p);

    auto result = mgr.getPolicy("found");
    EXPECT_TRUE(result.has_value());
}

TEST(RetentionPolicyMgmt, GetPolicyNotFound) {
    RetentionManager mgr;
    auto result = mgr.getPolicy("missing");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// shouldArchive / shouldPurge logic
// ============================================================================

TEST(RetentionLogic, ShouldArchiveOldEntity) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "arch";
    p.retention_period = 60s;
    p.archive_after    = 5s;
    mgr.registerPolicy(p);

    auto old_time = std::chrono::system_clock::now() - 10s;
    EXPECT_TRUE(mgr.shouldArchive("e1", old_time, "arch"));
}

TEST(RetentionLogic, ShouldNotArchiveNewEntity) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "arch2";
    p.retention_period = 60s;
    p.archive_after    = 3600s;
    mgr.registerPolicy(p);

    auto new_time = std::chrono::system_clock::now() - 1s;
    EXPECT_FALSE(mgr.shouldArchive("e1", new_time, "arch2"));
}

TEST(RetentionLogic, ShouldPurgeExpiredEntity) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "purge_pol";
    p.retention_period   = 5s;
    p.archive_after      = 2s;
    p.auto_purge_enabled = true;
    mgr.registerPolicy(p);

    auto old_time = std::chrono::system_clock::now() - 10s;
    EXPECT_TRUE(mgr.shouldPurge("e1", old_time, "purge_pol"));
}

TEST(RetentionLogic, ShouldNotPurgeWhenDisabled) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "no_purge";
    p.retention_period   = 5s;
    p.archive_after      = 2s;
    p.auto_purge_enabled = false;
    mgr.registerPolicy(p);

    auto old_time = std::chrono::system_clock::now() - 60s;
    EXPECT_FALSE(mgr.shouldPurge("e1", old_time, "no_purge"));
}

// ============================================================================
// archiveEntity / purgeEntity execution
// ============================================================================

TEST(RetentionExecution, ArchiveEntitySuccess) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "exec";
    p.retention_period = 60s;
    p.archive_after    = 5s;
    mgr.registerPolicy(p);

    auto old_time = std::chrono::system_clock::now() - 10s;
    bool handler_called = false;
    auto action = mgr.archiveEntity("entity_1", "exec",
        [&](const std::string&) { handler_called = true; return true; });

    EXPECT_TRUE(action.success);
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(action.action, "archived");
}

TEST(RetentionExecution, PurgeEntitySuccess) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "purge_exec";
    p.retention_period   = 5s;
    p.archive_after      = 2s;
    p.auto_purge_enabled = true;
    mgr.registerPolicy(p);

    bool handler_called = false;
    auto action = mgr.purgeEntity("entity_2", "purge_exec",
        [&](const std::string&) { handler_called = true; return true; });

    EXPECT_TRUE(action.success);
    EXPECT_TRUE(handler_called);
    EXPECT_EQ(action.action, "purged");
}

TEST(RetentionExecution, ArchiveHandlerFailureReflectedInAction) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "fail_arch";
    p.retention_period = 60s;
    p.archive_after    = 5s;
    mgr.registerPolicy(p);

    auto action = mgr.archiveEntity("e", "fail_arch",
        [](const std::string&) { return false; }); // handler fails

    EXPECT_FALSE(action.success);
}

// ============================================================================
// runRetentionCheck (batch)
// ============================================================================

TEST(RetentionCheck, BatchProcessesEntities) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "batch";
    p.retention_period   = 5s;
    p.archive_after      = 2s;
    p.auto_purge_enabled = true;
    mgr.registerPolicy(p);

    std::atomic<int> archived{0}, purged{0};
    auto now = std::chrono::system_clock::now();

    auto stats = mgr.runRetentionCheck(
        // entity_provider: return 3 old entities for "batch" policy
        [&](const std::string& policy) -> std::vector<std::pair<std::string, std::chrono::system_clock::time_point>> {
            if (policy != "batch") return {};
            return {
                {"e1", now - 60s},
                {"e2", now - 30s},
                {"e3", now - 10s}
            };
        },
        [&](const std::string&) { ++archived; return true; },
        [&](const std::string&) { ++purged;   return true; }
    );

    EXPECT_GE(stats.total_entities_scanned, 3u);
}

// ============================================================================
// History tracking
// ============================================================================

TEST(RetentionHistory, HistoryRecordsActions) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "hist";
    p.retention_period = 60s;
    p.archive_after    = 1s;
    mgr.registerPolicy(p);

    mgr.archiveEntity("e1", "hist", [](const std::string&) { return true; });
    mgr.archiveEntity("e2", "hist", [](const std::string&) { return true; });

    auto history = mgr.getHistory();
    EXPECT_GE(history.size(), 2u);
}

TEST(RetentionHistory, HistoryLimitWorks) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "hist_lim";
    p.retention_period = 60s;
    p.archive_after    = 1s;
    mgr.registerPolicy(p);

    for (int i = 0; i < 10; ++i) {
        mgr.archiveEntity("e" + std::to_string(i), "hist_lim",
            [](const std::string&) { return true; });
    }

    auto history = mgr.getHistory(3);
    EXPECT_EQ(history.size(), 3u);
}

// ============================================================================
// Async background job
// ============================================================================

TEST(RetentionBackgroundJob, StartStopLifecycle) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "bg";
    p.retention_period   = 5s;
    p.archive_after      = 1s;
    p.auto_purge_enabled = false;
    mgr.registerPolicy(p);

    EXPECT_FALSE(mgr.isBackgroundJobRunning());

    mgr.startBackgroundJob(
        1h,  // Long interval so it doesn't fire during test
        [](const std::string&) { return std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>{}; },
        [](const std::string&) { return true; },
        [](const std::string&) { return true; }
    );
    EXPECT_TRUE(mgr.isBackgroundJobRunning());

    mgr.stopBackgroundJob();
    EXPECT_FALSE(mgr.isBackgroundJobRunning());
}

TEST(RetentionBackgroundJob, StartTwiceIsNoop) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name             = "bg2";
    p.retention_period = 5s;
    p.archive_after    = 1s;
    mgr.registerPolicy(p);

    mgr.startBackgroundJob(
        1h,
        [](const std::string&) { return std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>{}; },
        [](const std::string&) { return true; },
        [](const std::string&) { return true; }
    );
    mgr.startBackgroundJob(  // Second call – should be a no-op
        1h,
        [](const std::string&) { return std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>{}; },
        [](const std::string&) { return true; },
        [](const std::string&) { return true; }
    );
    EXPECT_TRUE(mgr.isBackgroundJobRunning());
    mgr.stopBackgroundJob();
}

TEST(RetentionBackgroundJob, ComplianceMetricsUpdated) {
    RetentionManager mgr;
    RetentionManager::RetentionPolicy p;
    p.name               = "metrics";
    p.retention_period   = 1s;
    p.archive_after      = 0s;
    p.auto_purge_enabled = false;
    mgr.registerPolicy(p);

    std::atomic<int> runs{0};
    auto now = std::chrono::system_clock::now();

    mgr.startBackgroundJob(
        1s,
        [&](const std::string& pol) {
            ++runs;
            return std::vector<std::pair<std::string, std::chrono::system_clock::time_point>>{
                {"e1", now - 5s}
            };
        },
        [](const std::string&) { return true; },
        [](const std::string&) { return true; }
    );

    // Give the background job time to run at least once
    std::this_thread::sleep_for(1200ms);
    mgr.stopBackgroundJob();

    auto m = mgr.getComplianceMetrics();
    EXPECT_GE(m.policies_active, 1u);
}

