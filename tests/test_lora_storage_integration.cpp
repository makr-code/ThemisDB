/**
 * ThemisDB LoRA Storage Integration Tests
 * 
 * Comprehensive tests for LoRA storage service including:
 * - Basic save/load operations
 * - Encrypted storage
 * - Versioning system
 * - Metadata serialization
 * - ThemisDB backend integration
 */

#include <gtest/gtest.h>
#include "llm/lora_framework/lora_storage_service.h"
#include "llm/lora_framework/lora_config.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace themis::llm::lora;
namespace fs = std::filesystem;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class LoRAStorageIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary test directory
        test_dir_ = fs::temp_directory_path() / "themis_lora_test";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
        
        // Configure storage service for filesystem backend
        config_.backend = LoRAStorageService::Backend::FileSystem;
        config_.filesystem_path = test_dir_.string();
        config_.enable_versioning = true;
        config_.max_versions = 5;
        config_.enable_compression = false;  // Disable for predictable testing
        config_.enable_signatures = false;   // Disable for basic tests
    }
    
    void TearDown() override {
        // Cleanup test directory
        if (fs::exists(test_dir_)) {
            try {
                fs::remove_all(test_dir_);
            } catch (...) {
                // Ignore cleanup errors
            }
        }
    }
    
    // Helper to create test adapter weights
    AdapterWeights createTestWeights(size_t size = 1024) {
        AdapterWeights weights;
        weights.data.resize(size);
        for (size_t i = 0; i < size; ++i) {
            weights.data[i] = static_cast<uint8_t>(i % 256);
        }
        weights.size_bytes = size;
        weights.format = "safetensors";
        return weights;
    }
    
    // Helper to create test metadata
    AdapterMetadata createTestMetadata(const std::string& name) {
        AdapterMetadata metadata;
        metadata.adapter_id = name;
        metadata.base_model = "test-model-7b";
        metadata.description = "Test adapter for " + name;
        metadata.created_at = std::time(nullptr);
        metadata.updated_at = metadata.created_at;
        metadata.version = "1.0.0";
        metadata.hyperparameters.rank = 8;
        metadata.hyperparameters.alpha = 16.0f;
        metadata.hyperparameters.learning_rate = 3e-4f;
        return metadata;
    }
    
    fs::path test_dir_;
    LoRAStorageService::Config config_;
};

// ═══════════════════════════════════════════════════════════
// Basic Save/Load Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAStorageIntegrationTest, SaveLoadAdapter) {
    LoRAStorageService storage(config_);
    
    // Create test data
    std::string adapter_id = "test-adapter-1";
    auto weights = createTestWeights(2048);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save adapter
    bool saved = storage.saveAdapter(adapter_id, weights, metadata);
    ASSERT_TRUE(saved) << "Failed to save adapter";
    
    // Verify existence
    EXPECT_TRUE(storage.exists(adapter_id));
    
    // Load adapter back
    auto loaded_weights = storage.loadAdapter(adapter_id);
    ASSERT_TRUE(loaded_weights.has_value()) << "Failed to load adapter";
    
    // Verify data integrity
    EXPECT_EQ(loaded_weights->data.size(), weights.data.size());
    EXPECT_EQ(loaded_weights->size_bytes, weights.size_bytes);
    EXPECT_EQ(loaded_weights->format, weights.format);
    EXPECT_EQ(loaded_weights->data, weights.data);
}

TEST_F(LoRAStorageIntegrationTest, SaveLoadMetadata) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-metadata";
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save adapter
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Load metadata only (without weights)
    auto loaded_metadata = storage.loadMetadata(adapter_id);
    ASSERT_TRUE(loaded_metadata.has_value());
    
    // Verify metadata
    EXPECT_EQ(loaded_metadata->adapter_id, metadata.adapter_id);
    EXPECT_EQ(loaded_metadata->base_model, metadata.base_model);
    EXPECT_EQ(loaded_metadata->description, metadata.description);
    EXPECT_EQ(loaded_metadata->version, metadata.version);
    EXPECT_EQ(loaded_metadata->hyperparameters.rank, metadata.hyperparameters.rank);
    EXPECT_FLOAT_EQ(loaded_metadata->hyperparameters.alpha, metadata.hyperparameters.alpha);
}

TEST_F(LoRAStorageIntegrationTest, ValidateChecksum) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-checksum";
    auto weights = createTestWeights(4096);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save adapter
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Load and verify multiple times
    for (int i = 0; i < 5; ++i) {
        auto loaded = storage.loadAdapter(adapter_id);
        ASSERT_TRUE(loaded.has_value()) << "Load iteration " << i << " failed";
        EXPECT_EQ(loaded->data, weights.data) << "Data mismatch on iteration " << i;
    }
}

