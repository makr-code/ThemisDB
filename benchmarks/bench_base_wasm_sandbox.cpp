/*
 * ThemisDB | File: bench_base_wasm_sandbox.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * Status: Production Ready
 * Issue: #5631 — base module future enhancement items (mid-term Q1 2027)
 */

/**
 * @file bench_base_wasm_sandbox.cpp
 * @brief Dedicated base-module benchmarks for wasm/sandbox and taxonomy hot paths.
 *
 * Covers the mid-term roadmap target of "reduce proxy-like mappings through
 * additional dedicated base microbenchmarks" by providing first-class fixtures
 * for surfaces that were previously measured only via proxy:
 *
 * Release gates (extended set):
 *  - GATE-BASE-07: WasmPluginSandbox::loadFromBytes() valid WASM     throughput
 *  - GATE-BASE-08: WasmPluginSandbox::loadFromBytes() invalid bytes  fast-fail
 *  - GATE-BASE-09: AbiChecker::checkVersions()                       throughput
 *  - GATE-BASE-10: BaseErrorTaxonomy::resolveDescription()           throughput
 *  - GATE-BASE-11: BaseErrorTaxonomy::isKnownCode()                  throughput
 *  - GATE-BASE-12: ModuleSandbox::stats() on inactive sandbox        throughput
 *
 * Additional microbenchmarks (mid-term p95/p99 re-baseline):
 *  - WasmPluginSandbox::addHostFunction() and clearHostFunctions()
 *  - WasmPluginSandbox::hostFunctionCount()
 *  - WasmPluginSandbox::callExport() without runtime (fast-fail)
 *  - AbiChecker::checkRequiredSymbols() empty-list fast path
 *  - PluginDependencyGraph DOT/JSON/ASCII export throughput
 *  - ModuleSandbox construction/destruction throughput
 *
 * Measurement hygiene: all benchmarks use Google Benchmark fixtures, no I/O,
 * no real-time mode needed (CPU-bound operations only).  No external deps
 * beyond themis_core and libbenchmark.
 */

#include <benchmark/benchmark.h>

#include "themis/base/module_loader.h"
#include "themis/base/module_sandbox.h"
#include "themis/base/plugin_dependency_graph.h"
#include "themis/base/wasm_plugin_sandbox.h"
#include "themis/base/base_error_taxonomy.h"

#include <string>
#include <vector>

using namespace themis::modules;
using namespace themis::modules::BaseErrorTaxonomy;

// =============================================================================
// Shared helpers
// =============================================================================

namespace {

/// Minimal valid WASM binary (magic + version, no sections).
static const std::vector<uint8_t> kMinimalWasm = {
    0x00, 0x61, 0x73, 0x6d,   // magic \0asm
    0x01, 0x00, 0x00, 0x00    // version 1 LE
};

/// Invalid bytes (not WASM).
static const std::vector<uint8_t> kInvalidBytes = {
    0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01
};

} // anonymous namespace

// =============================================================================
// GATE-BASE-07: WasmPluginSandbox::loadFromBytes() — valid WASM throughput
// =============================================================================

class WasmLoadBytesFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        sandbox = std::make_unique<WasmPluginSandbox>();
    }

    void TearDown(const benchmark::State& /*s*/) override {
        sandbox.reset();
    }

    std::unique_ptr<WasmPluginSandbox> sandbox;
};

/// @note GATE-BASE-07: loadFromBytes() valid WASM, including unload per iteration
BENCHMARK_F(WasmLoadBytesFixture,
            GateBase07_LoadFromBytes_ValidWasm)(benchmark::State& state) {
    for (auto _ : state) {
        bool ok = sandbox->loadFromBytes(kMinimalWasm, "bench_wasm_mod");
        benchmark::DoNotOptimize(ok);
        sandbox->unload();
    }
    state.SetLabel("GATE-BASE-07: loadFromBytes() valid WASM (load+unload cycle)");
}

