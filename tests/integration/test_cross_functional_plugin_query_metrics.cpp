/**
 * @file test_cross_functional_plugin_query_metrics.cpp
 * @brief Cross-functional integration test: Plugin System + Query Engine + Metrics
 * 
 * Tests the complete workflow of plugin management integrated with query
 * processing and comprehensive metrics collection.
 * 
 * @author ThemisDB Team
 * @date January 2025
 */

#include <gtest/gtest.h>
#include "plugins/plugin_manager.h"
#include "observability/metrics_collector.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

using namespace themis::plugins;
using namespace themis::observability;
using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * @brief Cross-functional test for Plugin + Query + Metrics integration
 * 
 * This test validates that:
 * - Plugin system loads and manages plugins
 * - Query operations can use plugins
 * - All operations are tracked with metrics
 * - Components interact seamlessly
 */
class CrossFunctionalPluginQueryMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test plugin directory in temp location
        test_plugin_dir_ = fs::temp_directory_path() / "themis_cross_func_plugins";
        fs::create_directories(test_plugin_dir_);
        
        // Reset metrics
        MetricsCollector::getInstance().reset();
        
        // Create plugin manager
        plugin_manager_ = std::make_unique<PluginManager>();
    }
    
    void TearDown() override {
        plugin_manager_.reset();
        MetricsCollector::getInstance().reset();
        
        if (fs::exists(test_plugin_dir_)) {
            fs::remove_all(test_plugin_dir_);
        }
    }
    
    void createTestPluginManifest(const std::string& name, PluginType type) {
        json manifest;
        manifest["name"] = name;
        manifest["version"] = "1.0.0";
        manifest["type"] = static_cast<int>(type);
        manifest["author"] = "Cross-Functional Test";
        manifest["description"] = "Test plugin for integration testing";
        manifest["library"] = name + ".so";
        
        fs::path path = test_plugin_dir_ / (name + ".json");
        std::ofstream file(path.string());
        file << manifest.dump(2);
        file.close();
    }

    fs::path test_plugin_dir_;
    std::unique_ptr<PluginManager> plugin_manager_;
};

// ============================================================================
// Plugin Discovery with Metrics Tracking
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginDiscoveryWithMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create test plugins
    createTestPluginManifest("compute_plugin", PluginType::COMPUTE_BACKEND);
    createTestPluginManifest("content_plugin", PluginType::EMBEDDING);
    createTestPluginManifest("storage_plugin", PluginType::BLOB_STORAGE);
    
    auto start = std::chrono::steady_clock::now();
    
    auto discover = [&]() {
        auto scan_result = plugin_manager_->scanPluginDirectory(test_plugin_dir_.string());
        ASSERT_TRUE(scan_result.has_value());
        size_t count = scan_result.value();
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        EXPECT_EQ(count, 3);
        
        // Record metrics for plugin discovery
        metrics.recordQuery("plugin_scan", duration_ms, count);
        metrics.recordIndexScan("plugin_index", count);
    };
    EXPECT_NO_THROW(discover());
    
    // Verify metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginLoadingWithPerformanceMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create and discover plugins
    for (int i = 0; i < 5; i++) {
        std::string name = "test_plugin_" + std::to_string(i);
        createTestPluginManifest(name, PluginType::COMPUTE_BACKEND);
    }
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    // Attempt to load plugins and track metrics
    for (int i = 0; i < 5; i++) {
        std::string name = "test_plugin_" + std::to_string(i);
        
        auto start = std::chrono::steady_clock::now();
        
        auto load_plugin = [&]() {
            auto result = plugin_manager_->loadPlugin(name);
            IThemisPlugin* plugin = result.has_value() ? *result : nullptr;
            
            auto end = std::chrono::steady_clock::now();
            auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            // Record load attempt metrics (will fail without real library)
            metrics.recordQuery("plugin_load_attempt", duration_ms, plugin ? 1 : 0);
        };
        EXPECT_NO_THROW(load_plugin());
    }
    
    // Verify load metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

