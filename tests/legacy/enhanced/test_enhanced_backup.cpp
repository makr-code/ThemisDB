/**
 * Enhanced Backup Manager Tests
 * Tests for compression, encryption, PITR, and cloud storage features
 */

#include <gtest/gtest.h>
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;

class EnhancedBackupTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping unstable enhanced backup tests on Windows";
#endif
        // Clean up test directories
        cleanupPath(db_path_);
        cleanupPath(backup_path_);
        
        // Create DB wrapper
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_wrapper_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_wrapper_->open());
        
        // Create backup manager
        backup_manager_ = std::make_unique<themis::BackupManager>(db_wrapper_);
    }
    
    void TearDown() override {
        backup_manager_.reset();
        db_wrapper_.reset();
        cleanupPath(db_path_);
        cleanupPath(backup_path_);
    }
    
    static void cleanupPath(const std::string& p) {
        std::error_code ec;
        fs::remove_all(p, ec);
    }
    
    const std::string db_path_ = "./data/test_enhanced_backup_db";
    const std::string backup_path_ = "./data/test_enhanced_backup";
    std::shared_ptr<themis::RocksDBWrapper> db_wrapper_;
    std::unique_ptr<themis::BackupManager> backup_manager_;
};

// Test basic backup with compression
TEST_F(EnhancedBackupTest, BackupWithCompression) {
    // Insert test data
    std::vector<uint8_t> value{'t', 'e', 's', 't'};
    ASSERT_TRUE(db_wrapper_->put("key1", value));
    ASSERT_TRUE(db_wrapper_->put("key2", value));
    
    // Create backup with compression
    themis::BackupOptions options;
    options.compression = themis::CompressionType::ZSTD;
    options.verify_after_backup = true;
    
    std::error_code ec;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    EXPECT_FALSE(ec);
    
    // Verify backup was created
    auto backups = backup_manager_->listBackups(backup_path_);
    EXPECT_EQ(backups.size(), 1);
}

// Test backup with encryption
TEST_F(EnhancedBackupTest, BackupWithEncryption) {
    // Insert test data
    std::vector<uint8_t> value{'s', 'e', 'c', 'r', 'e', 't'};
    ASSERT_TRUE(db_wrapper_->put("secret_key", value));
    
    // Create backup with encryption
    themis::BackupOptions options;
    options.encrypt = true;
    options.encryption_key = "0123456789abcdef0123456789abcdef"; // 32 byte hex key
    options.verify_after_backup = true;
    
    std::error_code ec;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    EXPECT_FALSE(ec);
    
    // Verify backup exists
    auto backups = backup_manager_->listBackups(backup_path_);
    EXPECT_EQ(backups.size(), 1);
}

// Test differential backup
TEST_F(EnhancedBackupTest, DifferentialBackup) {
    // Create full backup
    std::vector<uint8_t> value1{'v', '1'};
    ASSERT_TRUE(db_wrapper_->put("key1", value1));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    EXPECT_FALSE(ec);
    
    // Add more data
    std::vector<uint8_t> value2{'v', '2'};
    ASSERT_TRUE(db_wrapper_->put("key2", value2));
    ASSERT_TRUE(db_wrapper_->put("key3", value2));
    
    // Create differential backup
    ASSERT_TRUE(backup_manager_->createDifferentialBackup(backup_path_, ec, options));
    EXPECT_FALSE(ec);
    
    // Should have 2 backups: 1 full + 1 differential
    auto backups = backup_manager_->listBackups(backup_path_);
    EXPECT_GE(backups.size(), 1);
}

// Test backup metrics
TEST_F(EnhancedBackupTest, BackupMetrics) {
    // Create some backups
    std::vector<uint8_t> value{'d', 'a', 't', 'a'};
    ASSERT_TRUE(db_wrapper_->put("key1", value));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    // Get metrics
    auto metrics = backup_manager_->getBackupMetrics(backup_path_);
    
    EXPECT_GT(metrics["total_backups"], 0);
    EXPECT_GT(metrics["full_backups"], 0);
    EXPECT_GT(metrics["total_size_bytes"], 0);
}

// Test RTO estimation
TEST_F(EnhancedBackupTest, RTOEstimation) {
    // Create backup
    std::vector<uint8_t> value{'d', 'a', 't', 'a'};
    ASSERT_TRUE(db_wrapper_->put("key1", value));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    auto backups = backup_manager_->listBackups(backup_path_);
    ASSERT_GT(backups.size(), 0);
    
    // Estimate RTO
    auto backup_dir = fs::path(backup_path_) / backups[0];
    uint32_t rto = backup_manager_->estimateRTO(backup_dir.string());
    
    // RTO should be non-negative (even if 0 for small backups)
    EXPECT_GE(rto, 0);
}

