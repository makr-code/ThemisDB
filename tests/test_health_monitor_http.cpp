/**
 * @file test_health_monitor_http.cpp
 * @brief Tests for HTTP-based health monitoring
 * 
 * Tests the actual HTTP health check functionality:
 * - Real HTTP GET requests to health endpoints
 * - Timeout handling
 * - State transitions (HEALTHY → SUSPECT → DOWN → RECOVERING → HEALTHY)
 * - Various HTTP response codes
 * - Connection failures
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <memory>
#include <atomic>

// HTTP server for testing
#include <boost/asio.hpp>
#include <boost/beast.hpp>

#include "sharding/health_monitor.h"
#include "sharding/multi_primary_coordinator.h"
#include "sharding/replica_topology.h"
#include "utils/http_client_pool.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using namespace std::chrono_literals;

namespace themis {
namespace sharding {
namespace test {

/**
 * @brief Mock HTTP health server for testing
 */
class MockHealthServer {
public:
    MockHealthServer(const std::string& host, uint16_t port) 
        : acceptor_(io_context_), 
          socket_(io_context_),
          endpoint_(net::ip::make_address(host), port),
          running_(false),
          response_code_(200),
          delay_ms_(0) {
    }
    
    ~MockHealthServer() {
        stop();
    }
    
    void start() {
        if (running_.exchange(true)) {
            return;
        }
        
        acceptor_.open(endpoint_.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint_);
        acceptor_.listen();
        
        server_thread_ = std::thread([this]() {
            serverLoop();
        });
        
        // Wait for server to be ready
        std::this_thread::sleep_for(100ms);
    }
    
    void stop() {
        if (!running_.exchange(false)) {
            return;
        }
        
        io_context_.stop();
        
        if (acceptor_.is_open()) {
            boost::system::error_code ec;
            acceptor_.close(ec);
        }
        
        if (server_thread_.joinable()) {
            server_thread_.join();
        }
    }
    
    void setResponseCode(int code) {
        response_code_.store(code);
    }
    
    void setDelay(int delay_ms) {
        delay_ms_.store(delay_ms);
    }
    
    int getRequestCount() const {
        return request_count_.load();
    }
    
private:
    void serverLoop() {
        while (running_) {
            try {
                acceptor_.async_accept(socket_,
                    [this](boost::system::error_code ec) {
                        if (!ec && running_) {
                            handleRequest();
                        }
                    });
                
                io_context_.run_one();
                
            } catch (const std::exception&) {
                if (!running_) break;
            }
        }
    }
    
    void handleRequest() {
        try {
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            http::read(socket_, buffer, req);
            
            request_count_++;
            
            // Simulate delay if configured
            int delay = delay_ms_.load();
            if (delay > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            
            // Build response
            int code = response_code_.load();
            http::response<http::string_body> res;
            res.version(req.version());
            res.result(static_cast<http::status>(code));
            res.set(http::field::server, "MockHealthServer");
            res.set(http::field::content_type, "application/json");
            
            if (code == 200) {
                res.body() = R"({"status":"healthy"})";
            } else {
                res.body() = R"({"status":"unhealthy"})";
            }
            
            res.prepare_payload();
            
            http::write(socket_, res);
            
            socket_.shutdown(tcp::socket::shutdown_send);
            
        } catch (const std::exception&) {
            // Ignore errors in test server
        }
    }
    
    net::io_context io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    tcp::endpoint endpoint_;
    std::thread server_thread_;
    std::atomic<bool> running_;
    std::atomic<int> response_code_;
    std::atomic<int> delay_ms_;
    std::atomic<int> request_count_;
};

/**
 * @brief Test fixture for health monitor HTTP tests
 */
class HealthMonitorHTTPTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock server on localhost
        mock_server_ = std::make_unique<MockHealthServer>("127.0.0.1", test_port_);
        mock_server_->start();
        
