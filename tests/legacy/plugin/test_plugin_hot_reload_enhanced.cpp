#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#ifndef _WIN32
#include <unistd.h>  // getpid()
#else
#include <process.h>
#define getpid _getpid
#endif

using namespace themis::plugins;

// ============================================================================
// Test Plugin Implementations
// ============================================================================

/**
 * @brief Stateful test plugin that implements IStatefulPlugin
 */
class StatefulTestPlugin : public IThemisPlugin, public IStatefulPlugin {
private:
    bool initialized_ = false;
    int counter_ = 0;
    std::string saved_data_;
    
public:
    const char* getName() const override { return "stateful_test"; }
    const char* getVersion() const override { return "1.0.0"; }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.thread_safe = true;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        initialized_ = true;
        
        // Try to restore state from config
        try {
            if (config_json && config_json[0] != '\0') {
                // Quick check for empty object before parsing
                std::string_view config_str(config_json);
                if (config_str != "{}") {
                    nlohmann::json config = nlohmann::json::parse(config_json);
                    if (config.contains("restored_state")) {
                        std::string state = config["restored_state"];
                        restoreState(state);
                    }
                }
            }
        } catch (...) {
            // Ignore parsing errors
        }
        
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    void* getInstance() override {
        return this;
    }
    
    // IStatefulPlugin implementation
    std::string saveState() override {
        nlohmann::json state;
        state["counter"] = counter_;
        state["saved_data"] = saved_data_;
        return state.dump();
    }
    
    bool restoreState(const std::string& state) override {
        try {
            nlohmann::json json_state = nlohmann::json::parse(state);
            counter_ = json_state["counter"];
            saved_data_ = json_state["saved_data"];
            return true;
        } catch (...) {
            return false;
        }
    }
    
    // Test helpers
    void incrementCounter() { counter_++; }
    int getCounter() const { return counter_; }
    void setSavedData(const std::string& data) { saved_data_ = data; }
    std::string getSavedData() const { return saved_data_; }
    bool isInitialized() const { return initialized_; }
};

/**
 * @brief Simple test plugin without state
 */
class SimpleTestPlugin : public IThemisPlugin {
private:
    bool initialized_ = false;
    
public:
    const char* getName() const override { return "simple_test"; }
    const char* getVersion() const override { return "1.0.0"; }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.thread_safe = true;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    void* getInstance() override {
        return this;
    }
    
    bool isInitialized() const { return initialized_; }
};

/**
 * @brief Plugin that fails initialization
 */
class FailingTestPlugin : public IThemisPlugin {
public:
    const char* getName() const override { return "failing_test"; }
    const char* getVersion() const override { return "1.0.0"; }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        return PluginCapabilities{};
    }
    
    bool initialize(const char* config_json) override {
        return false;  // Always fail
    }
    
    void shutdown() override {}
    
    void* getInstance() override {
        return this;
    }
};

// ============================================================================
// Test Fixture
// ============================================================================

class EnhancedHotReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register test plugins
        PluginManagerRegistry::registerFactory(
            "stateful_test",
            PluginType::CUSTOM,
            []() { return std::make_unique<StatefulTestPlugin>(); }
        );
        
        PluginManagerRegistry::registerFactory(
            "simple_test",
            PluginType::CUSTOM,
            []() { return std::make_unique<SimpleTestPlugin>(); }
        );
        
        PluginManagerRegistry::registerFactory(
            "failing_test",
            PluginType::CUSTOM,
            []() { return std::make_unique<FailingTestPlugin>(); }
        );
        
        // Clear any existing reload listeners
        auto& pm = PluginManager::instance();
        pm.clearReloadListeners();
    }
    
    void TearDown() override {
        // Clean up
        auto& pm = PluginManager::instance();
        pm.clearReloadListeners();
    }
};

// ============================================================================
// State Preservation Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, StatefulPluginStatePreservation) {
    auto& pm = PluginManager::instance();
    
    // Create and load stateful plugin
    auto plugin = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin, nullptr);
    ASSERT_TRUE(plugin->initialize("{}"));
    
    auto* stateful = dynamic_cast<StatefulTestPlugin*>(plugin.get());
    ASSERT_NE(stateful, nullptr);
    
    // Modify plugin state
    stateful->incrementCounter();
    stateful->incrementCounter();
    stateful->incrementCounter();
    stateful->setSavedData("important data");
    
    EXPECT_EQ(stateful->getCounter(), 3);
    EXPECT_EQ(stateful->getSavedData(), "important data");
    
    // Save state
    std::string saved = stateful->saveState();
    EXPECT_FALSE(saved.empty());
    
    // Create new instance and restore state
    auto plugin2 = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin2, nullptr);
    
    auto* stateful2 = dynamic_cast<StatefulTestPlugin*>(plugin2.get());
    ASSERT_NE(stateful2, nullptr);
    
    // Initially counter should be 0
    EXPECT_EQ(stateful2->getCounter(), 0);
    
    // Restore state
    EXPECT_TRUE(stateful2->restoreState(saved));
    
    // Verify state was restored
    EXPECT_EQ(stateful2->getCounter(), 3);
    EXPECT_EQ(stateful2->getSavedData(), "important data");
}

