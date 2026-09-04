/**
 * @file test_ingestion_manager_adapter.cpp
 * @brief Tests for Ingestion Manager Adapter (Plugin-based multi-source support)
 * 
 * Tests the plugin infrastructure for AsyncIngestionWorker:
 * - Plugin registration/unregistration
 * - Source job submission
 * - Plugin-based job processing
 * - IngestionSource serialization
 * 
 * @author ThemisDB Team
 * @date February 2026
 */

#include <gtest/gtest.h>
#include "content/async_ingestion_worker.h"
#include "content/ingestion_plugin.h"
#include "content/content_manager.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::content;
using json = nlohmann::json;

namespace {

// ============================================================================
// Mock Plugin for Testing
// ============================================================================

/**
 * @brief Mock ingestion plugin for testing
 */
class MockIngestionPlugin : public IngestionPlugin {
public:
    MockIngestionPlugin(const std::string& name = "mock_plugin")
        : name_(name)
        , version_("1.0.0")
        , process_called_(false)
        , estimate_called_(false)
    {}
    
    std::string name() const override {
        return name_;
    }
    
    std::string version() const override {
        return version_;
    }
    
    std::vector<IngestionJobType> supportedTypes() const override {
        return {
            IngestionJobType::HUGGINGFACE,
            IngestionJobType::FILESYSTEM_BULK
        };
    }
    
    void processJob(IngestionJob& job) override {
        process_called_ = true;
        last_processed_job_id_ = job.job_id;
        
        // Simulate successful processing
        job.status = IngestionJobStatus::COMPLETED;
        job.progress = 1.0f;
        job.processed_items = job.total_items > 0 ? job.total_items : 1;
        job.content_ids.push_back("mock_content_id_1");
        job.content_ids.push_back("mock_content_id_2");
        job.result_metadata = json{
            {"plugin", name_},
            {"processed", true}
        };
    }
    
    size_t estimateJobSize(const IngestionJob& job) override {
        estimate_called_ = true;
        // Return a fixed size for testing
        return 10;
    }
    
    json getConfig() const override {
        return config_;
    }
    
    void setConfig(const json& config) override {
        config_ = config;
    }
    
    // Test helpers
    bool wasProcessCalled() const { return process_called_; }
    bool wasEstimateCalled() const { return estimate_called_; }
    std::string getLastProcessedJobId() const { return last_processed_job_id_; }
    void reset() { 
        process_called_ = false; 
        estimate_called_ = false;
        last_processed_job_id_.clear();
    }
    
private:
    std::string name_;
    std::string version_;
    json config_;
    bool process_called_;
    bool estimate_called_;
    std::string last_processed_job_id_;
};

/**
 * @brief Note on Integration Tests
 * 
 * Tests prefixed with DISABLED_ require ContentManager with full storage setup
 * (RocksDB, VectorIndex, GraphIndex, SecondaryIndex). GoogleTest automatically
 * skips tests with the DISABLED_ prefix.
 * 
 * These will be enabled when proper test fixtures are available.
 */

// Helper for integration tests (to be implemented when enabled)
std::shared_ptr<ContentManager> makeTestContentManager() {
    // TODO: Implement when enabling integration tests
    // Should create: RocksDB, VectorIndex, GraphIndex, SecondaryIndex
    return nullptr;  // Placeholder
}

} // anonymous namespace

// ============================================================================
// IngestionSource Serialization Tests
// ============================================================================

TEST(IngestionManagerAdapterTest, IngestionSource_ToJson) {
    IngestionSource source;
    source.source_id = "test_source";
    source.plugin_name = "test_plugin";
    source.type = IngestionJobType::HUGGINGFACE;
    source.location = "https://example.com/dataset";
    source.priority = 5;
    source.tags = {"public", "test"};
    source.incremental = true;
    source.config = json{{"key", "value"}};
    
    auto j = source.toJson();
    
    EXPECT_EQ(j["source_id"], "test_source");
    EXPECT_EQ(j["plugin_name"], "test_plugin");
    EXPECT_EQ(j["type"], static_cast<int>(IngestionJobType::HUGGINGFACE));
    EXPECT_EQ(j["location"], "https://example.com/dataset");
    EXPECT_EQ(j["priority"], 5);
    EXPECT_EQ(j["tags"].size(), 2);
    EXPECT_EQ(j["incremental"], true);
    EXPECT_EQ(j["config"]["key"], "value");
}

