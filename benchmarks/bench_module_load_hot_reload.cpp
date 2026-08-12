/**
 * @file bench_module_load_hot_reload.cpp
 * @brief Performance benchmarks for base module load and hot-reload cycles
 *
 * Validates:
 *   BASE-PHASE3-PLANNED: Performance benchmarks for module load and hot-reload cycles
 *   (Issue #1575)
 *
 * Scenarios:
 *   - ModuleLoader: loadModule() fast-fail (non-existent path) — overhead baseline
 *   - ModuleLoader: unloadModule() on unknown name — overhead baseline
 *   - ModuleLoader: loadAllModules() with empty directory
 *   - ModuleLoader: getMetrics() query cost
 *   - ModuleLoader: isLoaded() / isModuleLoaded() query throughput
 *   - ModuleLoader: audit trail size growth
 *   - HotReloadManager: registration + reload cycle (existing coverage extended)
 *   - HotReloadManager: concurrent reload scalability (1..16 threads)
 */

#include <benchmark/benchmark.h>
#include "themis/base/module_loader.h"
#include "themis/base/hot_reload_manager.h"

#include <atomic>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

using namespace themis::modules;
namespace fs = std::filesystem;

// ─── ModuleLoader fixture ─────────────────────────────────────────────────────

class ModuleLoaderFixture : public benchmark::Fixture {
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

// ─── 1. loadModule() fast-fail (missing file) ────────────────────────────────

BENCHMARK_F(ModuleLoaderFixture, LoadModule_MissingFile)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        auto result = loader->loadModule(
            "/tmp/bench_nonexistent_module_" + std::to_string(n++) + ".so",
            "bench_mod");
        benchmark::DoNotOptimize(result.success);
    }
    state.SetLabel("loadModule() fast-fail (file not found)");
}

// ─── 2. unloadModule() on unknown name ───────────────────────────────────────

BENCHMARK_F(ModuleLoaderFixture, UnloadModule_Unknown)(benchmark::State& state) {
    for (auto _ : state) {
        loader->unloadModule("nonexistent_bench_module");
    }
    state.SetLabel("unloadModule() on unregistered name — early-return cost");
}

// ─── 3. isModuleLoaded() query — always false path ───────────────────────────

BENCHMARK_F(ModuleLoaderFixture, IsModuleLoaded_NotLoaded)(benchmark::State& state) {
    for (auto _ : state) {
        bool loaded = loader->isModuleLoaded("bench_nonexistent");
        benchmark::DoNotOptimize(loaded);
    }
    state.SetLabel("isModuleLoaded() negative path");
}

// ─── 4. loadAllModules() on empty / nonexistent directory ───────────────────

BENCHMARK_F(ModuleLoaderFixture, LoadAllModules_EmptyDir)(benchmark::State& state) {
    const char* kEmptyDir = "/tmp/bench_empty_module_dir";
    fs::create_directories(kEmptyDir);

    for (auto _ : state) {
        size_t count = loader->loadAllModules(kEmptyDir);
        benchmark::DoNotOptimize(count);
    }

    fs::remove_all(kEmptyDir);
    state.SetLabel("loadAllModules() on empty directory");
}

// ─── 5. getMetrics() overhead ────────────────────────────────────────────────

BENCHMARK_F(ModuleLoaderFixture, GetMetrics)(benchmark::State& state) {
    // Trigger a few load attempts to populate metrics.
    for (int i = 0; i < 10; ++i) {
        loader->loadModule("/tmp/bench_miss_" + std::to_string(i) + ".so", "bench");
    }

    for (auto _ : state) {
        auto metrics = loader->getMetrics();
        benchmark::DoNotOptimize(metrics.totalLoadAttempts);
    }
    state.SetLabel("getMetrics() after 10 failed load attempts");
}

// ─── 6. getAllLoadedModules() size-growth overhead ───────────────────────────

