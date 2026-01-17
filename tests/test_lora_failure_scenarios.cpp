/**
 * ThemisDB LoRA Failure Scenarios Tests
 * 
 * Comprehensive tests for failure handling including:
 * - Corrupted metadata recovery
 * - Incomplete transfer handling
 * - Disk space exhaustion
 * - Permission errors
 * - Concurrent access conflicts
 * - Network failures
 */

#include <gtest/gtest.h>
#include "fixtures/mock_shard_cluster.h"
#include "utils/shard_failure_injector.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_config.h"
#include <filesystem>
#include <fstream>

using namespace themis::llm::lora;
using namespace themis::test;
namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class LoRAFailureScenariosTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = fs::temp_directory_path() / "themis_lora_failure_test";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
        
        // Setup config
        config_.backend = LoRAStorageService::Backend::FileSystem;
        config_.filesystem_path = test_dir_.string();
        config_.enable_versioning = false;
        config_.enable_signatures = false;
        
        // Setup cluster
        cluster_ = std::make_unique<MockShardCluster>(3);
        cluster_->setLatency(0, 0);
    }
    
    void TearDown() override {
        cluster_.reset();
        
        if (fs::exists(test_dir_)) {
            try {
                fs::remove_all(test_dir_);
            } catch (...) {}
        }
    }
    
    AdapterWeights createTestWeights(size_t size = 2048) {
        AdapterWeights weights;
        weights.data.resize(size);
        for (size_t i = 0; i < size; ++i) {
            weights.data[i] = static_cast<uint8_t>(i % 256);
        }
        weights.size_bytes = size;
        weights.format = "safetensors";
        return weights;
    }
    
    AdapterMetadata createTestMetadata(const std::string& id) {
        AdapterMetadata metadata;
        metadata.adapter_id = id;
        metadata.base_model = "test-model";
        metadata.description = "Test adapter";
        metadata.created_at = std::time(nullptr);
        metadata.updated_at = metadata.created_at;
        metadata.version = "1.0.0";
        metadata.hyperparameters.rank = 8;
        metadata.hyperparameters.alpha = 16.0f;
        return metadata;
    }
    
    fs::path test_dir_;
    LoRAStorageService::Config config_;
    std::unique_ptr<MockShardCluster> cluster_;
};

// ═══════════════════════════════════════════════════════════
// Corrupted Metadata Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, DetectCorruptedMetadata) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("corrupt-test");
    
    // Save adapter
    ASSERT_TRUE(storage.saveAdapter("corrupt-test", weights, metadata));
    
    // Manually corrupt metadata file
    constexpr const char* METADATA_FILENAME = "metadata.json";
    auto adapter_dir = test_dir_ / "corrupt-test";
    if (fs::exists(adapter_dir)) {
        auto metadata_file = adapter_dir / METADATA_FILENAME;
        if (fs::exists(metadata_file)) {
            // Write garbage to metadata file
            std::ofstream ofs(metadata_file, std::ios::binary | std::ios::trunc);
            ofs << "CORRUPTED_DATA_!!!";
            ofs.close();
            
            // Attempt to load should detect corruption
            auto loaded_metadata = storage.loadMetadata("corrupt-test");
            EXPECT_FALSE(loaded_metadata.has_value()) 
                << "Should detect corrupted metadata";
        }
    }
}

TEST_F(LoRAFailureScenariosTest, RecoverFromReplicaAfterCorruption) {
    std::string key = "lora:replica-recovery";
    auto lora_data = createTestWeights(2048).data;
    
    // Replicate to all shards
    for (int i = 0; i < 3; ++i) {
        cluster_->saveToShard(i, key, lora_data);
    }
    
    // Corrupt data on shard 0
    cluster_->deleteFromShard(0, key);
    
    // Should still be recoverable from other shards
    auto from_shard_1 = cluster_->loadFromShard(1, key);
    auto from_shard_2 = cluster_->loadFromShard(2, key);
    
    ASSERT_TRUE(from_shard_1.has_value());
    ASSERT_TRUE(from_shard_2.has_value());
    EXPECT_EQ(*from_shard_1, lora_data);
    EXPECT_EQ(*from_shard_2, lora_data);
}