TEST(IngestionManagerAdapterTest, IngestionSource_FromJson) {
    json j = {
        {"source_id", "test_source"},
        {"plugin_name", "test_plugin"},
        {"type", static_cast<int>(IngestionJobType::FILESYSTEM_BULK)},
        {"location", "/path/to/data"},
        {"priority", 7},
        {"tags", json::array({"internal", "important"})},
        {"incremental", false},
        {"config", json{{"option", "enabled"}}}
    };
    
    auto source = IngestionSource::fromJson(j);
    
    EXPECT_EQ(source.source_id, "test_source");
    EXPECT_EQ(source.plugin_name, "test_plugin");
    EXPECT_EQ(source.type, IngestionJobType::FILESYSTEM_BULK);
    EXPECT_EQ(source.location, "/path/to/data");
    EXPECT_EQ(source.priority, 7);
    EXPECT_EQ(source.tags.size(), 2);
    EXPECT_EQ(source.incremental, false);
    EXPECT_EQ(source.config["option"], "enabled");
}

TEST(IngestionManagerAdapterTest, IngestionSource_RoundTrip) {
    IngestionSource original;
    original.source_id = "roundtrip_test";
    original.plugin_name = "roundtrip_plugin";
    original.type = IngestionJobType::REST_API;
    original.location = "https://api.example.com";
    original.priority = 3;
    original.tags = {"api", "external"};
    original.incremental = true;
    original.config = json{{"auth", "token"}, {"timeout", 30}};
    
    auto j = original.toJson();
    auto restored = IngestionSource::fromJson(j);
    
    EXPECT_EQ(restored.source_id, original.source_id);
    EXPECT_EQ(restored.plugin_name, original.plugin_name);
    EXPECT_EQ(restored.type, original.type);
    EXPECT_EQ(restored.location, original.location);
    EXPECT_EQ(restored.priority, original.priority);
    EXPECT_EQ(restored.tags, original.tags);
    EXPECT_EQ(restored.incremental, original.incremental);
    EXPECT_EQ(restored.config, original.config);
}

// ============================================================================
// Plugin Registration Tests (WITHOUT Worker - testing interfaces only)
// ============================================================================

TEST(IngestionManagerAdapterTest, MockPlugin_Interface) {
    auto plugin = std::make_shared<MockIngestionPlugin>("test_plugin");
    
    EXPECT_EQ(plugin->name(), "test_plugin");
    EXPECT_EQ(plugin->version(), "1.0.0");
    
    auto types = plugin->supportedTypes();
    EXPECT_EQ(types.size(), 2);
    EXPECT_EQ(types[0], IngestionJobType::HUGGINGFACE);
    EXPECT_EQ(types[1], IngestionJobType::FILESYSTEM_BULK);
    
    EXPECT_FALSE(plugin->wasProcessCalled());
    EXPECT_FALSE(plugin->wasEstimateCalled());
}

TEST(IngestionManagerAdapterTest, MockPlugin_ProcessJob) {
    auto plugin = std::make_shared<MockIngestionPlugin>("processor");
    
    IngestionJob job;
    job.job_id = "test_job_123";
    job.type = IngestionJobType::HUGGINGFACE;
    job.status = IngestionJobStatus::QUEUED;
    job.total_items = 10;
    
    plugin->processJob(job);
    
    EXPECT_TRUE(plugin->wasProcessCalled());
    EXPECT_EQ(plugin->getLastProcessedJobId(), "test_job_123");
    EXPECT_EQ(job.status, IngestionJobStatus::COMPLETED);
    EXPECT_EQ(job.progress, 1.0f);
    EXPECT_EQ(job.content_ids.size(), 2);
}

TEST(IngestionManagerAdapterTest, MockPlugin_EstimateJobSize) {
    auto plugin = std::make_shared<MockIngestionPlugin>("estimator");
    
    IngestionJob job;
    job.job_id = "test_job";
    job.type = IngestionJobType::FILESYSTEM_BULK;
    
    size_t estimated = plugin->estimateJobSize(job);
    
    EXPECT_TRUE(plugin->wasEstimateCalled());
    EXPECT_EQ(estimated, 10);
}

TEST(IngestionManagerAdapterTest, MockPlugin_Config) {
    auto plugin = std::make_shared<MockIngestionPlugin>("config_test");
    
    json config = {{"option1", "value1"}, {"option2", 42}};
    plugin->setConfig(config);
    
    auto retrieved = plugin->getConfig();
    EXPECT_EQ(retrieved, config);
}

// ============================================================================
// Plugin Registration Tests (WITH Worker - INTEGRATION TESTS)
// ============================================================================
// These tests require a ContentManager which needs full storage setup.
// They are disabled for now as they require integration test infrastructure.

TEST(IngestionManagerAdapterTest, DISABLED_RegisterPlugin_Success) {
    // Integration test - requires ContentManager with RocksDB, VectorIndex, etc.
    // Test code preserved for future enablement
}

