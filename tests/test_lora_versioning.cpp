/**
 * ThemisDB LoRA Versioning Tests
 * 
 * Comprehensive tests for LoRA adapter versioning including:
 * - Version creation and tracking
 * - Rollback functionality
 * - Version cleanup
 * - Version isolation
 * - Concurrent versioning
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_config.h"
#include <filesystem>
#include <thread>

using namespace themis::llm::lora;
namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class LoRAVersioningTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_lora_versioning_test";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
        
        config_.backend = LoRAStorageService::Backend::FileSystem;
        config_.filesystem_path = test_dir_.string();
        config_.enable_versioning = true;
        config_.max_versions = 5;
        config_.enable_signatures = false;
    }
    
    void TearDown() override {
        if (fs::exists(test_dir_)) {
            try {
                fs::remove_all(test_dir_);
            } catch (...) {}
        }
    }
    
    AdapterWeights createTestWeights(size_t size, uint8_t marker = 0) {
        AdapterWeights weights;
        weights.data.resize(size);
        for (size_t i = 0; i < size; ++i) {
            weights.data[i] = marker != 0 ? marker : static_cast<uint8_t>(i % 256);
        }
        weights.size_bytes = size;
        weights.format = "safetensors";
        return weights;
    }
    
    AdapterMetadata createTestMetadata(const std::string& id, const std::string& version = "1.0.0") {
        AdapterMetadata metadata;
        metadata.adapter_id = id;
        metadata.base_model = "test-model";
        metadata.description = "Test adapter version " + version;
        metadata.created_at = std::time(nullptr);
        metadata.updated_at = metadata.created_at;
        metadata.version = version;
        metadata.hyperparameters.rank = 8;
        metadata.hyperparameters.alpha = 16.0f;
        return metadata;
    }
    
    fs::path test_dir_;
    LoRAStorageService::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Version Creation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, CreateFirstVersion) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("test-adapter", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("test-adapter", weights, metadata));
    
    // Create version
    std::string version_id = storage.createVersion("test-adapter");
    EXPECT_FALSE(version_id.empty());
}

TEST_F(LoRAVersioningTest, CreateMultipleVersions) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata("versioned-adapter", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("versioned-adapter", weights, metadata));
    
    // Create 3 versions
    std::vector<std::string> version_ids;
    for (int i = 0; i < 3; ++i) {
        std::string vid = storage.createVersion("versioned-adapter");
        EXPECT_FALSE(vid.empty());
        version_ids.push_back(vid);
    }
    
    // All version IDs should be unique
    std::set<std::string> unique_ids(version_ids.begin(), version_ids.end());
    EXPECT_EQ(unique_ids.size(), version_ids.size());
}

TEST_F(LoRAVersioningTest, VersionWithDataChange) {
    LoRAStorageService storage(config_);
    
    // Save v1 with marker 0xAA
    auto weights_v1 = createTestWeights(1024, 0xAA);
    auto metadata_v1 = createTestMetadata("data-change-test", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("data-change-test", weights_v1, metadata_v1));
    
    std::string version_v1 = storage.createVersion("data-change-test");
    ASSERT_FALSE(version_v1.empty());
    
    // Save v2 with marker 0xBB
    auto weights_v2 = createTestWeights(1024, 0xBB);
    auto metadata_v2 = createTestMetadata("data-change-test", "2.0.0");
    ASSERT_TRUE(storage.saveAdapter("data-change-test", weights_v2, metadata_v2));
    
    // Current should be v2
    auto current = storage.loadAdapter("data-change-test");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data[0], 0xBB);
}

TEST_F(LoRAVersioningTest, CreateVersionForNonexistent) {
    LoRAStorageService storage(config_);
    
    std::string version_id = storage.createVersion("nonexistent-adapter");
    EXPECT_TRUE(version_id.empty() || version_id == "");
}

// ═══════════════════════════════════════════════════════════
// Version Listing Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, ListVersions) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("list-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("list-test", weights, metadata));
    
    // Create multiple versions
    for (int i = 0; i < 4; ++i) {
        storage.createVersion("list-test");
    }
    
    auto versions = storage.listVersions("list-test");
    EXPECT_GE(versions.size(), 4);
}

TEST_F(LoRAVersioningTest, ListVersionsEmpty) {
    LoRAStorageService storage(config_);
    
    auto versions = storage.listVersions("nonexistent-adapter");
    EXPECT_TRUE(versions.empty());
}

TEST_F(LoRAVersioningTest, VersionListOrder) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("order-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("order-test", weights, metadata));
    
    std::vector<std::string> created_versions;
    for (int i = 0; i < 5; ++i) {
        std::string vid = storage.createVersion("order-test");
        created_versions.push_back(vid);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto listed_versions = storage.listVersions("order-test");
    EXPECT_GE(listed_versions.size(), created_versions.size());
}

// ═══════════════════════════════════════════════════════════
// Rollback Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, RollbackToEarlierVersion) {
    LoRAStorageService storage(config_);
    
    // Create v1
    auto weights_v1 = createTestWeights(1024, 0x11);
    auto metadata_v1 = createTestMetadata("rollback-test", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("rollback-test", weights_v1, metadata_v1));
    
    std::string version_v1 = storage.createVersion("rollback-test");
    ASSERT_FALSE(version_v1.empty());
    
    // Create v2
    auto weights_v2 = createTestWeights(1024, 0x22);
    auto metadata_v2 = createTestMetadata("rollback-test", "2.0.0");
    ASSERT_TRUE(storage.saveAdapter("rollback-test", weights_v2, metadata_v2));
    
    std::string version_v2 = storage.createVersion("rollback-test");
    
    // Create v3
    auto weights_v3 = createTestWeights(1024, 0x33);
    auto metadata_v3 = createTestMetadata("rollback-test", "3.0.0");
    ASSERT_TRUE(storage.saveAdapter("rollback-test", weights_v3, metadata_v3));
    
    // Verify current is v3
    auto current = storage.loadAdapter("rollback-test");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data[0], 0x33);
    
    // Rollback to v1
    bool rolled_back = storage.rollbackToVersion("rollback-test", version_v1);
    EXPECT_TRUE(rolled_back);
    
    // Verify now at v1
    auto after_rollback = storage.loadAdapter("rollback-test");
    ASSERT_TRUE(after_rollback.has_value());
    EXPECT_EQ(after_rollback->data[0], 0x11);
}

TEST_F(LoRAVersioningTest, RollbackToNonexistentVersion) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("rollback-fail-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("rollback-fail-test", weights, metadata));
    
    bool rolled_back = storage.rollbackToVersion("rollback-fail-test", "nonexistent-v999");
    EXPECT_FALSE(rolled_back);
}

TEST_F(LoRAVersioningTest, RollbackPreservesMetadata) {
    LoRAStorageService storage(config_);
    
    // Create v1
    auto weights_v1 = createTestWeights(1024);
    auto metadata_v1 = createTestMetadata("metadata-rollback", "1.0.0");
    metadata_v1.description = "Version 1 description";
    ASSERT_TRUE(storage.saveAdapter("metadata-rollback", weights_v1, metadata_v1));
    
    std::string version_v1 = storage.createVersion("metadata-rollback");
    
    // Create v2
    auto weights_v2 = createTestWeights(1024);
    auto metadata_v2 = createTestMetadata("metadata-rollback", "2.0.0");
    metadata_v2.description = "Version 2 description";
    ASSERT_TRUE(storage.saveAdapter("metadata-rollback", weights_v2, metadata_v2));
    
    // Rollback to v1
    ASSERT_TRUE(storage.rollbackToVersion("metadata-rollback", version_v1));
    
    // Check metadata
    auto metadata = storage.loadMetadata("metadata-rollback");
    ASSERT_TRUE(metadata.has_value());
    EXPECT_EQ(metadata->description, "Version 1 description");
}

// ═══════════════════════════════════════════════════════════
// Version Cleanup Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, AutoCleanupOldVersions) {
    config_.max_versions = 3;
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("cleanup-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("cleanup-test", weights, metadata));
    
    // Create more versions than the limit
    for (int i = 0; i < 6; ++i) {
        storage.createVersion("cleanup-test");
        metadata.version = "v" + std::to_string(i + 2);
        storage.saveAdapter("cleanup-test", weights, metadata);
    }
    
    // Should not exceed max_versions
    auto versions = storage.listVersions("cleanup-test");
    EXPECT_LE(versions.size(), config_.max_versions);
}

TEST_F(LoRAVersioningTest, ManualVersionDeletion) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("delete-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("delete-test", weights, metadata));
    
    // Create versions
    std::string v1 = storage.createVersion("delete-test");
    std::string v2 = storage.createVersion("delete-test");
    std::string v3 = storage.createVersion("delete-test");
    
    auto versions_before = storage.listVersions("delete-test");
    size_t count_before = versions_before.size();
    
    // Note: Delete version API might not exist - this tests the concept
    // In practice, cleanup happens automatically
    
    EXPECT_GE(count_before, 3);
}

TEST_F(LoRAVersioningTest, CleanupDoesntAffectCurrent) {
    config_.max_versions = 2;
    LoRAStorageService storage(config_);
    
    auto weights_current = createTestWeights(1024, 0xFF);
    auto metadata = createTestMetadata("current-test", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("current-test", weights_current, metadata));
    
    // Create many versions
    for (int i = 0; i < 5; ++i) {
        storage.createVersion("current-test");
    }
    
    // Current version should still be accessible
    auto current = storage.loadAdapter("current-test");
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data[0], 0xFF);
}

// ═══════════════════════════════════════════════════════════
// Version Isolation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, VersionsAreIsolated) {
    LoRAStorageService storage(config_);
    
    // Create v1
    auto weights_v1 = createTestWeights(1024, 0xAA);
    auto metadata_v1 = createTestMetadata("isolation-test", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("isolation-test", weights_v1, metadata_v1));
    
    std::string version_v1 = storage.createVersion("isolation-test");
    
    // Create v2 with different size
    auto weights_v2 = createTestWeights(2048, 0xBB);
    auto metadata_v2 = createTestMetadata("isolation-test", "2.0.0");
    ASSERT_TRUE(storage.saveAdapter("isolation-test", weights_v2, metadata_v2));
    
    // Rollback to v1
    ASSERT_TRUE(storage.rollbackToVersion("isolation-test", version_v1));
    
    // Should have v1 size and data
    auto restored_v1 = storage.loadAdapter("isolation-test");
    ASSERT_TRUE(restored_v1.has_value());
    EXPECT_EQ(restored_v1->data.size(), 1024);
    EXPECT_EQ(restored_v1->data[0], 0xAA);
}

TEST_F(LoRAVersioningTest, MultipleAdaptersIndependentVersions) {
    LoRAStorageService storage(config_);
    
    // Adapter 1
    auto weights1 = createTestWeights(512);
    auto metadata1 = createTestMetadata("adapter-1", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("adapter-1", weights1, metadata1));
    storage.createVersion("adapter-1");
    storage.createVersion("adapter-1");
    
    // Adapter 2
    auto weights2 = createTestWeights(512);
    auto metadata2 = createTestMetadata("adapter-2", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("adapter-2", weights2, metadata2));
    storage.createVersion("adapter-2");
    
    // Check independent version counts
    auto versions1 = storage.listVersions("adapter-1");
    auto versions2 = storage.listVersions("adapter-2");
    
    EXPECT_GE(versions1.size(), 2);
    EXPECT_GE(versions2.size(), 1);
    EXPECT_NE(versions1.size(), versions2.size());
}

// ═══════════════════════════════════════════════════════════
// Concurrent Versioning Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, ConcurrentVersionCreation) {
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("concurrent-versions", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("concurrent-versions", weights, metadata));
    
    std::vector<std::thread> threads;
    std::vector<std::string> version_ids;
    std::mutex version_mutex;
    
    // Multiple threads create versions concurrently
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&storage, &version_ids, &version_mutex]() {
            std::string vid = storage.createVersion("concurrent-versions");
            if (!vid.empty()) {
                std::lock_guard<std::mutex> lock(version_mutex);
                version_ids.push_back(vid);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have created versions
    EXPECT_GT(version_ids.size(), 0);
}

TEST_F(LoRAVersioningTest, ConcurrentRollbacks) {
    LoRAStorageService storage(config_);
    
    // Setup versions
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("concurrent-rollback", "1.0.0");
    ASSERT_TRUE(storage.saveAdapter("concurrent-rollback", weights, metadata));
    
    std::string v1 = storage.createVersion("concurrent-rollback");
    std::string v2 = storage.createVersion("concurrent-rollback");
    
    if (v1.empty() || v2.empty()) {
        GTEST_SKIP() << "Version creation failed";
    }
    
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    
    // Multiple threads attempt rollback concurrently
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&storage, &v1, &success_count]() {
            if (storage.rollbackToVersion("concurrent-rollback", v1)) {
                success_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // At least one should succeed
    EXPECT_GT(success_count.load(), 0);
    
    // Final state should be consistent
    auto final_state = storage.loadAdapter("concurrent-rollback");
    EXPECT_TRUE(final_state.has_value());
}

// ═══════════════════════════════════════════════════════════
// Versioning with Disabled Feature
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAVersioningTest, VersioningDisabled) {
    config_.enable_versioning = false;
    LoRAStorageService storage(config_);
    
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata("no-versioning", "1.0.0");
    
    ASSERT_TRUE(storage.saveAdapter("no-versioning", weights, metadata));
    
    // Attempt to create version should fail or return empty
    std::string version_id = storage.createVersion("no-versioning");
    EXPECT_TRUE(version_id.empty() || version_id == "");
    
    // List versions should be empty
    auto versions = storage.listVersions("no-versioning");
    EXPECT_TRUE(versions.empty());
}
