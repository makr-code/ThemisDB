/**
 * @file test_plugin_manager_comprehensive.cpp
 * @brief Comprehensive integration-style tests for PluginManager
 *
 * Tests cover multi-plugin scenarios, dependency edge cases, type-indexed
 * lookups, reload listener call counts, metrics access, and attachHealthMonitor.
 */

#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include "plugins/plugin_health_monitor.h"
#include "plugins/self_healing_plugin.h"
#include "acceleration/plugin_security.h"
#include "utils/error_registry.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>

using namespace themis::plugins;
namespace fs = std::filesystem;

// ============================================================================
// Test fixture
// ============================================================================

class PluginManagerComprehensiveTest : public ::testing::Test {
protected:
    std::string test_dir_;
    PluginManager* manager_;

    void SetUp() override {
        test_dir_ = (fs::temp_directory_path() / "themis_test_pm_comp").string();
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
            // Hot-plug monitor shutdown can release handles asynchronously on Windows.
            std::error_code ec;
            fs::remove_all(test_dir_, ec);
            if (ec) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                ec.clear();
                fs::remove_all(test_dir_, ec);
            }
        }
    }

    void createManifest(const std::string& name,
                        const std::string& type = "custom",
                        bool auto_load = false,
                        const std::vector<std::string>& deps = {},
                        const std::string& version = "1.0.0") {
        std::string plugin_dir = test_dir_ + "/" + name;
        fs::create_directories(plugin_dir);

        nlohmann::json manifest = {
            {"name", name},
            {"version", version},
            {"type", type},
            {"description", "Comprehensive test plugin: " + name},
            {"binary", {
                {"windows", name + ".dll"},
                {"linux",   name + ".so"},
                {"macos",   name + ".dylib"}
            }},
            {"capabilities", {
                {"thread_safe",   true},
                {"streaming",     false},
                {"batching",      true},
                {"gpu_accelerated", false}
            }},
            {"auto_load", auto_load},
            {"load_priority", 100}
        };

        if (!deps.empty()) {
            manifest["dependencies"] = deps;
        }

        std::string manifest_path = plugin_dir + "/plugin.json";
        std::ofstream file(manifest_path);
        file << manifest.dump(2);
        file.close();

        // Release builds require manifest signature files; write expected hash.
        themis::acceleration::PluginSecurityPolicy policy;
        themis::acceleration::PluginSecurityVerifier verifier(policy);
        std::string manifest_hash = verifier.calculateFileHash(manifest_path);

        std::ofstream sig_file(manifest_path + ".sig");
        sig_file << manifest_hash;
    }
};

// ============================================================================
// Multi-plugin scanning
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, ScanDirectoryWithMixedTypesDiscoversAll) {
    createManifest("pmc_blob_001",    "blob_storage");
    createManifest("pmc_custom_001",  "custom");
    createManifest("pmc_importer_001","importer");

    auto result = manager_->scanPluginDirectory(test_dir_);
    EXPECT_TRUE(result.has_value());
    EXPECT_GE(*result, 3u);
}

TEST_F(PluginManagerComprehensiveTest, ListPluginsAfterScanIncludesDiscoveredNames) {
    createManifest("pmc_list_001");
    createManifest("pmc_list_002");
    manager_->scanPluginDirectory(test_dir_);

    auto all = manager_->listPlugins();
    // Ensure both discovered plugins are present in the registry
    bool found_001 = false, found_002 = false;
    for (const auto& m : all) {
        if (m.name == "pmc_list_001") found_001 = true;
        if (m.name == "pmc_list_002") found_002 = true;
    }
    EXPECT_TRUE(found_001);
    EXPECT_TRUE(found_002);
}

// ============================================================================
// Type-based lookups
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, GetPluginsByTypeReturnsEmptyForUnloadedPlugins) {
    createManifest("pmc_type_blob_001", "blob_storage");
    manager_->scanPluginDirectory(test_dir_);

    // Plugin is discovered but not loaded (no binary) — must return empty
    auto result = manager_->getPluginsByType(PluginType::BLOB_STORAGE);
    EXPECT_TRUE(result.empty());
}

TEST_F(PluginManagerComprehensiveTest, GetPluginsByTypeReturnsEmptyForUnusedType) {
    createManifest("pmc_type_custom_002", "custom");
    manager_->scanPluginDirectory(test_dir_);

    // EMBEDDING type has no registered plugin
    auto result = manager_->getPluginsByType(PluginType::EMBEDDING);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// autoLoadPlugins — no auto-load plugins
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, AutoLoadSkipsManualPlugins) {
    createManifest("pmc_manual_001", "custom", false);
    createManifest("pmc_manual_002", "custom", false);
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->autoLoadPlugins();
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(*result, 0u);  // None should have been loaded
}