static void BM_GetAllLoadedModules_Growth(benchmark::State& state) {
    const int kEvents = static_cast<int>(state.range(0));

    ModuleLoader loader;
    loader.setAllowUnsigned(true);

    // Trigger load attempts (all fail fast — no real .so)
    for (int i = 0; i < kEvents; ++i) {
        loader.loadModule("/tmp/bench_growth_" + std::to_string(i) + ".so", "bench_growth");
    }

    for (auto _ : state) {
        auto modules = loader.getAllLoadedModules();
        benchmark::DoNotOptimize(modules.size());
    }

    state.SetLabel("getAllLoadedModules() after " + std::to_string(kEvents) + " load attempts");
}
BENCHMARK(BM_GetAllLoadedModules_Growth)->Arg(0)->Arg(10)->Arg(100)->Arg(1000);

// ─── HotReloadManager fixtures ────────────────────────────────────────────────

class HotReloadLoadFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State& /*s*/) override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = false;
        cfg.enableRollback  = false;
        mgr    = std::make_unique<HotReloadManager>(cfg);
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

// ─── 7. Module registration throughput ───────────────────────────────────────

BENCHMARK_F(HotReloadLoadFixture, Registration_Throughput)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        std::string name = "bench_reg_" + std::to_string(n++);
        mgr->registerModule(name, *loader);
        benchmark::DoNotOptimize(name);
    }
    state.SetLabel("registerModule() throughput");
}

// ─── 8. Register + reload + unregister cycle ─────────────────────────────────

BENCHMARK_F(HotReloadLoadFixture, RegisterReloadUnregister_Cycle)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        std::string name = "bench_cycle_" + std::to_string(n++);
        mgr->registerModule(name, *loader);
        auto result = mgr->reloadModule(name, "/nonexistent.so");
        benchmark::DoNotOptimize(result.success);
        mgr->unregisterModule(name);
    }
    state.SetLabel("register → reload(fast-fail) → unregister cycle");
}

// ─── 9. Concurrent reload scalability ────────────────────────────────────────

BENCHMARK_F(HotReloadLoadFixture, ConcurrentReload_Scalability)(benchmark::State& state) {
    const int kThreads = static_cast<int>(state.range(0));

    // Pre-register modules
    for (int t = 0; t < kThreads; ++t) {
        mgr->registerModule("concurrent_" + std::to_string(t), *loader);
    }

    for (auto _ : state) {
        std::atomic<uint64_t> attempts{0};
        std::vector<std::thread> threads;
        threads.reserve(static_cast<size_t>(kThreads));

        for (int t = 0; t < kThreads; ++t) {
            threads.emplace_back([this, t, &attempts]() {
                const std::string name = "concurrent_" + std::to_string(t);
                for (int r = 0; r < 4; ++r) {
                    auto res = mgr->reloadModule(name, "/nonexistent.so");
                    attempts.fetch_add(1, std::memory_order_relaxed);
                    (void)res;
                }
            });
        }
        for (auto& th : threads) th.join();
        benchmark::DoNotOptimize(attempts.load());
    }

    state.SetItemsProcessed(state.iterations() * kThreads * 4LL);
    state.SetLabel("concurrent reload; threads=" + std::to_string(kThreads));
}
BENCHMARK_REGISTER_F(HotReloadLoadFixture, ConcurrentReload_Scalability)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
    ->UseRealTime();

// ─── 10. Hot-reload stats latency ────────────────────────────────────────────

BENCHMARK_F(HotReloadLoadFixture, StatsLatency)(benchmark::State& state) {
    mgr->registerModule("stats_target", *loader);
    for (int i = 0; i < 20; ++i) {
        mgr->reloadModule("stats_target", "/nonexistent.so");
    }

    for (auto _ : state) {
        auto s = mgr->getStats();
        benchmark::DoNotOptimize(s);
    }
    state.SetLabel("getStats() after 20 reload attempts");
}
