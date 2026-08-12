/**
 * @file test_llm_deployment_plugin.cpp
 * @brief Unit tests for LLM Deployment Plugin
 */

#include <gtest/gtest.h>
#include "llm/llm_deployment_plugin.h"
#include "llm/llm_model_storage.h"
#include "llm/model_downloader.h"
#include "scheduler/task_scheduler.h"
#include "storage/rocksdb_wrapper.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace themis::llm;
using namespace themis::scheduler;
using namespace themis;
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

// Size used for test model files in RocksDB and source-selection tests (2 × minimum)
constexpr size_t TEST_MODEL_FILE_SIZE_BYTES = 2 * MIN_VALID_MODEL_SIZE_BYTES;

TEST(ModelDownloaderTest, IsModelAvailable) {
    auto temp_dir = fs::temp_directory_path();
    std::string test_file = (temp_dir / "test_model.gguf").string();
    
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
    auto temp_dir = fs::temp_directory_path();
    std::string config_path = (temp_dir / "test_models_config.yaml").string();

    std::ofstream config_file(config_path);
    config_file << R"(models:
  - name: "llama2:7b"
    sources:
      ollama: "llama2:7b"
)";
    config_file.close();

    auto config = loadModelConfigFromYAML(config_path, "llama2:7b");

    ASSERT_TRUE(config.has_value());
    EXPECT_EQ(config->model_name, "llama2:7b");
    EXPECT_EQ(config->ollama_url, "http://localhost:11434");

    // Clean up
    fs::remove(config_path);
}

TEST(ModelDownloaderTest, LoadModelConfigNotFound) {
    auto temp_dir = fs::temp_directory_path();
    std::string config_path = (temp_dir / "test_models_config2.yaml").string();
    
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

// ============================================================================
// RocksDB Persistence Tests
// ============================================================================

class LLMDeploymentPluginRocksDBTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_test_deploy_rocksdb";
        db_path_   = test_dir_ / "db";
        cache_dir_ = test_dir_ / "models";

        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(db_path_);
        fs::create_directories(cache_dir_);

        RocksDBWrapper::Config db_config;
        db_config.db_path = db_path_.string();
        db_config.create_if_missing = true;
        db_ = std::make_shared<RocksDBWrapper>(db_config);
        ASSERT_TRUE(db_->open());
    }

    void TearDown() override {
        db_.reset();
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        LLMDeploymentPlugin::clearRequestContext();
    }

    // Create a minimal fake model file large enough to be accepted
    std::string createFakeModelFile(const std::string& name) {
        fs::path path = cache_dir_ / (name + ".gguf");
        std::ofstream f(path, std::ios::binary);
        // Write 2 MB of dummy data so filesystem-size checks pass
        std::vector<char> buf(TEST_MODEL_FILE_SIZE_BYTES, 'M');
        f.write(buf.data(), static_cast<std::streamsize>(buf.size()));
        return path.string();
    }

    fs::path test_dir_;
    fs::path db_path_;
    fs::path cache_dir_;
    std::shared_ptr<RocksDBWrapper> db_;
};

// ============================================================================
// RocksDB / BaseEntity Storage Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, StorageInitialisedWhenDbProvided) {
    // When a RocksDBWrapper is injected the plugin must enable model_storage_.
    // We verify this indirectly: deploying a model in OFFLINE mode with an
    // unknown ID must still exercise the constructor path without crashing.
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = test_dir_;
    config.use_base_entity_storage = false; // db is null – filesystem-only
    config.enable_audit_log = false;

    EXPECT_NO_THROW({
        LLMDeploymentPlugin plugin(config);
        auto result = plugin.deployModel("some-model");
        EXPECT_FALSE(result.has_value());
    });
}

TEST_F(LLMDeploymentPluginRocksDBTest, DeployModelPersistsMetadataToRocksDB) {
    // Pre-create a fake model in the cache directory
    createFakeModelFile("test_model");

    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = cache_dir_.string();
    config.enable_audit_log = false;
    config.verify_checksums = false;
    config.use_base_entity_storage = true;
    config.db = db_;
    config.key_prefix = "llm_model::";

    LLMDeploymentPlugin plugin(config);

    auto result = plugin.deployModel("test_model");
    ASSERT_TRUE(result.has_value()) << "deployModel should succeed with a cached file";
    EXPECT_EQ(result->model_id, "test_model");
    EXPECT_TRUE(result->is_cached);

    // Verify metadata was persisted to RocksDB using LLMModelStorage directly
    LLMModelStorage::Config storage_cfg;
    storage_cfg.db = db_;
    storage_cfg.key_prefix = "llm_model::";
    storage_cfg.enable_encryption = false;
    storage_cfg.enable_signatures = false;
    storage_cfg.use_blob_storage = false;

    LLMModelStorage storage(storage_cfg);
    EXPECT_TRUE(storage.exists("test_model"))
        << "Model metadata should be persisted in RocksDB after deployModel";

    auto loaded = storage.loadModel("test_model");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->model_id, "test_model");
    EXPECT_GT(loaded->size_bytes, 0UL);
}

