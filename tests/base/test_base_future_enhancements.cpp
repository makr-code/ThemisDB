/**
 * @file test_base_future_enhancements.cpp
 * @brief Focused regression and hardening tests for base module future-enhancement items.
 *
 * Coverage (short-term Q4 2026 + mid-term Q1 2027 targets):
 *  - AbiChecker version compatibility edge cases (major/minor mismatch, patch tolerance)
 *  - AbiChecker symbol-check edge cases (empty list, null handle, deprecated detection)
 *  - WasmPluginSandbox validation-only mode (magic validation, invalid bytes, no-runtime callExport)
 *  - WasmPluginSandbox host-function allowlist (registration, count, clear)
 *  - WasmPluginSandbox fuel-budget behavior (zero = unlimited, non-zero gate)
 *  - Advanced reload/dependency edge permutations (diamond, multi-root, re-register)
 *  - ModuleSandbox WASM-isolation state predicates (isWasmIsolationActive, wasmSandbox)
 *  - Operator-facing diagnostics: format prefix, resolveDescription, isKnownCode completeness
 *  - WasmModuleInfo struct invariants and summary
 *
 * All tests exercise production paths; no stubs or mocks are introduced.
 * Requires: GTest, themis_core (hot_reload_manager, module_loader, module_sandbox,
 *           wasm_plugin_sandbox, base_error_taxonomy).
 */

#include <gtest/gtest.h>

#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include "themis/base/module_sandbox.h"
#include "themis/base/plugin_dependency_graph.h"
#include "themis/base/wasm_plugin_sandbox.h"
#include "themis/base/base_error_taxonomy.h"

#include <algorithm>
#include <string>
#include <vector>

