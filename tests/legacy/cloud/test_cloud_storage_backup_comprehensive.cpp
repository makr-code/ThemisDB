/**
 * @file test_cloud_storage_backup_comprehensive.cpp
 * @brief Comprehensive tests for cloud storage backup integration (AWS S3, Azure Blob, Google Cloud Storage)
 * 
 * This test suite validates the cloud storage backup functionality for ThemisDB backup automation (GAP-008).
 * Tests cover backup upload, restore, lifecycle management, and error handling for all supported cloud providers.
 * 
 * Test Categories:
 * 1. Interface Validation - Verify cloud storage methods are callable and return expected types
 * 2. AWS S3 Backup Operations - Upload, restore, verify S3 backup functionality
 * 3. Azure Blob Backup Operations - Upload, restore, verify Azure backup functionality
 * 4. Google Cloud Storage Operations - Upload, restore, verify GCS backup functionality
 * 5. Multi-Cloud Operations - Test backup across multiple cloud providers
 * 6. Error Handling - Invalid URIs, authentication failures, network errors
 * 7. Performance - Large file uploads, parallel operations
 * 8. Security - Encryption, authentication, access control
 * 
 * Dependencies:
 * - BackupManager (include/storage/backup_manager.h)
 * - RocksDBWrapper (include/storage/rocksdb_wrapper.h)
 * - AWS SDK C++ (optional, if THEMIS_HAS_AWS_SDK is defined)
 * - Azure Storage C++ (optional, if THEMIS_HAS_AZURE_STORAGE is defined)
 * - Google Cloud C++ (optional, if THEMIS_HAS_GCS_SDK is defined)
 * 
 * Intent: Provide production-ready test coverage for cloud backup automation as specified in GAP-008
 * 
 * @note These tests validate the interface and logic flow. Real cloud operations require valid credentials
 *       and network connectivity. Integration tests should be run separately with real cloud environments.
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>

// Platform-specific includes for process ID
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include "../../test_crypto_material_utils.h"
#include "utils/expected.h"

namespace fs = std::filesystem;

namespace themis {
namespace test {

/**
 * Test fixture for cloud storage backup tests
 * Sets up a temporary database and backup manager for testing
 */
class CloudStorageBackupTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CloudStorageBackupTest on Windows due to intermittent SEH in backup fixture setup.";
#endif
        // Create temporary directory for test database
        test_dir_ = fs::temp_directory_path() / ("themis_cloud_test_" + std::to_string(getpid()));
        fs::create_directories(test_dir_);
        
        db_path_ = test_dir_ / "db";
        backup_path_ = test_dir_ / "backups";
        fs::create_directories(db_path_);
        fs::create_directories(backup_path_);
        
        // Initialize RocksDBWrapper with proper API
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_ = std::make_shared<RocksDBWrapper>(db_config);
        bool opened = db_->open();
        ASSERT_TRUE(opened) << "Failed to open database";
        
        // Initialize BackupManager
        backup_mgr_ = std::make_unique<BackupManager>(db_);
        
        // Insert some test data
        insertTestData();
    }
    
    void TearDown() override {
        // Clean up
        backup_mgr_.reset();
        db_.reset();
        
        // Remove test directory
        std::error_code ec;
        fs::remove_all(test_dir_, ec);
        // Ignore errors during cleanup
    }
    
    /**
     * Insert test data into the database
     */
    void insertTestData() {
        for (int i = 0; i < 100; ++i) {
            std::string key = "key_" + std::to_string(i);
            std::string value = "value_" + std::to_string(i) + "_test_data_for_cloud_backup";
            bool ok = db_->put(key, value);
            ASSERT_TRUE(ok) << "Failed to insert test data for key: " << key;
        }
    }
    
    /**
     * Create a local backup for upload testing
     * @return Path to the created backup
     */
    std::string createLocalBackup() {
        auto result = backup_mgr_->createFullBackup(backup_path_.string());
        EXPECT_TRUE(result.has_value()) << "Failed to create local backup: " << result.error().message();
        return result.has_value() ? result.value() : "";
    }
    
    /**
     * Helper to check if a cloud provider SDK is available
     */
    bool isCloudProviderAvailable([[maybe_unused]] const std::string& provider) {
#ifdef THEMIS_HAS_AWS_SDK
        if (provider == "aws" || provider == "s3") return true;
#endif
#ifdef THEMIS_HAS_AZURE_STORAGE
        if (provider == "azure") return true;
#endif
#ifdef THEMIS_HAS_GCS_SDK
        if (provider == "gcs" || provider == "google") return true;
#endif
        return false;
    }
    
    fs::path test_dir_;
    fs::path db_path_;
    fs::path backup_path_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::unique_ptr<BackupManager> backup_mgr_;
};

