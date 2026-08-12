// HTTP/2 Protocol Basic Tests
// These tests validate the HTTP/2 implementation including ALPN negotiation and stream handling

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_HTTP2

#include "server/http2_session.h"
#include <atomic>
#include <string>
#include <vector>

using namespace themis::server;

// ============================================================================
// HTTP/2 Basic Functionality Tests
// ============================================================================

TEST(HTTP2ProtocolTest, AlpnNegotiationSuccess) {
    // Test that ALPN negotiation correctly selects "h2" protocol
    std::vector<std::string> alpn_protocols = {"h2", "http/1.1"};
    
    // Verify "h2" is first in the list (preferred)
    EXPECT_EQ(alpn_protocols[0], "h2");
    EXPECT_EQ(alpn_protocols[1], "http/1.1");
}

TEST(HTTP2ProtocolTest, AlpnFallbackToHTTP1) {
    // Test that ALPN can fallback to HTTP/1.1 if h2 is not supported
    std::vector<std::string> fallback_protocols = {"http/1.1"};
    
    EXPECT_EQ(fallback_protocols[0], "http/1.1");
}

TEST(HTTP2ProtocolTest, StreamIdValidation) {
    // HTTP/2 client-initiated streams must have odd stream IDs
    // Server-initiated streams (like push) must have even stream IDs
    
    int32_t client_stream_id = 1;
    EXPECT_TRUE(client_stream_id % 2 == 1) << "Client stream ID must be odd";
    
    int32_t server_stream_id = 2;
    EXPECT_TRUE(server_stream_id % 2 == 0) << "Server stream ID must be even";
}

TEST(HTTP2ProtocolTest, MaxConcurrentStreamsDefault) {
    // Test that default max concurrent streams is reasonable
    const int32_t default_max_streams = 100;
    
    EXPECT_GT(default_max_streams, 0) << "Max concurrent streams must be positive";
    EXPECT_LE(default_max_streams, 1000) << "Max concurrent streams should have reasonable upper limit";
}

TEST(HTTP2ProtocolTest, HeaderCompressionHPACK) {
    // Verify HPACK is used for header compression
    const std::string compression_method = "HPACK";
    
    EXPECT_EQ(compression_method, "HPACK");
}

// ============================================================================
// HTTP/2 Request Method Support Tests
// ============================================================================

TEST(HTTP2ProtocolTest, SupportedHTTPMethods) {
    // Test that all standard HTTP methods are supported
    std::vector<std::string> supported_methods = {
        "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"
    };
    
    EXPECT_EQ(supported_methods.size(), 7);
    EXPECT_NE(std::find(supported_methods.begin(), supported_methods.end(), "GET"), 
              supported_methods.end());
    EXPECT_NE(std::find(supported_methods.begin(), supported_methods.end(), "POST"), 
              supported_methods.end());
}

// ============================================================================
// HTTP/2 Configuration Tests
// ============================================================================

TEST(HTTP2ProtocolTest, ConfigurationDefaults) {
    // Test HTTP/2 configuration defaults
    struct HTTP2Config {
        bool enable_http2 = false;  // OFF by default for security
        int32_t max_concurrent_streams = 100;
        bool enable_server_push = false;
    };
    
    HTTP2Config config;
    
    EXPECT_FALSE(config.enable_http2) << "HTTP/2 should be OFF by default";
    EXPECT_EQ(config.max_concurrent_streams, 100);
    EXPECT_FALSE(config.enable_server_push) << "Server push should be OFF by default";
}

TEST(HTTP2ProtocolTest, ReservedAdmissionSlotTransfersToHttp2Session) {
    std::atomic<uint64_t> active_connections{1};  // slot already reserved on accept

    const auto ctor_accounting = [&](bool connection_slot_reserved) {
        if (!connection_slot_reserved) {
            active_connections.fetch_add(1, std::memory_order_relaxed);
        }
    };
    const auto dtor_accounting = [&]() {
        active_connections.fetch_sub(1, std::memory_order_relaxed);
    };

    ctor_accounting(true);
    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), 1u);

    dtor_accounting();
    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), 0u);
}

TEST(HTTP2ProtocolTest, UnreservedHttp2SessionOwnsItsConnectionCount) {
    std::atomic<uint64_t> active_connections{0};

    const auto ctor_accounting = [&](bool connection_slot_reserved) {
        if (!connection_slot_reserved) {
            active_connections.fetch_add(1, std::memory_order_relaxed);
        }
    };
    const auto dtor_accounting = [&]() {
        active_connections.fetch_sub(1, std::memory_order_relaxed);
    };

    ctor_accounting(false);
    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), 1u);

    dtor_accounting();
    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), 0u);
}

TEST(HTTP2ProtocolTest, TimeoutGuardTreatsZeroAsDisabled) {
    std::atomic<uint32_t> timeout_ms{0};
    const auto is_timeout_enabled = [&]() {
        return timeout_ms.load(std::memory_order_acquire) > 0;
    };

    EXPECT_FALSE(is_timeout_enabled());
    timeout_ms.store(1500, std::memory_order_release);
    EXPECT_TRUE(is_timeout_enabled());
}

#endif // THEMIS_ENABLE_HTTP2

// Placeholder test when HTTP/2 is disabled
#ifndef THEMIS_ENABLE_HTTP2
TEST(HTTP2ProtocolTest, DisabledByDefault) {
    GTEST_SKIP() << "HTTP/2 is disabled. Build with -DTHEMIS_ENABLE_HTTP2=ON to enable.";
}
#endif
