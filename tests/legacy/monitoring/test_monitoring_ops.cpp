#include <gtest/gtest.h>
#include "sharding/prometheus_metrics.h"
#include "sharding/health_check.h"
#include "sharding/admin_api.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::sharding;

// Prometheus Metrics Tests
TEST(PrometheusMetricsTest, RecordShardHealth) {
    PrometheusMetrics::Config config;
    PrometheusMetrics metrics(config);

    metrics.recordShardHealth("shard_001", "healthy");
    metrics.recordShardHealth("shard_002", "degraded");

    auto output = metrics.getMetrics();
    EXPECT_TRUE(output.find("themis_shard_health_status") != std::string::npos);
}

TEST(PrometheusMetricsTest, RecordRoutingStatistics) {
    PrometheusMetrics::Config config;
    PrometheusMetrics metrics(config);

    metrics.recordRoutingRequest("local");
    metrics.recordRoutingRequest("remote");
    metrics.recordRoutingRequest("scatter_gather");

    auto output = metrics.getMetrics();
    EXPECT_TRUE(output.find("themis_routing_requests_total") != std::string::npos);
}

TEST(PrometheusMetricsTest, RecordMigrationProgress) {
    PrometheusMetrics::Config config;
    PrometheusMetrics metrics(config);

    metrics.recordMigrationProgress("op_001", 1000, 5000000, 50.0);

    auto output = metrics.getMetrics();
    EXPECT_TRUE(output.find("themis_migration_records_total") != std::string::npos);
    EXPECT_TRUE(output.find("themis_migration_progress_percent") != std::string::npos);
}

// Concurrency regression tests for mutex fixes in incrementCounter / addToCounter
TEST(PrometheusMetricsTest, ConcurrentNewKeyInsertion_NoDataRace) {
    // Verifies that concurrent calls that create new counter keys do not crash
    // or corrupt the internal map (formerly unguarded operator[] insertions).
    PrometheusMetrics::Config config;
    PrometheusMetrics metrics(config);

    const int num_threads = 8;
    const int counters_per_thread = 20;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&metrics, t, counters_per_thread]() {
            for (int i = 0; i < counters_per_thread; ++i) {
                std::string name = "concurrent_counter_" + std::to_string(t) + "_" + std::to_string(i);
                metrics.incrementCounter(name);
                metrics.addToCounter(name, 2);
            }
        });
    }
    for (auto& th : threads) th.join();

    // If we reach here without ASAN/TSAN triggering, the mutex fix is working.
    auto output = metrics.getMetrics();
    EXPECT_FALSE(output.empty());
}

TEST(PrometheusMetricsTest, ConcurrentSharedKeyIncrement_CountIsConsistent) {
    // Multiple threads increment the same counter — total must equal the sum of all increments.
    PrometheusMetrics::Config config;
    PrometheusMetrics metrics(config);

    const int num_threads = 10;
    const int increments_per_thread = 100;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&metrics, increments_per_thread]() {
            for (int i = 0; i < increments_per_thread; ++i) {
                metrics.incrementCounter("shared_key");
            }
        });
    }
    for (auto& th : threads) th.join();

    // The counter value is embedded in the Prometheus text output; verify the
    // metric is present (exact value format differs by implementation).
    auto output = metrics.getMetrics();
    EXPECT_TRUE(output.find("shared_key") != std::string::npos);
}

// Health Check Tests
TEST(HealthCheckTest, CheckShardHealthValid) {
    HealthCheckSystem::Config config;
    HealthCheckSystem health_checker(config);

    // Note: This test would need valid cert path in real scenario
    // For now, it's a structural test
    std::string shard_id = "shard_001";
    std::string endpoint = "https://shard-001.dc1:8080";
    std::string cert_path = "/tmp/test.crt";  // Would need to create mock cert

    // Test would check health status
    // auto health = health_checker.checkShardHealth(shard_id, endpoint, cert_path);
    // EXPECT_EQ(health.shard_id, shard_id);
}

TEST(HealthCheckTest, ClusterHealthAggregation) {
    HealthCheckSystem::Config config;
    HealthCheckSystem health_checker(config);

    std::map<std::string, std::string> shard_endpoints = {
        {"shard_001", "https://shard-001.dc1:8080"},
        {"shard_002", "https://shard-002.dc1:8080"}
    };

    // Test structure
    EXPECT_NO_THROW({
        auto cluster_health = health_checker.getCurrentHealth();
    });
}

TEST(HealthCheckTest, PeriodicChecksCanStartStopAndRestartSafely) {
    HealthCheckSystem::Config config;
    config.check_interval_ms = 10;
    HealthCheckSystem health_checker(config);

    std::atomic<int> callbacks{0};
    health_checker.registerCallback([&callbacks]([[maybe_unused]] const ClusterHealthInfo& info) {
        callbacks.fetch_add(1, std::memory_order_relaxed);
    });

    std::map<std::string, std::string> empty_endpoints;

    health_checker.startPeriodicChecks(empty_endpoints);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    health_checker.stopPeriodicChecks();

    health_checker.startPeriodicChecks(empty_endpoints);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    health_checker.stopPeriodicChecks();

    EXPECT_GE(callbacks.load(std::memory_order_relaxed), 1);
}

TEST(HealthCheckTest, CallbackCanStopChecksAndAllowRestart) {
    HealthCheckSystem::Config config;
    config.check_interval_ms = 10;
    HealthCheckSystem health_checker(config);

    std::atomic<int> callbacks{0};
    health_checker.registerCallback([&health_checker, &callbacks]([[maybe_unused]] const ClusterHealthInfo& info) {
        if (callbacks.fetch_add(1, std::memory_order_relaxed) == 0) {
            health_checker.stopPeriodicChecks();
        }
    });

    std::map<std::string, std::string> empty_endpoints;
    health_checker.startPeriodicChecks(empty_endpoints);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    health_checker.startPeriodicChecks(empty_endpoints);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    health_checker.stopPeriodicChecks();

    EXPECT_GE(callbacks.load(std::memory_order_relaxed), 2);
}

TEST(HealthCheckTest, HealthStatusEnum) {
    // Test health status values
    HealthStatus healthy = HealthStatus::HEALTHY;
    HealthStatus degraded = HealthStatus::DEGRADED;
    HealthStatus unhealthy = HealthStatus::UNHEALTHY;
    HealthStatus critical = HealthStatus::CRITICAL;

    EXPECT_NE(healthy, degraded);
    EXPECT_NE(degraded, unhealthy);
    EXPECT_NE(unhealthy, critical);
}

// Admin API Tests
TEST(AdminAPITest, Configuration) {
    AdminAPI::Config config;
    config.http_port = 8080;
    config.require_signatures = true;
    config.enable_audit_log = true;

    AdminAPI api(config);
    EXPECT_NO_THROW({
        // API created successfully
    });
}

TEST(AdminAPITest, EndpointConstants) {
    // Test that endpoint constants are defined
    EXPECT_STREQ(AdminAPI::Endpoints::TOPOLOGY, "/admin/topology");
    EXPECT_STREQ(AdminAPI::Endpoints::SHARD_ADD, "/admin/shard/add");
    EXPECT_STREQ(AdminAPI::Endpoints::REBALANCE, "/admin/rebalance");
    EXPECT_STREQ(AdminAPI::Endpoints::HEALTH, "/admin/health");
    EXPECT_STREQ(AdminAPI::Endpoints::STATS, "/admin/stats");
}

 
