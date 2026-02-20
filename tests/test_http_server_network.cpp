/**
 * @file test_http_server_network.cpp
 * @brief Tests for HttpServer network stack configuration and defaults
 *
 * Validates the network configuration and feature flags of HttpServer::Config
 * introduced/hardened as part of the Server Production Readiness initiative.
 * Tests do not require a running server; they validate configuration semantics.
 */

#include <gtest/gtest.h>
#include "server/http_server.h"

using Config = themis::server::HttpServer::Config;

// ---------------------------------------------------------------------------
// Default Network Configuration
// ---------------------------------------------------------------------------

TEST(HttpServerNetworkConfig, DefaultHostIsAllInterfaces) {
    Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(HttpServerNetworkConfig, DefaultPortIs8080) {
    Config cfg;
    EXPECT_EQ(cfg.port, 8080);
}

TEST(HttpServerNetworkConfig, DefaultThreadCountIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.num_threads, 0u);
}

TEST(HttpServerNetworkConfig, ThreadCountCanBeOverridden) {
    Config cfg;
    cfg.num_threads = 4;
    EXPECT_EQ(cfg.num_threads, 4u);
}

TEST(HttpServerNetworkConfig, HostCanBeOverridden) {
    Config cfg;
    cfg.host = "127.0.0.1";
    EXPECT_EQ(cfg.host, "127.0.0.1");
}

TEST(HttpServerNetworkConfig, PortCanBeOverridden) {
    Config cfg;
    cfg.port = 9090;
    EXPECT_EQ(cfg.port, 9090);
}

// ---------------------------------------------------------------------------
// TLS / SSL Configuration
// ---------------------------------------------------------------------------

TEST(HttpServerTlsConfig, TlsDisabledByDefault) {
    Config cfg;
    EXPECT_FALSE(cfg.enable_tls);
}

TEST(HttpServerTlsConfig, DefaultMinVersionIsTls13) {
    Config cfg;
    EXPECT_EQ(cfg.tls_min_version, "TLSv1.3");
}

TEST(HttpServerTlsConfig, TlsCanBeEnabled) {
    Config cfg;
    cfg.enable_tls = true;
    cfg.tls_cert_path = "/path/to/cert.pem";
    cfg.tls_key_path = "/path/to/key.pem";
    EXPECT_TRUE(cfg.enable_tls);
    EXPECT_FALSE(cfg.tls_cert_path.empty());
    EXPECT_FALSE(cfg.tls_key_path.empty());
}

TEST(HttpServerTlsConfig, MutualTlsDisabledByDefault) {
    Config cfg;
    EXPECT_FALSE(cfg.tls_require_client_cert);
}

TEST(HttpServerTlsConfig, MutualTlsCanBeEnabled) {
    Config cfg;
    cfg.tls_require_client_cert = true;
    cfg.tls_ca_cert_path = "/path/to/ca.pem";
    EXPECT_TRUE(cfg.tls_require_client_cert);
    EXPECT_FALSE(cfg.tls_ca_cert_path.empty());
}

TEST(HttpServerTlsConfig, CipherListDefaultIsEmpty) {
    Config cfg;
    // Empty cipher list means server uses secure defaults
    EXPECT_TRUE(cfg.tls_cipher_list.empty());
}

TEST(HttpServerTlsConfig, MinVersionCanBeSetToTls12) {
    Config cfg;
    cfg.tls_min_version = "TLSv1.2";
    EXPECT_EQ(cfg.tls_min_version, "TLSv1.2");
}

// ---------------------------------------------------------------------------
// Connection Limits
// ---------------------------------------------------------------------------

TEST(HttpServerConnectionConfig, MaxRequestSizeMbIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.max_request_size_mb, 0u);
}

TEST(HttpServerConnectionConfig, MaxHeaderSizeBytesIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.max_header_size_bytes, 0u);
}

TEST(HttpServerConnectionConfig, RequestTimeoutMsIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.request_timeout_ms, 0u);
}

TEST(HttpServerConnectionConfig, GracefulShutdownTimeoutIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.graceful_shutdown_timeout_ms, 0u);
}

// ---------------------------------------------------------------------------
// HTTP/2 Configuration
// ---------------------------------------------------------------------------

TEST(HttpServerHttp2Config, Http2DisabledByDefault) {
    Config cfg;
    EXPECT_FALSE(cfg.enable_http2);
}

TEST(HttpServerHttp2Config, Http2MaxConcurrentStreamsDefaultIsReasonable) {
    Config cfg;
    EXPECT_GT(cfg.http2_max_concurrent_streams, 0u);
    EXPECT_LE(cfg.http2_max_concurrent_streams, 10000u);
}

TEST(HttpServerHttp2Config, Http2InitialWindowSizeIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.http2_initial_window_size, 0u);
}

// ---------------------------------------------------------------------------
// WebSocket Configuration
// ---------------------------------------------------------------------------

TEST(HttpServerWebSocketConfig, WebSocketDisabledByDefault) {
    Config cfg;
    EXPECT_FALSE(cfg.enable_websocket);
}

TEST(HttpServerWebSocketConfig, WebSocketMaxMessageSizeIsPositive) {
    Config cfg;
    EXPECT_GT(cfg.websocket_max_message_size, 0u);
}

// ---------------------------------------------------------------------------
// Health/Error Service Configuration
// ---------------------------------------------------------------------------

TEST(HttpServerHealthConfig, HealthServiceEnabledByDefault) {
    Config cfg;
    EXPECT_TRUE(cfg.health_error_service_enabled);
}

TEST(HttpServerHealthConfig, DefaultHealthServicePort) {
    Config cfg;
    EXPECT_EQ(cfg.health_error_service_port, 9090);
}

TEST(HttpServerHealthConfig, DefaultHealthServiceBindAddress) {
    Config cfg;
    // Should default to localhost-only for security
    EXPECT_EQ(cfg.health_error_service_bind_address, "127.0.0.1");
}

TEST(HttpServerHealthConfig, HealthServicePortCanBeOverridden) {
    Config cfg;
    cfg.health_error_service_port = 19090;
    EXPECT_EQ(cfg.health_error_service_port, 19090);
}

// ---------------------------------------------------------------------------
// Constructor from Host/Port
// ---------------------------------------------------------------------------

TEST(HttpServerNetworkConfig, ConstructorWithHostPort) {
    Config cfg{"127.0.0.1", 9999};
    EXPECT_EQ(cfg.host, "127.0.0.1");
    EXPECT_EQ(cfg.port, 9999);
}

TEST(HttpServerNetworkConfig, ConstructorWithHostPortAndThreads) {
    Config cfg{"127.0.0.1", 9999, 4};
    EXPECT_EQ(cfg.host, "127.0.0.1");
    EXPECT_EQ(cfg.port, 9999);
    EXPECT_EQ(cfg.num_threads, 4u);
}