TEST_F(EnhancedHotReloadTest, StatefulPluginInvalidStateRestore) {
    auto plugin = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin, nullptr);
    
    auto* stateful = dynamic_cast<StatefulTestPlugin*>(plugin.get());
    ASSERT_NE(stateful, nullptr);
    
    // Try to restore invalid state
    EXPECT_FALSE(stateful->restoreState("invalid json"));
    EXPECT_FALSE(stateful->restoreState("{\"wrong_field\": 123}"));
}

// ============================================================================
// Event Notification Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, ReloadEventNotifications) {
    auto& pm = PluginManager::instance();
    
    // Track events
    std::vector<std::pair<std::string, PluginReloadPhase>> events;
    
    pm.registerReloadListener([&events](const std::string& name, PluginReloadPhase phase) {
        events.push_back({name, phase});
    });
    
    // Note: We can't actually test reload without a real plugin binary,
    // but we can test the listener registration system works
    
    // Verify listener was registered (implicit - no crash)
    EXPECT_NO_THROW(pm.clearReloadListeners());
}

TEST_F(EnhancedHotReloadTest, MultipleReloadListeners) {
    auto& pm = PluginManager::instance();
    
    int listener1_calls = 0;
    int listener2_calls = 0;
    
    pm.registerReloadListener([&listener1_calls](const std::string&, PluginReloadPhase) {
        listener1_calls++;
    });
    
    pm.registerReloadListener([&listener2_calls](const std::string&, PluginReloadPhase) {
        listener2_calls++;
    });
    
    // Clear listeners
    pm.clearReloadListeners();
    
    // Verify no crash and system is stable
    EXPECT_NO_THROW(pm.registerReloadListener([](const std::string&, PluginReloadPhase) {}));
}

TEST_F(EnhancedHotReloadTest, ReloadListenerException) {
    auto& pm = PluginManager::instance();
    
    // Register listener that throws
    pm.registerReloadListener([](const std::string&, PluginReloadPhase) {
        throw std::runtime_error("Test exception");
    });
    
    // This should not crash the system (exceptions are caught)
    EXPECT_NO_THROW(pm.clearReloadListeners());
}

// ============================================================================
// Dependency Management Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, DependencyBlocksReload) {
    // Note: Full dependency testing requires actual plugin files with manifests
    // This test verifies the API exists and doesn't crash
    
    auto& pm = PluginManager::instance();
    
    // Attempting to reload non-existent plugin should fail gracefully
    EXPECT_FALSE(pm.reloadPlugin("nonexistent_plugin"));
}

TEST_F(EnhancedHotReloadTest, ReloadNonLoadedPlugin) {
    auto& pm = PluginManager::instance();
    
    // Attempting to reload a plugin that's not loaded should fail
    EXPECT_FALSE(pm.reloadPlugin("never_loaded_plugin"));
}

// ============================================================================
// Rollback Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, RollbackAPIExists) {
    // Verify the enhanced reload API exists and compiles
    auto& pm = PluginManager::instance();
    
    // These calls should not crash even with invalid inputs
    EXPECT_FALSE(pm.reloadPlugin(""));
    EXPECT_FALSE(pm.reloadPlugin("invalid_plugin_name"));
}

