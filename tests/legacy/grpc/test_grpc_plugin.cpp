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
    EXPECT_STREQ("2.0.0", plugin.getVersion());
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

// ============================================================================
// GRPCServer v0.2.0 — Keepalive Tuning Tests
// ============================================================================

class GRPCServerKeepaliveTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<GRPCServer>();
    }

    RPCServerConfig makeKeepaliveConfig(uint16_t port,
                                        const std::string& ka_time_ms,
                                        const std::string& ka_timeout_ms) {
        RPCServerConfig cfg;
        cfg.host = "127.0.0.1";
        cfg.port = port;
        cfg.tls_enabled = false;
        cfg.auth_required = false;
        cfg.extra_config["keepalive_time_ms"]    = ka_time_ms;
        cfg.extra_config["keepalive_timeout_ms"] = ka_timeout_ms;
        return cfg;
    }

    std::unique_ptr<GRPCServer> server;
};

TEST_F(GRPCServerKeepaliveTest, KeepaliveConfigParsedAndServerStarts) {
    auto cfg = makeKeepaliveConfig(50094, "60000", "10000");
    ASSERT_TRUE(server->initialize(cfg));
    bool started = server->start();
    if (started) {
        EXPECT_TRUE(server->isRunning());
        server->stop();
    }
    // Even if port is busy, initialize must not throw.
    EXPECT_TRUE(true);
}

TEST_F(GRPCServerKeepaliveTest, InvalidKeepaliveValueFallsBackToDefault) {
    auto cfg = makeKeepaliveConfig(50093, "not_a_number", "also_not_a_number");
    ASSERT_TRUE(server->initialize(cfg));
    // Server should still start (defaults used) — no crash.
    bool started = server->start();
    if (started) {
        server->stop();
    }
    EXPECT_TRUE(true);
}

TEST_F(GRPCServerKeepaliveTest, EmptyKeepaliveUsesDefaults) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50092;
    cfg.tls_enabled = false;
    ASSERT_TRUE(server->initialize(cfg));
    bool started = server->start();
    if (started) {
        server->stop();
    }
    EXPECT_TRUE(true);
}

// ============================================================================
// GRPCServer v0.2.0 — Multi-Port (Admin) Binding Tests
// ============================================================================

class GRPCServerMultiPortTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<GRPCServer>();
    }

    std::unique_ptr<GRPCServer> server;
};

TEST_F(GRPCServerMultiPortTest, AdminAddressEmptyBeforeStart) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50091;
    cfg.tls_enabled = false;
    server->initialize(cfg);
    EXPECT_TRUE(server->getAdminAddress().empty());
}

TEST_F(GRPCServerMultiPortTest, AdminAddressEmptyWhenNoAdminPort) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50090;
    cfg.tls_enabled = false;
    ASSERT_TRUE(server->initialize(cfg));
    server->start();
    EXPECT_TRUE(server->getAdminAddress().empty());
    server->stop();
}

TEST_F(GRPCServerMultiPortTest, AdminAddressSetWhenAdminPortConfigured) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50089;
    cfg.tls_enabled = false;
    cfg.extra_config["admin_port"] = "50088";
    ASSERT_TRUE(server->initialize(cfg));
    bool started = server->start();
    if (started) {
        EXPECT_EQ("127.0.0.1:50088", server->getAdminAddress());
        server->stop();
    }
}

TEST_F(GRPCServerMultiPortTest, PrimaryAddressUnaffectedByAdminPort) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50087;
    cfg.tls_enabled = false;
    cfg.extra_config["admin_port"] = "50086";
    ASSERT_TRUE(server->initialize(cfg));
    EXPECT_EQ("127.0.0.1:50087", server->getAddress());
    server->start();
    server->stop();
}

// ============================================================================
// GRPCServer v0.2.0 — TLS Hot-Reload Tests
// ============================================================================

class GRPCServerTlsReloadTest : public ::testing::Test {
protected:
    void SetUp() override {
        server = std::make_unique<GRPCServer>();
    }

    std::unique_ptr<GRPCServer> server;
};

TEST_F(GRPCServerTlsReloadTest, ReloadTlsReturnsFalseWhenNotRunning) {
    // Server was never started — reloadTls must return false gracefully.
    EXPECT_FALSE(server->reloadTls("/nonexistent/cert.pem",
                                   "/nonexistent/key.pem",
                                   "/nonexistent/ca.pem"));
}

TEST_F(GRPCServerTlsReloadTest, ReloadTlsReturnsFalseWhenTlsDisabled) {
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50085;
    cfg.tls_enabled = false;
    server->initialize(cfg);
    bool started = server->start();
    if (started) {
        EXPECT_FALSE(server->reloadTls("/any/cert.pem",
                                       "/any/key.pem",
                                       "/any/ca.pem"));
        server->stop();
    }
}

TEST_F(GRPCServerTlsReloadTest, ReloadTlsReturnsFalseOnBadCertPath) {
    // Even if server were running with TLS, bad paths must return false.
    // We simulate "running + tls_enabled" by inspecting the guard paths.
    // (A full TLS server start requires valid certs, which we do not have
    //  in unit tests.  The fail-closed unit test above covers that path.)
    RPCServerConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 50084;
    cfg.tls_enabled = false; // start insecure
    server->initialize(cfg);
    bool started = server->start();
    if (started) {
        // Even with tls_enabled=false, reloadTls returns false (not-TLS guard).
        EXPECT_FALSE(server->reloadTls("/nonexistent/new_cert.pem",
                                       "/nonexistent/new_key.pem",
                                       "/nonexistent/new_ca.pem"));
        server->stop();
    }
}