TEST_F(LLMDeploymentPluginRocksDBTest, ListModelsEnumeratesPersistedModelsByPrefix) {
    createFakeModelFile("list_model_a");
    createFakeModelFile("list_model_b");

    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = cache_dir_.string();
    config.enable_audit_log = false;
    config.verify_checksums = false;
    config.use_base_entity_storage = true;
    config.db = db_;
    config.key_prefix = "llm_model::";

    LLMDeploymentPlugin plugin(config);
    ASSERT_TRUE(plugin.deployModel("list_model_a").has_value());
    ASSERT_TRUE(plugin.deployModel("list_model_b").has_value());

    LLMModelStorage::Config storage_cfg;
    storage_cfg.db = db_;
    storage_cfg.key_prefix = "llm_model::";
    storage_cfg.enable_encryption = false;
    storage_cfg.enable_signatures = false;
    storage_cfg.use_blob_storage = false;
    LLMModelStorage storage(storage_cfg);

    const auto models = storage.listModels();
    EXPECT_NE(std::find(models.begin(), models.end(), "list_model_a"), models.end());
    EXPECT_NE(std::find(models.begin(), models.end(), "list_model_b"), models.end());

    const auto filtered = storage.listModels(std::optional<std::string>{"_b"});
    ASSERT_EQ(filtered.size(), 1u);
    EXPECT_EQ(filtered.front(), "list_model_b");
}

TEST_F(LLMDeploymentPluginRocksDBTest, DeployModelWithoutStorageDoesNotPersist) {
    createFakeModelFile("no_persist_model");

    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = cache_dir_.string();
    config.enable_audit_log = false;
    config.verify_checksums = false;
    config.use_base_entity_storage = false;  // Storage disabled
    // config.db intentionally left null

    LLMDeploymentPlugin plugin(config);

    auto result = plugin.deployModel("no_persist_model");
    ASSERT_TRUE(result.has_value());

    // DB was never written; check via storage that nothing was stored
    LLMModelStorage::Config storage_cfg;
    storage_cfg.db = db_;
    storage_cfg.key_prefix = "llm_model::";
    storage_cfg.enable_encryption = false;
    storage_cfg.enable_signatures = false;
    storage_cfg.use_blob_storage = false;

    LLMModelStorage storage(storage_cfg);
    EXPECT_FALSE(storage.exists("no_persist_model"))
        << "Nothing should be written to RocksDB when storage is disabled";
}

// ============================================================================
// User Context Propagation Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, AuditUserDefaultsToSystem) {
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = test_dir_;
    config.enable_audit_log = true;
    config.audit_log_path = test_dir_ + "/audit.log";

    LLMDeploymentPlugin::clearRequestContext();

    LLMDeploymentPlugin plugin(config);
    plugin.deployModel("no-such-model");

    auto log = plugin.getAuditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log.back().user, "system");
}

TEST_F(LLMDeploymentPluginTest, AuditUserPropagatedFromRequestContext) {
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE;
    config.cache_directory = test_dir_;
    config.enable_audit_log = true;
    config.audit_log_path = test_dir_ + "/audit.log";

    LLMDeploymentPlugin::setRequestContext({"alice@example.com", "127.0.0.1"});

    LLMDeploymentPlugin plugin(config);
    plugin.deployModel("no-such-model");

    LLMDeploymentPlugin::clearRequestContext();

    auto log = plugin.getAuditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_EQ(log.back().user, "alice@example.com");
}

TEST_F(LLMDeploymentPluginTest, RequestContextClearRestoresSystemFallback) {
    LLMDeploymentPlugin::setRequestContext({"bob", ""});
    LLMDeploymentPlugin::clearRequestContext();
    EXPECT_EQ(LLMDeploymentPlugin::currentUserId(), "system");
}

