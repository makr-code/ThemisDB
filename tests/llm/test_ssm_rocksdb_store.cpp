/**
 * @file test_ssm_rocksdb_store.cpp
 * @brief Unit tests for SSM-State RocksDB persistence (P2-D04 gate validation).
 * @version 0.1.0-beta
 */

#include <gtest/gtest.h>
#include "llm/ssm_state_rocksdb_store.h"
#include "llm/i_ssm_plugin.h"
#include "storage/hlc.h"

#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/options.h>
#include <memory>
#include <filesystem>
#include <thread>
#include <vector>

namespace themis { namespace llm { namespace tests { 

namespace fs = std::filesystem;

/**
 * @brief Test fixture with a temporary RocksDB TransactionDB instance.
 */
class SSMRocksDBStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for RocksDB
        db_path_ = fs::temp_directory_path() / "themis_ssm_rocksdb_test";
        fs::remove_all(db_path_);
        fs::create_directories(db_path_);

        // Open RocksDB TransactionDB
        rocksdb::TransactionDBOptions txn_options;
        rocksdb::Options options;
        options.create_if_missing = true;

        rocksdb::TransactionDB* raw_db = nullptr;
        rocksdb::Status status = rocksdb::TransactionDB::Open(options, txn_options,
                                                              db_path_.string(), &raw_db);
        ASSERT_TRUE(status.ok());
        db_ = std::unique_ptr<rocksdb::TransactionDB>(raw_db);

        // Create store instance
        store_ = std::make_unique<SSMStateRocksDBStore>(db_.get());
    }

    void TearDown() override {
        store_.reset();
        db_.reset();
        fs::remove_all(db_path_);
    }

    SSMStateSnapshot createTestSnapshot(const std::string& session_id,
                                       int64_t physical_time = 0,
                                       int64_t logical_counter = 0) {
        SSMStateSnapshot snapshot;
        snapshot.session_id = session_id;
        snapshot.snapshot_ts = HLCTimestamp(
            physical_time > 0 ? physical_time : 
                std::chrono::system_clock::now().time_since_epoch().count() / 1000000,
            logical_counter);
        snapshot.hidden_state = "test_hidden_state_data";
        snapshot.cell_state = "test_cell_state_data";
        snapshot.metadata = R"({"model": "mamba-7b", "config": {}})";
        return snapshot;
    }

    fs::path db_path_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<SSMStateRocksDBStore> store_;
};

// --- Basic Functionality Tests ---

TEST_F(SSMRocksDBStoreTest, CheckpointAndResume) {
    auto snapshot = createTestSnapshot("session_001");
    
    // Checkpoint
    bool checkpoint_ok = store_->checkpoint("session_001", snapshot);
    EXPECT_TRUE(checkpoint_ok);
    
    // Resume most recent
    auto resumed = store_->resume("session_001");
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->session_id, "session_001");
    EXPECT_EQ(resumed->hidden_state, "test_hidden_state_data");
    EXPECT_EQ(resumed->cell_state, "test_cell_state_data");
}

TEST_F(SSMRocksDBStoreTest, CheckpointWithSpecificTimestamp) {
    int64_t physical_time = 1609459200000;  // 2021-01-01 00:00:00
    auto snapshot = createTestSnapshot("session_002", physical_time, 42);
    
    bool checkpoint_ok = store_->checkpoint("session_002", snapshot);
    EXPECT_TRUE(checkpoint_ok);
    
    // Resume with specific timestamp
    auto resumed = store_->resume("session_002", snapshot.snapshot_ts);
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->snapshot_ts.getPhysicalTime(), physical_time);
    EXPECT_EQ(resumed->snapshot_ts.getLogicalCounter(), 42);
}

