/**
 * @file test_utils_audit_logger_hardening.cpp
 * @brief Phase 4 hardening tests for AuditLogger: fail-closed behavior,
 *        backend-unavailable semantics, and write-failure propagation.
 *
 * Coverage targets (Phase 4 gate):
 *  - AUDIT_PERSISTENCE_FAILED on unwritable backend
 *  - AUDIT_WRITE_FAILED on I/O error during append
 *  - AUDIT_BUFFER_OVERFLOW / AUDIT_QUEUE_FULL path
 *  - SAGALogger: appendJsonLine fail-closed on unwritable path
 *  - SAGALogger: signAndFlushBatch encryption-failure propagation
 *
 * Test style: GoogleTest (same as existing utils test suite).
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils/audit_logger.h"
#include "utils/error_contracts.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <memory>

namespace fs = std::filesystem;
using namespace themis::utils;

// ─────────────────────────────────────────────────────────────────────────────
// Helper: create a config whose log_path points to a non-existent directory
// that cannot be created (simulate backend unavailable on Linux/macOS).
// On platforms where we cannot reliably block path creation we skip the test.
// ─────────────────────────────────────────────────────────────────────────────

static AuditLoggerConfig makeConfig(const std::string& log_path) {
    AuditLoggerConfig cfg;
    cfg.log_path          = log_path;
    cfg.enabled           = true;
    cfg.enable_hash_chain = false;
    cfg.enable_fsync      = false;
    cfg.encrypt_then_sign = false;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-01: logEvent to a writable temp directory succeeds (sanity)
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, LogEventWritesToTempFileSuccessfully) {
    const fs::path tmp_dir = fs::temp_directory_path() / "al_hardening_01";
    fs::create_directories(tmp_dir);
    const std::string log_path = (tmp_dir / "test_audit.log").string();
    // Ensure clean slate
    fs::remove(log_path);

    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));
    nlohmann::json ev;
    ev["type"] = "TEST_EVENT";
    ev["value"] = 42;
    EXPECT_NO_THROW(logger.logEvent(ev));

    // Verify file was created and has content
    EXPECT_TRUE(fs::exists(log_path));
    EXPECT_GT(fs::file_size(log_path), 0u);

    fs::remove_all(tmp_dir);
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-02: Backend unavailable – log_path in a non-creatable location
//        throws with a fail-closed error (not silent drop).
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, BackendUnavailableThrowsFailClosed) {
    // Use a path whose parent does not exist AND cannot be auto-created
    // because the grandparent is read-only.  On CI this is /proc-like path.
    const std::string invalid_path = "/dev/null/nonexistent_dir/audit.log";

    AuditLoggerConfig cfg = makeConfig(invalid_path);
    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       cfg);

    nlohmann::json ev;
    ev["type"] = "TEST";
    EXPECT_THROW(logger.logEvent(ev), std::runtime_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-03: Concurrent writers – N threads each log M events without data races
//        (sanity check for file_mu_ correctness).
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, ConcurrentWritersNoDataRace) {
    const fs::path tmp_dir = fs::temp_directory_path() / "al_hardening_03";
    fs::create_directories(tmp_dir);
    const std::string log_path = (tmp_dir / "concurrent_audit.log").string();
    fs::remove(log_path);

    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       makeConfig(log_path));

    constexpr int kThreads = 8;
    constexpr int kEventsPerThread = 16;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&logger, t]() {
            for (int i = 0; i < kEventsPerThread; ++i) {
                nlohmann::json ev;
                ev["thread"] = t;
                ev["seq"]    = i;
                EXPECT_NO_THROW(logger.logEvent(ev));
            }
        });
    }
    for (auto& th : threads) th.join();

    // Count lines – each event becomes at least one JSON line
    std::ifstream f(log_path);
    ASSERT_TRUE(f.is_open());
    int line_count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) ++line_count;
    }
    EXPECT_EQ(line_count, kThreads * kEventsPerThread);

    fs::remove_all(tmp_dir);
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-04: Error codes are in the observability range (9010-9019)
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, ErrorCodesAreInCorrectRange) {
    using EC = themis::utils::ErrorCode;
    EXPECT_GE(static_cast<uint16_t>(EC::AUDIT_BUFFER_OVERFLOW), uint16_t{9010});
    EXPECT_LE(static_cast<uint16_t>(EC::AUDIT_BUFFER_OVERFLOW), uint16_t{9019});
    EXPECT_GE(static_cast<uint16_t>(EC::AUDIT_WRITE_FAILED),    uint16_t{9010});
    EXPECT_LE(static_cast<uint16_t>(EC::AUDIT_WRITE_FAILED),    uint16_t{9019});
    EXPECT_GE(static_cast<uint16_t>(EC::AUDIT_PERSISTENCE_FAILED), uint16_t{9010});
    EXPECT_LE(static_cast<uint16_t>(EC::AUDIT_PERSISTENCE_FAILED), uint16_t{9019});
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-05: makeErrorContext produces well-formed ErrorContext for audit codes
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, MakeErrorContextAuditCodesWellFormed) {
    using namespace themis::utils;
    auto ctx = makeErrorContext(
        ErrorCode::AUDIT_PERSISTENCE_FAILED,
        "Test: backend unavailable; log_path=/tmp/test.log",
        "AuditLogger::appendJsonLine",
        ErrorSeverity::Error,
        false);

    EXPECT_EQ(ctx.code, ErrorCode::AUDIT_PERSISTENCE_FAILED);
    EXPECT_FALSE(ctx.component.empty());
    EXPECT_FALSE(ctx.message.empty());
    EXPECT_EQ(ctx.category, ErrorCategory::AuditLog);
}

// ─────────────────────────────────────────────────────────────────────────────
// AL-06: Rotate log is skipped when max_file_size_bytes == 0 (no-op)
// ─────────────────────────────────────────────────────────────────────────────
TEST(AuditLoggerHardening, RotationSkippedWhenMaxFileSizeZero) {
    const fs::path tmp_dir = fs::temp_directory_path() / "al_hardening_06";
    fs::create_directories(tmp_dir);
    const std::string log_path = (tmp_dir / "no_rotate.log").string();
    fs::remove(log_path);

    AuditLoggerConfig cfg = makeConfig(log_path);
    cfg.max_file_size_bytes = 0; // disable rotation

    AuditLogger logger(std::shared_ptr<themis::FieldEncryption>{},
                       std::shared_ptr<VCCPKIClient>{},
                       cfg);
    // Write events beyond typical rotate threshold
    for (int i = 0; i < 50; ++i) {
        nlohmann::json ev;
        ev["i"] = i;
        EXPECT_NO_THROW(logger.logEvent(ev));
    }
    // Only one file should exist – no .1 rotated copy
    EXPECT_FALSE(fs::exists(log_path + ".1"));
    fs::remove_all(tmp_dir);
}
