/**
 * @file test_llm_deployment_plugin.cpp
 * @brief Unit tests for LLM Deployment Plugin
 */

#include <gtest/gtest.h>
#include "llm/llm_deployment_plugin.h"
#include "llm/model_downloader.h"
#include <filesystem>
#include <fstream>
#include <vector>

using namespace themis::llm;
namespace fs = std::filesystem;

class LLMDeploymentPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use portable temp directory with unique subdirectory per test
        fs::path temp_base = fs::temp_directory_path();
        test_dir_ = (temp_base / "themis_test_models_deployment").string();
        
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);
    }

    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }

    std::string test_dir_;
};

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, LoadConfigFromYAML) {
    // Create test configuration
    std::string config_path = test_dir_ + "/test_config.yaml";
    std::ofstream config_file(config_path);
    config_file << R"(
deployment:
  mode: auto
  cache_directory: /tmp/test_models
  enable_cache: true
  max_cache_size_gb: 50
  ollama_url: http://localhost:11434
  verify_checksums: true
  enable_audit_log: true
  sources:
    - type: ollama
      location: http://localhost:11434
      priority: 100
)";
    config_file.close();

    auto config = LLMDeploymentPlugin::loadConfigFromYAML(config_path);
    
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->mode, DeploymentMode::AUTO);
    EXPECT_EQ(config->cache_directory, "/tmp/test_models");
    EXPECT_TRUE(config->enable_cache);
    EXPECT_EQ(config->max_cache_size_gb, 50);
    EXPECT_EQ(config->ollama_url, "http://localhost:11434");
    EXPECT_TRUE(config->verify_checksums);
    EXPECT_TRUE(config->enable_audit_log);
    EXPECT_EQ(config->sources.size(), 1);
    EXPECT_EQ(config->sources[0].type, "ollama");
}

TEST_F(LLMDeploymentPluginTest, PluginInitialization) {
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = test_dir_;
    config.enable_cache = true;
    config.enable_audit_log = false;

    LLMDeploymentPlugin plugin(config);

    EXPECT_TRUE(fs::exists(test_dir_));
    
    const auto& loaded_config = plugin.getConfig();
    EXPECT_EQ(loaded_config.mode, DeploymentMode::OFFLINE);
    EXPECT_EQ(loaded_config.cache_directory, test_dir_);
}

// ============================================================================
// Model Downloader Tests
// ============================================================================

// Minimum valid model size (models must be at least 1MB)
constexpr size_t MIN_VALID_MODEL_SIZE_BYTES = 1024 * 1024;

TEST(ModelDownloaderTest, IsModelAvailable) {
    std::string test_file = "/tmp/test_model.gguf";
    
    // Create test file larger than minimum size
    {
        std::ofstream file(test_file, std::ios::binary);
        std::vector<char> data(2 * MIN_VALID_MODEL_SIZE_BYTES, 'x');
        file.write(data.data(), data.size());
    }
    
    EXPECT_TRUE(ModelDownloader::isModelAvailable(test_file));
    
    // Clean up
    fs::remove(test_file);
    
    EXPECT_FALSE(ModelDownloader::isModelAvailable(test_file));
}

TEST(ModelDownloaderTest, SmallFileNotAvailable) {
    std::string test_file = "/tmp/small_model.gguf";
    
    // Create small file (< 1MB)
    {
        std::ofstream file(test_file);
        file << "small file";
    }
    
    EXPECT_FALSE(ModelDownloader::isModelAvailable(test_file));
    
    // Clean up
    fs::remove(test_file);
}

// ============================================================================
// Model Status Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, GetModelStatus) {
    DeploymentConfig config;
    config.cache_directory = test_dir_;
    config.enable_audit_log = false;
    
    LLMDeploymentPlugin plugin(config);
    
    auto status = plugin.getModelStatus("non-existent-model");
    EXPECT_FALSE(status.has_value());
}