// ============================================================================
// Plugin Query Operations with Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginQueryWithMetricsTracking) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create plugins of different types
    createTestPluginManifest("compute1", PluginType::COMPUTE_BACKEND);
    createTestPluginManifest("compute2", PluginType::COMPUTE_BACKEND);
    createTestPluginManifest("content1", PluginType::EMBEDDING);
    createTestPluginManifest("storage1", PluginType::BLOB_STORAGE);
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    // Query plugins by type with metrics
    std::vector<PluginType> types = {
        PluginType::COMPUTE_BACKEND,
        PluginType::EMBEDDING,
        PluginType::BLOB_STORAGE
    };
    
    for (auto type : types) {
        auto start = std::chrono::steady_clock::now();
        
        auto plugins = plugin_manager_->getPluginsByType(type);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record query metrics
        metrics.recordQuery("plugin_query_by_type", duration_ms, plugins.size());
        metrics.recordIndexScan("plugin_type_index", plugins.size());
    }
    
    // Verify query metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("index_scans"), std::string::npos);
}

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginInfoRetrievalWithCacheMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create and discover plugins
    for (int i = 0; i < 10; i++) {
        createTestPluginManifest("plugin" + std::to_string(i), PluginType::COMPUTE_BACKEND);
    }
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    // Query plugin info multiple times (simulating cache behavior)
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 10; i++) {
            std::string name = "plugin" + std::to_string(i);
            
            auto start = std::chrono::steady_clock::now();
            auto info = plugin_manager_->getManifest(name);
            auto end = std::chrono::steady_clock::now();
            auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            if (info.has_value()) {
                // Simulate cache hit on subsequent rounds
                if (round > 0) {
                    metrics.recordCacheHit("plugin_info_cache");
                } else {
                    metrics.recordCacheMiss("plugin_info_cache");
                }
                
                metrics.recordQuery("plugin_info_query", duration_ms, 1);
            }
        }
    }
    
    // Verify cache metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("cache_hits"), std::string::npos);
    EXPECT_NE(prometheus_metrics.find("cache_misses"), std::string::npos);
}

// ============================================================================
// Concurrent Plugin Operations with Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, ConcurrentPluginAccessWithMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create test plugins
    for (int i = 0; i < 20; i++) {
        createTestPluginManifest("concurrent_plugin" + std::to_string(i), PluginType::COMPUTE_BACKEND);
    }
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; t++) {
        threads.emplace_back([this, &metrics, t]() {
            for (int i = 0; i < 10; i++) {
                std::string name = "concurrent_plugin" + std::to_string((t + i) % 20);
                
                auto start = std::chrono::steady_clock::now();
                
                // Query plugin info
                auto info = plugin_manager_->getManifest(name);
                
                // Check if loaded
                [[maybe_unused]] bool loaded = plugin_manager_->isPluginLoaded(name);
                
                auto end = std::chrono::steady_clock::now();
                auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
                
                // Record concurrent access metrics
                metrics.recordQuery("concurrent_plugin_access", duration_ms, 1);
                
                if (info.has_value()) {
                    metrics.recordCacheHit("plugin_cache");
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify concurrent metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_FALSE(prometheus_metrics.empty());
}

// ============================================================================
// Plugin Statistics with Metrics Export
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginStatisticsWithMetricsExport) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create various plugin types
    createTestPluginManifest("compute_plugin", PluginType::COMPUTE_BACKEND);
    createTestPluginManifest("content_plugin", PluginType::EMBEDDING);
    createTestPluginManifest("storage_plugin", PluginType::BLOB_STORAGE);
    createTestPluginManifest("security_plugin", PluginType::CUSTOM);
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    auto start = std::chrono::steady_clock::now();
    
    auto export_stats = [&]() {
        // Get plugin statistics
        auto stats = plugin_manager_->listPlugins();
        
        // Get metrics export
        std::string metrics_export = metrics.getPrometheusMetrics();
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record statistics gathering metrics
        metrics.recordQuery("get_plugin_statistics", duration_ms, stats.size());
        
        EXPECT_FALSE(stats.empty());
        EXPECT_FALSE(metrics_export.empty());
    };
    EXPECT_NO_THROW(export_stats());
}

// ============================================================================
// Plugin Lifecycle with Full Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginLifecycleWithFullMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    createTestPluginManifest("lifecycle_plugin", PluginType::COMPUTE_BACKEND);
    
    // Discovery phase
    auto t1 = std::chrono::steady_clock::now();
    auto discovery = plugin_manager_->scanPluginDirectory(test_plugin_dir_.string());
    ASSERT_TRUE(discovery.has_value());
    size_t count = discovery.value();
    auto t2 = std::chrono::steady_clock::now();
    metrics.recordQuery("plugin_discovery", 
        std::chrono::duration<double, std::milli>(t2 - t1).count(), count);
    
    // Query phase
    t1 = std::chrono::steady_clock::now();
    auto info = plugin_manager_->getManifest("lifecycle_plugin");
    t2 = std::chrono::steady_clock::now();
    metrics.recordQuery("plugin_info_query",
        std::chrono::duration<double, std::milli>(t2 - t1).count(), 1);
    
    // Load attempt phase
    t1 = std::chrono::steady_clock::now();
    auto result = plugin_manager_->loadPlugin("lifecycle_plugin");
    IThemisPlugin* plugin = result.has_value() ? *result : nullptr;
    t2 = std::chrono::steady_clock::now();
    metrics.recordQuery("plugin_load",
        std::chrono::duration<double, std::milli>(t2 - t1).count(), plugin ? 1 : 0);
    
    // Check status phase
    t1 = std::chrono::steady_clock::now();
    bool loaded = plugin_manager_->isPluginLoaded("lifecycle_plugin");
    t2 = std::chrono::steady_clock::now();
    metrics.recordQuery("plugin_status_check",
        std::chrono::duration<double, std::milli>(t2 - t1).count(), loaded ? 1 : 0);
    
    // Verify comprehensive lifecycle metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

// ============================================================================
// Multi-Type Plugin Query with Index Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, MultiTypePluginQueryWithIndexMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    // Create mixed plugin types
    for (int i = 0; i < 20; i++) {
        PluginType type = static_cast<PluginType>(i % 4);
        createTestPluginManifest("mixed_plugin" + std::to_string(i), type);
    }
    
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    // Query all plugins
    auto start = std::chrono::steady_clock::now();
    auto all_plugins = plugin_manager_->listPlugins();
    auto end = std::chrono::steady_clock::now();
    
    metrics.recordQuery("get_all_plugins",
        std::chrono::duration<double, std::milli>(end - start).count(), all_plugins.size());
    metrics.recordFullScan("plugins", all_plugins.size());
    
    // Query by each type
    for (int type_id = 0; type_id < 4; type_id++) {
        PluginType type = static_cast<PluginType>(type_id);
        
        start = std::chrono::steady_clock::now();
        auto typed_plugins = plugin_manager_->getPluginsByType(type);
        end = std::chrono::steady_clock::now();
        
        metrics.recordQuery("get_plugins_by_type",
            std::chrono::duration<double, std::milli>(end - start).count(), typed_plugins.size());
        metrics.recordIndexScan("plugin_type_index", typed_plugins.size());
    }
    
    // Verify index usage metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("index_scans"), std::string::npos);
    EXPECT_NE(prometheus_metrics.find("full_scans"), std::string::npos);
}

// ============================================================================
// Plugin Hot-Reload with Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, PluginHotReloadWithMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    createTestPluginManifest("reload_plugin", PluginType::COMPUTE_BACKEND);
    ASSERT_TRUE(plugin_manager_->scanPluginDirectory(test_plugin_dir_.string()).has_value());
    
    // Attempt reload multiple times with metrics
    for (int i = 0; i < 5; i++) {
        auto start = std::chrono::steady_clock::now();
        
        auto reload_result = plugin_manager_->reloadPlugin("reload_plugin");
        bool result = reload_result.has_value();
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Record reload metrics
        metrics.recordQuery("plugin_reload", duration_ms, result ? 1 : 0);
        
        // Simulate resource usage during reload
        metrics.recordMemoryUsage(1024 * 1024 * (i + 1)); // Incremental memory
        metrics.recordCPUUsage(50.0 + i * 5.0);
    }
    
    // Verify reload and resource metrics
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_NE(prometheus_metrics.find("queries_total"), std::string::npos);
}

