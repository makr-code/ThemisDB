/// @file test_wasm_plugin_sandbox.cpp
/// @brief Unit tests for WasmPluginSandbox, WasmModuleValidator, and
///        WasmRuntime injection (Phase 3 – WASM-based plugin isolation).

#include <gtest/gtest.h>
#include "themis/base/wasm_plugin_sandbox.h"

#include <cstring>
#include <vector>
#include <string>

using namespace themis::modules;

// =============================================================================
// Helpers – minimal WASM binary builders
// =============================================================================

/// Minimal valid WASM binary: magic + version only (8 bytes).
static std::vector<uint8_t> minimalWasm() {
    return { 0x00, 0x61, 0x73, 0x6d,   // magic
             0x01, 0x00, 0x00, 0x00 }; // version 1
}

/// Corrupt header – wrong magic bytes.
static std::vector<uint8_t> corruptMagicWasm() {
    return { 0xDE, 0xAD, 0xBE, 0xEF,
             0x01, 0x00, 0x00, 0x00 };
}

/// Correct magic but wrong version (version 2).
static std::vector<uint8_t> wrongVersionWasm() {
    return { 0x00, 0x61, 0x73, 0x6d,
             0x02, 0x00, 0x00, 0x00 };
}

/// Too short to be WASM.
static std::vector<uint8_t> tooShortWasm() {
    return { 0x00, 0x61, 0x73 };
}

/// Build a LEB-128 encoded u32.
static std::vector<uint8_t> leb128(uint32_t v) {
    std::vector<uint8_t> out;
    do {
        uint8_t byte = v & 0x7f;
        v >>= 7;
        if (v) byte |= 0x80;
        out.push_back(byte);
    } while (v);
    return out;
}

/// Build a WASM name entry (length-prefixed UTF-8 string).
static std::vector<uint8_t> wasmName(const std::string& s) {
    std::vector<uint8_t> out = leb128(static_cast<uint32_t>(s.size()));
    for (char c : s) out.push_back(static_cast<uint8_t>(c));
    return out;
}

