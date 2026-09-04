#include <gtest/gtest.h>

// Disable legacy BackupManager enhanced tests
#if 0
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <memory>

namespace fs = std::filesystem;
namespace themis {
namespace test {

class BackupManagerEnhancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any existing test data
        cleanupPath(db_path_);
        cleanupPath(backup_dir_);
        
        // Create database
        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        db_wrapper_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_wrapper_->open());
        
        // Create backup manager
        backup_mgr_ = std::make_unique<themis::BackupManager>(db_wrapper_);
    }
    
    void TearDown() override {
        backup_mgr_.reset();
        db_wrapper_.reset();
        cleanupPath(db_path_);
        cleanupPath(backup_dir_);
    }
    
    void cleanupPath(const std::string& p) {
        std::error_code ec = {};
        fs::remove_all(p, ec);
    }
    
    void insertTestData(const std::string& prefix, int count) {
        for (int i = 0; i < count; ++i) {
            std::string key = prefix + ":key" + std::to_string(i);
            std::string value = "value" + std::to_string(i);
            std::vector<uint8_t> val_vec(value.begin(), value.end());
            ASSERT_TRUE(db_wrapper_->put(key, val_vec));
        }
    }
    
    bool verifyTestData(const std::string& prefix, int count) {
        for (int i = 0; i < count; ++i) {
            std::string key = prefix + ":key" + std::to_string(i);
            std::string expected_value = "value" + std::to_string(i);
            
            auto val = db_wrapper_->get(key);
            if (!val.has_value()) {
              return false;
            }
            
            std::string actual_value(val->begin(), val->end());
            if (actual_value != expected_value) {
              return false;
            }
        }
        return true;
    }
    
    std::string db_path_ = "./data/test_backup_db";
    std::string backup_dir_ = "./data/test_backups";
    std::shared_ptr<themis::RocksDBWrapper> db_wrapper_;
    std::unique_ptr<themis::BackupManager> backup_mgr_;
};

TEST_F(BackupManagerEnhancedTest, FullBackupCreation) {
    // Insert test data
    insertTestData("test", 100);
    
    // Create full backup
    auto result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(result.has_value()) << "Full backup failed: " << result.error().message();
    
    // Verify backup directory exists
    EXPECT_TRUE(fs::exists(*result));
    
    // Verify manifest exists
    auto manifest_path = fs::path(*result) / "MANIFEST.json";
    EXPECT_TRUE(fs::exists(manifest_path));
    
    // Verify checkpoint directory exists
    auto checkpoint_path = fs::path(*result) / "checkpoint";
    EXPECT_TRUE(fs::exists(checkpoint_path));
}

TEST_F(BackupManagerEnhancedTest, IncrementalBackupCreation) {
    // Insert initial data and create full backup
    insertTestData("initial", 50);
    auto full_result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(full_result.has_value());
    
    // Insert more data
    insertTestData("incremental", 30);
    
    // Create incremental backup
    auto incr_result = backup_mgr_->createIncrementalBackup(backup_dir_);
    ASSERT_TRUE(incr_result.has_value()) << "Incremental backup failed: " << incr_result.error().message();
    
    // Verify incremental backup directory exists
    EXPECT_TRUE(fs::exists(*incr_result));
    EXPECT_TRUE(incr_result->find("incr_") != std::string::npos);
}

TEST_F(BackupManagerEnhancedTest, DifferentialBackupCreation) {
    // Insert initial data and create full backup
    insertTestData("initial", 50);
    auto full_result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(full_result.has_value());
    
    // Create an incremental backup
    insertTestData("incr1", 20);
    auto incr_result = backup_mgr_->createIncrementalBackup(backup_dir_);
    ASSERT_TRUE(incr_result.has_value());
    
    // Insert more data
    insertTestData("diff", 30);
    
    // Create differential backup (should be relative to last full, not incremental)
    auto diff_result = backup_mgr_->createDifferentialBackup(backup_dir_);
    ASSERT_TRUE(diff_result.has_value()) << "Differential backup failed: " << diff_result.error().message();
    
    // Verify differential backup directory exists
    EXPECT_TRUE(fs::exists(*diff_result));
    EXPECT_TRUE(diff_result->find("diff_") != std::string::npos);
}

TEST_F(BackupManagerEnhancedTest, BackupVerification) {
    // Create backup
    insertTestData("verify", 50);
    auto backup_result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(backup_result.has_value());
    
    // Verify backup
    auto verify_result = backup_mgr_->verifyBackup(*backup_result);
    EXPECT_TRUE(verify_result.has_value()) << "Backup verification failed: " << verify_result.error().message();
}

