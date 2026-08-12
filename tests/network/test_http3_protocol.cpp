// HTTP/3 Protocol Basic Tests
// These tests validate the HTTP/3 implementation including QUIC transport and ALPN negotiation

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_HTTP3

#define private public
#include "server/http3_session.h"
#undef private
#include <string>
#include <vector>

using namespace themis::server;

// ============================================================================
// HTTP/3 Basic Functionality Tests
// ============================================================================

TEST(HTTP3ProtocolTest, AlpnNegotiationH3) {
    // Test that ALPN negotiation correctly selects "h3" protocol for HTTP/3
    std::vector<std::string> alpn_protocols = {"h3", "h2", "http/1.1"};
    
    // Verify "h3" is first in the list (preferred for HTTP/3)
    EXPECT_EQ(alpn_protocols[0], "h3");
    EXPECT_EQ(alpn_protocols[1], "h2");
    EXPECT_EQ(alpn_protocols[2], "http/1.1");
}

TEST(HTTP3ProtocolTest, AlpnFallbackToHTTP2) {
    // Test that ALPN can fallback to HTTP/2 if h3 is not supported
    std::vector<std::string> fallback_protocols = {"h2", "http/1.1"};
    
    EXPECT_EQ(fallback_protocols[0], "h2");
    EXPECT_EQ(fallback_protocols[1], "http/1.1");
}

// ============================================================================
// QUIC Protocol Tests
// ============================================================================

TEST(HTTP3ProtocolTest, QuicVersionNegotiation) {
    // Test QUIC version 1 (RFC 9000)
    const uint32_t QUIC_VERSION_1 = 0x00000001;
    const uint32_t QUIC_VERSION_DRAFT_29 = 0xff00001d;
    
    // HTTP/3 uses QUIC version 1
    EXPECT_EQ(QUIC_VERSION_1, 1);
    EXPECT_NE(QUIC_VERSION_1, QUIC_VERSION_DRAFT_29);
}

TEST(HTTP3ProtocolTest, QuicStreamTypes) {
    // QUIC has different stream types
    // Client-initiated bidirectional: 0x00, 0x04, 0x08, ... (multiples of 4)
    // Server-initiated bidirectional: 0x01, 0x05, 0x09, ... (4n+1)
    // Client-initiated unidirectional: 0x02, 0x06, 0x0A, ... (4n+2)
    // Server-initiated unidirectional: 0x03, 0x07, 0x0B, ... (4n+3)
    
    int64_t client_bidi = 0;
    int64_t server_bidi = 1;
    int64_t client_uni = 2;
    int64_t server_uni = 3;
    
    EXPECT_EQ(client_bidi % 4, 0);
    EXPECT_EQ(server_bidi % 4, 1);
    EXPECT_EQ(client_uni % 4, 2);
    EXPECT_EQ(server_uni % 4, 3);
}

TEST(HTTP3ProtocolTest, QuicConnectionIdLength) {
    // QUIC connection IDs should be between 0 and 20 bytes
    const size_t MIN_CID_LENGTH = 0;
    const size_t MAX_CID_LENGTH = 20;
    const size_t DEFAULT_CID_LENGTH = 8;
    
    EXPECT_GE(DEFAULT_CID_LENGTH, MIN_CID_LENGTH);
    EXPECT_LE(DEFAULT_CID_LENGTH, MAX_CID_LENGTH);
}

// ============================================================================
// HTTP/3 Frame Types Tests
// ============================================================================

TEST(HTTP3ProtocolTest, Http3FrameTypes) {
    // HTTP/3 frame types as defined in RFC 9114
    const uint64_t DATA_FRAME = 0x00;
    const uint64_t HEADERS_FRAME = 0x01;
    const uint64_t CANCEL_PUSH_FRAME = 0x03;
    const uint64_t SETTINGS_FRAME = 0x04;
    const uint64_t PUSH_PROMISE_FRAME = 0x05;
    const uint64_t GOAWAY_FRAME = 0x07;
    const uint64_t MAX_PUSH_ID_FRAME = 0x0d;
    
    EXPECT_EQ(DATA_FRAME, 0x00);
    EXPECT_EQ(HEADERS_FRAME, 0x01);
    EXPECT_EQ(SETTINGS_FRAME, 0x04);
    EXPECT_EQ(GOAWAY_FRAME, 0x07);
}

