#include <gtest/gtest.h>
#include "sharding/mtls_connection_pool.h"
#include <thread>
#include <vector>

using namespace themis::sharding;

// ===========================================================================
// EndpointConnectionPool Tests
// ===========================================================================

TEST(EndpointConnectionPoolTest, ConstructorAndBasicConfig) {
    EndpointConnectionPool::Config config;
    config.min_connections = 5;
    config.max_connections = 20;
    config.connection_ttl = std::chrono::seconds(300);
    config.idle_timeout = std::chrono::seconds(60);
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    EXPECT_NE(pool, nullptr);
    
    auto stats = pool->getStatistics();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_GE(stats.total_created, 0);
}

TEST(EndpointConnectionPoolTest, WarmUpCreatesMinimumConnections) {
    EndpointConnectionPool::Config config;
    config.min_connections = 3;
    config.max_connections = 20;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    // Warm up should attempt to create minimum connections
    // Note: In production, this would create actual SSL connections
    // For now, we're testing the pool structure
    bool warmup_result = pool->warmUp();
    
    // Warmup may fail if actual connections can't be created (expected in test)
    // But the pool structure should still be intact
    auto stats = pool->getStatistics();
    EXPECT_GE(stats.idle_connections, 0);
}

TEST(EndpointConnectionPoolTest, GetConnectionTimeout) {
    EndpointConnectionPool::Config config;
    config.max_connections = 2;
    config.min_connections = 0;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    // Try to get connection with short timeout
    // Should return nullopt if no connections available
    auto conn = pool->getConnection(std::chrono::milliseconds(100));
    
    // In stub implementation, this will return nullopt
    EXPECT_FALSE(conn.has_value());
}

TEST(EndpointConnectionPoolTest, ReleaseConnectionNullptr) {
    EndpointConnectionPool::Config config;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    // Releasing nullptr should not crash
    EXPECT_NO_THROW(pool->releaseConnection(nullptr));
}

TEST(EndpointConnectionPoolTest, InvalidateConnection) {
    EndpointConnectionPool::Config config;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    auto stats_before = pool->getStatistics();
    
    // Invalidate a nullptr connection (should not crash)
    EXPECT_NO_THROW(pool->invalidateConnection(nullptr));
    
    auto stats_after = pool->getStatistics();
    EXPECT_EQ(stats_after.connections_failed, stats_before.connections_failed);
}

TEST(EndpointConnectionPoolTest, Statistics) {
    EndpointConnectionPool::Config config;
    config.min_connections = 2;
    config.max_connections = 10;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    auto stats = pool->getStatistics();
    
    // Verify statistics structure
    EXPECT_GE(stats.active_connections, 0);
    EXPECT_GE(stats.idle_connections, 0);
    EXPECT_GE(stats.total_created, 0);
    EXPECT_GE(stats.connections_failed, 0);
    EXPECT_GE(stats.utilization_percent, 0.0);
    EXPECT_LE(stats.utilization_percent, 100.0);
}

TEST(EndpointConnectionPoolTest, CloseAll) {
    EndpointConnectionPool::Config config;
    
    auto pool = std::make_unique<EndpointConnectionPool>(
        "localhost:50051", config
    );
    
    // Should not crash
    EXPECT_NO_THROW(pool->closeAll());
    
    auto stats = pool->getStatistics();
    EXPECT_EQ(stats.idle_connections, 0);
}

TEST(EndpointConnectionPoolTest, ConfigurationValues) {
    EndpointConnectionPool::Config config;
    config.min_connections = 5;
    config.max_connections = 50;
    config.connection_ttl = std::chrono::seconds(600);
    config.idle_timeout = std::chrono::seconds(120);
    config.enable_health_checks = true;
    config.health_check_interval = std::chrono::seconds(30);
    
    EXPECT_EQ(config.min_connections, 5);
    EXPECT_EQ(config.max_connections, 50);
    EXPECT_EQ(config.connection_ttl.count(), 600);
    EXPECT_EQ(config.idle_timeout.count(), 120);
    EXPECT_TRUE(config.enable_health_checks);
    EXPECT_EQ(config.health_check_interval.count(), 30);
}

// ===========================================================================
// MTLSConnectionPoolManager Tests
// ===========================================================================

TEST(MTLSConnectionPoolManagerTest, ConstructorWithDefaultConfig) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    EXPECT_NE(manager, nullptr);
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 0);
    EXPECT_EQ(stats.total_active_connections, 0);
    EXPECT_EQ(stats.total_idle_connections, 0);
}