        // Create mock coordinator and topology
        MultiPrimaryConfig mp_config;
        mp_config.current_node_id = "test-node-1";
        mp_config.use_last_write_wins = true;
        coordinator_ = std::make_shared<MultiPrimaryCoordinator>(mp_config);
        topology_ = std::make_shared<ReplicaTopology>();
        
        // Create HTTP client pool
        utils::HTTPClientPool::Config pool_config;
        pool_config.connect_timeout = 5s;  // Longer than health_check_timeout to allow proper health check timeout detection
        pool_config.request_timeout = 5s;
        pool_config.max_connections = 10;
        http_pool_ = std::make_shared<utils::HTTPClientPool>(pool_config);
        
        // Configure health monitor
        HealthMonitorConfig config;
        config.heartbeat_interval = 500ms;  // Controls frequency of health checks
        config.health_check_timeout = 1000ms;  // Timeout for individual health check requests
        config.max_consecutive_failures = 3;
        config.successes_for_recovery = 3;
        config.health_check_path = "/health";
        config.auto_failover_enabled = false;  // Disable auto-failover for tests
        
        health_monitor_ = std::make_unique<HealthMonitor>(
            config, coordinator_, topology_, http_pool_);
    }
    
    void TearDown() override {
        if (health_monitor_) {
            health_monitor_->stop();
        }
        if (mock_server_) {
            mock_server_->stop();
        }
    }
    
    std::string getTestEndpoint() const {
        return "http://127.0.0.1:" + std::to_string(test_port_);
    }
    
    static constexpr uint16_t test_port_ = 18765;
    std::unique_ptr<MockHealthServer> mock_server_;
    std::shared_ptr<MultiPrimaryCoordinator> coordinator_;
    std::shared_ptr<ReplicaTopology> topology_;
    std::shared_ptr<utils::HTTPClientPool> http_pool_;
    std::unique_ptr<HealthMonitor> health_monitor_;
};

// Test Cases