/// Append bytes from src to dst.
static void append(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

/**
 * @brief Build a tiny WASM binary with one import entry (section id=2).
 *
 * Import section layout:
 *   section_id=2, section_size, count=1,
 *   module_name, function_name, kind=0 (func), type_index=0
 */
static std::vector<uint8_t> wasmWithImport(const std::string& mod,
                                            const std::string& fn) {
    // Build section body
    std::vector<uint8_t> body;
    append(body, leb128(1)); // count = 1
    append(body, wasmName(mod));
    append(body, wasmName(fn));
    body.push_back(0x00); // kind = function
    append(body, leb128(0)); // type index = 0

    // Build full binary
    std::vector<uint8_t> out = minimalWasm();
    out.push_back(0x02); // section id = import
    append(out, leb128(static_cast<uint32_t>(body.size())));
    append(out, body);
    return out;
}

/**
 * @brief Build a tiny WASM binary with one export entry (section id=7).
 */
static std::vector<uint8_t> wasmWithExport(const std::string& name) {
    std::vector<uint8_t> body;
    append(body, leb128(1));         // count = 1
    append(body, wasmName(name));    // export name
    body.push_back(0x00);            // kind = function
    append(body, leb128(0));         // function index = 0

    std::vector<uint8_t> out = minimalWasm();
    out.push_back(0x07); // section id = export
    append(out, leb128(static_cast<uint32_t>(body.size())));
    append(out, body);
    return out;
}

/**
 * @brief Build a WASM binary with a non-function (memory) import.
 *
 * Memory import descriptor: kind=0x02, then limits (flags=0x00, min=1 page).
 * This exercises the parser's conservative handling of non-function imports.
 */
static std::vector<uint8_t> wasmWithMemoryImport(const std::string& mod,
                                                   const std::string& name) {
    std::vector<uint8_t> body;
    append(body, leb128(1));           // count = 1
    append(body, wasmName(mod));
    append(body, wasmName(name));
    body.push_back(0x02);              // kind = memory
    body.push_back(0x00);              // limits flags: no maximum
    append(body, leb128(1));           // limits min = 1 page

    std::vector<uint8_t> out = minimalWasm();
    out.push_back(0x02); // section id = import
    append(out, leb128(static_cast<uint32_t>(body.size())));
    append(out, body);
    return out;
}

/**
 * @brief Build a WASM binary with a memory import followed by a function import.
 *
 * Import section layout (count=2):
 *   1. memory import  – kind=0x02, limits (no max, min=1)
 *   2. function import – kind=0x00, type_index=0
 *
 * This exercises that the parser correctly skips the memory descriptor and
 * continues to collect the subsequent function import.
 */
static std::vector<uint8_t> wasmWithMemoryThenFuncImport(
    const std::string& mem_mod, const std::string& mem_name,
    const std::string& fn_mod,  const std::string& fn_name) {
    std::vector<uint8_t> body;
    append(body, leb128(2));            // count = 2

    // Entry 1: memory import
    append(body, wasmName(mem_mod));
    append(body, wasmName(mem_name));
    body.push_back(0x02);               // kind = memory
    body.push_back(0x00);               // limits flags: no maximum
    append(body, leb128(1));            // limits min = 1 page

    // Entry 2: function import
    append(body, wasmName(fn_mod));
    append(body, wasmName(fn_name));
    body.push_back(0x00);               // kind = function
    append(body, leb128(0));            // type index = 0

    std::vector<uint8_t> out = minimalWasm();
    out.push_back(0x02); // section id = import
    append(out, leb128(static_cast<uint32_t>(body.size())));
    append(out, body);
    return out;
}

/**
 * @brief Build a WASM binary with: func import, memory import, func import.
 *
 * Tests that function imports on both sides of a non-function import are
 * correctly collected.
 */
static std::vector<uint8_t> wasmWithFuncMemoryFuncImports(
    const std::string& fn1_mod, const std::string& fn1_name,
    const std::string& mem_mod, const std::string& mem_name,
    const std::string& fn2_mod, const std::string& fn2_name) {
    std::vector<uint8_t> body;
    append(body, leb128(3));            // count = 3

    // Entry 1: function import
    append(body, wasmName(fn1_mod));
    append(body, wasmName(fn1_name));
    body.push_back(0x00);               // kind = function
    append(body, leb128(0));            // type index = 0

    // Entry 2: memory import
    append(body, wasmName(mem_mod));
    append(body, wasmName(mem_name));
    body.push_back(0x02);               // kind = memory
    body.push_back(0x00);               // limits flags: no maximum
    append(body, leb128(1));            // limits min = 1 page

    // Entry 3: function import
    append(body, wasmName(fn2_mod));
    append(body, wasmName(fn2_name));
    body.push_back(0x00);               // kind = function
    append(body, leb128(0));            // type index = 0

    std::vector<uint8_t> out = minimalWasm();
    out.push_back(0x02); // section id = import
    append(out, leb128(static_cast<uint32_t>(body.size())));
    append(out, body);
    return out;
}

// =============================================================================
// WasmModuleValidator tests
// =============================================================================

TEST(WasmModuleValidator, MagicBytesAreCorrect) {
    const uint8_t* magic = WasmModuleValidator::magicBytes();
    ASSERT_NE(magic, nullptr);
    EXPECT_EQ(magic[0], 0x00);
    EXPECT_EQ(magic[1], 0x61);
    EXPECT_EQ(magic[2], 0x73);
    EXPECT_EQ(magic[3], 0x6d);
}

TEST(WasmModuleValidator, ValidMinimalWasm) {
    auto info = WasmModuleValidator::validate(minimalWasm());
    EXPECT_TRUE(info.valid);
    EXPECT_EQ(info.wasm_version, 1u);
    EXPECT_EQ(info.byte_size, 8u);
}

TEST(WasmModuleValidator, InvalidMagicBytes) {
    auto info = WasmModuleValidator::validate(corruptMagicWasm());
    EXPECT_FALSE(info.valid);
}

TEST(WasmModuleValidator, TooShortBinary) {
    auto info = WasmModuleValidator::validate(tooShortWasm());
    EXPECT_FALSE(info.valid);
}

TEST(WasmModuleValidator, EmptyBinary) {
    auto info = WasmModuleValidator::validate({});
    EXPECT_FALSE(info.valid);
}

TEST(WasmModuleValidator, WasmWithImportSection) {
    auto bytes = wasmWithImport("themis", "log");
    auto info  = WasmModuleValidator::validate(bytes);
    EXPECT_TRUE(info.valid);
    ASSERT_EQ(info.imports.size(), 1u);
    EXPECT_EQ(info.imports[0], "themis.log");
}

TEST(WasmModuleValidator, WasmWithMemoryImportDoesNotCrash) {
    // A memory import has a non-function descriptor.
    // The parser should handle it without crashing or reading out-of-bounds.
    auto bytes = wasmWithMemoryImport("env", "memory");
    auto info  = WasmModuleValidator::validate(bytes);
    EXPECT_TRUE(info.valid);
    // Memory imports are non-function and must NOT appear in info.imports.
    EXPECT_TRUE(info.imports.empty());
}

TEST(WasmModuleValidator, MemoryImportBeforeFuncImportCollectsFuncImport) {
    // When a memory import precedes a function import, the parser must skip the
    // memory descriptor and continue so that the function import is collected.
    auto bytes = wasmWithMemoryThenFuncImport("env", "memory", "themis", "log");
    auto info  = WasmModuleValidator::validate(bytes);
    EXPECT_TRUE(info.valid);
    ASSERT_EQ(info.imports.size(), 1u);
    EXPECT_EQ(info.imports[0], "themis.log");
}

TEST(WasmModuleValidator, InterleavedMemoryAndFuncImportsCollectsAllFuncImports) {
    // Func import, then memory import, then another func import.
    // Both function imports must appear in info.imports regardless of the
    // interleaved memory import.
    auto bytes = wasmWithFuncMemoryFuncImports(
        "themis", "log",
        "env",    "memory",
        "themis", "abort");
    auto info  = WasmModuleValidator::validate(bytes);
    EXPECT_TRUE(info.valid);
    ASSERT_EQ(info.imports.size(), 2u);
    EXPECT_EQ(info.imports[0], "themis.log");
    EXPECT_EQ(info.imports[1], "themis.abort");
}

TEST(WasmModuleValidator, WasmWithExportSection) {
    auto bytes = wasmWithExport("process");
    auto info  = WasmModuleValidator::validate(bytes);
    EXPECT_TRUE(info.valid);
    ASSERT_EQ(info.exports.size(), 1u);
    EXPECT_EQ(info.exports[0], "process");
}

TEST(WasmModuleValidator, SummaryNonEmpty) {
    auto info = WasmModuleValidator::validate(minimalWasm());
    EXPECT_FALSE(info.summary().empty());
}

TEST(WasmModuleValidator, MissingFilePath) {
    auto info = WasmModuleValidator::validateFile("/nonexistent/path/plugin.wasm");
    EXPECT_FALSE(info.valid);
}

// =============================================================================
// WasmPluginSandbox – construction and configuration
// =============================================================================

TEST(WasmPluginSandbox, DefaultConstruction) {
    EXPECT_NO_THROW({ WasmPluginSandbox sb; });
}

TEST(WasmPluginSandbox, InitiallyNotLoaded) {
    WasmPluginSandbox sb;
    EXPECT_FALSE(sb.isLoaded());
}

TEST(WasmPluginSandbox, DefaultConfigValues) {
    WasmPluginSandbox::Config cfg;
    EXPECT_EQ(cfg.linear_memory_pages, 256u);
    EXPECT_EQ(cfg.max_memory_mb,       64u);
    EXPECT_EQ(cfg.max_cpu_time_seconds, 0u);
    EXPECT_FALSE(cfg.allow_unregistered_imports);
}

TEST(WasmPluginSandbox, NoRuntimeByDefault) {
    WasmPluginSandbox sb;
    EXPECT_FALSE(sb.hasRuntime());
    EXPECT_TRUE(sb.engineName().empty());
}

// =============================================================================
// WasmPluginSandbox – host function registration
// =============================================================================

TEST(WasmPluginSandbox, AddHostFunction) {
    WasmPluginSandbox sb;
    sb.addHostFunction({ "themis", "log", [](auto*, auto, auto&, auto&) { return true; }, "Log" });
    EXPECT_EQ(sb.hostFunctionCount(), 1u);
}

TEST(WasmPluginSandbox, ClearHostFunctions) {
    WasmPluginSandbox sb;
    sb.addHostFunction({ "themis", "log", [](auto*, auto, auto&, auto&) { return true; }, "Log" });
    sb.clearHostFunctions();
    EXPECT_EQ(sb.hostFunctionCount(), 0u);
}

TEST(WasmPluginSandbox, MultipleHostFunctions) {
    WasmPluginSandbox sb;
    sb.addHostFunction({ "themis", "log",   [](auto*, auto, auto&, auto&) { return true; }, "" });
    sb.addHostFunction({ "themis", "abort", [](auto*, auto, auto&, auto&) { return false; }, "" });
    EXPECT_EQ(sb.hostFunctionCount(), 2u);
}

// =============================================================================
// WasmPluginSandbox – loading invalid binaries
// =============================================================================

TEST(WasmPluginSandbox, LoadBytesInvalidMagic) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromBytes(corruptMagicWasm());
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.lastError().empty());
    EXPECT_FALSE(sb.isLoaded());
}