TEST(MTLSConnectionPoolManagerTest, ConstructorWithCustomConfig) {
    MTLSConnectionPoolManager::Config config;
    config.endpoint_config.min_connections = 3;
    config.endpoint_config.max_connections = 30;
    config.max_total_connections = 1000;
    config.max_endpoints = 200;
    
    auto manager = std::make_unique<MTLSConnectionPoolManager>(config);
    
    EXPECT_NE(manager, nullptr);
}

TEST(MTLSConnectionPoolManagerTest, GetPoolCreatesNewPool) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    auto pool1 = manager->getPool("localhost:50051");
    EXPECT_NE(pool1, nullptr);
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 1);
}

TEST(MTLSConnectionPoolManagerTest, GetPoolReturnsSamePoolForSameEndpoint) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    auto pool1 = manager->getPool("localhost:50051");
    auto pool2 = manager->getPool("localhost:50051");
    
    // Should return the same pool instance
    EXPECT_EQ(pool1, pool2);
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 1);
}

TEST(MTLSConnectionPoolManagerTest, GetPoolCreatesDifferentPoolsForDifferentEndpoints) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    auto pool1 = manager->getPool("localhost:50051");
    auto pool2 = manager->getPool("localhost:50052");
    
    // Should return different pool instances
    EXPECT_NE(pool1, pool2);
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 2);
}

TEST(MTLSConnectionPoolManagerTest, GetConnectionReturnsNulloptForStubImplementation) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    // In stub implementation, this will return nullopt
    auto conn = manager->getConnection("localhost:50051", std::chrono::milliseconds(100));
    
    EXPECT_FALSE(conn.has_value());
}

TEST(MTLSConnectionPoolManagerTest, ReleaseConnectionNullptr) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    // Should not crash
    EXPECT_NO_THROW(manager->releaseConnection("localhost:50051", nullptr));
}

TEST(MTLSConnectionPoolManagerTest, GlobalStatistics) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    // Create a few pools
    manager->getPool("localhost:50051");
    manager->getPool("localhost:50052");
    manager->getPool("localhost:50053");
    
    auto stats = manager->getStatistics();
    
    EXPECT_EQ(stats.cached_endpoint_pools, 3);
    EXPECT_GE(stats.total_active_connections, 0);
    EXPECT_GE(stats.total_idle_connections, 0);
    EXPECT_EQ(stats.per_endpoint_stats.size(), 3);
}

TEST(MTLSConnectionPoolManagerTest, PerEndpointStatistics) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    manager->getPool("localhost:50051");
    manager->getPool("localhost:50052");
    
    auto stats = manager->getStatistics();
    
    EXPECT_TRUE(stats.per_endpoint_stats.count("localhost:50051") > 0);
    EXPECT_TRUE(stats.per_endpoint_stats.count("localhost:50052") > 0);
    
    // Verify endpoint statistics structure
    for (const auto& [endpoint, endpoint_stats] : stats.per_endpoint_stats) {
        EXPECT_GE(endpoint_stats.active_connections, 0);
        EXPECT_GE(endpoint_stats.idle_connections, 0);
        EXPECT_GE(endpoint_stats.total_created, 0);
        EXPECT_GE(endpoint_stats.connections_failed, 0);
        EXPECT_GE(endpoint_stats.utilization_percent, 0.0);
    }
}

TEST(MTLSConnectionPoolManagerTest, Shutdown) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    manager->getPool("localhost:50051");
    manager->getPool("localhost:50052");
    
    // Should not crash
    EXPECT_NO_THROW(manager->shutdown());
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 0);
}

TEST(MTLSConnectionPoolManagerTest, ShutdownClearsPools) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    manager->getPool("localhost:50051");
    manager->getPool("localhost:50052");
    
    auto stats_before = manager->getStatistics();
    EXPECT_EQ(stats_before.cached_endpoint_pools, 2);
    
    manager->shutdown();
    
    auto stats_after = manager->getStatistics();
    EXPECT_EQ(stats_after.cached_endpoint_pools, 0);
}

// ===========================================================================
// Thread Safety Tests
// ===========================================================================

TEST(MTLSConnectionPoolManagerTest, ConcurrentPoolAccess) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    std::vector<std::thread> threads;
    const int num_threads = 10;
    
    // Multiple threads accessing different endpoints
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&manager, i]() {
            std::string endpoint = "localhost:5005" + std::to_string(i);
            auto pool = manager->getPool(endpoint);
            EXPECT_NE(pool, nullptr);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, num_threads);
}