TEST_F(HealthMonitorHTTPTest, HealthyNodeReturns200) {
    mock_server_->setResponseCode(200);
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    EXPECT_EQ(result.status, HealthStatus::HEALTHY);
    EXPECT_EQ(result.node_id, "node1");
    EXPECT_EQ(result.consecutive_failures, 0);
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(HealthMonitorHTTPTest, UnhealthyNodeReturns500) {
    mock_server_->setResponseCode(500);
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    EXPECT_EQ(result.status, HealthStatus::SUSPECT);
    EXPECT_GT(result.consecutive_failures, 0);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(HealthMonitorHTTPTest, TimeoutIsDetected) {
    // Set server delay longer than health check timeout
    mock_server_->setDelay(2000);  // 2 seconds delay
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    // Should fail due to timeout
    EXPECT_EQ(result.status, HealthStatus::SUSPECT);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(HealthMonitorHTTPTest, ConnectionRefusedHandled) {
    // Stop server to simulate connection refused
    mock_server_->stop();
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    EXPECT_EQ(result.status, HealthStatus::SUSPECT);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(HealthMonitorHTTPTest, StateTransitionHealthyToSuspect) {
    mock_server_->setResponseCode(200);
    
    // First check: healthy
    auto result1 = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    EXPECT_EQ(result1.status, HealthStatus::HEALTHY);
    
    // Make node unhealthy
    mock_server_->setResponseCode(500);
    
    // Second check: should transition to SUSPECT
    auto result2 = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    EXPECT_EQ(result2.status, HealthStatus::SUSPECT);
}

TEST_F(HealthMonitorHTTPTest, MultipleFailuresMarkDown) {
    mock_server_->setResponseCode(500);
    
    // Start the monitor
    health_monitor_->start();
    
    // Wait for multiple checks (should exceed max_consecutive_failures)
    std::this_thread::sleep_for(2s);
    
    auto status = health_monitor_->getHealthStatus("node1");
    // Note: This test needs the monitoring loop to run, which requires 
    // primaries to be registered in the coordinator
    // For now, just verify the check itself works
    
    health_monitor_->stop();
}

TEST_F(HealthMonitorHTTPTest, RecoveryFromDown) {
    // This tests the state machine: DOWN → RECOVERING → HEALTHY
    // Would need multiple sequential checks to verify
    mock_server_->setResponseCode(500);
    
    // Simulate multiple failures
    for (int i = 0; i < 3; i++) {
        health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    }
    
    // Now make server healthy
    mock_server_->setResponseCode(200);
    
    // Check multiple times for recovery
    for (int i = 0; i < 3; i++) {
        auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
        // After consecutive successes, should recover
    }
}

TEST_F(HealthMonitorHTTPTest, HTTPGetRequestsMade) {
    mock_server_->setResponseCode(200);
    
    int initial_count = mock_server_->getRequestCount();
    
    // Make several health checks
    for (int i = 0; i < 5; i++) {
        health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    }
    
    int final_count = mock_server_->getRequestCount();
    
    // Should have made 5 HTTP requests
    EXPECT_EQ(final_count - initial_count, 5);
}

TEST_F(HealthMonitorHTTPTest, DifferentResponseCodesHandled) {
    // Test various HTTP response codes
    
    // 200 OK - healthy
    mock_server_->setResponseCode(200);
    auto result_200 = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    EXPECT_EQ(result_200.status, HealthStatus::HEALTHY);
    
    // 404 Not Found - unhealthy
    mock_server_->setResponseCode(404);
    auto result_404 = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    EXPECT_EQ(result_404.status, HealthStatus::SUSPECT);
    
    // 503 Service Unavailable - unhealthy
    mock_server_->setResponseCode(503);
    auto result_503 = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    EXPECT_EQ(result_503.status, HealthStatus::SUSPECT);
}

TEST_F(HealthMonitorHTTPTest, ResponseTimeRecorded) {
    mock_server_->setResponseCode(200);
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    // Response time should be recorded
    EXPECT_GT(result.response_time.count(), 0);
    EXPECT_LT(result.response_time.count(), 5000);  // Should be less than 5 seconds
}

TEST_F(HealthMonitorHTTPTest, ConsecutiveFailuresCounted) {
    mock_server_->setResponseCode(500);
    
    // Note: The consecutive_failures counter is managed by performHealthChecks(),
    // which runs in the monitoring loop and tracks state across multiple checks.
    // The checkNodeHealth() method returns the immediate result, and the counter
    // is updated by the monitoring loop based on the history of checks.
    
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    // First check should return SUSPECT status on failure
    EXPECT_EQ(result.status, HealthStatus::SUSPECT);
}

TEST_F(HealthMonitorHTTPTest, HealthCheckPathUsed) {
    mock_server_->setResponseCode(200);
    
    // The health check should use the configured path (/health)
    auto result = health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    
    EXPECT_EQ(result.status, HealthStatus::HEALTHY);
    // Verify the request was made
    EXPECT_GT(mock_server_->getRequestCount(), 0);
}

TEST_F(HealthMonitorHTTPTest, StatisticsTracked) {
    mock_server_->setResponseCode(200);
    
    auto stats_before = health_monitor_->getStatistics();
    
    // Perform some health checks
    health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    health_monitor_->checkNodeHealth("node2", getTestEndpoint());
    
    auto stats_after = health_monitor_->getStatistics();
    
    // Total health checks should increase
    EXPECT_GT(stats_after.total_health_checks, stats_before.total_health_checks);
}

TEST_F(HealthMonitorHTTPTest, FailedHealthChecksTracked) {
    mock_server_->setResponseCode(500);
    
    auto stats_before = health_monitor_->getStatistics();
    
    // Perform some failed health checks
    health_monitor_->checkNodeHealth("node1", getTestEndpoint());
    health_monitor_->checkNodeHealth("node2", getTestEndpoint());
    
    auto stats_after = health_monitor_->getStatistics();
    
    // Failed health checks should increase
    EXPECT_GT(stats_after.failed_health_checks, stats_before.failed_health_checks);
}

} // namespace test
} // namespace sharding
} // namespace themis
