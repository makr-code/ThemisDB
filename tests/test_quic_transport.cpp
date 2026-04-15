/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_quic_transport.cpp                            ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:29:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     179                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c90319060b  2026-02-28  feat(network): QUIC/HTTP3 transport layer integration ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Unit tests for QuicTransport (include/network/quic_transport.h).
//
// These tests validate configuration defaults, port-validation logic,
// and statistics initialisation without requiring a live QUIC socket or
// TLS certificates.

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_HTTP3

#include "network/quic_transport.h"
#include <string>

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(QuicTransportTest, DefaultConfigPort) {
    QuicTransport::Config cfg;
    EXPECT_EQ(cfg.port, kQuicTransportDefaultPort);
    EXPECT_EQ(cfg.port, 8770u);
}

TEST(QuicTransportTest, DefaultConfigHost) {
    QuicTransport::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(QuicTransportTest, DefaultConfigThreads) {
    QuicTransport::Config cfg;
    EXPECT_GE(cfg.num_threads, 1u);
}

TEST(QuicTransportTest, DefaultIdleTimeout) {
    QuicTransport::Config cfg;
    // Default 30 s expressed in milliseconds
    EXPECT_EQ(cfg.max_idle_timeout_ms, 30000u);
}

TEST(QuicTransportTest, DefaultStreamLimits) {
    QuicTransport::Config cfg;
    EXPECT_EQ(cfg.max_streams_bidi, 100u);
    EXPECT_EQ(cfg.max_streams_uni, 3u);
}

TEST(QuicTransportTest, DefaultFlowControlWindows) {
    QuicTransport::Config cfg;
    // Connection window: 1 MB
    EXPECT_EQ(cfg.initial_max_data, 1024u * 1024u);
    // Stream window: 256 KB
    EXPECT_EQ(cfg.initial_max_stream_data_bidi, 256u * 1024u);
    EXPECT_EQ(cfg.initial_max_stream_data_uni, 256u * 1024u);
}

TEST(QuicTransportTest, DefaultZeroRTTEnabled) {
    QuicTransport::Config cfg;
    EXPECT_TRUE(cfg.enable_0rtt);
}

TEST(QuicTransportTest, DefaultMaxConnectionsUnlimited) {
    QuicTransport::Config cfg;
    EXPECT_EQ(cfg.max_connections, 0u);  // 0 = unlimited
}

// ─────────────────────────────────────────────────────────────────────────────
// Port validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(QuicTransportTest, ValidPortDefault) {
    EXPECT_TRUE(QuicTransport::isValidPort(kQuicTransportDefaultPort));
}

TEST(QuicTransportTest, InvalidPortZero) {
    EXPECT_FALSE(QuicTransport::isValidPort(0));
}

TEST(QuicTransportTest, InvalidPortHTTP) {
    EXPECT_FALSE(QuicTransport::isValidPort(80));
}

TEST(QuicTransportTest, InvalidPortHTTPS) {
    EXPECT_FALSE(QuicTransport::isValidPort(443));
}

TEST(QuicTransportTest, InvalidPortTCPWireProtocol) {
    EXPECT_FALSE(QuicTransport::isValidPort(8766));  // TCP wire protocol
}

TEST(QuicTransportTest, InvalidPortUDPFastPath) {
    EXPECT_FALSE(QuicTransport::isValidPort(8769));  // UDP fast-path
}

TEST(QuicTransportTest, ValidPortCustom) {
    EXPECT_TRUE(QuicTransport::isValidPort(8771));
    EXPECT_TRUE(QuicTransport::isValidPort(9000));
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics initialisation
// ─────────────────────────────────────────────────────────────────────────────

TEST(QuicTransportTest, InitialStatsAllZero) {
    // Construct without starting (no socket, no TLS) to inspect zero stats.
    QuicTransport::Config cfg;
    // Provide empty cert/key so createSslContext is not attempted on start()
    QuicTransport transport(cfg);

    QuicTransport::Stats s = transport.getStats();
    EXPECT_EQ(s.connections_accepted, 0u);
    EXPECT_EQ(s.connections_active, 0u);
    EXPECT_EQ(s.connections_closed, 0u);
    EXPECT_EQ(s.packets_received, 0u);
    EXPECT_EQ(s.packets_sent, 0u);
    EXPECT_EQ(s.bytes_received, 0u);
    EXPECT_EQ(s.bytes_sent, 0u);
    EXPECT_EQ(s.parse_errors, 0u);
    EXPECT_EQ(s.handshakes_completed, 0u);
    EXPECT_EQ(s.zero_rtt_accepted, 0u);
    EXPECT_EQ(s.migrations, 0u);
    EXPECT_EQ(s.connection_limit_drops, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Protocol constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(QuicTransportTest, QuicVersion1Constant) {
    // RFC 9000 defines QUIC version 1 as 0x00000001.
    EXPECT_EQ(kQuicVersion1, 0x00000001u);
}

TEST(QuicTransportTest, MinInitialMaxDataConstant) {
    EXPECT_EQ(kQuicMinInitialMaxData, 64u * 1024u);
}

TEST(QuicTransportTest, DefaultPortDoesNotConflict) {
    // Ensure default port 8770 differs from all other ThemisDB transports.
    EXPECT_NE(kQuicTransportDefaultPort, 8766u);  // TCP wire protocol
    EXPECT_NE(kQuicTransportDefaultPort, 8769u);  // UDP fast-path
    EXPECT_NE(kQuicTransportDefaultPort, 443u);   // HTTPS
    EXPECT_NE(kQuicTransportDefaultPort, 80u);    // HTTP
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning state
// ─────────────────────────────────────────────────────────────────────────────

TEST(QuicTransportTest, NotRunningBeforeStart) {
    QuicTransport::Config cfg;
    QuicTransport transport(cfg);
    EXPECT_FALSE(transport.isRunning());
}

#endif  // THEMIS_ENABLE_HTTP3
