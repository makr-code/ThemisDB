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

// ============================================================================
// PluginSandboxConfig defaults
// ============================================================================

TEST(PluginSandboxConfigTest, DefaultMemoryLimitIs256MiB) {
    PluginSandboxConfig cfg;
    EXPECT_EQ(256UL * 1024UL * 1024UL, cfg.memory_limit_bytes);
}

TEST(PluginSandboxConfigTest, DefaultTimeoutIs5Minutes) {
    PluginSandboxConfig cfg;
    EXPECT_EQ(300'000u, cfg.timeout_ms);
}

TEST(PluginSandboxConfigTest, ZeroDisablesLimits) {
    PluginSandboxConfig cfg;
    cfg.memory_limit_bytes = 0;
    cfg.timeout_ms         = 0;
    EXPECT_EQ(0u, cfg.memory_limit_bytes);
    EXPECT_EQ(0u, cfg.timeout_ms);
}

// ============================================================================
// importer_plugin.h — C ABI constants
// ============================================================================

TEST(ImporterPluginAbiTest, AbiVersionV1IsOne) {
    EXPECT_EQ(1u, static_cast<unsigned>(THEMIS_IMPORTER_PLUGIN_ABI_V1));
}

TEST(ImporterPluginAbiTest, CreateSymbolIsCorrect) {
    EXPECT_STREQ("themis_importer_create", THEMIS_IMPORTER_CREATE_SYMBOL);
}

TEST(ImporterPluginAbiTest, StructSizeIsNonZero) {
    EXPECT_GT(sizeof(THEMIS_IMPORTER_PLUGIN_V1), 0u);
}

TEST(ImporterPluginAbiTest, AllocatorNullFieldsAreZeroInitialised) {
    ThemisImporterAllocator alloc{};
    EXPECT_EQ(nullptr, alloc.alloc);
    EXPECT_EQ(nullptr, alloc.free);
    EXPECT_EQ(nullptr, alloc.user_data);
}

TEST(ImporterPluginAbiTest, V1StructZeroInitialisedHasNullFunctionPointers) {
    THEMIS_IMPORTER_PLUGIN_V1 desc{};
    EXPECT_EQ(0u,     desc.abi_version);
    EXPECT_EQ(0u,     desc.struct_size);
    EXPECT_EQ(nullptr, desc.name);
    EXPECT_EQ(nullptr, desc.create_instance);
    EXPECT_EQ(nullptr, desc.destroy_instance);
    EXPECT_EQ(nullptr, desc.initialize);
    EXPECT_EQ(nullptr, desc.import_data);
}

// ============================================================================
// ImporterPluginRegistry::loadPlugin() error paths
// ============================================================================

TEST_F(ImporterPluginApiTest, LoadPluginReturnsFalseForNonExistentPath) {
    bool ok = ImporterPluginRegistry::instance()
                  .loadPlugin("/nonexistent/oracle_importer.so");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(ImporterPluginRegistry::instance().lastLoadError().empty());
}

TEST_F(ImporterPluginApiTest, LoadPluginDoesNotRegisterOnFailure) {
    ImporterPluginRegistry::instance()
        .loadPlugin("/nonexistent/oracle_importer.so");
    // Registry should remain empty after a failed load
    EXPECT_TRUE(ImporterPluginRegistry::instance().listPlugins().empty());
}

TEST_F(ImporterPluginApiTest, LastLoadErrorIsEmptyBeforeAnyLoad) {
    // fresh registry via clear() in SetUp
    EXPECT_TRUE(ImporterPluginRegistry::instance().lastLoadError().empty());
}

TEST_F(ImporterPluginApiTest, UnloadPluginForUnknownNameIsNoop) {
    EXPECT_NO_THROW(
        ImporterPluginRegistry::instance().unloadPlugin("nonexistent_plugin"));
    EXPECT_TRUE(ImporterPluginRegistry::instance().listPlugins().empty());
}

TEST_F(ImporterPluginApiTest, LoadPluginSandboxWithZeroLimitsIsAccepted) {
    PluginSandboxConfig cfg;
    cfg.memory_limit_bytes = 0;
    cfg.timeout_ms         = 0;
    // Will fail at dlopen (no such file), but sandbox config is accepted
    bool ok = ImporterPluginRegistry::instance()
                  .loadPlugin("/nonexistent/plugin.so", cfg);
    EXPECT_FALSE(ok);
}

// ============================================================================
// ImporterRegistry alias
// ============================================================================

TEST_F(ImporterPluginApiTest, ImporterRegistryAliasIsSameSingleton) {
    // Both names must refer to the same singleton instance
    EXPECT_EQ(&ImporterRegistry::instance(),
              &ImporterPluginRegistry::instance());
}

TEST_F(ImporterPluginApiTest, ImporterRegistryLoadPluginFailsForBadPath) {
    bool ok = ImporterRegistry::instance()
                  .loadPlugin("/no/such/plugin.so");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(ImporterRegistry::instance().lastLoadError().empty());
}

// ============================================================================
// V1ImporterAdapter: basic lifecycle
// ============================================================================

// Build a minimal but complete THEMIS_IMPORTER_PLUGIN_V1 descriptor in-process
// to exercise V1ImporterAdapter without a real shared library.

namespace {

struct TestPluginState {
    bool initialized = false;
    bool cancelled   = false;
    bool destroyed   = false;
    int  import_rc   = 0;        ///< Return code for import_data
    uint64_t import_records = 5; ///< Records to report
    std::string schema_json;
};

static void* testV1Create(const ThemisImporterAllocator* /*alloc*/) {
    return new TestPluginState{};
}
static void testV1Destroy(void* p, const ThemisImporterAllocator* /*alloc*/) {
    delete static_cast<TestPluginState*>(p);
}
static int testV1Init(void* p, const char* /*cfg*/) {
    static_cast<TestPluginState*>(p)->initialized = true;
    return 0;
}
static int testV1Validate(void* /*p*/, const char* src,
                          char* err_buf, size_t err_size) {
    if (!src || src[0] == '\0') {
        if (err_buf && err_size > 0) {
            std::snprintf(err_buf, err_size, "source_path must not be empty");
        }
        return 1;
    }
    return 0;
}
static int testV1Import(void* p, const char* /*src*/, const char* /*opts*/,
                        uint64_t* imported, uint64_t* failed) {
    auto* state = static_cast<TestPluginState*>(p);
    if (imported) *imported = state->import_rc == 0 ? state->import_records : 0;
    if (failed)   *failed   = state->import_rc == 0 ? 0 : state->import_records;
    return state->import_rc;
}
static const char* testV1Schema(void* p, const char* /*src*/) {
    auto* state = static_cast<TestPluginState*>(p);
    if (state->schema_json.empty()) return nullptr;
    return state->schema_json.c_str();
}
static void testV1Cancel(void* p) {
    static_cast<TestPluginState*>(p)->cancelled = true;
}

static const THEMIS_IMPORTER_PLUGIN_V1 kTestDescriptor = {
    THEMIS_IMPORTER_PLUGIN_ABI_V1,
    static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
    "test_v1_plugin",
    "1.0.0",
    &testV1Create,
    &testV1Destroy,
    &testV1Init,
    &testV1Validate,
    &testV1Import,
    &testV1Schema,
    &testV1Cancel,
    {nullptr, nullptr, nullptr, nullptr}
};

} // anonymous namespace

class V1AdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ImporterPluginRegistry::instance().clear();
    }
    void TearDown() override {
        ImporterPluginRegistry::instance().clear();
    }
};

