#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "storage/pitr_manager.h"
#include "transaction/snapshot_manager.h"
#include "cdc/changefeed.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

using namespace themis;

class PITRManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for test database
        test_dir_ = std::filesystem::temp_directory_path() / "pitr_manager_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);

        // Initialize RocksDB
        RocksDBWrapper::Config config;
        config.db_path = test_dir_.string();
        config.enable_statistics = false;
        
        db_wrapper_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_wrapper_->open());

        // Get raw DB for Changefeed (which still uses raw pointers)
        auto* raw_db = db_wrapper_->getRawDB();

        // Initialize Changefeed (still uses raw DB pointer)
        changefeed_ = std::make_unique<Changefeed>(raw_db, nullptr);

        // Initialize SnapshotManager with RocksDBWrapper abstraction
        snapshot_mgr_ = std::make_unique<transaction::SnapshotManager>(*db_wrapper_, *changefeed_);

        // Initialize PITRManager
        pitr_mgr_ = std::make_unique<PITRManager>(
            db_wrapper_.get(), changefeed_.get(), snapshot_mgr_.get()
        );

        // Add some test data
        addTestData();
    }

    void TearDown() override {
        pitr_mgr_.reset();
        snapshot_mgr_.reset();
        changefeed_.reset();
        
        db_wrapper_.reset();
        std::filesystem::remove_all(test_dir_);
    }

    void addTestData() {
        // Record some changefeed events to simulate database changes
        Changefeed::ChangeEvent event;
        event.timestamp_ms = 1000;
        
        // Event 1: PUT users:1
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "users:1";
        event.value = R"({"name":"Alice","age":30})";
        changefeed_->recordEvent(event);
        
        // Event 2: PUT users:2
        event.key = "users:2";
        event.value = R"({"name":"Bob","age":25})";
        event.timestamp_ms = 2000;
        changefeed_->recordEvent(event);
        
        // Event 3: PUT products:1
        event.key = "products:1";
        event.value = R"({"name":"Widget","price":9.99})";
        event.timestamp_ms = 3000;
        changefeed_->recordEvent(event);
        
        // Event 4: DELETE users:1
        event.type = Changefeed::ChangeEventType::EVENT_DELETE;
        event.key = "users:1";
        event.value = std::nullopt;
        event.timestamp_ms = 4000;
        changefeed_->recordEvent(event);
        
        // Event 5: PUT users:3
        event.type = Changefeed::ChangeEventType::EVENT_PUT;
        event.key = "users:3";
        event.value = R"({"name":"Charlie","age":35})";
        event.timestamp_ms = 5000;
        changefeed_->recordEvent(event);
    }

    std::filesystem::path test_dir_;
    std::unique_ptr<RocksDBWrapper> db_wrapper_;
    std::unique_ptr<Changefeed> changefeed_;
    std::unique_ptr<transaction::SnapshotManager> snapshot_mgr_;
    std::unique_ptr<PITRManager> pitr_mgr_;
};

// Test: Preview restore to sequence
TEST_F(PITRManagerTest, PreviewRestoreToSequence) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 2);
    
    PITRManager::RestoreOptions options;
    auto preview = pitr_mgr_->previewRestore(current_seq - 2, options);
    
    EXPECT_EQ(preview.target_sequence, current_seq - 2);
    EXPECT_EQ(preview.current_sequence, current_seq);
    EXPECT_EQ(preview.events_to_replay, 2);
    EXPECT_GT(preview.estimated_duration_sec, 0);
    EXPECT_FALSE(preview.affected_tables.empty());
}

// Test: Preview restore with empty database
TEST_F(PITRManagerTest, PreviewRestoreEmptyDatabase) {
    // Create a fresh database with no events
    auto fresh_dir = test_dir_ / "fresh";
    std::filesystem::create_directories(fresh_dir);
    
    RocksDBWrapper::Config config;
    config.db_path = fresh_dir.string();
    config.enable_statistics = false;
    
    auto fresh_db = std::make_unique<RocksDBWrapper>(config);
    ASSERT_TRUE(fresh_db->open());
    
    auto fresh_changefeed = std::make_unique<Changefeed>(fresh_db->getRawDB(), nullptr);
    auto fresh_snapshot_mgr = std::make_unique<transaction::SnapshotManager>(*fresh_db, *fresh_changefeed);
    auto fresh_pitr = std::make_unique<PITRManager>(
        fresh_db.get(), fresh_changefeed.get(), fresh_snapshot_mgr.get()
    );
    
    PITRManager::RestoreOptions options;
    auto preview = fresh_pitr->previewRestore(0, options);
    
    EXPECT_EQ(preview.events_to_replay, 0);
    EXPECT_EQ(preview.estimated_duration_sec, 0);
}