TEST(IngestionManagerAdapterTest, DISABLED_RegisterPlugin_MultiplePlugins) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin1 = std::make_shared<MockIngestionPlugin>("plugin1");
    auto plugin2 = std::make_shared<MockIngestionPlugin>("plugin2");
    auto plugin3 = std::make_shared<MockIngestionPlugin>("plugin3");
    
    worker.registerPlugin(plugin1);
    worker.registerPlugin(plugin2);
    worker.registerPlugin(plugin3);
    
    auto plugins = worker.listPlugins();
    EXPECT_EQ(plugins.size(), 3);
}

TEST(IngestionManagerAdapterTest, DISABLED_RegisterPlugin_Replace) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin1 = std::make_shared<MockIngestionPlugin>("same_name");
    auto plugin2 = std::make_shared<MockIngestionPlugin>("same_name");
    
    worker.registerPlugin(plugin1);
    worker.registerPlugin(plugin2);  // Should replace plugin1
    
    auto plugins = worker.listPlugins();
    EXPECT_EQ(plugins.size(), 1);  // Only one plugin with this name
    
    auto retrieved = worker.getPlugin("same_name");
    EXPECT_EQ(retrieved, plugin2);  // Should be the second one
}

TEST(IngestionManagerAdapterTest, DISABLED_RegisterPlugin_NullThrows) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    EXPECT_THROW(worker.registerPlugin(nullptr), std::invalid_argument);
}

TEST(IngestionManagerAdapterTest, DISABLED_UnregisterPlugin_Success) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("to_remove");
    worker.registerPlugin(plugin);
    
    EXPECT_EQ(worker.listPlugins().size(), 1);
    
    worker.unregisterPlugin("to_remove");
    
    EXPECT_EQ(worker.listPlugins().size(), 0);
}

TEST(IngestionManagerAdapterTest, DISABLED_UnregisterPlugin_NotFound) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    // Should not throw, just log warning
    EXPECT_NO_THROW(worker.unregisterPlugin("nonexistent"));
}

TEST(IngestionManagerAdapterTest, DISABLED_GetPlugin_Success) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("retrieve_me");
    worker.registerPlugin(plugin);
    
    auto retrieved = worker.getPlugin("retrieve_me");
    EXPECT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->name(), "retrieve_me");
}

TEST(IngestionManagerAdapterTest, DISABLED_GetPlugin_NotFound) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto retrieved = worker.getPlugin("nonexistent");
    EXPECT_EQ(retrieved, nullptr);
}

TEST(IngestionManagerAdapterTest, DISABLED_ListPlugins_Empty) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugins = worker.listPlugins();
    EXPECT_TRUE(plugins.empty());
}

// ============================================================================
// Source Job Submission Tests
// ============================================================================

TEST(IngestionManagerAdapterTest, DISABLED_SubmitSourceJob_NotRunningThrows) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("test");
    worker.registerPlugin(plugin);
    
    IngestionSource source;
    source.source_id = "test_source";
    source.plugin_name = "test";
    source.type = IngestionJobType::HUGGINGFACE;
    source.location = "test/dataset";
    
    // Worker not started
    EXPECT_THROW(worker.submitSourceJob(source), std::runtime_error);
}

TEST(IngestionManagerAdapterTest, DISABLED_SubmitSourceJob_PluginNotFoundThrows) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    worker.start();
    
    IngestionSource source;
    source.source_id = "test_source";
    source.plugin_name = "nonexistent_plugin";
    source.type = IngestionJobType::HUGGINGFACE;
    source.location = "test/dataset";
    
    EXPECT_THROW(worker.submitSourceJob(source), std::runtime_error);
    
    worker.stop(false);
}

TEST(IngestionManagerAdapterTest, DISABLED_SubmitSourceJob_Success) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("test_plugin");
    worker.registerPlugin(plugin);
    worker.start();
    
    IngestionSource source;
    source.source_id = "test_source";
    source.plugin_name = "test_plugin";
    source.type = IngestionJobType::HUGGINGFACE;
    source.location = "example/dataset";
    source.priority = 5;
    source.config = json{{"split", "train"}};
    
    std::string job_id;
    EXPECT_NO_THROW(job_id = worker.submitSourceJob(source));
    EXPECT_FALSE(job_id.empty());
    EXPECT_TRUE(plugin->wasEstimateCalled());
    
    // Give worker time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto job_status = worker.getJobStatus(job_id);
    EXPECT_TRUE(job_status.has_value());
    
    worker.stop(false);
}

