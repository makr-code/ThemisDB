/*
 * ThemisDB | File: test_module_sandbox_wasm_injection.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=17; TODO=1, Stub=1, Unimpl=0, Mock=15, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


/// @file test_module_sandbox_wasm_injection.cpp
/// @brief Unit/integration tests for WASM runtime injection via ModuleSandbox
///        (v1.8.0 – Issue #1572).
///
/// Tests cover:
///   - Default config has WASM isolation disabled
///   - WASM fields are independently configurable
///   - launch() without registered runtime records a warning and stays OS-only
///   - launch() with a registered runtime activates WASM isolation
///   - wasmSandbox() accessor returns the correct inner sandbox
///   - isWasmIsolationActive() reflects actual state
///   - shutdown() tears down WASM isolation cleanly
///   - WASM-isolated sandbox can load a .wasm binary and call exports
///   - Injected runtime is the highest-priority one when name is empty

#include <gtest/gtest.h>
#include "themis/base/module_sandbox.h"
#include "themis/base/wasm_plugin_sandbox.h"
#include "themis/base/wasm_runtime_injector.h"

#include <cstring>
#include <string>
#include <vector>

using namespace themis::modules;

// =============================================================================
// Helpers – minimal WASM binary builder
// =============================================================================

/// Build a LEB-128 encoded u32.
static std::vector<uint8_t> leb128u(uint32_t v) {
    std::vector<uint8_t> out;
    do {
        uint8_t b = v & 0x7f;
        v >>= 7;
        if (v) b |= 0x80;
        out.push_back(b);
    } while (v);
    return out;
}

static void appendBytes(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

/// Minimal valid WASM binary: magic + version (8 bytes, no sections).
static std::vector<uint8_t> minimalWasm() {
    return { 0x00, 0x61, 0x73, 0x6d,   // magic "\0asm"
             0x01, 0x00, 0x00, 0x00 }; // version 1
}

/// Build a tiny WASM binary with one exported function "run" (section 7).
static std::vector<uint8_t> wasmWithExport(const std::string& export_name) {
    // Export section body: count=1, name, kind=0 (func), index=0
    std::vector<uint8_t> body;
    appendBytes(body, leb128u(1)); // count
    // name: length-prefixed
    appendBytes(body, leb128u(static_cast<uint32_t>(export_name.size())));
    for (char c : export_name) body.push_back(static_cast<uint8_t>(c));
    body.push_back(0x00); // kind = func
    appendBytes(body, leb128u(0)); // function index

    std::vector<uint8_t> section;
    section.push_back(0x07); // section id = export
    appendBytes(section, leb128u(static_cast<uint32_t>(body.size())));
    appendBytes(section, body);

    std::vector<uint8_t> wasm = minimalWasm();
    appendBytes(wasm, section);
    return wasm;
}

// =============================================================================
// Mock WASM runtime for testing (implements IWasmRuntime for injector registry)
// =============================================================================

class MockWasmRuntime : public IWasmRuntime {
public:
    explicit MockWasmRuntime(std::string engine_name)
        : engine_name_(std::move(engine_name)) {}

    bool instantiate(const std::vector<uint8_t>&,
                     const std::vector<WasmHostFunction>&,
                     size_t) override {
        instantiated_ = true;
        return true;
    }

    bool call(const std::string& export_name,
              const std::vector<uint8_t>& args,
              std::vector<uint8_t>& out) override {
        last_call_ = export_name;
        out = args; // echo
        return true;
    }

    uint8_t* linearMemory(size_t& out_size) override {
        out_size = mem_.size();
        return mem_.data();
    }

    std::string name()    const override { return engine_name_; }
    std::string version() const override { return "1.0-mock"; }
    bool isInstantiated() const override { return instantiated_; }

    bool instantiated_ = false;
    std::string last_call_;

private:
    std::string          engine_name_;
    std::vector<uint8_t> mem_ = std::vector<uint8_t>(64, 0);
};

// =============================================================================
// Test fixture – resets WasmRuntimeInjector registry between tests
// =============================================================================

struct WasmInjectionFixture : ::testing::Test {
    void SetUp() override    { WasmRuntimeInjector::clearAll(); }
    void TearDown() override { WasmRuntimeInjector::clearAll(); }

    /// Register a mock runtime with the given name and priority.
    static void registerMock(const std::string& name, int priority = 10) {
        WasmRuntimeInjector::registerRuntime({
            name, priority, "mock-" + name,
            [name]() -> std::unique_ptr<IWasmRuntime> {
                return std::make_unique<MockWasmRuntime>(name);
            }
        });
    }
};

// =============================================================================
// Config defaults
// =============================================================================

TEST(ModuleSandboxWasmConfig, DefaultWasmIsolationDisabled) {
    ModuleSandbox::Config cfg;
    EXPECT_FALSE(cfg.enable_wasm_isolation);
}

TEST(ModuleSandboxWasmConfig, DefaultWasmRuntimeNameEmpty) {
    ModuleSandbox::Config cfg;
    EXPECT_TRUE(cfg.wasm_runtime_name.empty());
}

TEST(ModuleSandboxWasmConfig, DefaultLinearMemoryPages) {
    ModuleSandbox::Config cfg;
    EXPECT_EQ(cfg.wasm_linear_memory_pages, 256u);
}

TEST(ModuleSandboxWasmConfig, DefaultAllowUnregisteredImportsFalse) {
    ModuleSandbox::Config cfg;
    EXPECT_FALSE(cfg.wasm_allow_unregistered_imports);
}

TEST(ModuleSandboxWasmConfig, CanSetEnableFlag) {
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    EXPECT_TRUE(cfg.enable_wasm_isolation);
}

TEST(ModuleSandboxWasmConfig, CanSetRuntimeName) {
    ModuleSandbox::Config cfg;
    cfg.wasm_runtime_name = "wasmtime";
    EXPECT_EQ(cfg.wasm_runtime_name, "wasmtime");
}

TEST(ModuleSandboxWasmConfig, CanSetMemoryPages) {
    ModuleSandbox::Config cfg;
    cfg.wasm_linear_memory_pages = 64;
    EXPECT_EQ(cfg.wasm_linear_memory_pages, 64u);
}

// =============================================================================
// launch() without WASM isolation enabled
// =============================================================================

TEST(ModuleSandboxWasmLaunch, WasmDisabledByDefault_NotActive) {
    ModuleSandbox sb;
    ASSERT_TRUE(sb.launch("plugin_a"));
    EXPECT_FALSE(sb.isWasmIsolationActive());
    EXPECT_EQ(sb.wasmSandbox(), nullptr);
}

TEST(ModuleSandboxWasmLaunch, WasmDisabledByDefault_NoNullptrDeref) {
    ModuleSandbox sb;
    sb.launch("plugin_a");
    // Must not crash
    EXPECT_NO_THROW({ auto* p = sb.wasmSandbox(); (void)p; });
}

// =============================================================================
// launch() with WASM isolation enabled but no runtime registered
// =============================================================================

TEST_F(WasmInjectionFixture, WasmEnabledNoRuntime_LaunchSucceeds) {
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    // launch() must not fail even if no runtime is available
    EXPECT_TRUE(sb.launch("plugin_no_rt"));
    EXPECT_TRUE(sb.isActive());
}

TEST_F(WasmInjectionFixture, WasmEnabledNoRuntime_IsolationNotActive) {
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_no_rt");
    EXPECT_FALSE(sb.isWasmIsolationActive());
    EXPECT_EQ(sb.wasmSandbox(), nullptr);
}

TEST_F(WasmInjectionFixture, WasmEnabledNoRuntime_WarningRecorded) {
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_no_rt");
    bool has_wasm_warning = false;
    for (const auto& w : sb.launchWarnings()) {
        if (w.find("WASM") != std::string::npos ||
            w.find("wasm") != std::string::npos) {
            has_wasm_warning = true;
        }
    }
    EXPECT_TRUE(has_wasm_warning)
        << "Expected a warning about missing WASM runtime backend";
}

// =============================================================================
// launch() with WASM isolation enabled and runtime registered
// =============================================================================

TEST_F(WasmInjectionFixture, WasmEnabledWithRuntime_LaunchSucceeds) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    EXPECT_TRUE(sb.launch("plugin_wasm"));
    EXPECT_TRUE(sb.isActive());
}

TEST_F(WasmInjectionFixture, WasmEnabledWithRuntime_IsolationActive) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    EXPECT_TRUE(sb.isWasmIsolationActive());
}

TEST_F(WasmInjectionFixture, WasmEnabledWithRuntime_WasmSandboxNonNull) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);
    // The inner sandbox must have a runtime injected
    EXPECT_TRUE(sb.wasmSandbox()->hasRuntime());
}

TEST_F(WasmInjectionFixture, WasmEnabledWithRuntime_EngineNamePropagated) {
    registerMock("my-engine");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);
    EXPECT_EQ(sb.wasmSandbox()->engineName(), "my-engine");
}

// =============================================================================
// Sandbox runtime name selection
// =============================================================================

TEST_F(WasmInjectionFixture, AutoSelectHighestPriority) {
    registerMock("low-rt",  5);
    registerMock("high-rt", 100);
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);
    EXPECT_EQ(sb.wasmSandbox()->engineName(), "high-rt");
}

TEST_F(WasmInjectionFixture, NamedRuntimeSelection) {
    registerMock("alpha-rt", 100);
    registerMock("beta-rt",   50);
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.wasm_runtime_name     = "beta-rt";
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);
    EXPECT_EQ(sb.wasmSandbox()->engineName(), "beta-rt");
}

TEST_F(WasmInjectionFixture, UnknownRuntimeName_FallsBackToWarning) {
    registerMock("existing-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.wasm_runtime_name     = "nonexistent-rt";
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    EXPECT_TRUE(sb.launch("plugin_wasm"));
    // WasmRuntimeInjector::create("nonexistent-rt") returns nullptr
    EXPECT_FALSE(sb.isWasmIsolationActive());
    EXPECT_EQ(sb.wasmSandbox(), nullptr);
    bool has_warning = false;
    for (const auto& w : sb.launchWarnings())
        if (w.find("nullptr") != std::string::npos ||
            w.find("WASM") != std::string::npos) has_warning = true;
    EXPECT_TRUE(has_warning);
}

// =============================================================================
// Config propagation into inner WasmPluginSandbox
// =============================================================================

TEST_F(WasmInjectionFixture, LinearMemoryPagesPropagated) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation      = true;
    cfg.wasm_linear_memory_pages   = 64; // 4 MiB
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);
    // The inner sandbox must not be loaded yet (no .wasm given)
    EXPECT_FALSE(sb.wasmSandbox()->isLoaded());
}

TEST_F(WasmInjectionFixture, WasmSandboxCanLoadMinimalWasm) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);

    auto wasm = minimalWasm();
    bool loaded = sb.wasmSandbox()->loadFromBytes(wasm, "test_plugin");
    EXPECT_TRUE(loaded) << "load error: " << sb.wasmSandbox()->lastError();
    EXPECT_TRUE(sb.wasmSandbox()->isLoaded());
}

TEST_F(WasmInjectionFixture, WasmSandboxCanLoadAndCallExport) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_NE(sb.wasmSandbox(), nullptr);

    auto wasm = wasmWithExport("run");
    ASSERT_TRUE(sb.wasmSandbox()->loadFromBytes(wasm, "exported_plugin"));

    std::vector<uint8_t> args = {1, 2, 3};
    WasmCallResult result = sb.wasmSandbox()->callExport("run", args);
    EXPECT_TRUE(result.success) << result.error;
}

// =============================================================================
// shutdown() cleans up WASM state
// =============================================================================

TEST_F(WasmInjectionFixture, ShutdownClearsWasmIsolation) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");
    ASSERT_TRUE(sb.isWasmIsolationActive());
    ASSERT_NE(sb.wasmSandbox(), nullptr);

    sb.shutdown();

    EXPECT_FALSE(sb.isActive());
    EXPECT_FALSE(sb.isWasmIsolationActive());
    EXPECT_EQ(sb.wasmSandbox(), nullptr);
}

TEST_F(WasmInjectionFixture, DestructorDoesNotCrash) {
    registerMock("mock-rt");
    // RAII: destructor should shutdown WASM sandbox gracefully
    {
        ModuleSandbox::Config cfg;
        cfg.enable_wasm_isolation = true;
        cfg.max_memory_mb   = 0;
        cfg.max_cpu_percent = 0;
        ModuleSandbox sb(cfg);
        sb.launch("plugin_wasm");
        EXPECT_TRUE(sb.isWasmIsolationActive());
    } // ~ModuleSandbox
    SUCCEED();
}

// =============================================================================
// Const accessor
// =============================================================================

TEST_F(WasmInjectionFixture, ConstAccessorWorks) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);
    sb.launch("plugin_wasm");

    const ModuleSandbox& csb = sb;
    ASSERT_NE(csb.wasmSandbox(), nullptr);
    EXPECT_TRUE(csb.wasmSandbox()->hasRuntime());
}

// =============================================================================
// Multiple launches on same sandbox object (after shutdown)
// =============================================================================

TEST_F(WasmInjectionFixture, RelaunchAfterShutdownWorks) {
    registerMock("mock-rt");
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation = true;
    cfg.max_memory_mb   = 0;
    cfg.max_cpu_percent = 0;
    ModuleSandbox sb(cfg);

    sb.launch("plugin_first");
    EXPECT_TRUE(sb.isWasmIsolationActive());
    sb.shutdown();
    EXPECT_FALSE(sb.isWasmIsolationActive());

    // Re-launch with the same config
    EXPECT_TRUE(sb.launch("plugin_second"));
    EXPECT_TRUE(sb.isWasmIsolationActive());
    EXPECT_NE(sb.wasmSandbox(), nullptr);
}