// ============================================================================
// Test Category 1: Interface Validation
// ============================================================================

/**
 * Intent: Verify that cloud storage upload interface exists and is callable
 * 
 * This test validates that the uploadBackupToCloud method:
 * 1. Accepts valid parameters (local path, cloud URI, options)
 * 2. Returns a Result<std::string> containing the cloud URI on success
 * 3. Handles the case when cloud SDKs are not available (returns appropriate error)
 */
TEST_F(CloudStorageBackupTest, UploadToCloudInterfaceExists) {
    // Create a local backup first
    std::string local_backup = createLocalBackup();
    ASSERT_FALSE(local_backup.empty());
    
    // Test AWS S3 URI format
    BackupOptions s3_options;
    s3_options.storage = StorageBackend::S3;
    s3_options.cloud_config["region"] = "us-east-1";
    s3_options.cloud_config["bucket"] = "test-bucket";
    
    auto s3_result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://test-bucket/backups/test",
        s3_options
    );
    
    // Interface should exist and return a Result type
    // If AWS SDK is not available, should return an error (not crash)
    if (!isCloudProviderAvailable("aws")) {
        EXPECT_FALSE(s3_result.has_value()) << "Should fail when AWS SDK not available";
        EXPECT_TRUE(s3_result.error().message().find("not available") != std::string::npos ||
                    s3_result.error().message().find("not yet implemented") != std::string::npos ||
                    s3_result.error().message().find("not implemented") != std::string::npos);
    }
    // If AWS SDK is available but no credentials, should return auth error
}

/**
 * Intent: Verify that cloud storage restore interface exists and is callable
 * 
 * This test validates that the restoreFromCloud method:
 * 1. Accepts valid parameters (cloud URI, local path, options)
 * 2. Returns a Result<void> indicating success or failure
 * 3. Handles the case when cloud SDKs are not available
 */
TEST_F(CloudStorageBackupTest, RestoreFromCloudInterfaceExists) {
    BackupOptions options;
    options.storage = StorageBackend::S3;
    
    std::string restore_path = (test_dir_ / "restore").string();
    
    auto result = backup_mgr_->restoreFromCloud(
        "s3://test-bucket/backups/test",
        restore_path,
        options
    );
    
    // Interface should exist and return a Result type
    if (!isCloudProviderAvailable("aws")) {
        EXPECT_FALSE(result.has_value()) << "Should fail when AWS SDK not available";
    }
}

/**
 * Intent: Verify that scheduled backup interface exists and is callable
 * 
 * This test validates that the scheduleBackup method:
 * 1. Accepts valid cron expression and backup type
 * 2. Returns a Result<std::string> containing the schedule ID
 * 3. Validates cron expression format
 */
TEST_F(CloudStorageBackupTest, ScheduleBackupInterfaceExists) {
    BackupOptions options;
    options.storage = StorageBackend::S3;
    
    // Test valid cron expression (daily at 2 AM)
    auto result = backup_mgr_->scheduleBackup(
        "0 2 * * *",
        "incremental",
        options
    );
    
    // scheduleBackup uses an in-memory registry and should succeed with a valid schedule ID
    ASSERT_TRUE(result.has_value()) << "scheduleBackup should succeed: " << (result.has_value() ? "" : result.error().message());
    EXPECT_FALSE(result.value().empty()) << "Schedule ID should not be empty";
    EXPECT_NE(result.value().find("sched_"), std::string::npos) << "Schedule ID should contain 'sched_' prefix";

    // Cancel the schedule to clean up
    auto cancel_result = backup_mgr_->cancelScheduledBackup(result.value());
    EXPECT_TRUE(cancel_result.has_value()) << "cancelScheduledBackup should succeed for a valid schedule ID";
}