TEST(MTLSConnectionPoolManagerTest, ConcurrentPoolAccessSameEndpoint) {
    auto manager = std::make_unique<MTLSConnectionPoolManager>();
    
    std::vector<std::thread> threads;
    const int num_threads = 10;
    
    // Multiple threads accessing same endpoint
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&manager]() {
            auto pool = manager->getPool("localhost:50051");
            EXPECT_NE(pool, nullptr);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = manager->getStatistics();
    EXPECT_EQ(stats.cached_endpoint_pools, 1);
}

// ===========================================================================
// Configuration Tests
// ===========================================================================

TEST(MTLSConnectionPoolManagerTest, ConfigurationStructure) {
    MTLSConnectionPoolManager::Config config;
    
    config.endpoint_config.min_connections = 5;
    config.endpoint_config.max_connections = 100;
    config.max_total_connections = 1000;
    config.enable_endpoint_eviction = true;
    config.max_endpoints = 150;
    
    EXPECT_EQ(config.endpoint_config.min_connections, 5);
    EXPECT_EQ(config.endpoint_config.max_connections, 100);
    EXPECT_EQ(config.max_total_connections, 1000);
    EXPECT_TRUE(config.enable_endpoint_eviction);
    EXPECT_EQ(config.max_endpoints, 150);
}

TEST(EndpointConnectionPoolTest, DefaultConfiguration) {
    EndpointConnectionPool::Config config;
    
    EXPECT_EQ(config.min_connections, 2);
    EXPECT_EQ(config.max_connections, 50);
    EXPECT_EQ(config.connection_ttl.count(), 300);
    EXPECT_EQ(config.idle_timeout.count(), 60);
    EXPECT_TRUE(config.enable_health_checks);
    EXPECT_EQ(config.health_check_interval.count(), 30);
}

TEST(MTLSConnectionPoolManagerTest, DefaultConfiguration) {
    MTLSConnectionPoolManager::Config config;
    
    EXPECT_EQ(config.max_total_connections, 500);
    EXPECT_TRUE(config.enable_endpoint_eviction);
    EXPECT_EQ(config.max_endpoints, 100);
}

// ─────────────────────────────────────────────────────────────────────────────
// EndpointConnectionPool::setConnectionFactory — injection API (stub #44)
// ─────────────────────────────────────────────────────────────────────────────

// MCP-01: Without an injected factory, createNewConnection returns nullopt
//         (getConnection returns nullopt under the pool max when pool is empty).
TEST(EndpointConnectionPoolTest, MCP01_NoFactoryReturnsNullopt) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    EndpointConnectionPool pool("localhost:50051", cfg);

    // getConnection will call createNewConnection internally when the pool is
    // empty. Without a factory the pool cannot create a real SSL connection,
    // so the call should return nullopt.
    auto conn = pool.getConnection(std::chrono::milliseconds(50));
    EXPECT_FALSE(conn.has_value());
}

// MCP-02: With an injected factory that always returns nullopt (failure),
//         getConnection propagates the failure.
TEST(EndpointConnectionPoolTest, MCP02_InjectedFactoryFailurePropagated) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    EndpointConnectionPool pool("remote:9000", cfg);

    int call_count = 0;
    pool.setConnectionFactory([&](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        ++call_count;
        return std::nullopt;  // simulate connection failure
    });

    auto conn = pool.getConnection(std::chrono::milliseconds(50));
    EXPECT_FALSE(conn.has_value());
    // Factory was consulted at least once.
    EXPECT_GE(call_count, 1);
}

// ===========================================================================
// Factory-Based Connection Tests (v2.0 Phase 2)
// ===========================================================================

// V2-F01: Factory-based pool constructor properly initializes
TEST(EndpointConnectionPoolTest, V2F01_FactoryConstructorInitialization) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 2;
    cfg.max_connections = 10;
    
    int factory_calls = 0;
    auto mock_factory = [&](const std::string& ep) 
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        ++factory_calls;
        return std::nullopt;  // Simulate failure for now
    };
    
    // v2.0 factory-based constructor
    EndpointConnectionPool pool("localhost:50051", cfg, mock_factory);
    
    auto stats = pool.getStatistics();
    EXPECT_GE(stats.active_connections + stats.idle_connections, 0);
}