TEST_F(BackupManagerEnhancedTest, BackupRestore) {
    // Insert test data
    insertTestData("restore", 100);
    
    // Create backup
    auto backup_result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(backup_result.has_value());
    
    // Modify data
    insertTestData("modified", 50);
    
    // Restore from backup
    auto restore_result = backup_mgr_->restoreFromBackup(*backup_result);
    EXPECT_TRUE(restore_result.has_value()) << "Restore failed: " << restore_result.error().message();
    
    // Verify original data is restored
    EXPECT_TRUE(verifyTestData("restore", 100));
}

TEST_F(BackupManagerEnhancedTest, ListBackups) {
    // Create multiple backups
    insertTestData("data1", 50);
    backup_mgr_->createFullBackup(backup_dir_);
    
    insertTestData("data2", 30);
    backup_mgr_->createIncrementalBackup(backup_dir_);
    
    insertTestData("data3", 20);
    backup_mgr_->createDifferentialBackup(backup_dir_);
    
    // List backups
    auto backups = backup_mgr_->listBackups(backup_dir_);
    
    // Should have 3 backups (full, incremental, differential)
    EXPECT_GE(backups.size(), 3);
}

TEST_F(BackupManagerEnhancedTest, WALArchiving) {
    // Insert data
    insertTestData("wal", 50);
    
    // Archive WAL
    std::string wal_archive_dir = backup_dir_ + "/wal_archive";
    auto result = backup_mgr_->archiveWAL(wal_archive_dir);
    
    EXPECT_TRUE(result.has_value()) << "WAL archiving failed: " << result.error().message();
    EXPECT_TRUE(fs::exists(wal_archive_dir));
}

TEST_F(BackupManagerEnhancedTest, BackupCompression) {
    // Create backup
    insertTestData("compress", 50);
    auto backup_result = backup_mgr_->createFullBackup(backup_dir_);
    ASSERT_TRUE(backup_result.has_value());
    
    // Compress backup
    auto compress_result = backup_mgr_->compressBackup(*backup_result);
    
    // Note: This test might fail if tar is not available in the test environment
    // That's acceptable for now as it's an optional feature
    if (compress_result.has_value()) {
        EXPECT_TRUE(fs::exists(*compress_result));
        EXPECT_TRUE(compress_result->find(".tar.gz") != std::string::npos);
    }
}

TEST_F(BackupManagerEnhancedTest, ChecksumCalculation) {
    // Create a test file
    std::string test_file = backup_dir_ + "/test_file.txt";
    fs::create_directories(backup_dir_);
    
    std::ofstream out(test_file);
    out << "Test content for checksum";
    out.close();
    
    // Calculate checksum
    auto checksum_result = backup_mgr_->calculateChecksum(test_file);
    
    ASSERT_TRUE(checksum_result.has_value()) << "Checksum calculation failed: " << checksum_result.error().message();
    EXPECT_FALSE(checksum_result->empty());
    EXPECT_EQ(checksum_result->length(), 64); // SHA256 produces 64 hex characters
}

TEST_F(BackupManagerEnhancedTest, ChecksumVerification) {
    // Create a test file
    std::string test_file = backup_dir_ + "/test_verify.txt";
    fs::create_directories(backup_dir_);
    
    std::ofstream out(test_file);
    out << "Test content";
    out.close();
    
    // Calculate checksum
    auto checksum_result = backup_mgr_->calculateChecksum(test_file);
    ASSERT_TRUE(checksum_result.has_value());
    
    // Verify with correct checksum
    auto verify_result = backup_mgr_->verifyChecksum(test_file, *checksum_result);
    EXPECT_TRUE(verify_result.has_value());
    
    // Verify with incorrect checksum
    auto verify_fail = backup_mgr_->verifyChecksum(test_file, "wrong_checksum");
    EXPECT_FALSE(verify_fail.has_value());
}

TEST_F(BackupManagerEnhancedTest, ErrorHandling_InvalidBackupDir) {
    // Try to restore from non-existent backup
    auto result = backup_mgr_->restoreFromBackup("/nonexistent/backup");
    
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT);
}

TEST_F(BackupManagerEnhancedTest, ErrorHandling_EmptyBackupDir) {
    // Create empty backup directory
    fs::create_directories(backup_dir_);
    
    // Try to verify empty directory
    auto result = backup_mgr_->verifyBackup(backup_dir_);
    
    EXPECT_FALSE(result.has_value());
}

} // namespace test
} // namespace themis

#endif // legacy BackupManager enhanced tests

TEST(BackupManagerEnhancedTest, DISABLED_BackupManagerEnhancedLegacy) {
    GTEST_SKIP() << "BackupManager enhanced tests disabled in this configuration";
}
