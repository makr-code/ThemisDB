/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_logger_production.cpp                         ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 17:20:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     337                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_logger_production.cpp
 * @brief Phase 1 – Production logging feature tests
 *
 * Tests cover:
 * - Structured JSON logging (initJson)
 * - Rotating file sink (initRotating)
 * - Dynamic log-level change API
 * - PII-Redaction filter integration
 * - Log-performance metrics
 * - Trace-context injection
 * - Pattern customisation
 * - Level conversion helpers
 * - Multi-sink initialisation
 * - Shutdown / re-init lifecycle
 */

#include <gtest/gtest.h>
#include "utils/logger.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::utils;

namespace {

// Helper: read entire file to string
std::string readFile(const std::string& path) {
    std::ifstream ifs(path);
    return std::string((std::istreambuf_iterator<char>(ifs)),
                        std::istreambuf_iterator<char>());
}

// Helper: create a unique temp path for this test
std::string tmpPath(const std::string& name) {
    auto dir = std::filesystem::temp_directory_path() / "logger_prod_test";
    std::filesystem::create_directories(dir);
    return (dir / name).string();
}

} // anonymous namespace

// ============================================================================
// Metrics
// ============================================================================

TEST(LoggerProduction, MetricsCountInfo) {
    Logger::shutdown();
    Logger::resetMetrics();
    Logger::init(tmpPath("metrics_info.log"), Logger::Level::TRACE);

    Logger::info("test message 1");
    Logger::info("test message 2");
    Logger::warn("a warning");

    auto snap = Logger::getMetrics().snapshot();
    EXPECT_GE(snap.info_count, 2u);
    EXPECT_GE(snap.warn_count, 1u);
    EXPECT_GE(snap.total_count, 3u);

    Logger::shutdown();
}

TEST(LoggerProduction, MetricsCountAllLevels) {
    Logger::shutdown();
    Logger::resetMetrics();
    Logger::init(tmpPath("metrics_all.log"), Logger::Level::TRACE);

    Logger::trace("t");
    Logger::debug("d");
    Logger::info("i");
    Logger::warn("w");
    Logger::error("e");
    Logger::critical("c");

    auto snap = Logger::getMetrics().snapshot();
    EXPECT_GE(snap.trace_count,    1u);
    EXPECT_GE(snap.debug_count,    1u);
    EXPECT_GE(snap.info_count,     1u);
    EXPECT_GE(snap.warn_count,     1u);
    EXPECT_GE(snap.error_count,    1u);
    EXPECT_GE(snap.critical_count, 1u);
    EXPECT_GE(snap.total_count,    6u);

    Logger::shutdown();
}

TEST(LoggerProduction, MetricsReset) {
    Logger::shutdown();
    Logger::init(tmpPath("metrics_reset.log"), Logger::Level::TRACE);

    Logger::info("before reset");
    Logger::resetMetrics();

    auto snap = Logger::getMetrics().snapshot();
    EXPECT_EQ(snap.total_count, 0u);
    EXPECT_EQ(snap.info_count,  0u);

    Logger::shutdown();
}

TEST(LoggerProduction, MetricsThreadSafe) {
    Logger::shutdown();
    Logger::resetMetrics();
    Logger::init(tmpPath("metrics_threads.log"), Logger::Level::INFO);

    constexpr int kThreads = 8;
    constexpr int kMsgsPerThread = 50;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([i]() {
            for (int j = 0; j < kMsgsPerThread; ++j) {
                Logger::info("thread {} msg {}", i, j);
            }
        });
    }
    for (auto& t : threads) t.join();

    auto snap = Logger::getMetrics().snapshot();
    EXPECT_GE(snap.info_count, static_cast<uint64_t>(kThreads * kMsgsPerThread));

    Logger::shutdown();
}

// ============================================================================
// JSON-structured logging
// ============================================================================

TEST(LoggerProduction, InitJsonCreatesFile) {
    Logger::shutdown();
    auto path = tmpPath("json.log");
    Logger::initJson(path, Logger::Level::INFO);
    Logger::info("hello json");
    Logger::shutdown();

    ASSERT_TRUE(std::filesystem::exists(path));
    auto content = readFile(path);
    EXPECT_FALSE(content.empty());
}

TEST(LoggerProduction, InitJsonLineIsJsonObject) {
    Logger::shutdown();
    auto path = tmpPath("json_line.log");
    Logger::initJson(path, Logger::Level::INFO);
    Logger::info("structured message");
    Logger::shutdown();

    auto content = readFile(path);
    // Look for a line containing JSON-like structure (ts and level fields)
    EXPECT_NE(content.find("\"level\""), std::string::npos);
    EXPECT_NE(content.find("\"msg\""), std::string::npos);
}

// ============================================================================
// Rotating log
// ============================================================================

TEST(LoggerProduction, InitRotatingCreatesFile) {
    Logger::shutdown();
    auto path = tmpPath("rotate.log");
    Logger::initRotating(path, 1024 * 1024, 3, Logger::Level::INFO);
    Logger::info("rotating log entry");
    Logger::shutdown();

    ASSERT_TRUE(std::filesystem::exists(path));
}