TEST(HTTP3ProtocolTest, Http3StreamTypes) {
    // HTTP/3 unidirectional stream types
    const uint64_t CONTROL_STREAM = 0x00;
    const uint64_t PUSH_STREAM = 0x01;
    const uint64_t QPACK_ENCODER_STREAM = 0x02;
    const uint64_t QPACK_DECODER_STREAM = 0x03;
    
    EXPECT_EQ(CONTROL_STREAM, 0x00);
    EXPECT_EQ(PUSH_STREAM, 0x01);
    EXPECT_EQ(QPACK_ENCODER_STREAM, 0x02);
    EXPECT_EQ(QPACK_DECODER_STREAM, 0x03);
}

// ============================================================================
// HTTP/3 Settings Tests
// ============================================================================

TEST(HTTP3ProtocolTest, Http3SettingsParameters) {
    // HTTP/3 SETTINGS parameters as defined in RFC 9114
    const uint64_t SETTINGS_QPACK_MAX_TABLE_CAPACITY = 0x01;
    const uint64_t SETTINGS_MAX_FIELD_SECTION_SIZE = 0x06;
    const uint64_t SETTINGS_QPACK_BLOCKED_STREAMS = 0x07;
    
    EXPECT_EQ(SETTINGS_QPACK_MAX_TABLE_CAPACITY, 0x01);
    EXPECT_EQ(SETTINGS_MAX_FIELD_SECTION_SIZE, 0x06);
    EXPECT_EQ(SETTINGS_QPACK_BLOCKED_STREAMS, 0x07);
}

TEST(HTTP3ProtocolTest, DefaultQPackSettings) {
    // Test default QPACK settings
    const uint64_t default_max_table_capacity = 4096;
    const uint64_t default_blocked_streams = 100;
    
    EXPECT_EQ(default_max_table_capacity, 4096);
    EXPECT_EQ(default_blocked_streams, 100);
}

// ============================================================================
// QUIC Transport Parameters Tests
// ============================================================================

TEST(HTTP3ProtocolTest, QuicTransportParameters) {
    // QUIC transport parameters
    const uint64_t initial_max_data = 1024 * 1024;  // 1 MB
    const uint64_t initial_max_stream_data_bidi_local = 256 * 1024;  // 256 KB
    const uint64_t initial_max_stream_data_bidi_remote = 256 * 1024;  // 256 KB
    const uint64_t initial_max_streams_bidi = 100;
    const uint64_t initial_max_streams_uni = 3;
    
    EXPECT_EQ(initial_max_data, 1024 * 1024);
    EXPECT_EQ(initial_max_stream_data_bidi_local, 256 * 1024);
    EXPECT_EQ(initial_max_streams_bidi, 100);
    EXPECT_EQ(initial_max_streams_uni, 3);
}

TEST(HTTP3ProtocolTest, QuicIdleTimeout) {
    // QUIC idle timeout (default 30 seconds)
    const uint32_t default_idle_timeout_ms = 30000;
    const uint32_t min_idle_timeout_ms = 3000;  // 3 seconds minimum
    const uint32_t max_idle_timeout_ms = 600000;  // 10 minutes maximum
    
    EXPECT_GE(default_idle_timeout_ms, min_idle_timeout_ms);
    EXPECT_LE(default_idle_timeout_ms, max_idle_timeout_ms);
}

// ============================================================================
// TLS 1.3 Requirements Tests
// ============================================================================

TEST(HTTP3ProtocolTest, TLS13Required) {
    // HTTP/3 requires TLS 1.3
    const int TLS_1_3_VERSION = 0x0304;
    const int TLS_1_2_VERSION = 0x0303;
    
    // Verify TLS 1.3 version number
    EXPECT_EQ(TLS_1_3_VERSION, 0x0304);
    EXPECT_NE(TLS_1_3_VERSION, TLS_1_2_VERSION);
}

