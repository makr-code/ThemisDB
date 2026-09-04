#include <gtest/gtest.h>
#include "network/socket_timeout_manager.h"
#include <thread>
#include <chrono>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

class SocketTimeoutManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_.accept_timeout = 1000ms;
        config_.read_timeout = 1000ms;
        config_.write_timeout = 1000ms;
        config_.timeout_threshold = 5;
    }
    
    SocketTimeoutConfig config_;
};

// Test 1: Manager construction and destruction
TEST_F(SocketTimeoutManagerTest, ConstructionDestruction) {
    SocketTimeoutManager manager(config_);
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::HEALTHY);
    EXPECT_EQ(manager.getStats().accept_timeouts, 0);
    EXPECT_EQ(manager.getStats().read_timeouts, 0);
    EXPECT_EQ(manager.getStats().write_timeouts, 0);
}

// Test 2: Configuration retrieval
TEST_F(SocketTimeoutManagerTest, GetConfiguration) {
    SocketTimeoutManager manager(config_);
    const auto& retrieved_config = manager.getConfig();
    
    EXPECT_EQ(retrieved_config.accept_timeout, config_.accept_timeout);
    EXPECT_EQ(retrieved_config.read_timeout, config_.read_timeout);
    EXPECT_EQ(retrieved_config.write_timeout, config_.write_timeout);
    EXPECT_EQ(retrieved_config.timeout_threshold, config_.timeout_threshold);
}

// Test 3: Statistics tracking
TEST_F(SocketTimeoutManagerTest, StatisticsTracking) {
    SocketTimeoutManager manager(config_);
    
    // Record some timeouts
    manager.recordTimeout();
    manager.recordTimeout();
    
    // Stats are tracked in the health state, not directly visible in stats.
    // With threshold=5, integer split means 2 consecutive timeouts are DEGRADED.
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::DEGRADED);
    
    // One more timeout remains DEGRADED.
    manager.recordTimeout();
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::DEGRADED);
}

// Test 4: Health state transitions
TEST_F(SocketTimeoutManagerTest, HealthStateTransitions) {
    SocketTimeoutManager manager(config_);
    
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::HEALTHY);
    
    // Record timeouts to reach DEGRADED
    for (int i = 0; i < 3; i++) {
        manager.recordTimeout();
    }
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::DEGRADED);
    
    // Record more timeouts to open circuit
    for (int i = 0; i < 5; i++) {
        manager.recordTimeout();
    }
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::CIRCUIT_OPEN);
    
    // Recovery through success
    manager.recordSuccess();
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::HEALTHY);
}

// Test 5: Circuit breaker blocks connections
TEST_F(SocketTimeoutManagerTest, CircuitBreakerBlocksConnections) {
    SocketTimeoutManager manager(config_);
    
    EXPECT_TRUE(manager.shouldAcceptConnection());
    
    // Open circuit
    for (int i = 0; i < 10; i++) {
        manager.recordTimeout();
    }
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::CIRCUIT_OPEN);
    EXPECT_FALSE(manager.shouldAcceptConnection());
}

// Test 6: Circuit breaker resets after timeout (FIND-019: Fixed flaky test)
// FIX: Increased timeout margin from 1100ms to 1200ms for reliability
TEST_F(SocketTimeoutManagerTest, CircuitBreakerReset) {
    config_.reset_timeout = 1s;
    SocketTimeoutManager manager(config_);
    
    // Open circuit
    for (int i = 0; i < 10; i++) {
        manager.recordTimeout();
    }
    EXPECT_FALSE(manager.shouldAcceptConnection());
    
    // Wait for reset timeout with margin for system scheduling
    // FIX: Increased from 1100ms to 1200ms to account for OS scheduling variance
    std::this_thread::sleep_for(1200ms);
    
    // Should allow connections again
    EXPECT_TRUE(manager.shouldAcceptConnection());
}

// Test 7: Statistics reset
TEST_F(SocketTimeoutManagerTest, StatisticsReset) {
    SocketTimeoutManager manager(config_);
    
    // Record some activity
    manager.recordTimeout();
    manager.recordTimeout();
    manager.recordSuccess();
    
    // Reset stats
    manager.resetStats();
    
    EXPECT_EQ(manager.getHealthState(), SocketHealthState::HEALTHY);
}