TEST_F(SSMRocksDBStoreTest, MultipleSnapshotsPerSession) {
    std::string session_id = "session_multi";
    
    // Create and checkpoint multiple snapshots
    std::vector<SSMStateSnapshot> snapshots;
    for (int i = 0; i < 5; ++i) {
        auto snap = createTestSnapshot(session_id, 1000000000000 + i * 1000, i);
        snapshots.push_back(snap);
        EXPECT_TRUE(store_->checkpoint(session_id, snap));
    }
    
    // Resume most recent (should be the last one)
    auto resumed = store_->resume(session_id);
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->snapshot_ts.getLogicalCounter(), 4);
    
    // Resume specific snapshot
    auto resumed_2 = store_->resume(session_id, snapshots[1].snapshot_ts);
    ASSERT_TRUE(resumed_2.has_value());
    EXPECT_EQ(resumed_2->snapshot_ts.getLogicalCounter(), 1);
}

TEST_F(SSMRocksDBStoreTest, ResumeNonexistentSession) {
    auto resumed = store_->resume("nonexistent_session");
    EXPECT_FALSE(resumed.has_value());
}

TEST_F(SSMRocksDBStoreTest, InvalidateSession) {
    std::string session_id = "session_to_invalidate";
    
    // Checkpoint some snapshots
    for (int i = 0; i < 3; ++i) {
        auto snap = createTestSnapshot(session_id, 1000000000000 + i * 1000, i);
        store_->checkpoint(session_id, snap);
    }
    
    // Verify snapshots exist
    auto resumed = store_->resume(session_id);
    EXPECT_TRUE(resumed.has_value());
    
    // Invalidate
    bool invalidate_ok = store_->invalidate(session_id);
    EXPECT_TRUE(invalidate_ok);
    
    // Verify all snapshots gone
    auto resumed_after = store_->resume(session_id);
    EXPECT_FALSE(resumed_after.has_value());
}

TEST_F(SSMRocksDBStoreTest, InvalidateNonexistentSession) {
    bool invalidate_ok = store_->invalidate("nonexistent_session");
    EXPECT_FALSE(invalidate_ok);
}

TEST_F(SSMRocksDBStoreTest, EmptySessionId) {
    auto snapshot = createTestSnapshot("", 1000000000000, 0);
    
    // Should fail for empty session ID
    bool checkpoint_ok = store_->checkpoint("", snapshot);
    EXPECT_FALSE(checkpoint_ok);
    
    auto resumed = store_->resume("");
    EXPECT_FALSE(resumed.has_value());
}

// --- Concurrency Tests ---

