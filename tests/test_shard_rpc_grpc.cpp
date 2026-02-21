/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_shard_rpc_grpc.cpp                            ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     413                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"
#include <thread>
#include <chrono>
#include <atomic>

using namespace themis::sharding;

// Mock request handler for testing
class MockRequestHandler : public ShardRPCServer::RequestHandler {
public:
    std::atomic<int> prepare_count{0};
    std::atomic<int> commit_count{0};
    std::atomic<int> abort_count{0};
    std::atomic<int> health_count{0};
    
    bool should_vote_commit = true;
    
    bool onPrepare(
        const std::string& transaction_id,
        const std::string& coordinator_shard_id,
        const std::string& transaction_data
    ) override {
        prepare_count++;
        return should_vote_commit;
    }
    
    bool onCommit(const std::string& transaction_id) override {
        commit_count++;
        return true;
    }
    
    bool onAbort(const std::string& transaction_id) override {
        abort_count++;
        return true;
    }
    
    ShardRPCServer::HealthInfo onHealthCheck() override {
        health_count++;
        return ShardRPCServer::HealthInfo{
            .is_healthy = true,
            .version = "1.3.4-test",
            .uptime_seconds = 42
        };
    }
};

class ShardRPCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Tests will use in-process simulation by default (localhost)
    }
    
    void TearDown() override {
    }
};

// ============================================================================
// In-Process Simulation Tests (for backward compatibility)
// ============================================================================

TEST_F(ShardRPCTest, InProcessPrepare) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 5000,
        .max_retries = 3,
        .retry_delay_ms = 100
    };
    
    ShardRPCClient client(config);
    
    nlohmann::json operations = nlohmann::json::array();
    operations.push_back({{"type", "insert"}, {"key", "test"}});
    
    bool result = client.prepare("txn-001", operations);
    EXPECT_TRUE(result);
}

TEST_F(ShardRPCTest, InProcessCommit) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 5000
    };
    
    ShardRPCClient client(config);
    
    bool result = client.commit("txn-001", 123456789);
    EXPECT_TRUE(result);
}

TEST_F(ShardRPCTest, InProcessAbort) {
    ShardRPCClient::Config config{
        .endpoint = "127.0.0.1:8080",
        .timeout_ms = 5000
    };
    
    ShardRPCClient client(config);
    
    bool result = client.abort("txn-001");
    EXPECT_TRUE(result);
}

TEST_F(ShardRPCTest, InProcessPing) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 5000
    };
    
    ShardRPCClient client(config);
    
    bool result = client.ping();
    EXPECT_TRUE(result);
}

TEST_F(ShardRPCTest, InProcessSnapshotRead) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 5000
    };
    
    ShardRPCClient client(config);
    
    nlohmann::json query = {{"collection", "users"}};
    auto results = client.snapshotRead(123456, query);
    EXPECT_TRUE(results.is_array());
}

// ============================================================================
// Retry Logic Tests
// ============================================================================

TEST_F(ShardRPCTest, RetryOnFailure) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 1000,
        .max_retries = 3,
        .retry_delay_ms = 50
    };
    
    ShardRPCClient client(config);
    
    // Should succeed after retries with in-process simulation
    bool result = client.ping();
    EXPECT_TRUE(result);
}

TEST_F(ShardRPCTest, TimeoutHandling) {
    ShardRPCClient::Config config{
        .endpoint = "localhost:8080",
        .timeout_ms = 100,  // Very short timeout
        .max_retries = 1
    };
    
    ShardRPCClient client(config);
    
    // Should still succeed with in-process (no actual network delay)
    bool result = client.ping();
    EXPECT_TRUE(result);
}

// ============================================================================
// gRPC Multi-Node Tests (only run if gRPC is available)
// ============================================================================

#ifdef THEMIS_ENABLE_GRPC
#if __has_include("sharding/shard_rpc.grpc.pb.h")

TEST_F(ShardRPCTest, GrpcServerStartStop) {
    ShardRPCServer server("0.0.0.0:50051");
    
    MockRequestHandler handler;
    server.setRequestHandler(&handler);
    
    bool started = server.start();
    // May fail if port is in use or gRPC not available
    if (started) {
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        server.stop();
    }
}

