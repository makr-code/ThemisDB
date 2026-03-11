/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_importer_plugin_api.cpp                       ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-09 18:09:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     512                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b5d3439    2026-03-09  fix(plugins): correct maturity header — tests are complete   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9a74cf8c  2026-02-28  fix(importers): code audit — cleanup includes, fix docstr... ║
    • c6a24a668  2026-02-28  feat(importers): add Plugin API for third-party importer ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// test_importer_plugin_api.cpp
//
// Unit tests for importers/importer_plugin_api.h covering:
//   - THEMIS_IMPORTER_PLUGIN_API_VERSION compile-time token
//   - ImporterPluginBase: default IThemisPlugin interface methods
//   - ImporterPluginRegistry: registerFactory, create, listPlugins,
//     hasPlugin, unregisterFactory, clear, thread safety
//   - ImporterPluginLoader: error handling for non-existent paths
//   - THEMIS_IMPORTER_PLUGIN_IMPL macro: correct entry-point generation

#include <gtest/gtest.h>

// Enable plugin export (not import) when defining createPlugin/destroyPlugin in this TU
#ifndef THEMIS_PLUGIN_EXPORTS
#define THEMIS_PLUGIN_EXPORTS
#endif
#include "importers/importer_plugin_api.h"

#include <atomic>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <nlohmann/json.hpp>

using namespace themis::importers;
using json = nlohmann::json;

// ============================================================================
// Minimal concrete importer used by multiple tests
// ============================================================================

/**
 * @brief Trivial importer that records calls; no real import logic.
 */
class StubImporter : public ImporterPluginBase {
public:
    // IThemisPlugin identifiers
    const char* getName()    const override { return "stub_importer"; }
    const char* getVersion() const override { return "0.1.0"; }

    // IImporter
    std::vector<std::string> getSupportedTypes() const override {
        return {"stub", "test"};
    }

    bool initialize(const std::string& /*config*/) override {
        initialized_ = true;
        return true;
    }

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override {
        if (source_path.empty()) {
            errors.push_back("source_path must not be empty");
            return false;
        }
        return true;
    }

    ImportStats importData(const std::string& /*source_path*/,
                           const ImportOptions& /*options*/,
                           ProgressCallback /*cb*/ = nullptr) override {
        ImportStats stats;
        stats.total_records    = 3;
        stats.imported_records = 3;
        return stats;
    }

    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& /*source_path*/,
        const ImportOptions& /*options*/) override
    {
        auto handle = std::make_shared<ImportHandle>();
        handle->id = "stub-async-job";
        handle->running.store(true);
        auto promise = std::make_shared<std::promise<ImportStats>>();
        handle->future = promise->get_future().share();
        std::thread([handle, promise]() {
            ImportStats stats;
            stats.total_records    = 3;
            stats.imported_records = 3;
            handle->running.store(false);
            promise->set_value(stats);
        }).detach();
        return handle;
    }

    void cancel() override { cancelled_ = true; }

    json getSourceSchema(const std::string& /*source_path*/) override {
        return json{{"columns", json::array()}};
    }

    bool initialized_ = false;
    bool cancelled_   = false;
};

// ============================================================================
// Fixture: isolate registry state per test
// ============================================================================

class ImporterPluginApiTest : public ::testing::Test {
protected:
    void SetUp() override {
        ImporterPluginRegistry::instance().clear();
    }
    void TearDown() override {
        ImporterPluginRegistry::instance().clear();
    }
};

// ============================================================================
// API version
// ============================================================================

TEST(ImporterPluginApiVersionTest, VersionMacroIsPositive) {
    EXPECT_GT(THEMIS_IMPORTER_PLUGIN_API_VERSION, 0);
}

// ============================================================================
// ImporterPluginBase: default IThemisPlugin methods
// ============================================================================

TEST_F(ImporterPluginApiTest, BaseTypeIsImporter) {
    StubImporter stub;
    EXPECT_EQ(themis::plugins::PluginType::IMPORTER, stub.getType());
}

TEST_F(ImporterPluginApiTest, BaseGetInstanceReturnsIImporterPtr) {
    StubImporter stub;
    void* inst = stub.getInstance();
    ASSERT_NE(nullptr, inst);
    // Must be the same IImporter* (no separate wrapper object)
    auto* as_importer = static_cast<IImporter*>(inst);
    EXPECT_EQ(static_cast<IImporter*>(&stub), as_importer);
}

TEST_F(ImporterPluginApiTest, BaseDefaultCapabilitiesAreFalse) {
    StubImporter stub;
    auto caps = stub.getCapabilities();
    EXPECT_FALSE(caps.supports_streaming);
    EXPECT_FALSE(caps.supports_batching);
    EXPECT_FALSE(caps.supports_transactions);
    EXPECT_FALSE(caps.thread_safe);
    EXPECT_FALSE(caps.gpu_accelerated);
}