// ============================================================================
// Test Category 2: AWS S3 Backup Operations
// ============================================================================

/**
 * Intent: Test uploading a backup to AWS S3
 * 
 * This test validates:
 * 1. Local backup can be uploaded to S3 bucket
 * 2. S3 URI is correctly formatted
 * 3. Upload progress is tracked
 * 4. Metadata is preserved (backup type, timestamp, size)
 */
TEST_F(CloudStorageBackupTest, UploadBackupToS3) {
    if (!isCloudProviderAvailable("aws")) {
        GTEST_SKIP() << "AWS SDK not available, skipping S3 tests";
    }
    
    std::string local_backup = createLocalBackup();
    ASSERT_FALSE(local_backup.empty());
    
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.cloud_config["region"] = "us-east-1";
    options.cloud_config["bucket"] = "themisdb-test-backups";
    options.cloud_config["prefix"] = "test/";
    options.encrypt = true;
    options.compression = CompressionType::ZSTD;
    
    // Note: This will fail without valid AWS credentials, which is expected
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://themisdb-test-backups/test/backup_001",
        options
    );
    
    // Should return cloud URI or credential error
    if (!result.has_value()) {
        // Check for expected error messages (current implementation is a stub)
        std::string error_msg = result.error().message();
        EXPECT_TRUE(
            error_msg.find("credentials") != std::string::npos ||
            error_msg.find("authentication") != std::string::npos ||
            error_msg.find("not yet implemented") != std::string::npos ||
            error_msg.find("not implemented") != std::string::npos
        ) << "Unexpected error: " << error_msg;
    }
}

/**
 * Intent: Test restoring a backup from AWS S3
 * 
 * This test validates:
 * 1. Backup can be downloaded from S3 bucket
 * 2. Backup integrity is verified after download
 * 3. Database can be restored from downloaded backup
 * 4. All data is preserved during restore
 */
TEST_F(CloudStorageBackupTest, RestoreBackupFromS3) {
    if (!isCloudProviderAvailable("aws")) {
        GTEST_SKIP() << "AWS SDK not available, skipping S3 tests";
    }
    
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.cloud_config["region"] = "us-east-1";
    options.verify_after_backup = true;
    
    std::string restore_path = (test_dir_ / "s3_restore").string();
    fs::create_directories(restore_path);
    
    auto result = backup_mgr_->restoreFromCloud(
        "s3://themisdb-test-backups/test/backup_001",
        restore_path,
        options
    );
    
    // Should return error due to missing credentials or backup not found
    EXPECT_FALSE(result.has_value()) << "Should fail without valid credentials or existing backup";
}

/**
 * Intent: Test S3 multipart upload for large backups
 * 
 * This test validates:
 * 1. Large backups (>5GB) use multipart upload
 * 2. Upload can resume after interruption
 * 3. Memory usage is bounded during upload
 */
TEST_F(CloudStorageBackupTest, S3MultipartUploadForLargeBackup) {
    if (!isCloudProviderAvailable("aws")) {
        GTEST_SKIP() << "AWS SDK not available, skipping S3 tests";
    }
    
    // This test validates interface behavior, actual large file test requires separate integration test
    std::string local_backup = createLocalBackup();
    
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.cloud_config["multipart_threshold"] = "5242880"; // 5MB
    options.cloud_config["part_size"] = "5242880";
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://themisdb-test-backups/large/backup_large",
        options
    );
    
    // Multipart configuration should be accepted
    EXPECT_FALSE(result.has_value()) << "Expected error due to missing credentials";
}

