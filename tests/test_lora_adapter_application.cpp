/**
 * @file test_lora_adapter_application.cpp
 * @brief Tests for LoRa adapter application to loaded models
 * 
 * This test suite validates the LoRa adapter application functionality
 * in the LlamaCppInferenceEngine, including:
 * - Basic adapter loading and application
 * - Multiple adapter management
 * - Adapter removal and cleanup
 * - Format conversion
 * - Validation of adapter application
 */

#include <gtest/gtest.h>
#include "llm/llamacpp_inference_engine.h"
#include "llm/lora_framework/lora_storage_service.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <fstream>

using namespace themis::llm;
using namespace themis::llm::lora;

namespace fs = std::filesystem;

/**
 * @brief Test fixture for LoRa adapter application tests
 */
class LoRAAdapterApplicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directories
        temp_dir_ = fs::temp_directory_path() / "themis_lora_adapter_test";
        db_path_ = temp_dir_ / "test_db";
        
        fs::create_directories(temp_dir_);
        fs::create_directories(db_path_);
        
        // Initialize RocksDB for storage backend
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.enable_blobdb = true;
        db_config.blob_size_threshold = 4096;
        
        db_ = std::make_shared<RocksDBWrapper>(db_config);
        db_->open();
        
        // Initialize LoRa storage service
        LoRAStorageService::Config storage_config;
        storage_config.backend = LoRAStorageService::Backend::ThemisDB;
        storage_config.db = db_;
        storage_config.enable_encryption = false;
        storage_config.enable_signatures = false;
        
        lora_storage_ = std::make_shared<LoRAStorageService>(storage_config);
        
        // Setup inference engine config
        engine_config_.n_ctx = 2048;
        engine_config_.n_threads = 4;
        engine_config_.n_gpu_layers = 0;
        engine_config_.lora_storage = lora_storage_;
        
        // Create mock model file
        createMockModelFile();
        
        // Create and load mock adapters
        createMockAdapters();
    }
    
    void TearDown() override {
        if (db_) {
            db_->close();
            db_.reset();
        }
        
        // Clean up temporary directory
        try {
            fs::remove_all(temp_dir_);
        } catch (const std::exception& e) {
            // Ignore cleanup errors
        }
    }
    
    void createMockModelFile() {
        model_path_ = (temp_dir_ / "test_model.gguf").string();
        
        std::ofstream file(model_path_, std::ios::binary);
        // Write GGUF magic number
        file.write("GGUF", 4);
        // Write version
        uint32_t version = 3;
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        // Write some dummy data
        std::vector<char> dummy(1024, 0);
        file.write(dummy.data(), dummy.size());
        file.close();
    }
    
    void createMockAdapters() {
        // Create mock adapter 1
        AdapterWeights weights1;
        weights1.format = "gguf";
        weights1.size_bytes = 32 * 1024 * 1024; // 32 MB
        weights1.data.resize(weights1.size_bytes, 0);
        weights1.hyperparameters.rank = 8;
        weights1.hyperparameters.alpha = 16.0f;
        
        AdapterMetadata metadata1;
        metadata1.adapter_id = "test-adapter-1";
        metadata1.version = "v1";
        metadata1.base_model = "test-model";
        metadata1.description = "Test adapter 1";
        metadata1.created_at = std::chrono::system_clock::now();
        metadata1.updated_at = metadata1.created_at;
        
        lora_storage_->saveAdapter("test-adapter-1", weights1, metadata1);
        
        // Create mock adapter 2
        AdapterWeights weights2;
        weights2.format = "safetensors";
        weights2.size_bytes = 16 * 1024 * 1024; // 16 MB
        weights2.data.resize(weights2.size_bytes, 0);
        weights2.hyperparameters.rank = 4;
        weights2.hyperparameters.alpha = 8.0f;
        
        AdapterMetadata metadata2;
        metadata2.adapter_id = "test-adapter-2";
        metadata2.version = "v1";
        metadata2.base_model = "test-model";
        metadata2.description = "Test adapter 2";
        metadata2.created_at = std::chrono::system_clock::now();
        metadata2.updated_at = metadata2.created_at;
        
        lora_storage_->saveAdapter("test-adapter-2", weights2, metadata2);
    }
    
    fs::path temp_dir_;
    fs::path db_path_;
    std::string model_path_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<LoRAStorageService> lora_storage_;
    LlamaCppInferenceEngine::Config engine_config_;
};