TEST_F(ImporterPluginApiTest, BaseInitializeCharPtrDelegatesToString) {
    StubImporter stub;
    EXPECT_TRUE(stub.initialize("{}"));
    EXPECT_TRUE(stub.initialized_);
}

TEST_F(ImporterPluginApiTest, BaseInitializeNullptrIsSafe) {
    StubImporter stub;
    // nullptr should not crash; delegates to initialize(std::string{})
    EXPECT_TRUE(stub.initialize(nullptr));
}

TEST_F(ImporterPluginApiTest, BaseShutdownIsNoOp) {
    StubImporter stub;
    // Must not throw or crash
    EXPECT_NO_THROW(stub.shutdown());
}

TEST_F(ImporterPluginApiTest, GetNameAndVersionDelegateToConcreteClass) {
    StubImporter stub;
    EXPECT_STREQ("stub_importer", stub.getName());
    EXPECT_STREQ("0.1.0",         stub.getVersion());
}

// ============================================================================
// ImporterPluginRegistry
// ============================================================================

TEST_F(ImporterPluginApiTest, RegisterAndCreatePlugin) {
    ImporterPluginRegistry::instance().registerFactory(
        "stub_importer",
        []() -> std::shared_ptr<IImporter> {
            return std::make_shared<StubImporter>();
        });

    auto inst = ImporterPluginRegistry::instance().create("stub_importer");
    ASSERT_NE(nullptr, inst);

    ImportStats stats = inst->importData("/any/path", ImportOptions{});
    EXPECT_EQ(3u, stats.imported_records);
}

TEST_F(ImporterPluginApiTest, CreateUnregisteredPluginReturnsNullptr) {
    auto inst = ImporterPluginRegistry::instance().create("does_not_exist");
    EXPECT_EQ(nullptr, inst);
}

TEST_F(ImporterPluginApiTest, ListPluginsReflectsRegistrations) {
    EXPECT_TRUE(ImporterPluginRegistry::instance().listPlugins().empty());

    ImporterPluginRegistry::instance().registerFactory(
        "alpha", []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });
    ImporterPluginRegistry::instance().registerFactory(
        "beta",  []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });

    auto names = ImporterPluginRegistry::instance().listPlugins();
    EXPECT_EQ(2u, names.size());
    EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "alpha"));
    EXPECT_NE(names.end(), std::find(names.begin(), names.end(), "beta"));
}

TEST_F(ImporterPluginApiTest, HasPluginReturnsTrueAfterRegistration) {
    EXPECT_FALSE(ImporterPluginRegistry::instance().hasPlugin("stub_importer"));

    ImporterPluginRegistry::instance().registerFactory(
        "stub_importer",
        []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });

    EXPECT_TRUE(ImporterPluginRegistry::instance().hasPlugin("stub_importer"));
}

TEST_F(ImporterPluginApiTest, UnregisterRemovesFactory) {
    ImporterPluginRegistry::instance().registerFactory(
        "stub_importer",
        []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });

    EXPECT_TRUE(ImporterPluginRegistry::instance().hasPlugin("stub_importer"));
    ImporterPluginRegistry::instance().unregisterFactory("stub_importer");
    EXPECT_FALSE(ImporterPluginRegistry::instance().hasPlugin("stub_importer"));

    // create() must return nullptr after unregister
    EXPECT_EQ(nullptr, ImporterPluginRegistry::instance().create("stub_importer"));
}

TEST_F(ImporterPluginApiTest, UnregisterNonExistentIsNoop) {
    EXPECT_NO_THROW(ImporterPluginRegistry::instance().unregisterFactory("no_such_plugin"));
}

TEST_F(ImporterPluginApiTest, RegisterOverwritesPreviousFactory) {
    // Register a factory that returns an importer reporting 5 records.
    struct FiveRecordImporter : StubImporter {
        ImportStats importData(const std::string&, const ImportOptions&,
                               ProgressCallback = nullptr) override {
            ImportStats s;
            s.total_records    = 5;
            s.imported_records = 5;
            return s;
        }
    };

    ImporterPluginRegistry::instance().registerFactory(
        "overwrite_test",
        []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });
    ImporterPluginRegistry::instance().registerFactory(
        "overwrite_test",
        []() -> std::shared_ptr<IImporter> { return std::make_shared<FiveRecordImporter>(); });

    auto inst = ImporterPluginRegistry::instance().create("overwrite_test");
    ASSERT_NE(nullptr, inst);
    auto stats = inst->importData("/path", ImportOptions{});
    EXPECT_EQ(5u, stats.imported_records);
}