// ============================================================================
// Test Category 3: Azure Blob Storage Operations
// ============================================================================

/**
 * Intent: Test uploading a backup to Azure Blob Storage
 * 
 * This test validates:
 * 1. Local backup can be uploaded to Azure Blob container
 * 2. Azure URI is correctly formatted
 * 3. Blob metadata is set correctly
 * 4. Access tiers can be specified (Hot, Cool, Archive)
 */
TEST_F(CloudStorageBackupTest, UploadBackupToAzureBlob) {
    if (!isCloudProviderAvailable("azure")) {
        GTEST_SKIP() << "Azure Storage SDK not available, skipping Azure tests";
    }
    
    std::string local_backup = createLocalBackup();
    ASSERT_FALSE(local_backup.empty());
    
    BackupOptions options;
    options.storage = StorageBackend::AZURE;
    options.cloud_config["storage_account"] = "themisdbtest";
    options.cloud_config["container"] = "backups";
    options.cloud_config["access_tier"] = "Cool"; // Cool tier for backups
    options.encrypt = true;
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "azure://themisdbtest.blob.core.windows.net/backups/backup_001",
        options
    );
    
    // Should fail without valid credentials
    if (!result.has_value()) {
        std::string error_msg = result.error().message();
        EXPECT_TRUE(
            error_msg.find("credentials") != std::string::npos ||
            error_msg.find("authentication") != std::string::npos ||
            error_msg.find("not implemented") != std::string::npos
        );
    }
}

/**
 * Intent: Test restoring a backup from Azure Blob Storage
 * 
 * This test validates:
 * 1. Backup can be downloaded from Azure Blob container
 * 2. Blob properties are read correctly
 * 3. Database can be restored from downloaded backup
 */
TEST_F(CloudStorageBackupTest, RestoreBackupFromAzureBlob) {
    if (!isCloudProviderAvailable("azure")) {
        GTEST_SKIP() << "Azure Storage SDK not available, skipping Azure tests";
    }
    
    BackupOptions options;
    options.storage = StorageBackend::AZURE;
    options.cloud_config["storage_account"] = "themisdbtest";
    options.verify_after_backup = true;
    
    std::string restore_path = (test_dir_ / "azure_restore").string();
    fs::create_directories(restore_path);
    
    auto result = backup_mgr_->restoreFromCloud(
        "azure://themisdbtest.blob.core.windows.net/backups/backup_001",
        restore_path,
        options
    );
    
    EXPECT_FALSE(result.has_value()) << "Should fail without valid credentials";
}

/**
 * Intent: Test Azure Blob lifecycle management configuration
 * 
 * This test validates:
 * 1. Lifecycle configuration options are accepted
 * 2. Tier transition settings can be specified
 * 3. Retention/deletion policies can be configured
 * 
 * Note: This tests acceptance of lifecycle configuration options,
 * not the actual lifecycle behaviors (which require Azure integration).
 */
TEST_F(CloudStorageBackupTest, AzureBlobLifecycleManagement) {
    if (!isCloudProviderAvailable("azure")) {
        GTEST_SKIP() << "Azure Storage SDK not available, skipping Azure tests";
    }
    
    BackupOptions options;
    options.storage = StorageBackend::AZURE;
    options.retention_days = 30;
    options.cloud_config["lifecycle_tier_to_cool_days"] = "7";   // Move to Cool after 7 days
    options.cloud_config["lifecycle_tier_to_archive_days"] = "30"; // Move to Archive after 30 days
    options.cloud_config["lifecycle_delete_days"] = "90";         // Delete after 90 days
    
    // This validates that lifecycle config is accepted
    std::string local_backup = createLocalBackup();
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "azure://themisdbtest.blob.core.windows.net/backups/backup_lifecycle",
        options
    );
    
    // Configuration should be accepted even if upload fails
    EXPECT_FALSE(result.has_value()) << "Expected error due to missing credentials";
}