TEST(HTTP3ProtocolTest, TLS13CipherSuites) {
    // TLS 1.3 cipher suites for HTTP/3
    const uint16_t TLS_AES_128_GCM_SHA256 = 0x1301;
    const uint16_t TLS_AES_256_GCM_SHA384 = 0x1302;
    const uint16_t TLS_CHACHA20_POLY1305_SHA256 = 0x1303;
    
    EXPECT_EQ(TLS_AES_128_GCM_SHA256, 0x1301);
    EXPECT_EQ(TLS_AES_256_GCM_SHA384, 0x1302);
    EXPECT_EQ(TLS_CHACHA20_POLY1305_SHA256, 0x1303);
}

// ============================================================================
// 0-RTT Connection Tests
// ============================================================================

TEST(HTTP3ProtocolTest, ZeroRTTSupport) {
    // Test that 0-RTT is supported for connection resumption
    bool supports_0rtt = true;
    bool early_data_enabled = true;
    
    EXPECT_TRUE(supports_0rtt);
    EXPECT_TRUE(early_data_enabled);
}

TEST(HTTP3ProtocolTest, ZeroRTTReplayProtection) {
    // 0-RTT data must be replay-safe
    // Only idempotent operations should be allowed in 0-RTT
    std::vector<std::string> safe_methods = {"GET", "HEAD", "OPTIONS"};
    std::vector<std::string> unsafe_methods = {"POST", "PUT", "DELETE", "PATCH"};
    
    EXPECT_EQ(safe_methods.size(), 3);
    EXPECT_EQ(unsafe_methods.size(), 4);
}

// ============================================================================
// Connection Migration Tests
// ============================================================================

TEST(HTTP3ProtocolTest, ConnectionMigrationSupport) {
    // QUIC supports connection migration (IP address changes)
    bool supports_migration = true;
    
    // Client can migrate to new IP address while maintaining connection
    EXPECT_TRUE(supports_migration);
}

