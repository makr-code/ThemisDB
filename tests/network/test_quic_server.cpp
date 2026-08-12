// Unit tests for QUICServer and QUICClient (include/network/quic_server.h).
//
// These tests validate:
//   - QUICServer::Config default values
//   - QUICServer port validation (isValidPort)
//   - Congestion-control validation (isValidCongestionControl)
//   - QUICServer statistics initialisation (all counters = 0)
//   - QUICServer isRunning() state before start()
//   - Protocol constants (kQuicServerDefaultPort, kQuicServerVersion1, ALPN)
//   - QUICClient::Config defaults
//   - QUICClient URL parsing (QUICClient::parseUrl)
//   - QUICClient stream-ID assignment logic
//   - QUICClient connection state before connect()
//
// Tests are labelled QS-01 … QS-35 in comments for traceability.

#include <gtest/gtest.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#ifdef THEMIS_ENABLE_HTTP3

#include "network/quic_server.h"
#include <string>
#include <memory>
#include <sstream>

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// QS-01 … QS-10  QUICServer::Config defaults
// ─────────────────────────────────────────────────────────────────────────────

// QS-01
TEST(QUICServerTest, DefaultConfigPort) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.port, kQuicServerDefaultPort);
    EXPECT_EQ(cfg.port, 8769u);
}

// QS-02
TEST(QUICServerTest, DefaultConfigHost) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

// QS-03
TEST(QUICServerTest, DefaultConfigThreads) {
    QUICServer::Config cfg;
    EXPECT_GE(cfg.num_threads, 1u);
}

// QS-04
TEST(QUICServerTest, DefaultMaxStreamsPerConnection) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.max_streams_per_connection, 100u);
}

// QS-05
TEST(QUICServerTest, DefaultZeroRTTEnabled) {
    QUICServer::Config cfg;
    EXPECT_TRUE(cfg.enable_0rtt);
}

// QS-06
TEST(QUICServerTest, DefaultCongestionControlBBR) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.congestion_control, "bbr");
}

// QS-07
TEST(QUICServerTest, DefaultIdleTimeoutSec) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.max_idle_timeout_sec, 60u);
}

// QS-08
TEST(QUICServerTest, DefaultMaxConnectionsUnlimited) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.max_connections, 0u);  // 0 = unlimited
}

// QS-09
TEST(QUICServerTest, DefaultTlsPathsEmpty) {
    QUICServer::Config cfg;
    EXPECT_TRUE(cfg.tls_cert_path.empty());
    EXPECT_TRUE(cfg.tls_key_path.empty());
}