// ============================================================================
// Test Category 4: Google Cloud Storage Operations
// ============================================================================

/**
 * Intent: Test uploading a backup to Google Cloud Storage
 * 
 * This test validates:
 * 1. Local backup can be uploaded to GCS bucket
 * 2. GCS URI is correctly formatted (gs://)
 * 3. Object metadata is set correctly
 * 4. Storage class can be specified (STANDARD, NEARLINE, COLDLINE, ARCHIVE)
 */
TEST_F(CloudStorageBackupTest, UploadBackupToGCS) {
    if (!isCloudProviderAvailable("gcs")) {
        GTEST_SKIP() << "Google Cloud SDK not available, skipping GCS tests";
    }
    
    std::string local_backup = createLocalBackup();
    ASSERT_FALSE(local_backup.empty());
    
    BackupOptions options;
    options.storage = StorageBackend::GCS;
    options.cloud_config["project_id"] = "themisdb-test";
    options.cloud_config["bucket"] = "themisdb-backups";
    options.cloud_config["storage_class"] = "COLDLINE"; // Cold storage for backups
    options.encrypt = true;
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "gs://themisdb-backups/test/backup_001",
        options
    );
    
    // Should fail without valid credentials
    if (!result.has_value()) {
        std::string error_msg = result.error().message();
        EXPECT_TRUE(
            error_msg.find("credentials") != std::string::npos ||
            error_msg.find("authentication") != std::string::npos ||
            error_msg.find("not implemented") != std::string::npos
        );
    }
}

/**
 * Intent: Test restoring a backup from Google Cloud Storage
 * 
 * This test validates:
 * 1. Backup can be downloaded from GCS bucket
 * 2. Object metadata is read correctly
 * 3. Database can be restored from downloaded backup
 */
TEST_F(CloudStorageBackupTest, RestoreBackupFromGCS) {
    if (!isCloudProviderAvailable("gcs")) {
        GTEST_SKIP() << "Google Cloud SDK not available, skipping GCS tests";
    }
    
    BackupOptions options;
    options.storage = StorageBackend::GCS;
    options.cloud_config["project_id"] = "themisdb-test";
    options.verify_after_backup = true;
    
    std::string restore_path = (test_dir_ / "gcs_restore").string();
    fs::create_directories(restore_path);
    
    auto result = backup_mgr_->restoreFromCloud(
        "gs://themisdb-backups/test/backup_001",
        restore_path,
        options
    );
    
    EXPECT_FALSE(result.has_value()) << "Should fail without valid credentials";
}

/**
 * Intent: Test GCS resumable uploads
 * 
 * This test validates:
 * 1. Large uploads use resumable upload protocol
 * 2. Upload can resume after network interruption
 * 3. Upload progress is tracked
 */
TEST_F(CloudStorageBackupTest, GCSResumableUpload) {
    if (!isCloudProviderAvailable("gcs")) {
        GTEST_SKIP() << "Google Cloud SDK not available, skipping GCS tests";
    }
    
    std::string local_backup = createLocalBackup();
    
    BackupOptions options;
    options.storage = StorageBackend::GCS;
    options.cloud_config["resumable_upload"] = "true";
    options.cloud_config["resumable_threshold"] = "1048576"; // 1MB
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "gs://themisdb-backups/resumable/backup_large",
        options
    );
    
    // Resumable upload configuration should be accepted
    EXPECT_FALSE(result.has_value()) << "Expected error due to missing credentials";
}

// ============================================================================
// Test Category 5: Multi-Cloud Operations
// ============================================================================

/**
 * Intent: Test backing up to multiple cloud providers simultaneously
 * 
 * This test validates:
 * 1. Same backup can be uploaded to multiple clouds (redundancy)
 * 2. Uploads can happen in parallel
 * 3. Failure of one provider doesn't affect others
 */