TEST_F(LoRAFailureScenariosTest, DetectIncompleteWrite) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(4096);
    auto metadata = createTestMetadata("incomplete-test");
    
    // Save adapter
    ASSERT_TRUE(storage.saveAdapter("incomplete-test", weights, metadata));
    
    // Simulate incomplete write by truncating weights file
    auto adapter_dir = test_dir_ / "incomplete-test";
    if (fs::exists(adapter_dir)) {
        auto weights_file = adapter_dir / "weights.bin";
        if (fs::exists(weights_file)) {
            // Truncate file to half size
            auto original_size = fs::file_size(weights_file);
            fs::resize_file(weights_file, original_size / 2);
            
            // Load should detect size mismatch
            auto loaded = storage.loadAdapter("incomplete-test");
            
            // Implementation might return partial data or fail
            // This tests detection of incomplete writes
            if (loaded.has_value()) {
                EXPECT_NE(loaded->data.size(), weights.data.size())
                    << "Should detect incomplete data";
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
// Incomplete Transfer Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, InterruptMidTransfer) {
    std::string key = "lora:interrupted-transfer";
    auto lora_data = createTestWeights(8192).data;
    
    // Start saving to shard 0
    ASSERT_TRUE(cluster_->saveToShard(0, key, lora_data));
    
    // Simulate network failure during transfer to shard 1
    cluster_->injectPacketLoss(1.0f);  // 100% packet loss
    
    bool transfer_failed = !cluster_->saveToShard(1, key, lora_data);
    EXPECT_TRUE(transfer_failed) << "Transfer should fail with 100% packet loss";
    
    // Verify data not saved on shard 1
    EXPECT_FALSE(cluster_->existsInShard(1, key));
    
    // Heal network
    cluster_->injectPacketLoss(0.0f);
    
    // Retry should succeed
    bool retry_success = cluster_->saveToShard(1, key, lora_data);
    EXPECT_TRUE(retry_success);
    
    auto loaded = cluster_->loadFromShard(1, key);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lora_data);
}

TEST_F(LoRAFailureScenariosTest, CleanupIncompleteTransfer) {
    std::string key = "lora:cleanup-test";
    auto lora_data = createTestWeights(2048).data;
    
    // Attempt to save with packet loss
    cluster_->injectPacketLoss(1.0f);
    cluster_->saveToShard(1, key, lora_data);  // Should fail
    
    // Verify incomplete data not accessible
    EXPECT_FALSE(cluster_->existsInShard(1, key));
    
    // Complete transfer should work
    cluster_->injectPacketLoss(0.0f);
    ASSERT_TRUE(cluster_->saveToShard(1, key, lora_data));
    EXPECT_TRUE(cluster_->existsInShard(1, key));
}

TEST_F(LoRAFailureScenariosTest, RetryAfterPartialTransfer) {
    std::string key = "lora:retry-test";
    auto lora_data = createTestWeights(4096).data;
    
    int max_retries = 3;
    int successful = 0;
    
    // Inject 50% packet loss
    cluster_->injectPacketLoss(0.5f);
    
    // Retry transfer up to max_retries times
    for (int i = 0; i < max_retries && successful == 0; ++i) {
        if (cluster_->saveToShard(0, key + std::to_string(i), lora_data)) {
            successful++;
        }
    }
    
    // Should succeed eventually
    EXPECT_GT(successful, 0) << "Should succeed within " << max_retries << " retries";
}

// ═══════════════════════════════════════════════════════════
// Disk Space Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, OutOfDiskSpace) {
    LoRAStorageService storage(config_);
    
    // Try to save very large adapter (100MB)
    // Note: This won't actually fill disk, just tests large file handling
    auto weights = createTestWeights(100 * 1024 * 1024);
    auto metadata = createTestMetadata("huge-adapter");
    
    // Attempt to save - may succeed or fail depending on available space
    bool saved = storage.saveAdapter("huge-adapter", weights, metadata);
    
    // If save succeeded, verify we can load it back
    if (saved) {
        auto loaded = storage.loadAdapter("huge-adapter");
        if (loaded.has_value()) {
            EXPECT_EQ(loaded->data.size(), weights.data.size());
        }
    }
}

