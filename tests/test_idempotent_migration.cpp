/**
 * Integration tests for Idempotent Data Migration
 * 
 * Tests that migrations are retry-safe and don't duplicate data
 */

#include <gtest/gtest.h>
#include "sharding/data_migrator.h"
#include "sharding/prometheus_metrics.h"
#include <filesystem>
#include <fstream>

using namespace themis::sharding;

class IdempotentMigrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp directory for idempotency state
        test_dir_ = "/tmp/themis_idempotency_test";
        std::filesystem::create_directories(test_dir_);
        
        // Configure data migrator with idempotency enabled
        config_.source_endpoint = "https://shard_1:8080";
        config_.target_endpoint = "https://shard_2:8080";
        config_.cert_path = "/tmp/test.crt";
        config_.key_path = "/tmp/test.key";
        config_.ca_cert_path = "/tmp/ca.crt";
        config_.batch_size = 100;
        config_.enable_idempotency = true;
        config_.idempotency_store_path = test_dir_;
        config_.verify_integrity = false; // Skip integrity check for unit tests
    }
    
    void TearDown() override {
        // Cleanup test directory
        std::filesystem::remove_all(test_dir_);
    }
    
    DataMigratorConfig config_;
    std::string test_dir_;
};

// ============================================================================
// Migration ID Generation Tests
// ============================================================================

TEST_F(IdempotentMigrationTest, DeterministicMigrationId) {
    auto migrator1 = std::make_shared<DataMigrator>(config_);
    auto migrator2 = std::make_shared<DataMigrator>(config_);
    
    // Generate migration IDs with same parameters
    std::string id1 = migrator1->generateMigrationId("shard_1", "shard_2", 0, 1000);
    std::string id2 = migrator2->generateMigrationId("shard_1", "shard_2", 0, 1000);
    
    // Should be identical (deterministic)
    EXPECT_EQ(id1, id2);
    EXPECT_FALSE(id1.empty());
    EXPECT_TRUE(id1.find("migration_") == 0);
}

TEST_F(IdempotentMigrationTest, DifferentParametersGiveDifferentIds) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    std::string id1 = migrator->generateMigrationId("shard_1", "shard_2", 0, 1000);
    std::string id2 = migrator->generateMigrationId("shard_1", "shard_2", 1000, 2000);
    std::string id3 = migrator->generateMigrationId("shard_1", "shard_3", 0, 1000);
    
    // Different parameters should give different IDs
    EXPECT_NE(id1, id2);
    EXPECT_NE(id1, id3);
    EXPECT_NE(id2, id3);
}

TEST_F(IdempotentMigrationTest, DeterministicBatchId) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    std::string migration_id = "migration_12345";
    std::string batch1 = migrator->generateBatchId(migration_id, 0);
    std::string batch2 = migrator->generateBatchId(migration_id, 1);
    std::string batch3 = migrator->generateBatchId(migration_id, 0); // Same as batch1
    
    EXPECT_EQ(batch1, batch3); // Same parameters = same ID
    EXPECT_NE(batch1, batch2); // Different index = different ID
    
    EXPECT_EQ(batch1, "migration_12345_batch_0");
    EXPECT_EQ(batch2, "migration_12345_batch_1");
}

// ============================================================================
// Idempotency Tracking Tests
// ============================================================================

TEST_F(IdempotentMigrationTest, MigrationCompletionTracking) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    std::string migration_id = "test_migration_123";
    
    // Initially not completed
    EXPECT_FALSE(migrator->isMigrationCompleted(migration_id));
    
    // Mark as completed
    migrator->markMigrationCompleted(migration_id);
    
    // Now should be completed
    EXPECT_TRUE(migrator->isMigrationCompleted(migration_id));
}

TEST_F(IdempotentMigrationTest, BatchCompletionTracking) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    std::string batch_id = "test_batch_456";
    
    // Initially not completed
    EXPECT_FALSE(migrator->isBatchCompleted(batch_id));
    
    // Mark as completed
    migrator->markBatchCompleted(batch_id);
    
    // Now should be completed
    EXPECT_TRUE(migrator->isBatchCompleted(batch_id));
}

// ============================================================================
// Persistence Tests
// ============================================================================

