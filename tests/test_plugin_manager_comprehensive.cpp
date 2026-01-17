/**
 * @file test_plugin_manager_comprehensive.cpp
 * @brief Comprehensive unit tests for Plugin Manager
 * 
 * Tests plugin loading, unloading, lifecycle management, security verification,
 * manifest parsing, hot-reload, and thread safety.
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

using namespace themis::plugins;
using json = nlohmann::json;
namespace fs = std::filesystem;

class PluginManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary plugin directory
        test_plugin_dir_ = "/tmp/themis_plugin_test";
        fs::create_directories(test_plugin_dir_);
        
        manager_ = std::make_unique<PluginManager>();
    }
    
    void TearDown() override {
        // Clean up test directory
        if (fs::exists(test_plugin_dir_)) {
            fs::remove_all(test_plugin_dir_);
        }
    }
    
    void createMockManifest(const std::string& plugin_name, PluginType type) {
        json manifest;
        manifest["name"] = plugin_name;
        manifest["version"] = "1.0.0";
        manifest["type"] = static_cast<int>(type);
        manifest["author"] = "Test Author";
        manifest["description"] = "Test plugin";
        manifest["library"] = plugin_name + ".so";
        
        std::string manifest_path = test_plugin_dir_ + "/" + plugin_name + ".json";
        std::ofstream file(manifest_path);
        file << manifest.dump(2);
    }
    
    std::string test_plugin_dir_;
    std::unique_ptr<PluginManager> manager_;
};

// ============================================================================
// Construction and Basic Interface Tests
// ============================================================================

TEST_F(PluginManagerTest, ConstructorTest) {
    EXPECT_NO_THROW({
        PluginManager manager;
    });
}

TEST_F(PluginManagerTest, ManagerInitialization) {
    EXPECT_NE(manager_, nullptr);
}

// ============================================================================
// Plugin Directory Scanning Tests
// ============================================================================

TEST_F(PluginManagerTest, ScanEmptyDirectory) {
    size_t count = manager_->scanPluginDirectory(test_plugin_dir_);
    EXPECT_EQ(count, 0);
}

TEST_F(PluginManagerTest, ScanNonexistentDirectory) {
    EXPECT_NO_THROW({
        size_t count = manager_->scanPluginDirectory("/nonexistent/directory");
        EXPECT_EQ(count, 0);
    });
}

TEST_F(PluginManagerTest, ScanDirectoryWithManifests) {
    createMockManifest("plugin1", PluginType::COMPUTE_BACKEND);
    createMockManifest("plugin2", PluginType::CONTENT_PROCESSOR);
    
    size_t count = manager_->scanPluginDirectory(test_plugin_dir_);
    EXPECT_EQ(count, 2);
}

TEST_F(PluginManagerTest, ScanDirectoryMultipleTimes) {
    createMockManifest("plugin1", PluginType::COMPUTE_BACKEND);
    
    size_t count1 = manager_->scanPluginDirectory(test_plugin_dir_);
    size_t count2 = manager_->scanPluginDirectory(test_plugin_dir_);
    
    EXPECT_EQ(count1, count2);
}

// ============================================================================
// Plugin Loading Tests
// ============================================================================

TEST_F(PluginManagerTest, LoadNonexistentPlugin) {
    IThemisPlugin* plugin = manager_->loadPlugin("nonexistent_plugin");
    EXPECT_EQ(plugin, nullptr);
}

TEST_F(PluginManagerTest, LoadPluginWithoutScanning) {
    // Try to load plugin without scanning directory first
    IThemisPlugin* plugin = manager_->loadPlugin("some_plugin");
    EXPECT_EQ(plugin, nullptr);
}

TEST_F(PluginManagerTest, LoadPluginFromInvalidPath) {
    EXPECT_NO_THROW({
        IThemisPlugin* plugin = manager_->loadPluginFromPath("/nonexistent/plugin.so");
        EXPECT_EQ(plugin, nullptr);
    });
}

TEST_F(PluginManagerTest, LoadPluginWithEmptyConfig) {
    EXPECT_NO_THROW({
        IThemisPlugin* plugin = manager_->loadPluginFromPath("/tmp/test.so", "{}");
        // Expected to fail without actual library
    });
}

// ============================================================================
// Plugin Unloading Tests
// ============================================================================

TEST_F(PluginManagerTest, UnloadNonexistentPlugin) {
    EXPECT_NO_THROW({
        bool result = manager_->unloadPlugin("nonexistent");
        EXPECT_FALSE(result);
    });
}

TEST_F(PluginManagerTest, UnloadAllWithoutPlugins) {
    EXPECT_NO_THROW({
        manager_->unloadAll();
    });
}

// ============================================================================
// Plugin Query Tests
// ============================================================================

TEST_F(PluginManagerTest, IsLoadedNonexistent) {
    bool loaded = manager_->isLoaded("nonexistent_plugin");
    EXPECT_FALSE(loaded);
}

TEST_F(PluginManagerTest, GetPluginInfoNonexistent) {
    EXPECT_NO_THROW({
        auto info = manager_->getPluginInfo("nonexistent");
        EXPECT_FALSE(info.has_value());
    });
}

TEST_F(PluginManagerTest, GetPluginsByTypeEmpty) {
    auto plugins = manager_->getPluginsByType(PluginType::COMPUTE_BACKEND);
    EXPECT_TRUE(plugins.empty());
}

TEST_F(PluginManagerTest, GetAllPluginsEmpty) {
    auto plugins = manager_->getAllPlugins();
    EXPECT_TRUE(plugins.empty());
}

TEST_F(PluginManagerTest, GetLoadedPluginsEmpty) {
    auto plugins = manager_->getLoadedPlugins();
    EXPECT_TRUE(plugins.empty());
}

// ============================================================================
// Plugin Lifecycle Tests
// ============================================================================

TEST_F(PluginManagerTest, ReloadNonexistentPlugin) {
    EXPECT_NO_THROW({
        bool result = manager_->reloadPlugin("nonexistent");
        EXPECT_FALSE(result);
    });
}

TEST_F(PluginManagerTest, EnableDisablePlugin) {
    createMockManifest("test_plugin", PluginType::COMPUTE_BACKEND);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    EXPECT_NO_THROW({
        manager_->disablePlugin("test_plugin");
        manager_->enablePlugin("test_plugin");
    });
}

// ============================================================================
// Manifest Parsing Tests
// ============================================================================

TEST_F(PluginManagerTest, ParseValidManifest) {
    createMockManifest("valid_plugin", PluginType::CONTENT_PROCESSOR);
    
    size_t count = manager_->scanPluginDirectory(test_plugin_dir_);
    EXPECT_EQ(count, 1);
    
    auto info = manager_->getPluginInfo("valid_plugin");
    EXPECT_TRUE(info.has_value());
}

TEST_F(PluginManagerTest, ParseInvalidManifest) {
    std::string manifest_path = test_plugin_dir_ + "/invalid.json";
    std::ofstream file(manifest_path);
    file << "{ invalid json";
    file.close();
    
    EXPECT_NO_THROW({
        size_t count = manager_->scanPluginDirectory(test_plugin_dir_);
        // Should handle invalid JSON gracefully
    });
}

TEST_F(PluginManagerTest, ParseEmptyManifest) {
    std::string manifest_path = test_plugin_dir_ + "/empty.json";
    std::ofstream file(manifest_path);
    file << "{}";
    file.close();
    
    EXPECT_NO_THROW({
        manager_->scanPluginDirectory(test_plugin_dir_);
    });
}

// ============================================================================
// Plugin Type Tests
// ============================================================================

TEST_F(PluginManagerTest, MultiplePluginTypes) {
    createMockManifest("compute1", PluginType::COMPUTE_BACKEND);
    createMockManifest("compute2", PluginType::COMPUTE_BACKEND);
    createMockManifest("content1", PluginType::CONTENT_PROCESSOR);
    createMockManifest("storage1", PluginType::STORAGE_BACKEND);
    
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    auto compute_plugins = manager_->getPluginsByType(PluginType::COMPUTE_BACKEND);
    EXPECT_EQ(compute_plugins.size(), 2);
    
    auto content_plugins = manager_->getPluginsByType(PluginType::CONTENT_PROCESSOR);
    EXPECT_EQ(content_plugins.size(), 1);
    
    auto storage_plugins = manager_->getPluginsByType(PluginType::STORAGE_BACKEND);
    EXPECT_EQ(storage_plugins.size(), 1);
}

// ============================================================================
// Security Verification Tests
// ============================================================================

TEST_F(PluginManagerTest, SecurityVerificationInterface) {
    // Test that security verification is called (won't pass without real plugin)
    EXPECT_NO_THROW({
        manager_->loadPluginFromPath("/tmp/test_plugin.so");
    });
}

TEST_F(PluginManagerTest, LoadUnsignedPlugin) {
    // In debug mode, unsigned plugins should be allowed
    // In release mode, they should be rejected
    EXPECT_NO_THROW({
        manager_->loadPluginFromPath("/tmp/unsigned.so");
    });
}

// ============================================================================
// Dependency Resolution Tests
// ============================================================================

TEST_F(PluginManagerTest, LoadPluginWithDependencies) {
    json manifest1;
    manifest1["name"] = "dependent_plugin";
    manifest1["version"] = "1.0.0";
    manifest1["type"] = static_cast<int>(PluginType::COMPUTE_BACKEND);
    manifest1["dependencies"] = json::array({"base_plugin"});
    
    std::ofstream file1(test_plugin_dir_ + "/dependent_plugin.json");
    file1 << manifest1.dump(2);
    file1.close();
    
    createMockManifest("base_plugin", PluginType::COMPUTE_BACKEND);
    
    EXPECT_NO_THROW({
        manager_->scanPluginDirectory(test_plugin_dir_);
    });
}

// ============================================================================
// Hot Reload Tests
// ============================================================================

TEST_F(PluginManagerTest, HotReloadInterface) {
    createMockManifest("hot_reload_plugin", PluginType::COMPUTE_BACKEND);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    EXPECT_NO_THROW({
        bool result = manager_->reloadPlugin("hot_reload_plugin");
        // Expected to fail without actual library
    });
}

TEST_F(PluginManagerTest, HotReloadNonLoaded) {
    createMockManifest("unloaded_plugin", PluginType::COMPUTE_BACKEND);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    bool result = manager_->reloadPlugin("unloaded_plugin");
    EXPECT_FALSE(result);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginConfigurationJSON) {
    json config;
    config["setting1"] = "value1";
    config["setting2"] = 42;
    config["setting3"] = true;
    
    EXPECT_NO_THROW({
        manager_->loadPluginFromPath("/tmp/test.so", config.dump());
    });
}

TEST_F(PluginManagerTest, PluginConfigurationEmpty) {
    EXPECT_NO_THROW({
        manager_->loadPluginFromPath("/tmp/test.so", "");
    });
}

TEST_F(PluginManagerTest, PluginConfigurationInvalid) {
    EXPECT_NO_THROW({
        manager_->loadPluginFromPath("/tmp/test.so", "invalid json");
    });
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(PluginManagerTest, ConcurrentScanDirectory) {
    createMockManifest("plugin1", PluginType::COMPUTE_BACKEND);
    createMockManifest("plugin2", PluginType::CONTENT_PROCESSOR);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this]() {
            manager_->scanPluginDirectory(test_plugin_dir_);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should complete without crashes
    SUCCEED();
}

TEST_F(PluginManagerTest, ConcurrentQueries) {
    createMockManifest("test_plugin", PluginType::COMPUTE_BACKEND);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([this]() {
            auto plugins = manager_->getAllPlugins();
            auto info = manager_->getPluginInfo("test_plugin");
            bool loaded = manager_->isLoaded("test_plugin");
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    SUCCEED();
}

TEST_F(PluginManagerTest, ConcurrentLoadUnload) {
    createMockManifest("concurrent_plugin", PluginType::COMPUTE_BACKEND);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([this]() {
            manager_->loadPlugin("concurrent_plugin");
            manager_->unloadPlugin("concurrent_plugin");
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    SUCCEED();
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(PluginManagerTest, LoadPluginWithMissingLibrary) {
    json manifest;
    manifest["name"] = "missing_lib";
    manifest["version"] = "1.0.0";
    manifest["type"] = static_cast<int>(PluginType::COMPUTE_BACKEND);
    manifest["library"] = "nonexistent.so";
    
    std::ofstream file(test_plugin_dir_ + "/missing_lib.json");
    file << manifest.dump(2);
    file.close();
    
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    EXPECT_NO_THROW({
        IThemisPlugin* plugin = manager_->loadPlugin("missing_lib");
        EXPECT_EQ(plugin, nullptr);
    });
}

TEST_F(PluginManagerTest, GetStatistics) {
    createMockManifest("plugin1", PluginType::COMPUTE_BACKEND);
    createMockManifest("plugin2", PluginType::CONTENT_PROCESSOR);
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    EXPECT_NO_THROW({
        auto stats = manager_->getStatistics();
        EXPECT_TRUE(stats.is_object());
    });
}

// ============================================================================
// Plugin Metadata Tests
// ============================================================================

TEST_F(PluginManagerTest, PluginVersion) {
    json manifest;
    manifest["name"] = "version_test";
    manifest["version"] = "2.5.1";
    manifest["type"] = static_cast<int>(PluginType::COMPUTE_BACKEND);
    
    std::ofstream file(test_plugin_dir_ + "/version_test.json");
    file << manifest.dump(2);
    file.close();
    
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    auto info = manager_->getPluginInfo("version_test");
    EXPECT_TRUE(info.has_value());
}

TEST_F(PluginManagerTest, PluginAuthorAndDescription) {
    json manifest;
    manifest["name"] = "meta_test";
    manifest["version"] = "1.0.0";
    manifest["type"] = static_cast<int>(PluginType::COMPUTE_BACKEND);
    manifest["author"] = "ThemisDB Team";
    manifest["description"] = "Test plugin for metadata";
    
    std::ofstream file(test_plugin_dir_ + "/meta_test.json");
    file << manifest.dump(2);
    file.close();
    
    manager_->scanPluginDirectory(test_plugin_dir_);
    
    auto info = manager_->getPluginInfo("meta_test");
    EXPECT_TRUE(info.has_value());
}

// ============================================================================
// Cleanup Tests
// ============================================================================

TEST_F(PluginManagerTest, DestructorCleansUp) {
    {
        PluginManager temp_manager;
        // Load some plugins
        createMockManifest("cleanup_test", PluginType::COMPUTE_BACKEND);
        temp_manager.scanPluginDirectory(test_plugin_dir_);
    }
    // Destructor should clean up without crashing
    SUCCEED();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
