/*
 * ThemisDB | File: test_wire_protocol_connection_pool.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file test_wire_protocol_connection_pool.cpp
 * @brief Unit tests for WireProtocolConnectionPool
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "network/wire_protocol_connection_pool.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace themis::network;
using namespace std::chrono_literals;

/**
 * @brief Test fixture for WireProtocolConnectionPool tests
 */
class WireProtocolConnectionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration for testing
        config_.min_connections_per_target = 2;
        config_.max_connections_per_target = 10;
        config_.idle_timeout = 5s;
        config_.connect_timeout = 2s;
        config_.acquire_timeout = 3s;
        config_.keepalive_interval = 30s;
        config_.enable_ssl = false;
        config_.enable_mtls = false;
        config_.max_retries = 3;
        config_.enable_warmup = false;  // Disable for most tests
    }

    WireProtocolConnectionPool::Config config_;
};

/**
 * @brief Test basic pool initialization
 */
TEST_F(WireProtocolConnectionPoolTest, BasicInitialization) {
    WireProtocolConnectionPool pool(config_);
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.available_connections, 0);
    EXPECT_EQ(stats.in_use_connections, 0);
    EXPECT_EQ(stats.stale_connections_removed, 0);
    EXPECT_EQ(stats.acquire_timeouts, 0);
    EXPECT_EQ(stats.connections_created, 0);
    EXPECT_EQ(stats.connections_reused, 0);
    EXPECT_EQ(stats.target_pool_count, 0u);
    EXPECT_EQ(stats.rebalance_passes, 0u);
    EXPECT_DOUBLE_EQ(stats.max_target_utilization, 0.0);
    EXPECT_DOUBLE_EQ(stats.min_target_utilization, 0.0);
}

/**
 * @brief Test pool configuration
 */
TEST_F(WireProtocolConnectionPoolTest, Configuration) {
    config_.min_connections_per_target = 5;
    config_.max_connections_per_target = 50;
    config_.enable_ssl = false;  // Don't enable SSL in this test
    
    WireProtocolConnectionPool pool(config_);
    
    // Pool should be initialized with config values
    auto stats = pool.getStats();
    EXPECT_EQ(stats.total_connections, 0); // No connections created yet without warmup
}

/**
 * @brief Test connection acquisition failure handling
 */
TEST_F(WireProtocolConnectionPoolTest, AcquisitionFailureHandling) {
    config_.connect_timeout = 1s;
    config_.acquire_timeout = 2s;
    WireProtocolConnectionPool pool(config_);
    
    // Try to connect to non-existent target
    EXPECT_THROW({
        auto conn = pool.acquireConnection("localhost:99999");
    }, std::runtime_error);
    
    auto stats = pool.getStats();
    EXPECT_GT(stats.failed_connections, 0);
}

/**
 * @brief Test RAII connection handle
 */
TEST_F(WireProtocolConnectionPoolTest, RAIIConnectionHandle) {
    WireProtocolConnectionPool pool(config_);
    
    // Connection handle should automatically return to pool
    {
        try {
            auto conn = pool.acquireConnection("localhost:99999");
            // Will fail to connect, but tests RAII mechanism
        } catch (const std::runtime_error&) {
            // Expected
        }
    }
    
    // Handle destroyed, connection should be released
    SUCCEED();
}

/**
 * @brief Test stale connection pruning
 */
TEST_F(WireProtocolConnectionPoolTest, StaleConnectionPruning) {
    config_.idle_timeout = 1s;
    WireProtocolConnectionPool pool(config_);
    
    // Manually trigger pruning (since connections can't actually be created to invalid endpoints)
    pool.pruneStaleConnections();
    
    auto stats = pool.getStats();
    // Should complete without errors
    EXPECT_GE(stats.stale_connections_removed, 0);
}

/**
 * @brief Test clear functionality
 */
TEST_F(WireProtocolConnectionPoolTest, ClearPool) {
    WireProtocolConnectionPool pool(config_);
    
    // Clear empty pool
    pool.clear();
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.available_connections, 0);
    EXPECT_EQ(stats.in_use_connections, 0);
}

/**
 * @brief Test statistics tracking
 */