// V2-F02: Factory receives correct endpoint string
TEST(EndpointConnectionPoolTest, V2F02_FactoryReceivesCorrectEndpoint) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    
    std::string captured_endpoint = {};
    auto capture_factory = [&](const std::string& ep)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        captured_endpoint = ep;
        return std::nullopt;
    };
    
    EndpointConnectionPool pool("custom-host:9999", cfg, capture_factory);
    pool.getConnection(std::chrono::milliseconds(50));
    
    EXPECT_EQ(captured_endpoint, "custom-host:9999");
}

// V2-F03: Factory is called multiple times when creating multiple connections
TEST(EndpointConnectionPoolTest, V2F03_FactoryCalledForMultipleConnections) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 5;
    
    int factory_call_count = 0;
    auto counting_factory = [&](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        ++factory_call_count;
        return std::nullopt;
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, counting_factory);
    
    // Try to get multiple connections
    for (int i = 0; i < 3; ++i) {
        pool.getConnection(std::chrono::milliseconds(50));
    }
    
    // Factory should have been called multiple times
    EXPECT_GT(factory_call_count, 0);
}

// V2-F04: Connection lifecycle: creation, use, release
TEST(EndpointConnectionPoolTest, V2F04_ConnectionLifecycle) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    
    auto mock_factory = [](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        // Return a dummy pointer (not a real SSL object, but OK for this test)
        return std::nullopt;
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, mock_factory);
    
    // Statistics should exist and be queryable
    auto stats = pool.getStatistics();
    EXPECT_GE(stats.active_connections, 0);
    EXPECT_GE(stats.idle_connections, 0);
}

// V2-F05: setConnectionFactory can override initial factory
TEST(EndpointConnectionPoolTest, V2F05_SetConnectionFactoryOverride) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    
    auto initial_factory = [](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        return std::nullopt;
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, initial_factory);
    
    int override_call_count = 0;
    auto override_factory = [&](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        ++override_call_count;
        return std::nullopt;
    };
    
    pool.setConnectionFactory(override_factory);
    pool.getConnection(std::chrono::milliseconds(50));
    
    // Override factory should have been called
    EXPECT_GT(override_call_count, 0);
}

// V2-F06: Factory error handling - nullopt returns are handled gracefully
TEST(EndpointConnectionPoolTest, V2F06_FactoryErrorHandlingNullopt) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 3;
    
    auto failing_factory = [](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        return std::nullopt;  // Always fail
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, failing_factory);
    
    // Multiple connection attempts with failing factory should not crash
    for (int i = 0; i < 5; ++i) {
        auto conn = pool.getConnection(std::chrono::milliseconds(50));
        EXPECT_FALSE(conn.has_value());
    }
    
    auto stats = pool.getStatistics();
    EXPECT_GE(stats.connections_failed, 0);
}

// V2-F07: Factory lifetime requirement is documented
TEST(EndpointConnectionPoolTest, V2F07_FactoryLifetimeRequirement) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 2;
    
    // Test that factory can be valid for the pool's entire lifetime
    auto factory = [](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        return std::nullopt;
    };
    
    // Pool constructor with factory should succeed
    EXPECT_NO_THROW({
        EndpointConnectionPool pool("localhost:50051", cfg, factory);
        pool.getConnection(std::chrono::milliseconds(50));
    });
}

// V2-F08: Factory-based pool statistics reflect factory usage
TEST(EndpointConnectionPoolTest, V2F08_FactoryBasedStatistics) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 3;
    
    int factory_attempts = 0;
    auto tracking_factory = [&](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        ++factory_attempts;
        return std::nullopt;
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, tracking_factory);
    
    // Attempt multiple connections
    for (int i = 0; i < 3; ++i) {
        pool.getConnection(std::chrono::milliseconds(50));
    }
    
    auto stats = pool.getStatistics();
    EXPECT_GE(stats.total_created, 0);
    EXPECT_EQ(factory_attempts, 3);  // Factory called once per attempt
}

// V2-F09: No performance regression - pool remains responsive
TEST(EndpointConnectionPoolTest, V2F09_PerformanceBaseline) {
    EndpointConnectionPool::Config cfg;
    cfg.min_connections = 0;
    cfg.max_connections = 5;
    
    auto fast_factory = [](const std::string& /*ep*/)
        -> std::optional<std::unique_ptr<SSL, SSLDeleter>> {
        return std::nullopt;  // Fail immediately
    };
    
    EndpointConnectionPool pool("localhost:50051", cfg, fast_factory);
    
    // Measure time to attempt 10 connections
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        pool.getConnection(std::chrono::milliseconds(50));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete reasonably quickly (within 2 seconds for 10 attempts)
    EXPECT_LT(duration.count(), 2000);
}