TEST(WasmPluginSandbox, LoadBytesTooShort) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromBytes(tooShortWasm());
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.isLoaded());
}

TEST(WasmPluginSandbox, LoadBytesEmptyBuffer) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromBytes({});
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.isLoaded());
}

TEST(WasmPluginSandbox, LoadBytesWrongVersion) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromBytes(wrongVersionWasm());
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.isLoaded());
}

// =============================================================================
// WasmPluginSandbox – loading valid minimal WASM (no imports = no allowlist check)
// =============================================================================

TEST(WasmPluginSandbox, LoadMinimalWasmSucceeds) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromBytes(minimalWasm(), "test_module");
    EXPECT_TRUE(ok) << "Error: " << sb.lastError();
    EXPECT_TRUE(sb.isLoaded());
}

TEST(WasmPluginSandbox, LoadMinimalWasmHasLinearMemory) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_NE(sb.linearMemory(), nullptr);
    EXPECT_GT(sb.linearMemorySize(), 0u);
}

TEST(WasmPluginSandbox, LinearMemoryIsZeroInitialized) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    const uint8_t* mem = sb.linearMemory();
    size_t sz = sb.linearMemorySize();
    // Spot-check first and last 16 bytes
    for (size_t i = 0; i < std::min(sz, size_t(16)); ++i)
        EXPECT_EQ(mem[i], 0u);
    if (sz >= 16) {
        for (size_t i = sz - 16; i < sz; ++i)
            EXPECT_EQ(mem[i], 0u);
    }
}