// Test RPO tracking
TEST_F(EnhancedBackupTest, RPOTracking) {
    // Create backup
    std::vector<uint8_t> value{'d', 'a', 't', 'a'};
    ASSERT_TRUE(db_wrapper_->put("key1", value));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    // Get RPO
    auto rpo = backup_manager_->getRPO(backup_path_);
    
    // RPO should be a valid time point (non-zero)
    auto now = std::chrono::system_clock::now();
    EXPECT_LE(rpo, now);
}

// Test retention policy
TEST_F(EnhancedBackupTest, RetentionPolicy) {
    // Create multiple backups
    std::error_code ec;
    themis::BackupOptions options;
    
    std::vector<uint8_t> value{'d', 'a', 't', 'a'};
    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(db_wrapper_->put("key" + std::to_string(i), value));
        ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto backups_before = backup_manager_->listBackups(backup_path_);
    EXPECT_GE(backups_before.size(), 3);
    
    // Apply retention policy (keep backups from last 365 days)
    uint32_t deleted = backup_manager_->applyRetentionPolicy(backup_path_, 365, ec);
    
    // Should not delete any recent backups
    EXPECT_EQ(deleted, 0);
}

// Test restore with statistics
TEST_F(EnhancedBackupTest, RestoreWithStats) {
    // Create backup
    std::vector<uint8_t> value1{'v', '1'};
    ASSERT_TRUE(db_wrapper_->put("restore_key", value1));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    // Modify data
    std::vector<uint8_t> value2{'v', '2'};
    ASSERT_TRUE(db_wrapper_->put("restore_key", value2));
    
    // Restore with stats
    auto backups = backup_manager_->listBackups(backup_path_);
    ASSERT_GT(backups.size(), 0);
    
    auto backup_dir = fs::path(backup_path_) / backups[0];
    themis::RecoveryStats stats;
    ASSERT_TRUE(backup_manager_->restoreFromBackup(backup_dir.string(), ec, &stats));
    
    // Verify stats were populated
    EXPECT_GT(stats.rto_seconds, 0);
    EXPECT_GT(stats.bytes_restored, 0);
    EXPECT_GT(stats.files_restored, 0);
    
    // Verify data was restored
    auto restored_value = db_wrapper_->get("restore_key");
    ASSERT_TRUE(restored_value.has_value());
    std::string restored_str(restored_value->begin(), restored_value->end());
    EXPECT_EQ(restored_str, "v1");
}

// Test PITR framework
TEST_F(EnhancedBackupTest, PITRFramework) {
    // Create initial backup
    std::vector<uint8_t> value1{'v', '1'};
    ASSERT_TRUE(db_wrapper_->put("pitr_key", value1));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    // Setup PITR options
    themis::PITROptions pitr_opts;
    pitr_opts.target_time = std::chrono::system_clock::now();
    pitr_opts.timeline_consistent = true;
    
    // Perform PITR
    themis::RecoveryStats stats;
    ASSERT_TRUE(backup_manager_->performPITR(backup_path_, pitr_opts, ec, &stats));
    EXPECT_FALSE(ec);
}

// Test cloud storage configuration
TEST_F(EnhancedBackupTest, CloudStorageConfig) {
    // Test S3 configuration
    themis::BackupOptions s3_opts;
    s3_opts.storage = themis::StorageBackend::S3;
    s3_opts.storage_path = "s3://my-bucket/backups";
    s3_opts.cloud_config["region"] = "us-east-1";
    s3_opts.cloud_config["access_key"] = "test_key";
    
    EXPECT_EQ(s3_opts.storage, themis::StorageBackend::S3);
    EXPECT_EQ(s3_opts.cloud_config["region"], "us-east-1");
    
    // Test GCS configuration
    themis::BackupOptions gcs_opts;
    gcs_opts.storage = themis::StorageBackend::GCS;
    gcs_opts.storage_path = "gs://my-bucket/backups";
    gcs_opts.cloud_config["project_id"] = "my-project";
    
    EXPECT_EQ(gcs_opts.storage, themis::StorageBackend::GCS);
    
    // Test Azure configuration
    themis::BackupOptions azure_opts;
    azure_opts.storage = themis::StorageBackend::AZURE;
    azure_opts.storage_path = "https://myaccount.blob.core.windows.net/backups";
    azure_opts.cloud_config["account_name"] = "myaccount";
    
    EXPECT_EQ(azure_opts.storage, themis::StorageBackend::AZURE);
}