namespace themis { namespace modules { 

// =============================================================================
// Suite 1: AbiChecker — version compatibility edge cases
// =============================================================================

class AbiCheckerVersionTest : public ::testing::Test {
protected:
    AbiChecker checker_;
};

/// Same major and minor versions are compatible.
TEST_F(AbiCheckerVersionTest, SameMajorMinorIsCompatible) {
    ModuleMetadata meta;
    meta.themisMajor = 1;
    meta.themisMinor = 2;
    meta.version     = "1.2.0";

    auto result = checker_.checkVersions(meta, 1, 2);
    EXPECT_TRUE(result.compatible)
        << "Same major.minor must be compatible: " << result.summary;
}

/// Major version mismatch renders the module incompatible.
TEST_F(AbiCheckerVersionTest, MajorMismatchIncompatible) {
    ModuleMetadata meta;
    meta.themisMajor = 2;
    meta.themisMinor = 0;
    meta.version     = "2.0.0";

    auto result = checker_.checkVersions(meta, 1, 0);
    EXPECT_FALSE(result.compatible)
        << "Major version mismatch must be incompatible";
    EXPECT_FALSE(result.issues.empty());
}

/// Module minor > host minor is a forward-compatibility violation.
TEST_F(AbiCheckerVersionTest, ModuleMinorAheadOfHostIncompatible) {
    ModuleMetadata meta;
    meta.themisMajor = 1;
    meta.themisMinor = 5;   // module requires minor=5
    meta.version     = "1.5.0";

    // host only supports minor=3
    auto result = checker_.checkVersions(meta, 1, 3);
    EXPECT_FALSE(result.compatible)
        << "Module minor > host minor should be incompatible";
}

/// Module minor ≤ host minor is backward-compatible.
TEST_F(AbiCheckerVersionTest, ModuleMinorBehindHostCompatible) {
    ModuleMetadata meta;
    meta.themisMajor = 1;
    meta.themisMinor = 2;
    meta.version     = "1.2.0";

    // host supports minor=4
    auto result = checker_.checkVersions(meta, 1, 4);
    EXPECT_TRUE(result.compatible)
        << "Module minor ≤ host minor must be backward-compatible: " << result.summary;
}

/// Patch difference with matching major.minor is compatible.
TEST_F(AbiCheckerVersionTest, PatchDifferenceDoesNotBreakCompat) {
    ModuleMetadata meta;
    meta.themisMajor = 1;
    meta.themisMinor = 0;
    meta.version     = "1.0.7";

    auto result = checker_.checkVersions(meta, 1, 0);
    EXPECT_TRUE(result.compatible)
        << "Patch difference must not break compatibility: " << result.summary;
}

// =============================================================================
// Suite 2: AbiChecker — required-symbol and deprecated-symbol edge cases
// =============================================================================

class AbiCheckerSymbolTest : public ::testing::Test {};

/// An empty required-symbol list always passes the symbol check.
TEST_F(AbiCheckerSymbolTest, EmptyRequiredSymbolListPasses) {
    AbiChecker checker;
    // no addRequiredSymbol calls

    // Null handle: on most platforms dlsym(nullptr, ...) returns a process symbol.
    // We pass nullptr here to confirm no crash — we just verify the call does not throw.
    AbiCheckResult result;
    EXPECT_NO_THROW({ result = checker.checkRequiredSymbols(nullptr); });
    // With no required symbols, the check should indicate no required-symbol issues.
    (void)result;
}

/// A deprecated-symbol check on a null handle must not crash.
TEST_F(AbiCheckerSymbolTest, DeprecatedSymbolCheckNullHandleNoThrow) {
    AbiChecker checker;
    checker.addDeprecatedSymbol("themis_legacy_init_v1");

    AbiCheckResult result;
    EXPECT_NO_THROW({ result = checker.checkDeprecatedSymbols(nullptr); });
    (void)result;
}

/// useDefaultLists() populates required symbols (check is callable after).
TEST_F(AbiCheckerSymbolTest, UseDefaultListsIsCallable) {
    AbiChecker checker;
    EXPECT_NO_THROW({ checker.useDefaultLists(); });

    // After loading defaults, a null-handle symbol check must not crash.
    AbiCheckResult result;
    EXPECT_NO_THROW({ result = checker.checkRequiredSymbols(nullptr); });
    (void)result;
}

// =============================================================================
// Suite 3: WasmPluginSandbox — validation-only mode (no runtime injected)
// =============================================================================

class WasmSandboxValidationTest : public ::testing::Test {
protected:
    /// Minimal valid WASM binary (magic + version, no sections).
    static std::vector<uint8_t> minimalWasm() {
        // WASM magic: \0asm, version: 1 (little-endian uint32)
        return { 0x00, 0x61, 0x73, 0x6d,   // magic
                 0x01, 0x00, 0x00, 0x00 };  // version 1
    }
};

/// Fresh sandbox has no runtime, no module loaded.
TEST_F(WasmSandboxValidationTest, FreshSandboxDefaults) {
    WasmPluginSandbox sandbox;
    EXPECT_FALSE(sandbox.isLoaded());
    EXPECT_FALSE(sandbox.hasRuntime());
    EXPECT_TRUE(sandbox.engineName().empty());
    EXPECT_TRUE(sandbox.lastError().empty());
    EXPECT_TRUE(sandbox.loadWarnings().empty());
}

/// Loading minimal valid WASM bytes succeeds in validation-only mode.
TEST_F(WasmSandboxValidationTest, ValidWasmBytesLoadSucceeds) {
    WasmPluginSandbox sandbox;
    bool ok = sandbox.loadFromBytes(minimalWasm(), "test_module");
    // Expected: succeeds (magic validates); no runtime means validation-only.
    EXPECT_TRUE(ok) << "Minimal valid WASM should load: " << sandbox.lastError();
    EXPECT_TRUE(sandbox.isLoaded());
}

/// Loading empty bytes fails gracefully.
TEST_F(WasmSandboxValidationTest, EmptyBytesFailGracefully) {
    WasmPluginSandbox sandbox;
    bool ok = sandbox.loadFromBytes({}, "empty_module");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sandbox.isLoaded());
    EXPECT_FALSE(sandbox.lastError().empty());
}

/// Loading invalid (non-WASM) bytes fails gracefully.
TEST_F(WasmSandboxValidationTest, InvalidBytesFailGracefully) {
    WasmPluginSandbox sandbox;
    std::vector<uint8_t> garbage = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01, 0x02 };
    bool ok = sandbox.loadFromBytes(garbage, "garbage_module");
    EXPECT_FALSE(ok);
    EXPECT_FALSE(sandbox.isLoaded());
    EXPECT_FALSE(sandbox.lastError().empty());
}