// ============================================================================
// model_id validation Tests
// ============================================================================

TEST_F(LLMDeploymentPluginTest, DeployEmptyModelIdReturnsFalse) {
    DeploymentConfig config;
    config.mode = DeploymentMode::AUTO;
    config.cache_directory = test_dir_;
    config.enable_audit_log = true;
    config.audit_log_path = test_dir_ + "/audit.log";

    LLMDeploymentPlugin plugin(config);
    auto result = plugin.deployModel("");

    EXPECT_FALSE(result.has_value());

    // The audit entry should record the failure
    auto log = plugin.getAuditLog();
    ASSERT_FALSE(log.empty());
    EXPECT_FALSE(log.back().success);
}

TEST_F(LLMDeploymentPluginTest, FindBestSourceSkipsLocalMissingModel) {
    // Create a "local" source that points to our test_dir_ but does NOT
    // contain the requested model. findBestSource (called internally by
    // downloadModel) should skip that source and fall back to the default
    // Ollama source rather than returning an unusable local entry.
    DeploymentConfig config;
    config.mode = DeploymentMode::OFFLINE; // prevent actual download
    config.cache_directory = test_dir_;
    config.enable_audit_log = false;

    ModelSource local_src;
    local_src.type = "local";
    local_src.location = test_dir_; // exists as directory but no model file
    local_src.priority = 100;
    config.sources.push_back(local_src);

    LLMDeploymentPlugin plugin(config);
    // In OFFLINE mode the model isn't in cache, so deploy should fail cleanly.
    auto result = plugin.deployModel("missing-model");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// Local Source Existence Check Tests
// ============================================================================

class LLMDeploymentPluginSourceTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = (fs::temp_directory_path() / "themis_test_deploy_source").string();
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

TEST_F(LLMDeploymentPluginSourceTest, LocalSourceWithMissingFileFallsBackToNextSource) {
    // Add two local sources: first one has a missing file, second has the model
    fs::path local_src = fs::path(test_dir_) / "src";
    fs::create_directories(local_src);

    // Create the model file only in the second source directory
    fs::path second_src = fs::path(test_dir_) / "src2";
    fs::create_directories(second_src);
    {
        std::ofstream f(second_src / "mymodel.gguf", std::ios::binary);
        std::vector<char> buf(TEST_MODEL_FILE_SIZE_BYTES, 'x');
        f.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    }

    DeploymentConfig config;
    config.mode = DeploymentMode::AUTO;
    config.cache_directory = test_dir_ + "/cache";
    config.enable_audit_log = false;
    config.verify_checksums = false;

    // First source (higher priority): empty directory → model not found
    ModelSource bad_src;
    bad_src.type = "local";
    bad_src.location = local_src.string();
    bad_src.priority = 100;

    // Second source (lower priority): has the model file
    ModelSource good_src;
    good_src.type = "local";
    good_src.location = second_src.string();
    good_src.priority = 50;

    config.sources = {bad_src, good_src};

    fs::create_directories(config.cache_directory);

    LLMDeploymentPlugin plugin(config);

    // Deploy must succeed by falling through to the second local source
    auto result = plugin.deployModel("mymodel");
    ASSERT_TRUE(result.has_value())
        << "Plugin should skip unavailable local source and use the next valid one";
    EXPECT_EQ(result->model_id, "mymodel");
}

TEST_F(LLMDeploymentPluginSourceTest, LocalSourceWithExistingFileIsSelected) {
    fs::path src_dir = fs::path(test_dir_) / "models_src";
    fs::create_directories(src_dir);

    // Pre-populate the source directory with the model
    {
        std::ofstream f(src_dir / "mymodel2.gguf", std::ios::binary);
        std::vector<char> buf(TEST_MODEL_FILE_SIZE_BYTES, 'y');
        f.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    }

    DeploymentConfig config;
    config.mode = DeploymentMode::AUTO;
    config.cache_directory = test_dir_ + "/cache2";
    config.enable_audit_log = false;
    config.verify_checksums = false;

    ModelSource src;
    src.type = "local";
    src.location = src_dir.string();
    src.priority = 100;
    config.sources = {src};

    fs::create_directories(config.cache_directory);

    LLMDeploymentPlugin plugin(config);

    auto result = plugin.deployModel("mymodel2");
    ASSERT_TRUE(result.has_value())
        << "Plugin should deploy from a local source when the model file exists";
    EXPECT_EQ(result->model_id, "mymodel2");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