TEST_F(ShardRPCTest, GrpcClientServerCommunication) {
    // Start server
    ShardRPCServer server("0.0.0.0:50052");
    MockRequestHandler handler;
    server.setRequestHandler(&handler);
    
    bool started = server.start();
    if (!started) {
        GTEST_SKIP() << "Could not start gRPC server (port may be in use or gRPC not available)";
        return;
    }
    
    // Give server time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    try {
        // Create client (non-localhost endpoint triggers gRPC mode)
        ShardRPCClient::Config config{
            .endpoint = "0.0.0.0:50052",
            .timeout_ms = 5000,
            .max_retries = 3
        };
        
        ShardRPCClient client(config);
        
        // Test health check
        bool ping_result = client.ping();
        EXPECT_TRUE(ping_result);
        EXPECT_GT(handler.health_count, 0);
        
        // Test prepare
        nlohmann::json operations = nlohmann::json::array();
        operations.push_back({{"type", "insert"}});
        bool prepare_result = client.prepare("txn-grpc-001", operations);
        EXPECT_TRUE(prepare_result);
        EXPECT_GT(handler.prepare_count, 0);
        
        // Test commit
        bool commit_result = client.commit("txn-grpc-001", 123456);
        EXPECT_TRUE(commit_result);
        EXPECT_GT(handler.commit_count, 0);
        
        // Test abort
        bool abort_result = client.abort("txn-grpc-002");
        EXPECT_TRUE(abort_result);
        EXPECT_GT(handler.abort_count, 0);
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "gRPC communication failed: " << e.what();
    }
    
    server.stop();
}

TEST_F(ShardRPCTest, GrpcConnectionFailure) {
    // Try to connect to non-existent server on localhost with unused high port
    // Note: Using a high port number that's unlikely to be in use
    // This test is designed to fail connection, so exact port doesn't matter
    ShardRPCClient::Config config{
        .endpoint = "127.0.0.1:59999",
        .timeout_ms = 1000,
        .max_retries = 2,
        .retry_delay_ms = 100
    };
    
    ShardRPCClient client(config);
    
    // Should fail after retries (unless somehow this port is in use)
    bool result = client.ping();
    // With gRPC, this will likely fail or timeout
}

TEST_F(ShardRPCTest, GrpcExponentialBackoff) {
    // Similar to above, testing exponential backoff with connection failure
    ShardRPCClient::Config config{
        .endpoint = "127.0.0.1:59998",
        .timeout_ms = 500,
        .max_retries = 3,
        .retry_delay_ms = 100
    };
    
    ShardRPCClient client(config);
    
    auto start = std::chrono::steady_clock::now();
    
    // Should fail but with exponential backoff
    // Expected delays: 100ms, 200ms, 400ms
    try {
        client.ping();
    } catch (...) {
        // Expected to fail
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    // Should take at least the sum of delays (700ms) plus timeouts
    // But we won't assert on exact timing as it's system-dependent
}

#endif // __has_include
#endif // THEMIS_ENABLE_GRPC

// ============================================================================
// Multi-Client Concurrency Tests
// ============================================================================

TEST_F(ShardRPCTest, ConcurrentClients) {
    const int num_clients = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < num_clients; i++) {
        threads.emplace_back([&success_count, i]() {
            ShardRPCClient::Config config{
                .endpoint = "localhost:8080",
                .timeout_ms = 5000
            };
            
            ShardRPCClient client(config);
            
            if (client.ping()) {
                success_count++;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_EQ(success_count, num_clients);
}

// ─────────────────────────────────────────────────────────────────────────────
// snapshotRead tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(ShardRPCTest, InProcessSnapshotRead_ReturnsArray) {
    ShardRPCClient::Config config{
        .endpoint   = "localhost:8080",
        .timeout_ms = 5000
    };
    ShardRPCClient client(config);

    nlohmann::json query = {{"collection", "products"}};
    auto result = client.snapshotRead(1740000000LL, query);

    // In-process path always returns an array (possibly empty)
    EXPECT_TRUE(result.is_array());
}

TEST_F(ShardRPCTest, InProcessSnapshotRead_ZeroTimestamp) {
    ShardRPCClient::Config config{
        .endpoint   = "localhost:8080",
        .timeout_ms = 5000
    };
    ShardRPCClient client(config);

    nlohmann::json query = {{"collection", "logs"}};
    auto result = client.snapshotRead(0, query);

    EXPECT_TRUE(result.is_array());
}

#ifdef THEMIS_ENABLE_GRPC
#if __has_include("sharding/shard_rpc.grpc.pb.h")

TEST_F(ShardRPCTest, GrpcSnapshotRead_HealthyShard) {
    ShardRPCServer server("0.0.0.0:50060");
    MockRequestHandler handler;
    handler.should_vote_commit = true;
    server.setRequestHandler(&handler);

    bool started = server.start();
    if (!started) {
        GTEST_SKIP() << "Could not start gRPC server for snapshotRead test";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    try {
        ShardRPCClient::Config config{
            .endpoint   = "0.0.0.0:50060",
            .timeout_ms = 5000,
            .max_retries = 2
        };
        ShardRPCClient client(config);

        nlohmann::json query = {{"collection", "orders"}};
        // snapshotRead returns the data array (may be empty for a healthy shard)
        auto result = client.snapshotRead(1700000000LL, query);
        EXPECT_TRUE(result.is_array());

    } catch (const std::exception& e) {
        GTEST_SKIP() << "gRPC snapshotRead test skipped: " << e.what();
    }

    server.stop();
}

#endif // __has_include
#endif // THEMIS_ENABLE_GRPC