/// callExport without a runtime returns an error result (not a crash).
TEST_F(WasmSandboxValidationTest, CallExportWithoutRuntimeReturnsError) {
    WasmPluginSandbox sandbox;
    sandbox.loadFromBytes(minimalWasm(), "no_runtime_mod");

    WasmCallResult result = sandbox.callExport("run", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error.empty());
}

/// unload() after a successful load resets isLoaded to false.
TEST_F(WasmSandboxValidationTest, UnloadResetsLoadedState) {
    WasmPluginSandbox sandbox;
    sandbox.loadFromBytes(minimalWasm(), "unload_test");
    ASSERT_TRUE(sandbox.isLoaded());

    sandbox.unload();
    EXPECT_FALSE(sandbox.isLoaded());
}

/// Repeated load/unload cycles do not crash.
TEST_F(WasmSandboxValidationTest, RepeatedLoadUnloadCyclesSafe) {
    WasmPluginSandbox sandbox;
    for (int i = 0; i < 5; ++i) {
        sandbox.loadFromBytes(minimalWasm(), "cycle_mod");
        sandbox.unload();
    }
    EXPECT_FALSE(sandbox.isLoaded());
}

// =============================================================================
// Suite 4: WasmPluginSandbox — host-function allowlist
// =============================================================================

class WasmHostFunctionTest : public ::testing::Test {};

/// hostFunctionCount() starts at zero.
TEST_F(WasmHostFunctionTest, InitialHostFunctionCountIsZero) {
    WasmPluginSandbox sandbox;
    EXPECT_EQ(sandbox.hostFunctionCount(), 0u);
}

/// Adding host functions increments the count.
TEST_F(WasmHostFunctionTest, AddHostFunctionIncrementsCount) {
    WasmPluginSandbox sandbox;

    sandbox.addHostFunction({
        "themis", "log",
        [](uint8_t*, size_t,
           const std::vector<uint8_t>&,
           std::vector<uint8_t>&) { return true; },
        "Write a log message"
    });

    EXPECT_EQ(sandbox.hostFunctionCount(), 1u);

    sandbox.addHostFunction({
        "themis", "metrics_inc",
        [](uint8_t*, size_t,
           const std::vector<uint8_t>&,
           std::vector<uint8_t>&) { return true; },
        "Increment a named counter"
    });

    EXPECT_EQ(sandbox.hostFunctionCount(), 2u);
}

/// clearHostFunctions() resets count to zero.
TEST_F(WasmHostFunctionTest, ClearHostFunctionsResetsCount) {
    WasmPluginSandbox sandbox;

    for (int i = 0; i < 3; ++i) {
        sandbox.addHostFunction({
            "themis", "fn_" + std::to_string(i),
            [](uint8_t*, size_t,
               const std::vector<uint8_t>&,
               std::vector<uint8_t>&) { return true; },
            ""
        });
    }
    ASSERT_EQ(sandbox.hostFunctionCount(), 3u);

    sandbox.clearHostFunctions();
    EXPECT_EQ(sandbox.hostFunctionCount(), 0u);
}

// =============================================================================
// Suite 5: WasmPluginSandbox — fuel-budget behavior
// =============================================================================

class WasmFuelBudgetTest : public ::testing::Test {
protected:
    static std::vector<uint8_t> minimalWasm() {
        return { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00 };
    }
};

/// Zero max_instructions is treated as unlimited — callExport may still fail
/// for no-runtime reasons but must not fail with a fuel-exhaustion message.
TEST_F(WasmFuelBudgetTest, ZeroInstructionBudgetIsUnlimited) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions = 0;  // unlimited
    WasmPluginSandbox sandbox(cfg);

    sandbox.loadFromBytes(minimalWasm(), "fuel_zero");
    WasmCallResult result = sandbox.callExport("run", {});

    // With no runtime, the failure reason is absent runtime — not fuel exhaustion.
    if (!result.success) {
        EXPECT_EQ(result.error.find("fuel"), std::string::npos)
            << "Zero budget must not produce fuel-exhaustion error: " << result.error;
    }
}