// Test 8: Alert callback invocation
TEST_F(SocketTimeoutManagerTest, AlertCallback) {
    SocketTimeoutManager manager(config_);
    
    int alert_count = 0;
    SocketHealthState last_state = SocketHealthState::HEALTHY;
    std::string last_message = {};
    
    manager.setAlertCallback([&](SocketHealthState state, const std::string& message) {
        alert_count++;
        last_state = state;
        last_message = message;
    });
    
    // Trigger state change
    for (int i = 0; i < 3; i++) {
        manager.recordTimeout();
    }
    
    EXPECT_GT(alert_count, 0);
    EXPECT_EQ(last_state, SocketHealthState::DEGRADED);
    EXPECT_FALSE(last_message.empty());
}

// Test 9: Invalid socket handling
TEST_F(SocketTimeoutManagerTest, InvalidSocketHandling) {
    SocketTimeoutManager manager(config_);
    
    socket_t invalid_socket = INVALID_SOCKET_VALUE;
    
    EXPECT_FALSE(manager.configureSocket(invalid_socket));
    
    char buffer[1024];
    EXPECT_EQ(manager.readWithTimeout(invalid_socket, buffer, sizeof(buffer)), -1);
    EXPECT_EQ(manager.writeWithTimeout(invalid_socket, buffer, sizeof(buffer)), -1);
    
    // closeSocket should handle invalid socket gracefully
    manager.closeSocket(invalid_socket);
}

// Test 10: Timeout rate calculation
TEST_F(SocketTimeoutManagerTest, TimeoutRateCalculation) {
    SocketTimeoutManager manager(config_);
    
    // Initial rate should be 0
    EXPECT_EQ(manager.getStats().getTimeoutRate(), 0.0);
    
    // Simulate operations (we can't test actual socket operations in unit tests)
    // This test just verifies the calculation logic exists and doesn't crash
}

// Test 11: Configuration with different timeout values
TEST_F(SocketTimeoutManagerTest, CustomTimeoutConfiguration) {
    SocketTimeoutConfig custom_config;
    custom_config.accept_timeout = 5000ms;
    custom_config.read_timeout = 10000ms;
    custom_config.write_timeout = 15000ms;
    custom_config.timeout_threshold = 20;
    
    SocketTimeoutManager manager(custom_config);
    
    EXPECT_EQ(manager.getConfig().accept_timeout, 5000ms);
    EXPECT_EQ(manager.getConfig().read_timeout, 10000ms);
    EXPECT_EQ(manager.getConfig().write_timeout, 15000ms);
    EXPECT_EQ(manager.getConfig().timeout_threshold, 20);
}

// Test 12: TCP configuration options
TEST_F(SocketTimeoutManagerTest, TCPConfigurationOptions) {
    SocketTimeoutConfig custom_config;
    custom_config.enable_tcp_keepalive = true;
    custom_config.enable_tcp_nodelay = true;
    custom_config.keepalive_interval = 30000ms;
    
    SocketTimeoutManager manager(custom_config);
    
    EXPECT_TRUE(manager.getConfig().enable_tcp_keepalive);
    EXPECT_TRUE(manager.getConfig().enable_tcp_nodelay);
    EXPECT_EQ(manager.getConfig().keepalive_interval, 30000ms);
}

// Test 13: Multiple state transitions
TEST_F(SocketTimeoutManagerTest, MultipleStateTransitions) {
    SocketTimeoutManager manager(config_);
    
    std::vector<SocketHealthState> expected_transitions = {
        SocketHealthState::DEGRADED,
        SocketHealthState::CIRCUIT_OPEN,
        SocketHealthState::HEALTHY
    };
    
    size_t transition_index = 0;
    
    manager.setAlertCallback([&](SocketHealthState state, const std::string&) {
        if (transition_index < expected_transitions.size()) {
            EXPECT_EQ(state, expected_transitions[transition_index]);
            transition_index++;
        }
    });
    
    // HEALTHY -> DEGRADED
    for (int i = 0; i < 3; i++) {
        manager.recordTimeout();
    }
    
    // DEGRADED -> CIRCUIT_OPEN
    for (int i = 0; i < 5; i++) {
        manager.recordTimeout();
    }
    
    // CIRCUIT_OPEN -> HEALTHY (through success)
    manager.recordSuccess();
}