// ============================================================================
// Dependency edge cases
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, LoadPluginWithCircularDependencyFails) {
    // A → B → A (circular)
    createManifest("pmc_circ_a_001", "custom", false, {"pmc_circ_b_001"});
    createManifest("pmc_circ_b_001", "custom", false, {"pmc_circ_a_001"});
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->loadPlugin("pmc_circ_a_001");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().code() ==
                    themis::errors::ErrorCode::ERR_PLUGIN_CIRCULAR_DEPENDENCY ||
                result.error().code() ==
                    themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

TEST_F(PluginManagerComprehensiveTest, LoadPluginWithUnregisteredDependencyFails) {
    createManifest("pmc_dep_child_001", "custom", false, {"pmc_dep_parent_missing"});
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->loadPlugin("pmc_dep_child_001");
    EXPECT_FALSE(result.has_value());
    EXPECT_TRUE(result.error().code() ==
                    themis::errors::ErrorCode::ERR_PLUGIN_MISSING_DEPENDENCY ||
                result.error().code() ==
                    themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND);
}

// ============================================================================
// getManifest — version field
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, GetManifestReturnsCorrectVersion) {
    createManifest("pmc_ver_test_001", "custom", false, {}, "2.3.1");
    manager_->scanPluginDirectory(test_dir_);

    auto result = manager_->getManifest("pmc_ver_test_001");
    EXPECT_TRUE(result.has_value());
    if (result.has_value()) {
        EXPECT_EQ(result->version, "2.3.1");
    }
}

// ============================================================================
// Reload listeners
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, MultipleReloadListenersCanBeRegistered) {
    std::atomic<int> call_count{0};

    manager_->registerReloadListener(
        [&call_count](const std::string&, PluginReloadPhase) { call_count++; });
    manager_->registerReloadListener(
        [&call_count](const std::string&, PluginReloadPhase) { call_count++; });

    // Listeners exist but no reload triggered yet
    EXPECT_EQ(call_count.load(), 0);

    // Clearing must not invoke listeners
    manager_->clearReloadListeners();
    EXPECT_EQ(call_count.load(), 0);
}

TEST_F(PluginManagerComprehensiveTest, ClearListenersIsIdempotent) {
    manager_->clearReloadListeners();
    manager_->clearReloadListeners();  // Should not crash
    EXPECT_TRUE(true);
}

// ============================================================================
// Metrics access
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, GetMetricsReturnsSameReference) {
    const PluginMetrics& m1 = manager_->getMetrics();
    const PluginMetrics& m2 = manager_->getMetrics();
    EXPECT_EQ(&m1, &m2);
}

TEST_F(PluginManagerComprehensiveTest, GetMutableMetricsReturnsSameAsConst) {
    PluginMetrics& mut = manager_->getMetricsMutable();
    const PluginMetrics& cst = manager_->getMetrics();
    // Both should point to the same underlying object
    EXPECT_EQ(&mut, &cst);
}

// ============================================================================
// attachHealthMonitor
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, AttachNullHealthMonitorDoesNotCrash) {
    manager_->attachHealthMonitor(nullptr);
    EXPECT_TRUE(true);  // No exception
}

TEST_F(PluginManagerComprehensiveTest, AttachAndDetachHealthMonitor) {
    PluginHealthMonitor monitor;
    manager_->attachHealthMonitor(&monitor);
    manager_->attachHealthMonitor(nullptr);  // Detach
    EXPECT_TRUE(true);
}

// ============================================================================
// Hot-plug with scan
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, EnableHotPlugThenScanDiscoversPlugins) {
    createManifest("pmc_hotplug_scan_001");

    bool ok = manager_->enableHotPlug(test_dir_);
    EXPECT_TRUE(ok);

    auto result = manager_->scanPluginDirectory(test_dir_);
    EXPECT_TRUE(result.has_value());
    EXPECT_GE(*result, 1u);

    manager_->disableHotPlug();
}

// ============================================================================
// negotiateCapabilities — with requirements
// ============================================================================

TEST_F(PluginManagerComprehensiveTest, NegotiateCapabilitiesRequirementsForUnloadedPlugin) {
    createManifest("pmc_neg_disc_001");
    manager_->scanPluginDirectory(test_dir_);

    PluginCapabilityRequirement req;
    req.capability_name = "thread_safe";

    PluginNegotiationResult result =
        manager_->negotiateCapabilities("pmc_neg_disc_001", {req});
    // Plugin is not loaded — negotiation must fail
    EXPECT_FALSE(result.success);
}