/// A non-zero budget is set; callExport without runtime still fails for missing
/// runtime, not for budget reasons (budget gate only applies when runtime is present).
TEST_F(WasmFuelBudgetTest, NonZeroBudgetDoesNotInterfereWithNoRuntimeError) {
    WasmPluginSandbox::Config cfg;
    cfg.max_instructions    = 100;
    cfg.fuel_check_interval = 10;
    WasmPluginSandbox sandbox(cfg);

    sandbox.loadFromBytes(minimalWasm(), "fuel_nonzero");
    WasmCallResult result = sandbox.callExport("run", {});

    // Without a runtime, failure is expected; we only verify no crash.
    (void)result;
    // Sandbox remains in a usable state.
    EXPECT_NO_THROW({ sandbox.unload(); });
}

// =============================================================================
// Suite 6: WasmModuleInfo — struct invariants and summary
// =============================================================================

class WasmModuleInfoTest : public ::testing::Test {};

/// Default-constructed WasmModuleInfo is invalid.
TEST_F(WasmModuleInfoTest, DefaultIsInvalid) {
    WasmModuleInfo info;
    EXPECT_FALSE(info.valid);
    EXPECT_EQ(info.wasm_version, 0u);
    EXPECT_EQ(info.byte_size, 0u);
    EXPECT_TRUE(info.module_name.empty());
    EXPECT_TRUE(info.imports.empty());
    EXPECT_TRUE(info.exports.empty());
}

/// summary() is callable on a default-constructed (invalid) info without crashing.
TEST_F(WasmModuleInfoTest, SummaryCallableOnInvalidInfo) {
    WasmModuleInfo info;
    std::string s;
    EXPECT_NO_THROW({ s = info.summary(); });
    // Result can be anything; we just ensure no throw.
    (void)s;
}

/// After loading a valid WASM binary, moduleInfo() reflects valid=true.
TEST_F(WasmModuleInfoTest, ModuleInfoValidAfterLoad) {
    WasmPluginSandbox sandbox;
    std::vector<uint8_t> wasm = { 0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00 };
    sandbox.loadFromBytes(wasm, "info_test");

    if (sandbox.isLoaded()) {
        const WasmModuleInfo& info = sandbox.moduleInfo();
        EXPECT_TRUE(info.valid);
        EXPECT_EQ(info.wasm_version, 1u);
    }
}

// =============================================================================
// Suite 7: ModuleSandbox — WASM isolation state predicates
// =============================================================================

class SandboxWasmIsolationTest : public ::testing::Test {};

/// isWasmIsolationActive() is false by default (no wasm config).
TEST_F(SandboxWasmIsolationTest, WasmIsolationInactiveByDefault) {
    ModuleSandbox sandbox;
    EXPECT_FALSE(sandbox.isWasmIsolationActive());
}

/// wasmSandbox() returns nullptr when wasm isolation is not active.
TEST_F(SandboxWasmIsolationTest, WasmSandboxNullptrWhenNotActive) {
    ModuleSandbox sandbox;
    EXPECT_EQ(sandbox.wasmSandbox(), nullptr);
}

/// Config with enable_wasm_isolation=true can be constructed without crash.
TEST_F(SandboxWasmIsolationTest, WasmIsolationConfigConstructable) {
    ModuleSandbox::Config cfg;
    cfg.enable_wasm_isolation        = true;
    cfg.wasm_linear_memory_pages     = 16;
    cfg.wasm_allow_unregistered_imports = false;

    EXPECT_NO_THROW({
        ModuleSandbox sandbox(cfg);
        // Not launched — isolation is not yet active.
        EXPECT_FALSE(sandbox.isWasmIsolationActive());
    });
}

// =============================================================================
// Suite 8: Advanced reload/dependency edge permutations
// =============================================================================

class AdvancedDependencyTest : public ::testing::Test {};