// ============================================================================
// Error Handling with Comprehensive Metrics
// ============================================================================

TEST_F(CrossFunctionalPluginQueryMetricsTest, ErrorHandlingWithMetrics) {
    auto& metrics = MetricsCollector::getInstance();
    
    auto error_flow = [&]() {
        // Query non-existent plugin
        auto start = std::chrono::steady_clock::now();
        auto info = plugin_manager_->getManifest("nonexistent");
        auto end = std::chrono::steady_clock::now();
        (void)info;
        
        metrics.recordQuery("plugin_query_error",
            std::chrono::duration<double, std::milli>(end - start).count(), 0);
        metrics.recordCacheMiss("plugin_info_cache");
        
        // Load non-existent plugin
        start = std::chrono::steady_clock::now();
        auto result = plugin_manager_->loadPlugin("nonexistent");
        IThemisPlugin* plugin = result.has_value() ? *result : nullptr;
        end = std::chrono::steady_clock::now();
        
        metrics.recordQuery("plugin_load_error",
            std::chrono::duration<double, std::milli>(end - start).count(), 0);
        
        EXPECT_EQ(plugin, nullptr);
        EXPECT_FALSE(result.has_value());
    };
    EXPECT_NO_THROW(error_flow());
    
    // Verify error metrics are tracked
    std::string prometheus_metrics = metrics.getPrometheusMetrics();
    EXPECT_FALSE(prometheus_metrics.empty());
}

// ============================================================================
// Main
// ============================================================================


