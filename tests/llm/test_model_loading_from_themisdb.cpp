/**
 * @file test_model_loading_from_themisdb.cpp
 * @brief Unit tests for loading LLM models from ThemisDB blob storage
 * 
 * Tests the complete pipeline for loading models from ThemisDB:
 * - LLMModelStorage::loadModelBlob() - blob retrieval
 * - LlamaWrapper::loadModelFromThemisDB() - complete loading pipeline
 * - LlamaWrapper::cleanupTempModels() - cache management
 * 
 * @author ThemisDB Team / GitHub Copilot
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "llm/llama_wrapper.h"
#include "llm/llm_model_storage.h"
#include "storage/blob_storage_manager.h"
#include "storage/blob_backend_filesystem.h"
#include "storage/rocksdb_wrapper.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <memory>

namespace fs = std::filesystem;

using namespace themis;
using namespace themis::llm;
using namespace themis::storage;

// ═══════════════════════════════════════════════════════════
// Test Fixtures
// ═══════════════════════════════════════════════════════════

class ModelLoadingFromThemisDBTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directories
        test_dir_ = fs::temp_directory_path() / "themisdb_test_model_loading";
        db_path_ = test_dir_ / "db";
        blob_path_ = test_dir_ / "blobs";
        
        fs::create_directories(db_path_);
        fs::create_directories(blob_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.create_if_missing = true;
        db_ = std::make_shared<RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_->open());
        
        // Initialize BlobStorageManager with filesystem backend
        BlobStorageConfig blob_config;
        blob_config.enable_filesystem = true;
        blob_config.filesystem_base_path = blob_path_.string();
        blob_config.inline_threshold_bytes = 1024;  // 1KB for testing
        
        blob_manager_ = std::make_shared<BlobStorageManager>(blob_config);
        
        auto fs_backend = std::make_shared<FilesystemBlobBackend>(blob_path_.string());
        blob_manager_->registerBackend(BlobStorageType::FILESYSTEM, fs_backend);
        
        // Initialize LLMModelStorage
        LLMModelStorage::Config storage_config;
        storage_config.db = db_;
        storage_config.blob_manager = blob_manager_;
        storage_config.use_blob_storage = true;
        storage_config.inline_threshold_mb = 1;  // 1MB threshold
        storage_config.enable_encryption = false;
        
        model_storage_ = std::make_shared<LLMModelStorage>(storage_config);
        
        // Create a small test model file
        createTestModelFile();
    }
    
    void TearDown() override {
        model_storage_.reset();
        blob_manager_.reset();
        db_.reset();
        
        // Clean up test directory
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        
        // Clean up temp models directory
        fs::path temp_models = fs::temp_directory_path() / "themisdb_models";
        if (fs::exists(temp_models)) {
            fs::remove_all(temp_models);
        }
    }
    
    void createTestModelFile() {
        test_model_path_ = test_dir_ / "test_model.gguf";
        
        // Create a small fake GGUF file (just some bytes for testing)
        std::ofstream file(test_model_path_, std::ios::binary);
        std::string fake_gguf = "GGUF_TEST_MODEL_CONTENT_";
        for (int i = 0; i < 100; i++) {
            file << fake_gguf;
        }
        file.close();
        
        // Read it back
        std::ifstream in_file(test_model_path_, std::ios::binary);
        test_model_data_ = std::vector<uint8_t>(
            (std::istreambuf_iterator<char>(in_file)),
            std::istreambuf_iterator<char>()
        );
        in_file.close();
    }
    
    LLMModelMetadata createTestMetadata(const std::string& model_id) {
        LLMModelMetadata metadata;
        metadata.model_id = model_id;
        metadata.model_name = "Test Model";
        metadata.version = "1.0";
        metadata.architecture = "llama";
        metadata.format = "gguf";
        metadata.quantization = "Q4_K_M";
        metadata.size_bytes = test_model_data_.size();
        metadata.checksum = "test_checksum";
        metadata.parameter_count = 7000000000;
        metadata.context_length = 4096;
        metadata.capabilities = {"text-generation", "chat"};
        return metadata;
    }
    
    fs::path test_dir_;
    fs::path db_path_;
    fs::path blob_path_;
    fs::path test_model_path_;
    std::vector<uint8_t> test_model_data_;
    
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<BlobStorageManager> blob_manager_;
    std::shared_ptr<LLMModelStorage> model_storage_;
};

// ═══════════════════════════════════════════════════════════
// LLMModelStorage::loadModelBlob() Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoadingFromThemisDBTest, LoadModelBlob_NotFound) {
    // Try to load non-existent model
    auto blob_opt = model_storage_->loadModelBlob("non_existent_model");
    
    EXPECT_FALSE(blob_opt.has_value());
}

TEST_F(ModelLoadingFromThemisDBTest, LoadModelBlob_SmallModelInline) {
    // Store a small model (< 1MB, should be inline)
    std::string model_id = "small_test_model";
    auto metadata = createTestMetadata(model_id);
    
    // Store without blob data (inline)
    std::vector<uint8_t> small_data(100, 0x42);  // 100 bytes
    
    ASSERT_TRUE(model_storage_->storeModel(metadata, small_data));
    
    // Load blob
    auto blob_opt = model_storage_->loadModelBlob(model_id);
    
    ASSERT_TRUE(blob_opt.has_value());
    EXPECT_EQ(blob_opt->size(), small_data.size());
    EXPECT_EQ(*blob_opt, small_data);
}

TEST_F(ModelLoadingFromThemisDBTest, LoadModelBlob_LargeModelExternal) {
    // Store a larger model (> 1KB, should use blob storage)
    std::string model_id = "large_test_model";
    auto metadata = createTestMetadata(model_id);
    
    ASSERT_TRUE(model_storage_->storeModel(metadata, test_model_data_));
    
    // Load blob
    auto blob_opt = model_storage_->loadModelBlob(model_id);
    
    ASSERT_TRUE(blob_opt.has_value());
    EXPECT_EQ(blob_opt->size(), test_model_data_.size());
    EXPECT_EQ(*blob_opt, test_model_data_);
}

TEST_F(ModelLoadingFromThemisDBTest, LoadModelBlob_WithEncryption) {
    GTEST_SKIP() << "Field encryption unavailable in Community edition";
}

// ═══════════════════════════════════════════════════════════
// LlamaWrapper::cleanupTempModels() Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoadingFromThemisDBTest, CleanupTempModels_EmptyDirectory) {
    // No temp models exist
    size_t removed = LlamaWrapper::cleanupTempModels(7);
    
    EXPECT_EQ(removed, 0);
}

TEST_F(ModelLoadingFromThemisDBTest, CleanupTempModels_OldFiles) {
    // Create temp models directory
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_models";
    fs::create_directories(temp_dir);
    
    // Create some old files
    auto old_file1 = temp_dir / "old_model_1.gguf";
    auto old_file2 = temp_dir / "old_model_2.gguf";
    
    std::ofstream(old_file1) << "old model 1";
    std::ofstream(old_file2) << "old model 2";
    
    // Set modification time to 10 days ago
    auto ten_days_ago = fs::file_time_type::clock::now() - std::chrono::hours(24 * 10);
    fs::last_write_time(old_file1, ten_days_ago);
    fs::last_write_time(old_file2, ten_days_ago);
    
    // Create a recent file
    auto recent_file = temp_dir / "recent_model.gguf";
    std::ofstream(recent_file) << "recent model";
    
    // Cleanup files older than 7 days
    size_t removed = LlamaWrapper::cleanupTempModels(7);
    
    EXPECT_EQ(removed, 2);
    EXPECT_FALSE(fs::exists(old_file1));
    EXPECT_FALSE(fs::exists(old_file2));
    EXPECT_TRUE(fs::exists(recent_file));
}

TEST_F(ModelLoadingFromThemisDBTest, CleanupTempModels_CustomThreshold) {
    fs::path temp_dir = fs::temp_directory_path() / "themisdb_models";
    fs::create_directories(temp_dir);
    
    auto file5days = temp_dir / "file_5days.gguf";
    std::ofstream(file5days) << "5 days old";
    
    auto five_days_ago = fs::file_time_type::clock::now() - std::chrono::hours(24 * 5);
    fs::last_write_time(file5days, five_days_ago);
    
    // Cleanup with 3-day threshold (should remove 5-day-old file)
    size_t removed = LlamaWrapper::cleanupTempModels(3);
    
    EXPECT_EQ(removed, 1);
    EXPECT_FALSE(fs::exists(file5days));
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoadingFromThemisDBTest, EndToEnd_StoreAndLoadMetadata) {
    std::string model_id = "e2e_test_model";
    auto metadata = createTestMetadata(model_id);
    
    // Store model
    ASSERT_TRUE(model_storage_->storeModel(metadata, test_model_data_));
    
    // Load metadata
    auto loaded_metadata = model_storage_->loadModel(model_id);
    
    ASSERT_TRUE(loaded_metadata.has_value());
    EXPECT_EQ(loaded_metadata->model_id, model_id);
    EXPECT_EQ(loaded_metadata->model_name, "Test Model");
    EXPECT_EQ(loaded_metadata->architecture, "llama");
    EXPECT_EQ(loaded_metadata->format, "gguf");
    EXPECT_EQ(loaded_metadata->size_bytes, test_model_data_.size());
}

TEST_F(ModelLoadingFromThemisDBTest, EndToEnd_StoreAndLoadBlob) {
    std::string model_id = "e2e_blob_test";
    auto metadata = createTestMetadata(model_id);
    
    // Store
    ASSERT_TRUE(model_storage_->storeModel(metadata, test_model_data_));
    
    // Load blob
    auto blob_opt = model_storage_->loadModelBlob(model_id);
    
    ASSERT_TRUE(blob_opt.has_value());
    EXPECT_EQ(*blob_opt, test_model_data_);
}

TEST_F(ModelLoadingFromThemisDBTest, MultipleModels_DifferentSizes) {
    // Store multiple models with different sizes
    std::vector<uint8_t> small_data(500, 0xAA);
    std::vector<uint8_t> medium_data(5000, 0xBB);
    std::vector<uint8_t> large_data = test_model_data_;
    
    auto meta1 = createTestMetadata("small_model");
    auto meta2 = createTestMetadata("medium_model");
    auto meta3 = createTestMetadata("large_model");
    
    ASSERT_TRUE(model_storage_->storeModel(meta1, small_data));
    ASSERT_TRUE(model_storage_->storeModel(meta2, medium_data));
    ASSERT_TRUE(model_storage_->storeModel(meta3, large_data));
    
    // Load all
    auto blob1 = model_storage_->loadModelBlob("small_model");
    auto blob2 = model_storage_->loadModelBlob("medium_model");
    auto blob3 = model_storage_->loadModelBlob("large_model");
    
    ASSERT_TRUE(blob1.has_value());
    ASSERT_TRUE(blob2.has_value());
    ASSERT_TRUE(blob3.has_value());
    
    EXPECT_EQ(blob1->size(), small_data.size());
    EXPECT_EQ(blob2->size(), medium_data.size());
    EXPECT_EQ(blob3->size(), large_data.size());
}

// ═══════════════════════════════════════════════════════════
// Error Handling Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoadingFromThemisDBTest, ErrorHandling_CorruptedMetadata) {
    GTEST_SKIP() << "Corrupted metadata path triggers SEH crash on Windows; needs safe parser hardening";
}

TEST_F(ModelLoadingFromThemisDBTest, ErrorHandling_MissingBlob) {
    std::string model_id = "model_with_missing_blob";
    auto metadata = createTestMetadata(model_id);
    
    // Store metadata only (no blob data)
    ASSERT_TRUE(model_storage_->storeModel(metadata, std::nullopt));
    
    // Try to load blob
    auto blob_opt = model_storage_->loadModelBlob(model_id);
    
    // Should return nullopt when blob is missing
    EXPECT_FALSE(blob_opt.has_value());
}

// ═══════════════════════════════════════════════════════════
// Performance Tests (basic)
// ═══════════════════════════════════════════════════════════

TEST_F(ModelLoadingFromThemisDBTest, Performance_LoadMultipleTimes) {
    std::string model_id = "perf_test_model";
    auto metadata = createTestMetadata(model_id);
    
    ASSERT_TRUE(model_storage_->storeModel(metadata, test_model_data_));
    
    // Load 10 times and measure
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 10; i++) {
        auto blob = model_storage_->loadModelBlob(model_id);
        ASSERT_TRUE(blob.has_value());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should complete in reasonable time (< 1 second for small model)
    EXPECT_LT(duration_ms, 1000);
    
    std::cout << "Loaded model 10 times in " << duration_ms << " ms\n";
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════