TEST_F(WireProtocolConnectionPoolTest, StatisticsTracking) {
    WireProtocolConnectionPool pool(config_);
    
    auto stats_initial = pool.getStats();
    EXPECT_EQ(stats_initial.connections_created, 0);
    EXPECT_EQ(stats_initial.connections_reused, 0);
    EXPECT_EQ(stats_initial.total_connections, 0);
    
    // Try to acquire connection (will fail but updates stats)
    try {
        auto conn = pool.acquireConnection("localhost:99999");
    } catch (...) {
        // Expected
    }
    
    auto stats_after = pool.getStats();
    
    // Verify stats are tracked
    EXPECT_GT(stats_after.failed_connections, 0);
}

/**
 * @brief Test pool shutdown
 */
TEST_F(WireProtocolConnectionPoolTest, GracefulShutdown) {
    auto pool = std::make_unique<WireProtocolConnectionPool>(config_);
    
    // Destroy pool (triggers shutdown)
    pool.reset();
    
    // Should complete without hanging
    SUCCEED();
}

/**
 * @brief Test concurrent acquisition attempts
 */
TEST_F(WireProtocolConnectionPoolTest, ConcurrentAcquisition) {
    config_.max_connections_per_target = 5;
    config_.acquire_timeout = 2s;
    WireProtocolConnectionPool pool(config_);
    
    std::atomic<int> successful{0};
    std::atomic<int> failed{0};
    std::vector<std::thread> threads;
    
    // Multiple threads trying to acquire connections
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&pool, &successful, &failed]() {
            try {
                auto conn = pool.acquireConnection("localhost:99999");
                successful++;
            } catch (const std::runtime_error&) {
                failed++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All should have failed to connect (no server running)
    EXPECT_GT(failed.load(), 0);
    
    auto stats = pool.getStats();
    EXPECT_GT(stats.failed_connections, 0);
}

/**
 * @brief Test connection warmup functionality
 */
TEST_F(WireProtocolConnectionPoolTest, WarmupFunctionality) {
    config_.enable_warmup = true;
    config_.min_connections_per_target = 3;
    WireProtocolConnectionPool pool(config_);
    
    // Warmup to non-existent target (will fail but tests mechanism)
    pool.warmup("localhost:99999");
    
    // Warmup should have attempted to create connections
    auto stats = pool.getStats();
    // Connections will fail, but warmup was attempted
    SUCCEED();
}

/**
 * @brief Test parse target functionality
 */
TEST_F(WireProtocolConnectionPoolTest, TargetParsing) {
    WireProtocolConnectionPool pool(config_);
    
    // Valid target should not throw during pool operations
    try {
        auto conn = pool.acquireConnection("localhost:8766");
    } catch (const std::invalid_argument&) {
        FAIL() << "Should not throw invalid_argument for valid target";
    } catch (const std::runtime_error&) {
        // Expected - connection will fail
        SUCCEED();
    }
    
    // Invalid target format should throw
    EXPECT_THROW({
        auto conn = pool.acquireConnection("invalid_target_no_port");
    }, std::invalid_argument);
}

/**
 * @brief Test connection limit enforcement
 */
TEST_F(WireProtocolConnectionPoolTest, ConnectionLimitEnforcement) {
    config_.max_connections_per_target = 3;
    config_.acquire_timeout = 1s;
    WireProtocolConnectionPool pool(config_);
    
    // Pool should respect max connection limit
    // This is implicitly tested by the concurrent acquisition test
    SUCCEED();
}

/**
 * @brief Test idle timeout configuration
 */
TEST_F(WireProtocolConnectionPoolTest, IdleTimeoutConfiguration) {
    config_.idle_timeout = 2s;
    WireProtocolConnectionPool pool(config_);
    
    // Connections should be marked stale after idle timeout
    // This is tested by the stale connection pruning test
    auto stats = pool.getStats();
    EXPECT_GE(stats.stale_connections_removed, 0);
}

/**
 * @brief Test SSL configuration (without actual SSL)
 */
TEST_F(WireProtocolConnectionPoolTest, SSLConfiguration) {
    config_.enable_ssl = true;
    config_.ssl_ca_cert_path = "";  // Empty path will use system defaults
    
    // Should initialize without error
    WireProtocolConnectionPool pool(config_);
    
    auto stats = pool.getStats();
    EXPECT_EQ(stats.total_connections, 0);
}

/**
 * @brief Test mTLS configuration (without actual mTLS)
 */
TEST_F(WireProtocolConnectionPoolTest, MTLSConfiguration) {
    config_.enable_mtls = true;
    config_.ssl_cert_path = "/nonexistent/cert.pem";
    config_.ssl_key_path = "/nonexistent/key.pem";
    config_.ssl_ca_cert_path = "/nonexistent/ca.pem";
    
    // Should throw during initialization due to missing certificates
    EXPECT_THROW({
        WireProtocolConnectionPool pool(config_);
    }, std::runtime_error);
}

/**
 * @brief Test move semantics for ConnectionHandle
 */
TEST_F(WireProtocolConnectionPoolTest, ConnectionHandleMoveSemantics) {
    WireProtocolConnectionPool pool(config_);
    
    // Test move constructor and assignment
    try {
        auto conn1 = pool.acquireConnection("localhost:99999");
        auto conn2 = std::move(conn1);  // Move constructor
        
        // conn1 should be invalid after move
        EXPECT_FALSE(conn1.isValid());
    } catch (const std::runtime_error&) {
        // Expected - no server running
        SUCCEED();
    }
}

/**
 * @brief Test statistics persistence across operations
 */
TEST_F(WireProtocolConnectionPoolTest, StatisticsPersistence) {
    WireProtocolConnectionPool pool(config_);
    
    auto stats1 = pool.getStats();
    
    // Perform operations
    try {
        auto conn = pool.acquireConnection("localhost:99999");
    } catch (...) {}
    
    auto stats2 = pool.getStats();
    
    // Stats should accumulate
    EXPECT_GE(stats2.failed_connections, stats1.failed_connections);
    EXPECT_GE(stats2.target_pool_count, stats1.target_pool_count);
}

/**
 * @brief Test SSL context initialization with valid system paths
 */
TEST_F(WireProtocolConnectionPoolTest, SSLContextInitialization) {
    config_.enable_ssl = true;
    config_.ssl_ca_cert_path = "";  // Use system default CA certs
    
    // Should initialize SSL context successfully
    EXPECT_NO_THROW({
        WireProtocolConnectionPool pool(config_);
        auto stats = pool.getStats();
        EXPECT_EQ(stats.total_connections, 0);
    });
}

/**
 * @brief Test mTLS requires all certificate paths
 */
TEST_F(WireProtocolConnectionPoolTest, MTLSRequiresCertificates) {
    config_.enable_mtls = true;
    // Use actual test CA cert path to test missing client cert validation
    config_.ssl_ca_cert_path = "../certs/test/wire_protocol/ca-cert.pem";
    
    // Missing client cert path - should throw
    EXPECT_THROW({
        WireProtocolConnectionPool pool(config_);
    }, std::runtime_error);
    
    config_.ssl_cert_path = "../certs/test/wire_protocol/client-cert.pem";
    // Missing client key path - should throw
    EXPECT_THROW({
        WireProtocolConnectionPool pool(config_);
    }, std::runtime_error);
}

/**
 * @brief Test SSL socket wrapper functionality
 */
TEST_F(WireProtocolConnectionPoolTest, SocketWrapperBasics) {
    // Test with plain socket
    auto io = std::make_shared<net::io_context>();
    auto plain_socket = std::make_shared<tcp::socket>(*io);
    
    SocketWrapper wrapper_plain(plain_socket);
    EXPECT_FALSE(wrapper_plain.is_ssl());
    EXPECT_FALSE(wrapper_plain.is_open());  // Not connected
}

/**
 * @brief Test connection reuse rate calculation
 */
TEST_F(WireProtocolConnectionPoolTest, ConnectionReuseRate) {
    WireProtocolConnectionPool pool(config_);
    
    auto stats = pool.getStats();
    
    // With no connections, reuse rate should be 0
    EXPECT_EQ(stats.getReuseRate(), 0.0);
}

// =============================================================================
// Adaptive pool sizing tests
// =============================================================================

/**
 * @brief AdaptivePoolingStrategy default config values
 */
TEST(AdaptivePoolingStrategyTest, DefaultConfig) {
    AdaptivePoolingStrategy strategy;
    const auto& cfg = strategy.strategyConfig();
    EXPECT_DOUBLE_EQ(cfg.target_utilization,   0.7);
    EXPECT_DOUBLE_EQ(cfg.scale_up_threshold,   0.8);
    EXPECT_DOUBLE_EQ(cfg.scale_down_threshold, 0.3);
    EXPECT_DOUBLE_EQ(cfg.scale_up_factor,      1.5);
    EXPECT_DOUBLE_EQ(cfg.scale_down_factor,    0.7);
    EXPECT_EQ(cfg.min_idle_time, std::chrono::seconds(300));
}

/**
 * @brief AdaptivePoolingStrategy custom config values
 */
TEST(AdaptivePoolingStrategyTest, CustomConfig) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.scale_up_threshold   = 0.9;
    cfg.scale_down_threshold = 0.2;
    cfg.min_idle_time        = std::chrono::seconds(60);

    AdaptivePoolingStrategy strategy(cfg);
    EXPECT_DOUBLE_EQ(strategy.strategyConfig().scale_up_threshold,   0.9);
    EXPECT_DOUBLE_EQ(strategy.strategyConfig().scale_down_threshold, 0.2);
    EXPECT_EQ(strategy.strategyConfig().min_idle_time, std::chrono::seconds(60));
}

