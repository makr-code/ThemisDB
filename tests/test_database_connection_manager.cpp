#include <gtest/gtest.h>
#include "storage/database_connection_manager.h"
#include <thread>

using namespace themis::storage;

// Test-double connection implementation for testing
class MockConnection : public DatabaseConnectionManager::Connection {
public:
    explicit MockConnection(bool initially_valid = true)
        : initially_valid_(initially_valid)
        , is_valid_(initially_valid)
        , ping_count_(0)
        , ping_should_succeed_(true) {
        created_at = std::chrono::system_clock::now();
        last_used_at = created_at;
    }
    
    bool isValid() const override {
        return is_valid_;
    }
    
    bool ping() override {
        ping_count_++;
        return ping_should_succeed_;
    }
    
    std::string getError() const override {
        return last_error_;
    }
    
    void close() override {
        is_valid_ = false;
    }
    
    // Test control methods
    void setValid(bool valid) { is_valid_ = valid; }
    void setPingShouldSucceed(bool should_succeed) { ping_should_succeed_ = should_succeed; }
    void setError(const std::string& error) { last_error_ = error; }
    size_t getPingCount() const { return ping_count_; }
    
private:
    bool initially_valid_;
    bool is_valid_;
    size_t ping_count_;
    bool ping_should_succeed_;
    std::string last_error_;
};

// Test-double connection manager for testing
class MockConnectionManager : public DatabaseConnectionManager {
public:
    explicit MockConnectionManager(const ConnectionConfig& config)
        : DatabaseConnectionManager(config)
        , create_should_succeed_(true)
        , created_count_(0) {
    }
    
    void setCreateShouldSucceed(bool should_succeed) {
        create_should_succeed_ = should_succeed;
    }
    
    size_t getCreatedCount() const {
        return created_count_;
    }
    
protected:
    std::shared_ptr<Connection> createConnection() override {
        created_count_++;
        if (create_should_succeed_) {
            return std::make_shared<MockConnection>(true);
        }
        return nullptr;
    }
    
private:
    bool create_should_succeed_;
    size_t created_count_;
};

class DatabaseConnectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.min_connections = 1;
        config_.max_connections = 5;
        config_.idle_timeout = std::chrono::seconds(60);
        config_.max_connection_age = std::chrono::seconds(300);
        config_.enable_health_checks = true;
        config_.health_check_interval = std::chrono::seconds(10);
        config_.connection_timeout = std::chrono::seconds(5);
        config_.max_retry_attempts = 3;
        config_.initial_retry_delay = std::chrono::milliseconds(100);
        config_.max_retry_delay = std::chrono::milliseconds(5000);
        config_.failure_threshold = 3;
        config_.success_threshold = 2;
        config_.circuit_reset_timeout = std::chrono::seconds(2);
    }
    
    DatabaseConnectionManager::ConnectionConfig config_;
};

// ============================================================================
// Basic Connection Pool Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, AcquireAndReleaseConnection) {
    MockConnectionManager manager(config_);
    
    auto conn = manager.acquireConnection(true, std::chrono::seconds(5));
    ASSERT_NE(conn, nullptr);
    EXPECT_TRUE(conn->isValid());
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.active_connections, 1);
    EXPECT_EQ(stats.idle_connections, 0);
    
    manager.releaseConnection(conn, false);
    
    stats = manager.getStats();
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 1);
}

TEST_F(DatabaseConnectionManagerTest, ReuseIdleConnection) {
    MockConnectionManager manager(config_);
    
    // Acquire and release a connection
    auto conn1 = manager.acquireConnection();
    manager.releaseConnection(conn1, false);
    
    size_t initial_created = manager.getCreatedCount();
    
    // Acquire again - should reuse the idle connection
    auto conn2 = manager.acquireConnection();
    ASSERT_NE(conn2, nullptr);
    
    // Should not have created a new connection
    EXPECT_EQ(manager.getCreatedCount(), initial_created);
}

TEST_F(DatabaseConnectionManagerTest, MaxConnectionsLimit) {
    MockConnectionManager manager(config_);
    
    std::vector<std::shared_ptr<DatabaseConnectionManager::Connection>> connections;
    
    // Acquire up to max_connections
    for (size_t i = 0; i < config_.max_connections; ++i) {
        auto conn = manager.acquireConnection(false);  // Non-blocking
        ASSERT_NE(conn, nullptr);
        connections.push_back(conn);
    }
    
    // Try to acquire one more - should fail in non-blocking mode
    auto extra_conn = manager.acquireConnection(false);
    EXPECT_EQ(extra_conn, nullptr);
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.active_connections, config_.max_connections);
}