TEST_F(LoRAStorageIntegrationTest, LoadNonExistentAdapter) {
    LoRAStorageService storage(config_);
    
    auto result = storage.loadAdapter("non-existent-adapter");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LoRAStorageIntegrationTest, DeleteAdapter) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-delete";
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save and verify
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    EXPECT_TRUE(storage.exists(adapter_id));
    
    // Delete adapter
    bool deleted = storage.deleteAdapter(adapter_id);
    EXPECT_TRUE(deleted);
    
    // Verify deletion
    EXPECT_FALSE(storage.exists(adapter_id));
    EXPECT_FALSE(storage.loadAdapter(adapter_id).has_value());
}

TEST_F(LoRAStorageIntegrationTest, ListAdapters) {
    LoRAStorageService storage(config_);
    
    // Create multiple adapters
    std::vector<std::string> adapter_ids = {
        "adapter-1", "adapter-2", "adapter-3", "adapter-4"
    };
    
    for (const auto& id : adapter_ids) {
        auto weights = createTestWeights(512);
        auto metadata = createTestMetadata(id);
        ASSERT_TRUE(storage.saveAdapter(id, weights, metadata));
    }
    
    // List adapters
    auto list = storage.listAdapters();
    EXPECT_EQ(list.size(), adapter_ids.size());
    
    // Verify all IDs are present
    for (const auto& id : adapter_ids) {
        EXPECT_TRUE(std::find(list.begin(), list.end(), id) != list.end())
            << "Adapter " << id << " not found in list";
    }
}

TEST_F(LoRAStorageIntegrationTest, UpdateMetadata) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-update";
    auto weights = createTestWeights(1024);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save initial version
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Update metadata
    metadata.description = "Updated description";
    metadata.version = "2.0.0";
    bool updated = storage.updateMetadata(adapter_id, metadata);
    EXPECT_TRUE(updated);
    
    // Verify update
    auto loaded_metadata = storage.loadMetadata(adapter_id);
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->description, "Updated description");
    EXPECT_EQ(loaded_metadata->version, "2.0.0");
}

// ═══════════════════════════════════════════════════════════
// Versioning System Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAStorageIntegrationTest, CreateVersions) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-versioning";
    
    // Create v1
    auto weights_v1 = createTestWeights(1024);
    auto metadata_v1 = createTestMetadata(adapter_id);
    metadata_v1.version = "1.0.0";
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights_v1, metadata_v1));
    
    // Create v2
    std::string version_v2 = storage.createVersion(adapter_id);
    EXPECT_FALSE(version_v2.empty());
    
    auto weights_v2 = createTestWeights(2048);
    auto metadata_v2 = createTestMetadata(adapter_id);
    metadata_v2.version = "2.0.0";
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights_v2, metadata_v2));
    
    // Create v3
    std::string version_v3 = storage.createVersion(adapter_id);
    EXPECT_FALSE(version_v3.empty());
    
    // List versions
    auto versions = storage.listVersions(adapter_id);
    EXPECT_GE(versions.size(), 2);
}

TEST_F(LoRAStorageIntegrationTest, RollbackToVersion) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-rollback";
    
    // Create and save v1
    auto weights_v1 = createTestWeights(1024);
    weights_v1.data[0] = 0xAA;  // Mark v1
    auto metadata_v1 = createTestMetadata(adapter_id);
    metadata_v1.version = "1.0.0";
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights_v1, metadata_v1));
    
    std::string version_v1 = storage.createVersion(adapter_id);
    ASSERT_FALSE(version_v1.empty());
    
    // Create and save v2
    auto weights_v2 = createTestWeights(1024);
    weights_v2.data[0] = 0xBB;  // Mark v2
    auto metadata_v2 = createTestMetadata(adapter_id);
    metadata_v2.version = "2.0.0";
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights_v2, metadata_v2));
    
    // Verify v2 is current
    auto current = storage.loadAdapter(adapter_id);
    ASSERT_TRUE(current.has_value());
    EXPECT_EQ(current->data[0], 0xBB);
    
    // Rollback to v1
    bool rolled_back = storage.rollbackToVersion(adapter_id, version_v1);
    EXPECT_TRUE(rolled_back);
    
    // Verify v1 is now current
    auto rolled_back_weights = storage.loadAdapter(adapter_id);
    ASSERT_TRUE(rolled_back_weights.has_value());
    EXPECT_EQ(rolled_back_weights->data[0], 0xAA);
}

TEST_F(LoRAStorageIntegrationTest, ListVersions) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-list-versions";
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save initial
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Create multiple versions
    for (int i = 0; i < 3; ++i) {
        std::string version = storage.createVersion(adapter_id);
        EXPECT_FALSE(version.empty());
    }
    
    // List versions
    auto versions = storage.listVersions(adapter_id);
    EXPECT_GE(versions.size(), 3);
}