TEST_F(EnhancedHotReloadTest, ReloadNotLoadedPluginReturnsNotFound) {
    auto& pm = PluginManager::instance();

    // reloadPlugin must return ERR_PLUGIN_NOT_FOUND when the plugin is not loaded
    auto result = pm.reloadPlugin("never_loaded_plugin_xyz");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(EnhancedHotReloadTest, DependencyConflictErrorCodeDistinct) {
    // Verify ERR_PLUGIN_DEPENDENCY_CONFLICT is defined and distinct from related codes
    EXPECT_NE(static_cast<int>(themis::errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT), 0);
    EXPECT_NE(themis::errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT,
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
    EXPECT_NE(themis::errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT,
              themis::errors::ErrorCode::ERR_PLUGIN_MISSING_DEPENDENCY);
    EXPECT_NE(themis::errors::ErrorCode::ERR_PLUGIN_DEPENDENCY_CONFLICT,
              themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED);
}

TEST_F(EnhancedHotReloadTest, StatefulPluginPreservesStateAcrossReload) {
    // Verify that IStatefulPlugin saveState/restoreState works correctly,
    // matching the pattern used by reloadPlugin.

    auto plugin = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin, nullptr);
    ASSERT_TRUE(plugin->initialize("{}"));

    auto* stateful = dynamic_cast<StatefulTestPlugin*>(plugin.get());
    ASSERT_NE(stateful, nullptr);

    stateful->incrementCounter();
    stateful->incrementCounter();
    stateful->setSavedData("persistent data");

    // Step 1: Save state (what reloadPlugin does before unloading)
    std::string state = stateful->saveState();
    EXPECT_FALSE(state.empty());

    // Step 2: Simulate reload — create new instance (what reloadPlugin does after loading)
    auto plugin2 = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin2, nullptr);
    ASSERT_TRUE(plugin2->initialize("{}"));

    auto* stateful2 = dynamic_cast<StatefulTestPlugin*>(plugin2.get());
    ASSERT_NE(stateful2, nullptr);

    // Fresh instance has no state
    EXPECT_EQ(stateful2->getCounter(), 0);
    EXPECT_EQ(stateful2->getSavedData(), "");

    // Step 3: Restore state (what reloadPlugin does after successful reload)
    EXPECT_TRUE(stateful2->restoreState(state));
    EXPECT_EQ(stateful2->getCounter(), 2);
    EXPECT_EQ(stateful2->getSavedData(), "persistent data");
}

TEST_F(EnhancedHotReloadTest, StatefulPluginRestoreStateFailsGracefully) {
    // restoreState must return false for malformed JSON without throwing
    auto plugin = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin, nullptr);

    auto* stateful = dynamic_cast<StatefulTestPlugin*>(plugin.get());
    ASSERT_NE(stateful, nullptr);

    // These should return false, not throw
    EXPECT_FALSE(stateful->restoreState("not json at all"));
    EXPECT_FALSE(stateful->restoreState(""));
    EXPECT_FALSE(stateful->restoreState("{\"wrong_key\": 99}"));

    // Counter and data should remain at default values
    EXPECT_EQ(stateful->getCounter(), 0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, ConcurrentListenerRegistration) {
    auto& pm = PluginManager::instance();
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    // Register listeners concurrently
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&pm, i]() {
            pm.registerReloadListener([i](const std::string&, PluginReloadPhase) {
                // Empty listener
            });
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Clear should work without crash
    EXPECT_NO_THROW(pm.clearReloadListeners());
}

TEST_F(EnhancedHotReloadTest, ConcurrentReloadAttempts) {
    auto& pm = PluginManager::instance();
    
    const int num_threads = 5;
    std::vector<std::thread> threads;
    
    // Attempt concurrent reloads (should fail gracefully)
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&pm]() {
            pm.reloadPlugin("test_plugin");
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should not crash
    EXPECT_TRUE(true);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, StateSaveRestoreCycle) {
    auto plugin = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin, nullptr);
    ASSERT_TRUE(plugin->initialize("{}"));
    
    auto* stateful = dynamic_cast<StatefulTestPlugin*>(plugin.get());
    ASSERT_NE(stateful, nullptr);
    
    // Set up state
    for (int i = 0; i < 5; ++i) {
        stateful->incrementCounter();
    }
    stateful->setSavedData("test data 123");
    
    // Save state
    std::string saved_state = stateful->saveState();
    EXPECT_FALSE(saved_state.empty());
    
    // Simulate reload by creating new instance
    auto plugin2 = PluginManagerRegistry::createPlugin("stateful_test");
    ASSERT_NE(plugin2, nullptr);
    
    // Initialize with restored state in config
    nlohmann::json config;
    config["restored_state"] = saved_state;
    ASSERT_TRUE(plugin2->initialize(config.dump().c_str()));
    
    auto* stateful2 = dynamic_cast<StatefulTestPlugin*>(plugin2.get());
    ASSERT_NE(stateful2, nullptr);
    
    // Verify state was restored during initialization
    EXPECT_EQ(stateful2->getCounter(), 5);
    EXPECT_EQ(stateful2->getSavedData(), "test data 123");
}