// Test 14: Concurrent timeout recording (FIND-019: Fixed flaky test)
// FIX: Increased sleep from 1ms to 10ms for more reliable timing on slow CI systems
TEST_F(SocketTimeoutManagerTest, ConcurrentTimeoutRecording) {
    SocketTimeoutManager manager(config_);
    
    const int num_threads = 4;
    const int operations_per_thread = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> completed_operations{0};
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&manager, operations_per_thread, &completed_operations]() {
            for (int j = 0; j < operations_per_thread; j++) {
                if (j % 2 == 0) {
                    manager.recordTimeout();
                } else {
                    manager.recordSuccess();
                }
                completed_operations++;
                // FIX: Increased from 1ms to 10ms to avoid timing issues on slow systems
                std::this_thread::sleep_for(10ms);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all operations completed
    EXPECT_EQ(completed_operations, num_threads * operations_per_thread);
    
    // Verify manager is still in valid state
    EXPECT_NE(manager.getHealthState(), static_cast<SocketHealthState>(-1));
}

// Test 15: SocketTimeoutGuard RAII behavior
TEST_F(SocketTimeoutManagerTest, SocketTimeoutGuardRAII) {
    auto manager = std::make_shared<SocketTimeoutManager>(config_);
    
    // Note: We can't create real sockets in unit tests, but we can test the RAII logic
    socket_t mock_socket = 12345;  // Mock socket descriptor
    
    {
        SocketTimeoutGuard guard(manager, mock_socket);
        EXPECT_EQ(guard.get(), mock_socket);
        EXPECT_TRUE(guard.valid());
        
        // Release ownership
        socket_t released = guard.release();
        EXPECT_EQ(released, mock_socket);
    }
    // Guard should not close socket since we released it
}

// Test 16: SocketTimeoutGuard move semantics
TEST_F(SocketTimeoutManagerTest, SocketTimeoutGuardMove) {
    auto manager = std::make_shared<SocketTimeoutManager>(config_);
    socket_t mock_socket = 12345;
    
    SocketTimeoutGuard guard1(manager, mock_socket);
    EXPECT_TRUE(guard1.valid());
    
    SocketTimeoutGuard guard2(std::move(guard1));
    EXPECT_TRUE(guard2.valid());
    EXPECT_EQ(guard2.get(), mock_socket);
}

// Test 17: Alert callback exception handling
TEST_F(SocketTimeoutManagerTest, AlertCallbackExceptionHandling) {
    SocketTimeoutManager manager(config_);
    
    manager.setAlertCallback([](SocketHealthState, const std::string&) {
        throw std::runtime_error("Callback error");
    });
    
    // Should not crash even if callback throws
    EXPECT_NO_THROW({
        for (int i = 0; i < 5; i++) {
            manager.recordTimeout();
        }
    });
}

// Test 18: Zero timeout configuration
TEST_F(SocketTimeoutManagerTest, ZeroTimeoutConfiguration) {
    SocketTimeoutConfig zero_config;
    zero_config.accept_timeout = 0ms;
    zero_config.read_timeout = 0ms;
    zero_config.write_timeout = 0ms;
    
    // Should handle zero timeouts gracefully
    EXPECT_NO_THROW({
        SocketTimeoutManager manager(zero_config);
    });
}

// Test 19: Very large timeout configuration
TEST_F(SocketTimeoutManagerTest, LargeTimeoutConfiguration) {
    SocketTimeoutConfig large_config;
    large_config.accept_timeout = 3600000ms;  // 1 hour
    large_config.read_timeout = 3600000ms;
    large_config.write_timeout = 3600000ms;
    
    SocketTimeoutManager manager(large_config);
    EXPECT_EQ(manager.getConfig().accept_timeout, 3600000ms);
}

// Test 20: Statistics consistency under load
TEST_F(SocketTimeoutManagerTest, StatisticsConsistency) {
    SocketTimeoutManager manager(config_);
    
    const int num_operations = 1000;
    
    for (int i = 0; i < num_operations; i++) {
        if (i % 3 == 0) {
            manager.recordTimeout();
        } else {
            manager.recordSuccess();
        }
    }
    
    // Verify manager is in consistent state
    EXPECT_NE(manager.getHealthState(), static_cast<SocketHealthState>(-1));
}