TEST_F(V1AdapterTest, AdapterGetNameReturnsPluginName) {
    V1ImporterAdapter adapter(&kTestDescriptor, PluginSandboxConfig{});
    EXPECT_STREQ("test_v1_plugin", adapter.getName());
}

TEST_F(V1AdapterTest, AdapterInitializeCallsV1Init) {
    V1ImporterAdapter adapter(&kTestDescriptor, PluginSandboxConfig{});
    EXPECT_TRUE(adapter.initialize("{}"));
}

TEST_F(V1AdapterTest, AdapterValidateSourceEmptyPathFails) {
    V1ImporterAdapter adapter(&kTestDescriptor, PluginSandboxConfig{});
    std::vector<std::string> errors;
    EXPECT_FALSE(adapter.validateSource("", errors));
    EXPECT_FALSE(errors.empty());
}

TEST_F(V1AdapterTest, AdapterValidateSourceNonEmptyPathSucceeds) {
    V1ImporterAdapter adapter(&kTestDescriptor, PluginSandboxConfig{});
    std::vector<std::string> errors;
    EXPECT_TRUE(adapter.validateSource("/some/path", errors));
    EXPECT_TRUE(errors.empty());
}

TEST_F(V1AdapterTest, AdapterImportDataReturnsStats) {
    PluginSandboxConfig cfg;
    cfg.timeout_ms = 0;  // disable timeout for this test
    V1ImporterAdapter adapter(&kTestDescriptor, cfg);
    ImportStats stats = adapter.importData("/path", ImportOptions{});
    EXPECT_EQ(5u, stats.imported_records);
    EXPECT_EQ(0u, stats.failed_records);
    EXPECT_TRUE(stats.errors.empty());
}