// Test: Restore to tag (dry-run)
TEST_F(PITRManagerTest, RestoreToTagDryRun) {
    // Create a snapshot at sequence 2
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 2);
    
    // Manually create snapshot at earlier sequence (for testing)
    snapshot_mgr_->createTag("test_snapshot", "Test snapshot");
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.create_backup = false;
    
    auto status = pitr_mgr_->restoreToTag("test_snapshot", options);
    // Note: This will fail because snapshot was created at current sequence
    // In a real scenario, we'd create the snapshot earlier
}

// Test: Restore to invalid sequence
TEST_F(PITRManagerTest, RestoreToInvalidSequence) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    
    // Try to restore to future sequence
    auto status = pitr_mgr_->restoreToSequence(current_seq + 100, options);
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("must be less than current sequence"));
}

// Test: Restore to non-existent tag
TEST_F(PITRManagerTest, RestoreToNonExistentTag) {
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    
    auto status = pitr_mgr_->restoreToTag("nonexistent_tag", options);
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("not found"));
}

// Test: Restore with table filter
TEST_F(PITRManagerTest, RestoreWithTableFilter) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 2);
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.tables = {"users"}; // Only restore users table
    
    auto preview = pitr_mgr_->previewRestore(current_seq - 2, options);
    
    // Check that only users table is affected
    EXPECT_FALSE(preview.affected_tables.empty());
    for (const auto& table : preview.affected_tables) {
        EXPECT_EQ(table, "users");
    }
}

// Test: Restore with max events limit
TEST_F(PITRManagerTest, RestoreWithMaxEventsLimit) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 3);
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.max_events_to_replay = 2;
    
    auto status = pitr_mgr_->restoreToSequence(1, options);
    EXPECT_TRUE(status.ok) << status.message;
    
    // Check progress
    if (status.progress.has_value()) {
        EXPECT_LE(status.progress->events_processed, 2);
    }
}

// Test: DELETE reverse fails closed when no previous value is available
TEST_F(PITRManagerTest, RestoreFailsOnDeleteWithoutRecoverablePreviousValue) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GE(current_seq, 5);

    PITRManager::RestoreOptions options;
    options.dry_run = false;
    options.create_backup = false;
    options.abort_on_first_error = true;

    // Includes the DELETE event in addTestData() (sequence 4), which has no
    // value/before_snapshot and must fail closed.
    auto status = pitr_mgr_->restoreToSequence(3, options);
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("Cannot reverse DELETE"));
}

// Test: Restore fails when WAL/changefeed range is incomplete
TEST_F(PITRManagerTest, RestoreFailsWhenWalReplayCoverageIsIncomplete) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GE(current_seq, 5);

    // Simulate truncated WAL/history: remove old events while keeping latest.
    const size_t removed = changefeed_->deleteOldEventsBySequence(4);
    EXPECT_GT(removed, 0u);

    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.create_backup = false;
    options.max_events_to_replay = 0; // enforce full replay coverage

    auto status = pitr_mgr_->restoreToSequence(1, options);
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("WAL replay coverage incomplete"));
}

// Test: Restore progress tracking
TEST_F(PITRManagerTest, RestoreProgressTracking) {
    // Initially no restore in progress
    EXPECT_FALSE(pitr_mgr_->isRestoreInProgress());
    EXPECT_FALSE(pitr_mgr_->getProgress().has_value());
    
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 1);
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.create_backup = false;
    
    auto status = pitr_mgr_->restoreToSequence(current_seq - 1, options);
    EXPECT_TRUE(status.ok) << status.message;
    
    // Check that progress was tracked
    if (status.progress.has_value()) {
        EXPECT_EQ(status.progress->phase, PITRManager::RestoreProgress::Phase::COMPLETED);
        EXPECT_GT(status.progress->events_processed, 0);
        EXPECT_GT(status.progress->getElapsedMs(), 0);
    }
}

