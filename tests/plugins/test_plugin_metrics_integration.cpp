#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "plugins/plugin_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::plugins;

// Test plugin implementation for integration testing
class IntegrationTestPlugin : public IThemisPlugin {
private:
    bool initialized_ = false;
    
public:
    const char* getName() const override { return "integration_test_plugin"; }
    const char* getVersion() const override { return "1.0.0"; }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.thread_safe = true;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        // Simulate initialization time
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    void* getInstance() override {
        return this;
    }
};

class PluginMetricsIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Register test plugin factory
        PluginManagerRegistry::registerFactory(
            "integration_test_plugin",
            PluginType::CUSTOM,
            []() { return std::make_unique<IntegrationTestPlugin>(); }
        );
    }
    
    void TearDown() override {
        // Cleanup is handled by PluginManager destructor
    }
};

TEST_F(PluginMetricsIntegrationTest, MetricsRecordedOnLoad) {
    auto& pm = PluginManager::instance();
    
    // Create plugin via registry (simulates loading)
    auto plugin = PluginManagerRegistry::createPlugin("integration_test_plugin");
    ASSERT_NE(plugin, nullptr);
    
    // Initialize plugin
    EXPECT_TRUE(plugin->initialize("{}"));
    
    // Manually record load metrics (in real scenario, PluginManager does this)
    auto& metrics = pm.getMetricsMutable();
    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::milliseconds(50);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    metrics.recordLoad("integration_test_plugin", duration);
    
    // Verify metrics
    auto stats = pm.getMetrics().getStats("integration_test_plugin");
    EXPECT_EQ(stats.load_time, duration);
    EXPECT_GT(std::chrono::system_clock::now() - stats.loaded_at, std::chrono::milliseconds(0));
}

TEST_F(PluginMetricsIntegrationTest, FunctionCallMetrics) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    // Simulate multiple function calls with varying latencies
    for (int i = 0; i < 10; i++) {
        metrics.recordCall("integration_test_plugin", std::chrono::microseconds(1000 * (i + 1)));
    }
    
    auto stats = pm.getMetrics().getStats("integration_test_plugin");
    EXPECT_EQ(stats.function_calls, 10);
    EXPECT_GT(stats.avg_call_latency_ms, 0.0);
    EXPECT_GT(stats.p95_call_latency_ms, stats.avg_call_latency_ms);
    EXPECT_GE(stats.p99_call_latency_ms, stats.p95_call_latency_ms);
}

TEST_F(PluginMetricsIntegrationTest, ErrorTracking) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    // Record some errors
    for (int i = 0; i < 5; i++) {
        metrics.recordError("integration_test_plugin");
    }
    
    auto stats = pm.getMetrics().getStats("integration_test_plugin");
    EXPECT_EQ(stats.errors, 5);
}

TEST_F(PluginMetricsIntegrationTest, MemoryTracking) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    // Update memory usage
    size_t memory_usage = 1024 * 1024 * 10; // 10MB
    metrics.updateMemoryUsage("integration_test_plugin", memory_usage);
    
    auto stats = pm.getMetrics().getStats("integration_test_plugin");
    EXPECT_EQ(stats.memory_bytes, memory_usage);
}

TEST_F(PluginMetricsIntegrationTest, ReloadMetrics) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    // Record initial load
    metrics.recordLoad("integration_test_plugin", std::chrono::milliseconds(100));
    
    // Record reload
    metrics.recordReload("integration_test_plugin", std::chrono::milliseconds(80));
    
    auto stats = pm.getMetrics().getStats("integration_test_plugin");
    EXPECT_EQ(stats.reload_count, 1);
    EXPECT_EQ(stats.last_reload_time, std::chrono::milliseconds(80));
}

TEST_F(PluginMetricsIntegrationTest, GetAllStats) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    // Add metrics for multiple plugins
    metrics.recordLoad("plugin1", std::chrono::milliseconds(100));
    metrics.recordLoad("plugin2", std::chrono::milliseconds(200));
    metrics.recordLoad("integration_test_plugin", std::chrono::milliseconds(150));
    
    auto all_stats = pm.getMetrics().getAllStats();
    EXPECT_GE(all_stats.size(), 3);
    EXPECT_TRUE(all_stats.find("plugin1") != all_stats.end());
    EXPECT_TRUE(all_stats.find("plugin2") != all_stats.end());
    EXPECT_TRUE(all_stats.find("integration_test_plugin") != all_stats.end());
}

TEST_F(PluginMetricsIntegrationTest, CompleteLifecycle) {
    auto& pm = PluginManager::instance();
    auto& metrics = pm.getMetricsMutable();
    
    const std::string plugin_name = "lifecycle_plugin";
    
    // Simulate complete plugin lifecycle
    // 1. Load
    metrics.recordLoad(plugin_name, std::chrono::milliseconds(200));
    
    // 2. Multiple calls
    for (int i = 0; i < 100; i++) {
        auto latency = std::chrono::microseconds(5000 + (i * 100)); // 5-15ms
        metrics.recordCall(plugin_name, latency);
    }
    
    // 3. Update memory periodically
    metrics.updateMemoryUsage(plugin_name, 1024 * 1024 * 5);
    
    // 4. Some errors
    for (int i = 0; i < 3; i++) {
        metrics.recordError(plugin_name);
    }
    
    // 5. Reload
    metrics.recordReload(plugin_name, std::chrono::milliseconds(180));
    
    // Verify all metrics
    auto stats = pm.getMetrics().getStats(plugin_name);
    
    EXPECT_EQ(stats.load_time, std::chrono::milliseconds(200));
    EXPECT_EQ(stats.last_reload_time, std::chrono::milliseconds(180));
    EXPECT_EQ(stats.reload_count, 1);
    EXPECT_EQ(stats.function_calls, 100);
    EXPECT_EQ(stats.errors, 3);
    EXPECT_EQ(stats.memory_bytes, 1024 * 1024 * 5);
    EXPECT_GT(stats.avg_call_latency_ms, 0.0);
    EXPECT_GT(stats.p95_call_latency_ms, 0.0);
    EXPECT_GT(stats.p99_call_latency_ms, 0.0);
}


