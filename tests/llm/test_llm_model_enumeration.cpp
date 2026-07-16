/**
 * @file test_llm_model_enumeration.cpp
 * @brief Tests for LLM Model Storage Enumeration (Stub #303)
 */

#include <gtest/gtest.h>
#include "llm/llm_model_storage.h"
#include "storage/rocksdb_wrapper.h"
#include <filesystem>
#include <memory>
#include <chrono>

namespace themis {
namespace test {

using namespace llm;

class LLMModelEnumerationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database directory
        auto now = std::chrono::system_clock::now().time_since_epoch();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        db_path_ = std::filesystem::temp_directory_path() / 
                   ("themis_llm_enum_test_" + std::to_string(ms));
        std::filesystem::create_directories(db_path_);
        
        // Initialize RocksDB
        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.enable_wal = true;
        db_ = std::make_unique<RocksDBWrapper>(db_config);
        
        // Create model storage with the database
        LLMModelStorage::Config config;
        config.db = std::shared_ptr<RocksDBWrapper>(db_.get(), [](RocksDBWrapper*){});
        config.key_prefix = "llm_model::";
        
        storage_ = std::make_unique<LLMModelStorage>(config);
    }
    
    void TearDown() override {
        storage_.reset();
        db_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }
    
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<LLMModelStorage> storage_;
    std::filesystem::path db_path_;
};

// Test 1: listModels returns empty vector initially
TEST_F(LLMModelEnumerationTest, ListModelsEmptyInitially) {
    auto models = storage_->listModels();
    EXPECT_EQ(models.size(), 0);
}

// Test 2: Store a single model and list it
TEST_F(LLMModelEnumerationTest, ListSingleModel) {
    LLMModelMetadata metadata;
    metadata.model_id = "test-model-1";
    metadata.model_name = "Test Model 1";
    metadata.version = "1.0";
    metadata.architecture = "llama";
    
    ASSERT_TRUE(storage_->storeModel(metadata));
    
    auto models = storage_->listModels();
    ASSERT_EQ(models.size(), 1);
    EXPECT_EQ(models[0], "test-model-1");
}

// Test 3: Store multiple models and list them all
TEST_F(LLMModelEnumerationTest, ListMultipleModels) {
    std::vector<std::string> model_ids = {
        "llama-7b",
        "mistral-7b",
        "falcon-40b",
        "phi-2"
    };
    
    for (const auto& id : model_ids) {
        LLMModelMetadata metadata;
        metadata.model_id = id;
        metadata.model_name = id;
        metadata.version = "1.0";
        metadata.architecture = "test";
        ASSERT_TRUE(storage_->storeModel(metadata));
    }
    
    auto models = storage_->listModels();
    ASSERT_EQ(models.size(), model_ids.size());
    
    // Verify all models are in the list
    for (const auto& id : model_ids) {
        auto it = std::find(models.begin(), models.end(), id);
        EXPECT_NE(it, models.end()) << "Model " << id << " not found in list";
    }
}

// Test 4: Filter models by substring
TEST_F(LLMModelEnumerationTest, FilterModelsBySubstring) {
    std::vector<std::string> model_ids = {
        "llama-7b",
        "llama-13b",
        "llama-70b",
        "mistral-7b"
    };
    
    for (const auto& id : model_ids) {
        LLMModelMetadata metadata;
        metadata.model_id = id;
        metadata.model_name = id;
        metadata.version = "1.0";
        metadata.architecture = "test";
        ASSERT_TRUE(storage_->storeModel(metadata));
    }
    
    // Filter for "llama" models
    auto models = storage_->listModels("llama");
    ASSERT_EQ(models.size(), 3);
    
    for (const auto& model : models) {
        EXPECT_NE(model.find("llama"), std::string::npos);
    }
}

// Test 5: Filter returns only matching models
TEST_F(LLMModelEnumerationTest, FilterReturnsOnlyMatching) {
    LLMModelMetadata metadata;
    metadata.model_id = "test-abc-123";
    metadata.model_name = "Test ABC";
    metadata.version = "1.0";
    metadata.architecture = "test";
    ASSERT_TRUE(storage_->storeModel(metadata));
    
    metadata.model_id = "prod-xyz-456";
    metadata.model_name = "Prod XYZ";
    ASSERT_TRUE(storage_->storeModel(metadata));
    
    // Filter for "prod" models
    auto models = storage_->listModels("prod");
    ASSERT_EQ(models.size(), 1);
    EXPECT_EQ(models[0], "prod-xyz-456");
}

// Test 6: listModels after delete
TEST_F(LLMModelEnumerationTest, ListModelsAfterDelete) {
    LLMModelMetadata metadata;
    metadata.model_id = "model-1";
    metadata.model_name = "Model 1";
    metadata.version = "1.0";
    metadata.architecture = "test";
    ASSERT_TRUE(storage_->storeModel(metadata));
    
    metadata.model_id = "model-2";
    metadata.model_name = "Model 2";
    ASSERT_TRUE(storage_->storeModel(metadata));
    
    // Verify both are listed
    auto models = storage_->listModels();
    ASSERT_EQ(models.size(), 2);
    
    // Delete one model
    ASSERT_TRUE(storage_->deleteModel("model-1"));
    
    // Verify only one remains
    models = storage_->listModels();
    ASSERT_EQ(models.size(), 1);
    EXPECT_EQ(models[0], "model-2");
}

// Test 7: RocksDBWrapper::prefixIterator (if available)
TEST_F(LLMModelEnumerationTest, PrefixIteratorIfAvailable) {
    // Store test data via db directly
    db_->put("llm_model::model-1", "data1");
    db_->put("llm_model::model-2", "data2");
    db_->put("other::model-3", "data3");
    
    // Try to use prefixIterator if available
    auto result = db_->prefixIterator("llm_model::");
    if (result) {
        auto iter = std::move(result.value());
        int count = 0;
        while (iter.Valid()) {
            std::string_view key = iter.key();
            EXPECT_TRUE(key.find("llm_model::") == 0) << "Key not matching prefix: " << key;
            count++;
            iter.Next();
        }
        EXPECT_EQ(count, 2) << "Expected to find 2 keys with prefix";
    }
}

} // namespace test
} // namespace themis