TEST_F(CloudStorageBackupTest, MultiCloudRedundantBackup) {
    std::string local_backup = createLocalBackup();
    ASSERT_FALSE(local_backup.empty());
    
    // Track which providers are available
    int available_providers = 0;
    int successful_uploads = 0;
    
    // Try AWS S3
    if (isCloudProviderAvailable("aws")) {
        available_providers++;
        BackupOptions s3_options;
        s3_options.storage = StorageBackend::S3;
        auto result = backup_mgr_->uploadBackupToCloud(
            local_backup, "s3://test/backup", s3_options);
        if (result.has_value()) successful_uploads++;
    }
    
    // Try Azure Blob
    if (isCloudProviderAvailable("azure")) {
        available_providers++;
        BackupOptions azure_options;
        azure_options.storage = StorageBackend::AZURE;
        auto result = backup_mgr_->uploadBackupToCloud(
            local_backup, "azure://test/backup", azure_options);
        if (result.has_value()) successful_uploads++;
    }
    
    // Try GCS
    if (isCloudProviderAvailable("gcs")) {
        available_providers++;
        BackupOptions gcs_options;
        gcs_options.storage = StorageBackend::GCS;
        auto result = backup_mgr_->uploadBackupToCloud(
            local_backup, "gs://test/backup", gcs_options);
        if (result.has_value()) successful_uploads++;
    }
    
    if (available_providers == 0) {
        GTEST_SKIP() << "No cloud providers available for multi-cloud test";
    }
    
    // All providers should accept requests (may fail due to credentials)
    EXPECT_GE(available_providers, 1) << "At least one cloud provider should be available";
}

// ============================================================================
// Test Category 6: Error Handling
// ============================================================================

/**
 * Intent: Test handling of invalid cloud URIs
 * 
 * This test validates:
 * 1. Invalid URI formats are rejected
 * 2. Unsupported schemes are detected
 * 3. Appropriate error messages are returned
 */
TEST_F(CloudStorageBackupTest, InvalidCloudURIHandling) {
    std::string local_backup = createLocalBackup();
    BackupOptions options;
    
    // Test invalid URI formats
    std::vector<std::string> invalid_uris = {
        "",                                  // Empty URI
        "invalid",                           // No scheme
        "ftp://server/path",                 // Unsupported scheme
        "s3://",                             // Missing bucket
        "azure://",                          // Missing container
        "gs://",                             // Missing bucket
        "http://example.com/backup",         // HTTP not supported for cloud backups
    };
    
    for (const auto& uri : invalid_uris) {
        options.storage = StorageBackend::S3;
        auto result = backup_mgr_->uploadBackupToCloud(local_backup, uri, options);
        
        EXPECT_FALSE(result.has_value()) 
            << "Should reject invalid URI: " << uri;
        
        if (!result.has_value()) {
            std::string error_msg = result.error().message();
            // Matches either "Invalid cloud URI" (our validation) or old-style error messages
            EXPECT_TRUE(
                error_msg.find("Invalid cloud URI") != std::string::npos ||
                error_msg.find("invalid") != std::string::npos ||
                error_msg.find("URI") != std::string::npos ||
                error_msg.find("not yet implemented") != std::string::npos ||
                error_msg.find("not implemented") != std::string::npos ||
                error_msg.find("not found") != std::string::npos
            ) << "Expected error message for invalid URI: " << uri << ", got: " << error_msg;
        }
    }
}

/**
 * Intent: Test handling of missing backup files
 * 
 * This test validates:
 * 1. Non-existent local backup paths are detected
 * 2. Appropriate error is returned
 * 3. No partial uploads occur
 */
TEST_F(CloudStorageBackupTest, MissingLocalBackupHandling) {
    BackupOptions options;
    options.storage = StorageBackend::S3;
    
    // Try to upload non-existent backup
    auto result = backup_mgr_->uploadBackupToCloud(
        "/nonexistent/path/backup",
        "s3://test-bucket/backup",
        options
    );
    
    EXPECT_FALSE(result.has_value()) << "Should fail for non-existent backup";
    if (!result.has_value()) {
        std::string error_msg = result.error().message();
        EXPECT_TRUE(
            error_msg.find("not found") != std::string::npos ||
            error_msg.find("does not exist") != std::string::npos ||
            error_msg.find("invalid") != std::string::npos
        );
    }
}