TEST(WasmPluginSandbox, ModuleInfoPopulated) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm(), "my_module"));
    EXPECT_TRUE(sb.moduleInfo().valid);
    EXPECT_EQ(sb.moduleInfo().wasm_version, 1u);
}

TEST(WasmPluginSandbox, LoadWarningsAreStrings) {
    WasmPluginSandbox sb;
    sb.loadFromBytes(minimalWasm());
    for (const auto& w : sb.loadWarnings())
        EXPECT_FALSE(w.empty());
}

// =============================================================================
// WasmPluginSandbox – allowlist enforcement
// =============================================================================

TEST(WasmPluginSandbox, UnregisteredImportIsRejected) {
    // Build a WASM with one import that is NOT in the allowlist.
    auto bytes = wasmWithImport("themis", "secret_fn");
    WasmPluginSandbox sb;
    // No host functions registered → import rejected by default
    bool ok = sb.loadFromBytes(bytes, "restricted_plugin");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.lastError().empty());
    EXPECT_FALSE(sb.isLoaded());
}

TEST(WasmPluginSandbox, RegisteredImportIsAccepted) {
    auto bytes = wasmWithImport("themis", "log");
    WasmPluginSandbox sb;
    sb.addHostFunction({ "themis", "log", [](auto*, auto, auto&, auto&) { return true; }, "Log" });
    bool ok = sb.loadFromBytes(bytes, "allowed_plugin");
    EXPECT_TRUE(ok) << sb.lastError();
    EXPECT_TRUE(sb.isLoaded());
}

