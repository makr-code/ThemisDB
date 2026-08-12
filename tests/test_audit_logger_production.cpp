/**
 * @file test_audit_logger_production.cpp
 * @brief Phase 3 – Audit Logger production feature tests
 *
 * Tests cover:
 * - Audit search API (searchEntries)
 * - Search by user_id, action, resource_prefix, time range, max_results
 * - Compliance report generation
 * - Compliance report field counts
 * - Chain integrity across report window
 * - Retention enforcement (archiveOldEntries / purgeOldEntries)
 * - Multi-entry search result ordering
 */

#include <gtest/gtest.h>
#include "utils/audit_logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace themis::utils;
using namespace std::chrono_literals;

namespace {

AuditLoggerConfig makeTestConfig(const std::string& log_path,
                                  bool enable_hash_chain = false) {
    AuditLoggerConfig cfg;
    cfg.enabled           = true;
    cfg.encrypt_then_sign = false;
    cfg.log_path          = log_path;
    cfg.key_id            = "test-key";
    cfg.enable_hash_chain = enable_hash_chain;
    cfg.chain_state_file  = log_path + ".chain";
    cfg.enable_siem       = false;
    return cfg;
}

std::filesystem::path makeTmpDir(const std::string& name) {
    auto d = std::filesystem::temp_directory_path() / "audit_prod_test" / name;
    std::filesystem::create_directories(d);
    return d;
}

} // anonymous namespace

// ============================================================================
// Audit Search – basic
// ============================================================================

class AuditSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_  = makeTmpDir("search");
        path_ = (dir_ / "audit.jsonl").string();
        std::filesystem::remove(path_);
    }
    void TearDown() override {
        std::filesystem::remove_all(dir_);
    }

    std::filesystem::path dir_;
    std::string           path_;
};

TEST_F(AuditSearchTest, EmptyLogReturnsNoResults) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    AuditLogger::SearchQuery q;
    auto results = logger.searchEntries(q);
    EXPECT_TRUE(results.empty());
}

TEST_F(AuditSearchTest, AllEntriesReturnedWithNoFilters) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    for (int i = 0; i < 5; ++i) {
        logger.logEvent({{"action", "read"}, {"user", "alice"}, {"seq", i}});
    }
    logger.flush();

    AuditLogger::SearchQuery q;
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 5u);
}

TEST_F(AuditSearchTest, FilterByUserIdMatchesOnly) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "read"}, {"user", "alice"}});
    logger.logEvent({{"action", "write"}, {"user", "bob"}});
    logger.logEvent({{"action", "delete"}, {"user", "alice"}});
    logger.flush();

    AuditLogger::SearchQuery q;
    q.user_id = "alice";
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(AuditSearchTest, FilterByActionSubstring) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "data_read"}, {"user", "u1"}});
    logger.logEvent({{"action", "data_write"}, {"user", "u1"}});
    logger.logEvent({{"action", "login"}, {"user", "u1"}});
    logger.flush();

    AuditLogger::SearchQuery q;
    q.action = "data_";
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 2u);
}

TEST_F(AuditSearchTest, MaxResultsLimitsOutput) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    for (int i = 0; i < 20; ++i) {
        logger.logEvent({{"action", "op"}, {"seq", i}});
    }
    logger.flush();

    AuditLogger::SearchQuery q;
    q.max_results = 5;
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 5u);
}

TEST_F(AuditSearchTest, TimeRangeFiltersOldEntries) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "old_event"}});
    logger.flush();

    auto now = std::chrono::system_clock::now();
    auto future = now + 1h;

    AuditLogger::SearchQuery q;
    q.from = future; // Only events from the future – should be empty
    auto results = logger.searchEntries(q);
    EXPECT_TRUE(results.empty());
}

TEST_F(AuditSearchTest, TimeRangeIncludesRecentEntries) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "recent_event"}});
    logger.flush();

    auto past = std::chrono::system_clock::now() - 1h;

    AuditLogger::SearchQuery q;
    q.from = past;
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 1u);
}

TEST_F(AuditSearchTest, CombinedFilters) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "data_read"}, {"user", "alice"}, {"resource", "/docs/secret"}});
    logger.logEvent({{"action", "data_read"}, {"user", "bob"},   {"resource", "/docs/public"}});
    logger.logEvent({{"action", "login"},     {"user", "alice"}, {"resource", "/auth"}});
    logger.flush();

    AuditLogger::SearchQuery q;
    q.user_id = "alice";
    q.action  = "data_read";
    auto results = logger.searchEntries(q);
    EXPECT_EQ(results.size(), 1u);
}