// QS-10
TEST(QUICServerTest, DefaultFlowControlWindows) {
    QUICServer::Config cfg;
    EXPECT_EQ(cfg.initial_max_data,              1024u * 1024u);  // 1 MB
    EXPECT_EQ(cfg.initial_max_stream_data_bidi,  256u  * 1024u);  // 256 KB
    EXPECT_EQ(cfg.initial_max_stream_data_uni,   256u  * 1024u);
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-11 … QS-17  QUICServer::isValidPort
// ─────────────────────────────────────────────────────────────────────────────

// QS-11
TEST(QUICServerTest, ValidPortDefault) {
    EXPECT_TRUE(QUICServer::isValidPort(kQuicServerDefaultPort));
    EXPECT_TRUE(QUICServer::isValidPort(8769u));
}

// QS-12
TEST(QUICServerTest, InvalidPortZero) {
    EXPECT_FALSE(QUICServer::isValidPort(0u));
}

// QS-13
TEST(QUICServerTest, InvalidPortHTTP) {
    EXPECT_FALSE(QUICServer::isValidPort(80u));
}

// QS-14
TEST(QUICServerTest, InvalidPortHTTPS) {
    EXPECT_FALSE(QUICServer::isValidPort(443u));
}

// QS-15
TEST(QUICServerTest, InvalidPortTCPWireProtocol) {
    EXPECT_FALSE(QUICServer::isValidPort(8766u));
}

// QS-16
TEST(QUICServerTest, InvalidPortQuicTransport) {
    EXPECT_FALSE(QUICServer::isValidPort(8770u));
}

// QS-17
TEST(QUICServerTest, ValidCustomPorts) {
    EXPECT_TRUE(QUICServer::isValidPort(8775u));
    EXPECT_TRUE(QUICServer::isValidPort(9000u));
    EXPECT_TRUE(QUICServer::isValidPort(1024u));
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-18 … QS-21  Congestion control validation
// ─────────────────────────────────────────────────────────────────────────────

// QS-18
TEST(QUICServerTest, CongestionControlBBRValid) {
    EXPECT_TRUE(QUICServer::isValidCongestionControl("bbr"));
}

// QS-19
TEST(QUICServerTest, CongestionControlCubicValid) {
    EXPECT_TRUE(QUICServer::isValidCongestionControl("cubic"));
}

// QS-20
TEST(QUICServerTest, CongestionControlCaseInsensitive) {
    EXPECT_TRUE(QUICServer::isValidCongestionControl("BBR"));
    EXPECT_TRUE(QUICServer::isValidCongestionControl("CUBIC"));
    EXPECT_TRUE(QUICServer::isValidCongestionControl("Bbr"));
}

// QS-21
TEST(QUICServerTest, CongestionControlInvalidRejected) {
    EXPECT_FALSE(QUICServer::isValidCongestionControl(""));
    EXPECT_FALSE(QUICServer::isValidCongestionControl("reno"));
    EXPECT_FALSE(QUICServer::isValidCongestionControl("vegas"));
    EXPECT_FALSE(QUICServer::isValidCongestionControl("unknown"));
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-22 … QS-24  Protocol constants
// ─────────────────────────────────────────────────────────────────────────────

// QS-22
TEST(QUICServerTest, DefaultPortConstant) {
    EXPECT_EQ(kQuicServerDefaultPort, 8769u);
}

// QS-23
TEST(QUICServerTest, QuicVersion1Constant) {
    // RFC 9000 defines QUIC version 1 as 0x00000001.
    EXPECT_EQ(kQuicServerVersion1, 0x00000001u);
}

// QS-24
TEST(QUICServerTest, AlpnConstant) {
    // HTTP/3 ALPN negotiation identifier.
    EXPECT_STREQ(kQuicServerAlpn, "h3");
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-25 … QS-27  QUICServer statistics initialisation
// ─────────────────────────────────────────────────────────────────────────────

// QS-25
TEST(QUICServerTest, InitialStatsAllZero) {
    QUICServer::Config cfg;
    QUICServer server(cfg);

    QUICServer::Stats s = server.getStats();
    EXPECT_EQ(s.total_connections,      0u);
    EXPECT_EQ(s.active_connections,     0u);
    EXPECT_EQ(s.total_streams,          0u);
    EXPECT_EQ(s.zero_rtt_accepted,      0u);
    EXPECT_EQ(s.zero_rtt_rejected,      0u);
    EXPECT_EQ(s.migrations,             0u);
    EXPECT_EQ(s.handshakes_completed,   0u);
    EXPECT_EQ(s.connection_limit_drops, 0u);
    EXPECT_EQ(s.packets_received,       0u);
    EXPECT_EQ(s.packets_sent,           0u);
    EXPECT_EQ(s.bytes_received,         0u);
    EXPECT_EQ(s.bytes_sent,             0u);
}

// QS-26
TEST(QUICServerTest, StatsStructHasZeroRTTFields) {
    QUICServer::Stats s{};
    // Confirm the Stats struct carries 0-RTT and migration metrics.
    EXPECT_EQ(s.zero_rtt_accepted, 0u);
    EXPECT_EQ(s.zero_rtt_rejected, 0u);
    EXPECT_EQ(s.migrations,        0u);
}

// QS-27
TEST(QUICServerTest, NotRunningBeforeStart) {
    QUICServer::Config cfg;
    QUICServer server(cfg);
    EXPECT_FALSE(server.isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-28 … QS-32  QUICClient::Config defaults and URL parsing
// ─────────────────────────────────────────────────────────────────────────────

// QS-28
TEST(QUICClientTest, DefaultConfigConnectTimeout) {
    QUICClient::Config cfg;
    EXPECT_EQ(cfg.connect_timeout.count(), 5);
}

// QS-29
TEST(QUICClientTest, DefaultConfigZeroRTTEnabled) {
    QUICClient::Config cfg;
    EXPECT_TRUE(cfg.enable_0rtt);
}

// QS-30
TEST(QUICClientTest, DefaultConfigVerifyTLS) {
    QUICClient::Config cfg;
    EXPECT_TRUE(cfg.verify_tls);
}

// QS-31
TEST(QUICClientTest, DefaultConfigCongestionControl) {
    QUICClient::Config cfg;
    EXPECT_EQ(cfg.congestion_control, "bbr");
}

// QS-32
TEST(QUICClientTest, NotConnectedBeforeConnect) {
    QUICClient client("quic://127.0.0.1:8769");
    EXPECT_FALSE(client.isConnected());
}

// ─────────────────────────────────────────────────────────────────────────────
// QS-33 … QS-35  QUICClient::parseUrl
// ─────────────────────────────────────────────────────────────────────────────

// QS-33
TEST(QUICClientTest, ParseUrlValid) {
    std::string host;
    uint16_t    port = 0;
    EXPECT_TRUE(QUICClient::parseUrl("quic://server.example.com:8769", host, port));
    EXPECT_EQ(host, "server.example.com");
    EXPECT_EQ(port, 8769u);
}

// QS-34
TEST(QUICClientTest, ParseUrlLocalhost) {
    std::string host;
    uint16_t    port = 0;
    EXPECT_TRUE(QUICClient::parseUrl("quic://127.0.0.1:8769", host, port));
    EXPECT_EQ(host, "127.0.0.1");
    EXPECT_EQ(port, 8769u);
}

// QS-35
TEST(QUICClientTest, ParseUrlInvalidScheme) {
    std::string host;
    uint16_t    port = 0;
    EXPECT_FALSE(QUICClient::parseUrl("http://server.example.com:8769", host, port));
    EXPECT_FALSE(QUICClient::parseUrl("", host, port));
    EXPECT_FALSE(QUICClient::parseUrl("quic://noport", host, port));
}

TEST(QUICClientTest, ParseUrlInvalidPortStringRejected) {
    std::string host;
    uint16_t    port = 0;
    EXPECT_FALSE(QUICClient::parseUrl("quic://server.example.com:not-a-port", host, port));
    EXPECT_FALSE(QUICClient::parseUrl("quic://server.example.com:", host, port));
}

TEST(QUICClientTest, ParseUrlOutOfRangePortRejected) {
    std::string host;
    uint16_t    port = 0;
    EXPECT_FALSE(QUICClient::parseUrl("quic://server.example.com:70000", host, port));
    EXPECT_FALSE(QUICClient::parseUrl("quic://server.example.com:999999999999", host, port));
}

// QS-36
TEST(QUICClientTest, VerifyTlsDisabledLogsVerifyNoneFallback) {
    auto previous_logger = spdlog::default_logger();
    auto previous_level = spdlog::get_level();

    std::ostringstream capture;
    auto sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(capture);
    auto logger = std::make_shared<spdlog::logger>("quic_tls_verify_none_test", sink);
    logger->set_pattern("%v");
    logger->set_level(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::warn);

    QUICClient::Config cfg;
    cfg.verify_tls = false;
    QUICClient client("quic://127.0.0.1:8769", cfg);

    EXPECT_NO_THROW(client.connect());
    client.disconnect();

    const auto logs = capture.str();
    EXPECT_NE(logs.find("verify_none fallback active"), std::string::npos);
    EXPECT_NE(logs.find("verify_tls=false"), std::string::npos);

    spdlog::set_default_logger(previous_logger);
    spdlog::set_level(previous_level);
}

TEST(QUICClientTest, InvalidCongestionControlRejectedBeforeConnect) {
    QUICClient::Config cfg;
    cfg.congestion_control = "reno";

    QUICClient client("quic://127.0.0.1:8769", cfg);
    EXPECT_THROW(client.connect(), std::runtime_error);
    EXPECT_FALSE(client.isConnected());
}

#endif  // THEMIS_ENABLE_HTTP3