TEST(WasmPluginSandbox, AllowUnregisteredImportsFlag) {
    auto bytes = wasmWithImport("ext", "anything");
    WasmPluginSandbox::Config cfg;
    cfg.allow_unregistered_imports = true;
    WasmPluginSandbox sb(cfg);
    bool ok = sb.loadFromBytes(bytes, "permissive_plugin");
    EXPECT_TRUE(ok) << sb.lastError();
}

TEST(WasmPluginSandbox, NonFunctionImportDoesNotCrash) {
    // A binary with a memory (non-function) import must not crash the parser.
    auto bytes = wasmWithMemoryImport("env", "memory");
    WasmPluginSandbox::Config cfg;
    cfg.allow_unregistered_imports = true;
    WasmPluginSandbox sb(cfg);
    bool ok = sb.loadFromBytes(bytes, "memory_plugin");
    EXPECT_TRUE(ok) << sb.lastError();
    // Memory imports are non-function and must NOT be recorded in imports.
    EXPECT_TRUE(sb.moduleInfo().imports.empty());
}

// =============================================================================
// WasmPluginSandbox – callExport without a runtime
// =============================================================================

TEST(WasmPluginSandbox, CallExportWithoutRuntimeFails) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    auto result = sb.callExport("process", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(WasmPluginSandbox, CallExportBeforeLoadFails) {
    WasmPluginSandbox sb;
    auto result = sb.callExport("process", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

TEST(WasmPluginSandbox, StatsAttemptedCountedOnCallWithoutRuntime) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    sb.callExport("fn", {});
    EXPECT_EQ(sb.stats().calls_attempted, 1u);
    EXPECT_EQ(sb.stats().calls_trapped,   1u);
    EXPECT_EQ(sb.stats().calls_succeeded, 0u);
}

// =============================================================================
// WasmPluginSandbox – unload / reload
// =============================================================================

TEST(WasmPluginSandbox, UnloadResetsState) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    ASSERT_TRUE(sb.isLoaded());
    sb.unload();
    EXPECT_FALSE(sb.isLoaded());
    EXPECT_EQ(sb.linearMemory(), nullptr);
    EXPECT_EQ(sb.linearMemorySize(), 0u);
}

TEST(WasmPluginSandbox, ReloadAfterUnload) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm(), "first"));
    sb.unload();
    EXPECT_FALSE(sb.isLoaded());
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm(), "second"));
    EXPECT_TRUE(sb.isLoaded());
}

TEST(WasmPluginSandbox, DestructorWithLoadedModule) {
    {
        WasmPluginSandbox sb;
        ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
        ASSERT_TRUE(sb.isLoaded());
    } // ~WasmPluginSandbox should call unload() without crash
    SUCCEED();
}

// =============================================================================
// WasmPluginSandbox – runtime injection via a mock
// =============================================================================

/**
 * @brief Minimal mock WasmRuntime for unit tests.
 */
class MockWasmRuntime : public WasmRuntime {
public:
    bool instantiate_called = false;
    bool call_called        = false;
    bool call_result        = true; // Configurable: true = success, false = trap

    bool instantiate(const std::vector<uint8_t>&,
                     const std::vector<WasmHostFunction>&,
                     uint8_t*, size_t) override {
        instantiate_called = true;
        return true;
    }

    bool call(const std::string&, const std::vector<uint8_t>&,
              std::vector<uint8_t>& out) override {
        call_called = true;
        if (call_result) out = { 0x42 }; // dummy return value
        return call_result;
    }

    void destroy() override {}

    std::string engineName() const override { return "mock-1.0"; }
};

TEST(WasmPluginSandbox, RuntimeInjectionReported) {
    WasmPluginSandbox sb;
    auto rt = std::make_unique<MockWasmRuntime>();
    sb.setRuntime(std::move(rt));
    EXPECT_TRUE(sb.hasRuntime());
    EXPECT_EQ(sb.engineName(), "mock-1.0");
}

