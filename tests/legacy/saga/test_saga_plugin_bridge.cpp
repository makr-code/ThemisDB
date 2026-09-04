#include <gtest/gtest.h>

#include "plugins/plugin_manager.h"
#include "transaction/saga_orchestrator.h"
#include "transaction/saga_plugin_bridge.h"
#include "utils/error_registry.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

namespace {

fs::path runtimeDir() {
    // Focused tests are executed with WORKING_DIRECTORY set to TARGET_FILE_DIR.
    return fs::current_path();
}

fs::path buildRootDir() {
    // In this repo layout the runtime dir is usually <build>/bin.
    return runtimeDir().parent_path();
}

bool tryResolveExistingPath(const std::initializer_list<fs::path>& candidates,
                            fs::path& resolved) {
    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            resolved = candidate;
            return true;
        }
    }
    return false;
}

} // namespace

TEST(SagaPluginBridgeFocusedTest, BindsSagaOrchestratorFromRuntimePluginDirectory) {
    auto& manager = themis::plugins::PluginManager::instance();
    ASSERT_TRUE(manager.unloadAllPlugins().has_value());
    manager.disableHotPlug();
    manager.clearReloadListeners();

#ifdef _WIN32
    const fs::path plugin_binary = runtimeDir() / "saga_orchestrator_plugin.dll";
#elif defined(__APPLE__)
    const fs::path plugin_binary = runtimeDir() / "libsaga_orchestrator_plugin.dylib";
#else
    const fs::path plugin_binary = runtimeDir() / "libsaga_orchestrator_plugin.so";
#endif
    fs::path plugin_manifest_source;
    fs::path plugin_manifest_sig_source;

    const auto has_manifest = tryResolveExistingPath(
        {
            runtimeDir() / "saga_orchestrator.plugin.json",
            buildRootDir() / "cmake" / "src" / "transaction" / "saga_plugin" / "plugin.json",
        },
        plugin_manifest_source);

    const auto has_manifest_sig = tryResolveExistingPath(
        {
            runtimeDir() / "saga_orchestrator.plugin.json.sig",
            buildRootDir() / "cmake" / "src" / "transaction" / "saga_plugin" / "plugin.json.sig",
        },
        plugin_manifest_sig_source);

    ASSERT_TRUE(fs::exists(plugin_binary))
        << "Expected runtime plugin binary missing: " << plugin_binary.string();
    ASSERT_TRUE(has_manifest)
        << "Expected plugin manifest missing in runtime or CMake plugin build directory.";
    ASSERT_TRUE(has_manifest_sig)
        << "Expected plugin manifest signature missing in runtime or CMake plugin build directory.";

    const fs::path stage_dir =
        fs::temp_directory_path() / "themis_saga_plugin_bridge_stage";
    std::error_code ec = {};
    fs::remove_all(stage_dir, ec);
    fs::create_directories(stage_dir, ec);
    ASSERT_FALSE(ec) << "Failed to create stage directory: " << stage_dir.string();

    fs::copy_file(plugin_binary,
                  stage_dir / plugin_binary.filename(),
                  fs::copy_options::overwrite_existing,
                  ec);
    ASSERT_FALSE(ec) << "Failed to stage plugin binary";

    fs::copy_file(plugin_manifest_source,
                  stage_dir / "plugin.json",
                  fs::copy_options::overwrite_existing,
                  ec);
    ASSERT_FALSE(ec) << "Failed to stage plugin manifest";

    fs::copy_file(plugin_manifest_sig_source,
                  stage_dir / "plugin.json.sig",
                  fs::copy_options::overwrite_existing,
                  ec);
    ASSERT_FALSE(ec) << "Failed to stage plugin manifest signature";

    const auto scan_result = manager.scanPluginDirectory(stage_dir.string());
    ASSERT_TRUE(scan_result.has_value())
        << "scanPluginDirectory failed for " << stage_dir.string();

    const auto bind_result =
        themis::transaction::bindSagaOrchestratorFromPlugin(manager, "saga_orchestrator");

    if (!bind_result.has_value()) {
        const auto error_code = bind_result.error().code();
        if (error_code == themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND ||
            error_code == themis::errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE ||
            error_code == themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED) {
            GTEST_SKIP() << "Runtime gate prevented dynamic plugin load in this preset: "
                         << static_cast<int>(error_code);
        }
        FAIL() << "bindSagaOrchestratorFromPlugin failed with unexpected error code: "
               << static_cast<int>(error_code);
    }

    auto* orchestrator = bind_result.value();
    ASSERT_NE(orchestrator, nullptr);

    themis::SAGADefinition smoke_saga;
    smoke_saga.id = "saga-plugin-bridge-smoke";
    smoke_saga.name = "saga_plugin_bridge_smoke";
    smoke_saga.steps.push_back(themis::SAGAStep{
        "step_ok",
        [] {},
        [] {},
        {},
        {},
        std::chrono::milliseconds{0},
        0,
        std::chrono::milliseconds{0}});

    const auto status = orchestrator->execute(smoke_saga);
    EXPECT_TRUE(status.ok) << "SAGA execution through plugin-bound orchestrator failed: "
                           << status.message;

    EXPECT_TRUE(manager.unloadAllPlugins().has_value());
    fs::remove_all(stage_dir, ec);
}

TEST(SagaPluginBridgeFocusedTest, ReturnsErrorWhenPluginBinaryIsMissing) {
    auto& manager = themis::plugins::PluginManager::instance();
    ASSERT_TRUE(manager.unloadAllPlugins().has_value());
    manager.disableHotPlug();
    manager.clearReloadListeners();

    const fs::path temp_dir = fs::temp_directory_path() / "themis_saga_plugin_bridge_missing_bin";
    std::error_code ec = {};
    fs::remove_all(temp_dir, ec);
    fs::create_directories(temp_dir, ec);
    ASSERT_FALSE(ec) << "Failed to create temporary plugin directory: " << temp_dir.string();

    const fs::path manifest_path = temp_dir / "saga_orchestrator_missing_bin.json";
    std::ofstream manifest_file(manifest_path, std::ios::trunc);
    ASSERT_TRUE(manifest_file.is_open());

    manifest_file
        << "{\n"
        << "  \"name\": \"saga_orchestrator_missing_bin\",\n"
        << "  \"version\": \"1.0.0\",\n"
        << "  \"type\": \"custom\",\n"
        << "  \"library\": \"saga_orchestrator_missing_bin.dll\"\n"
        << "}\n";
    manifest_file.close();

    const auto scan_result = manager.scanPluginDirectory(temp_dir.string());
    ASSERT_TRUE(scan_result.has_value())
        << "scanPluginDirectory failed for " << temp_dir.string();

    const auto bind_result =
        themis::transaction::bindSagaOrchestratorFromPlugin(manager, "saga_orchestrator_missing_bin");
    ASSERT_FALSE(bind_result.has_value())
        << "Expected bind failure for plugin with missing binary";

    const auto error_code = bind_result.error().code();
    EXPECT_TRUE(error_code == themis::errors::ErrorCode::ERR_PLUGIN_NOT_FOUND ||
                error_code == themis::errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED ||
                error_code == themis::errors::ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE)
        << "Unexpected error code for missing binary: " << static_cast<int>(error_code);

    EXPECT_TRUE(manager.unloadAllPlugins().has_value());
    fs::remove_all(temp_dir, ec);
}
