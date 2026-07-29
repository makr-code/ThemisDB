/*
 * ThemisDB | File: test_plugin_manager.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_plugin_manager.cpp
 * @brief Unit tests for PluginManager core operations
 *
 * Tests cover:
 * - Singleton identity
 * - Empty manager state (no loaded plugins)
 * - scanPluginDirectory: error on missing dir, zero on empty dir, discovery on manifests
 * - getPlugin / loadPlugin error paths for unregistered plugins
 * - isPluginLoaded state transitions (unregistered → discovered → not loaded)
 * - getManifest: success after scan, error before scan
 * - unloadPlugin / unloadAllPlugins error paths and empty-state success
 * - getPluginsByType on a manager with no loaded plugins
 * - autoLoadPlugins when all manifests have auto_load=false
 * - Reload listener registration and clearing
 * - Hot-plug enable / disable / isEnabled
 * - negotiateCapabilities for an unknown plugin
 */

#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include "acceleration/plugin_security.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::plugins;
namespace fs = std::filesystem;

// ============================================================================
// Test fixture
// ============================================================================

class PluginManagerTest : public ::testing::Test {
protected:
    std::string test_dir_;
    PluginManager* manager_;

    void SetUp() override {
        test_dir_ = (fs::temp_directory_path() / "themis_test_plugin_manager").string();
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directories(test_dir_);

        manager_ = &PluginManager::instance();
        manager_->disableHotPlug();
        manager_->unloadAllPlugins();
        manager_->clearReloadListeners();
    }

    void TearDown() override {
        manager_->disableHotPlug();
        manager_->unloadAllPlugins();
        manager_->clearReloadListeners();

        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }

    // Helper: create a plugin.json manifest in test_dir_/<name>/
    void createManifest(const std::string& name,
                        const std::string& type = "custom",
                        bool auto_load = false,
                        const std::vector<std::string>& deps = {},
                        const nlohmann::json& extra_fields = {}) {
        std::string plugin_dir = test_dir_ + "/" + name;
        fs::create_directories(plugin_dir);

        nlohmann::json manifest = {
            {"name", name},
            {"version", "1.0.0"},
            {"type", type},
            {"description", "Test plugin: " + name},
            {"binary", {
                {"windows", name + ".dll"},
                {"linux",   name + ".so"},
                {"macos",   name + ".dylib"}
            }},
            {"capabilities", {
                {"thread_safe", true},
                {"streaming",   false}
            }},
            {"auto_load", auto_load},
            {"load_priority", 100}
        };

        if (!deps.empty()) {
            manifest["dependencies"] = deps;
        }
        for (auto it = extra_fields.begin(); it != extra_fields.end(); ++it) {
            manifest[it.key()] = it.value();
        }

        std::string manifest_path = plugin_dir + "/plugin.json";
        std::ofstream file(manifest_path);
        file << manifest.dump(2);
        file.close();

        // Release builds require manifest signature files; write expected hash.
        themis::acceleration::PluginSecurityPolicy policy;
        themis::acceleration::PluginSecurityVerifier verifier(policy);
        std::string manifest_hash = verifier.calculateFileHash(manifest_path);

        std::string sig_path = manifest_path + ".sig";
        std::ofstream sig_file(sig_path);
        sig_file << manifest_hash;
        sig_file.close();
    }
};

// ============================================================================
// Singleton
// ============================================================================

TEST_F(PluginManagerTest, SingletonReturnsSameInstance) {
    PluginManager& a = PluginManager::instance();
    PluginManager& b = PluginManager::instance();
    EXPECT_EQ(&a, &b);
}

// ============================================================================
// Empty state
// ============================================================================

TEST_F(PluginManagerTest, InitialLoadedPluginsEmpty) {
    auto loaded = manager_->listLoadedPlugins();
    EXPECT_TRUE(loaded.empty());
}

TEST_F(PluginManagerTest, IsPluginLoadedReturnsFalseForUnregisteredPlugin) {
    EXPECT_FALSE(manager_->isPluginLoaded("nonexistent_plugin_abc123"));
}

TEST_F(PluginManagerTest, GetPluginsByTypeEmptyWhenNoneLoaded) {
    auto plugins = manager_->getPluginsByType(PluginType::CUSTOM);
    EXPECT_TRUE(plugins.empty());
}

// ============================================================================
// scanPluginDirectory
// ============================================================================

TEST_F(PluginManagerTest, ScanNonExistentDirectoryReturnsError) {
    auto result = manager_->scanPluginDirectory("/nonexistent/path/xyz_themis_test");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
}

TEST_F(PluginManagerTest, ScanEmptyDirectoryReturnsZero) {
    auto result = manager_->scanPluginDirectory(test_dir_);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);
}

TEST_F(PluginManagerTest, ScanDirectoryDiscoversOneManifest) {
    createManifest("pm_scan_single_001");

    auto result = manager_->scanPluginDirectory(test_dir_);
    EXPECT_TRUE(result.has_value());
    EXPECT_GE(*result, 1u);
}

TEST_F(PluginManagerTest, ScanDirectoryDiscoversMultipleManifests) {
    createManifest("pm_scan_multi_001");
    createManifest("pm_scan_multi_002");
    createManifest("pm_scan_multi_003");

    auto result = manager_->scanPluginDirectory(test_dir_);
    EXPECT_TRUE(result.has_value());
    EXPECT_GE(*result, 3u);
}

TEST_F(PluginManagerTest, DiscoveredPluginIsNotLoadedAfterScan) {
    createManifest("pm_discovered_only_001");
    manager_->scanPluginDirectory(test_dir_);

    // Plugin registered in registry but binary is absent — not loaded
    EXPECT_FALSE(manager_->isPluginLoaded("pm_discovered_only_001"));
}

