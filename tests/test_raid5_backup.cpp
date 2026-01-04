#include <gtest/gtest.h>
#include "storage/backup_manager.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

static void cleanupPath(const std::string& p) {
    std::error_code ec; 
    fs::remove_all(p, ec);
}

// Helper to set environment variable
static void setEnv(const char* name, const char* value) {
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

// Helper to unset environment variable
static void unsetEnv(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

class RAID5BackupTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up any previous test data
        cleanupPath("./data/raid5_backup_test");
        cleanupPath("./data/raid5_backup_dest");
        
        // Save original environment
        saved_raid_group_ = std::getenv("THEMIS_RAID_GROUP");
        saved_shard_id_ = std::getenv("THEMIS_SHARD_ID");
        saved_shards_ = std::getenv("THEMIS_SHARDS");
    }
    
    void TearDown() override {
        // Restore original environment
        if (saved_raid_group_) {
            setEnv("THEMIS_RAID_GROUP", saved_raid_group_);
        } else {
            unsetEnv("THEMIS_RAID_GROUP");
        }
        
        if (saved_shard_id_) {
            setEnv("THEMIS_SHARD_ID", saved_shard_id_);
        } else {
            unsetEnv("THEMIS_SHARD_ID");
        }
        
        if (saved_shards_) {
            setEnv("THEMIS_SHARDS", saved_shards_);
        } else {
            unsetEnv("THEMIS_SHARDS");
        }
        
        // Cleanup test data
        cleanupPath("./data/raid5_backup_test");
        cleanupPath("./data/raid5_backup_dest");
    }
    
    const char* saved_raid_group_ = nullptr;
    const char* saved_shard_id_ = nullptr;
    const char* saved_shards_ = nullptr;
};

TEST_F(RAID5BackupTest, DetectRAID5Configuration) {
    // Set up RAID5 environment
    setEnv("THEMIS_RAID_GROUP", "raid5");
    setEnv("THEMIS_SHARD_ID", "raid5-1");
    setEnv("THEMIS_SHARDS", "shard1:18765,shard2:18765,shard3:18765");
    
    // Detect configuration
    auto config = themis::BackupManager::detectRAIDConfiguration();
    
    // Verify detection
    EXPECT_EQ(config.mode, themis::RAIDMode::RAID5);
    EXPECT_EQ(config.raid_group, "raid5");
    EXPECT_EQ(config.shards.size(), 3);
    EXPECT_EQ(config.data_shards, 2);  // N-1 for RAID5
    EXPECT_EQ(config.parity_shards, 1);
    EXPECT_TRUE(config.is_coordinated);
}

TEST_F(RAID5BackupTest, DetectNoRAIDConfiguration) {
    // Clear RAID environment
    unsetEnv("THEMIS_RAID_GROUP");
    unsetEnv("THEMIS_SHARD_ID");
    unsetEnv("THEMIS_SHARDS");
    
    // Detect configuration
    auto config = themis::BackupManager::detectRAIDConfiguration();
    
    // Should detect no RAID
    EXPECT_EQ(config.mode, themis::RAIDMode::NONE);
    EXPECT_EQ(config.shards.size(), 0);
    EXPECT_FALSE(config.is_coordinated);
}

TEST_F(RAID5BackupTest, ManifestIncludesRAIDInfo) {
    // Set up RAID5 environment
    setEnv("THEMIS_RAID_GROUP", "raid5");
    setEnv("THEMIS_SHARD_ID", "raid5-1");
    setEnv("THEMIS_SHARDS", "shard1:18765,shard2:18765,shard3:18765");
    
    const std::string db_path = "./data/raid5_backup_test/db";
    const std::string backup_path = "./data/raid5_backup_dest";
    
    // Create DB
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());
    
    // Write some test data
    std::vector<uint8_t> value{'t','e','s','t'};
    ASSERT_TRUE(db->put("test:key1", value));
    
    // Create backup manager
    themis::BackupManager backup_mgr(db);
    
    // Create full backup
    std::error_code ec;
    ASSERT_TRUE(backup_mgr.createFullBackup(backup_path, ec));
    
    // Find the backup directory
    auto backups = backup_mgr.listBackups(backup_path);
    ASSERT_FALSE(backups.empty());
    
    // Read and verify manifest
    auto manifest_path = fs::path(backup_path) / backups[0] / "MANIFEST.json";
    ASSERT_TRUE(fs::exists(manifest_path));
    
    std::ifstream manifest_file(manifest_path);
    ASSERT_TRUE(manifest_file.is_open());
    
    nlohmann::json manifest;
    manifest_file >> manifest;
    
    // Verify RAID information is present
    ASSERT_TRUE(manifest.contains("raid"));
    EXPECT_EQ(manifest["raid"]["mode"], "RAID5");
    EXPECT_EQ(manifest["raid"]["raid_group"], "raid5");
    EXPECT_EQ(manifest["raid"]["total_shards"], 3);
    EXPECT_EQ(manifest["raid"]["data_shards"], 2);
    EXPECT_EQ(manifest["raid"]["parity_shards"], 1);
    EXPECT_TRUE(manifest["raid"]["is_coordinated"]);
    
    // Verify shards array
    ASSERT_TRUE(manifest["raid"].contains("shards"));
    auto shards_array = manifest["raid"]["shards"];
    EXPECT_EQ(shards_array.size(), 3);
    
    // Verify backup note warning
    ASSERT_TRUE(manifest["raid"].contains("backup_note"));
    std::string note = manifest["raid"]["backup_note"];
    EXPECT_TRUE(note.find("ALL shards") != std::string::npos);
    EXPECT_TRUE(note.find("data + parity") != std::string::npos);
}

