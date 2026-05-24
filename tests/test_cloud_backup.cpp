/*
 * ThemisDB | File: test_cloud_backup.cpp | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 93/100
 * Gap Summary: total=11; TODO=1, Stub=1, Unimpl=0, Mock=9, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include <gtest/gtest.h>
#include "sharding/cloud_backup.h"
#include "sharding/cloud_agent.h"
#include "sharding/shard_topology.h"
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <memory>
#include <filesystem>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#define setenv(name, value, overwrite) _putenv_s(name, value)
#define unsetenv(name) _putenv_s(name, "")
#endif

using namespace themis;
using namespace themis::sharding;

// ============================================================================
// Cloud Backup Coordinator Tests
// ============================================================================

class CloudBackupTest : public ::testing::Test {
protected:
    void SetUp() override {
        setS3UploadFn({});
        setS3DownloadFn({});
        setS3DeleteFn({});
        setS3ListFn({});
        setS3ExistsFn({});
        setGCSUploadFn({});
        setGCSDownloadFn({});
        setGCSDeleteFn({});
        setGCSListFn({});
        setGCSExistsFn({});
        setAzureUploadFn({});
        setAzureDownloadFn({});
        setAzureDeleteFn({});
        setAzureListFn({});
        setAzureExistsFn({});

        // Create unique temporary paths for each test
        auto temp_base = std::filesystem::temp_directory_path();
        auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        db_path_ = temp_base / ("test_cloud_backup_db_" + std::to_string(timestamp));
        local_backup_dir_ = temp_base / ("test_backups_" + std::to_string(timestamp));
        
        // Create temporary database for testing
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.memtable_size_mb = 16;
        db_config.block_cache_size_mb = 16;
        
        db_ = std::make_shared<RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_->open());
        
        // Create backup manager
        backup_manager_ = std::make_shared<BackupManager>(db_);
        
        // Create cloud agent (minimal config for testing)
        ShardTopology::Config topology_config;
        topology_ = std::make_shared<ShardTopology>(topology_config);
        
        CloudAgent::Config agent_config;
        agent_config.agent_id = "test_agent";
        agent_config.enable_health_monitoring = false;
        cloud_agent_ = std::make_shared<CloudAgent>(topology_, nullptr, nullptr, agent_config);
        
        // Enable mock mode for testing
        setenv("THEMIS_CLOUD_BACKUP_MOCK", "1", 1);
    }
    
    void TearDown() override {
        setS3UploadFn({});
        setS3DownloadFn({});
        setS3DeleteFn({});
        setS3ListFn({});
        setS3ExistsFn({});
        setGCSUploadFn({});
        setGCSDownloadFn({});
        setGCSDeleteFn({});
        setGCSListFn({});
        setGCSExistsFn({});
        setAzureUploadFn({});
        setAzureDownloadFn({});
        setAzureDeleteFn({});
        setAzureListFn({});
        setAzureExistsFn({});

        coordinator_.reset();
        cloud_agent_.reset();
        topology_.reset();
        backup_manager_.reset();
        db_.reset();
        
        // Clean up temporary directories
        std::filesystem::remove_all(db_path_);
        std::filesystem::remove_all(local_backup_dir_);
        unsetenv("THEMIS_CLOUD_BACKUP_MOCK");
    }
    
    std::filesystem::path db_path_;
    std::filesystem::path local_backup_dir_;
    
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<BackupManager> backup_manager_;
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<CloudAgent> cloud_agent_;
    std::unique_ptr<CloudBackupCoordinator> coordinator_;
};

// Test: Create coordinator with S3 config
TEST_F(CloudBackupTest, CreateCoordinatorS3) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    EXPECT_NE(coordinator_, nullptr);
}

// Test: Create coordinator with Azure config
TEST_F(CloudBackupTest, CreateCoordinatorAzure) {
    CloudBackupConfig config;
    config.provider = "azure";
    config.azure_account = "testaccount";
    config.azure_container = "backups";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    EXPECT_NE(coordinator_, nullptr);
}

// Test: Create coordinator with GCS config
TEST_F(CloudBackupTest, CreateCoordinatorGCS) {
    CloudBackupConfig config;
    config.provider = "gcs";
    config.gcs_project_id = "test-project";
    config.gcs_bucket = "test-bucket";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    EXPECT_NE(coordinator_, nullptr);
}

// Test: Create backup (mock mode)
TEST_F(CloudBackupTest, CreateBackupMockMode) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1", "shard2"};
    bool success = coordinator_->createBackup("backup-001", shard_ids);
    
    // In mock mode, should succeed
    EXPECT_TRUE(success);
}

// Test: List backups
TEST_F(CloudBackupTest, ListBackups) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    // Initially empty
    auto backups = coordinator_->listBackups();
    size_t initial_count = backups.size();
    
    // Create a backup
    std::vector<std::string> shard_ids = {"shard1"};
    coordinator_->createBackup("backup-001", shard_ids);
    
    // Should have one more backup
    backups = coordinator_->listBackups();
    EXPECT_EQ(backups.size(), initial_count + 1);
}

// Test: Get backup info
TEST_F(CloudBackupTest, GetBackupInfo) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1", "shard2"};
    coordinator_->createBackup("backup-001", shard_ids);
    
    auto info = coordinator_->getBackupInfo("backup-001");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->backup_id, "backup-001");
    EXPECT_EQ(info->shard_ids.size(), 2);
    EXPECT_EQ(info->storage_provider, "s3");
}

// Test: Get non-existent backup info
TEST_F(CloudBackupTest, GetNonExistentBackupInfo) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    auto info = coordinator_->getBackupInfo("non-existent");
    EXPECT_FALSE(info.has_value());
}

// Test: Delete backup
TEST_F(CloudBackupTest, DeleteBackup) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1"};
    coordinator_->createBackup("backup-to-delete", shard_ids);
    
    // Verify exists
    auto info = coordinator_->getBackupInfo("backup-to-delete");
    EXPECT_TRUE(info.has_value());
    
    // Delete
    bool deleted = coordinator_->deleteBackup("backup-to-delete");
    EXPECT_TRUE(deleted);
    
    // Verify deleted
    info = coordinator_->getBackupInfo("backup-to-delete");
    EXPECT_FALSE(info.has_value());
}

// Test: Delete non-existent backup
TEST_F(CloudBackupTest, DeleteNonExistentBackup) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    bool deleted = coordinator_->deleteBackup("non-existent");
    EXPECT_FALSE(deleted);
}

// Test: Set replication target
TEST_F(CloudBackupTest, SetReplicationTarget) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> endpoints = {
        "shard1.eu-west-1.example.com:18765",
        "shard2.eu-west-1.example.com:18765"
    };
    
    bool success = coordinator_->setReplicationTarget("eu-west-1", endpoints);
    EXPECT_TRUE(success);
}

// Test: Enable continuous replication
TEST_F(CloudBackupTest, EnableContinuousReplication) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    // Set up target first
    std::vector<std::string> endpoints = {"shard1.eu:18765"};
    coordinator_->setReplicationTarget("eu-west-1", endpoints);
    
    // Enable replication
    bool success = coordinator_->enableContinuousReplication("eu-west-1");
    EXPECT_TRUE(success);
}

// Test: Disable continuous replication
TEST_F(CloudBackupTest, DisableContinuousReplication) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    // Set up target
    std::vector<std::string> endpoints = {"shard1.eu:18765"};
    coordinator_->setReplicationTarget("eu-west-1", endpoints);
    coordinator_->enableContinuousReplication("eu-west-1");
    
    // Disable replication
    bool success = coordinator_->disableContinuousReplication("eu-west-1");
    EXPECT_TRUE(success);
}

// Test: Multiple backups
TEST_F(CloudBackupTest, MultipleBackups) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1", "shard2", "shard3"};
    
    // Create multiple backups
    coordinator_->createBackup("backup-001", shard_ids);
    coordinator_->createBackup("backup-002", shard_ids);
    coordinator_->createBackup("backup-003", shard_ids);
    
    auto backups = coordinator_->listBackups();
    EXPECT_GE(backups.size(), 3);
}

// Test: Backup with compression enabled
TEST_F(CloudBackupTest, BackupWithCompression) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    config.enable_compression = true;
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-compressed", shard_ids);
    EXPECT_TRUE(success);
}

// Test: Backup with encryption enabled
TEST_F(CloudBackupTest, BackupWithEncryption) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    config.enable_encryption = true;
    config.encryption_key = "test-encryption-key-32bytes!!!";
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-encrypted", shard_ids);
    EXPECT_TRUE(success);
}

// Test: Without mock mode (should fail without real SDK)
TEST_F(CloudBackupTest, WithoutMockModeFails) {
    // Disable mock mode
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");
    
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-no-mock", shard_ids);
    
    // Without SDK integration and without mock mode, should fail
    EXPECT_FALSE(success);
    
    // Re-enable for teardown
    setenv("THEMIS_CLOUD_BACKUP_MOCK", "1", 1);
}

// Test: Restore backup (mock mode)
TEST_F(CloudBackupTest, RestoreBackupMockMode) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();
    
    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );
    
    std::vector<std::string> shard_ids = {"shard1", "shard2"};
    
    // Create backup first
    coordinator_->createBackup("backup-for-restore", shard_ids);
    
    // Restore backup
    bool success = coordinator_->restoreBackup("backup-for-restore", shard_ids);
    EXPECT_TRUE(success);
}

TEST_F(CloudBackupTest, RestoreBackupUsesS3DownloadCallbackWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-for-s3-callback", shard_ids));

    bool called = false;
    setS3DownloadFn([&called](const std::string& bucket,
                              const std::string& remote_path,
                              const std::string& local_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-s3-callback/shard1");
        std::ofstream out(local_path);
        out << "callback-download";
        return out.good();
    });

    bool success = coordinator_->restoreBackup("backup-for-s3-callback", shard_ids);
    EXPECT_TRUE(success);
    EXPECT_TRUE(called);
}

TEST_F(CloudBackupTest, CreateBackupUsesS3UploadCallbackWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    bool called = false;
    setS3UploadFn([&called](const std::string& bucket,
                            const std::string& local_path,
                            const std::string& remote_path,
                            const std::map<std::string, std::string>& metadata) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-s3-upload-callback/shard1");
        EXPECT_TRUE(std::filesystem::exists(local_path));
        auto backup_id_it = metadata.find("backup_id");
        auto shard_id_it = metadata.find("shard_id");
        EXPECT_NE(backup_id_it, metadata.end());
        EXPECT_NE(shard_id_it, metadata.end());
        if (backup_id_it != metadata.end()) {
            EXPECT_EQ(backup_id_it->second, "backup-for-s3-upload-callback");
        }
        if (shard_id_it != metadata.end()) {
            EXPECT_EQ(shard_id_it->second, "shard1");
        }
        return true;
    });

    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-for-s3-upload-callback", shard_ids);
    EXPECT_TRUE(success);
    EXPECT_TRUE(called);
}

TEST_F(CloudBackupTest, DeleteBackupUsesGCSDeleteCallbackWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "gcs";
    config.gcs_project_id = "test-project";
    config.gcs_bucket = "test-bucket";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-for-gcs-delete-callback", shard_ids));

    bool called = false;
    setGCSDeleteFn([&called](const std::string& bucket, const std::string& remote_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-gcs-delete-callback/shard1");
        return true;
    });

    bool deleted = coordinator_->deleteBackup("backup-for-gcs-delete-callback");
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(called);
}

TEST_F(CloudBackupTest, DeleteBackupUsesS3DeleteCallbackWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-for-s3-delete-callback", shard_ids));

    bool called = false;
    setS3DeleteFn([&called](const std::string& bucket, const std::string& remote_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-s3-delete-callback/shard1");
        return true;
    });

    bool deleted = coordinator_->deleteBackup("backup-for-s3-delete-callback");
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(called);
}

TEST_F(CloudBackupTest, CreateAndRestoreUseGCSUploadDownloadCallbacksWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "gcs";
    config.gcs_project_id = "test-project";
    config.gcs_bucket = "test-bucket";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    bool upload_called = false;
    setGCSUploadFn([&upload_called](const std::string& bucket,
                                    const std::string& local_path,
                                    const std::string& remote_path,
                                    const std::map<std::string, std::string>&) {
        upload_called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-gcs-upload-download-callback/shard1");
        EXPECT_TRUE(std::filesystem::exists(local_path));
        return true;
    });

    bool download_called = false;
    setGCSDownloadFn([&download_called](const std::string& bucket,
                                        const std::string& remote_path,
                                        const std::string& local_path) {
        download_called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "backup-for-gcs-upload-download-callback/shard1");
        std::ofstream out(local_path);
        out << "gcs-callback-download";
        return out.good();
    });

    std::vector<std::string> shard_ids = {"shard1"};
    EXPECT_TRUE(coordinator_->createBackup("backup-for-gcs-upload-download-callback", shard_ids));
    EXPECT_TRUE(coordinator_->restoreBackup("backup-for-gcs-upload-download-callback", shard_ids));
    EXPECT_TRUE(upload_called);
    EXPECT_TRUE(download_called);
}

TEST_F(CloudBackupTest, CreateRestoreDeleteUseAzureCallbacksWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "azure";
    config.azure_account = "account";
    config.azure_container = "container";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    bool upload_called = false;
    setAzureUploadFn([&upload_called](const std::string& account_name,
                                      const std::string& container,
                                      const std::string& local_path,
                                      const std::string& remote_path,
                                      const std::map<std::string, std::string>&) {
        upload_called = true;
        EXPECT_EQ(account_name, "account");
        EXPECT_EQ(container, "container");
        EXPECT_EQ(remote_path, "backup-for-azure-callbacks/shard1");
        EXPECT_TRUE(std::filesystem::exists(local_path));
        return true;
    });

    bool download_called = false;
    setAzureDownloadFn([&download_called](const std::string& account_name,
                                          const std::string& container,
                                          const std::string& remote_path,
                                          const std::string& local_path) {
        download_called = true;
        EXPECT_EQ(account_name, "account");
        EXPECT_EQ(container, "container");
        EXPECT_EQ(remote_path, "backup-for-azure-callbacks/shard1");
        std::ofstream out(local_path);
        out << "azure-callback-download";
        return out.good();
    });

    bool delete_called = false;
    setAzureDeleteFn([&delete_called](const std::string& account_name,
                                      const std::string& container,
                                      const std::string& remote_path) {
        delete_called = true;
        EXPECT_EQ(account_name, "account");
        EXPECT_EQ(container, "container");
        EXPECT_EQ(remote_path, "backup-for-azure-callbacks/shard1");
        return true;
    });

    std::vector<std::string> shard_ids = {"shard1"};
    EXPECT_TRUE(coordinator_->createBackup("backup-for-azure-callbacks", shard_ids));
    EXPECT_TRUE(coordinator_->restoreBackup("backup-for-azure-callbacks", shard_ids));
    EXPECT_TRUE(coordinator_->deleteBackup("backup-for-azure-callbacks"));
    EXPECT_TRUE(upload_called);
    EXPECT_TRUE(download_called);
    EXPECT_TRUE(delete_called);
}