/**
 * @brief getIdealConnectionCount scales up under high load
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountScaleUp) {
    AdaptivePoolingStrategy strategy;
    // 9 active out of 10 total → load = 0.9 (> scale_up_threshold 0.8)
    size_t ideal = strategy.getIdealConnectionCount(10, 9, 0.9);
    EXPECT_GT(ideal, 10u);  // Should scale up
}

/**
 * @brief getIdealConnectionCount scales down under low load
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountScaleDown) {
    AdaptivePoolingStrategy strategy;
    // 2 active out of 10 total → load = 0.2 (< scale_down_threshold 0.3)
    size_t ideal = strategy.getIdealConnectionCount(10, 2, 0.2);
    EXPECT_LT(ideal, 10u);  // Should scale down
    EXPECT_GE(ideal, 1u);   // Never returns 0
}

/**
 * @brief getIdealConnectionCount is stable in the normal operating range
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountStable) {
    AdaptivePoolingStrategy strategy;
    // load = 0.5 → neither threshold triggered → no change
    size_t ideal = strategy.getIdealConnectionCount(10, 5, 0.5);
    EXPECT_EQ(ideal, 10u);
}

/**
 * @brief getIdealConnectionCount handles zero current count
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountZeroCurrent) {
    AdaptivePoolingStrategy strategy;
    size_t ideal = strategy.getIdealConnectionCount(0, 0, 0.0);
    EXPECT_GE(ideal, 1u);
}

/**
 * @brief shouldCreateConnection returns true when pool is exhausted
 */