TEST_F(LLMDeploymentPluginTest, ListCachedModels) {
    DeploymentConfig config;
    config.cache_directory = test_dir_;
    config.enable_audit_log = false;
    
    LLMDeploymentPlugin plugin(config);
    
    auto models = plugin.listCachedModels();
    EXPECT_TRUE(models.empty());
}

// ============================================================================
// Cache Management Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, GetCacheSize) {
    DeploymentConfig config;
    config.cache_directory = test_dir_;
    config.enable_audit_log = false;
    
    LLMDeploymentPlugin plugin(config);
    
    size_t cache_size = plugin.getCacheSize();
    EXPECT_EQ(cache_size, 0);
}

TEST_F(LLMDeploymentPluginTest, GetCacheStats) {
    DeploymentConfig config;
    config.cache_directory = test_dir_;
    config.max_cache_size_gb = 75;
    config.enable_audit_log = false;
    
    LLMDeploymentPlugin plugin(config);
    
    auto stats = plugin.getCacheStats();
    
    EXPECT_TRUE(stats.contains("total_models"));
    EXPECT_TRUE(stats.contains("total_size_bytes"));
    EXPECT_TRUE(stats.contains("cache_directory"));
    EXPECT_TRUE(stats.contains("max_cache_size_gb"));
    EXPECT_TRUE(stats.contains("loaded_models"));
    
    EXPECT_EQ(stats["total_models"], 0);
    EXPECT_EQ(stats["total_size_bytes"], 0);
    EXPECT_EQ(stats["cache_directory"], test_dir_);
    EXPECT_EQ(stats["max_cache_size_gb"], 75);
}

// ============================================================================
// Audit Log Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, AuditLogDisabled) {
    DeploymentConfig config;
    config.cache_directory = test_dir_;
    config.enable_audit_log = false;
    
    LLMDeploymentPlugin plugin(config);
    
    auto log_entries = plugin.getAuditLog();
    EXPECT_TRUE(log_entries.empty());
}

// ============================================================================
// Deployment Mode Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, OfflineModeRejectsDownload) {
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = test_dir_;
    config.enable_audit_log = true;
    config.audit_log_path = test_dir_ + "/audit.log";
    
    LLMDeploymentPlugin plugin(config);
    
    // Try to deploy a non-existent model in OFFLINE mode
    auto result = plugin.deployModel("non-existent-model");
    
    EXPECT_FALSE(result.has_value());
    
    // Check audit log
    auto audit = plugin.getAuditLog();
    EXPECT_FALSE(audit.empty());
    EXPECT_FALSE(audit.back().success);
}

// ============================================================================
// YAML Configuration Loader Tests
// ============================================================================

TEST(ModelDownloaderTest, LoadModelConfigFromYAML) {
    std::string config_path = "/tmp/test_models_config.yaml";
    std::ofstream config_file(config_path);
    config_file << R"(
ollama_url: http://test-server:11434
download_dir: /tmp/models
models:
  llama2:7b:
    use_cache: true
    timeout_seconds: 300
)";
    config_file.close();

    auto config = loadModelConfigFromYAML(config_path, "llama2:7b");
    
    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->model_name, "llama2:7b");
    EXPECT_EQ(config->ollama_url, "http://test-server:11434");
    EXPECT_EQ(config->download_dir, "/tmp/models");
    EXPECT_TRUE(config->use_cache);
    EXPECT_EQ(config->timeout_seconds, 300);
    
    // Clean up
    fs::remove(config_path);
}

TEST(ModelDownloaderTest, LoadModelConfigNotFound) {
    std::string config_path = "/tmp/test_models_config2.yaml";
    std::ofstream config_file(config_path);
    config_file << R"(
models:
  llama2:7b:
    use_cache: true
)";
    config_file.close();

    auto config = loadModelConfigFromYAML(config_path, "non-existent-model");
    
    EXPECT_FALSE(config.has_value());
    
    // Clean up
    fs::remove(config_path);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