TEST_F(SSMRocksDBStoreTest, ConcurrentCheckpoints) {
    std::vector<std::thread> threads;
    std::vector<bool> results(10, false);
    
    auto checkpoint_worker = [this, &results](int thread_id) {
        for (int i = 0; i < 5; ++i) {
            std::string session = "session_" + std::to_string(thread_id);
            auto snap = createTestSnapshot(session, 1000000000000 + thread_id * 1000 + i, i);
            results[thread_id] = store_->checkpoint(session, snap);
        }
    };
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(checkpoint_worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All checkpoints should succeed
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
}

TEST_F(SSMRocksDBStoreTest, ConcurrentResumes) {
    // Pre-populate sessions
    std::vector<std::string> sessions;
    for (int i = 0; i < 5; ++i) {
        std::string session = "session_" + std::to_string(i);
        sessions.push_back(session);
        auto snap = createTestSnapshot(session, 1000000000000 + i * 1000, 0);
        store_->checkpoint(session, snap);
    }
    
    // Resume concurrently
    std::vector<std::thread> threads;
    std::vector<bool> results(10, false);
    
    auto resume_worker = [this, &sessions, &results](int thread_id) {
        for (const auto& session : sessions) {
            auto resumed = store_->resume(session);
            results[thread_id] = resumed.has_value();
        }
    };
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(resume_worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All resumes should succeed
    for (bool result : results) {
        EXPECT_TRUE(result);
    }
}

// --- HLC Timestamp Ordering Tests ---

TEST_F(SSMRocksDBStoreTest, HLCTimestampOrdering) {
    std::string session = "session_hlc";
    
    // Create snapshots with specific HLC timestamps
    std::vector<HLCTimestamp> timestamps = {
        HLCTimestamp(1000, 0),
        HLCTimestamp(1000, 1),
        HLCTimestamp(1001, 0),
        HLCTimestamp(1002, 5),
    };
    
    for (const auto& ts : timestamps) {
        SSMStateSnapshot snap;
        snap.session_id = session;
        snap.snapshot_ts = ts;
        snap.hidden_state = "data_" + std::to_string(ts.getLogicalCounter());
        EXPECT_TRUE(store_->checkpoint(session, snap));
    }
    
    // Most recent should be (1002, 5)
    auto most_recent = store_->resume(session);
    ASSERT_TRUE(most_recent.has_value());
    EXPECT_EQ(most_recent->snapshot_ts.getPhysicalTime(), 1002);
    EXPECT_EQ(most_recent->snapshot_ts.getLogicalCounter(), 5);
}

// --- Compaction Tests ---

TEST_F(SSMRocksDBStoreTest, CompactOldSnapshots) {
    std::string session = "session_compact";
    int64_t now_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
    
    // Create old snapshot (outside retention window)
    int64_t old_time = now_ms - (25 * 60 * 60 * 1000);  // 25 hours ago
    auto old_snap = createTestSnapshot(session, old_time, 0);
    store_->checkpoint(session, old_snap);
    
    // Create new snapshot (within retention window)
    int64_t new_time = now_ms - (1 * 60 * 60 * 1000);  // 1 hour ago
    auto new_snap = createTestSnapshot(session, new_time, 1);
    store_->checkpoint(session, new_snap);
    
    // Verify both exist
    auto before = store_->resume(session);
    EXPECT_TRUE(before.has_value());
    
    // Compact with 24-hour window (old snapshot should be deleted)
    int64_t retention_24h = 24 * 60 * 60 * 1000;
    bool compact_ok = store_->compact(retention_24h);
    EXPECT_TRUE(compact_ok);
    
    // New snapshot should still exist
    auto after = store_->resume(session);
    ASSERT_TRUE(after.has_value());
    EXPECT_EQ(after->snapshot_ts.getLogicalCounter(), 1);
}

// --- Statistics Tests ---

TEST_F(SSMRocksDBStoreTest, StatisticsTracking) {
    auto snap1 = createTestSnapshot("session_stats", 1000000000000, 0);
    store_->checkpoint("session_stats", snap1);
    
    std::string stats = store_->getStatistics();
    EXPECT_FALSE(stats.empty());
    EXPECT_NE(stats.find("total_checkpoints"), std::string::npos);
}

TEST_F(SSMRocksDBStoreTest, CheckpointFailure) {
    // Close DB to simulate failure
    db_.reset();
    
    auto snap = createTestSnapshot("session_fail", 1000000000000, 0);
    bool checkpoint_ok = store_->checkpoint("session_fail", snap);
    
    // Should handle gracefully (either return false or throw)
    // This test verifies the implementation doesn't crash
    EXPECT_FALSE(checkpoint_ok);
}

// --- Serialization Tests ---

TEST_F(SSMRocksDBStoreTest, SnapshotSerialization) {
    SSMStateSnapshot original;
    original.session_id = "test_session";
    original.snapshot_ts = HLCTimestamp(1609459200000, 42);
    original.hidden_state = "hidden_data_xyz";
    original.cell_state = "cell_data_abc";
    original.metadata = R"({"model": "mamba", "version": "1.0"})";
    
    store_->checkpoint("test_session", original);
    
    auto resumed = store_->resume("test_session");
    ASSERT_TRUE(resumed.has_value());
    
    EXPECT_EQ(resumed->session_id, original.session_id);
    EXPECT_EQ(resumed->snapshot_ts.getPhysicalTime(), original.snapshot_ts.getPhysicalTime());
    EXPECT_EQ(resumed->snapshot_ts.getLogicalCounter(), original.snapshot_ts.getLogicalCounter());
}
} } } // namespace themis::llm::tests
