/*
 * ThemisDB | File: bench_base_hot_paths.cpp | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * Status: Production Ready
 * Issue: #5631 — base module open roadmap and future enhancement items
 */

/**
 * @file bench_base_hot_paths.cpp
 * @brief Google Benchmark release-gate benchmarks for base module hot paths.
 *
 * Release gates:
 *  - GATE-BASE-01: ModuleLoader::isModuleLoaded()          throughput ≥ 500k ops/s
 *  - GATE-BASE-02: ModuleLoader::getMetrics()              throughput ≥ 100k ops/s
 *  - GATE-BASE-03: HotReloadManager::registeredModules()   p99 ≤ 5µs  (10 modules)
 *  - GATE-BASE-04: HotReloadManager::isRollbackAvailable() p99 ≤ 1µs
 *  - GATE-BASE-05: PluginDependencyGraph::buildFromResolver() for 100-node chain ≤ 1ms
 *  - GATE-BASE-06: HotReloadManager::reloadModule()        fast-fail path ≤ 50µs
 *
 * Pattern: mirrors bench_hot_reload_manager.cpp and bench_module_load_hot_reload.cpp
 */

#include <benchmark/benchmark.h>
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include "themis/base/plugin_dependency_graph.h"

#include <string>
#include <vector>

using namespace themis::modules;

// =============================================================================
// Shared fixture helpers
// =============================================================================

namespace {

HotReloadManager::Config permissiveHotReloadConfig() {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    cfg.preserveState   = false;
    cfg.enableRollback  = true;
    return cfg;
}

} // anonymous namespace

// =============================================================================
// ModuleLoader fixture
// =============================================================================

class BaseHotPathsLoaderFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        loader = std::make_unique<ModuleLoader>();
        loader->setAllowUnsigned(true);
    }

    void TearDown(const benchmark::State& /*s*/) override {
        loader->unloadAllModules();
        loader.reset();
    }

    std::unique_ptr<ModuleLoader> loader;
};

// =============================================================================
// GATE-BASE-01: ModuleLoader::isModuleLoaded() throughput ≥ 500k ops/s
// =============================================================================

/// @note GATE-BASE-01: isModuleLoaded() throughput ≥ 500k ops/s
BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase01_IsModuleLoaded)(benchmark::State& state) {
    for (auto _ : state) {
        bool loaded = loader->isModuleLoaded("bench_not_loaded");
        benchmark::DoNotOptimize(loaded);
    }
    state.SetLabel("GATE-BASE-01: isModuleLoaded() negative path — gate: ≥500k ops/s");
}

/// @note GATE-BASE-01 (variant): isModuleLoaded with pre-populated loader
BENCHMARK_F(BaseHotPathsLoaderFixture,
            GateBase01_IsModuleLoaded_WithLoadedModules)(benchmark::State& state) {
    // Attempt loads (will fail — no real .so, but metadata is exercised)
    for (int i = 0; i < 10; ++i) {
        loader->loadModule("/tmp/bench_base_hot_" + std::to_string(i) + ".so",
                           "bench_base_hot_" + std::to_string(i));
    }
    for (auto _ : state) {
        bool loaded = loader->isModuleLoaded("bench_base_hot_0");
        benchmark::DoNotOptimize(loaded);
    }
    state.SetLabel("GATE-BASE-01: isModuleLoaded() after 10 attempted loads");
}

// =============================================================================
// GATE-BASE-02: ModuleLoader::getMetrics() throughput ≥ 100k ops/s
// =============================================================================

/// @note GATE-BASE-02: getMetrics() throughput ≥ 100k ops/s
BENCHMARK_F(BaseHotPathsLoaderFixture, GateBase02_GetMetrics)(benchmark::State& state) {
    // Populate metrics with a few failed load attempts.
    for (int i = 0; i < 5; ++i) {
        loader->loadModule("/tmp/bench_metrics_" + std::to_string(i) + ".so",
                           "bench_metrics");
    }

    for (auto _ : state) {
        auto m = loader->getMetrics();
        benchmark::DoNotOptimize(m.totalLoadAttempts);
    }
    state.SetLabel("GATE-BASE-02: getMetrics() after 5 load attempts — gate: ≥100k ops/s");
}

/// @note GATE-BASE-02 (zero-state variant): getMetrics() on a fresh loader
BENCHMARK_F(BaseHotPathsLoaderFixture,
            GateBase02_GetMetrics_FreshLoader)(benchmark::State& state) {
    for (auto _ : state) {
        auto m = loader->getMetrics();
        benchmark::DoNotOptimize(m.successfulLoads);
    }
    state.SetLabel("GATE-BASE-02: getMetrics() fresh loader — gate: ≥100k ops/s");
}

// =============================================================================
// HotReloadManager fixture
// =============================================================================

class BaseHotPathsReloadFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        mgr    = std::make_unique<HotReloadManager>(permissiveHotReloadConfig());
        loader = std::make_unique<ModuleLoader>();
        loader->setAllowUnsigned(true);
    }

    void TearDown(const benchmark::State& /*s*/) override {
        mgr.reset();
        loader.reset();
    }

    std::unique_ptr<HotReloadManager> mgr;
    std::unique_ptr<ModuleLoader>     loader;
};

// =============================================================================
// GATE-BASE-03: HotReloadManager::registeredModules() p99 ≤ 5µs (10 modules)
// =============================================================================

/// @note GATE-BASE-03: registeredModules() p99 ≤ 5µs with 10 modules registered
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase03_RegisteredModules_10)(benchmark::State& state) {
    const int kN = 10;
    for (int i = 0; i < kN; ++i) {
        mgr->registerModule("gate03_mod_" + std::to_string(i), *loader);
    }

    for (auto _ : state) {
        auto names = mgr->registeredModules();
        benchmark::DoNotOptimize(names);
    }

    state.SetItemsProcessed(state.iterations() * kN);
    state.SetLabel("GATE-BASE-03: registeredModules() 10 modules — gate: p99 ≤ 5µs");
}

/// @note GATE-BASE-03 (scaling variant): registeredModules() with N modules
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase03_RegisteredModules_Scaling)(benchmark::State& state) {
    const int kN = static_cast<int>(state.range(0));
    for (int i = 0; i < kN; ++i) {
        mgr->registerModule("gate03_scale_" + std::to_string(i), *loader);
    }

    for (auto _ : state) {
        auto names = mgr->registeredModules();
        benchmark::DoNotOptimize(names);
    }

    state.SetItemsProcessed(state.iterations() * kN);
    state.SetLabel("GATE-BASE-03: registeredModules() scaling — gate: p99 ≤ 5µs");
}
BENCHMARK_REGISTER_F(BaseHotPathsReloadFixture,
                     GateBase03_RegisteredModules_Scaling)
    ->Arg(1)->Arg(10)->Arg(50)->Arg(100);

// =============================================================================
// GATE-BASE-04: HotReloadManager::isRollbackAvailable() p99 ≤ 1µs
// =============================================================================

/// @note GATE-BASE-04: isRollbackAvailable() p99 ≤ 1µs — registered, no backup
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase04_IsRollbackAvailable_NoBackup)(benchmark::State& state) {
    mgr->registerModule("gate04_mod", *loader);

    for (auto _ : state) {
        bool avail = mgr->isRollbackAvailable("gate04_mod");
        benchmark::DoNotOptimize(avail);
    }

    state.SetLabel("GATE-BASE-04: isRollbackAvailable() — registered, no backup — gate: p99 ≤ 1µs");
}

/// @note GATE-BASE-04 (not-registered path): isRollbackAvailable() on unknown module
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase04_IsRollbackAvailable_NotRegistered)(benchmark::State& state) {
    for (auto _ : state) {
        bool avail = mgr->isRollbackAvailable("unregistered_gate04");
        benchmark::DoNotOptimize(avail);
    }

    state.SetLabel("GATE-BASE-04: isRollbackAvailable() — not registered — gate: p99 ≤ 1µs");
}

// =============================================================================
// GATE-BASE-05: PluginDependencyGraph::buildFromResolver() 100-node chain ≤ 1ms
// =============================================================================

/// @note GATE-BASE-05: buildFromResolver() for 100-node chain ≤ 1ms
static void BM_GateBase05_BuildFromResolver_100Node(benchmark::State& state) {
    const int kN = 100;

    // Build resolver once outside the timed loop — we benchmark the graph build.
    ModuleDependencyResolver resolver;
    resolver.registerModule("chain_0", "1.0.0", {});
    for (int i = 1; i < kN; ++i) {
        ModuleDependency dep;
        dep.name     = "chain_" + std::to_string(i - 1);
        dep.required = true;
        resolver.registerModule("chain_" + std::to_string(i), "1.0.0", {dep});
    }

    for (auto _ : state) {
        PluginDependencyGraph graph;
        graph.buildFromResolver(resolver);
        benchmark::DoNotOptimize(graph.nodeCount());
    }

    state.SetLabel("GATE-BASE-05: buildFromResolver() 100-node chain — gate: ≤ 1ms");
}
BENCHMARK(BM_GateBase05_BuildFromResolver_100Node);