TEST_F(ImporterPluginApiTest, ClearRemovesAllFactories) {
    ImporterPluginRegistry::instance().registerFactory(
        "a", []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });
    ImporterPluginRegistry::instance().registerFactory(
        "b", []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });

    ImporterPluginRegistry::instance().clear();
    EXPECT_TRUE(ImporterPluginRegistry::instance().listPlugins().empty());
}

TEST_F(ImporterPluginApiTest, FactoryProducesIndependentInstances) {
    ImporterPluginRegistry::instance().registerFactory(
        "stub_importer",
        []() -> std::shared_ptr<IImporter> { return std::make_shared<StubImporter>(); });

    auto a = ImporterPluginRegistry::instance().create("stub_importer");
    auto b = ImporterPluginRegistry::instance().create("stub_importer");
    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_NE(a.get(), b.get());
}

// ============================================================================
// ImporterPluginRegistry: basic thread safety
// ============================================================================

TEST_F(ImporterPluginApiTest, ConcurrentRegistrationAndCreation) {
    constexpr int kThreads = 8;
    std::vector<std::future<void>> futures;
    futures.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        futures.push_back(std::async(std::launch::async, [i]() {
            std::string name = "plugin_" + std::to_string(i);
            ImporterPluginRegistry::instance().registerFactory(
                name,
                []() -> std::shared_ptr<IImporter> {
                    return std::make_shared<StubImporter>();
                });
            auto inst = ImporterPluginRegistry::instance().create(name);
            EXPECT_NE(nullptr, inst);
        }));
    }

    for (auto& f : futures) {
        EXPECT_NO_THROW(f.get());
    }

    EXPECT_EQ(kThreads, static_cast<int>(
        ImporterPluginRegistry::instance().listPlugins().size()));
}

// ============================================================================
// ImporterPluginLoader: error handling (no real .so loaded)
// ============================================================================

TEST_F(ImporterPluginApiTest, LoaderInitiallyNotLoaded) {
    ImporterPluginLoader loader;
    EXPECT_FALSE(loader.isLoaded());
    EXPECT_TRUE(loader.loadedName().empty());
}

TEST_F(ImporterPluginApiTest, LoaderReturnsFalseForNonExistentPath) {
    ImporterPluginLoader loader;
    bool ok = loader.load("/nonexistent/path/to/plugin.so");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(loader.isLoaded());
    EXPECT_FALSE(loader.lastError().empty());
}

TEST_F(ImporterPluginApiTest, LoaderUnloadWhenNotLoadedIsNoop) {
    ImporterPluginLoader loader;
    EXPECT_NO_THROW(loader.unload());
    EXPECT_FALSE(loader.isLoaded());
}

TEST_F(ImporterPluginApiTest, LoaderMoveConstructor) {
    ImporterPluginLoader a;
    // Put loader in a known error state to verify state transfers
    a.load("/nonexistent/path.so");
    std::string err_a = a.lastError();

    ImporterPluginLoader b(std::move(a));
    EXPECT_FALSE(b.isLoaded());
    EXPECT_EQ(err_a, b.lastError());
}

TEST_F(ImporterPluginApiTest, LoaderMoveAssignment) {
    ImporterPluginLoader a;
    a.load("/nonexistent/path.so");
    std::string err_a = a.lastError();

    ImporterPluginLoader b;
    b = std::move(a);
    EXPECT_EQ(err_a, b.lastError());
}

// ============================================================================
// THEMIS_IMPORTER_PLUGIN_IMPL macro
// ============================================================================

// Verify that the macro produces the correct C-linkage entry points.
// We define a second trivial plugin class in this translation unit and
// apply the macro, then call the generated functions directly.

class MacroTestImporter final : public ImporterPluginBase {
public:
    const char* getName()    const override { return "macro_test_importer"; }
    const char* getVersion() const override { return "0.0.1"; }

    std::vector<std::string> getSupportedTypes() const override { return {"test"}; }
    bool initialize(const std::string&) override { return true; }
    bool validateSource(const std::string&, std::vector<std::string>&) override { return true; }

    ImportStats importData(const std::string&, const ImportOptions&,
                           ProgressCallback = nullptr) override { return {}; }
    std::shared_ptr<ImportHandle> importDataAsync(const std::string&,
                                                  const ImportOptions&) override {
        return std::make_shared<ImportHandle>();
    }
    void cancel() override {}
    json getSourceSchema(const std::string&) override { return json::object(); }
};

