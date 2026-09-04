// Unit tests for GRPCServer and GRPCPlugin lifecycle (src/rpc_grpc/grpc_plugin.h).
//
// These tests cover:
//   - GRPCPlugin metadata (name, version, capabilities)
//   - GRPCServer config/init without a live socket
//   - registerService null-guard
//   - Lifecycle: start (insecure), isRunning, stop
//   - Fail-closed TLS: bad cert paths must throw
//   - uptime_seconds advances while server is running
//
// The tests use insecure credentials on a random-port address so they work
// in CI without any TLS certificate files.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_GRPC

#include "rpc_grpc/grpc_plugin.h"
#include <chrono>
#include <thread>

using namespace themis::plugins::rpc;
using namespace themis::plugins::rpc::grpc_plugin;

// ─────────────────────────────────────────────────────────────────────────────
// GRPCPlugin metadata
// ─────────────────────────────────────────────────────────────────────────────

TEST(GRPCPluginTest, Name) {
    GRPCPlugin plugin;
    EXPECT_STREQ(plugin.getName(), "grpc");
}

TEST(GRPCPluginTest, Version) {
    GRPCPlugin plugin;
    EXPECT_NE(plugin.getVersion(), nullptr);
    EXPECT_GT(std::string(plugin.getVersion()).size(), 0u);
}

TEST(GRPCPluginTest, Capabilities) {
    GRPCPlugin plugin;
    auto caps = plugin.getCapabilities();
    EXPECT_TRUE(caps.supports_streaming);
    EXPECT_TRUE(caps.supports_batching);
    EXPECT_TRUE(caps.thread_safe);
    EXPECT_FALSE(caps.gpu_accelerated);
}

TEST(GRPCPluginTest, InitializeAndShutdown) {
    GRPCPlugin plugin;
    EXPECT_TRUE(plugin.initialize(nullptr));
    plugin.shutdown();
    // Second init after shutdown must succeed
    EXPECT_TRUE(plugin.initialize("{}"));
    plugin.shutdown();
}

TEST(GRPCPluginTest, CreateServerReturnsNonNull) {
    GRPCPlugin plugin;
    auto server = plugin.createServer();
    EXPECT_NE(server, nullptr);
}

TEST(GRPCPluginTest, Protocol) {
    GRPCPlugin plugin;
    EXPECT_EQ(plugin.getProtocol(), RPCProtocol::GRPC);
}

TEST(GRPCPluginTest, DefaultPort) {
    GRPCPlugin plugin;
    EXPECT_GT(plugin.getDefaultPort(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// GRPCServer configuration and pre-start state
// ─────────────────────────────────────────────────────────────────────────────

TEST(GRPCServerTest, ProtocolIsGRPC) {
    GRPCServer server;
    EXPECT_EQ(server.getProtocol(), RPCProtocol::GRPC);
}

TEST(GRPCServerTest, NotRunningBeforeStart) {
    GRPCServer server;
    EXPECT_FALSE(server.isRunning());
}

TEST(GRPCServerTest, InitializeSetsAddress) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59881;
    cfg.tls_enabled = false;

    EXPECT_TRUE(server.initialize(cfg));
    EXPECT_EQ(server.getAddress(), "127.0.0.1:59881");
}

TEST(GRPCServerTest, InitializeEmptyHostDefaultsZero) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.port = 0;
    cfg.tls_enabled = false;
    EXPECT_TRUE(server.initialize(cfg));
}

TEST(GRPCServerTest, RegisterNullServiceIsNoOp) {
    GRPCServer server;
    // Must not crash or throw when service_impl is nullptr
    EXPECT_NO_THROW(server.registerService(nullptr));
}

TEST(GRPCServerTest, StatsZeroBeforeStart) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59882;
    cfg.tls_enabled = false;
    server.initialize(cfg);

    auto stats = server.getStats();
    EXPECT_EQ(stats.total_requests, 0u);
    EXPECT_EQ(stats.successful_requests, 0u);
    EXPECT_EQ(stats.failed_requests, 0u);
    EXPECT_EQ(stats.uptime_seconds, 0u);
}

TEST(GRPCServerTest, ResetStatsWorks) {
    GRPCServer server;
    server.resetStats();
    auto stats = server.getStats();
    EXPECT_EQ(stats.total_requests, 0u);
    EXPECT_EQ(stats.uptime_seconds, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle: start (insecure), isRunning, stop
// ─────────────────────────────────────────────────────────────────────────────

TEST(GRPCServerTest, StartInsecureAndStop) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59883;   // fixed port – low collision risk in CI
    cfg.tls_enabled = false;
    cfg.auth_required = false;

    ASSERT_TRUE(server.initialize(cfg));
    ASSERT_TRUE(server.start());
    EXPECT_TRUE(server.isRunning());

    server.stop();
    EXPECT_FALSE(server.isRunning());
}

TEST(GRPCServerTest, StartAlreadyRunningReturnsFalse) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59884;
    cfg.tls_enabled = false;
    cfg.auth_required = false;

    server.initialize(cfg);
    ASSERT_TRUE(server.start());
    // Second start must return false
    EXPECT_FALSE(server.start());
    server.stop();
}

TEST(GRPCServerTest, UptimeAdvancesWhileRunning) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59885;
    cfg.tls_enabled = false;
    cfg.auth_required = false;

    server.initialize(cfg);
    ASSERT_TRUE(server.start());

    // Wait briefly so at least 1 second elapses
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto stats = server.getStats();
    EXPECT_GE(stats.uptime_seconds, 1u);

    server.stop();

    // After stop, uptime is no longer advancing (snapshot at 0)
    auto stopped_stats = server.getStats();
    EXPECT_EQ(stopped_stats.uptime_seconds, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Fail-closed TLS: bad cert path must throw (not fall back to insecure)
// ─────────────────────────────────────────────────────────────────────────────

TEST(GRPCServerTest, TLSWithBadCertPathThrowsOnStart) {
    GRPCServer server;
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59886;
    cfg.tls_enabled = true;
    cfg.tls_cert_path  = "/nonexistent/cert.pem";
    cfg.tls_key_path   = "/nonexistent/key.pem";
    cfg.tls_ca_cert_path = "/nonexistent/ca.pem";
    cfg.auth_required  = true;

    ASSERT_TRUE(server.initialize(cfg));
    // Fail-closed: start must either return false or throw – must not succeed
    bool started = false;
    try {
        started = server.start();
    } catch (const std::exception&) {
        started = false;
    }
    EXPECT_FALSE(started);
    if (server.isRunning()) {
      server.stop();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GRPCPlugin createServer integration
// ─────────────────────────────────────────────────────────────────────────────

TEST(GRPCPluginTest, CreatedServerCanStartAndStop) {
    GRPCPlugin plugin;
    plugin.initialize(nullptr);

    auto server = plugin.createServer();
    ASSERT_NE(server, nullptr);

    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 59887;
    cfg.tls_enabled = false;
    cfg.auth_required = false;

    ASSERT_TRUE(server->initialize(cfg));
    ASSERT_TRUE(server->start());
    EXPECT_TRUE(server->isRunning());
    server->stop();
    EXPECT_FALSE(server->isRunning());

    plugin.shutdown();
}

#else  // !THEMIS_ENABLE_GRPC

// Placeholder so the translation unit is never empty
TEST(GRPCPluginLifecycleTest, SkippedNoGRPC) {
    GTEST_SKIP() << "THEMIS_ENABLE_GRPC not set – gRPC plugin tests skipped";
}

#endif  // THEMIS_ENABLE_GRPC