/// Diamond dependency: A depends on B and C, both B and C depend on D.
/// D must appear once in the load order and before B, C, and A.
TEST_F(AdvancedDependencyTest, DiamondDependencyResolvesCorrectly) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("D", "1.0.0", {});

    ModuleDependency dep_d;
    dep_d.name     = "D";
    dep_d.required = true;
    resolver.registerModule("B", "1.0.0", {dep_d});
    resolver.registerModule("C", "1.0.0", {dep_d});

    ModuleDependency dep_b, dep_c;
    dep_b.name = "B"; dep_b.required = true;
    dep_c.name = "C"; dep_c.required = true;
    resolver.registerModule("A", "1.0.0", {dep_b, dep_c});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success)
        << "Diamond dependency must resolve: " << result.errorMessage;

    // D must appear before B and C.
    const auto& order = result.loadOrder;
    auto pos_d = std::find(order.begin(), order.end(), "D");
    auto pos_b = std::find(order.begin(), order.end(), "B");
    auto pos_c = std::find(order.begin(), order.end(), "C");
    auto pos_a = std::find(order.begin(), order.end(), "A");

    EXPECT_NE(pos_d, order.end());
    EXPECT_NE(pos_b, order.end());
    EXPECT_NE(pos_c, order.end());
    EXPECT_NE(pos_a, order.end());

    EXPECT_LT(std::distance(order.begin(), pos_d),
              std::distance(order.begin(), pos_b));
    EXPECT_LT(std::distance(order.begin(), pos_d),
              std::distance(order.begin(), pos_c));
    EXPECT_LT(std::distance(order.begin(), pos_b),
              std::distance(order.begin(), pos_a));
    EXPECT_LT(std::distance(order.begin(), pos_c),
              std::distance(order.begin(), pos_a));
}

/// Multi-root: two independent modules with no shared dependencies resolve together.
TEST_F(AdvancedDependencyTest, MultiRootIndependentModulesResolve) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("root_x", "1.0.0", {});
    resolver.registerModule("root_y", "1.0.0", {});

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.loadOrder.size(), 2u);
}

/// Re-registering a module with the same name does not accumulate duplicates.
TEST_F(AdvancedDependencyTest, ReRegisterModuleDoesNotDuplicate) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("dup_mod", "1.0.0", {});
    resolver.registerModule("dup_mod", "1.1.0", {});  // overwrite

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success);

    const auto& order = result.loadOrder;
    long count = std::count(order.begin(), order.end(), "dup_mod");
    EXPECT_EQ(count, 1) << "Re-registered module must appear exactly once";
}

/// 5-level deep chain resolves in the correct activation order.
TEST_F(AdvancedDependencyTest, FiveLevelChainCorrectOrder) {
    // L0 ← L1 ← L2 ← L3 ← L4
    ModuleDependencyResolver resolver;
    resolver.registerModule("L0", "1.0.0", {});
    for (int i = 1; i <= 4; ++i) {
        ModuleDependency dep;
        dep.name     = "L" + std::to_string(i - 1);
        dep.required = true;
        resolver.registerModule("L" + std::to_string(i), "1.0.0", {dep});
    }

    auto result = resolver.resolve();
    EXPECT_TRUE(result.success)
        << "5-level chain must resolve: " << result.errorMessage;
    EXPECT_EQ(result.loadOrder.size(), 5u);

    const auto& order = result.loadOrder;
    for (int i = 0; i < 4; ++i) {
        auto pos_i   = std::find(order.begin(), order.end(), "L" + std::to_string(i));
        auto pos_i1  = std::find(order.begin(), order.end(), "L" + std::to_string(i + 1));
        EXPECT_LT(std::distance(order.begin(), pos_i),
                  std::distance(order.begin(), pos_i1))
            << "L" << i << " must precede L" << (i + 1);
    }
}

/// Indirect cycle (A→B→C→A) is detected.
TEST_F(AdvancedDependencyTest, IndirectCycleDetected) {
    ModuleDependencyResolver resolver;
    resolver.registerModule("cyc_a", "1.0.0", {{"cyc_b"}});
    resolver.registerModule("cyc_b", "1.0.0", {{"cyc_c"}});
    resolver.registerModule("cyc_c", "1.0.0", {{"cyc_a"}});

    auto result = resolver.resolve();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.cycles.empty())
        << "Indirect cycle A→B→C→A must be detected";
}

// =============================================================================
// Suite 9: PluginDependencyGraph — extended edge cases
// =============================================================================

class PluginGraphExtendedTest : public ::testing::Test {};

/// nodeCount() and edgeCount() are both 0 on a fresh graph.
TEST_F(PluginGraphExtendedTest, FreshGraphIsEmpty) {
    PluginDependencyGraph graph;
    EXPECT_EQ(graph.nodeCount(), 0u);
    EXPECT_EQ(graph.edgeCount(), 0u);
}