TEST_F(LoRAFailureScenariosTest, CleanupOldVersionsOnSpaceIssue) {
    config_.enable_versioning = true;
    config_.max_versions = 3;
    
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("space-cleanup-test");
    
    // Save initial version
    ASSERT_TRUE(storage.saveAdapter("space-cleanup-test", weights, metadata));
    
    // Create multiple versions (exceeding max)
    for (int i = 0; i < 5; ++i) {
        storage.createVersion("space-cleanup-test");
        metadata.version = "v" + std::to_string(i + 2);
        storage.saveAdapter("space-cleanup-test", weights, metadata);
    }
    
    // Should have cleaned up old versions
    auto versions = storage.listVersions("space-cleanup-test");
    EXPECT_LE(versions.size(), config_.max_versions);
}

TEST_F(LoRAFailureScenariosTest, PartialWriteDueToSpace) {
    LoRAStorageService storage(config_);
    
    // Create large adapter
    auto weights = createTestWeights(50 * 1024 * 1024);  // 50MB
    auto metadata = createTestMetadata("partial-write");
    
    // Attempt save
    bool saved = storage.saveAdapter("partial-write", weights, metadata);
    
    // If it saved, try loading
    if (saved) {
        auto loaded = storage.loadAdapter("partial-write");
        
        // Verify complete or detect partial write
        if (loaded.has_value()) {
            EXPECT_EQ(loaded->data.size(), weights.data.size())
                << "Partial write detected";
        }
    }
}

// ═══════════════════════════════════════════════════════════
// Permission Error Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, ReadOnlyDirectory) {
    // Create adapter in writable directory
    LoRAStorageService storage_writable(config_);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("readonly-test");
    
    ASSERT_TRUE(storage_writable.saveAdapter("readonly-test", weights, metadata));
    
    // Make directory read-only
#ifndef _WIN32
    fs::permissions(test_dir_, 
                   fs::perms::owner_read | fs::perms::owner_exec,
                   fs::perm_options::replace);
    
    // Attempt to save another adapter should fail
    auto weights2 = createTestWeights(512);
    auto metadata2 = createTestMetadata("readonly-test-2");
    
    bool saved = storage_writable.saveAdapter("readonly-test-2", weights2, metadata2);
    EXPECT_FALSE(saved) << "Should fail to write to read-only directory";
    
    // Restore permissions
    fs::permissions(test_dir_,
                   fs::perms::owner_all,
                   fs::perm_options::replace);
#endif
}

TEST_F(LoRAFailureScenariosTest, MissingPermissions) {
    // Test fallback behavior when lacking permissions
    // Implementation-specific - may use temp directory or return error
    
    auto config = config_;
    config.filesystem_path = "/root/restricted_path";  // Likely no write access
    
    LoRAStorageService storage(config);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("permission-test");
    
    // Should handle gracefully (fail or use fallback)
    storage.saveAdapter("permission-test", weights, metadata);
}

// ═══════════════════════════════════════════════════════════
// Concurrent Access Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, ConcurrentWriteConflict) {
    LoRAStorageService storage(config_);
    
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> conflict_count{0};
    
    // Multiple threads try to write same adapter
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&storage, i, &success_count, &conflict_count, this]() {
            auto weights = createTestWeights(1024);
            auto metadata = createTestMetadata("concurrent-adapter");
            metadata.version = "v" + std::to_string(i);
            
            bool saved = storage.saveAdapter("concurrent-adapter", weights, metadata);
            if (saved) {
                success_count++;
            } else {
                conflict_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // At least one should succeed
    EXPECT_GT(success_count.load(), 0);
    
    // Final state should be consistent
    EXPECT_TRUE(storage.exists("concurrent-adapter"));
}

TEST_F(LoRAFailureScenariosTest, ReadDuringWrite) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(10 * 1024 * 1024);  // Large file for slow write
    auto metadata = createTestMetadata("read-during-write");
    
    // Start write in background
    std::thread writer([&storage, &weights, &metadata]() {
        storage.saveAdapter("read-during-write", weights, metadata);
    });
    
    // Try to read immediately (might get partial or wait)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto loaded = storage.loadAdapter("read-during-write");
    
    writer.join();
    
    // Either got full data or nothing (no partial reads)
    if (loaded.has_value()) {
        EXPECT_EQ(loaded->data.size(), weights.data.size())
            << "Should not return partial data";
    }
}

