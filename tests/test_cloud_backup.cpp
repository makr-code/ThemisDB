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
    void installDefaultCloudCallbacks() {
        setS3UploadFn([](const std::string&, const std::string&, const std::string&,
                         const std::map<std::string, std::string>&) {
            return true;
        });
        setS3DownloadFn([](const std::string&, const std::string&, const std::string& local_path) {
            std::ofstream out(local_path);
            out << "s3-default-download";
            return out.good();
        });
        setS3DeleteFn([](const std::string&, const std::string&) {
            return true;
        });
        setS3ListFn([](const std::string&, const std::string&) {
            return std::vector<std::string>{};
        });
        setS3ExistsFn([](const std::string&, const std::string&) {
            return true;
        });

        setAzureUploadFn([](const std::string&, const std::string&, const std::string&,
                            const std::string&, const std::map<std::string, std::string>&) {
            return true;
        });
        setAzureDownloadFn([](const std::string&, const std::string&, const std::string&,
                              const std::string& local_path) {
            std::ofstream out(local_path);
            out << "azure-default-download";
            return out.good();
        });
        setAzureDeleteFn([](const std::string&, const std::string&, const std::string&) {
            return true;
        });
        setAzureListFn([](const std::string&, const std::string&, const std::string&) {
            return std::vector<std::string>{};
        });
        setAzureExistsFn([](const std::string&, const std::string&, const std::string&) {
            return true;
        });

        setGCSUploadFn([](const std::string&, const std::string&, const std::string&,
                          const std::map<std::string, std::string>&) {
            return true;
        });
        setGCSDownloadFn([](const std::string&, const std::string&, const std::string& local_path) {
            std::ofstream out(local_path);
            out << "gcs-default-download";
            return out.good();
        });
        setGCSDeleteFn([](const std::string&, const std::string&) {
            return true;
        });
        setGCSListFn([](const std::string&, const std::string&) {
            return std::vector<std::string>{};
        });
        setGCSExistsFn([](const std::string&, const std::string&) {
            return true;
        });
    }

    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping CloudBackupTest on Windows due to intermittent SEH in cloud backup fixture setup.";
#endif
        setS3UploadFn({});
        setS3DownloadFn({});
        setS3DeleteFn({});
        setS3ListFn({});
        setS3ExistsFn({});
        setAzureUploadFn({});
        setAzureDownloadFn({});
        setAzureDeleteFn({});
        setAzureListFn({});
        setAzureExistsFn({});
        setGCSUploadFn({});
        setGCSDownloadFn({});
        setGCSDeleteFn({});
        setGCSListFn({});
        setGCSExistsFn({});

        installDefaultCloudCallbacks();

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
        setAzureUploadFn({});
        setAzureDownloadFn({});
        setAzureDeleteFn({});
        setAzureListFn({});
        setAzureExistsFn({});
        setGCSUploadFn({});
        setGCSDownloadFn({});
        setGCSDeleteFn({});
        setGCSListFn({});
        setGCSExistsFn({});

        coordinator_.reset();
        cloud_agent_.reset();
        topology_.reset();
        backup_manager_.reset();
        db_.reset();
        
        // Clean up temporary directories
        std::error_code ec;
        std::filesystem::remove_all(db_path_, ec);
        std::filesystem::remove_all(local_backup_dir_, ec);
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

TEST_F(CloudBackupTest, DeleteBackupFailsClosedForEmptyBackupId) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    EXPECT_FALSE(coordinator_->deleteBackup(""));
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

TEST_F(CloudBackupTest, SetReplicationTargetFailsClosedForEmptyDatacenterId) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> endpoints = {"endpoint1"};
    EXPECT_FALSE(coordinator_->setReplicationTarget("", endpoints));
}

TEST_F(CloudBackupTest, SetReplicationTargetFailsClosedForEmptyEndpoints) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> empty_endpoints;
    EXPECT_FALSE(coordinator_->setReplicationTarget("datacenter-1", empty_endpoints));
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