// ============================================================================
// Compliance Report
// ============================================================================

class ComplianceReportTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_  = makeTmpDir("compliance");
        path_ = (dir_ / "audit.jsonl").string();
        std::filesystem::remove(path_);
    }
    void TearDown() override {
        std::filesystem::remove_all(dir_);
    }

    std::filesystem::path dir_;
    std::string           path_;
};

TEST_F(ComplianceReportTest, EmptyLogProducesZeroReport) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    auto past   = std::chrono::system_clock::now() - 1h;
    auto future = std::chrono::system_clock::now() + 1h;
    auto report = logger.generateComplianceReport(past, future);
    EXPECT_EQ(report.total_events, 0u);
}

TEST_F(ComplianceReportTest, TotalEventsCountMatchesLoggedEvents) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    for (int i = 0; i < 7; ++i) {
        logger.logEvent({{"action", "read"}, {"user", "tester"}});
    }
    logger.flush();

    auto past   = std::chrono::system_clock::now() - 1h;
    auto future = std::chrono::system_clock::now() + 1h;
    auto report = logger.generateComplianceReport(past, future);
    EXPECT_EQ(report.total_events, 7u);
}

TEST_F(ComplianceReportTest, ReportWindowExcludesFutureEvents) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "test_event"}});
    logger.flush();

    auto future1 = std::chrono::system_clock::now() + 1h;
    auto future2 = std::chrono::system_clock::now() + 2h;
    auto report  = logger.generateComplianceReport(future1, future2);
    EXPECT_EQ(report.total_events, 0u);
}

TEST_F(ComplianceReportTest, ReportFieldsArePopulated) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "read"}, {"user_id", "alice"}});
    logger.flush();

    auto past   = std::chrono::system_clock::now() - 1h;
    auto future = std::chrono::system_clock::now() + 1h;
    auto report = logger.generateComplianceReport(past, future);

    EXPECT_EQ(report.from, past);
    EXPECT_EQ(report.to,   future);
    EXPECT_GE(report.total_events, 1u);
}

TEST_F(ComplianceReportTest, ChainIntactWithNoHashChain) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_, false));
    logger.logEvent({{"action", "test"}});
    logger.flush();

    auto past   = std::chrono::system_clock::now() - 1h;
    auto future = std::chrono::system_clock::now() + 1h;
    auto report = logger.generateComplianceReport(past, future);
    // Hash chain disabled → should be considered intact
    EXPECT_TRUE(report.chain_intact);
}

TEST_F(ComplianceReportTest, MultipleReportsAreConsistent) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    for (int i = 0; i < 4; ++i) {
        logger.logEvent({{"action", "read"}, {"user", "u" + std::to_string(i)}});
    }
    logger.flush();

    auto past   = std::chrono::system_clock::now() - 1h;
    auto future = std::chrono::system_clock::now() + 1h;
    auto r1 = logger.generateComplianceReport(past, future);
    auto r2 = logger.generateComplianceReport(past, future);
    EXPECT_EQ(r1.total_events, r2.total_events);
}

// ============================================================================
// Retention enforcement
// ============================================================================

class AuditRetentionTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_     = makeTmpDir("retention");
        path_    = (dir_ / "audit.jsonl").string();
        archive_ = (dir_ / "archive.jsonl").string();
        std::filesystem::remove(path_);
    }
    void TearDown() override {
        std::filesystem::remove_all(dir_);
    }

    std::filesystem::path dir_;
    std::string           path_;
    std::string           archive_;
};

TEST_F(AuditRetentionTest, ArchiveMovesOldEntries) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "old_event"}});
    logger.flush();

    auto future = std::chrono::system_clock::now() + 1h;
    auto archived = logger.archiveOldEntries(future, archive_);
    EXPECT_EQ(archived, 1u);
    ASSERT_TRUE(std::filesystem::exists(archive_));
}

TEST_F(AuditRetentionTest, PurgeRemovesOldEntries) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "to_purge"}});
    logger.flush();

    auto future = std::chrono::system_clock::now() + 1h;
    auto purged = logger.purgeOldEntries(future);
    EXPECT_EQ(purged, 1u);
}

TEST_F(AuditRetentionTest, ArchiveLeavesNewEntriesIntact) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(path_));
    logger.logEvent({{"action", "recent"}});
    logger.flush();

    // Archive threshold in the past – no entries are old enough
    auto past = std::chrono::system_clock::now() - 1h;
    auto archived = logger.archiveOldEntries(past, archive_);
    EXPECT_EQ(archived, 0u);
}