TEST_F(RAID5BackupTest, VerificationChecksRAIDCompleteness) {
    // Set up RAID5 environment
    setEnv("THEMIS_RAID_GROUP", "raid5");
    setEnv("THEMIS_SHARD_ID", "raid5-1");
    setEnv("THEMIS_SHARDS", "shard1:18765,shard2:18765,shard3:18765");
    
    const std::string db_path = "./data/raid5_backup_test/db";
    const std::string backup_path = "./data/raid5_backup_dest";
    
    // Create DB and backup
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());
    
    std::vector<uint8_t> value{'t','e','s','t'};
    ASSERT_TRUE(db->put("test:key1", value));
    
    themis::BackupManager backup_mgr(db);
    
    std::error_code ec;
    ASSERT_TRUE(backup_mgr.createFullBackup(backup_path, ec));
    
    auto backups = backup_mgr.listBackups(backup_path);
    ASSERT_FALSE(backups.empty());
    
    // Verify backup passes RAID completeness check
    auto backup_dir = fs::path(backup_path) / backups[0];
    EXPECT_TRUE(backup_mgr.verifyBackup(backup_dir.string(), ec));
}

TEST_F(RAID5BackupTest, RAID6DetectionWith4Shards) {
    // Set up RAID6 environment (needs at least 4 shards)
    setEnv("THEMIS_RAID_GROUP", "raid6");
    setEnv("THEMIS_SHARD_ID", "raid6-1");
    setEnv("THEMIS_SHARDS", "shard1:18765,shard2:18765,shard3:18765,shard4:18765");
    
    // Detect configuration
    auto config = themis::BackupManager::detectRAIDConfiguration();
    
    // Verify RAID6 detection
    EXPECT_EQ(config.mode, themis::RAIDMode::RAID6);
    EXPECT_EQ(config.raid_group, "raid6");
    EXPECT_EQ(config.shards.size(), 4);
    EXPECT_EQ(config.data_shards, 2);  // N-2 for RAID6
    EXPECT_EQ(config.parity_shards, 2);
    EXPECT_TRUE(config.is_coordinated);
}

TEST_F(RAID5BackupTest, NoRAIDInManifestForStandardBackup) {
    // Clear RAID environment for standard backup
    unsetEnv("THEMIS_RAID_GROUP");
    unsetEnv("THEMIS_SHARD_ID");
    unsetEnv("THEMIS_SHARDS");
    
    const std::string db_path = "./data/raid5_backup_test/db_noraid";
    const std::string backup_path = "./data/raid5_backup_dest_noraid";
    
    cleanupPath(db_path);
    cleanupPath(backup_path);
    
    // Create DB
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());
    
    std::vector<uint8_t> value{'t','e','s','t'};
    ASSERT_TRUE(db->put("test:key1", value));
    
    // Create backup without RAID
    themis::BackupManager backup_mgr(db);
    
    std::error_code ec;
    ASSERT_TRUE(backup_mgr.createFullBackup(backup_path, ec));
    
    auto backups = backup_mgr.listBackups(backup_path);
    ASSERT_FALSE(backups.empty());
    
    // Read manifest
    auto manifest_path = fs::path(backup_path) / backups[0] / "MANIFEST.json";
    std::ifstream manifest_file(manifest_path);
    nlohmann::json manifest;
    manifest_file >> manifest;
    
    // Verify RAID mode is NONE
    ASSERT_TRUE(manifest.contains("raid"));
    EXPECT_EQ(manifest["raid"]["mode"], "NONE");
    EXPECT_FALSE(manifest["raid"]["is_coordinated"]);
    
    cleanupPath(db_path);
    cleanupPath(backup_path);
}

// Test that backup manager warns about RAID5 completeness requirements
TEST_F(RAID5BackupTest, LogsWarningForRAID5Requirements) {
    // Set up RAID5 environment
    setEnv("THEMIS_RAID_GROUP", "raid5");
    setEnv("THEMIS_SHARD_ID", "raid5-1");
    setEnv("THEMIS_SHARDS", "shard1:18765,shard2:18765,shard3:18765");
    
    const std::string db_path = "./data/raid5_backup_test/db_warn";
    
    cleanupPath(db_path);
    
    // Create DB
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path;
    auto db = std::make_shared<themis::RocksDBWrapper>(cfg);
    ASSERT_TRUE(db->open());
    
    // Creating backup manager should detect and log RAID configuration
    themis::BackupManager backup_mgr(db);
    
    // The constructor should have logged information about RAID5 detection
    // We can't directly test logging output in this unit test,
    // but we can verify the configuration is correct
    auto config = themis::BackupManager::detectRAIDConfiguration();
    EXPECT_EQ(config.mode, themis::RAIDMode::RAID5);
    EXPECT_EQ(config.shards.size(), 3);
    
    cleanupPath(db_path);
}