TEST_F(CloudBackupTest, EnableContinuousReplicationFailsClosedForEmptyDatacenterId) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    EXPECT_FALSE(coordinator_->enableContinuousReplication(""));
}

TEST_F(CloudBackupTest, DisableContinuousReplicationFailsClosedForEmptyDatacenterId) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    EXPECT_FALSE(coordinator_->disableContinuousReplication(""));
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

    setS3UploadFn({});
    setS3DownloadFn({});
    setS3DeleteFn({});
    setS3ListFn({});
    setS3ExistsFn({});
    
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

TEST_F(CloudBackupTest, LegacyMockEnvDoesNotBypassMissingCallbacks) {
    // Legacy environment-based success bypass must not be accepted anymore.
    setenv("THEMIS_CLOUD_BACKUP_MOCK", "1", 1);

    setS3UploadFn({});
    setS3DownloadFn({});
    setS3DeleteFn({});
    setS3ListFn({});
    setS3ExistsFn({});

    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-legacy-mock-env", shard_ids);
    EXPECT_FALSE(success);
}

TEST_F(CloudBackupTest, CreateBackupFailsClosedWithoutBackupManager) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, nullptr, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    bool success = coordinator_->createBackup("backup-no-backup-manager", shard_ids);
    EXPECT_FALSE(success);
}

TEST_F(CloudBackupTest, RestoreBackupFailsClosedWithoutBackupManager) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-restore-no-backup-manager", shard_ids));

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, nullptr, config
    );

    bool success = coordinator_->restoreBackup("backup-restore-no-backup-manager", shard_ids);
    EXPECT_FALSE(success);
}

TEST_F(CloudBackupTest, RestoreBackupFailsClosedForEmptyBackupId) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    std::vector<std::string> shard_ids = {"shard1"};
    EXPECT_FALSE(coordinator_->restoreBackup("", shard_ids));
}

TEST_F(CloudBackupTest, RestoreBackupFailsClosedForEmptyShardList) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    EXPECT_FALSE(coordinator_->restoreBackup("backup-without-shards", {}));
}

TEST_F(CloudBackupTest, RestoreBackupFailsClosedForEmptyShardIdEntry) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    EXPECT_FALSE(coordinator_->restoreBackup("backup-with-empty-shard-id", {""}));
}

TEST_F(CloudBackupTest, RestoreBackupFailsClosedForShardOutsideBackupCatalog) {
    CloudBackupConfig config;
    config.provider = "s3";
    config.s3_bucket = "test-bucket";
    config.s3_region = "us-east-1";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    ASSERT_TRUE(coordinator_->createBackup("backup-catalog-membership", {"shard1"}));
    EXPECT_FALSE(coordinator_->restoreBackup("backup-catalog-membership", {"shard2"}));
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
    setS3UploadFn([](const std::string&, const std::string&, const std::string&,
                     const std::map<std::string, std::string>&) {
        return true;
    });
    ASSERT_TRUE(coordinator_->createBackup("backup-for-s3-callback", shard_ids));

    bool called = false;
    setS3DownloadFn([&called](const std::string& bucket,
                              const std::string& remote_path,
                              const std::string& local_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "/backup-for-s3-callback/shard1");
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
        EXPECT_EQ(remote_path, "/backup-for-s3-upload-callback/shard1");
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
    setGCSUploadFn([](const std::string&, const std::string&, const std::string&,
                      const std::map<std::string, std::string>&) {
        return true;
    });
    ASSERT_TRUE(coordinator_->createBackup("backup-for-gcs-delete-callback", shard_ids));

    bool called = false;
    setGCSDeleteFn([&called](const std::string& bucket, const std::string& remote_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "/backup-for-gcs-delete-callback/shard1");
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
    setS3UploadFn([](const std::string&, const std::string&, const std::string&,
                     const std::map<std::string, std::string>&) {
        return true;
    });
    ASSERT_TRUE(coordinator_->createBackup("backup-for-s3-delete-callback", shard_ids));

    bool called = false;
    setS3DeleteFn([&called](const std::string& bucket, const std::string& remote_path) {
        called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "/backup-for-s3-delete-callback/shard1");
        return true;
    });

    bool deleted = coordinator_->deleteBackup("backup-for-s3-delete-callback");
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(called);
}

TEST_F(CloudBackupTest, CreateAndRestoreUseGCSCallbacksWithoutMockMode) {
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
                                    const std::map<std::string, std::string>& metadata) {
        upload_called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "/backup-for-gcs-upload-download-callback/shard1");
        EXPECT_TRUE(std::filesystem::exists(local_path));
        EXPECT_EQ(metadata.at("backup_id"), "backup-for-gcs-upload-download-callback");
        EXPECT_EQ(metadata.at("shard_id"), "shard1");
        return true;
    });

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-for-gcs-upload-download-callback", shard_ids));
    EXPECT_TRUE(upload_called);

    bool download_called = false;
    setGCSDownloadFn([&download_called](const std::string& bucket,
                                        const std::string& remote_path,
                                        const std::string& local_path) {
        download_called = true;
        EXPECT_EQ(bucket, "test-bucket");
        EXPECT_EQ(remote_path, "/backup-for-gcs-upload-download-callback/shard1");
        std::ofstream out(local_path);
        out << "callback-download";
        return out.good();
    });

    bool restored = coordinator_->restoreBackup("backup-for-gcs-upload-download-callback", shard_ids);
    EXPECT_TRUE(restored);
    EXPECT_TRUE(download_called);
}