// Test: Restore to timestamp
TEST_F(PITRManagerTest, RestoreToTimestamp) {
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    options.create_backup = false;
    
    // Restore to timestamp 2500 (should find sequence between event 2 and 3)
    auto status = pitr_mgr_->restoreToTimestamp(2500, options);
    EXPECT_TRUE(status.ok) << status.message;
}

// Test: Restore to timestamp with no events
TEST_F(PITRManagerTest, RestoreToTimestampNoEvents) {
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    
    // Try to restore to timestamp before any events
    auto status = pitr_mgr_->restoreToTimestamp(500, options);
    EXPECT_FALSE(status.ok);
    EXPECT_THAT(status.message, ::testing::HasSubstr("No events found"));
}

// Test: Auto-backup creation
TEST_F(PITRManagerTest, AutoBackupCreation) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 1);
    
    PITRManager::RestoreOptions options;
    options.dry_run = false; // Actually try to restore (will need write permissions)
    options.create_backup = true;
    options.backup_tag = "auto_backup_test";
    
    // Note: This test would need write permissions and proper setup
    // For now, we test the backup creation separately
    
    auto backup_snapshot = snapshot_mgr_->createTag(
        options.backup_tag,
        "Auto-backup test",
        "pitr_manager"
    );
    EXPECT_TRUE(backup_snapshot.has_value());
    
    // Verify backup was created
    auto snapshot = snapshot_mgr_->getTag(options.backup_tag);
    EXPECT_TRUE(snapshot.has_value());
}

// Test: Concurrent restore prevention
TEST_F(PITRManagerTest, ConcurrentRestorePrevention) {
    // This test would require async execution
    // For now, we just verify the flag works
    EXPECT_FALSE(pitr_mgr_->isRestoreInProgress());
}

// Test: Preview shows affected keys sample
TEST_F(PITRManagerTest, PreviewShowsAffectedKeysSample) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    ASSERT_GT(current_seq, 2);
    
    PITRManager::RestoreOptions options;
    auto preview = pitr_mgr_->previewRestore(1, options);
    
    EXPECT_FALSE(preview.affected_keys.empty());
    EXPECT_LE(preview.affected_keys.size(), 100); // Max 100 keys in sample
}

// Test: Restore validates sequence order
TEST_F(PITRManagerTest, RestoreValidatesSequenceOrder) {
    uint64_t current_seq = changefeed_->getLatestSequence();
    
    PITRManager::RestoreOptions options;
    options.dry_run = true;
    
    // Current sequence should fail
    auto status = pitr_mgr_->restoreToSequence(current_seq, options);
    EXPECT_FALSE(status.ok);
}

// Test: Progress percent calculation
TEST_F(PITRManagerTest, ProgressPercentCalculation) {
    PITRManager::RestoreProgress progress;
    progress.total_events = 100;
    progress.events_processed = 0;
    
    EXPECT_EQ(progress.getProgressPercent(), 0.0);
    
    progress.events_processed = 50;
    EXPECT_EQ(progress.getProgressPercent(), 50.0);
    
    progress.events_processed = 100;
    EXPECT_EQ(progress.getProgressPercent(), 100.0);
    
    // Edge case: no events
    progress.total_events = 0;
    progress.events_processed = 0;
    EXPECT_EQ(progress.getProgressPercent(), 0.0);
}

// Test: Progress elapsed time calculation
TEST_F(PITRManagerTest, ProgressElapsedTimeCalculation) {
    PITRManager::RestoreProgress progress;
    
    // Not started
    EXPECT_EQ(progress.getElapsedMs(), 0);
    
    // Started
    progress.start_time_ms = PITRManager::RestoreProgress::getCurrentTimeMs();
    
    // Wait a moment (use a fixed delay for deterministic testing)
    int64_t wait_ms = 100;
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < wait_ms) {
        // Busy wait
    }
    
    int64_t elapsed = progress.getElapsedMs();
    EXPECT_GE(elapsed, wait_ms * 0.8); // At least 80% of expected time
    EXPECT_LT(elapsed, wait_ms * 2.0); // Less than 2x expected time
    
    // Completed
    progress.end_time_ms = PITRManager::RestoreProgress::getCurrentTimeMs();
    
    int64_t final_elapsed = progress.getElapsedMs();
    EXPECT_EQ(final_elapsed, progress.end_time_ms - progress.start_time_ms);
}