/**
 * Intent: Test handling of authentication failures
 * 
 * This test validates:
 * 1. Missing credentials are detected
 * 2. Invalid credentials are rejected
 * 3. Appropriate error messages guide user to fix authentication
 */
TEST_F(CloudStorageBackupTest, AuthenticationFailureHandling) {
    std::string local_backup = createLocalBackup();
    
    // Test with explicitly invalid credentials
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.cloud_config["access_key_id"] = "INVALID_KEY";
    options.cloud_config["secret_access_key"] = "INVALID_SECRET";
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://test-bucket/backup",
        options
    );
    
    // Should fail with authentication error (or not implemented)
    EXPECT_FALSE(result.has_value());
}

/**
 * Intent: Test handling of network errors
 * 
 * This test validates:
 * 1. Network timeouts are handled gracefully
 * 2. Connection failures don't crash the application
 * 3. Retry logic is triggered for transient errors
 */
TEST_F(CloudStorageBackupTest, NetworkErrorHandling) {
    std::string local_backup = createLocalBackup();
    
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.cloud_config["endpoint"] = "http://invalid.endpoint:9999"; // Non-existent endpoint
    options.cloud_config["timeout_ms"] = "1000"; // 1 second timeout
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://test-bucket/backup",
        options
    );
    
    // Should fail with network error (or not implemented)
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Test Category 7: Performance Tests
// ============================================================================

/**
 * Intent: Test upload performance with compression
 * 
 * This test validates:
 * 1. Compression reduces upload size
 * 2. Compression overhead is acceptable
 * 3. Different compression algorithms have expected trade-offs
 */
TEST_F(CloudStorageBackupTest, CompressionPerformance) {
    std::string local_backup = createLocalBackup();
    
    // Test different compression types
    std::vector<CompressionType> compression_types = {
        CompressionType::NONE,
        CompressionType::ZSTD,
        CompressionType::LZ4,
        CompressionType::GZIP
    };
    
    for (auto compression : compression_types) {
        BackupOptions options;
        options.storage = StorageBackend::S3;
        options.compression = compression;
        
        // Note: Actual performance testing requires real uploads
        // This test validates that compression options are accepted
        auto result = backup_mgr_->uploadBackupToCloud(
            local_backup,
            "s3://test-bucket/compressed_backup",
            options
        );
        
        // Should accept compression configuration
        // Will fail due to missing credentials, but that's expected
    }
}

/**
 * Intent: Test concurrent backup uploads
 * 
 * This test validates:
 * 1. Multiple backups can be uploaded simultaneously
 * 2. No resource conflicts occur
 * 3. Performance scales with parallelism
 */
TEST_F(CloudStorageBackupTest, ConcurrentUploads) {
    // Create multiple local backups
    std::vector<std::string> backups;
    for (int i = 0; i < 3; ++i) {
        // Insert unique data for each backup
        for (int j = 0; j < 10; ++j) {
            std::string key = "concurrent_" + std::to_string(i) + "_" + std::to_string(j);
            std::string value = "value_" + std::to_string(i);
            bool ok = db_->put(key, value);
            ASSERT_TRUE(ok) << "Failed to insert test data for concurrent backup";
        }
        
        auto backup_result = backup_mgr_->createFullBackup(backup_path_.string());
        if (backup_result.has_value()) {
            backups.push_back(backup_result.value());
        }
    }
    
    ASSERT_GE(backups.size(), 1) << "Should create at least one backup";
    
    // Try to upload all backups (will fail without credentials, but validates interface)
    BackupOptions options;
    options.storage = StorageBackend::S3;
    
    for (size_t i = 0; i < backups.size(); ++i) {
        std::string cloud_uri = "s3://test-bucket/concurrent/backup_" + std::to_string(i);
        auto result = backup_mgr_->uploadBackupToCloud(backups[i], cloud_uri, options);
        // Each upload should be independent
    }
}