// ============================================================================
// getPlugin / loadPlugin error paths
// ============================================================================

TEST_F(PluginManagerTest, GetPluginNotInRegistry) {
    auto result = manager_->getPlugin("pm_not_registered_xyz");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerTest, LoadPluginNotInRegistry) {
    auto result = manager_->loadPlugin("pm_not_registered_xyz");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerTest, LoadPluginWithMissingBinaryFails) {
    // Manifest is registered but no .so/.dll exists → load must fail
    createManifest("pm_no_binary_001");
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->loadPlugin("pm_no_binary_001");
    EXPECT_FALSE(result.has_value());
    // Depending on runtime edition/license gating this may be rejected as
    // NOT_FOUND before binary/signature checks; otherwise LOAD_FAILED or
    // INVALID_SIGNATURE are valid outcomes.
    EXPECT_TRUE(
        result.error().code() == themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND ||
        result.error().code() == themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED ||
        result.error().code() == themis::errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE);
}

// ============================================================================
// getManifest
// ============================================================================

TEST_F(PluginManagerTest, GetManifestForNonExistentPluginReturnsError) {
    auto result = manager_->getManifest("pm_no_such_manifest_xyz");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerTest, GetManifestAfterScanSucceeds) {
    createManifest("pm_manifest_test_001");
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->getManifest("pm_manifest_test_001");
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        EXPECT_EQ(result->name, "pm_manifest_test_001");
        EXPECT_EQ(result->version, "1.0.0");
    }
}

TEST_F(PluginManagerTest, GetManifestParsesPrivateMetadata) {
    createManifest(
        "pm_private_manifest_001",
        "custom",
        false,
        {},
        nlohmann::json{
            {"visibility", "private"},
            {"allowed_editions", nlohmann::json::array({"enterprise", "hyperscaler"})},
            {"license_feature", "private_connector_pack"},
            {"min_themisdb_version", "2.4.0"},
            {"compatible_core_abi", "plugin-abi-v2"}
        });
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->getManifest("pm_private_manifest_001");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->visibility, "private");
    ASSERT_EQ(result->allowed_editions.size(), 2u);
    EXPECT_EQ(result->allowed_editions[0], "enterprise");
    EXPECT_EQ(result->license_feature, "private_connector_pack");
    EXPECT_EQ(result->min_themisdb_version, "2.4.0");
    EXPECT_EQ(result->compatible_core_abi, "plugin-abi-v2");
}

// ============================================================================
// unloadPlugin / unloadAllPlugins
// ============================================================================

TEST_F(PluginManagerTest, UnloadNonExistentPluginReturnsError) {
    auto result = manager_->unloadPlugin("pm_no_such_plugin_xyz");
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerTest, UnloadDiscoveredButUnloadedPluginReturnsError) {
    createManifest("pm_unload_disc_001");
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->unloadPlugin("pm_unload_disc_001");
    EXPECT_FALSE(result.has_value());
    // Plugin is registered but not loaded — should fail
    EXPECT_EQ(result.error().code(),
              themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerTest, UnloadAllPluginsWhenEmptySucceeds) {
    auto result = manager_->unloadAllPlugins();
    EXPECT_TRUE(result.has_value());
}

TEST_F(PluginManagerTest, ListLoadedPluginsEmptyAfterUnloadAll) {
    manager_->unloadAllPlugins();
    EXPECT_TRUE(manager_->listLoadedPlugins().empty());
}

// ============================================================================
// Reload listeners
// ============================================================================

TEST_F(PluginManagerTest, RegisterReloadListenerAndClearDoesNotThrow) {
    bool reached = false;
    manager_->registerReloadListener(
        [&reached](const std::string&, PluginReloadPhase) { reached = true; });

    // Clear without any reload — must not crash
    manager_->clearReloadListeners();
    EXPECT_FALSE(reached);  // Listener was never triggered
}

// ============================================================================
// Hot-plug monitoring
// ============================================================================

TEST_F(PluginManagerTest, HotPlugInitiallyDisabled) {
    EXPECT_FALSE(manager_->isHotPlugEnabled());
}

TEST_F(PluginManagerTest, EnableHotPlugOnValidDirectorySucceeds) {
    bool ok = manager_->enableHotPlug(test_dir_);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(manager_->isHotPlugEnabled());
    manager_->disableHotPlug();
}

TEST_F(PluginManagerTest, DisableHotPlugWhenEnabled) {
    manager_->enableHotPlug(test_dir_);
    manager_->disableHotPlug();
    EXPECT_FALSE(manager_->isHotPlugEnabled());
}

TEST_F(PluginManagerTest, EnableHotPlugOnNonExistentDirectoryFails) {
    bool ok = manager_->enableHotPlug("/nonexistent/path/xyz_hot_plug_test");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(manager_->isHotPlugEnabled());
}

// ============================================================================
// negotiateCapabilities
// ============================================================================

TEST_F(PluginManagerTest, NegotiateCapabilitiesForUnknownPluginReturnsFalse) {
    PluginNegotiationResult result =
        manager_->negotiateCapabilities("pm_unknown_plugin_xyz", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(PluginManagerTest, NegotiateCapabilitiesForDiscoveredButUnloadedPluginReturnsFalse) {
    createManifest("pm_negotiate_disc_001");
    manager_->scanPluginDirectory(test_dir_);

    PluginNegotiationResult result =
        manager_->negotiateCapabilities("pm_negotiate_disc_001", {});
    // Plugin is known but not loaded — negotiation must report failure
    EXPECT_FALSE(result.success);
}