// ═══════════════════════════════════════════════════════════
// Basic Adapter Loading Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, LoadAndApplySingleAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model first
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Load and apply adapter
    bool applied = engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f);
    EXPECT_TRUE(applied);
    
    // Verify adapter is active
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
    
    // Verify it's in the active adapters list
    auto active = engine.getActiveAdapters();
    EXPECT_EQ(active.size(), 1);
    EXPECT_EQ(active[0], "test-adapter-1");
}

TEST_F(LoRAAdapterApplicationTest, LoadAdapterWithoutModel) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Try to load adapter without model (should fail)
    bool applied = engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f);
    EXPECT_FALSE(applied);
    
    // Verify adapter is not active
    EXPECT_FALSE(engine.isAdapterActive("test-adapter-1"));
}

TEST_F(LoRAAdapterApplicationTest, LoadNonExistentAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model first
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Try to load non-existent adapter
    bool applied = engine.loadAndApplyLoRAAdapter("non-existent-adapter", 1.0f);
    EXPECT_FALSE(applied);
    
    // Verify no adapters are active
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
}

TEST_F(LoRAAdapterApplicationTest, LoadAdapterWithDifferentScales) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Test different scale values
    std::vector<float> scales = {0.0f, 0.5f, 1.0f, 2.0f};
    
    for (float scale : scales) {
        // Remove previous adapter if exists
        if (engine.isAdapterActive("test-adapter-1")) {
            engine.removeAdapter("test-adapter-1");
        }
        
        // Apply with different scale
        bool applied = engine.loadAndApplyLoRAAdapter("test-adapter-1", scale);
        EXPECT_TRUE(applied);
        EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
    }
}

// ═══════════════════════════════════════════════════════════
// Multiple Adapter Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, ApplyMultipleAdapters) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Apply multiple adapters
    std::vector<std::pair<std::string, float>> adapters = {
        {"test-adapter-1", 1.0f},
        {"test-adapter-2", 0.5f}
    };
    
    bool all_applied = engine.applyMultipleAdapters(adapters);
    EXPECT_TRUE(all_applied);
    
    // Verify both adapters are active
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-2"));
    
    auto active = engine.getActiveAdapters();
    EXPECT_EQ(active.size(), 2);
}

TEST_F(LoRAAdapterApplicationTest, ApplyMultipleAdaptersWithFailure) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Try to apply mix of valid and invalid adapters
    std::vector<std::pair<std::string, float>> adapters = {
        {"test-adapter-1", 1.0f},
        {"non-existent-adapter", 0.5f},
        {"test-adapter-2", 0.8f}
    };
    
    bool all_applied = engine.applyMultipleAdapters(adapters);
    EXPECT_FALSE(all_applied); // Should return false due to one failure
    
    // But valid adapters should still be applied
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-2"));
    EXPECT_FALSE(engine.isAdapterActive("non-existent-adapter"));
}

TEST_F(LoRAAdapterApplicationTest, LoadSameAdapterTwice) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Apply adapter first time
    bool applied1 = engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f);
    EXPECT_TRUE(applied1);
    
    // Try to apply same adapter again (should succeed without reloading)
    bool applied2 = engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f);
    EXPECT_TRUE(applied2);
    
    // Should still have only one active adapter
    auto active = engine.getActiveAdapters();
    EXPECT_EQ(active.size(), 1);
}

// ═══════════════════════════════════════════════════════════
// Adapter Removal Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, RemoveSingleAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model and adapter
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f));
    
    // Verify adapter is active
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
    
    // Remove adapter
    bool removed = engine.removeAdapter("test-adapter-1");
    EXPECT_TRUE(removed);
    
    // Verify adapter is no longer active
    EXPECT_FALSE(engine.isAdapterActive("test-adapter-1"));
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
}

TEST_F(LoRAAdapterApplicationTest, RemoveNonExistentAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Try to remove non-existent adapter
    bool removed = engine.removeAdapter("non-existent-adapter");
    EXPECT_FALSE(removed);
}

TEST_F(LoRAAdapterApplicationTest, ClearAllAdapters) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model and multiple adapters
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-2", 0.5f));
    
    // Verify both adapters are active
    EXPECT_EQ(engine.getActiveAdapters().size(), 2);
    
    // Clear all adapters
    engine.clearAllAdapters();
    
    // Verify no adapters are active
    EXPECT_FALSE(engine.isAdapterActive("test-adapter-1"));
    EXPECT_FALSE(engine.isAdapterActive("test-adapter-2"));
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
}

