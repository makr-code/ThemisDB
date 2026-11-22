/**
 * @file test_onnx_model_loader.cpp
 * @brief Unit tests for ONNX Model Loader
 */

#include "rag/onnx_model_loader.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

using namespace themis::rag::judge;

class ONNXModelLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_cache_dir = "./test_model_cache";
        std::filesystem::create_directories(test_cache_dir);
    }
    
    void TearDown() override {
        if (std::filesystem::exists(test_cache_dir)) {
            std::filesystem::remove_all(test_cache_dir);
        }
    }
    
    void createDummyModelFile(const std::string& path) {
        std::ofstream file(path);
        file << "Dummy ONNX model content for testing\n";
        file << "This is not a real ONNX model, just test data\n";
        file.close();
    }
    
    std::string test_cache_dir;
};

// ═══════════════════════════════════════════════════════════
// Constructor and Configuration Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, ConstructorDefault) {
    ONNXModelLoader loader;
    EXPECT_EQ(loader.getConfig().cache_dir, "./models");
    EXPECT_TRUE(loader.getConfig().verify_checksum);
}

TEST_F(ONNXModelLoaderTest, ConstructorWithConfig) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    config.auto_download = true;
    
    ONNXModelLoader loader(config);
    EXPECT_EQ(loader.getConfig().cache_dir, test_cache_dir);
    EXPECT_FALSE(loader.getConfig().verify_checksum);
    EXPECT_TRUE(loader.getConfig().auto_download);
}

// ═══════════════════════════════════════════════════════════
// Model Loading Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, LoadModelFromLocalPath) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    // Create dummy model file
    std::string model_path = test_cache_dir + "/test_model.onnx";
    createDummyModelFile(model_path);
    
    auto model_info = loader.loadModel(model_path);
    ASSERT_TRUE(model_info.has_value());
    EXPECT_EQ(model_info->model_path, model_path);
    EXPECT_FALSE(model_info->model_name.empty());
    EXPECT_GT(model_info->model_size_bytes, 0);
}

TEST_F(ONNXModelLoaderTest, LoadModelNonExistentPath) {
    ONNXModelLoader loader;
    auto model_info = loader.loadModel("/nonexistent/model.onnx");
    EXPECT_FALSE(model_info.has_value());
}

TEST_F(ONNXModelLoaderTest, LoadModelEmptyPath) {
    ONNXModelLoader loader;
    auto model_info = loader.loadModel("");
    EXPECT_FALSE(model_info.has_value());
}

// ═══════════════════════════════════════════════════════════
// Model Caching Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, IsModelCachedAfterLoad) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    std::string model_path = test_cache_dir + "/cached_model.onnx";
    createDummyModelFile(model_path);
    
    auto model_info = loader.loadModel(model_path);
    ASSERT_TRUE(model_info.has_value());
    
    EXPECT_TRUE(loader.isModelCached(model_info->model_name));
}

TEST_F(ONNXModelLoaderTest, GetCachedModelPath) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    std::string model_path = test_cache_dir + "/path_test_model.onnx";
    createDummyModelFile(model_path);
    
    auto model_info = loader.loadModel(model_path);
    ASSERT_TRUE(model_info.has_value());
    
    std::string cached_path = loader.getCachedModelPath(model_info->model_name);
    EXPECT_FALSE(cached_path.empty());
    EXPECT_TRUE(std::filesystem::exists(cached_path));
}

TEST_F(ONNXModelLoaderTest, ListCachedModels) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    // Load multiple models
    std::string model1 = test_cache_dir + "/model1.onnx";
    std::string model2 = test_cache_dir + "/model2.onnx";
    createDummyModelFile(model1);
    createDummyModelFile(model2);
    
    loader.loadModel(model1);
    loader.loadModel(model2);
    
    auto cached_models = loader.listCachedModels();
    EXPECT_GE(cached_models.size(), 2);
}

