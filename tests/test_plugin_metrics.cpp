#include <gtest/gtest.h>
#include "plugins/plugin_metrics.h"
#include <thread>
#include <chrono>

using namespace themis::plugins;

class PluginMetricsTest : public ::testing::Test {
protected:
    PluginMetrics metrics;
    
    void SetUp() override {
        metrics.resetAll();
    }
};

TEST_F(PluginMetricsTest, RecordLoad) {
    auto duration = std::chrono::milliseconds(450);
    metrics.recordLoad("test_plugin", duration);
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.load_time, duration);
    EXPECT_GT(std::chrono::system_clock::now() - stats.loaded_at, std::chrono::milliseconds(0));
}

TEST_F(PluginMetricsTest, RecordReload) {
    metrics.recordLoad("test_plugin", std::chrono::milliseconds(100));
    
    auto duration = std::chrono::milliseconds(200);
    metrics.recordReload("test_plugin", duration);
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.reload_count, 1);
    EXPECT_EQ(stats.last_reload_time, duration);
}

TEST_F(PluginMetricsTest, RecordMultipleReloads) {
    metrics.recordLoad("test_plugin", std::chrono::milliseconds(100));
    
    metrics.recordReload("test_plugin", std::chrono::milliseconds(150));
    metrics.recordReload("test_plugin", std::chrono::milliseconds(180));
    metrics.recordReload("test_plugin", std::chrono::milliseconds(120));
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.reload_count, 3);
    EXPECT_EQ(stats.last_reload_time, std::chrono::milliseconds(120));
}

TEST_F(PluginMetricsTest, RecordCall) {
    metrics.recordCall("test_plugin", std::chrono::microseconds(12500)); // 12.5ms
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.function_calls, 1);
    EXPECT_NEAR(stats.avg_call_latency_ms, 12.5, 0.1);
}

TEST_F(PluginMetricsTest, RecordMultipleCalls) {
    // Record multiple calls with different latencies
    metrics.recordCall("test_plugin", std::chrono::microseconds(10000));  // 10ms
    metrics.recordCall("test_plugin", std::chrono::microseconds(20000));  // 20ms
    metrics.recordCall("test_plugin", std::chrono::microseconds(30000));  // 30ms
    metrics.recordCall("test_plugin", std::chrono::microseconds(15000));  // 15ms
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.function_calls, 4);
    
    // Average should be (10 + 20 + 30 + 15) / 4 = 18.75
    EXPECT_NEAR(stats.avg_call_latency_ms, 18.75, 0.1);
}

TEST_F(PluginMetricsTest, PercentileCalculation) {
    // Add 100 samples with known distribution
    for (int i = 1; i <= 100; i++) {
        metrics.recordCall("test_plugin", std::chrono::microseconds(i * 1000)); // 1ms to 100ms
    }
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.function_calls, 100);
    
    // P95 should be around 95ms
    EXPECT_NEAR(stats.p95_call_latency_ms, 95.0, 2.0);
    
    // P99 should be around 99ms
    EXPECT_NEAR(stats.p99_call_latency_ms, 99.0, 2.0);
    
    // Average should be around 50.5ms
    EXPECT_NEAR(stats.avg_call_latency_ms, 50.5, 1.0);
}

TEST_F(PluginMetricsTest, RecordError) {
    metrics.recordError("test_plugin");
    metrics.recordError("test_plugin");
    metrics.recordError("test_plugin");
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.errors, 3);
}

TEST_F(PluginMetricsTest, UpdateMemoryUsage) {
    metrics.updateMemoryUsage("test_plugin", 1024 * 1024); // 1MB
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.memory_bytes, 1024 * 1024);
    
    // Update to different value
    metrics.updateMemoryUsage("test_plugin", 2 * 1024 * 1024); // 2MB
    
    stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.memory_bytes, 2 * 1024 * 1024);
}

TEST_F(PluginMetricsTest, MultiplePlugins) {
    metrics.recordLoad("plugin1", std::chrono::milliseconds(100));
    metrics.recordLoad("plugin2", std::chrono::milliseconds(200));
    metrics.recordLoad("plugin3", std::chrono::milliseconds(300));
    
    auto all_stats = metrics.getAllStats();
    EXPECT_EQ(all_stats.size(), 3);
    
    EXPECT_EQ(all_stats["plugin1"].load_time, std::chrono::milliseconds(100));
    EXPECT_EQ(all_stats["plugin2"].load_time, std::chrono::milliseconds(200));
    EXPECT_EQ(all_stats["plugin3"].load_time, std::chrono::milliseconds(300));
}

TEST_F(PluginMetricsTest, ResetStats) {
    metrics.recordLoad("test_plugin", std::chrono::milliseconds(100));
    metrics.recordCall("test_plugin", std::chrono::microseconds(10000));
    metrics.recordError("test_plugin");
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_GT(stats.load_time.count(), 0);
    
    metrics.resetStats("test_plugin");
    
    stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.load_time.count(), 0);
    EXPECT_EQ(stats.function_calls, 0);
    EXPECT_EQ(stats.errors, 0);
}

TEST_F(PluginMetricsTest, ResetAll) {
    metrics.recordLoad("plugin1", std::chrono::milliseconds(100));
    metrics.recordLoad("plugin2", std::chrono::milliseconds(200));
    
    EXPECT_EQ(metrics.getAllStats().size(), 2);
    
    metrics.resetAll();
    
    EXPECT_EQ(metrics.getAllStats().size(), 0);
}

TEST_F(PluginMetricsTest, ThreadSafety) {
    // Test concurrent access from multiple threads
    const int num_threads = 10;
    const int calls_per_thread = 100;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, calls_per_thread]() {
            for (int i = 0; i < calls_per_thread; i++) {
                metrics.recordCall("test_plugin", std::chrono::microseconds(10000));
                metrics.recordError("test_plugin");
                metrics.updateMemoryUsage("test_plugin", 1024 * (i + 1));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.function_calls, num_threads * calls_per_thread);
    EXPECT_EQ(stats.errors, num_threads * calls_per_thread);
}

TEST_F(PluginMetricsTest, EmptyStats) {
    auto stats = metrics.getStats("nonexistent_plugin");
    EXPECT_EQ(stats.load_time.count(), 0);
    EXPECT_EQ(stats.function_calls, 0);
    EXPECT_EQ(stats.errors, 0);
}

TEST_F(PluginMetricsTest, LatencySampleLimit) {
    // Record more than MAX_SAMPLES (1000) calls
    for (int i = 0; i < 1500; i++) {
        metrics.recordCall("test_plugin", std::chrono::microseconds(i * 100));
    }
    
    auto stats = metrics.getStats("test_plugin");
    EXPECT_EQ(stats.function_calls, 1500);
    
    // Percentiles should still be calculated correctly
    // (based on the most recent 1000 samples)
    EXPECT_GT(stats.p95_call_latency_ms, 0);
    EXPECT_GT(stats.p99_call_latency_ms, 0);
    EXPECT_GE(stats.p99_call_latency_ms, stats.p95_call_latency_ms);
}


