/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_grpc_plugin.cpp                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-14 07:14:45                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     257                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 63cde823d4  2026-04-08  Add unit tests for Ethics AI and RAG Context Engine plugins ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>

#include "rpc_grpc/grpc_plugin.h"
#include "plugins/rpc_plugin_interface.h"

using namespace themis::plugins;
using namespace themis::plugins::rpc;
using namespace themis::plugins::rpc::grpc_plugin;

// ============================================================================
// GRPCPlugin Interface Tests
// ============================================================================

class GRPCPluginTest : public ::testing::Test {
protected:
    GRPCPlugin plugin;
};

TEST_F(GRPCPluginTest, GetName) {
    EXPECT_STREQ("grpc", plugin.getName());
}

TEST_F(GRPCPluginTest, GetVersion) {
    EXPECT_STREQ("1.0.0", plugin.getVersion());
}

TEST_F(GRPCPluginTest, GetType) {
    EXPECT_EQ(PluginType::CUSTOM, plugin.getType());
}

TEST_F(GRPCPluginTest, GetCapabilities) {
    auto caps = plugin.getCapabilities();
    EXPECT_TRUE(caps.supports_streaming);
    EXPECT_TRUE(caps.supports_batching);
    EXPECT_TRUE(caps.thread_safe);
    EXPECT_FALSE(caps.supports_transactions);
    EXPECT_FALSE(caps.gpu_accelerated);
}

TEST_F(GRPCPluginTest, InitializeNullConfig) {
    EXPECT_TRUE(plugin.initialize(nullptr));
}

TEST_F(GRPCPluginTest, InitializeWithJsonConfig) {
    EXPECT_TRUE(plugin.initialize(R"({"host":"127.0.0.1","port":50051})"));
}

TEST_F(GRPCPluginTest, InitializeTwiceIsIdempotent) {
    EXPECT_TRUE(plugin.initialize(nullptr));
    EXPECT_TRUE(plugin.initialize(nullptr));
}

TEST_F(GRPCPluginTest, ShutdownAfterInitialize) {
    plugin.initialize(nullptr);
    EXPECT_NO_THROW(plugin.shutdown());
}

TEST_F(GRPCPluginTest, GetInstanceNonNull) {
    plugin.initialize(nullptr);
    EXPECT_NE(nullptr, plugin.getInstance());
}

TEST_F(GRPCPluginTest, GetInstanceIsThis) {
    plugin.initialize(nullptr);
    EXPECT_EQ(static_cast<void*>(&plugin), plugin.getInstance());
}

TEST_F(GRPCPluginTest, GetProtocolIsGRPC) {
    EXPECT_EQ(RPCProtocol::GRPC, plugin.getProtocol());
}

TEST_F(GRPCPluginTest, GetDefaultPortIs50051) {
    EXPECT_EQ(50051u, plugin.getDefaultPort());
}

TEST_F(GRPCPluginTest, GetProtocolDescriptionNonEmpty) {
    const char* desc = plugin.getProtocolDescription();
    ASSERT_NE(nullptr, desc);
    EXPECT_GT(std::strlen(desc), 0u);
}

TEST_F(GRPCPluginTest, GetProtocolDescriptionContainsGRPC) {
    std::string desc = plugin.getProtocolDescription();
    EXPECT_NE(std::string::npos, desc.find("gRPC"));
}

TEST_F(GRPCPluginTest, CreateServerReturnsNonNull) {
    auto server = plugin.createServer();
    EXPECT_NE(nullptr, server);
}

TEST_F(GRPCPluginTest, CreateServerMultipleTimes) {
    auto s1 = plugin.createServer();
    auto s2 = plugin.createServer();
    EXPECT_NE(nullptr, s1);
    EXPECT_NE(nullptr, s2);
    // Each call must return a distinct instance
    EXPECT_NE(s1.get(), s2.get());
}

// ============================================================================
// GRPCServer Lifecycle Tests
// ============================================================================

class GRPCServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<GRPCServer>();
    }

    RPCServerConfig makeInsecureConfig(uint16_t port = 50099) {
        RPCServerConfig cfg;
        cfg.host = "127.0.0.1";
        cfg.port = port;
        cfg.tls_enabled = false;
        cfg.auth_required = false;
        return cfg;
    }

    RPCServerConfig makeBadTLSConfig() {
        RPCServerConfig cfg;
        cfg.host = "127.0.0.1";
        cfg.port = 50100;
        cfg.tls_enabled = true;
        cfg.tls_cert_path = "/nonexistent/cert.pem";
        cfg.tls_key_path  = "/nonexistent/key.pem";
        cfg.tls_ca_cert_path = "/nonexistent/ca.pem";
        cfg.auth_required = false;
        return cfg;
    }

    std::unique_ptr<GRPCServer> server;
};

TEST_F(GRPCServerTest, InitialStateNotRunning) {
    EXPECT_FALSE(server->isRunning());
}

TEST_F(GRPCServerTest, GetProtocolIsGRPC) {
    EXPECT_EQ(RPCProtocol::GRPC, server->getProtocol());
}

TEST_F(GRPCServerTest, InitializeReturnsTrue) {
    auto cfg = makeInsecureConfig();
    EXPECT_TRUE(server->initialize(cfg));
}

TEST_F(GRPCServerTest, GetAddressAfterInitialize) {
    auto cfg = makeInsecureConfig(50099);
    server->initialize(cfg);
    EXPECT_EQ("127.0.0.1:50099", server->getAddress());
}

TEST_F(GRPCServerTest, GetAddressBeforeInitializeEmpty) {
    // No initialize call
    EXPECT_TRUE(server->getAddress().empty());
}

TEST_F(GRPCServerTest, GetStatsZeroedAfterInit) {
    auto cfg = makeInsecureConfig();
    server->initialize(cfg);
    auto stats = server->getStats();
    EXPECT_EQ(0u, stats.total_requests);
    EXPECT_EQ(0u, stats.successful_requests);
    EXPECT_EQ(0u, stats.failed_requests);
    EXPECT_EQ(0u, stats.active_connections);
}

TEST_F(GRPCServerTest, ResetStatsZeroesAll) {
    auto cfg = makeInsecureConfig();
    server->initialize(cfg);
    server->resetStats();
    auto stats = server->getStats();
    EXPECT_EQ(0u, stats.total_requests);
    EXPECT_EQ(0u, stats.uptime_seconds);
}

TEST_F(GRPCServerTest, RegisterNullServiceNoCrash) {
    EXPECT_NO_THROW(server->registerService(nullptr));
}

TEST_F(GRPCServerTest, IsNotRunningBeforeStart) {
    auto cfg = makeInsecureConfig();
    server->initialize(cfg);
    EXPECT_FALSE(server->isRunning());
}

TEST_F(GRPCServerTest, StartInsecureAndStop) {
    auto cfg = makeInsecureConfig(50097);
    ASSERT_TRUE(server->initialize(cfg));
    bool started = server->start();
    if (started) {
        EXPECT_TRUE(server->isRunning());
        server->stop();
        EXPECT_FALSE(server->isRunning());
    } else {
        // Port may be in use in CI; verify graceful failure
        EXPECT_FALSE(server->isRunning());
    }
}

TEST_F(GRPCServerTest, StartAlreadyRunningReturnsFalse) {
    auto cfg = makeInsecureConfig(50096);
    ASSERT_TRUE(server->initialize(cfg));
    bool first = server->start();
    if (first) {
        bool second = server->start();
        EXPECT_FALSE(second);
        server->stop();
    }
}

TEST_F(GRPCServerTest, StopWithoutStartNoCrash) {
    EXPECT_NO_THROW(server->stop());
}

TEST_F(GRPCServerTest, FailClosedTLSReturnsFalse) {
    // When TLS is enabled but cert files don't exist, start() must return false
    // (fail-closed: no insecure fallback).
    auto cfg = makeBadTLSConfig();
    ASSERT_TRUE(server->initialize(cfg));
    bool started = server->start();
    EXPECT_FALSE(started);
    EXPECT_FALSE(server->isRunning());
}

TEST_F(GRPCServerTest, DestructorStopsRunningServer) {
    auto cfg = makeInsecureConfig(50095);
    server->initialize(cfg);
    bool started = server->start();
    if (started) {
        // Destructor must not crash when server is still running
        EXPECT_NO_THROW(server.reset());
    }
}