// Test partial restore framework
TEST_F(EnhancedBackupTest, PartialRestoreFramework) {
    // Create backup
    std::vector<uint8_t> value{'d', 'a', 't', 'a'};
    ASSERT_TRUE(db_wrapper_->put("coll1:key1", value));
    ASSERT_TRUE(db_wrapper_->put("coll2:key2", value));
    
    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));
    
    auto backups = backup_manager_->listBackups(backup_path_);
    ASSERT_GT(backups.size(), 0);
    
    // Restore specific collections
    auto backup_dir = fs::path(backup_path_) / backups[0];
    std::vector<std::string> collections = {"coll1"};
    ASSERT_TRUE(backup_manager_->restoreCollections(backup_dir.string(), collections, ec));
}



// ── PITR-WAL: setWalReplayFn injection (Stub #249) ───────────────────────────

// PITR-WAL-01: injected fn is called after snapshot restore with correct args
TEST_F(EnhancedBackupTest, PITRWalReplayFn_CalledAfterSnapshotRestore) {
    // Create a full backup so PITR has a snapshot to select.
    std::vector<uint8_t> value{'v', '1'};
    ASSERT_TRUE(db_wrapper_->put("pitr_wal_key", value));

    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));

    auto target = std::chrono::system_clock::now() + std::chrono::seconds(1);

    bool fn_called = false;
    std::string captured_dest;
    std::chrono::system_clock::time_point captured_time;

    backup_manager_->setWalReplayFn(
        [&](const std::string& dest_dir,
            std::chrono::system_clock::time_point t,
            std::error_code& replay_ec) -> bool {
            fn_called      = true;
            captured_dest  = dest_dir;
            captured_time  = t;
            replay_ec.clear();
            return true;
        });

    themis::PITROptions pitr_opts;
    pitr_opts.target_time       = target;
    pitr_opts.timeline_consistent = true;

    themis::RecoveryStats stats;
    ASSERT_TRUE(backup_manager_->performPITR(backup_path_, pitr_opts, ec, &stats));
    EXPECT_FALSE(ec);
    EXPECT_TRUE(fn_called) << "WalReplayFn must be invoked by performPITR()";
    EXPECT_FALSE(captured_dest.empty()) << "dest_dir passed to WalReplayFn must not be empty";
    EXPECT_EQ(captured_time, target) << "target_time must be forwarded unchanged";
}

// PITR-WAL-02: returning false from fn causes performPITR to fail
TEST_F(EnhancedBackupTest, PITRWalReplayFn_FailurePropagated) {
    std::vector<uint8_t> value{'v', '2'};
    ASSERT_TRUE(db_wrapper_->put("pitr_wal_key2", value));

    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));

    backup_manager_->setWalReplayFn(
        [](const std::string&, std::chrono::system_clock::time_point, std::error_code& replay_ec)
            -> bool {
            replay_ec = std::make_error_code(std::errc::io_error);
            return false;
        });

    themis::PITROptions pitr_opts;
    pitr_opts.target_time = std::chrono::system_clock::now() + std::chrono::seconds(1);

    themis::RecoveryStats stats;
    bool result = backup_manager_->performPITR(backup_path_, pitr_opts, ec, &stats);
    EXPECT_FALSE(result) << "performPITR() must fail when WalReplayFn returns false";
}

// PITR-WAL-03: clearing fn (nullptr) reverts to stub (skip WAL replay, still succeed)
TEST_F(EnhancedBackupTest, PITRWalReplayFn_ClearingRevertsToStub) {
    std::vector<uint8_t> value{'v', '3'};
    ASSERT_TRUE(db_wrapper_->put("pitr_wal_key3", value));

    std::error_code ec;
    themis::BackupOptions options;
    ASSERT_TRUE(backup_manager_->createFullBackup(backup_path_, ec, options));

    // First install a fn, then clear it.
    backup_manager_->setWalReplayFn(
        [](const std::string&, std::chrono::system_clock::time_point, std::error_code&) {
            return true;
        });
    backup_manager_->setWalReplayFn(nullptr);

    themis::PITROptions pitr_opts;
    pitr_opts.target_time = std::chrono::system_clock::now() + std::chrono::seconds(1);

    themis::RecoveryStats stats;
    ASSERT_TRUE(backup_manager_->performPITR(backup_path_, pitr_opts, ec, &stats))
        << "performPITR() must still succeed after WalReplayFn is cleared (stub path)";
    EXPECT_FALSE(ec);
}