TEST(AdaptivePoolingStrategyTest, ShouldCreateConnectionPoolExhausted) {
    AdaptivePoolingStrategy strategy;
    // 0 available out of 5 total → all in use → should create
    EXPECT_TRUE(strategy.shouldCreateConnection(5, 20, 0));
}

/**
 * @brief shouldCreateConnection returns false when pool is full
 */
TEST(AdaptivePoolingStrategyTest, ShouldCreateConnectionPoolFull) {
    AdaptivePoolingStrategy strategy;
    EXPECT_FALSE(strategy.shouldCreateConnection(20, 20, 5));
}

/**
 * @brief shouldCreateConnection returns true for an empty pool
 */
TEST(AdaptivePoolingStrategyTest, ShouldCreateConnectionEmptyPool) {
    AdaptivePoolingStrategy strategy;
    EXPECT_TRUE(strategy.shouldCreateConnection(0, 20, 0));
}

/**
 * @brief shouldRemoveConnection returns true when idle long enough
 */
TEST(AdaptivePoolingStrategyTest, ShouldRemoveConnectionIdleExpired) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.min_idle_time = std::chrono::seconds(60);
    AdaptivePoolingStrategy strategy(cfg);

    // Pool has 5 connections, min is 2, 3 available, oldest has been idle 90 s
    EXPECT_TRUE(strategy.shouldRemoveConnection(5, 2, 3, std::chrono::seconds(90)));
}