/// @note GATE-BASE-05 (scaling): buildFromResolver() with variable chain length
static void BM_GateBase05_BuildFromResolver_Scaling(benchmark::State& state) {
    const int kN = static_cast<int>(state.range(0));

    ModuleDependencyResolver resolver;
    resolver.registerModule("sc_chain_0", "1.0.0", {});
    for (int i = 1; i < kN; ++i) {
        ModuleDependency dep;
        dep.name     = "sc_chain_" + std::to_string(i - 1);
        dep.required = true;
        resolver.registerModule("sc_chain_" + std::to_string(i), "1.0.0", {dep});
    }

    for (auto _ : state) {
        PluginDependencyGraph graph;
        graph.buildFromResolver(resolver);
        benchmark::DoNotOptimize(graph.edgeCount());
    }

    state.SetLabel("GATE-BASE-05: buildFromResolver() scaling — gate: ≤ 1ms @ 100 nodes");
}
BENCHMARK(BM_GateBase05_BuildFromResolver_Scaling)
    ->Arg(10)->Arg(50)->Arg(100)->Arg(200);

/// @note GATE-BASE-05 (topological order): topologicalOrder() on 100-node chain
static void BM_GateBase05_TopologicalOrder_100Node(benchmark::State& state) {
    const int kN = 100;
    ModuleDependencyResolver resolver;
    resolver.registerModule("topo_0", "1.0.0", {});
    for (int i = 1; i < kN; ++i) {
        ModuleDependency dep;
        dep.name     = "topo_" + std::to_string(i - 1);
        dep.required = true;
        resolver.registerModule("topo_" + std::to_string(i), "1.0.0", {dep});
    }

    PluginDependencyGraph graph;
    graph.buildFromResolver(resolver);

    for (auto _ : state) {
        auto order = graph.topologicalOrder();
        benchmark::DoNotOptimize(order);
    }

    state.SetLabel("GATE-BASE-05: topologicalOrder() 100-node chain");
}
BENCHMARK(BM_GateBase05_TopologicalOrder_100Node);

// =============================================================================
// GATE-BASE-06: HotReloadManager::reloadModule() fast-fail path ≤ 50µs
// =============================================================================

/// @note GATE-BASE-06: reloadModule() fast-fail (non-existent binary) ≤ 50µs
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase06_ReloadModule_FastFail)(benchmark::State& state) {
    mgr->registerModule("gate06_target", *loader);

    for (auto _ : state) {
        auto result = mgr->reloadModule("gate06_target", "/nonexistent_gate06.so");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("GATE-BASE-06: reloadModule() fast-fail — gate: ≤ 50µs");
}

/// @note GATE-BASE-06 (unregistered path): reloadModule() on unregistered module
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase06_ReloadModule_Unregistered)(benchmark::State& state) {
    for (auto _ : state) {
        auto result = mgr->reloadModule("gate06_ghost", "/nonexistent.so");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("GATE-BASE-06: reloadModule() unregistered-module fast-fail — gate: ≤ 50µs");
}

/// @note GATE-BASE-06 (with callbacks): reloadModule() fast-fail with 5 no-op callbacks
BENCHMARK_F(BaseHotPathsReloadFixture,
            GateBase06_ReloadModule_FastFail_WithCallbacks)(benchmark::State& state) {
    mgr->registerModule("gate06_cb_target", *loader);
    for (int i = 0; i < 5; ++i) {
        mgr->addReloadCallback(
            [](const std::string&, HotReloadManager::ReloadPhase) {});
    }

    for (auto _ : state) {
        auto result = mgr->reloadModule("gate06_cb_target", "/nonexistent_gate06.so");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("GATE-BASE-06: reloadModule() fast-fail + 5 callbacks — gate: ≤ 50µs");
}

// =============================================================================
// Additional hot-path coverage: getCurrentVersion, getStats, rollback
// =============================================================================

/// getCurrentVersion on a registered-but-unloaded module.
BENCHMARK_F(BaseHotPathsReloadFixture,
            HotPath_GetCurrentVersion_Unloaded)(benchmark::State& state) {
    mgr->registerModule("ver_bench_mod", *loader);

    for (auto _ : state) {
        auto ver = mgr->getCurrentVersion("ver_bench_mod");
        benchmark::DoNotOptimize(ver.has_value());
    }

    state.SetLabel("getCurrentVersion() — unloaded module (nullopt path)");
}

/// getStats() read overhead.
BENCHMARK_F(BaseHotPathsReloadFixture,
            HotPath_GetStats)(benchmark::State& state) {
    mgr->registerModule("stats_bench_mod", *loader);
    for (int i = 0; i < 10; ++i) {
        mgr->reloadModule("stats_bench_mod", "/nonexistent.so");
    }

    for (auto _ : state) {
        auto s = mgr->getStats();
        benchmark::DoNotOptimize(s.totalReloads);
    }

    state.SetLabel("getStats() after 10 reload attempts");
}

/// rollback() fast-fail (no backup available).
BENCHMARK_F(BaseHotPathsReloadFixture,
            HotPath_Rollback_NoBackup)(benchmark::State& state) {
    mgr->registerModule("rollback_bench_mod", *loader);

    for (auto _ : state) {
        auto result = mgr->rollback("rollback_bench_mod");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("rollback() — no backup available (early-return path)");
}

BENCHMARK_MAIN();
