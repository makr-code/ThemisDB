// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for StorageAuditLogger:
//   - Open creates a segment file in the given directory
//   - logPut / logDel / logCheckpoint / logRecovery / logCompaction / logSnapshot
//   - lastSequence() increments with each entry
//   - Log content format: timestamp, sequence, event token, key, extra
//   - Rotation: new segment created when max_file_bytes exceeded
//   - segmentCount() reflects rotation
//   - flush() succeeds without error
//   - eventName() maps all event types
//   - segmentName() produces expected filename
//   - Thread-safe concurrent logging

#include <gtest/gtest.h>
#include "storage/storage_audit_logger.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    return {std::istreambuf_iterator<char>(f), {}};
}

static std::string auditDir() {
    return (fs::temp_directory_path() /
            ("themis_audit_" +
             std::to_string(
                 std::chrono::system_clock::now().time_since_epoch().count())))
               .string();
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic open and log
// ─────────────────────────────────────────────────────────────────────────────

TEST(StorageAuditTest, Open_CreatesSegmentFile) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    {
        auto res = StorageAuditLogger::open(cfg);
        ASSERT_TRUE(res.has_value());
        EXPECT_EQ((*res)->segmentCount(), 1u);
        EXPECT_EQ((*res)->lastSequence(), 0u);
    }
    fs::remove_all(dir);
}

TEST(StorageAuditTest, LogPut_WritesEntryAndIncrementsSequence) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    std::string content;
    {
        auto res = StorageAuditLogger::open(cfg);
        ASSERT_TRUE(res.has_value());
        auto& logger = *res;
        EXPECT_TRUE(logger->logPut("user:42", "bytes=64").has_value());
        EXPECT_EQ(logger->lastSequence(), 1u);
        EXPECT_TRUE(logger->flush().has_value());
        std::string seg = (fs::path(dir) / StorageAuditLogger::segmentName(0)).string();
        content = readFile(seg);
    }
    EXPECT_NE(content.find("PUT"), std::string::npos);
    EXPECT_NE(content.find("user:42"), std::string::npos);
    EXPECT_NE(content.find("bytes=64"), std::string::npos);
    fs::remove_all(dir);
}

TEST(StorageAuditTest, LogDel_WritesDelLine) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    std::string content;
    {
        auto logger = *StorageAuditLogger::open(cfg);
        EXPECT_TRUE(logger->logDel("user:99").has_value());
        (void)logger->flush();
        content = readFile(
            (fs::path(dir) / StorageAuditLogger::segmentName(0)).string());
    }
    EXPECT_NE(content.find("DEL"), std::string::npos);
    EXPECT_NE(content.find("user:99"), std::string::npos);
    fs::remove_all(dir);
}

TEST(StorageAuditTest, AllEventTypes_LogSuccessfully) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    {
        auto logger = *StorageAuditLogger::open(cfg);
        EXPECT_TRUE(logger->logPut("k", "").has_value());
        EXPECT_TRUE(logger->logDel("k", "").has_value());
        EXPECT_TRUE(logger->logCheckpoint("seq=100").has_value());
        EXPECT_TRUE(logger->logRecovery("entries=50").has_value());
        EXPECT_TRUE(logger->logCompaction("range=all").has_value());
        EXPECT_TRUE(logger->logSnapshot("snap-2025").has_value());
        EXPECT_EQ(logger->lastSequence(), 6u);
    }
    fs::remove_all(dir);
}

TEST(StorageAuditTest, SequenceMonotonicallyIncreases) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    {
        auto logger = *StorageAuditLogger::open(cfg);
        for (int i = 0; i < 20; ++i) {
            (void)logger->logPut("key:" + std::to_string(i));
        }
        EXPECT_EQ(logger->lastSequence(), 20u);
    }
    fs::remove_all(dir);
}

// ─────────────────────────────────────────────────────────────────────────────
// Rotation
// ─────────────────────────────────────────────────────────────────────────────

TEST(StorageAuditTest, Rotation_NewSegmentCreatedWhenLimitReached) {
    std::string dir = auditDir();
    // Very small rotation threshold so it rotates quickly
    StorageAuditLogger::Config cfg{dir, /*max_file_bytes=*/200};
    {
        auto logger = *StorageAuditLogger::open(cfg);
        for (int i = 0; i < 50; ++i) {
            (void)logger->logPut("key-" + std::to_string(i), "extra-data-padding");
        }
        EXPECT_GT(logger->segmentCount(), 1u);
    }
    fs::remove_all(dir);
}

TEST(StorageAuditTest, SegmentCount_StartsAtOne) {
    std::string dir = auditDir();
    {
        auto logger = *StorageAuditLogger::open(StorageAuditLogger::Config{dir});
        EXPECT_EQ(logger->segmentCount(), 1u);
    }
    fs::remove_all(dir);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(StorageAuditTest, EventName_AllTypes) {
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::PUT),         "PUT");
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::DEL),         "DEL");
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::CHECKPOINT),  "CHECKPOINT");
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::RECOVERY),    "RECOVERY");
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::COMPACTION),  "COMPACTION");
    EXPECT_EQ(StorageAuditLogger::eventName(StorageAuditLogger::Event::SNAPSHOT),    "SNAPSHOT");
}

TEST(StorageAuditTest, SegmentName_Format) {
    EXPECT_EQ(StorageAuditLogger::segmentName(0),      "audit_000000.log");
    EXPECT_EQ(StorageAuditLogger::segmentName(1),      "audit_000001.log");
    EXPECT_EQ(StorageAuditLogger::segmentName(999999), "audit_999999.log");
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread safety
// ─────────────────────────────────────────────────────────────────────────────

TEST(StorageAuditTest, ConcurrentLogging_NoRaceConditions) {
    std::string dir = auditDir();
    StorageAuditLogger::Config cfg{dir};
    {
        auto logger = *StorageAuditLogger::open(cfg);
        constexpr int kThreads = 8;
        constexpr int kPerThread = 50;
        std::vector<std::thread> threads;
        threads.reserve(kThreads);
        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([&, t]() {
                for (int i = 0; i < kPerThread; ++i) {
                    (void)logger->logPut("thread:" + std::to_string(t) + ":" + std::to_string(i));
                }
            });
        }
        for (auto& th : threads) {
          th.join();
        }
        EXPECT_EQ(logger->lastSequence(),
                  static_cast<uint64_t>(kThreads * kPerThread));
    }
    fs::remove_all(dir);
}
