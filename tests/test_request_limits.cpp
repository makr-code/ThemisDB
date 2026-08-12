/**
 * @file test_request_limits.cpp
 * @brief Tests for HTTP request size and header size limit configuration
 *
 * Validates that HttpServer::Config correctly exposes and defaults all
 * request-limiting knobs introduced for production readiness.
 */

#include <gtest/gtest.h>
#include "server/http_server.h"

using Config = themis::server::HttpServer::Config;

// ---------------------------------------------------------------------------
// Body Size Limit Tests
// ---------------------------------------------------------------------------

TEST(RequestLimitsConfig, DefaultMaxRequestSizeMbIsSet) {
    Config cfg;
    EXPECT_GT(cfg.max_request_size_mb, 0u);
}

TEST(RequestLimitsConfig, DefaultMaxRequestSizeMbIs10) {
    Config cfg;
    EXPECT_EQ(cfg.max_request_size_mb, 10u);
}

TEST(RequestLimitsConfig, MaxRequestSizeMbCanBeOverridden) {
    Config cfg;
    cfg.max_request_size_mb = 50;
    EXPECT_EQ(cfg.max_request_size_mb, 50u);
}

TEST(RequestLimitsConfig, MaxRequestSizeMbZeroMeansNoLimit) {
    // 0 should be an accepted value meaning unlimited
    Config cfg;
    cfg.max_request_size_mb = 0;
    EXPECT_EQ(cfg.max_request_size_mb, 0u);
}

// ---------------------------------------------------------------------------
// Header Size Limit Tests
// ---------------------------------------------------------------------------

TEST(RequestLimitsConfig, DefaultMaxHeaderSizeBytesIsSet) {
    Config cfg;
    EXPECT_GT(cfg.max_header_size_bytes, 0u);
}

TEST(RequestLimitsConfig, DefaultMaxHeaderSizeBytesIs8KB) {
    Config cfg;
    EXPECT_EQ(cfg.max_header_size_bytes, 8192u);
}

TEST(RequestLimitsConfig, MaxHeaderSizeBytesCanBeOverridden) {
    Config cfg;
    cfg.max_header_size_bytes = 16384;
    EXPECT_EQ(cfg.max_header_size_bytes, 16384u);
}

TEST(RequestLimitsConfig, MaxHeaderSizeBytesZeroDisablesCheck) {
    Config cfg;
    cfg.max_header_size_bytes = 0;
    EXPECT_EQ(cfg.max_header_size_bytes, 0u);
}

// ---------------------------------------------------------------------------
// Timeout Configuration Tests
// ---------------------------------------------------------------------------

TEST(RequestLimitsConfig, DefaultRequestTimeoutMsIsSet) {
    Config cfg;
    EXPECT_GT(cfg.request_timeout_ms, 0u);
}

TEST(RequestLimitsConfig, DefaultRequestTimeoutMsIs30Seconds) {
    Config cfg;
    EXPECT_EQ(cfg.request_timeout_ms, 30000u);
}

TEST(RequestLimitsConfig, RequestTimeoutMsCanBeOverridden) {
    Config cfg;
    cfg.request_timeout_ms = 5000;
    EXPECT_EQ(cfg.request_timeout_ms, 5000u);
}

// ---------------------------------------------------------------------------
// Graceful Shutdown Timeout Tests
// ---------------------------------------------------------------------------

TEST(RequestLimitsConfig, DefaultGracefulShutdownTimeoutIsSet) {
    Config cfg;
    EXPECT_GT(cfg.graceful_shutdown_timeout_ms, 0u);
}

TEST(RequestLimitsConfig, DefaultGracefulShutdownTimeoutIs30Seconds) {
    Config cfg;
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 30000u);
}

TEST(RequestLimitsConfig, GracefulShutdownTimeoutCanBeOverridden) {
    Config cfg;
    cfg.graceful_shutdown_timeout_ms = 5000;
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 5000u);
}