/// @note GATE-BASE-07 (load-only): loadFromBytes without unload (measures first load)
static void BM_GateBase07_LoadFromBytes_ValidWasm_FirstLoad(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        WasmPluginSandbox sb;
        state.ResumeTiming();

        bool ok = sb.loadFromBytes(kMinimalWasm, "bench_wasm_first");
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("GATE-BASE-07: loadFromBytes() valid WASM — first load per sandbox");
}
BENCHMARK(BM_GateBase07_LoadFromBytes_ValidWasm_FirstLoad);

// =============================================================================
// GATE-BASE-08: WasmPluginSandbox::loadFromBytes() — invalid bytes fast-fail
// =============================================================================

/// @note GATE-BASE-08: loadFromBytes() invalid bytes — fast-fail throughput
BENCHMARK_F(WasmLoadBytesFixture,
            GateBase08_LoadFromBytes_InvalidFastFail)(benchmark::State& state) {
    for (auto _ : state) {
        bool ok = sandbox->loadFromBytes(kInvalidBytes, "bench_invalid");
        benchmark::DoNotOptimize(ok);
        // No unload needed — load failed.
    }
    state.SetLabel("GATE-BASE-08: loadFromBytes() invalid bytes fast-fail");
}

/// @note GATE-BASE-08 (empty): loadFromBytes() empty buffer fast-fail
BENCHMARK_F(WasmLoadBytesFixture,
            GateBase08_LoadFromBytes_EmptyFastFail)(benchmark::State& state) {
    for (auto _ : state) {
        bool ok = sandbox->loadFromBytes({}, "bench_empty");
        benchmark::DoNotOptimize(ok);
    }
    state.SetLabel("GATE-BASE-08: loadFromBytes() empty buffer fast-fail");
}

// =============================================================================
// GATE-BASE-09: AbiChecker::checkVersions() throughput
// =============================================================================

class AbiCheckerFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        checker = std::make_unique<AbiChecker>();

        meta_compat.themisMajor = 1;
        meta_compat.themisMinor = 2;
        meta_compat.version     = "1.2.0";

        meta_incompat.themisMajor = 2;
        meta_incompat.themisMinor = 0;
        meta_incompat.version     = "2.0.0";
    }

    void TearDown(const benchmark::State& /*s*/) override {
        checker.reset();
    }

    std::unique_ptr<AbiChecker> checker;
    ModuleMetadata meta_compat;
    ModuleMetadata meta_incompat;
};

/// @note GATE-BASE-09: checkVersions() — compatible path throughput
BENCHMARK_F(AbiCheckerFixture,
            GateBase09_CheckVersions_Compatible)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = checker->checkVersions(meta_compat, 1, 2);
        benchmark::DoNotOptimize(result.compatible);
    }
    state.SetLabel("GATE-BASE-09: checkVersions() compatible — gate: throughput");
}

/// @note GATE-BASE-09 (incompatible): checkVersions() — major mismatch path
BENCHMARK_F(AbiCheckerFixture,
            GateBase09_CheckVersions_Incompatible)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = checker->checkVersions(meta_incompat, 1, 0);
        benchmark::DoNotOptimize(result.compatible);
    }
    state.SetLabel("GATE-BASE-09: checkVersions() incompatible (major mismatch)");
}

/// @note GATE-BASE-09: checkRequiredSymbols() — empty required-list fast path
BENCHMARK_F(AbiCheckerFixture,
            GateBase09_CheckRequiredSymbols_EmptyList)(benchmark::State& state) {
    // No required symbols registered → immediate pass.
    for (auto _ : state) {
        auto result = checker->checkRequiredSymbols(nullptr);
        benchmark::DoNotOptimize(result.compatible);
    }
    state.SetLabel("GATE-BASE-09: checkRequiredSymbols() empty list — fast path");
}

// =============================================================================
// GATE-BASE-10: BaseErrorTaxonomy::resolveDescription() throughput
// =============================================================================