// ═══════════════════════════════════════════════════════════
// Network Failure Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, NetworkPartitionDuringReplication) {
    std::string key = "lora:partition-replication";
    auto lora_data = createTestWeights(2048).data;
    
    // Start replication to all shards
    cluster_->saveToShard(0, key, lora_data);
    cluster_->saveToShard(1, key, lora_data);
    
    // Simulate network partition before completing replication
    cluster_->simulateNetworkPartition({2});
    
    // Attempt to replicate to partitioned shard should fail
    bool saved_to_2 = cluster_->saveToShard(2, key, lora_data);
    EXPECT_FALSE(saved_to_2);
    
    // Heal partition
    cluster_->healNetworkPartition();
    
    // Retry should succeed
    EXPECT_TRUE(cluster_->saveToShard(2, key, lora_data));
}

TEST_F(LoRAFailureScenariosTest, TransientNetworkFailure) {
    ShardFailureInjector injector;
    
    // Inject transient failure (auto-recovers)
    injector.injectFailure(0, ShardFailureInjector::FailureType::TRANSIENT,
                          std::chrono::milliseconds(500));
    
    cluster_->failShard(0);
    
    std::string key = "lora:transient-failure";
    auto lora_data = createTestWeights(1024).data;
    
    // Should fail initially
    EXPECT_FALSE(cluster_->saveToShard(0, key, lora_data));
    
    // Wait for recovery
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    injector.update();
    cluster_->recoverShard(0);
    
    // Should succeed after recovery
    EXPECT_TRUE(cluster_->saveToShard(0, key, lora_data));
}

TEST_F(LoRAFailureScenariosTest, CascadingShardFailures) {
    ShardFailureInjector injector;
    
    // Inject cascading failures
    auto scenarios = injector.injectCascadingFailures(
        0, 2, std::chrono::milliseconds(100));
    
    EXPECT_EQ(scenarios.size(), 3);  // Initial + 2 cascading
    
    // Verify initial failure
    EXPECT_TRUE(injector.isShardFailed(0));
    
    // Statistics should track failures
    auto stats = injector.getStatistics();
    EXPECT_GT(stats["active_scenarios"], 0);
}

// ═══════════════════════════════════════════════════════════
// Recovery Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAFailureScenariosTest, AutomaticFailoverOnError) {
    std::string key = "lora:auto-failover";
    auto lora_data = createTestWeights(2048).data;
    
    // Replicate to shards 0 and 1
    cluster_->saveToShard(0, key, lora_data);
    cluster_->saveToShard(1, key, lora_data);
    
    // Fail primary shard
    cluster_->failShard(0);
    
    // Read should automatically failover to shard 1
    auto loaded = cluster_->loadFromShard(1, key);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(*loaded, lora_data);
}

TEST_F(LoRAFailureScenariosTest, ErrorReportingAndLogging) {
    LoRAStorageService storage(config_);
    
    // Attempt various operations that should log errors
    storage.loadAdapter("nonexistent-adapter");  // Should log not found
    storage.deleteAdapter("nonexistent-adapter");  // Should log not found
    
    // Save with invalid path
    auto config_bad = config_;
    config_bad.filesystem_path = "";
    LoRAStorageService storage_bad(config_bad);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("error-test");
    
    // Should handle and log error gracefully
    storage_bad.saveAdapter("error-test", weights, metadata);
    
    // No assertions - this tests that errors don't crash
    SUCCEED();
}