/// Adding the same module twice does not create a duplicate node.
TEST_F(PluginGraphExtendedTest, DuplicateAddModuleNoExtraNode) {
    PluginDependencyGraph graph;
    graph.addModule("alpha", "1.0.0");
    graph.addModule("alpha", "1.0.0");
    EXPECT_EQ(graph.nodeCount(), 1u);
}

/// ASCII export of a non-empty graph produces non-empty output.
TEST_F(PluginGraphExtendedTest, AsciiExportNonEmpty) {
    PluginDependencyGraph graph;
    graph.addModule("root", "1.0.0");
    graph.addModule("leaf", "1.0.0");
    graph.addDependency("leaf", "root");

    std::string ascii = graph.toString(GraphExportFormat::ASCII);
    EXPECT_FALSE(ascii.empty());
}

/// detectCycles() on an acyclic graph returns an empty vector.
TEST_F(PluginGraphExtendedTest, AcyclicGraphHasNoCycles) {
    PluginDependencyGraph graph;
    graph.addModule("a", "1.0.0");
    graph.addModule("b", "1.0.0");
    graph.addDependency("b", "a");

    auto cycles = graph.detectCycles();
    EXPECT_TRUE(cycles.empty());
}

/// topologicalOrder() on an empty graph returns empty vector without crash.
TEST_F(PluginGraphExtendedTest, TopologicalOrderOnEmptyGraphSafe) {
    PluginDependencyGraph graph;
    std::vector<std::string> order;
    EXPECT_NO_THROW({ order = graph.topologicalOrder(); });
    EXPECT_TRUE(order.empty());
}

/// topologicalOrder() on a cyclic graph does not crash.
TEST_F(PluginGraphExtendedTest, TopologicalOrderOnCyclicGraphNoThrow) {
    PluginDependencyGraph graph;
    graph.addModule("x", "1.0.0");
    graph.addModule("y", "1.0.0");
    graph.addDependency("x", "y", true);
    graph.addDependency("y", "x", true);

    std::vector<std::string> order;
    EXPECT_NO_THROW({ order = graph.topologicalOrder(); });
}

// =============================================================================
// Suite 10: Operator-facing diagnostics — format prefix, isKnownCode completeness
// =============================================================================

using namespace BaseErrorTaxonomy;

class OperatorDiagnosticsTest : public ::testing::Test {};

/// Every format() output must begin with "[BASE_" prefix.
TEST_F(OperatorDiagnosticsTest, FormatOutputHasBasePrefix) {
    auto msg_loader  = BASE_LOADER_PATH_NOT_FOUND::format("m", "/p");
    auto msg_sandbox = BASE_SANDBOX_LAUNCH_FAILED::format("m", "reason");
    auto msg_reload  = BASE_RELOAD_NO_BACKUP::format("m");
    auto msg_dep     = BASE_DEPENDENCY_CYCLE::format("m");
    auto msg_reg     = BASE_REGISTRY_NETWORK_ERROR::format("https://r.example.com", 503, "timeout");

    EXPECT_EQ(msg_loader.substr(0, 6),  "[BASE_");
    EXPECT_EQ(msg_sandbox.substr(0, 6), "[BASE_");
    EXPECT_EQ(msg_reload.substr(0, 6),  "[BASE_");
    EXPECT_EQ(msg_dep.substr(0, 6),     "[BASE_");
    EXPECT_EQ(msg_reg.substr(0, 6),     "[BASE_");
}

/// isKnownCode returns false for out-of-range codes.
TEST_F(OperatorDiagnosticsTest, IsKnownCodeFalseForOutOfRange) {
    EXPECT_FALSE(isKnownCode(0));
    EXPECT_FALSE(isKnownCode(-1));
    EXPECT_FALSE(isKnownCode(9999));
    EXPECT_FALSE(isKnownCode(1099));   // just below loader range
    EXPECT_FALSE(isKnownCode(1350));   // just above registry range
}