/// @note GATE-BASE-10: resolveDescription() for a known code throughput
static void BM_GateBase10_ResolveDescription_Known(benchmark::State& state) {
    constexpr int kCode = BASE_LOADER_ABI_MISMATCH::code;
    for (auto _ : state) {
        auto desc = resolveDescription(kCode);
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-BASE-10: resolveDescription() known code throughput");
}
BENCHMARK(BM_GateBase10_ResolveDescription_Known);

/// @note GATE-BASE-10 (unknown): resolveDescription() for an unknown code (returns empty)
static void BM_GateBase10_ResolveDescription_Unknown(benchmark::State& state) {
    for (auto _ : state) {
        auto desc = resolveDescription(9999);
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-BASE-10: resolveDescription() unknown code fast-fail");
}
BENCHMARK(BM_GateBase10_ResolveDescription_Unknown);

/// @note GATE-BASE-10: format() diagnostic builder throughput (loader path)
static void BM_GateBase10_Format_LoaderPath(benchmark::State& state) {
    const std::string mod  = "bench_module";
    const std::string path = "/tmp/bench.so";
    for (auto _ : state) {
        auto msg = BASE_LOADER_PATH_NOT_FOUND::format(mod, path);
        benchmark::DoNotOptimize(msg);
    }
    state.SetLabel("GATE-BASE-10: BASE_LOADER_PATH_NOT_FOUND::format() throughput");
}
BENCHMARK(BM_GateBase10_Format_LoaderPath);

/// @note GATE-BASE-10: format() diagnostic builder throughput (ABI mismatch)
static void BM_GateBase10_Format_AbiMismatch(benchmark::State& state) {
    const std::string mod = "analytics_plugin";
    const std::string hv  = "1.0";
    const std::string mv  = "2.0";
    for (auto _ : state) {
        auto msg = BASE_LOADER_ABI_MISMATCH::format(mod, hv, mv);
        benchmark::DoNotOptimize(msg);
    }
    state.SetLabel("GATE-BASE-10: BASE_LOADER_ABI_MISMATCH::format() throughput");
}
BENCHMARK(BM_GateBase10_Format_AbiMismatch);

// =============================================================================
// GATE-BASE-11: BaseErrorTaxonomy::isKnownCode() throughput
// =============================================================================

/// @note GATE-BASE-11: isKnownCode() — known code throughput
static void BM_GateBase11_IsKnownCode_Known(benchmark::State& state) {
    constexpr int kCode = BASE_SANDBOX_DEGRADED::code;
    for (auto _ : state) {
        bool known = isKnownCode(kCode);
        benchmark::DoNotOptimize(known);
    }
    state.SetLabel("GATE-BASE-11: isKnownCode() known code — gate: throughput");
}
BENCHMARK(BM_GateBase11_IsKnownCode_Known);

/// @note GATE-BASE-11 (unknown): isKnownCode() — out-of-range fast-fail throughput
static void BM_GateBase11_IsKnownCode_Unknown(benchmark::State& state) {
    for (auto _ : state) {
        bool known = isKnownCode(9999);
        benchmark::DoNotOptimize(known);
    }
    state.SetLabel("GATE-BASE-11: isKnownCode() unknown code fast-fail");
}
BENCHMARK(BM_GateBase11_IsKnownCode_Unknown);

// =============================================================================
// GATE-BASE-12: ModuleSandbox::stats() on inactive sandbox throughput
// =============================================================================

class SandboxStatsFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        ModuleSandbox::Config cfg;
        cfg.max_memory_mb   = 64;
        cfg.max_cpu_percent = 25;
        sandbox = std::make_unique<ModuleSandbox>(cfg);
        // Not launched — inactive sandbox, fast stats() path.
    }

    void TearDown(const benchmark::State& /*s*/) override {
        sandbox.reset();
    }

    std::unique_ptr<ModuleSandbox> sandbox;
};

