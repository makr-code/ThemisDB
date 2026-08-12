/// @file bench_hot_reload_manager.cpp
/// @brief Performance benchmarks for HotReloadManager
///
/// Measures the overhead of the HotReloadManager operations:
/// - Module registration / unregistration
/// - Hot-reload attempt throughput (failure path, no I/O)
/// - Callback dispatch cost
/// - State-save/restore callback overhead
/// - Concurrent reload contention

#include <benchmark/benchmark.h>
#include "themis/base/hot_reload_manager.h"
#include "themis/base/module_loader.h"
#include <atomic>
#include <string>
#include <thread>

using namespace themis::modules;

// =============================================================================
// Benchmark fixture
// =============================================================================

class HotReloadBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const benchmark::State&) override {
        HotReloadManager::Config cfg;
        cfg.verifySignature = false;
        cfg.preserveState   = false;
        cfg.enableRollback  = true;
        mgr    = std::make_unique<HotReloadManager>(cfg);
        loader = std::make_unique<ModuleLoader>();
        loader->setAllowUnsigned(true);
    }

    void TearDown(const benchmark::State&) override {
        mgr.reset();
        loader.reset();
    }

    std::unique_ptr<HotReloadManager> mgr;
    std::unique_ptr<ModuleLoader>     loader;
};

// =============================================================================
// Registration overhead
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, RegisterUnregister)(benchmark::State& state) {
    int n = 0;
    for (auto _ : state) {
        std::string name = "bench_mod_" + std::to_string(n++);
        mgr->registerModule(name, *loader);
        benchmark::DoNotOptimize(name);
        mgr->unregisterModule(name);
    }
    state.SetLabel("Register + Unregister single module");
}

BENCHMARK_F(HotReloadBenchFixture, RegisteredModulesList)(benchmark::State& state) {
    // Pre-populate
    const int N = 100;
    for (int i = 0; i < N; ++i) {
        mgr->registerModule("mod_" + std::to_string(i), *loader);
    }

    for (auto _ : state) {
        auto names = mgr->registeredModules();
        benchmark::DoNotOptimize(names);
    }

    state.SetItemsProcessed(state.iterations() * N);
    state.SetLabel("List 100 registered modules");
}

// =============================================================================
// Reload attempt throughput (fast-fail, no disk I/O)
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, ReloadAttemptFastFail)(benchmark::State& state) {
    mgr->registerModule("bench_target", *loader);

    for (auto _ : state) {
        auto result = mgr->reloadModule("bench_target", "/nonexistent.so");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("Reload attempt on non-existent binary (fast-fail path)");
}

BENCHMARK_F(HotReloadBenchFixture, RollbackWithNoBackup)(benchmark::State& state) {
    mgr->registerModule("bench_rollback", *loader);

    for (auto _ : state) {
        auto result = mgr->rollback("bench_rollback");
        benchmark::DoNotOptimize(result.success);
    }

    state.SetLabel("Rollback attempt with no backup (early-return path)");
}

// =============================================================================
// Callback dispatch cost
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, ZeroCallbacksDispatch)(benchmark::State& state) {
    mgr->registerModule("bench_cb0", *loader);
    // No callbacks registered.
    for (auto _ : state) {
        auto result = mgr->reloadModule("bench_cb0", "/nonexistent.so");
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("Reload with 0 callbacks");
}

BENCHMARK_F(HotReloadBenchFixture, OneCallbackDispatch)(benchmark::State& state) {
    mgr->registerModule("bench_cb1", *loader);
    mgr->addReloadCallback([](const std::string&, HotReloadManager::ReloadPhase) {});

    for (auto _ : state) {
        auto result = mgr->reloadModule("bench_cb1", "/nonexistent.so");
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("Reload with 1 no-op callback");
}

BENCHMARK_F(HotReloadBenchFixture, TenCallbacksDispatch)(benchmark::State& state) {
    mgr->registerModule("bench_cb10", *loader);
    for (int i = 0; i < 10; ++i) {
        mgr->addReloadCallback([](const std::string&, HotReloadManager::ReloadPhase) {});
    }

    for (auto _ : state) {
        auto result = mgr->reloadModule("bench_cb10", "/nonexistent.so");
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("Reload with 10 no-op callbacks");
}

// =============================================================================
// State-save overhead
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, ReloadWithStateSaveCallback)(benchmark::State& state) {
    HotReloadManager::Config cfg;
    cfg.verifySignature = false;
    cfg.preserveState   = true;   // enable state preservation
    cfg.enableRollback  = false;
    auto mgr2 = HotReloadManager(cfg);
    ModuleLoader loader2;
    loader2.setAllowUnsigned(true);

    mgr2.registerModule("bench_state", loader2);
    mgr2.setStateSaveCallback([](const std::string&) -> std::string {
        // Simulate lightweight state serialisation.
        return R"({"counter":42,"data":"bench"})";
    });

    for (auto _ : state) {
        auto result = mgr2.reloadModule("bench_state", "/nonexistent.so");
        benchmark::DoNotOptimize(result);
    }
    state.SetLabel("Reload with state-save callback (fails before reload)");
}

// =============================================================================
// Version query
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, GetCurrentVersionMiss)(benchmark::State& state) {
    // Module registered but not loaded.
    mgr->registerModule("bench_ver", *loader);

    for (auto _ : state) {
        auto ver = mgr->getCurrentVersion("bench_ver");
        benchmark::DoNotOptimize(ver);
    }
    state.SetLabel("getCurrentVersion (unloaded module, nullopt)");
}

BENCHMARK_F(HotReloadBenchFixture, GetCurrentVersionNotRegistered)(benchmark::State& state) {
    for (auto _ : state) {
        auto ver = mgr->getCurrentVersion("nonexistent");
        benchmark::DoNotOptimize(ver);
    }
    state.SetLabel("getCurrentVersion (unregistered module)");
}

// =============================================================================
// Stats read overhead
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, GetStats)(benchmark::State& state) {
    for (auto _ : state) {
        auto s = mgr->getStats();
        benchmark::DoNotOptimize(s);
    }
    state.SetLabel("getStats() under no contention");
}

// =============================================================================
// Concurrency: parallel reload attempts on the same module
// =============================================================================

BENCHMARK_F(HotReloadBenchFixture, ConcurrentReloads)(benchmark::State& state) {
    mgr->registerModule("concurrent_bench", *loader);

    const int threads_count = static_cast<int>(state.range(0));
    std::atomic<bool> start_flag{false};

    for (auto _ : state) {
        state.PauseTiming();
        mgr->resetStats();
        std::vector<std::thread> threads;
        threads.reserve(threads_count);
        std::atomic<int> ready{0};

        for (int t = 0; t < threads_count; ++t) {
            threads.emplace_back([this, &start_flag, &ready]() {
                ++ready;
                while (!start_flag.load(std::memory_order_acquire)) {}
                mgr->reloadModule("concurrent_bench", "/nonexistent.so");
            });
        }
        // Wait for all threads to be ready.
        while (ready.load() < threads_count) {}
        state.ResumeTiming();

        start_flag.store(true, std::memory_order_release);
        for (auto& th : threads) th.join();
        start_flag.store(false, std::memory_order_release);
    }

    state.SetLabel("Concurrent reload attempts (threads = " +
                   std::to_string(threads_count) + ")");
}
BENCHMARK_REGISTER_F(HotReloadBenchFixture, ConcurrentReloads)
    ->Arg(1)->Arg(2)->Arg(4)->Arg(8);

BENCHMARK_MAIN();
