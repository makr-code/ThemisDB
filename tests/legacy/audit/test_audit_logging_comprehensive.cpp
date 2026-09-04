/**
 * @file test_audit_logging_comprehensive.cpp
 * @brief Comprehensive tests for the AuditLogger security component
 *
 * Tests cover:
 * - Basic event logging
 * - Security event logging (all event types)
 * - Hash chain integrity
 * - Chain state persistence and verification
 * - Log file creation and append behavior
 * - Multiple sequential events
 * - Concurrent logging
 * - Configuration options
 */

#include <gtest/gtest.h>
#include "utils/audit_logger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <sstream>

using namespace themis::utils;

namespace {

AuditLoggerConfig makeTestConfig(const std::string& log_path,
                                  bool enable_hash_chain = true) {
    AuditLoggerConfig cfg;
    cfg.enabled = true;
    cfg.encrypt_then_sign = false; // No encryption for unit tests
    cfg.log_path = log_path;
    cfg.key_id = "test-key";
    cfg.enable_hash_chain = enable_hash_chain;
    cfg.chain_state_file = log_path + ".chain";
    cfg.enable_siem = false;
    return cfg;
}

// Count lines in a file
size_t countLines(const std::string& path) {
    std::ifstream f(path);
    size_t lines = 0;
    std::string line = {};
    while (std::getline(f, line)) {
        if (!line.empty()) {
          ++lines;
        }
    }
    return lines;
}

} // anonymous namespace

class AuditLoggerComprehensiveTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        const auto nonce = static_cast<long long>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        tmp_dir_ = std::filesystem::temp_directory_path() /
                   (std::string("audit_test_") + info->test_suite_name() + "_" + info->name() + "_" + std::to_string(nonce));
        std::filesystem::create_directories(tmp_dir_);
        log_path_ = (tmp_dir_ / "audit.jsonl").string();
        chain_path_ = log_path_ + ".chain";
    }

    void TearDown() override {
        std::error_code ec = {};
        std::filesystem::remove_all(tmp_dir_, ec);
    }

    std::filesystem::path tmp_dir_;
    std::string log_path_ = {};
    std::string chain_path_;
};

// ============================================================================
// Basic Logging Tests
// ============================================================================

TEST_F(AuditLoggerComprehensiveTest, LogEvent_CreatesFile) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logEvent({{"action", "test"}, {"user", "alice"}});
    logger.flush();

    EXPECT_TRUE(std::filesystem::exists(log_path_));
}

TEST_F(AuditLoggerComprehensiveTest, LogEvent_AppendMultipleEntries) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));

    for (int i = 0; i < 5; ++i) {
        logger.logEvent({{"seq", i}, {"action", "test"}});
    }
    logger.flush();

    EXPECT_GE(countLines(log_path_), 5u);
}

TEST_F(AuditLoggerComprehensiveTest, LogEvent_DisabledLogger_NoFileCreated) {
    auto cfg = makeTestConfig(log_path_);
    cfg.enabled = false;
    AuditLogger logger(nullptr, nullptr, cfg);

    logger.logEvent({{"action", "should not log"}});
    logger.flush();

    EXPECT_FALSE(std::filesystem::exists(log_path_));
}

TEST_F(AuditLoggerComprehensiveTest, LogEvent_RecordContainsTimestamp) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logEvent({{"action", "test"}});
    logger.flush();

    std::ifstream f(log_path_);
    std::string line = {};
    std::getline(f, line);
    ASSERT_FALSE(line.empty());

    auto j = nlohmann::json::parse(line);
    EXPECT_TRUE(j.contains("ts"));
    EXPECT_GT(j["ts"].get<uint64_t>(), 0u);
}

// ============================================================================
// Security Event Logging Tests
// ============================================================================

TEST_F(AuditLoggerComprehensiveTest, LogSecurityEvent_LoginSuccess) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logSecurityEvent(
        SecurityEventType::LOGIN_SUCCESS,
        "alice",
        "authentication",
        {{"method", "password"}}
    );
    logger.flush();

    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuditLoggerComprehensiveTest, LogSecurityEvent_LoginFailed) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logSecurityEvent(
        SecurityEventType::LOGIN_FAILED,
        "bob",
        "authentication",
        {{"reason", "invalid_password"}}
    );
    logger.flush();

    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuditLoggerComprehensiveTest, LogSecurityEvent_PermissionDenied) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logSecurityEvent(
        SecurityEventType::PERMISSION_DENIED,
        "user1",
        "/admin/keys",
        {{"action", "delete"}, {"roles", {"readonly"}}}
    );
    logger.flush();

    EXPECT_GE(countLines(log_path_), 1u);
}

TEST_F(AuditLoggerComprehensiveTest, LogSecurityEvent_MultipleTypes) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));

    std::vector<SecurityEventType> events = {
        SecurityEventType::LOGIN_SUCCESS,
        SecurityEventType::DATA_READ,
        SecurityEventType::DATA_WRITE,
        SecurityEventType::LOGOUT,
        SecurityEventType::CONFIG_CHANGED,
        SecurityEventType::ROLE_CHANGED,
        SecurityEventType::KEY_ROTATED,
        SecurityEventType::BRUTE_FORCE_DETECTED,
        SecurityEventType::RATE_LIMIT_EXCEEDED,
        SecurityEventType::SUSPICIOUS_ACTIVITY,
    };

    for (auto evt : events) {
        logger.logSecurityEvent(evt, "test-user", "resource");
    }
    logger.flush();

    EXPECT_GE(countLines(log_path_), events.size());
}