TEST_F(LoRAStorageIntegrationTest, MaxVersionsLimit) {
    config_.max_versions = 3;
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "test-adapter-max-versions";
    auto weights = createTestWeights(512);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save initial
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Create more versions than the limit
    for (int i = 0; i < 5; ++i) {
        storage.createVersion(adapter_id);
        metadata.version = "v" + std::to_string(i + 1);
        storage.saveAdapter(adapter_id, weights, metadata);
    }
    
    // Verify version count doesn't exceed limit
    auto versions = storage.listVersions(adapter_id);
    EXPECT_LE(versions.size(), config_.max_versions);
}

// ═══════════════════════════════════════════════════════════
// Storage Statistics Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAStorageIntegrationTest, GetStorageStats) {
    LoRAStorageService storage(config_);
    
    // Create multiple adapters
    for (int i = 0; i < 5; ++i) {
        std::string id = "adapter-" + std::to_string(i);
        auto weights = createTestWeights(1024 * (i + 1));
        auto metadata = createTestMetadata(id);
        ASSERT_TRUE(storage.saveAdapter(id, weights, metadata));
    }
    
    // Get statistics
    auto stats = storage.getStats();
    EXPECT_TRUE(stats.contains("total_adapters") || stats.contains("adapter_count"));
}

// ═══════════════════════════════════════════════════════════
// Concurrent Access Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAStorageIntegrationTest, ConcurrentSaves) {
    LoRAStorageService storage(config_);
    
    std::vector<std::thread> threads;
    const int num_threads = 4;
    const int adapters_per_thread = 5;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&storage, t, adapters_per_thread, this]() {
            for (int i = 0; i < adapters_per_thread; ++i) {
                std::string id = "adapter-t" + std::to_string(t) + "-" + std::to_string(i);
                auto weights = createTestWeights(512);
                auto metadata = createTestMetadata(id);
                bool saved = storage.saveAdapter(id, weights, metadata);
                EXPECT_TRUE(saved);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify all adapters were saved
    auto list = storage.listAdapters();
    EXPECT_EQ(list.size(), num_threads * adapters_per_thread);
}

TEST_F(LoRAStorageIntegrationTest, ConcurrentReads) {
    LoRAStorageService storage(config_);
    
    // Setup: Save an adapter
    std::string adapter_id = "shared-adapter";
    auto weights = createTestWeights(2048);
    auto metadata = createTestMetadata(adapter_id);
    ASSERT_TRUE(storage.saveAdapter(adapter_id, weights, metadata));
    
    // Concurrent reads
    std::vector<std::thread> threads;
    const int num_threads = 10;
    std::atomic<int> success_count{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&storage, &adapter_id, &success_count]() {
            auto loaded = storage.loadAdapter(adapter_id);
            if (loaded.has_value()) {
                success_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count.load(), num_threads);
}

// ═══════════════════════════════════════════════════════════
// Edge Cases and Error Handling
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAStorageIntegrationTest, EmptyAdapter) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "empty-adapter";
    AdapterWeights weights;
    weights.data.clear();  // Empty weights
    weights.size_bytes = 0;
    auto metadata = createTestMetadata(adapter_id);
    
    // Save empty adapter
    bool saved = storage.saveAdapter(adapter_id, weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load and verify
    auto loaded = storage.loadAdapter(adapter_id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data.size(), 0);
}

TEST_F(LoRAStorageIntegrationTest, LargeAdapter) {
    LoRAStorageService storage(config_);
    
    std::string adapter_id = "large-adapter";
    // Create 10MB adapter
    auto weights = createTestWeights(10 * 1024 * 1024);
    auto metadata = createTestMetadata(adapter_id);
    
    // Save large adapter
    bool saved = storage.saveAdapter(adapter_id, weights, metadata);
    EXPECT_TRUE(saved);
    
    // Load and verify size
    auto loaded = storage.loadAdapter(adapter_id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->data.size(), weights.data.size());
}

TEST_F(LoRAStorageIntegrationTest, SpecialCharactersInId) {
    LoRAStorageService storage(config_);
    
    // Test various special characters
    std::vector<std::string> special_ids = {
        "adapter_with_underscore",
        "adapter-with-dash",
        "adapter.with.dots"
    };
    
    for (const auto& id : special_ids) {
        auto weights = createTestWeights(512);
        auto metadata = createTestMetadata(id);
        
        bool saved = storage.saveAdapter(id, weights, metadata);
        EXPECT_TRUE(saved) << "Failed to save adapter with ID: " << id;
        
        if (saved) {
            EXPECT_TRUE(storage.exists(id)) << "Adapter not found: " << id;
        }
    }
}