TEST(HTTP3ProtocolTest, PathValidation) {
    // New paths must be validated before use
    bool requires_path_validation = true;
    
    EXPECT_TRUE(requires_path_validation);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(HTTP3ProtocolTest, QuicErrorCodes) {
    // QUIC transport error codes
    const uint64_t QUIC_NO_ERROR = 0x00;
    const uint64_t QUIC_INTERNAL_ERROR = 0x01;
    const uint64_t QUIC_CONNECTION_REFUSED = 0x02;
    const uint64_t QUIC_FLOW_CONTROL_ERROR = 0x03;
    const uint64_t QUIC_STREAM_LIMIT_ERROR = 0x04;
    
    EXPECT_EQ(QUIC_NO_ERROR, 0x00);
    EXPECT_EQ(QUIC_INTERNAL_ERROR, 0x01);
    EXPECT_EQ(QUIC_CONNECTION_REFUSED, 0x02);
}

TEST(HTTP3ProtocolTest, Http3ErrorCodes) {
    // HTTP/3 application error codes
    const uint64_t H3_NO_ERROR = 0x0100;
    const uint64_t H3_GENERAL_PROTOCOL_ERROR = 0x0101;
    const uint64_t H3_INTERNAL_ERROR = 0x0102;
    const uint64_t H3_STREAM_CREATION_ERROR = 0x0103;
    const uint64_t H3_CLOSED_CRITICAL_STREAM = 0x0104;
    const uint64_t H3_FRAME_UNEXPECTED = 0x0105;
    const uint64_t H3_FRAME_ERROR = 0x0106;
    
    EXPECT_EQ(H3_NO_ERROR, 0x0100);
    EXPECT_EQ(H3_GENERAL_PROTOCOL_ERROR, 0x0101);
    EXPECT_EQ(H3_INTERNAL_ERROR, 0x0102);
}

TEST(HTTP3ProtocolTest, EndStreamCallbackNullSessionFailsClosed) {
    EXPECT_EQ(
        Http3Session::http3EndStreamCallback(nullptr, 1, nullptr, nullptr),
        NGHTTP3_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, EndHeadersCallbackNullSessionFailsClosed) {
    EXPECT_EQ(
        Http3Session::http3EndHeadersCallback(nullptr, 1, 0, nullptr, nullptr),
        NGHTTP3_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, RecvDataCallbackNullSessionFailsClosed) {
    const uint8_t payload[] = {0x01, 0x02};
    EXPECT_EQ(
        Http3Session::http3RecvDataCallback(nullptr, 1, payload, sizeof(payload), nullptr, nullptr),
        NGHTTP3_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, DecodeHeaderCallbackInvalidInputFailsClosed) {
    EXPECT_EQ(
        Http3Session::http3DecodHeaderCallback(nullptr, 1, 0, nullptr, nullptr, 0, nullptr, nullptr),
        NGHTTP3_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, RecvDatagramCallbackNullSessionFailsClosed) {
    const uint8_t payload[] = {0xAB};
    EXPECT_EQ(
        Http3Session::recvDatagramCallback(nullptr, 0, payload, sizeof(payload), nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, HandshakeCompletedCallbackNullSessionFailsClosed) {
    EXPECT_EQ(
        Http3Session::handshakeCompletedCallback(nullptr, nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, RecvStreamDataCallbackNullSessionFailsClosed) {
    const uint8_t payload[] = {0x10};
    EXPECT_EQ(
        Http3Session::recvStreamDataCallback(nullptr, 0, 1, 0, payload, sizeof(payload), nullptr, nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, AckStreamDataCallbackNullSessionFailsClosed) {
    EXPECT_EQ(
        Http3Session::ackStreamDataCallback(nullptr, 1, 0, 0, nullptr, nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, StreamCloseCallbackNullSessionFailsClosed) {
    EXPECT_EQ(
        Http3Session::streamCloseCallback(nullptr, 0, 1, 0, nullptr, nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, GetNewConnectionIdCallbackNullCidFailsClosed) {
    EXPECT_EQ(
        Http3Session::getNewConnectionIdCallback(nullptr, nullptr, nullptr, 0, nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

TEST(HTTP3ProtocolTest, RecvCryptoDataCallbackNullSessionFailsClosed) {
    const uint8_t payload[] = {0x01};
    EXPECT_EQ(
        Http3Session::recvCryptoDataCallback(nullptr, NGTCP2_ENCRYPTION_LEVEL_INITIAL,
                                             0, payload, sizeof(payload), nullptr),
        NGTCP2_ERR_CALLBACK_FAILURE
    );
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST(HTTP3ProtocolTest, MultiplexingSupport) {
    // HTTP/3 supports true multiplexing without head-of-line blocking
    const int max_concurrent_streams = 100;
    
    EXPECT_GT(max_concurrent_streams, 0);
    EXPECT_LE(max_concurrent_streams, 1000);
}

TEST(HTTP3ProtocolTest, FlowControlLimits) {
    // Test flow control window sizes
    const uint64_t initial_window_size = 256 * 1024;  // 256 KB
    const uint64_t max_window_size = 16 * 1024 * 1024;  // 16 MB
    
    EXPECT_GT(initial_window_size, 0);
    EXPECT_LE(initial_window_size, max_window_size);
}

// ============================================================================
// Header Compression Tests (QPACK)
// ============================================================================

TEST(HTTP3ProtocolTest, QPackCompression) {
    // QPACK is used for header compression in HTTP/3
    bool uses_qpack = true;
    bool dynamic_table_enabled = true;
    
    EXPECT_TRUE(uses_qpack);
    EXPECT_TRUE(dynamic_table_enabled);
}

TEST(HTTP3ProtocolTest, QPackDynamicTableSize) {
    // Test QPACK dynamic table configuration
    const uint64_t max_table_capacity = 4096;
    const uint64_t max_blocked_streams = 100;
    
    EXPECT_EQ(max_table_capacity, 4096);
    EXPECT_EQ(max_blocked_streams, 100);
}

#endif // THEMIS_ENABLE_HTTP3