TEST(WasmPluginSandbox, RuntimeInstantiatedOnLoad) {
    WasmPluginSandbox sb;
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm(), "mocked_plugin"));
    EXPECT_TRUE(raw->instantiate_called);
}

TEST(WasmPluginSandbox, CallExportSucceedsWithRuntime) {
    WasmPluginSandbox sb;
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    auto result = sb.callExport("process", {});
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(raw->call_called);
    ASSERT_EQ(result.output.size(), 1u);
    EXPECT_EQ(result.output[0], 0x42u);
}

TEST(WasmPluginSandbox, CallExportTrapCountedWithRuntime) {
    WasmPluginSandbox sb;
    auto* raw = new MockWasmRuntime();
    raw->call_result = false; // simulate WASM trap
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    auto result = sb.callExport("fn", {});
    EXPECT_FALSE(result.success);
    EXPECT_EQ(sb.stats().calls_trapped, 1u);
}

TEST(WasmPluginSandbox, StatsAccumulateAcrossCalls) {
    WasmPluginSandbox sb;
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    sb.callExport("fn1", {});
    sb.callExport("fn2", {});
    sb.callExport("fn3", {});
    EXPECT_EQ(sb.stats().calls_attempted, 3u);
    EXPECT_EQ(sb.stats().calls_succeeded, 3u);
}

TEST(WasmPluginSandbox, ExportsParsedWithRuntime) {
    auto bytes = wasmWithExport("run");
    WasmPluginSandbox sb;
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(bytes));
    ASSERT_EQ(sb.moduleInfo().exports.size(), 1u);
    EXPECT_EQ(sb.moduleInfo().exports[0], "run");
}

// =============================================================================
// WasmPluginSandbox – linear memory custom configuration
// =============================================================================

TEST(WasmPluginSandbox, ZeroLinearMemoryPages) {
    WasmPluginSandbox::Config cfg;
    cfg.linear_memory_pages = 0;
    WasmPluginSandbox sb(cfg);
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.linearMemorySize(), 0u);
}

TEST(WasmPluginSandbox, OnePageLinearMemory) {
    WasmPluginSandbox::Config cfg;
    cfg.linear_memory_pages = 1;
    WasmPluginSandbox sb(cfg);
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.linearMemorySize(), 65536u); // 1 × 64 KiB
}

TEST(WasmPluginSandbox, FourPagesLinearMemory) {
    WasmPluginSandbox::Config cfg;
    cfg.linear_memory_pages = 4;
    WasmPluginSandbox sb(cfg);
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.linearMemorySize(), 4u * 65536u);
}

// =============================================================================
// WasmPluginSandbox – load from non-existent file
// =============================================================================

TEST(WasmPluginSandbox, LoadFromNonExistentFile) {
    WasmPluginSandbox sb;
    bool ok = sb.loadFromFile("/nonexistent/plugin.wasm");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sb.lastError().empty());
    EXPECT_FALSE(sb.isLoaded());
}

// =============================================================================
// WasmPluginSandbox – fuel / instruction metering
// =============================================================================

// remainingFuel() returns UINT64_MAX when max_instructions == 0 (unlimited).
TEST(WasmPluginSandboxFuel, UnlimitedFuelByDefault) {
    WasmPluginSandbox sb;
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.remainingFuel(), UINT64_MAX);
}

// remainingFuel() is initialised from max_instructions at load time.
TEST(WasmPluginSandboxFuel, FuelInitialisedFromConfig) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions = 100;
    WasmPluginSandbox sb(cfg);
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.remainingFuel(), 100u);
}

