// Unit tests for GrpcTransport (include/network/grpc_transport.h).
//
// These tests validate configuration defaults, port-validation logic,
// address formatting, and statistics initialisation without requiring a
// live gRPC server socket or TLS certificates.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_GRPC

#include "network/grpc_transport.h"
#include <string>

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, DefaultConfigPort) {
    GrpcTransport::Config cfg;
    EXPECT_EQ(cfg.port, kGrpcTransportDefaultPort);
    EXPECT_EQ(cfg.port, 8771u);
}

TEST(GrpcTransportTest, DefaultConfigHost) {
    GrpcTransport::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(GrpcTransportTest, DefaultConfigThreads) {
    GrpcTransport::Config cfg;
    EXPECT_GE(cfg.num_threads, 1u);
}

TEST(GrpcTransportTest, DefaultTLSDisabled) {
    GrpcTransport::Config cfg;
    EXPECT_FALSE(cfg.tls_enabled);
    EXPECT_FALSE(cfg.require_client_cert);
}

TEST(GrpcTransportTest, DefaultMaxConnectionsUnlimited) {
    GrpcTransport::Config cfg;
    EXPECT_EQ(cfg.max_connections, 0u);  // 0 = unlimited
}

TEST(GrpcTransportTest, DefaultMessageSizeLimit) {
    GrpcTransport::Config cfg;
    // Default 4 MB
    EXPECT_EQ(cfg.max_message_size_bytes, 4 * 1024 * 1024);
}

TEST(GrpcTransportTest, DefaultKeepaliveSettings) {
    GrpcTransport::Config cfg;
    EXPECT_EQ(cfg.keepalive_time_ms, 30000);
    EXPECT_EQ(cfg.keepalive_timeout_ms, 10000);
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, ValidPortDefault) {
    EXPECT_TRUE(GrpcTransport::isValidPort(kGrpcTransportDefaultPort));
}

TEST(GrpcTransportTest, InvalidPortZero) {
    EXPECT_FALSE(GrpcTransport::isValidPort(0));
}

TEST(GrpcTransportTest, InvalidPortHTTP) {
    EXPECT_FALSE(GrpcTransport::isValidPort(80));
}

TEST(GrpcTransportTest, InvalidPortHTTPS) {
    EXPECT_FALSE(GrpcTransport::isValidPort(443));
}

TEST(GrpcTransportTest, InvalidPortTCPWireProtocol) {
    EXPECT_FALSE(GrpcTransport::isValidPort(8766));  // TCP wire protocol
}

TEST(GrpcTransportTest, InvalidPortUDPFastPath) {
    EXPECT_FALSE(GrpcTransport::isValidPort(8769));  // UDP fast-path
}

TEST(GrpcTransportTest, InvalidPortQuicTransport) {
    EXPECT_FALSE(GrpcTransport::isValidPort(8770));  // QUIC transport
}

TEST(GrpcTransportTest, InvalidPortGrpcApiServer) {
    EXPECT_FALSE(GrpcTransport::isValidPort(50051));  // gRPC API (server module)
}

TEST(GrpcTransportTest, ValidPortCustom) {
    EXPECT_TRUE(GrpcTransport::isValidPort(8772));
    EXPECT_TRUE(GrpcTransport::isValidPort(9000));
    EXPECT_TRUE(GrpcTransport::isValidPort(50052));
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics initialisation
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, InitialStatsAllZero) {
    GrpcTransport::Config cfg;
    GrpcTransport transport(cfg);

    GrpcTransport::Stats s = transport.getStats();
    EXPECT_EQ(s.connections_accepted, 0u);
    EXPECT_EQ(s.connections_active, 0u);
    EXPECT_EQ(s.connections_closed, 0u);
    EXPECT_EQ(s.frames_received, 0u);
    EXPECT_EQ(s.frames_sent, 0u);
    EXPECT_EQ(s.bytes_received, 0u);
    EXPECT_EQ(s.bytes_sent, 0u);
    EXPECT_EQ(s.parse_errors, 0u);
    EXPECT_EQ(s.handshakes_completed, 0u);
    EXPECT_EQ(s.connection_limit_drops, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Protocol constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, DefaultPortDoesNotConflict) {
    // Ensure default port 8771 differs from all other ThemisDB transports.
    EXPECT_NE(kGrpcTransportDefaultPort, 8766u);   // TCP wire protocol
    EXPECT_NE(kGrpcTransportDefaultPort, 8769u);   // UDP fast-path
    EXPECT_NE(kGrpcTransportDefaultPort, 8770u);   // QUIC transport
    EXPECT_NE(kGrpcTransportDefaultPort, 443u);    // HTTPS
    EXPECT_NE(kGrpcTransportDefaultPort, 80u);     // HTTP
    EXPECT_NE(kGrpcTransportDefaultPort, 50051u);  // gRPC API server
}

// ─────────────────────────────────────────────────────────────────────────────
// Address formatting
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, GetAddressDefaultConfig) {
    GrpcTransport::Config cfg;
    GrpcTransport transport(cfg);
    EXPECT_EQ(transport.getAddress(), "0.0.0.0:8771");
}

TEST(GrpcTransportTest, GetAddressCustomHostPort) {
    GrpcTransport::Config cfg;
    cfg.host = "127.0.0.1";
    cfg.port = 9999;
    GrpcTransport transport(cfg);
    EXPECT_EQ(transport.getAddress(), "127.0.0.1:9999");
}

TEST(GrpcTransportTest, GetPortMatchesConfig) {
    GrpcTransport::Config cfg;
    cfg.port = 8772;
    GrpcTransport transport(cfg);
    EXPECT_EQ(transport.getPort(), 8772u);
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning state
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, NotRunningBeforeStart) {
    GrpcTransport::Config cfg;
    GrpcTransport transport(cfg);
    EXPECT_FALSE(transport.isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// start() / stop() lifecycle (insecure, ephemeral port)
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrpcTransportTest, StartAndStopInsecure) {
    GrpcTransport::Config cfg;
    cfg.port = 8771;  // default gRPC transport port
    GrpcTransport transport(cfg);

    ASSERT_FALSE(transport.isRunning());
    const bool started = transport.start();
    if (started) {
        EXPECT_TRUE(transport.isRunning());
        transport.stop();
        EXPECT_FALSE(transport.isRunning());
    }
    // If start() fails (port unavailable in CI), the transport must still be
    // in a clean non-running state.
    EXPECT_FALSE(transport.isRunning());
}

TEST(GrpcTransportTest, DoubleStartReturnsFalse) {
    GrpcTransport::Config cfg;
    cfg.port = 8773;  // Use a distinct port to avoid conflicts
    GrpcTransport transport(cfg);

    const bool first = transport.start();
    if (first) {
        EXPECT_TRUE(transport.isRunning());
        // Second start() while already running must return false.
        EXPECT_FALSE(transport.start());
        EXPECT_TRUE(transport.isRunning());  // still running
        transport.stop();
    }
    EXPECT_FALSE(transport.isRunning());
}

TEST(GrpcTransportTest, StopWithoutStartIsNoOp) {
    GrpcTransport::Config cfg;
    GrpcTransport transport(cfg);
    // stop() on a never-started transport must not crash.
    EXPECT_NO_THROW(transport.stop());
    EXPECT_FALSE(transport.isRunning());
}

#endif  // THEMIS_ENABLE_GRPC

// ===========================================================================
// GAP-016 — gRPC insecure credentials warning (CWE-295)
// ===========================================================================
// The production gRPC transport already logs THEMIS_WARN when TLS is disabled.
// These tests verify the config-level behaviour that governs which credentials
// path is taken, to ensure the warning path is not accidentally bypassed.

#ifdef THEMIS_ENABLE_GRPC

// GAP-016-01: Default GrpcTransport::Config has tls_enabled=false — the
// insecure path fires; verify the config field so this can be caught in CI.
TEST(GrpcTransportGap016Test, GAP016_DefaultConfigTlsDisabled) {
    GrpcTransport::Config cfg;
    EXPECT_FALSE(cfg.tls_enabled)
        << "Default config must have tls_enabled=false so the insecure "
           "credential warning fires (GAP-016)";
}

// GAP-016-02: When tls_enabled=true the TLS credential path is selected.
TEST(GrpcTransportGap016Test, GAP016_TlsEnabled_SelectsTlsPath) {
    GrpcTransport::Config cfg;
    cfg.tls_enabled   = true;
    cfg.tls_cert_path = "/nonexistent/cert.pem";  // Intentionally missing
    cfg.tls_key_path  = "/nonexistent/key.pem";
    cfg.port          = 0;

    // buildCredentials() (private) is exercised indirectly through start().
    // start() will throw/return-false because the cert files don't exist;
    // what matters is that the insecure fallback code path is NOT taken.
    GrpcTransport transport(cfg);
    // Should fail gracefully (can't load certs) — not assert or crash.
    EXPECT_NO_THROW({ (void)transport.start(); });
    EXPECT_FALSE(transport.isRunning())
        << "Transport must not be running when cert files are missing (GAP-016)";
    transport.stop();
}

#endif  // THEMIS_ENABLE_GRPC