// Apply the macro to generate the C-linkage entry points used by
// ImporterPluginLoader.  Note: THEMIS_IMPORTER_PLUGIN_IMPL expands to
// `extern "C" { ... }`, so `createPlugin` / `destroyPlugin` receive
// C (external) linkage regardless of any enclosing namespace.  The test
// binary must not link any other translation unit that also defines these
// two symbols with C linkage (e.g. postgres_importer.cpp's own plugin
// entry points must not be linked into the same executable).
#ifndef THEMIS_PLUGIN_EXPORTS
#define THEMIS_PLUGIN_EXPORTS
#endif
THEMIS_IMPORTER_PLUGIN_IMPL(MacroTestImporter)

TEST_F(ImporterPluginApiTest, MacroCreatesCorrectPlugin) {
    // createPlugin / destroyPlugin are defined at file scope by THEMIS_IMPORTER_PLUGIN_IMPL
    auto* plugin_raw = createPlugin();
    ASSERT_NE(nullptr, plugin_raw);
    EXPECT_STREQ("macro_test_importer", plugin_raw->getName());
    EXPECT_EQ(themis::plugins::PluginType::IMPORTER, plugin_raw->getType());

    void* inst = plugin_raw->getInstance();
    ASSERT_NE(nullptr, inst);
    auto* importer_ptr = static_cast<IImporter*>(inst);
    EXPECT_NE(nullptr, importer_ptr);

    destroyPlugin(plugin_raw);
}

TEST_F(ImporterPluginApiTest, MacroDestroyPluginIsNullSafe) {
    // Passing nullptr must not crash (implementation uses `delete nullptr`)
    EXPECT_NO_THROW(destroyPlugin(nullptr));
}

// ============================================================================
// ImporterPluginDescriptor struct
// ============================================================================

TEST_F(ImporterPluginApiTest, DescriptorDefaultConstruction) {
    ImporterPluginDescriptor desc;
    EXPECT_TRUE(desc.name.empty());
    EXPECT_TRUE(desc.version.empty());
    EXPECT_TRUE(desc.supported_types.empty());
    EXPECT_FALSE(desc.capabilities.supports_streaming);
}

TEST_F(ImporterPluginApiTest, DescriptorFieldAssignment) {
    ImporterPluginDescriptor desc;
    desc.name    = "my_importer";
    desc.version = "2.3.0";
    desc.supported_types = {"csv", "tsv"};
    desc.capabilities.supports_streaming = true;

    EXPECT_EQ("my_importer", desc.name);
    EXPECT_EQ("2.3.0",       desc.version);
    ASSERT_EQ(2u,            desc.supported_types.size());
    EXPECT_EQ("csv",         desc.supported_types[0]);
    EXPECT_TRUE(desc.capabilities.supports_streaming);
}

// ============================================================================
// StubImporter: validate IImporter contract through ImporterPluginBase
// ============================================================================

TEST_F(ImporterPluginApiTest, ValidateSourceEmptyPathFails) {
    StubImporter stub;
    std::vector<std::string> errors;
    EXPECT_FALSE(stub.validateSource("", errors));
    EXPECT_FALSE(errors.empty());
}

TEST_F(ImporterPluginApiTest, ValidateSourceNonEmptyPathSucceeds) {
    StubImporter stub;
    std::vector<std::string> errors;
    EXPECT_TRUE(stub.validateSource("/some/path.sql", errors));
    EXPECT_TRUE(errors.empty());
}

TEST_F(ImporterPluginApiTest, ImportDataReturnsStats) {
    StubImporter stub;
    ImportOptions opts;
    auto stats = stub.importData("/path", opts);
    EXPECT_EQ(3u, stats.total_records);
    EXPECT_EQ(3u, stats.imported_records);
}

TEST_F(ImporterPluginApiTest, CancelSetsCancelledFlag) {
    StubImporter stub;
    EXPECT_FALSE(stub.cancelled_);
    stub.cancel();
    EXPECT_TRUE(stub.cancelled_);
}

TEST_F(ImporterPluginApiTest, GetSourceSchemaReturnsJson) {
    StubImporter stub;
    auto schema = stub.getSourceSchema("/path");
    EXPECT_TRUE(schema.is_object());
    EXPECT_TRUE(schema.contains("columns"));
}

TEST_F(ImporterPluginApiTest, ImportDataStreamingDefaultDelegatesToImportData) {
    StubImporter stub;
    ImportOptions opts;
    std::vector<std::string> received_tables;
    auto stats = stub.importDataStreaming("/path", opts,
        [&](const std::string& table, const json&) -> bool {
            received_tables.push_back(table);
            return true;
        });
    // Default implementation in IImporter delegates; stub records 3 rows
    EXPECT_EQ(3u, stats.imported_records);
}
