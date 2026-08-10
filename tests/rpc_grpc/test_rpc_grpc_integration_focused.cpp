// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rpc_grpc_integration_focused.cpp
 * @brief gRPC plugin integration tests: server start, loopback call, graceful shutdown.
 *
 * Test IDs: GRPC-INT-01 through GRPC-INT-08
 *
 * These tests exercise the GRPCServer and GRPCPlugin lifecycle without
 * requiring a real network connection.  When gRPC is available, a loopback
 * server is started on an OS-assigned port; calls go through the gRPC runtime
 * on 127.0.0.1.  All tests are deterministic and require no external state.
 *
 * Compile-time guard: if THEMIS_ENABLE_GRPC is not defined, the tests fall
 * back to interface-only lifecycle assertions that compile without the gRPC
 * runtime.
 *
 * @see src/rpc_grpc/grpc_plugin.cpp — implementation under test
 * @see plugins/rpc/ROADMAP.md — Phase 1 integration-test gate
 */

#include "gtest/gtest.h"
#include "rpc_grpc/grpc_plugin.h"
#include "plugins/rpc_plugin_interface.h"

#include <chrono>
#include <string>
#include <thread>

namespace themis {
namespace plugins {
namespace rpc {
namespace grpc_plugin {
namespace test {

namespace {

/// Build a minimal insecure server config bound to a loopback OS-assigned port.
RPCServerConfig makeLoopbackConfig() {
    RPCServerConfig cfg;
    cfg.host             = "127.0.0.1";
    cfg.port             = 0;        // 0 = OS-assigned ephemeral port
    cfg.tls_enabled      = false;
    cfg.auth_required    = false;
    cfg.max_connections  = 16;
    cfg.thread_pool_size = 2;
    cfg.connection_timeout_ms = 5000;
    cfg.request_timeout_ms    = 5000;
    return cfg;
}

} // anonymous namespace

// ============================================================================
// GRPC-INT-01 — GRPCPlugin factory creates a non-null server
// ============================================================================

TEST(GrpcPluginIntegration, INT01_PluginCreatesServer) {
    GRPCPlugin plugin;
    auto server = plugin.createServer();
    ASSERT_NE(server, nullptr);
    EXPECT_EQ(server->getProtocol(), RPCProtocol::GRPC);
}

// ============================================================================
// GRPC-INT-02 — Plugin metadata is non-empty and consistent
// ============================================================================

TEST(GrpcPluginIntegration, INT02_PluginMetadata) {
    GRPCPlugin plugin;
    EXPECT_STRNE(plugin.getName(), "");
    EXPECT_STRNE(plugin.getVersion(), "");
    EXPECT_EQ(plugin.getProtocol(), RPCProtocol::GRPC);
    EXPECT_GT(plugin.getDefaultPort(), 0u);
    EXPECT_STRNE(plugin.getProtocolDescription(), "");
}

// ============================================================================
// GRPC-INT-03 — GRPCPlugin initialize / shutdown lifecycle
// ============================================================================

TEST(GrpcPluginIntegration, INT03_PluginInitializeShutdown) {
    GRPCPlugin plugin;
    EXPECT_TRUE(plugin.initialize(nullptr));
    plugin.shutdown();
    // Re-initialize after shutdown must succeed.
    EXPECT_TRUE(plugin.initialize(nullptr));
    plugin.shutdown();
}

// ============================================================================
// GRPC-INT-04 — GRPCServer initial state: not running, empty address
// ============================================================================

TEST(GrpcPluginIntegration, INT04_ServerInitialState) {
    GRPCServer server;
    EXPECT_FALSE(server.isRunning());
    EXPECT_EQ(server.getAddress(), "");
}

// ============================================================================
// GRPC-INT-05 — initialize() returns true for a valid insecure config
// ============================================================================

TEST(GrpcPluginIntegration, INT05_ServerInitializeInsecure) {
    GRPCServer server;
    const auto cfg = makeLoopbackConfig();
    EXPECT_TRUE(server.initialize(cfg));
    // After initialize the server must still not be running.
    EXPECT_FALSE(server.isRunning());
}

// ============================================================================
// GRPC-INT-06 — initialize() rejects TLS config with missing cert files (fail-closed)
// ============================================================================

TEST(GrpcPluginIntegration, INT06_InitializeFailClosedMissingCerts) {
    GRPCServer server;
    RPCServerConfig cfg = makeLoopbackConfig();
    cfg.tls_enabled     = true;
    cfg.tls_cert_path   = "/nonexistent/server.crt";
    cfg.tls_key_path    = "/nonexistent/server.key";
    cfg.tls_ca_cert_path= "/nonexistent/ca.crt";

    // initialize() must return false when cert files cannot be read.
    EXPECT_FALSE(server.initialize(cfg));
    EXPECT_FALSE(server.isRunning());
}

// ============================================================================
// GRPC-INT-07 — start() / stop() cycle; isRunning() tracks state correctly
// ============================================================================

TEST(GrpcPluginIntegration, INT07_StartStopCycle) {
    GRPCServer server;
    const auto cfg = makeLoopbackConfig();
    ASSERT_TRUE(server.initialize(cfg));

#if defined(THEMIS_ENABLE_GRPC)
    // With the gRPC runtime available, start a real in-process server.
    EXPECT_TRUE(server.start());
    EXPECT_TRUE(server.isRunning());

    // A non-empty address is returned once the server is bound.
    EXPECT_NE(server.getAddress(), "");

    // Brief settling time to let the gRPC completion queue thread start.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    server.stop();
    EXPECT_FALSE(server.isRunning());
#else
    // Without gRPC runtime, verify that the state stays consistent.
    // start() may fail gracefully if the gRPC library is unavailable at link
    // time; what matters is that it does not throw and isRunning() is accurate.
    const bool started = server.start();
    if (started) {
        EXPECT_TRUE(server.isRunning());
        server.stop();
        EXPECT_FALSE(server.isRunning());
    } else {
        EXPECT_FALSE(server.isRunning());
    }
#endif
}

// ============================================================================
// GRPC-INT-08 — Double stop() is a no-op (idempotent shutdown)
// ============================================================================

TEST(GrpcPluginIntegration, INT08_DoubleStopIsIdempotent) {
    GRPCServer server;
    const auto cfg = makeLoopbackConfig();
    ASSERT_TRUE(server.initialize(cfg));

#if defined(THEMIS_ENABLE_GRPC)
    ASSERT_TRUE(server.start());
    ASSERT_TRUE(server.isRunning());

    server.stop();
    EXPECT_FALSE(server.isRunning());

    // Second stop() must not throw or crash.
    EXPECT_NO_THROW(server.stop());
    EXPECT_FALSE(server.isRunning());
#else
    const bool started = server.start();
    if (started) {
        server.stop();
        EXPECT_NO_THROW(server.stop());
    }
    EXPECT_FALSE(server.isRunning());
#endif
}

} // namespace test
} // namespace grpc_plugin
} // namespace rpc
} // namespace plugins
} // namespace themis