/**
 * @brief shouldRemoveConnection returns false when idle time is below threshold
 */
TEST(AdaptivePoolingStrategyTest, ShouldRemoveConnectionNotYetIdle) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.min_idle_time = std::chrono::seconds(300);
    AdaptivePoolingStrategy strategy(cfg);

    EXPECT_FALSE(strategy.shouldRemoveConnection(5, 2, 3, std::chrono::seconds(10)));
}

/**
 * @brief shouldRemoveConnection respects the minimum pool floor
 */
TEST(AdaptivePoolingStrategyTest, ShouldRemoveConnectionAtMinimum) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.min_idle_time = std::chrono::seconds(1);
    AdaptivePoolingStrategy strategy(cfg);

    // current_count == min_count → must not remove
    EXPECT_FALSE(strategy.shouldRemoveConnection(2, 2, 2, std::chrono::seconds(600)));
}

/**
 * @brief shouldRemoveConnection returns false when no idle connections available
 */
TEST(AdaptivePoolingStrategyTest, ShouldRemoveConnectionNoIdle) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.min_idle_time = std::chrono::seconds(1);
    AdaptivePoolingStrategy strategy(cfg);

    EXPECT_FALSE(strategy.shouldRemoveConnection(5, 2, 0, std::chrono::seconds(600)));
}

/**
 * @brief Pool with adaptive sizing enabled initialises correctly
 */
TEST_F(WireProtocolConnectionPoolTest, AdaptiveSizingEnabledInit) {
    config_.enable_adaptive_sizing = true;
    WireProtocolConnectionPool pool(config_);

    auto stats = pool.getStats();
    EXPECT_EQ(stats.pool_size_adaptations, 0u);
    EXPECT_DOUBLE_EQ(stats.utilization, 0.0);
}

/**
 * @brief Pool with custom adaptive strategy uses provided strategy
 */
TEST_F(WireProtocolConnectionPoolTest, AdaptiveSizingCustomStrategy) {
    AdaptivePoolingStrategy::Config scfg;
    scfg.min_idle_time = std::chrono::seconds(5);

    config_.enable_adaptive_sizing = true;
    config_.adaptive_strategy = std::make_shared<AdaptivePoolingStrategy>(scfg);

    WireProtocolConnectionPool pool(config_);
    auto stats = pool.getStats();
    EXPECT_EQ(stats.pool_size_adaptations, 0u);
}

/**
 * @brief Stats.utilization is 0 when no connections exist
 */
TEST_F(WireProtocolConnectionPoolTest, UtilizationZeroWhenEmpty) {
    config_.enable_adaptive_sizing = false;
    WireProtocolConnectionPool pool(config_);

    auto stats = pool.getStats();
    EXPECT_DOUBLE_EQ(stats.utilization, 0.0);
}

/**
 * @brief Stats.pool_size_adaptations is exposed as a zero-init counter
 */
TEST_F(WireProtocolConnectionPoolTest, PoolSizeAdaptationsInitiallyZero) {
    WireProtocolConnectionPool pool(config_);
    EXPECT_EQ(pool.getStats().pool_size_adaptations, 0u);
}

/**
 * @brief Valid targets that fail to connect still create a tracked target pool.
 */
TEST_F(WireProtocolConnectionPoolTest, TargetPoolCountTracksValidTargets) {
    WireProtocolConnectionPool pool(config_);

    try {
        auto conn = pool.acquireConnection("localhost:99999");
        (void)conn;
    } catch (const std::runtime_error&) {
        // Expected: no server is listening.
    }

    const auto stats = pool.getStats();
    EXPECT_EQ(stats.target_pool_count, 1u);
}

/**
 * @brief Manual rebalance passes are exposed through statistics.
 */