TEST_F(LoRAAdapterApplicationTest, ClearAdaptersWhenNoneActive) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model but no adapters
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Clear adapters (should be safe to call)
    engine.clearAllAdapters();
    
    // Verify no adapters are active
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
}

// ═══════════════════════════════════════════════════════════
// Format Conversion Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, LoadGGUFFormatAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Load GGUF format adapter
    bool applied = engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f);
    EXPECT_TRUE(applied);
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
}

TEST_F(LoRAAdapterApplicationTest, LoadSafetensorsFormatAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Load safetensors format adapter (should handle conversion)
    bool applied = engine.loadAndApplyLoRAAdapter("test-adapter-2", 1.0f);
    EXPECT_TRUE(applied);
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-2"));
}

// ═══════════════════════════════════════════════════════════
// Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, ValidateAdapterApplication) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model and adapter
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f));
    
    // Validate adapter application
    bool validated = engine.validateAdapterApplication("test-adapter-1");
    EXPECT_TRUE(validated);
}

TEST_F(LoRAAdapterApplicationTest, ValidateNonActiveAdapter) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model but don't apply adapter
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Try to validate non-active adapter
    bool validated = engine.validateAdapterApplication("test-adapter-1");
    EXPECT_FALSE(validated);
}

// ═══════════════════════════════════════════════════════════
// Cleanup Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, CleanupOnModelUnload) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model and adapters
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f));
    ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-2", 0.5f));
    
    // Verify adapters are active
    EXPECT_EQ(engine.getActiveAdapters().size(), 2);
    
    // Unload model (should cleanup adapters)
    engine.unloadModel();
    
    // Verify adapters are cleared
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
}

TEST_F(LoRAAdapterApplicationTest, TempFileCleanup) {
    // Create temp directory for adapters
    fs::path adapter_temp_dir = fs::temp_directory_path() / "themis_adapters";
    
    {
        LlamaCppInferenceEngine engine(engine_config_);
        
        // Load model and adapter
        ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
        ASSERT_TRUE(engine.loadAndApplyLoRAAdapter("test-adapter-1", 1.0f));
        
        // Check if temp directory was created
        EXPECT_TRUE(fs::exists(adapter_temp_dir));
        
        // Engine destructor should cleanup temp files
    }
    
    // Temp files should be cleaned up (directory may still exist but should be empty or removed)
    if (fs::exists(adapter_temp_dir)) {
        // Directory exists but should be empty or only contain directories
        int file_count = 0;
        for (const auto& entry : fs::directory_iterator(adapter_temp_dir)) {
            if (entry.is_regular_file()) {
                file_count++;
            }
        }
        EXPECT_EQ(file_count, 0); // No regular files should remain
    }
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(LoRAAdapterApplicationTest, LoadFromThemisDBIntegration) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // Load and apply adapter using ThemisDB integration
    bool loaded = engine.loadAdapterFromThemisDB("test-adapter-1");
    EXPECT_TRUE(loaded);
    
    // Verify adapter is active
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-1"));
}

TEST_F(LoRAAdapterApplicationTest, CompleteWorkflow) {
    LlamaCppInferenceEngine engine(engine_config_);
    
    // 1. Load model
    ASSERT_TRUE(engine.loadModel(model_path_, "test-model"));
    
    // 2. Apply multiple adapters
    std::vector<std::pair<std::string, float>> adapters = {
        {"test-adapter-1", 1.0f},
        {"test-adapter-2", 0.7f}
    };
    ASSERT_TRUE(engine.applyMultipleAdapters(adapters));
    
    // 3. Validate adapters
    EXPECT_TRUE(engine.validateAdapterApplication("test-adapter-1"));
    EXPECT_TRUE(engine.validateAdapterApplication("test-adapter-2"));
    
    // 4. Remove one adapter
    ASSERT_TRUE(engine.removeAdapter("test-adapter-1"));
    EXPECT_FALSE(engine.isAdapterActive("test-adapter-1"));
    EXPECT_TRUE(engine.isAdapterActive("test-adapter-2"));
    
    // 5. Clear all remaining adapters
    engine.clearAllAdapters();
    EXPECT_EQ(engine.getActiveAdapters().size(), 0);
    
    // 6. Unload model
    engine.unloadModel();
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