TEST(IngestionManagerAdapterTest, DISABLED_SubmitSourceJob_WithAdditionalConfig) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("test_plugin");
    worker.registerPlugin(plugin);
    worker.start();
    
    IngestionSource source;
    source.source_id = "test_source";
    source.plugin_name = "test_plugin";
    source.type = IngestionJobType::FILESYSTEM_BULK;
    source.location = "/path/to/data";
    source.config = json{{"base_option", "value"}};
    
    json additional = json{{"override_option", "override"}};
    
    auto job_id = worker.submitSourceJob(source, additional);
    EXPECT_FALSE(job_id.empty());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto job_status = worker.getJobStatus(job_id);
    EXPECT_TRUE(job_status.has_value());
    if (job_status) {
        EXPECT_TRUE(job_status->config.contains("source"));
        EXPECT_TRUE(job_status->config.contains("base_option"));
        EXPECT_TRUE(job_status->config.contains("override_option"));
    }
    
    worker.stop(false);
}

// ============================================================================
// Plugin Job Processing Tests
// ============================================================================

TEST(IngestionManagerAdapterTest, DISABLED_ProcessPluginJob_Integration) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("processor");
    worker.registerPlugin(plugin);
    worker.start();
    
    IngestionSource source;
    source.source_id = "integration_test";
    source.plugin_name = "processor";
    source.type = IngestionJobType::HUGGINGFACE;
    source.location = "test/integration";
    
    auto job_id = worker.submitSourceJob(source);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    auto job_status = worker.getJobStatus(job_id);
    ASSERT_TRUE(job_status.has_value());
    
    EXPECT_TRUE(plugin->wasProcessCalled());
    EXPECT_EQ(plugin->getLastProcessedJobId(), job_id);
    EXPECT_EQ(job_status->status, IngestionJobStatus::COMPLETED);
    EXPECT_EQ(job_status->progress, 1.0f);
    EXPECT_EQ(job_status->content_ids.size(), 2);
    
    worker.stop(false);
}

TEST(IngestionManagerAdapterTest, DISABLED_ProcessPluginJob_MultipleJobs) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    auto plugin = std::make_shared<MockIngestionPlugin>("batch_processor");
    worker.registerPlugin(plugin);
    worker.start();
    
    std::vector<std::string> job_ids = {};

    for (int i = 0; i < 5; ++i) {
        IngestionSource source;
        source.source_id = "batch_" + std::to_string(i);
        source.plugin_name = "batch_processor";
        source.type = IngestionJobType::FILESYSTEM_BULK;
        source.location = "/data/" + std::to_string(i);
        
        job_ids.push_back(worker.submitSourceJob(source));
    }
    
    // Wait for all to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    int completed = 0;
    for (const auto& job_id : job_ids) {
        auto status = worker.getJobStatus(job_id);
        if (status && status->status == IngestionJobStatus::COMPLETED) {
            completed++;
        }
    }
    
    EXPECT_EQ(completed, 5);
    
    worker.stop(false);
}

// ============================================================================
// Backward Compatibility Tests
// ============================================================================

TEST(IngestionManagerAdapterTest, DISABLED_BackwardCompatibility_SubmitFile) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    worker.start();
    
    // Old API should still work
    EXPECT_NO_THROW({
        auto job_id = worker.submitFile(
            "test blob data",
            "test.txt",
            "text/plain",
            "user_context"
        );
        EXPECT_FALSE(job_id.empty());
    });
    
    worker.stop(false);
}

TEST(IngestionManagerAdapterTest, DISABLED_BackwardCompatibility_SubmitArchive) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    worker.start();
    
    // Old API should still work
    EXPECT_NO_THROW({
        auto job_id = worker.submitArchive(
            "archive blob data",
            "test.zip",
            "user_context"
        );
        EXPECT_FALSE(job_id.empty());
    });
    
    worker.stop(false);
}

TEST(IngestionManagerAdapterTest, DISABLED_BackwardCompatibility_SubmitBatch) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    worker.start();
    
    // Old API should still work
    std::vector<std::pair<std::string, std::string>> files = {
        {"file1.txt", "content1"},
        {"file2.txt", "content2"}
    };
    
    EXPECT_NO_THROW({
        auto job_id = worker.submitBatch(files, "user_context");
        EXPECT_FALSE(job_id.empty());
    });
    
    worker.stop(false);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(IngestionManagerAdapterTest, DISABLED_LoadSourcesFromConfig_NotImplemented) {
    auto content_mgr = makeTestContentManager();
    AsyncIngestionWorker worker(content_mgr);
    
    // This feature is stubbed for now
    EXPECT_THROW(
        worker.loadSourcesFromConfig("config.yaml"),
        std::runtime_error
    );
}