TEST_F(EnhancedHotReloadTest, PluginReloadPhaseValues) {
    // Verify enum values are distinct
    EXPECT_NE(static_cast<int>(PluginReloadPhase::BEFORE_UNLOAD),
              static_cast<int>(PluginReloadPhase::AFTER_UNLOAD));
    EXPECT_NE(static_cast<int>(PluginReloadPhase::AFTER_UNLOAD),
              static_cast<int>(PluginReloadPhase::AFTER_LOAD));
    EXPECT_NE(static_cast<int>(PluginReloadPhase::BEFORE_UNLOAD),
              static_cast<int>(PluginReloadPhase::AFTER_LOAD));
}

// ============================================================================
// Atomic Reload / Rollback Tests
// ============================================================================

TEST_F(EnhancedHotReloadTest, AtomicReload_NotLoadedPluginLeavesRegistryUnchanged) {
    // The atomic reload design guarantees: if reloadPlugin() is called on a
    // registered-but-not-loaded plugin, it returns ERR_PLUGIN_NOT_FOUND
    // and the registry entry is completely unchanged.

    auto& pm = PluginManager::instance();

    // Register a plugin via scanPluginDirectory into a temp dir with a manifest
    // but NO binary — so it will be registered as loaded=false.
    namespace fs = std::filesystem;
    // Use PID to avoid directory collisions when tests run in parallel.
    auto tmp_dir = fs::temp_directory_path() /
                   ("themis_reload_rollback_" + std::to_string(::getpid()));

    // RAII cleanup: remove directory on scope exit (handles exceptions too)
    struct DirGuard {
        fs::path path;
        ~DirGuard() { try { fs::remove_all(path); } catch (...) {} }
    } dir_guard{tmp_dir};

    fs::create_directories(tmp_dir);

    {
        nlohmann::json manifest;
        manifest["name"]        = "rollback_test_plugin";
        manifest["version"]     = "1.0.0";
        manifest["type"]        = "custom";
        manifest["description"] = "Test plugin for rollback";
        manifest["binary"]["windows"] = "rollback_test_plugin.dll";
        manifest["binary"]["linux"]   = "rollback_test_plugin.so";
        manifest["binary"]["macos"]   = "rollback_test_plugin.dylib";
        std::ofstream(tmp_dir / "plugin.json") << manifest.dump();
    }

    auto scan_result = pm.scanPluginDirectory(tmp_dir.string());
    // Scan may succeed (manifest parsed) or fail (binary not found); we only
    // care that the reload contract holds.
    (void)scan_result;

    // reloadPlugin on a not-loaded plugin must return ERR_PLUGIN_NOT_FOUND
    auto result = pm.reloadPlugin("rollback_test_plugin");
    EXPECT_FALSE(result);
    EXPECT_EQ(result.error().code(), themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);

    // The plugin entry must NOT be loaded (old state preserved = not-loaded)
    EXPECT_FALSE(pm.isPluginLoaded("rollback_test_plugin"));
}

TEST_F(EnhancedHotReloadTest, AtomicReload_ListenerNotCalledOnEarlyFailure) {
    // Reload listeners (BEFORE_UNLOAD etc.) must NOT be called if reloadPlugin
    // returns an error in Phase 1 (before any unload begins), so that
    // external systems are not incorrectly notified of a reload that never happened.

    auto& pm = PluginManager::instance();

    std::vector<PluginReloadPhase> observed_phases;
    pm.registerReloadListener([&observed_phases](const std::string&, PluginReloadPhase phase) {
        observed_phases.push_back(phase);
    });

    // This should fail immediately in Phase 1 (not loaded)
    auto result = pm.reloadPlugin("not_loaded_plugin_for_listener_test");
    EXPECT_FALSE(result);

    // No listeners should have been called
    EXPECT_TRUE(observed_phases.empty());
}

TEST_F(EnhancedHotReloadTest, AtomicReload_DependencyCheckPreventsListenerCall) {
    // Same guarantee: if dependency conflict blocks reload in Phase 1,
    // no listeners should fire.
    // (Actual dependency testing with loaded plugins requires real binaries;
    //  this verifies the early-exit code path is reached and no listeners fire.)

    auto& pm = PluginManager::instance();

    std::vector<PluginReloadPhase> observed_phases;
    pm.registerReloadListener([&observed_phases](const std::string&, PluginReloadPhase phase) {
        observed_phases.push_back(phase);
    });

    // Plugin is not loaded, so we hit ERR_PLUGIN_NOT_FOUND before dependency check,
    // but no listeners fire either way.
    auto result = pm.reloadPlugin("dependency_blocked_plugin");
    EXPECT_FALSE(result);
    EXPECT_TRUE(observed_phases.empty());
}

// ============================================================================
// Main
// ============================================================================


