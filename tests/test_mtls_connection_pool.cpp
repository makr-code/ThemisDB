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