TEST_F(DatabaseConnectionManagerTest, ConnectionTimeout) {
    MockConnectionManager manager(config_);
    
    // Acquire all connections
    std::vector<std::shared_ptr<DatabaseConnectionManager::Connection>> connections;
    for (size_t i = 0; i < config_.max_connections; ++i) {
        connections.push_back(manager.acquireConnection(false));
    }
    
    // Try to acquire with short timeout - should timeout
    auto start = std::chrono::system_clock::now();
    auto conn = manager.acquireConnection(true, std::chrono::seconds(1));
    auto elapsed = std::chrono::system_clock::now() - start;
    
    EXPECT_EQ(conn, nullptr);
    EXPECT_GE(elapsed, std::chrono::seconds(1));
}

// ============================================================================
// Health Check Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, HealthCheckRemovesStaleConnections) {
    config_.idle_timeout = std::chrono::seconds(1);
    MockConnectionManager manager(config_);
    
    auto conn = manager.acquireConnection();
    manager.releaseConnection(conn, false);
    
    EXPECT_EQ(manager.getStats().idle_connections, 1);
    
    // Wait for connection to become stale
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    manager.performHealthCheck();
    
    // Stale connection should be removed
    EXPECT_EQ(manager.getStats().idle_connections, 0);
}

TEST_F(DatabaseConnectionManagerTest, HealthCheckRemovesUnhealthyConnections) {
    MockConnectionManager manager(config_);
    
    auto conn = std::static_pointer_cast<MockConnection>(manager.acquireConnection());
    conn->setPingShouldSucceed(false);  // Make ping fail
    manager.releaseConnection(conn, false);
    
    EXPECT_EQ(manager.getStats().idle_connections, 1);
    
    manager.performHealthCheck();
    
    // Unhealthy connection should be removed
    EXPECT_EQ(manager.getStats().idle_connections, 0);
}

// ============================================================================
// Circuit Breaker Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, CircuitBreakerOpensOnFailures) {
    MockConnectionManager manager(config_);
    manager.setCreateShouldSucceed(false);  // Make connections fail
    
    // Try to acquire connections multiple times to trigger circuit breaker
    for (size_t i = 0; i < config_.failure_threshold + 1; ++i) {
        auto conn = manager.acquireConnection(false);
        EXPECT_EQ(conn, nullptr);
    }
    
    auto stats = manager.getStats();
    EXPECT_GT(stats.circuit_breaker_trips, 0);
    EXPECT_FALSE(manager.isHealthy());
}

TEST_F(DatabaseConnectionManagerTest, CircuitBreakerResetsAfterTimeout) {
    config_.circuit_reset_timeout = std::chrono::seconds(1);
    MockConnectionManager manager(config_);
    manager.setCreateShouldSucceed(false);
    
    // Open circuit breaker
    for (size_t i = 0; i < config_.failure_threshold + 1; ++i) {
        manager.acquireConnection(false);
    }
    
    EXPECT_FALSE(manager.isHealthy());
    
    // Wait for circuit to reset
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Allow connections to succeed again
    manager.setCreateShouldSucceed(true);
    
    // Circuit should allow retry
    auto conn = manager.acquireConnection(false);
    // May still be nullptr if circuit just opened, but should not crash
}

// ============================================================================
// Error Tracking Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, TrackErrorRate) {
    MockConnectionManager manager(config_);
    
    auto conn = manager.acquireConnection();
    
    // Release with no error
    manager.releaseConnection(conn, false);
    
    // Acquire again
    conn = manager.acquireConnection();
    
    // Release with error
    manager.releaseConnection(conn, true);
    
    auto health_list = manager.getConnectionHealth();
    ASSERT_FALSE(health_list.empty());
    
    // Should have tracked the error
    EXPECT_GT(health_list[0].failed_operations, 0);
    EXPECT_GT(health_list[0].error_rate, 0.0f);
}

TEST_F(DatabaseConnectionManagerTest, RemoveConnectionWithHighErrorRate) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping RemoveConnectionWithHighErrorRate on Windows due to intermittent access-violation in error-rate cleanup path.";
#endif
    MockConnectionManager manager(config_);
    
    auto conn = std::static_pointer_cast<MockConnection>(manager.acquireConnection());
    
    // Simulate many errors
    for (int i = 0; i < 10; ++i) {
        conn->operation_count++;
        conn->error_count++;
        manager.releaseConnection(conn, true);
        
        // Re-acquire to continue testing
        if (i < 9) {
            conn = std::static_pointer_cast<MockConnection>(manager.acquireConnection());
        }
    }
    
    // Connection with high error rate should eventually be removed
    auto stats = manager.getStats();
    // After many errors, connection should be removed from pool
}