TEST_F(IdempotentMigrationTest, StatePersistedAcrossInstances) {
    std::string migration_id = "persistent_migration_789";
    std::string batch_id = "persistent_batch_101";
    
    {
        // First instance - mark as completed
        auto migrator1 = std::make_shared<DataMigrator>(config_);
        migrator1->markMigrationCompleted(migration_id);
        migrator1->markBatchCompleted(batch_id);
        // Destructor saves state
    }
    
    {
        // Second instance - should load state
        auto migrator2 = std::make_shared<DataMigrator>(config_);
        
        // Should remember completion from first instance
        EXPECT_TRUE(migrator2->isMigrationCompleted(migration_id));
        EXPECT_TRUE(migrator2->isBatchCompleted(batch_id));
    }
}

TEST_F(IdempotentMigrationTest, StateFilesCreated) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    migrator->markMigrationCompleted("test_migration");
    migrator->markBatchCompleted("test_batch");
    
    // Check that state files exist
    namespace fs = std::filesystem;
    EXPECT_TRUE(fs::exists(fs::path(test_dir_) / "completed_migrations.json"));
    EXPECT_TRUE(fs::exists(fs::path(test_dir_) / "completed_batches.json"));
}

TEST_F(IdempotentMigrationTest, StateFilesHaveCorrectFormat) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    migrator->markMigrationCompleted("migration_1");
    migrator->markMigrationCompleted("migration_2");
    migrator->markBatchCompleted("batch_1");
    
    // Read and parse migration file
    namespace fs = std::filesystem;
    fs::path migrations_file = fs::path(test_dir_) / "completed_migrations.json";
    
    std::ifstream ifs(migrations_file);
    nlohmann::json j;
    ifs >> j;
    
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 2);
    EXPECT_TRUE(std::find(j.begin(), j.end(), "migration_1") != j.end());
    EXPECT_TRUE(std::find(j.begin(), j.end(), "migration_2") != j.end());
}

// ============================================================================
// Retry Safety Tests (Mock)
// ============================================================================

TEST_F(IdempotentMigrationTest, RepeatedMigrationReturnsAlreadyCompleted) {
    // Note: This test would need a mock mTLS client to actually run migration
    // For now, we test the idempotency check logic
    
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    // Simulate a completed migration
    std::string migration_id = migrator->generateMigrationId(
        "shard_1", "shard_2", 0, 1000
    );
    migrator->markMigrationCompleted(migration_id);
    
    // Attempting migration again should immediately return success
    // without actually migrating (idempotency)
    MigrationResult result = migrator->migrate("shard_1", "shard_2", 0, 1000);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.was_already_completed);
    EXPECT_EQ(result.migration_id, migration_id);
    EXPECT_EQ(result.records_migrated, 0); // No actual migration happened
}

TEST_F(IdempotentMigrationTest, BatchSkippedIfAlreadyCompleted) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    // Pre-mark some batches as completed
    std::string migration_id = "test_migration";
    migrator->markBatchCompleted(migration_id + "_batch_0");
    migrator->markBatchCompleted(migration_id + "_batch_2");
    
    // Verify they are marked
    EXPECT_TRUE(migrator->isBatchCompleted(migration_id + "_batch_0"));
    EXPECT_FALSE(migrator->isBatchCompleted(migration_id + "_batch_1"));
    EXPECT_TRUE(migrator->isBatchCompleted(migration_id + "_batch_2"));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(IdempotentMigrationTest, ConcurrentBatchCompletion) {
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    const int num_threads = 10;
    const int batches_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([migrator, i, batches_per_thread]() {
            for (int j = 0; j < batches_per_thread; ++j) {
                std::string batch_id = "batch_" + std::to_string(i * batches_per_thread + j);
                migrator->markBatchCompleted(batch_id);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all batches were marked
    for (int i = 0; i < num_threads * batches_per_thread; ++i) {
        std::string batch_id = "batch_" + std::to_string(i);
        EXPECT_TRUE(migrator->isBatchCompleted(batch_id));
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(IdempotentMigrationTest, EmptyStateDirectoryCreated) {
    // Use a directory that doesn't exist
    config_.idempotency_store_path = "/tmp/themis_nonexistent_dir_test";
    
    // Cleanup if exists
    std::filesystem::remove_all(config_.idempotency_store_path);
    
    // Creating migrator should create directory
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    EXPECT_TRUE(std::filesystem::exists(config_.idempotency_store_path));
    
    // Cleanup
    std::filesystem::remove_all(config_.idempotency_store_path);
}

TEST_F(IdempotentMigrationTest, IdempotencyDisabled) {
    config_.enable_idempotency = false;
    auto migrator = std::make_shared<DataMigrator>(config_);
    
    // With idempotency disabled, these methods should still work
    // but state won't be persisted
    migrator->markMigrationCompleted("test");
    EXPECT_TRUE(migrator->isMigrationCompleted("test"));
}