TEST_F(V1AdapterTest, AdapterImportDataRecordsErrorOnNonZeroReturnCode) {
    // Override import_rc in the state by using a custom descriptor
    static TestPluginState gState;
    gState = TestPluginState{};          // reset to default state each run
    gState.import_rc = 42;
    static const THEMIS_IMPORTER_PLUGIN_V1 kErrDescriptor = {
        THEMIS_IMPORTER_PLUGIN_ABI_V1,
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
        "err_v1_plugin", "1.0.0",
        [](const ThemisImporterAllocator*) -> void* { return &gState; },
        [](void*, const ThemisImporterAllocator*) {},
        testV1Init, testV1Validate, testV1Import,
        testV1Schema, testV1Cancel,
        {nullptr, nullptr, nullptr, nullptr}
    };
    PluginSandboxConfig cfg;
    cfg.timeout_ms = 0;
    V1ImporterAdapter adapter(&kErrDescriptor, cfg);
    ImportStats stats = adapter.importData("/path", ImportOptions{});
    EXPECT_FALSE(stats.errors.empty());
}

TEST_F(V1AdapterTest, AdapterGetSchemaReturnsJsonObject) {
    // Override schema in state
    static TestPluginState gSchemaState;
    gSchemaState = TestPluginState{};    // reset to default state each run
    gSchemaState.schema_json = R"({"tables":[]})";
    static const THEMIS_IMPORTER_PLUGIN_V1 kSchemaDescriptor = {
        THEMIS_IMPORTER_PLUGIN_ABI_V1,
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
        "schema_v1_plugin", "1.0.0",
        [](const ThemisImporterAllocator*) -> void* { return &gSchemaState; },
        [](void*, const ThemisImporterAllocator*) {},
        testV1Init, testV1Validate, testV1Import,
        testV1Schema, testV1Cancel,
        {nullptr, nullptr, nullptr, nullptr}
    };
    V1ImporterAdapter adapter(&kSchemaDescriptor, PluginSandboxConfig{});
    json schema = adapter.getSourceSchema("/any");
    EXPECT_TRUE(schema.is_object());
    EXPECT_TRUE(schema.contains("tables"));
}

TEST_F(V1AdapterTest, AdapterGetSchemaReturnsEmptyObjectWhenNullptr) {
    V1ImporterAdapter adapter(&kTestDescriptor, PluginSandboxConfig{});
    // kTestDescriptor's get_schema returns nullptr for empty schema_json
    json schema = adapter.getSourceSchema("/any");
    EXPECT_TRUE(schema.is_object());
}

TEST_F(V1AdapterTest, AdapterCancelCallsV1Cancel) {
    static TestPluginState gCancelState;
    gCancelState = TestPluginState{};   // reset: cancelled must start as false
    static const THEMIS_IMPORTER_PLUGIN_V1 kCancelDescriptor = {
        THEMIS_IMPORTER_PLUGIN_ABI_V1,
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
        "cancel_v1_plugin", "1.0.0",
        [](const ThemisImporterAllocator*) -> void* { return &gCancelState; },
        [](void*, const ThemisImporterAllocator*) {},
        testV1Init, testV1Validate, testV1Import,
        testV1Schema, testV1Cancel,
        {nullptr, nullptr, nullptr, nullptr}
    };
    V1ImporterAdapter adapter(&kCancelDescriptor, PluginSandboxConfig{});
    EXPECT_FALSE(gCancelState.cancelled);
    adapter.cancel();
    EXPECT_TRUE(gCancelState.cancelled);
}