TEST_F(AuditLoggerComprehensiveTest, LogSecurityEvent_WithDetails) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    nlohmann::json details = {
        {"ip", "192.168.1.1"},
        {"user_agent", "TestAgent/1.0"},
        {"resource_id", "doc-12345"}
    };
    logger.logSecurityEvent(
        SecurityEventType::DATA_READ,
        "analyst",
        "data/reports",
        details
    );
    logger.flush();

    EXPECT_GE(countLines(log_path_), 1u);
}

// ============================================================================
// Hash Chain Tests
// ============================================================================

TEST_F(AuditLoggerComprehensiveTest, HashChain_InitialChainStateEmpty) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_));
    auto state = logger.getChainState();
    EXPECT_TRUE(state.contains("entry_count"));
    EXPECT_EQ(state["entry_count"].get<uint64_t>(), 0u);
}

TEST_F(AuditLoggerComprehensiveTest, HashChain_EntryCountIncrements) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_));

    for (int i = 0; i < 3; ++i) {
        logger.logEvent({{"seq", i}});
    }
    logger.flush();

    auto state = logger.getChainState();
    EXPECT_GE(state["entry_count"].get<uint64_t>(), 3u);
}

TEST_F(AuditLoggerComprehensiveTest, HashChain_VerifyIntegrity_ValidChain) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_));

    logger.logEvent({{"action", "init"}});
    logger.logEvent({{"action", "update"}});
    logger.logEvent({{"action", "finalize"}});
    logger.flush();

    EXPECT_TRUE(logger.verifyChainIntegrity());
}

TEST_F(AuditLoggerComprehensiveTest, HashChain_LogEntriesContainPrevHash) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_));

    logger.logEvent({{"action", "first"}});
    logger.logEvent({{"action", "second"}});
    logger.flush();

    // Verify log file contains chain entries
    std::ifstream f(log_path_);
    std::string line = {};
    bool has_chain_entry = false;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            auto j = nlohmann::json::parse(line);
            if (j.contains("chain_entry")) {
                has_chain_entry = true;
                EXPECT_TRUE(j.contains("prev_hash"));
                break;
            }
        }
    }
    EXPECT_TRUE(has_chain_entry);
}

TEST_F(AuditLoggerComprehensiveTest, HashChain_WithoutChain_NoChainEntry) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logEvent({{"action", "test"}});
    logger.flush();

    std::ifstream f(log_path_);
    std::string line = {};
    std::getline(f, line);
    ASSERT_FALSE(line.empty());

    auto j = nlohmann::json::parse(line);
    // No hash chain fields when disabled
    EXPECT_FALSE(j.contains("chain_entry"));
}

// ============================================================================
// Concurrent Logging Tests
// ============================================================================

TEST_F(AuditLoggerComprehensiveTest, ConcurrentLogging_ThreadSafe) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));

    constexpr int THREADS = 4;
    constexpr int EVENTS_PER_THREAD = 10;
    std::atomic<int> completed{0};
    std::vector<std::thread> threads;

    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back([&logger, &completed, i]() {
            for (int j = 0; j < EVENTS_PER_THREAD; ++j) {
                logger.logSecurityEvent(
                    SecurityEventType::DATA_READ,
                    "user-" + std::to_string(i),
                    "/data",
                    {{"seq", j}}
                );
            }
            completed++;
        });
    }

    for (auto& t : threads) {
      t.join();
    }
    logger.flush();

    EXPECT_EQ(completed.load(), THREADS);
    EXPECT_GE(countLines(log_path_), static_cast<size_t>(THREADS * EVENTS_PER_THREAD));
}

// ============================================================================
// Compliance / Structured Log Tests
// ============================================================================

TEST_F(AuditLoggerComprehensiveTest, LogEntries_ContainCategoryField) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));
    logger.logEvent({{"user", "alice"}, {"action", "login"}});
    logger.flush();

    std::ifstream f(log_path_);
    std::string line = {};
    std::getline(f, line);
    ASSERT_FALSE(line.empty());

    auto j = nlohmann::json::parse(line);
    EXPECT_EQ(j["category"].get<std::string>(), "AUDIT");
}

TEST_F(AuditLoggerComprehensiveTest, SecurityEvents_SystemAccountAudit) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));

    // Server lifecycle events
    logger.logSecurityEvent(SecurityEventType::SERVER_STARTED, "system", "server");
    logger.logSecurityEvent(SecurityEventType::SERVER_STOPPED, "system", "server");
    logger.flush();

    EXPECT_GE(countLines(log_path_), 2u);
}

TEST_F(AuditLoggerComprehensiveTest, SecurityEvents_KeyManagement) {
    AuditLogger logger(nullptr, nullptr, makeTestConfig(log_path_, false));

    logger.logSecurityEvent(SecurityEventType::KEY_CREATED, "admin", "key-001");
    logger.logSecurityEvent(SecurityEventType::KEY_ROTATED, "admin", "key-001");
    logger.logSecurityEvent(SecurityEventType::KEY_DELETED, "admin", "key-old");
    logger.flush();

    EXPECT_GE(countLines(log_path_), 3u);
}