// ═══════════════════════════════════════════════════════════
// Checksum Validation Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, ValidateModelChecksumMatch) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    
    ONNXModelLoader loader(config);
    
    std::string model_path = test_cache_dir + "/checksum_model.onnx";
    createDummyModelFile(model_path);
    
    // Load to get checksum
    auto model_info = loader.loadModel(model_path);
    ASSERT_TRUE(model_info.has_value());
    
    std::string checksum = model_info->checksum;
    EXPECT_FALSE(checksum.empty());
    
    // Validate with same checksum
    EXPECT_TRUE(loader.validateModelChecksum(model_path, checksum));
}

TEST_F(ONNXModelLoaderTest, ValidateModelChecksumMismatch) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    
    ONNXModelLoader loader(config);
    
    std::string model_path = test_cache_dir + "/mismatch_model.onnx";
    createDummyModelFile(model_path);
    
    // Use incorrect checksum
    std::string wrong_checksum = "0000000000000000000000000000000000000000000000000000000000000000";
    EXPECT_FALSE(loader.validateModelChecksum(model_path, wrong_checksum));
}

// ═══════════════════════════════════════════════════════════
// Cache Management Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, ClearSpecificModel) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    std::string model_path = test_cache_dir + "/clear_test.onnx";
    createDummyModelFile(model_path);
    
    auto model_info = loader.loadModel(model_path);
    ASSERT_TRUE(model_info.has_value());
    
    size_t cleared = loader.clearCache(model_info->model_name);
    EXPECT_EQ(cleared, 1);
    EXPECT_FALSE(loader.isModelCached(model_info->model_name));
}

TEST_F(ONNXModelLoaderTest, ClearAllModels) {
    ONNXModelLoaderConfig config;
    config.cache_dir = test_cache_dir;
    config.verify_checksum = false;
    
    ONNXModelLoader loader(config);
    
    // Load multiple models
    for (int i = 0; i < 3; i++) {
        std::string model_path = test_cache_dir + "/model_" + std::to_string(i) + ".onnx";
        createDummyModelFile(model_path);
        loader.loadModel(model_path);
    }
    
    size_t cleared = loader.clearCache();  // Clear all
    EXPECT_GE(cleared, 3);
}

// ═══════════════════════════════════════════════════════════
// Model Registry Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, RegisterAndGetModelInfo) {
    ONNXModelLoader loader;
    
    ONNXModelInfo info;
    info.model_name = "custom_model";
    info.model_path = "/path/to/custom.onnx";
    info.model_size_bytes = 1000000;
    
    loader.registerModel(info);
    
    auto retrieved = loader.getModelInfo("custom_model");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->model_name, "custom_model");
    EXPECT_EQ(retrieved->model_size_bytes, 1000000);
}

TEST_F(ONNXModelLoaderTest, GetNonExistentModelInfo) {
    ONNXModelLoader loader;
    auto info = loader.getModelInfo("nonexistent_model");
    EXPECT_FALSE(info.has_value());
}

// ═══════════════════════════════════════════════════════════
// NLI Model Factory Tests
// ═══════════════════════════════════════════════════════════

TEST_F(ONNXModelLoaderTest, NLIModelFactoryDeberta) {
    auto info = NLIModelFactory::getDebertaV3LargeMNLI();
    EXPECT_EQ(info.model_name, "deberta-v3-large-mnli");
    EXPECT_FALSE(info.model_url.empty());
    EXPECT_GT(info.model_size_bytes, 0);
}

TEST_F(ONNXModelLoaderTest, NLIModelFactoryRoberta) {
    auto info = NLIModelFactory::getRobertaLargeMNLI();
    EXPECT_EQ(info.model_name, "roberta-large-mnli");
    EXPECT_FALSE(info.model_url.empty());
}

TEST_F(ONNXModelLoaderTest, NLIModelFactoryBart) {
    auto info = NLIModelFactory::getBartLargeMNLI();
    EXPECT_EQ(info.model_name, "bart-large-mnli");
    EXPECT_FALSE(info.model_url.empty());
}

TEST_F(ONNXModelLoaderTest, NLIModelFactoryGetAllSupported) {
    auto models = NLIModelFactory::getAllSupportedModels();
    EXPECT_GE(models.size(), 3);
    
    // Check that all models have required fields
    for (const auto& model : models) {
        EXPECT_FALSE(model.model_name.empty());
        EXPECT_GT(model.model_size_bytes, 0);
    }
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════
