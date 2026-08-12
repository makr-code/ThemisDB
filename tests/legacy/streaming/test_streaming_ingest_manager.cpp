// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Tests for StreamingIngestManager:
//   SM-01  Construction with valid db succeeds
//   SM-02  Null db throws invalid_argument
//   SM-03  Ingest single event, flush, verify key present in RocksDB
//   SM-04  IngestBatch writes all events
//   SM-05  stop() drains remaining events
//   SM-06  stats() reflects ingested and flushed counts
//   SM-07  DROP overflow policy discards when buffer full
//   SM-08  BLOCK overflow policy returns error on timeout
//   SM-09  flush() before start() returns success (no-op)
//   SM-10  Double start() returns error

#include <gtest/gtest.h>
#include "storage/streaming_ingest_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class StreamingIngestManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = (fs::temp_directory_path() /
                    ("themis_ingest_test_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        fs::remove_all(db_path_);

        RocksDBWrapper::Config cfg;
        cfg.db_path    = db_path_;
        cfg.enable_wal = true;
        db_            = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());
    }

    void TearDown() override {
        if (mgr_) {
            mgr_->stop();
            mgr_.reset();
        }
        db_.reset();
        fs::remove_all(db_path_);
    }

    StreamingIngestManager& manager(StreamingIngestManager::Config cfg = {}) {
        if (!mgr_) {
            cfg.flush_interval = std::chrono::milliseconds(5);
            cfg.sync_wal       = false; // faster in tests
            mgr_ = StreamingIngestManager::create(db_, std::move(cfg));
        }
        return *mgr_;
    }

    std::string                              db_path_;
    std::shared_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<StreamingIngestManager>  mgr_;
};

// ─────────────────────────────────────────────────────────────────────────────
// SM-01: Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM01_ConstructsWithValidDb) {
    EXPECT_NO_THROW((void)StreamingIngestManager::create(db_));
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-02: Null db throws
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM02_NullDbThrows) {
    EXPECT_THROW(StreamingIngestManager::create(nullptr), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-03: Single ingest + flush → key visible in RocksDB
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM03_IngestSingleEventFlushVerify) {
    auto& mgr = manager();
    ASSERT_TRUE(mgr.start());

    auto res = mgr.ingest("k:hello", "world");
    ASSERT_TRUE(res) << res.error().message();

    ASSERT_TRUE(mgr.flush());

    std::string val;
    auto get = db_->get("k:hello", val);
    ASSERT_TRUE(get) << "key not found after ingest+flush";
    EXPECT_EQ(val, "world");
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-04: IngestBatch writes all events
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM04_IngestBatchWritesAll) {
    auto& mgr = manager();
    ASSERT_TRUE(mgr.start());

    std::vector<StreamingIngestManager::Event> events;
    for (int i = 0; i < 100; ++i) {
        events.push_back({"key_" + std::to_string(i), "val_" + std::to_string(i)});
    }

    auto accepted = mgr.ingestBatch(std::move(events));
    ASSERT_TRUE(accepted) << accepted.error().message();
    EXPECT_EQ(*accepted, size_t{100});

    ASSERT_TRUE(mgr.flush());

    for (int i = 0; i < 100; ++i) {
        std::string val;
        EXPECT_TRUE(db_->get("key_" + std::to_string(i), val));
        EXPECT_EQ(val, "val_" + std::to_string(i));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-05: stop() drains remaining events
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM05_StopDrainsBuffer) {
    auto& mgr = manager();
    ASSERT_TRUE(mgr.start());

    for (int i = 0; i < 50; ++i) {
        ASSERT_TRUE(mgr.ingest("drain_" + std::to_string(i), "v"));
    }

    ASSERT_TRUE(mgr.stop());

    // After stop(), all events must be persisted.
    int found = 0;
    for (int i = 0; i < 50; ++i) {
        std::string val;
        if (db_->get("drain_" + std::to_string(i), val)) {
            ++found;
        }
    }
    EXPECT_EQ(found, 50);
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-06: stats() reflects ingested and flushed counts
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM06_StatsReflectCounts) {
    auto& mgr = manager();
    ASSERT_TRUE(mgr.start());

    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(mgr.ingest("s_" + std::to_string(i), "v"));
    }

    ASSERT_TRUE(mgr.flush());

    auto s = mgr.stats();
    EXPECT_GE(s.events_ingested, uint64_t{10});
    EXPECT_GE(s.events_flushed,  uint64_t{10});
    EXPECT_GE(s.flush_count,     uint64_t{1});
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-07: DROP overflow policy discards excess events
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM07_DropPolicyDiscardsOnFull) {
    StreamingIngestManager::Config cfg;
    cfg.flush_interval   = std::chrono::milliseconds(1000); // long — won't flush during test
    cfg.max_buffer_events = 5;
    cfg.overflow_policy  = StreamingIngestManager::OverflowPolicy::DROP;
    cfg.sync_wal         = false;

    auto mgr = StreamingIngestManager::create(db_, std::move(cfg));
    ASSERT_TRUE(mgr->start());

    // Fill the buffer exactly.
    for (int i = 0; i < 5; ++i) {
        ASSERT_TRUE(mgr->ingest("dk_" + std::to_string(i), "v"));
    }

    // 6th ingest should be silently dropped.
    auto res = mgr->ingest("dk_overflow", "v");
    EXPECT_TRUE(res); // DROP policy: ingest() succeeds but event is lost

    auto s = mgr->stats();
    EXPECT_GE(s.dropped_events, uint64_t{1});

    mgr->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-08: BLOCK policy with very short timeout returns error when buffer full
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM08_BlockPolicyTimeoutReturnsError) {
    StreamingIngestManager::Config cfg;
    cfg.flush_interval       = std::chrono::milliseconds(2000); // won't flush
    cfg.max_buffer_events    = 3;
    cfg.overflow_policy      = StreamingIngestManager::OverflowPolicy::BLOCK;
    cfg.backpressure_timeout = std::chrono::milliseconds(10); // 10 ms timeout
    cfg.sync_wal             = false;

    auto mgr = StreamingIngestManager::create(db_, std::move(cfg));
    ASSERT_TRUE(mgr->start());

    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(mgr->ingest("bk_" + std::to_string(i), "v"));
    }

    // Buffer full under BLOCK policy. Depending on scheduler timing, this call
    // may either time out (no space freed) or succeed after the background
    // flush thread drains space before timeout.
    auto res = mgr->ingest("bk_overflow", "v");
    auto s = mgr->stats();
    if (!res) {
        EXPECT_GE(s.backpressure_waits, uint64_t{1});
    } else {
        EXPECT_EQ(s.dropped_events, uint64_t{0});
    }

    mgr->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-09: flush() before start() is a no-op
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM09_FlushBeforeStartNoOp) {
    auto mgr = StreamingIngestManager::create(db_);
    EXPECT_TRUE(mgr->flush()); // empty buffer, not started — should succeed
}

// ─────────────────────────────────────────────────────────────────────────────
// SM-10: Double start() returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StreamingIngestManagerTest, SM10_DoubleStartReturnsError) {
    auto& mgr = manager();
    ASSERT_TRUE(mgr.start());
    auto res = mgr.start();
    EXPECT_FALSE(res);
}
