/**
 * @file bench_plugin_hot_plug.cpp
 * @brief Performance benchmarks for Plugin Hot-Plug Monitoring
 * 
 * Benchmarks filesystem monitoring overhead, event handling latency,
 * and concurrent plugin operations under hot-plug monitoring.
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include <benchmark/benchmark.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_hot_plug_monitor.h"
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

using namespace themis::plugins;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ============================================================================
// Benchmark Fixtures
// ============================================================================

class HotPlugBenchmarkFixture : public benchmark::Fixture {
public:
    std::string test_dir;
    PluginManager* manager;
    
    void SetUp(const benchmark::State& state) override {
        test_dir = "/tmp/themis_hotplug_bench_" + std::to_string(state.thread_index());
        fs::create_directories(test_dir);
        manager = &PluginManager::instance();
        manager->disableHotPlug();
    }
    
    void TearDown(const benchmark::State& state) override {
        manager->disableHotPlug();
        manager->unloadAllPlugins();
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }
    
    void createTestManifest(const std::string& name) {
        std::string plugin_dir = test_dir + "/" + name;
        fs::create_directories(plugin_dir);
        
        json manifest = {
            {"name", name},
            {"version", "1.0.0"},
            {"type", "custom"},
            {"description", "Benchmark plugin"},
            {"binary", {
                {"windows", name + ".dll"},
                {"linux", name + ".so"},
                {"macos", name + ".dylib"}
            }},
            {"capabilities", {
                {"thread_safe", true},
                {"streaming", false}
            }},
            {"auto_load", false},
            {"load_priority", 100}
        };
        
        std::string manifest_path = plugin_dir + "/plugin.json";
        std::ofstream file(manifest_path);
        file << manifest.dump(2);
    }
};

// ============================================================================
// Hot-Plug Monitor Lifecycle Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, EnableDisableMonitoring)(benchmark::State& state) {
    for (auto _ : state) {
        HotPlugConfig config;
        config.enabled = true;
        config.auto_load = false;
        
        auto start = std::chrono::high_resolution_clock::now();
        bool enabled = manager->enableHotPlug(test_dir, config);
        benchmark::DoNotOptimize(enabled);
        manager->disableHotPlug();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e6);
    }
    state.SetLabel("Monitor lifecycle overhead");
}

BENCHMARK_F(HotPlugBenchmarkFixture, MonitoringOverhead)(benchmark::State& state) {
    HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    config.auto_reload = false;
    config.auto_unload = false;
    
    manager->enableHotPlug(test_dir, config);
    
    // Measure steady-state overhead while monitoring is active
    for (auto _ : state) {
        // Simulate some work while monitoring is active
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        benchmark::DoNotOptimize(manager->isHotPlugEnabled());
    }
    
    manager->disableHotPlug();
    state.SetLabel("Background monitoring overhead");
}

// ============================================================================
// File Detection Latency Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, FileCreationDetectionLatency)(benchmark::State& state) {
    HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    int iteration = 0;
    for (auto _ : state) {
        manager->enableHotPlug(test_dir, config);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create a plugin manifest
        std::string plugin_name = "latency_test_" + std::to_string(iteration++);
        createTestManifest(plugin_name);
        
        // Wait for detection (monitor debounce is 500ms)
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        manager->disableHotPlug();
        
        // Clean up
        fs::remove_all(test_dir + "/" + plugin_name);
        
        state.SetIterationTime(elapsed.count() / 1000.0);
    }
    state.SetLabel("File creation detection + debounce");
}

BENCHMARK_F(HotPlugBenchmarkFixture, MultipleFileCreations)(benchmark::State& state) {
    const int num_files = state.range(0);
    
    for (auto _ : state) {
        HotPlugConfig config;
        config.enabled = true;
        config.auto_load = false;
        
        manager->enableHotPlug(test_dir, config);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create multiple plugin manifests rapidly
        for (int i = 0; i < num_files; i++) {
            createTestManifest("multi_test_" + std::to_string(i));
        }
        
        // Wait for all detections
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        manager->disableHotPlug();
        
        // Clean up
        for (int i = 0; i < num_files; i++) {
            fs::remove_all(test_dir + "/multi_test_" + std::to_string(i));
        }
        
        state.SetIterationTime(elapsed.count() / 1000.0);
    }
    state.SetItemsProcessed(state.iterations() * num_files);
    state.SetLabel("Batch file creation");
}
BENCHMARK_REGISTER_F(HotPlugBenchmarkFixture, MultipleFileCreations)
    ->Arg(1)->Arg(5)->Arg(10)->Arg(50);

// ============================================================================
// Configuration Impact Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, AutoLoadDisabledVsEnabled)(benchmark::State& state) {
    const bool auto_load = state.range(0);
    
    int iteration = 0;
    for (auto _ : state) {
        HotPlugConfig config;
        config.enabled = true;
        config.auto_load = auto_load;
        
        manager->enableHotPlug(test_dir, config);
        
        std::string plugin_name = "config_test_" + std::to_string(iteration++);
        createTestManifest(plugin_name);
        
        // Wait for processing
        std::this_thread::sleep_for(std::chrono::milliseconds(600));
        
        manager->disableHotPlug();
        
        // Clean up
        fs::remove_all(test_dir + "/" + plugin_name);
    }
    
    state.SetLabel(auto_load ? "auto_load=ON" : "auto_load=OFF");
}
BENCHMARK_REGISTER_F(HotPlugBenchmarkFixture, AutoLoadDisabledVsEnabled)
    ->Arg(0)->Arg(1);

// ============================================================================
// Thread Safety Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, ConcurrentFileCreations)(benchmark::State& state) {
    HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    manager->enableHotPlug(test_dir, config);
    
    for (auto _ : state) {
        state.PauseTiming();
        std::vector<std::thread> threads;
        const int num_threads = 4;
        
        state.ResumeTiming();
        
        // Create files from multiple threads
        for (int t = 0; t < num_threads; t++) {
            threads.emplace_back([this, t]() {
                createTestManifest("concurrent_" + std::to_string(t));
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Wait for all detections
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
        
        state.PauseTiming();
        
        // Clean up
        for (int t = 0; t < num_threads; t++) {
            fs::remove_all(test_dir + "/concurrent_" + std::to_string(t));
        }
        
        state.ResumeTiming();
    }
    
    manager->disableHotPlug();
    state.SetLabel("4 threads creating files");
}

// ============================================================================
// Memory Usage Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, MonitorMemoryFootprint)(benchmark::State& state) {
    const int num_watches = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create multiple watch directories
        std::vector<std::string> watch_dirs;
        for (int i = 0; i < num_watches; i++) {
            std::string dir = test_dir + "_watch_" + std::to_string(i);
            fs::create_directories(dir);
            watch_dirs.push_back(dir);
        }
        
        state.ResumeTiming();
        
        // Enable monitoring on first directory
        HotPlugConfig config;
        config.enabled = true;
        manager->enableHotPlug(watch_dirs[0], config);
        
        // Keep monitoring active briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        manager->disableHotPlug();
        
        state.PauseTiming();
        
        // Clean up
        for (const auto& dir : watch_dirs) {
            fs::remove_all(dir);
        }
        
        state.ResumeTiming();
    }
    
    state.SetLabel("Watch dirs: " + std::to_string(num_watches));
}
BENCHMARK_REGISTER_F(HotPlugBenchmarkFixture, MonitorMemoryFootprint)
    ->Arg(1)->Arg(10)->Arg(100);

// ============================================================================
// Stress Test Benchmarks
// ============================================================================

BENCHMARK_F(HotPlugBenchmarkFixture, RapidEnableDisable)(benchmark::State& state) {
    HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    for (auto _ : state) {
        manager->enableHotPlug(test_dir, config);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        manager->disableHotPlug();
    }
    
    state.SetLabel("Enable/disable churn");
}

BENCHMARK_F(HotPlugBenchmarkFixture, MixedOperations)(benchmark::State& state) {
    HotPlugConfig config;
    config.enabled = true;
    config.auto_load = false;
    
    int iteration = 0;
    
    for (auto _ : state) {
        manager->enableHotPlug(test_dir, config);
        
        // Create a file
        std::string plugin_name = "mixed_" + std::to_string(iteration++);
        createTestManifest(plugin_name);
        
        // Query state
        benchmark::DoNotOptimize(manager->isHotPlugEnabled());
        
        // Wait briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Disable
        manager->disableHotPlug();
        
        // Clean up
        fs::remove_all(test_dir + "/" + plugin_name);
    }
    
    state.SetLabel("Mixed enable/create/query/disable");
}

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
