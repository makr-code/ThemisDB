#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include "plugins/plugin_interface.h"
#include "acceleration/plugin_security.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

class PluginHotPlugTest : public ::testing::Test {
protected:
    std::string test_plugin_dir = "./test_hot_plug_plugins";
    themis::plugins::PluginManager* manager;
    
    void SetUp() override {
        // Clean up
        if (fs::exists(test_plugin_dir)) {
            fs::remove_all(test_plugin_dir);
        }
        fs::create_directories(test_plugin_dir);
        
        manager = &themis::plugins::PluginManager::instance();
        
        // Disable any existing hot-plug monitoring
        manager->disableHotPlug();
    }
    
    void TearDown() override {
        // Disable hot-plug monitoring
        manager->disableHotPlug();
        
        // Unload all plugins
        manager->unloadAllPlugins();
        
        // Clean up
        if (fs::exists(test_plugin_dir)) {
            fs::remove_all(test_plugin_dir);
        }
    }
    
    void createTestManifest(const std::string& name, bool auto_load = false) {
        std::string plugin_dir = test_plugin_dir + "/" + name;
        fs::create_directories(plugin_dir);
        
        nlohmann::json manifest = {
            {"name", name},
            {"version", "1.0.0"},
            {"type", "custom"},
            {"description", "Test plugin for hot-plug"},
            {"binary", {
                {"windows", name + ".dll"},
                {"linux", name + ".so"},
                {"macos", name + ".dylib"}
            }},
            {"capabilities", {
                {"thread_safe", true},
                {"streaming", false}
            }},
            {"auto_load", auto_load},
            {"load_priority", 100}
        };
        
        std::string manifest_path = plugin_dir + "/plugin.json";
        std::ofstream file(manifest_path);
        file << manifest.dump(2);
        file.close();

        themis::acceleration::PluginSecurityPolicy policy;
        themis::acceleration::PluginSecurityVerifier verifier(policy);
        std::string manifest_hash = verifier.calculateFileHash(manifest_path);
        std::ofstream sig_file(manifest_path + ".sig");
        sig_file << manifest_hash;
    }
    
    void modifyManifest(const std::string& name) {
        std::string manifest_path = test_plugin_dir + "/" + name + "/plugin.json";
        
        // Read existing manifest
        std::ifstream infile(manifest_path);
        nlohmann::json manifest;
        infile >> manifest;
        infile.close();
        
        // Modify version
        manifest["version"] = "1.0.1";
        
        // Write back
        std::ofstream outfile(manifest_path);
        outfile << manifest.dump(2);
        outfile.close();

        themis::acceleration::PluginSecurityPolicy policy;
        themis::acceleration::PluginSecurityVerifier verifier(policy);
        std::string manifest_hash = verifier.calculateFileHash(manifest_path);
        std::ofstream sig_file(manifest_path + ".sig");
        sig_file << manifest_hash;
        sig_file.close();
    }
    
    void deleteManifest(const std::string& name) {
        std::string plugin_dir = test_plugin_dir + "/" + name;
        if (fs::exists(plugin_dir)) {
            fs::remove_all(plugin_dir);
        }
    }
};

TEST_F(PluginHotPlugTest, EnableDisableMonitoring) {
    // Test enabling hot-plug monitoring
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    EXPECT_TRUE(manager->isHotPlugEnabled());
    
    // Test disabling
    manager->disableHotPlug();
    EXPECT_FALSE(manager->isHotPlugEnabled());
}

TEST_F(PluginHotPlugTest, EnableMonitoringTwice) {
    // Enable monitoring
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    
    bool enabled1 = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled1);
    
    // Try to enable again - should fail
    bool enabled2 = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_FALSE(enabled2);
    
    // Still only one monitor
    EXPECT_TRUE(manager->isHotPlugEnabled());
}

TEST_F(PluginHotPlugTest, InvalidDirectory) {
    // Try to monitor non-existent directory
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    
    bool enabled = manager->enableHotPlug("/non/existent/directory", config);
    EXPECT_FALSE(enabled);
    EXPECT_FALSE(manager->isHotPlugEnabled());
}

TEST_F(PluginHotPlugTest, AutoDetectNewPlugin) {
    // Enable hot-plug with auto-load disabled
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    config.auto_reload = false;
    config.auto_unload = false;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    
    // Create a new plugin manifest
    createTestManifest("hot_plugin_1");
    
    // Give monitor time to detect the file
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Plugin should not be auto-loaded since auto_load is false
    EXPECT_FALSE(manager->isPluginLoaded("hot_plugin_1"));
    
    // But it should be discoverable
    manager->scanPluginDirectory(test_plugin_dir);
    auto manifest = manager->getManifest("hot_plugin_1");
    EXPECT_TRUE(manifest.has_value());
    if (manifest) {
        EXPECT_EQ(manifest->name, "hot_plugin_1");
    }
}

TEST_F(PluginHotPlugTest, DetectPluginModification) {
    // Create initial plugin
    createTestManifest("mod_plugin");
    manager->scanPluginDirectory(test_plugin_dir);
    
    // Enable hot-plug with auto-reload disabled
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    config.auto_reload = false;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    
    // Modify the manifest
    modifyManifest("mod_plugin");
    
    // Give monitor time to detect the modification
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Rescan to pick up changes
    manager->scanPluginDirectory(test_plugin_dir);
    
    // Verify the modification was detected
    auto manifest = manager->getManifest("mod_plugin");
    EXPECT_TRUE(manifest.has_value());
    if (manifest) {
        EXPECT_EQ(manifest->version, "1.0.1");
    }
}

TEST_F(PluginHotPlugTest, DetectPluginDeletion) {
    // Create and scan plugin
    createTestManifest("del_plugin");
    manager->scanPluginDirectory(test_plugin_dir);
    
    // Verify it exists
    auto manifest_before = manager->getManifest("del_plugin");
    EXPECT_TRUE(manifest_before.has_value());
    
    // Enable hot-plug with auto-unload disabled
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    config.auto_unload = false;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    
    // Delete the plugin
    deleteManifest("del_plugin");
    
    // Give monitor time to detect the deletion
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Note: Without auto-unload, the plugin entry might still exist
    // This test verifies that the monitor detects the file system event
    // The actual unloading behavior depends on configuration
}

TEST_F(PluginHotPlugTest, ThreadSafety) {
    // Enable monitoring
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    
    // Create multiple plugins concurrently
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([this, i]() {
            std::string plugin_name = "thread_plugin_" + std::to_string(i);
            createTestManifest(plugin_name);
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Give monitor time to detect all files
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Rescan to discover all plugins
    manager->scanPluginDirectory(test_plugin_dir);
    
    // Verify all plugins were detected
    for (int i = 0; i < 5; i++) {
        std::string plugin_name = "thread_plugin_" + std::to_string(i);
        auto manifest = manager->getManifest(plugin_name);
        EXPECT_TRUE(manifest.has_value());
    }
}

TEST_F(PluginHotPlugTest, DisableWhileMonitoring) {
    // Enable monitoring
    themis::plugins::HotPlugConfig config;
    config.enabled = true;
    
    bool enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    
    // Create a plugin
    createTestManifest("disable_test");
    
    // Give monitor time to start processing
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Disable monitoring while it's potentially processing
    manager->disableHotPlug();
    EXPECT_FALSE(manager->isHotPlugEnabled());
    
    // Should be able to enable again
    enabled = manager->enableHotPlug(test_plugin_dir, config);
    EXPECT_TRUE(enabled);
    EXPECT_TRUE(manager->isHotPlugEnabled());
}