// Each callExport() with a runtime deducts fuel_check_interval units.
TEST(WasmPluginSandboxFuel, FuelDeductedPerCall) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 10;
    cfg.fuel_check_interval = 3; // 3 units deducted per call
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    ASSERT_EQ(sb.remainingFuel(), 10u);

    auto r1 = sb.callExport("fn", {});
    EXPECT_TRUE(r1.success);
    EXPECT_EQ(sb.remainingFuel(), 7u); // 10 - 3

    auto r2 = sb.callExport("fn", {});
    EXPECT_TRUE(r2.success);
    EXPECT_EQ(sb.remainingFuel(), 4u); // 7 - 3

    auto r3 = sb.callExport("fn", {});
    EXPECT_TRUE(r3.success);
    EXPECT_EQ(sb.remainingFuel(), 1u); // 4 - 3
}

// When fuel reaches zero, callExport() returns a structured error.
TEST(WasmPluginSandboxFuel, ExhaustedFuelReturnsError) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 2;
    cfg.fuel_check_interval = 1;
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    EXPECT_TRUE(sb.callExport("fn", {}).success); // fuel: 2 -> 1
    EXPECT_TRUE(sb.callExport("fn", {}).success); // fuel: 1 -> 0

    // Budget now exhausted – next call must fail with a descriptive error.
    auto result = sb.callExport("fn", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
    EXPECT_NE(result.error.find("fuel exhausted"), std::string::npos);
    EXPECT_EQ(sb.remainingFuel(), 0u);
}

// Fuel-exhausted call is counted in stats as a trap.
TEST(WasmPluginSandboxFuel, ExhaustedFuelCountedAsTrap) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 1;
    cfg.fuel_check_interval = 1;
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    sb.callExport("fn", {}); // consumes last fuel unit

    sb.callExport("fn", {}); // fuel-exhausted: should trap
    EXPECT_GE(sb.stats().calls_trapped, 1u);
}

// Simulated "infinite loop": a mock that would run forever is safely
// stopped after max_instructions / fuel_check_interval call attempts.
TEST(WasmPluginSandboxFuel, InfiniteLoopBoundedByFuelBudget) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 5;
    cfg.fuel_check_interval = 1;
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    // Drive the "infinite loop": keep calling until we get a fuel error.
    int successful = 0;
    WasmCallResult last;
    for (int i = 0; i < 100; ++i) {
        last = sb.callExport("loop", {});
        if (!last.success) break;
        ++successful;
    }

    // Must have been stopped exactly at the budget boundary.
    EXPECT_EQ(successful, static_cast<int>(cfg.max_instructions));
    EXPECT_FALSE(last.success);
    EXPECT_NE(last.error.find("fuel exhausted"), std::string::npos);
    EXPECT_EQ(sb.remainingFuel(), 0u);
}

// Reload resets the fuel counter to the configured budget.
TEST(WasmPluginSandboxFuel, ReloadResetsFuel) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 3;
    cfg.fuel_check_interval = 1;
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    // Exhaust the budget.
    sb.callExport("fn", {});
    sb.callExport("fn", {});
    sb.callExport("fn", {});
    EXPECT_EQ(sb.remainingFuel(), 0u);
    EXPECT_FALSE(sb.callExport("fn", {}).success); // exhausted

    // Reload with the same config must restore fuel to 3.
    sb.setRuntime(std::make_unique<MockWasmRuntime>());
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));
    EXPECT_EQ(sb.remainingFuel(), 3u);
    EXPECT_TRUE(sb.callExport("fn", {}).success);
}

// fuel_check_interval larger than remaining fuel clamps to zero gracefully.
TEST(WasmPluginSandboxFuel, LargeIntervalClampsToZero) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions   = 5;
    cfg.fuel_check_interval = 10; // single call costs more than the total budget
    WasmPluginSandbox sb(cfg);
    auto* raw = new MockWasmRuntime();
    sb.setRuntime(std::unique_ptr<WasmRuntime>(raw));
    ASSERT_TRUE(sb.loadFromBytes(minimalWasm()));

    // First call: deducts min(10, 5) = clamps remaining to 0.
    auto r1 = sb.callExport("fn", {});
    EXPECT_TRUE(r1.success);           // call goes through (fuel was > 0 before)
    EXPECT_EQ(sb.remainingFuel(), 0u); // now exhausted

    // Second call must be rejected.
    auto r2 = sb.callExport("fn", {});
    EXPECT_FALSE(r2.success);
}