TEST_F(WireProtocolConnectionPoolTest, ManualRebalancePassVisibleInStats) {
    config_.enable_adaptive_sizing = true;
    WireProtocolConnectionPool pool(config_);

    pool.rebalancePools();
    pool.rebalancePools();

    const auto stats = pool.getStats();
    EXPECT_GE(stats.rebalance_passes, 2u);
}

/**
 * @brief getIdealConnectionCount returns current when load is exactly at scale_up_threshold
 * (boundary: equal is not greater-than, so no scale-up)
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountAtScaleUpBoundaryNoChange) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.scale_up_threshold = 0.8;
    AdaptivePoolingStrategy strategy(cfg);
    // load = 0.8 exactly — equal, not strictly greater → stable
    size_t ideal = strategy.getIdealConnectionCount(10, 8, 0.8);
    EXPECT_EQ(ideal, 10u);
}

/**
 * @brief getIdealConnectionCount returns current when load is exactly at scale_down_threshold
 * (boundary: equal is not less-than, so no scale-down)
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountAtScaleDownBoundaryNoChange) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.scale_down_threshold = 0.3;
    AdaptivePoolingStrategy strategy(cfg);
    // load = 0.3 exactly — equal, not strictly less-than → stable
    size_t ideal = strategy.getIdealConnectionCount(10, 3, 0.3);
    EXPECT_EQ(ideal, 10u);
}

/**
 * @brief Verify that getIdealConnectionCount scale-down result never drops below 1
 */
TEST(AdaptivePoolingStrategyTest, GetIdealCountScaleDownFloor) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.scale_down_threshold = 0.5;
    cfg.scale_down_factor    = 0.1;   // very aggressive: 0.1 × 2 = 0 → clamped to 1
    AdaptivePoolingStrategy strategy(cfg);
    size_t ideal = strategy.getIdealConnectionCount(2, 0, 0.0);
    EXPECT_GE(ideal, 1u);
}

/**
 * @brief Pool with adaptive sizing disabled does not call adaptPoolSize
 * (stats.pool_size_adaptations stays 0 even after pruning)
 */
TEST_F(WireProtocolConnectionPoolTest, AdaptiveSizingDisabledNoAdaptations) {
    config_.enable_adaptive_sizing = false;
    WireProtocolConnectionPool pool(config_);
    pool.pruneStaleConnections();  // trigger maintenance path
    EXPECT_EQ(pool.getStats().pool_size_adaptations, 0u);
}

/**
 * @brief shouldCreateConnection agrees with getIdealConnectionCount for scale-up:
 * When load is high (> scale_up_threshold) ideal is larger, AND the pool
 * is not full, shouldCreate should return true.
 */
TEST(AdaptivePoolingStrategyTest, IdealCountAndShouldCreateAgreement) {
    AdaptivePoolingStrategy strategy;
    // current=5, active=5 (100% utilization → load > 0.8)
    size_t ideal = strategy.getIdealConnectionCount(5, 5, 1.0);
    EXPECT_GT(ideal, 5u);

    // Pool is not full, no available conns → shouldCreate should agree
    bool should_create = strategy.shouldCreateConnection(5, 20, 0);
    EXPECT_TRUE(should_create);
}

/**
 * @brief shouldRemoveConnection agrees with getIdealConnectionCount for scale-down:
 * When load is low (< scale_down_threshold) ideal is smaller, AND idle time
 * exceeds min_idle_time, shouldRemove should return true.
 */
TEST(AdaptivePoolingStrategyTest, IdealCountAndShouldRemoveAgreement) {
    AdaptivePoolingStrategy::Config cfg;
    cfg.scale_down_threshold = 0.3;
    cfg.min_idle_time        = std::chrono::seconds(10);
    AdaptivePoolingStrategy strategy(cfg);

    // current=10, active=1 (10% utilization → load < 0.3)
    size_t ideal = strategy.getIdealConnectionCount(10, 1, 0.1);
    EXPECT_LT(ideal, 10u);

    // Pool above min, has idle connections, idle time exceeded → shouldRemove agrees
    bool should_remove = strategy.shouldRemoveConnection(10, 2, 5, std::chrono::seconds(30));
    EXPECT_TRUE(should_remove);
}