/// @note GATE-BASE-12: stats() on inactive sandbox — fast path throughput
BENCHMARK_F(SandboxStatsFixture,
            GateBase12_Stats_InactiveSandbox)(benchmark::State& state) {
    for (auto _ : state) {
        auto s = sandbox->stats();
        benchmark::DoNotOptimize(s.peak_memory_bytes);
    }
    state.SetLabel("GATE-BASE-12: ModuleSandbox::stats() inactive — gate: throughput");
}

/// @note GATE-BASE-12: isActive() on inactive sandbox
BENCHMARK_F(SandboxStatsFixture,
            GateBase12_IsActive_InactiveSandbox)(benchmark::State& state) {
    for (auto _ : state) {
        bool active = sandbox->isActive();
        benchmark::DoNotOptimize(active);
    }
    state.SetLabel("GATE-BASE-12: ModuleSandbox::isActive() inactive path");
}

/// @note GATE-BASE-12: isWasmIsolationActive() predicate throughput
BENCHMARK_F(SandboxStatsFixture,
            GateBase12_IsWasmIsolationActive)(benchmark::State& state) {
    for (auto _ : state) {
        bool wasm = sandbox->isWasmIsolationActive();
        benchmark::DoNotOptimize(wasm);
    }
    state.SetLabel("GATE-BASE-12: ModuleSandbox::isWasmIsolationActive() predicate");
}

// =============================================================================
// Additional microbenchmarks: WasmPluginSandbox management hot paths
// =============================================================================

/// WasmPluginSandbox construction/destruction throughput.
static void BM_WasmSandbox_ConstructDestruct(benchmark::State& state) {
    for (auto _ : state) {
        WasmPluginSandbox sandbox;
        benchmark::DoNotOptimize(sandbox.isLoaded());
    }
    state.SetLabel("WasmPluginSandbox construction/destruction throughput");
}
BENCHMARK(BM_WasmSandbox_ConstructDestruct);

/// addHostFunction() throughput (single registration per call).
static void BM_WasmSandbox_AddHostFunction(benchmark::State& state) {
    WasmPluginSandbox sandbox;
    const WasmHostFunction fn = {
        "themis", "log",
        [](uint8_t*, size_t, const std::vector<uint8_t>&, std::vector<uint8_t>&) {
            return true;
        },
        "bench log"
    };

    for (auto _ : state) {
        state.PauseTiming();
        sandbox.clearHostFunctions();
        state.ResumeTiming();

        sandbox.addHostFunction(fn);
        benchmark::DoNotOptimize(sandbox.hostFunctionCount());
    }
    state.SetLabel("WasmPluginSandbox::addHostFunction() single registration");
}
BENCHMARK(BM_WasmSandbox_AddHostFunction);

/// hostFunctionCount() read throughput.
static void BM_WasmSandbox_HostFunctionCount(benchmark::State& state) {
    WasmPluginSandbox sandbox;
    for (int i = 0; i < 10; ++i) {
        sandbox.addHostFunction({
            "themis", "fn_" + std::to_string(i),
            [](uint8_t*, size_t,
               const std::vector<uint8_t>&,
               std::vector<uint8_t>&) { return true; },
            ""
        });
    }

    for (auto _ : state) {
        size_t cnt = sandbox.hostFunctionCount();
        benchmark::DoNotOptimize(cnt);
    }
    state.SetLabel("WasmPluginSandbox::hostFunctionCount() with 10 functions");
}
BENCHMARK(BM_WasmSandbox_HostFunctionCount);

/// callExport() without runtime — fast-fail path throughput.
static void BM_WasmSandbox_CallExport_NoRuntime(benchmark::State& state) {
    WasmPluginSandbox sandbox;
    sandbox.loadFromBytes(kMinimalWasm, "bench_no_runtime");

    for (auto _ : state) {
        WasmCallResult result = sandbox.callExport("run", {});
        benchmark::DoNotOptimize(result.success);
    }
    state.SetLabel("WasmPluginSandbox::callExport() no-runtime fast-fail");
}
BENCHMARK(BM_WasmSandbox_CallExport_NoRuntime);

// =============================================================================
// Additional microbenchmarks: PluginDependencyGraph export throughput
// =============================================================================

class GraphExportFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        graph = std::make_unique<PluginDependencyGraph>();

        // Build a 20-node chain for export benchmarks.
        graph->addModule("exp_0", "1.0.0");
        for (int i = 1; i < 20; ++i) {
            graph->addModule("exp_" + std::to_string(i), "1.0.0");
            graph->addDependency("exp_" + std::to_string(i),
                                 "exp_" + std::to_string(i - 1));
        }
    }

    void TearDown(const benchmark::State& /*s*/) override {
        graph.reset();
    }

    std::unique_ptr<PluginDependencyGraph> graph;
};

/// DOT export throughput on a 20-node chain.
BENCHMARK_F(GraphExportFixture, GraphExport_DOT_20Node)(benchmark::State& state) {
    for (auto _ : state) {
        auto dot = graph->toString(GraphExportFormat::DOT);
        benchmark::DoNotOptimize(dot);
    }
    state.SetLabel("PluginDependencyGraph DOT export — 20-node chain");
}

/// JSON export throughput on a 20-node chain.
BENCHMARK_F(GraphExportFixture, GraphExport_JSON_20Node)(benchmark::State& state) {
    for (auto _ : state) {
        auto json = graph->toString(GraphExportFormat::JSON);
        benchmark::DoNotOptimize(json);
    }
    state.SetLabel("PluginDependencyGraph JSON export — 20-node chain");
}

/// ASCII export throughput on a 20-node chain.
BENCHMARK_F(GraphExportFixture, GraphExport_ASCII_20Node)(benchmark::State& state) {
    for (auto _ : state) {
        auto ascii = graph->toString(GraphExportFormat::ASCII);
        benchmark::DoNotOptimize(ascii);
    }
    state.SetLabel("PluginDependencyGraph ASCII export — 20-node chain");
}

/// detectCycles() throughput on a known-acyclic graph.
BENCHMARK_F(GraphExportFixture, GraphDetectCycles_Acyclic)(benchmark::State& state) {
    for (auto _ : state) {
        auto cycles = graph->detectCycles();
        benchmark::DoNotOptimize(cycles);
    }
    state.SetLabel("PluginDependencyGraph::detectCycles() acyclic 20-node chain");
}

// =============================================================================
// Additional microbenchmarks: ModuleSandbox construction throughput
// =============================================================================

/// ModuleSandbox default construction/destruction throughput.
static void BM_ModuleSandbox_ConstructDestruct_Default(benchmark::State& state) {
    for (auto _ : state) {
        ModuleSandbox sandbox;
        benchmark::DoNotOptimize(sandbox.isActive());
    }
    state.SetLabel("ModuleSandbox default construction/destruction");
}
BENCHMARK(BM_ModuleSandbox_ConstructDestruct_Default);

/// ModuleSandbox construction with non-default config throughput.
static void BM_ModuleSandbox_ConstructDestruct_Config(benchmark::State& state) {
    ModuleSandbox::Config cfg;
    cfg.max_memory_mb        = 64;
    cfg.max_cpu_percent      = 25;
    cfg.enable_wasm_isolation = false;

    for (auto _ : state) {
        ModuleSandbox sandbox(cfg);
        benchmark::DoNotOptimize(sandbox.isActive());
    }
    state.SetLabel("ModuleSandbox construction with config (no wasm isolation)");
}
BENCHMARK(BM_ModuleSandbox_ConstructDestruct_Config);

/// lastError() read on a fresh (never-launched) sandbox.
static void BM_ModuleSandbox_LastError_Fresh(benchmark::State& state) {
    ModuleSandbox sandbox;
    for (auto _ : state) {
        const std::string& err = sandbox.lastError();
        benchmark::DoNotOptimize(err);
    }
    state.SetLabel("ModuleSandbox::lastError() on fresh sandbox (empty string path)");
}
BENCHMARK(BM_ModuleSandbox_LastError_Fresh);

BENCHMARK_MAIN();
