/**
 * @file bench_plugin_system.cpp
 * @brief Performance benchmarks for Plugin System
 * 
 * Benchmarks plugin loading, unloading, querying, hot-reload,
 * and concurrent access patterns.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <benchmark/benchmark.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <vector>
#include <random>

using namespace themis::plugins;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// Test Setup Utilities
// ============================================================================

class PluginBenchmarkFixture {
public:
    static std::string test_dir;
    static std::once_flag setup_flag;
    
    static void setupOnce() {
        std::call_once(setup_flag, []() {
            test_dir = "/tmp/themis_plugin_bench";
            fs::create_directories(test_dir);
            
            // Create mock plugin manifests
            for (int i = 0; i < 100; i++) {
                createManifest("plugin" + std::to_string(i), 
                              static_cast<PluginType>(i % 4));
            }
        });
    }
    
    static void createManifest(const std::string& name, PluginType type) {
        json manifest;
        manifest["name"] = name;
        manifest["version"] = "1.0.0";
        manifest["type"] = static_cast<int>(type);
        manifest["author"] = "Benchmark";
        manifest["description"] = "Benchmark plugin";
        manifest["library"] = name + ".so";
        
        std::string path = test_dir + "/" + name + ".json";
        std::ofstream file(path);
        file << manifest.dump(2);
    }
};

std::string PluginBenchmarkFixture::test_dir;
std::once_flag PluginBenchmarkFixture::setup_flag;

// ============================================================================
// Directory Scanning Benchmarks
// ============================================================================

static void BM_ScanEmptyDirectory(benchmark::State& state) {
    PluginManager manager;
    std::string empty_dir = "/tmp/themis_empty_bench";
    fs::create_directories(empty_dir);
    
    for (auto _ : state) {
        auto count_result = manager.scanPluginDirectory(empty_dir);
        size_t count = count_result.has_value() ? *count_result : 0;
        benchmark::DoNotOptimize(count);
    }
    
    fs::remove_all(empty_dir);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ScanEmptyDirectory);

static void BM_ScanDirectoryWithPlugins(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager manager;
        state.ResumeTiming();
        
        auto count_result = manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
        size_t count = count_result.has_value() ? *count_result : 0;
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ScanDirectoryWithPlugins);

static void BM_ReScanDirectory(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto count_result = manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
        size_t count = count_result.has_value() ? *count_result : 0;
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ReScanDirectory);

// ============================================================================
// Plugin Query Benchmarks
// ============================================================================

static void BM_IsPluginLoaded(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        bool loaded = manager.isPluginLoaded("plugin0");
        benchmark::DoNotOptimize(loaded);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_IsPluginLoaded);

static void BM_GetPluginInfo(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto info = manager.getManifest("plugin0");
        benchmark::DoNotOptimize(info);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetPluginInfo);

static void BM_GetAllPlugins(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto plugins = manager.listPlugins();
        benchmark::DoNotOptimize(plugins);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetAllPlugins);

static void BM_GetPluginsByType(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    PluginType type = PluginType::COMPUTE_BACKEND;
    for (auto _ : state) {
        auto plugins = manager.getPluginsByType(type);
        benchmark::DoNotOptimize(plugins);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetPluginsByType);

static void BM_GetLoadedPlugins(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto plugins = manager.listLoadedPlugins();
        benchmark::DoNotOptimize(plugins);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetLoadedPlugins);

// ============================================================================
// Plugin Loading Benchmarks
// ============================================================================

static void BM_LoadNonexistentPlugin(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto plugin = manager.loadPlugin("plugin0");
        benchmark::DoNotOptimize(plugin);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LoadNonexistentPlugin);

// ============================================================================
// Manifest Parsing Benchmarks
// ============================================================================

static void BM_ManifestParsing(benchmark::State& state) {
    const int num_manifests = state.range(0);
    std::string bench_dir = "/tmp/themis_manifest_bench";
    fs::create_directories(bench_dir);
    
    for (int i = 0; i < num_manifests; i++) {
        PluginBenchmarkFixture::createManifest(
            "manifest_plugin" + std::to_string(i),
            PluginType::COMPUTE_BACKEND
        );
    }
    
    for (auto _ : state) {
        PluginManager manager;
        auto count_result = manager.scanPluginDirectory(bench_dir);
        size_t count = count_result.has_value() ? *count_result : 0;
        benchmark::DoNotOptimize(count);
    }
    
    fs::remove_all(bench_dir);
    state.SetItemsProcessed(state.iterations() * num_manifests);
}
BENCHMARK(BM_ManifestParsing)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Plugin Enable/Disable Benchmarks
// ============================================================================

static void BM_LoadUnloadPlugin(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto load_result = manager.loadPlugin("plugin0");
        auto unload_result = manager.unloadPlugin("plugin0");
        benchmark::DoNotOptimize(load_result);
        benchmark::DoNotOptimize(unload_result);
    }
    
    state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(BM_LoadUnloadPlugin);

// ============================================================================
// Hot Reload Benchmarks
// ============================================================================

static void BM_ReloadPlugin(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto result = manager.reloadPlugin("plugin0");
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ReloadPlugin);

// ============================================================================
// Statistics Benchmarks
// ============================================================================

static void BM_GetMetricsSnapshot(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    for (auto _ : state) {
        auto stats = manager.getMetrics().getAllStats();
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_GetMetricsSnapshot);

// ============================================================================
// Concurrent Access Benchmarks
// ============================================================================

static void BM_ConcurrentQueries(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    static PluginManager manager;
    
    if (state.thread_index() == 0) {
        (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    
    for (auto _ : state) {
        int idx = dis(gen);
        std::string plugin_name = "plugin" + std::to_string(idx);
        
        auto info = manager.getManifest(plugin_name);
        benchmark::DoNotOptimize(info);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentQueries)->ThreadRange(1, 16);

static void BM_ConcurrentScans(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    static PluginManager manager;
    
    for (auto _ : state) {
        auto count_result = manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
        size_t count = count_result.has_value() ? *count_result : 0;
        benchmark::DoNotOptimize(count);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentScans)->ThreadRange(1, 8);

static void BM_ConcurrentGetAllPlugins(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    static PluginManager manager;
    
    if (state.thread_index() == 0) {
        (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    }
    
    for (auto _ : state) {
        auto plugins = manager.listPlugins();
        benchmark::DoNotOptimize(plugins);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ConcurrentGetAllPlugins)->ThreadRange(1, 16);

// ============================================================================
// Memory Overhead Benchmarks
// ============================================================================

static void BM_MemoryOverhead(benchmark::State& state) {
    const int num_plugins = state.range(0);
    std::string memory_dir = "/tmp/themis_memory_bench";
    fs::create_directories(memory_dir);
    
    for (int i = 0; i < num_plugins; i++) {
        PluginBenchmarkFixture::createManifest(
            "mem_plugin" + std::to_string(i),
            PluginType::COMPUTE_BACKEND
        );
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager manager;
        state.ResumeTiming();
        
        manager.scanPluginDirectory(memory_dir);
        
        state.PauseTiming();
        // Manager destructor called here
        state.ResumeTiming();
    }
    
    fs::remove_all(memory_dir);
    state.SetItemsProcessed(state.iterations() * num_plugins);
}
BENCHMARK(BM_MemoryOverhead)->Arg(10)->Arg(100)->Arg(1000);

// ============================================================================
// Filter and Search Benchmarks
// ============================================================================

static void BM_FilterPluginsByType(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    std::vector<PluginType> types = {
        PluginType::COMPUTE_BACKEND,
        PluginType::BLOB_STORAGE,
        PluginType::EXPORTER,
        PluginType::CUSTOM
    };
    
    int idx = 0;
    for (auto _ : state) {
        auto plugins = manager.getPluginsByType(types[idx % types.size()]);
        benchmark::DoNotOptimize(plugins);
        idx++;
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_FilterPluginsByType);

// ============================================================================
// Batch Operations Benchmarks
// ============================================================================

static void BM_BatchPluginQueries(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    PluginManager manager;
    (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
    
    const int batch_size = state.range(0);
    
    for (auto _ : state) {
        for (int i = 0; i < batch_size; i++) {
            std::string plugin_name = "plugin" + std::to_string(i % 100);
            auto info = manager.getManifest(plugin_name);
            benchmark::DoNotOptimize(info);
        }
    }
    
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_BatchPluginQueries)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Cleanup Benchmarks
// ============================================================================

static void BM_ManagerDestruction(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    
    const int num_plugins = state.range(0);
    std::string cleanup_dir = "/tmp/themis_cleanup_bench";
    fs::create_directories(cleanup_dir);
    
    for (int i = 0; i < num_plugins; i++) {
        PluginBenchmarkFixture::createManifest(
            "cleanup_plugin" + std::to_string(i),
            PluginType::COMPUTE_BACKEND
        );
    }
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager* manager = new PluginManager();
        (void)manager->scanPluginDirectory(cleanup_dir);
        state.ResumeTiming();
        
        delete manager;
    }
    
    fs::remove_all(cleanup_dir);
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_ManagerDestruction)->Arg(10)->Arg(50)->Arg(100);

// ============================================================================
// Real-World Scenario Benchmarks
// ============================================================================

static void BM_TypicalWorkflow(benchmark::State& state) {
    PluginBenchmarkFixture::setupOnce();
    
    for (auto _ : state) {
        state.PauseTiming();
        PluginManager manager;
        state.ResumeTiming();
        
        // Scan directory
        (void)manager.scanPluginDirectory(PluginBenchmarkFixture::test_dir);
        
        // Query some plugins
        auto compute_plugins = manager.getPluginsByType(PluginType::COMPUTE_BACKEND);
        auto all_plugins = manager.listPlugins();
        
        // Get specific plugin info
        auto info = manager.getManifest("plugin0");
        
        // Check if loaded
        bool loaded = manager.isPluginLoaded("plugin0");
        
        // Get statistics
        auto stats = manager.getMetrics().getAllStats();
        
        benchmark::DoNotOptimize(compute_plugins);
        benchmark::DoNotOptimize(all_plugins);
        benchmark::DoNotOptimize(info);
        benchmark::DoNotOptimize(loaded);
        benchmark::DoNotOptimize(stats);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_TypicalWorkflow);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