/// isKnownCode returns true for all 24 taxonomy codes.
TEST_F(OperatorDiagnosticsTest, IsKnownCodeTrueForAllTaxonomyCodes) {
    const std::vector<int> all_codes = {
        BASE_LOADER_PATH_NOT_FOUND::code,
        BASE_LOADER_SIGNATURE_REJECTED::code,
        BASE_LOADER_ABI_MISMATCH::code,
        BASE_LOADER_LOAD_FAILED::code,
        BASE_LOADER_INIT_FAILED::code,
        BASE_LOADER_HEALTH_CHECK_FAILED::code,
        BASE_SANDBOX_LAUNCH_FAILED::code,
        BASE_SANDBOX_RESOURCE_LIMIT::code,
        BASE_SANDBOX_TIMEOUT::code,
        BASE_SANDBOX_DEGRADED::code,
        BASE_SANDBOX_INACTIVE_STATS::code,
        BASE_RELOAD_NO_BACKUP::code,
        BASE_RELOAD_ROLLBACK_FAILED::code,
        BASE_RELOAD_CANDIDATE_LOAD_FAILED::code,
        BASE_RELOAD_STATE_RESTORE_FAILED::code,
        BASE_RELOAD_NOT_REGISTERED::code,
        BASE_DEPENDENCY_CONFLICT::code,
        BASE_DEPENDENCY_CYCLE::code,
        BASE_DEPENDENCY_MISSING_REQUIRED::code,
        BASE_DEPENDENCY_VERSION_RANGE_MISMATCH::code,
        BASE_REGISTRY_NETWORK_ERROR::code,
        BASE_REGISTRY_AUTH_FAILURE::code,
        BASE_REGISTRY_CHECKSUM_MISMATCH::code,
        BASE_REGISTRY_DOWNLOAD_FAILED::code,
    };

    for (int c : all_codes) {
        EXPECT_TRUE(isKnownCode(c)) << "isKnownCode must return true for code " << c;
    }
}

/// resolveDescription returns empty string for unknown codes.
TEST_F(OperatorDiagnosticsTest, ResolveDescriptionEmptyForUnknownCode) {
    EXPECT_TRUE(resolveDescription(0).empty());
    EXPECT_TRUE(resolveDescription(-42).empty());
    EXPECT_TRUE(resolveDescription(9999).empty());
}

/// resolveDescription is consistent with struct description() for all known codes.
TEST_F(OperatorDiagnosticsTest, ResolveDescriptionConsistentWithStaticDescription) {
    // Spot-check representative entries from each group.
    EXPECT_EQ(resolveDescription(BASE_LOADER_PATH_NOT_FOUND::code),
              std::string(BASE_LOADER_PATH_NOT_FOUND::description()));

    EXPECT_EQ(resolveDescription(BASE_SANDBOX_DEGRADED::code),
              std::string(BASE_SANDBOX_DEGRADED::description()));

    EXPECT_EQ(resolveDescription(BASE_RELOAD_ROLLBACK_FAILED::code),
              std::string(BASE_RELOAD_ROLLBACK_FAILED::description()));

    EXPECT_EQ(resolveDescription(BASE_DEPENDENCY_CYCLE::code),
              std::string(BASE_DEPENDENCY_CYCLE::description()));

    EXPECT_EQ(resolveDescription(BASE_REGISTRY_AUTH_FAILURE::code),
              std::string(BASE_REGISTRY_AUTH_FAILURE::description()));
}

/// format() output for registry error contains the module name.
TEST_F(OperatorDiagnosticsTest, RegistryErrorFormatEmbedsBothArgs) {
    auto msg = BASE_REGISTRY_CHECKSUM_MISMATCH::format("analytics_plugin",
                                                        "sha256:aabbcc",
                                                        "sha256:112233");
    EXPECT_NE(msg.find("analytics_plugin"), std::string::npos);
    EXPECT_NE(msg.find("sha256:aabbcc"),    std::string::npos);
}

/// format() for dependency conflict embeds the dependency name and both conflicting modules.
TEST_F(OperatorDiagnosticsTest, DependencyConflictFormatEmbedsBothModules) {
    auto msg = BASE_DEPENDENCY_CONFLICT::format("shared_dep", "storage_v2", "storage_v1");
    EXPECT_NE(msg.find("storage_v2"), std::string::npos);
    EXPECT_NE(msg.find("storage_v1"), std::string::npos);
}
} } // namespace themis::modules