TEST_F(V1AdapterTest, AdapterImportDataAsyncCompletesSuccessfully) {
    PluginSandboxConfig cfg;
    cfg.timeout_ms = 0;
    auto adapter = std::make_shared<V1ImporterAdapter>(&kTestDescriptor, cfg);
    auto handle = adapter->importDataAsync("/path", ImportOptions{});
    ASSERT_NE(nullptr, handle);
    // Wait for the async job
    auto stats = handle->future.get();
    EXPECT_EQ(5u, stats.imported_records);
}

// ============================================================================
// Sandbox: memory limit tracking
// ============================================================================

TEST_F(V1AdapterTest, SandboxAllocatorTracksAllocationsBelowLimit) {
    // Verify that the counting allocator accepts allocations below the limit
    // and correctly decrements the counter when memory is freed.
    // We test the allocator directly via a descriptor whose create_instance
    // uses the provided allocator.

    struct AllocTrackState {
        void*  ptr_from_alloc = nullptr;
        bool   alloc_succeeded = false;
    };
    static AllocTrackState gTrackState;
    gTrackState = AllocTrackState{};    // reset each run

    static const THEMIS_IMPORTER_PLUGIN_V1 kTrackDescriptor = {
        THEMIS_IMPORTER_PLUGIN_ABI_V1,
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
        "track_alloc_plugin", "1.0.0",
        [](const ThemisImporterAllocator* alloc) -> void* {
            if (alloc && alloc->alloc) {
                // Allocate 32 bytes — within the 1 KiB limit
                gTrackState.ptr_from_alloc = alloc->alloc(32, alloc->user_data);
                gTrackState.alloc_succeeded = (gTrackState.ptr_from_alloc != nullptr);
                // Free it back so the counter returns to 0
                if (gTrackState.ptr_from_alloc && alloc->free) {
                    alloc->free(gTrackState.ptr_from_alloc, alloc->user_data);
                    gTrackState.ptr_from_alloc = nullptr;
                }
            }
            return new AllocTrackState{};
        },
        [](void* p, const ThemisImporterAllocator*) { delete static_cast<AllocTrackState*>(p); },
        testV1Init, testV1Validate, testV1Import,
        testV1Schema, testV1Cancel,
        {nullptr, nullptr, nullptr, nullptr}
    };

    PluginSandboxConfig cfg;
    cfg.memory_limit_bytes = 1024;  // 1 KiB — 32-byte alloc fits
    cfg.timeout_ms = 0;
    V1ImporterAdapter adapter(&kTrackDescriptor, cfg);
    EXPECT_TRUE(gTrackState.alloc_succeeded);
    // After free, bytes_used should be 0 again (no limit exceeded)
    ImportStats stats = adapter.importData("/path", ImportOptions{});
    EXPECT_TRUE(stats.errors.empty());  // no OOM error
}

TEST_F(V1AdapterTest, SandboxAllocatorRejectsAllocationsBeyondLimit) {
    // Verify that the counting allocator returns nullptr when the memory
    // limit is exceeded.

    struct OomCheckState {
        bool alloc_failed_as_expected = false;
    };
    static OomCheckState gOomState;
    gOomState = OomCheckState{};        // reset each run

    static const THEMIS_IMPORTER_PLUGIN_V1 kOomDescriptor = {
        THEMIS_IMPORTER_PLUGIN_ABI_V1,
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),
        "oom_plugin", "1.0.0",
        [](const ThemisImporterAllocator* alloc) -> void* {
            if (alloc && alloc->alloc) {
                // Try to allocate 1 MiB — exceeds the 10-byte limit
                void* p = alloc->alloc(1024 * 1024, alloc->user_data);
                gOomState.alloc_failed_as_expected = (p == nullptr);
                if (p && alloc->free) alloc->free(p, alloc->user_data);
            }
            return new OomCheckState{};
        },
        [](void* p, const ThemisImporterAllocator*) { delete static_cast<OomCheckState*>(p); },
        testV1Init, testV1Validate, testV1Import,
        testV1Schema, testV1Cancel,
        {nullptr, nullptr, nullptr, nullptr}
    };

    PluginSandboxConfig cfg;
    cfg.memory_limit_bytes = 10;  // 10 bytes — 1 MiB allocation must fail
    cfg.timeout_ms = 0;
    V1ImporterAdapter adapter(&kOomDescriptor, cfg);
    EXPECT_TRUE(gOomState.alloc_failed_as_expected);
}