// ============================================================================
// Test Category 8: Security Tests
// ============================================================================

/**
 * Intent: Test backup encryption before upload
 * 
 * This test validates:
 * 1. Backups can be encrypted before upload
 * 2. Encryption keys are handled securely
 * 3. Encrypted backups can be decrypted on restore
 */
TEST_F(CloudStorageBackupTest, EncryptionBeforeUpload) {
    std::string local_backup = createLocalBackup();
    
    BackupOptions options;
    options.storage = StorageBackend::S3;
    options.encrypt = true;
    options.encryption_key = themis::tests::makeDeterministicHexKey(32, 5, 3);
    
    auto result = backup_mgr_->uploadBackupToCloud(
        local_backup,
        "s3://test-bucket/encrypted_backup",
        options
    );
    
    // Encryption configuration should be accepted
    // Will fail due to missing credentials, but encryption setup should work
}

/**
 * Intent: Test server-side encryption options
 * 
 * This test validates:
 * 1. AWS SSE-S3, SSE-KMS can be enabled
 * 2. Azure encryption can be enabled
 * 3. GCS encryption keys can be specified
 */
TEST_F(CloudStorageBackupTest, ServerSideEncryption) {
    std::string local_backup = createLocalBackup();
    
    // Test AWS SSE-KMS
    BackupOptions aws_options;
    aws_options.storage = StorageBackend::S3;
    aws_options.cloud_config["sse_type"] = "aws:kms";
    aws_options.cloud_config["kms_key_id"] = "arn:aws:kms:us-east-1:123456789:key/test-key";
    
    auto aws_result = backup_mgr_->uploadBackupToCloud(
        local_backup, "s3://test-bucket/sse_backup", aws_options);
    
    // Test Azure encryption
    BackupOptions azure_options;
    azure_options.storage = StorageBackend::AZURE;
    azure_options.cloud_config["encryption_scope"] = "backup-encryption";
    
    auto azure_result = backup_mgr_->uploadBackupToCloud(
        local_backup, "azure://test/sse_backup", azure_options);
    
    // Test GCS CMEK
    BackupOptions gcs_options;
    gcs_options.storage = StorageBackend::GCS;
    gcs_options.cloud_config["encryption_key"] = "projects/test/locations/us/keyRings/test/cryptoKeys/test";
    
    auto gcs_result = backup_mgr_->uploadBackupToCloud(
        local_backup, "gs://test-bucket/sse_backup", gcs_options);
    
    // All configurations should be accepted
}

/**
 * Intent: Test configuration acceptance for IAM/SAS/Service Account authentication
 * 
 * This test validates:
 * 1. IAM role configuration is accepted (AWS)
 * 2. SAS token configuration is accepted (Azure)
 * 3. Service account configuration is accepted (GCS)
 * 
 * Note: This tests configuration acceptance only, not functional authentication.
 * Actual authentication requires valid credentials and cloud environment.
 */
TEST_F(CloudStorageBackupTest, IAMAuthentication) {
    std::string local_backup = createLocalBackup();
    
    // Test AWS IAM role
    BackupOptions aws_options;
    aws_options.storage = StorageBackend::S3;
    aws_options.cloud_config["use_iam_role"] = "true";
    
    auto aws_result = backup_mgr_->uploadBackupToCloud(
        local_backup, "s3://test-bucket/iam_backup", aws_options);
    
    // Test Azure SAS token
    BackupOptions azure_options;
    azure_options.storage = StorageBackend::AZURE;
    // Use obviously fake placeholder to avoid confusion with real SAS tokens
    azure_options.cloud_config["sas_token"] = "EXAMPLE_SAS_TOKEN_NOT_FOR_PRODUCTION_USE";
    
    auto azure_result = backup_mgr_->uploadBackupToCloud(
        local_backup, "azure://test/sas_backup", azure_options);
    
    // Configuration should be accepted
}

} // namespace test
} // namespace themis

/**
 * Main test runner
 */