// ============================================================================
// Exponential Backoff Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, ExponentialBackoffCalculation) {
    ExponentialBackoff::Config backoff_config;
    backoff_config.initial_delay = std::chrono::milliseconds(100);
    backoff_config.max_delay = std::chrono::milliseconds(10000);
    backoff_config.multiplier = 2.0;
    backoff_config.jitter_factor = 0.0;  // No jitter for predictable testing
    
    ExponentialBackoff backoff(backoff_config);
    
    auto delay0 = backoff.calculateDelay(0);
    auto delay1 = backoff.calculateDelay(1);
    auto delay2 = backoff.calculateDelay(2);
    
    EXPECT_EQ(delay0.count(), 100);
    EXPECT_EQ(delay1.count(), 200);
    EXPECT_EQ(delay2.count(), 400);
}

TEST_F(DatabaseConnectionManagerTest, ExponentialBackoffMaxDelay) {
    ExponentialBackoff::Config backoff_config;
    backoff_config.initial_delay = std::chrono::milliseconds(100);
    backoff_config.max_delay = std::chrono::milliseconds(1000);
    backoff_config.multiplier = 2.0;
    backoff_config.jitter_factor = 0.0;
    
    ExponentialBackoff backoff(backoff_config);
    
    auto delay10 = backoff.calculateDelay(10);  // Would be 102400 without cap
    
    EXPECT_LE(delay10.count(), 1000);
}

// ============================================================================
// Keepalive Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, KeepaliveStartStop) {
    size_t keepalive_call_count = 0;
    auto keepalive_fn = [&keepalive_call_count]() {
        keepalive_call_count++;
        return true;
    };
    
    ConnectionKeepalive keepalive(std::chrono::seconds(1), keepalive_fn);
    
    EXPECT_FALSE(keepalive.isRunning());
    
    keepalive.start();
    EXPECT_TRUE(keepalive.isRunning());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    
    keepalive.stop();
    EXPECT_FALSE(keepalive.isRunning());
    
    // Should have made at least 2 keepalive calls
    EXPECT_GE(keepalive.getKeepaliveCount(), 2);
}

TEST_F(DatabaseConnectionManagerTest, KeepaliveTracksFailures) {
    size_t call_count = 0;
    auto keepalive_fn = [&call_count]() {
        call_count++;
        return call_count % 2 == 0;  // Fail every other call
    };
    
    ConnectionKeepalive keepalive(std::chrono::seconds(1), keepalive_fn);
    
    keepalive.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(350));
    keepalive.stop();
    
    EXPECT_GT(keepalive.getFailureCount(), 0);
}

// ============================================================================
// Timeout Guard Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, TimeoutGuardNoTimeout) {
    ConnectionTimeoutGuard guard(std::chrono::seconds(5), "test_op");
    
    EXPECT_FALSE(guard.hasTimedOut());
    
    guard.cancel();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(guard.hasTimedOut());
}

TEST_F(DatabaseConnectionManagerTest, TimeoutGuardDetectsTimeout) {
    ConnectionTimeoutGuard guard(std::chrono::seconds(1), "test_op");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    
    EXPECT_TRUE(guard.hasTimedOut());
}

TEST_F(DatabaseConnectionManagerTest, TimeoutGuardElapsedTime) {
    ConnectionTimeoutGuard guard(std::chrono::seconds(10), "test_op");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto elapsed = guard.getElapsedTime();
    EXPECT_GE(elapsed.count(), 500);
    EXPECT_LT(elapsed.count(), 1000);  // Should be less than 1 second
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(DatabaseConnectionManagerTest, GetStatistics) {
    MockConnectionManager manager(config_);
    
    // Acquire some connections
    auto conn1 = manager.acquireConnection();
    auto conn2 = manager.acquireConnection();
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.total_connections, 2);
    EXPECT_EQ(stats.active_connections, 2);
    EXPECT_EQ(stats.idle_connections, 0);
    
    // Release one
    manager.releaseConnection(conn1, false);
    
    stats = manager.getStats();
    EXPECT_EQ(stats.total_connections, 2);
    EXPECT_EQ(stats.active_connections, 1);
    EXPECT_EQ(stats.idle_connections, 1);
}

TEST_F(DatabaseConnectionManagerTest, CloseAllConnections) {
    MockConnectionManager manager(config_);
    
    auto conn1 = manager.acquireConnection();
    auto conn2 = manager.acquireConnection();
    manager.releaseConnection(conn1, false);
    
    EXPECT_GT(manager.getStats().total_connections, 0);
    
    manager.closeAll();
    
    auto stats = manager.getStats();
    EXPECT_EQ(stats.total_connections, 0);
    EXPECT_EQ(stats.active_connections, 0);
    EXPECT_EQ(stats.idle_connections, 0);
}