TEST_F(CloudBackupTest, CreateAndRestoreAndDeleteUseAzureCallbacksWithoutMockMode) {
    unsetenv("THEMIS_CLOUD_BACKUP_MOCK");

    CloudBackupConfig config;
    config.provider = "azure";
    config.azure_account = "testaccount";
    config.azure_container = "testcontainer";
    config.local_backup_dir = local_backup_dir_.string();

    coordinator_ = std::make_unique<CloudBackupCoordinator>(
        cloud_agent_, backup_manager_, config
    );

    bool upload_called = false;
    setAzureUploadFn([&upload_called](const std::string& account,
                                      const std::string& container,
                                      const std::string& local_path,
                                      const std::string& remote_path,
                                      const std::map<std::string, std::string>& metadata) {
        upload_called = true;
        EXPECT_EQ(account, "testaccount");
        EXPECT_EQ(container, "testcontainer");
        EXPECT_TRUE(std::filesystem::exists(local_path));
        EXPECT_EQ(remote_path, "/backup-for-azure-callback/shard1");
        EXPECT_EQ(metadata.at("backup_id"), "backup-for-azure-callback");
        return true;
    });

    std::vector<std::string> shard_ids = {"shard1"};
    ASSERT_TRUE(coordinator_->createBackup("backup-for-azure-callback", shard_ids));
    EXPECT_TRUE(upload_called);

    bool download_called = false;
    setAzureDownloadFn([&download_called](const std::string& account,
                                          const std::string& container,
                                          const std::string& remote_path,
                                          const std::string& local_path) {
        download_called = true;
        EXPECT_EQ(account, "testaccount");
        EXPECT_EQ(container, "testcontainer");
        EXPECT_EQ(remote_path, "/backup-for-azure-callback/shard1");
        std::ofstream out(local_path);
        out << "azure-callback-download";
        return out.good();
    });

    bool restored = coordinator_->restoreBackup("backup-for-azure-callback", shard_ids);
    EXPECT_TRUE(restored);
    EXPECT_TRUE(download_called);

    bool delete_called = false;
    setAzureDeleteFn([&delete_called](const std::string& account,
                                      const std::string& container,
                                      const std::string& remote_path) {
        delete_called = true;
        EXPECT_EQ(account, "testaccount");
        EXPECT_EQ(container, "testcontainer");
        EXPECT_EQ(remote_path, "/backup-for-azure-callback/shard1");
        return true;
    });

    bool deleted = coordinator_->deleteBackup("backup-for-azure-callback");
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(delete_called);
}