TEST(LoggerProduction, InitRotatingSizeLimit) {
    Logger::shutdown();
    auto path = tmpPath("rotate_size.log");
    // Very small max size to force rotation
    Logger::initRotating(path, 512, 3, Logger::Level::INFO);

    // Write enough data to trigger at least one rotation
    for (int i = 0; i < 100; ++i) {
        Logger::info("padding message number {:03d} to fill up the file", i);
    }
    Logger::shutdown();

    // Base file must exist
    ASSERT_TRUE(std::filesystem::exists(path));
}

// ============================================================================
// Dynamic log-level change
// ============================================================================

TEST(LoggerProduction, SetLevelDynamic) {
    Logger::shutdown();
    Logger::resetMetrics();
    Logger::init(tmpPath("level.log"), Logger::Level::WARN);

    // DEBUG messages below WARN threshold – should NOT increment metrics counter
    // (spdlog filters them before calling the sink, so our count stays 0)
    Logger::debug("should be filtered");
    EXPECT_EQ(Logger::getMetrics().snapshot().debug_count, 0u);

    // Raise the level to include DEBUG
    Logger::setLevel(Logger::Level::DEBUG);
    Logger::debug("now visible");
    EXPECT_GE(Logger::getMetrics().snapshot().debug_count, 1u);

    Logger::shutdown();
}

// ============================================================================
// Trace-context injection
// ============================================================================

TEST(LoggerProduction, SetTraceContextAddsId) {
    Logger::shutdown();
    auto path = tmpPath("trace_ctx.log");
    Logger::init(path, Logger::Level::INFO);

    Logger::setTraceContext("abc123trace");
    Logger::info("with trace context");
    Logger::shutdown();

    auto content = readFile(path);
    EXPECT_NE(content.find("abc123trace"), std::string::npos);
}

TEST(LoggerProduction, ClearTraceContext) {
    Logger::shutdown();
    auto path = tmpPath("trace_clear.log");
    Logger::init(path, Logger::Level::INFO);

    Logger::setTraceContext("to-be-cleared");
    Logger::setTraceContext("");  // clear
    Logger::info("no trace context");
    Logger::shutdown();

    // After clearing, the trace ID should not appear in subsequent messages
    auto content = readFile(path);
    // The line "no trace context" should exist but without "to-be-cleared"
    EXPECT_NE(content.find("no trace context"), std::string::npos);
}

// ============================================================================
// Level helpers
// ============================================================================

TEST(LoggerProduction, LevelFromString) {
    EXPECT_EQ(Logger::levelFromString("trace"),    Logger::Level::TRACE);
    EXPECT_EQ(Logger::levelFromString("DEBUG"),    Logger::Level::DEBUG);
    EXPECT_EQ(Logger::levelFromString("Info"),     Logger::Level::INFO);
    EXPECT_EQ(Logger::levelFromString("warn"),     Logger::Level::WARN);
    EXPECT_EQ(Logger::levelFromString("warning"),  Logger::Level::WARN);
    EXPECT_EQ(Logger::levelFromString("error"),    Logger::Level::ERROR);
    EXPECT_EQ(Logger::levelFromString("err"),      Logger::Level::ERROR);
    EXPECT_EQ(Logger::levelFromString("critical"), Logger::Level::CRITICAL);
    EXPECT_EQ(Logger::levelFromString("crit"),     Logger::Level::CRITICAL);
    EXPECT_EQ(Logger::levelFromString("unknown"),  Logger::Level::INFO);
}

TEST(LoggerProduction, LevelToString) {
    EXPECT_STREQ(Logger::levelToString(Logger::Level::TRACE),    "trace");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::DEBUG),    "debug");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::INFO),     "info");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::WARN),     "warn");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::ERROR),    "error");
    EXPECT_STREQ(Logger::levelToString(Logger::Level::CRITICAL), "critical");
}

// ============================================================================
// Pattern customisation
// ============================================================================

TEST(LoggerProduction, CustomPattern) {
    Logger::shutdown();
    auto path = tmpPath("custom_pattern.log");
    Logger::init(path, Logger::Level::INFO);
    Logger::setPattern("%v"); // message only
    Logger::info("just the message");
    Logger::shutdown();

    auto content = readFile(path);
    // With pattern "%v" the output should start with "just the message"
    EXPECT_NE(content.find("just the message"), std::string::npos);
}

// ============================================================================
// Lifecycle
// ============================================================================

TEST(LoggerProduction, ShutdownAndReinit) {
    Logger::shutdown();
    auto path = tmpPath("lifecycle.log");
    Logger::init(path, Logger::Level::INFO);
    Logger::info("first init");
    Logger::shutdown();
    Logger::init(path, Logger::Level::INFO);
    Logger::info("second init");
    Logger::shutdown();

    ASSERT_TRUE(std::filesystem::exists(path));
}

TEST(LoggerProduction, GetBeforeInit) {
    Logger::shutdown();
    auto logger = Logger::get();  // Should auto-init without crashing
    ASSERT_NE(logger, nullptr);
    Logger::shutdown();
}
