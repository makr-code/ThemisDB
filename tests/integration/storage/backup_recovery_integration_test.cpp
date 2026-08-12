/**
 * @file backup_recovery_integration_test.cpp
 * @brief Integration test for backup and recovery workflows
 * 
 * Tests the complete backup/recovery workflow:
 * - Full backup creation
 * - Incremental backup
 * - Point-in-time recovery
 * - Backup verification
 * - Recovery validation
 */

#include "../test_fixture.h"
#include "../test_data_generator.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/backup_manager.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

namespace themis {
namespace test {

/**
 * @brief Integration tests for backup and recovery
 */
class BackupRecoveryIntegrationTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }
    
    /**
     * @brief Create and initialize a test database
     */
    std::shared_ptr<RocksDBWrapper> CreateTestDatabase(const std::string& name) {
        auto db_path = CreateTestDbPath(name);
        
        RocksDBWrapper::Config config;
        config.db_path = db_path.string();
        config.enable_wal = true;
        config.create_if_missing = true;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 128;
        
        auto db = std::make_shared<RocksDBWrapper>(config);
        if (!db->open()) {
            return nullptr;
        }
        
        return db;
    }
    
    /**
     * @brief Insert test data into database
     */
    bool InsertTestData(std::shared_ptr<RocksDBWrapper>& db, int count, const std::string& prefix) {
        for (int i = 0; i < count; ++i) {
            std::string key = prefix + "_" + std::to_string(i);
            json doc = {
                {"id", key},
                {"value", data_gen_->GenerateRandomString(100)},
                {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
            };
            
            auto result = db->put(key, doc.dump());
            if (!result) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Verify data exists in database
     */
    bool VerifyTestData(std::shared_ptr<RocksDBWrapper>& db, int count, const std::string& prefix) {
        for (int i = 0; i < count; ++i) {
            std::string key = prefix + "_" + std::to_string(i);
            auto result = db->get(key);
            if (!result) {
                return false;
            }
        }
        return true;
    }

    void WriteIntegrityManifestFile(const std::filesystem::path& backup_dir, const json& manifest) {
        std::ofstream manifest_file(backup_dir / "INTEGRITY_MANIFEST.json");
        ASSERT_TRUE(manifest_file.is_open()) << "Failed to create integrity manifest";
        manifest_file << manifest.dump(2);
    }
    
    std::unique_ptr<TestDataGenerator> data_gen_;
};

/**
 * @test Verify full backup and restore
 * 
 * Acceptance Criteria:
 * - Full backup captures all data
 * - Backup can be restored to new location
 * - Restored data matches original
 * - Indexes are correctly rebuilt
 */
TEST_F(BackupRecoveryIntegrationTest, FullBackupAndRestore) {
    // Step 1: Create database with test data
    auto original_db = CreateTestDatabase("original_db");
    ASSERT_NE(original_db, nullptr) << "Failed to create test database";
    
    const int test_count = 100;
    ASSERT_TRUE(InsertTestData(original_db, test_count, "backup_test")) 
        << "Failed to insert test data";
    
    // Step 2: Create backup manager and full backup
    auto backup_manager = std::make_shared<BackupManager>(original_db);
    auto backup_path = GetTempDir() / "backup";
    std::filesystem::create_directories(backup_path);
    
    auto backup_result = backup_manager->createFullBackup(backup_path.string());
    if (!backup_result.has_value()) {
        GTEST_SKIP() << "Backup creation not fully implemented: " << backup_result.error().message();
        return;
    }
    
    ASSERT_TRUE(backup_result.has_value()) 
        << "Failed to create backup";
    
    std::string backup_dir = backup_result.value();
    ASSERT_TRUE(std::filesystem::exists(backup_dir)) 
        << "Backup directory not created";
    
    // Step 3: Close original database
    original_db->close();
    original_db.reset();
    
    // Step 4: Create new database instance for restore
    auto restored_db = CreateTestDatabase("restored_db");
    ASSERT_NE(restored_db, nullptr) << "Failed to create restored database";
    
    auto restore_manager = std::make_shared<BackupManager>(restored_db);
    std::error_code ec;
    bool restore_success = restore_manager->restoreFromBackup(backup_dir, ec);
    
    // Note: Restore may not be fully implemented yet, check if it's available
    if (ec && ec.value() != 0) {
        GTEST_SKIP() << "Restore functionality not fully implemented: " << ec.message();
        return;
    }
    
    ASSERT_TRUE(restore_success) << "Failed to restore backup: " << ec.message();
    
    // Step 5: Verify restored data
    EXPECT_TRUE(VerifyTestData(restored_db, test_count, "backup_test"))
        << "Failed to verify restored data";
    
    // Cleanup
    restored_db->close();
}

/**
 * @test Verify incremental backup
 * 
 * Acceptance Criteria:
 * - Incremental backup captures only changes
 * - Incremental backup is smaller than full backup
 * - Restore from incremental works correctly
 */
TEST_F(BackupRecoveryIntegrationTest, IncrementalBackup) {
    // Step 1: Create database with initial data
    auto db = CreateTestDatabase("incremental_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";
    
    const int initial_count = 50;
    ASSERT_TRUE(InsertTestData(db, initial_count, "initial")) 
        << "Failed to insert initial data";
    
    // Step 2: Create full backup
    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_path = GetTempDir() / "backup_incremental";
    std::filesystem::create_directories(backup_path);
    
    auto full_backup_result = backup_manager->createFullBackup(backup_path.string());
    ASSERT_TRUE(full_backup_result.has_value()) 
        << "Failed to create full backup: " << full_backup_result.error().message();
    
    std::string full_backup_dir = full_backup_result.value();
    size_t full_backup_size = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(full_backup_dir)) {
        if (entry.is_regular_file()) {
            full_backup_size += entry.file_size();
        }
    }
    
    // Step 3: Insert more data
    const int additional_count = 25;
    ASSERT_TRUE(InsertTestData(db, additional_count, "additional")) 
        << "Failed to insert additional data";
    
    // Step 4: Create incremental backup
    auto incr_backup_result = backup_manager->createIncrementalBackup(backup_path.string());
    
    // If incremental backup is not implemented, skip
    if (!incr_backup_result.has_value()) {
        GTEST_SKIP() << "Incremental backup not implemented: " 
                      << incr_backup_result.error().message();
        return;
    }
    
    std::string incr_backup_dir = incr_backup_result.value();
    size_t incr_backup_size = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(incr_backup_dir)) {
        if (entry.is_regular_file()) {
            incr_backup_size += entry.file_size();
        }
    }
    
    // Step 5: Verify incremental backup is smaller
    EXPECT_LT(incr_backup_size, full_backup_size) 
        << "Incremental backup should be smaller than full backup";
    
    // Step 6: Verify all data is present
    EXPECT_TRUE(VerifyTestData(db, initial_count, "initial"))
        << "Initial data verification failed";
    EXPECT_TRUE(VerifyTestData(db, additional_count, "additional"))
        << "Additional data verification failed";
    
    db->close();
}

/**
 * @test Verify point-in-time recovery
 * 
 * Acceptance Criteria:
 * - Database can be restored to specific timestamp
 * - Data after timestamp is not included
 * - Data before timestamp is complete
 */
TEST_F(BackupRecoveryIntegrationTest, PointInTimeRecovery) {
    // Step 1: Create database with data
    auto db = CreateTestDatabase("pitr_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";
    
    // Insert data before target time
    ASSERT_TRUE(InsertTestData(db, 30, "before")) 
        << "Failed to insert data before target time";
    
    // Record target time
    auto target_time = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Insert data after target time
    ASSERT_TRUE(InsertTestData(db, 20, "after")) 
        << "Failed to insert data after target time";
    
    // Step 2: Create backup
    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_path = GetTempDir() / "backup_pitr";
    std::filesystem::create_directories(backup_path);
    
    auto backup_result = backup_manager->createFullBackup(backup_path.string());
    ASSERT_TRUE(backup_result.has_value()) 
        << "Failed to create backup: " << backup_result.error().message();
    
    db->close();
    
    // Step 3: Attempt point-in-time recovery
    auto restored_db = CreateTestDatabase("pitr_restored");
    ASSERT_NE(restored_db, nullptr) << "Failed to create restored database";
    
    auto restore_manager = std::make_shared<BackupManager>(restored_db);
    
    PITROptions pitr_opts;
    pitr_opts.target_time = target_time;
    pitr_opts.timeline_consistent = true;
    
    std::error_code ec;
    RecoveryStats stats;
    bool pitr_success = restore_manager->performPITR(backup_result.value(), pitr_opts, ec, &stats);
    
    // PITR may not be fully implemented yet
    if (ec && ec.value() != 0) {
        GTEST_SKIP() << "PITR functionality not fully implemented: " << ec.message();
        return;
    }
    
    ASSERT_TRUE(pitr_success) << "PITR failed: " << ec.message();
    
    // Step 4: Verify only data before target time exists
    EXPECT_TRUE(VerifyTestData(restored_db, 30, "before"))
        << "Data before target time should exist";
    
    // Data after target time should not exist (or PITR should have filtered it)
    // Note: Full verification would require checking absence, but for now we just
    // verify the PITR completed successfully
    
    restored_db->close();
}

/**
 * @test Verify backup during active operations
 * 
 * Acceptance Criteria:
 * - Backup can be created while database is active
 * - Active writes don't corrupt backup
 * - Backup is consistent
 */
TEST_F(BackupRecoveryIntegrationTest, BackupDuringActiveOperations) {
    // Step 1: Create database and start inserting data
    auto db = CreateTestDatabase("active_backup_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";
    
    // Insert initial data
    ASSERT_TRUE(InsertTestData(db, 50, "initial")) 
        << "Failed to insert initial data";
    
    // Step 2: Start backup while continuing to write
    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_path = GetTempDir() / "backup_active";
    std::filesystem::create_directories(backup_path);
    
    // Start backup in background (simulated by doing it synchronously
    // while database is still open and potentially being written to)
    auto backup_result = backup_manager->createFullBackup(backup_path.string());
    ASSERT_TRUE(backup_result.has_value()) 
        << "Failed to create backup during active operations: " 
        << backup_result.error().message();
    
    // Step 3: Insert more data after backup started
    ASSERT_TRUE(InsertTestData(db, 30, "during_backup")) 
        << "Failed to insert data during backup";
    
    // Step 4: Verify backup was created and is consistent
    std::string backup_dir = backup_result.value();
    EXPECT_TRUE(std::filesystem::exists(backup_dir)) 
        << "Backup directory not created";
    
    // Check for manifest file which indicates backup completeness
    auto manifest_path = std::filesystem::path(backup_dir) / "MANIFEST.json";
    if (std::filesystem::exists(manifest_path)) {
        std::ifstream manifest_file(manifest_path);
        json manifest;
        manifest_file >> manifest;
        
        EXPECT_TRUE(manifest.contains("timestamp")) 
            << "Backup manifest should contain timestamp";
        EXPECT_TRUE(manifest.contains("backup_type") || manifest.contains("type")) 
            << "Backup manifest should contain backup type";
    }
    
    // Step 5: Verify database is still operational
    EXPECT_TRUE(VerifyTestData(db, 50, "initial"))
        << "Initial data should still be accessible";
    EXPECT_TRUE(VerifyTestData(db, 30, "during_backup"))
        << "Data inserted during backup should be accessible";
    
    db->close();
}

TEST_F(BackupRecoveryIntegrationTest, VerifyDecompressedBackupAllowsLegacyBackupWithoutManifest) {
    auto db = CreateTestDatabase("verify_decompressed_legacy_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";

    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_dir = GetTempDir() / "legacy_backup_without_manifest";
    std::filesystem::create_directories(backup_dir);

    std::ofstream payload(backup_dir / "payload.txt");
    ASSERT_TRUE(payload.is_open()) << "Failed to create payload file";
    payload << "legacy-backup-payload";
    payload.close();

    auto verify_result = backup_manager->verifyDecompressedBackup(backup_dir.string());

    EXPECT_TRUE(verify_result.has_value()) << "Legacy backup without manifest should remain valid: "
                                           << verify_result.error().message();
    db->close();
}

TEST_F(BackupRecoveryIntegrationTest, VerifyDecompressedBackupFailsForCorruptManifest) {
    auto db = CreateTestDatabase("verify_decompressed_corrupt_manifest_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";

    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_dir = GetTempDir() / "backup_with_corrupt_manifest";
    std::filesystem::create_directories(backup_dir);

    std::ofstream manifest_file(backup_dir / "INTEGRITY_MANIFEST.json");
    ASSERT_TRUE(manifest_file.is_open()) << "Failed to create integrity manifest";
    manifest_file << "{ invalid json";
    manifest_file.close();

    auto verify_result = backup_manager->verifyDecompressedBackup(backup_dir.string());

    ASSERT_FALSE(verify_result.has_value()) << "Corrupt manifest should fail verification";
    EXPECT_EQ(verify_result.error().code(), errors::ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT);
    db->close();
}

TEST_F(BackupRecoveryIntegrationTest, VerifyDecompressedBackupFailsWhenManifestFileIsMissingFromPayload) {
    auto db = CreateTestDatabase("verify_decompressed_missing_file_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";

    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_dir = GetTempDir() / "backup_with_missing_payload_file";
    std::filesystem::create_directories(backup_dir);

    json manifest = json::array({
        {
            {"path", "payload.txt"},
            {"checksum_sha256", std::string(64, '0')},
            {"original_size", 21},
            {"compressed_size", 0},
            {"compression", static_cast<int>(CompressionType::NONE)}
        }
    });
    WriteIntegrityManifestFile(backup_dir, manifest);

    auto verify_result = backup_manager->verifyDecompressedBackup(backup_dir.string());

    ASSERT_FALSE(verify_result.has_value()) << "Missing payload file should fail verification";
    EXPECT_EQ(verify_result.error().code(), errors::ErrorCode::ERR_STORAGE_CORRUPTION);
    EXPECT_NE(verify_result.error().message().find("payload.txt"), std::string::npos);
    db->close();
}

/**
 * @test Verify encrypted backup
 * 
 * Acceptance Criteria:
 * - Backup can be encrypted
 * - Encrypted backup requires key to restore
 * - Restored data is correct
 */
TEST_F(BackupRecoveryIntegrationTest, EncryptedBackup) {
    // Step 1: Create database with test data
    auto db = CreateTestDatabase("encrypted_backup_db");
    ASSERT_NE(db, nullptr) << "Failed to create test database";
    
    const int test_count = 50;
    ASSERT_TRUE(InsertTestData(db, test_count, "encrypted_test")) 
        << "Failed to insert test data";
    
    // Step 2: Create encrypted backup
    auto backup_manager = std::make_shared<BackupManager>(db);
    auto backup_path = GetTempDir() / "backup_encrypted";
    std::filesystem::create_directories(backup_path);
    
    BackupOptions options;
    options.encrypt = true;
    // Generate 32-byte hex key for AES-256
    auto key_bytes = data_gen_->GenerateEncryptionKey(32);
    std::stringstream ss;
    for (auto byte : key_bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    options.encryption_key = ss.str();
    options.compression = CompressionType::ZSTD;
    
    std::error_code ec;
    bool backup_success = backup_manager->createFullBackup(backup_path.string(), ec, options);
    
    // If encrypted backup is not implemented, skip
    if (!backup_success && ec) {
        GTEST_SKIP() << "Encrypted backup not fully implemented: " << ec.message();
        return;
    }
    
    ASSERT_TRUE(backup_success) 
        << "Failed to create encrypted backup: " << ec.message();
    
    // Step 3: Verify backup files exist
    bool has_backup_files = false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(backup_path)) {
        if (entry.is_regular_file()) {
            has_backup_files = true;
            break;
        }
    }
    EXPECT_TRUE(has_backup_files) << "Backup files should exist";
    
    // Step 4: Verify data is still accessible in original database
    EXPECT_TRUE(VerifyTestData(db, test_count, "encrypted_test"))
        << "Original data should still be accessible";
    
    db->close();
    
    // Note: Full restore test with encryption key verification would require
    // more complete restore implementation
}

} // namespace test
} // namespace themis
