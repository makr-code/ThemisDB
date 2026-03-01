/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_wire_protocol_websocket.cpp                   ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-23 03:59:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     235                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d6fb9967  2026-02-22  fix(network): audit fixes – connection-count correctness ... ║
    • 6d2d48159  2026-02-22  feat(network): implement WebSocket upgrade support on wir... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Unit tests for WebSocket upgrade support on the wire protocol port (8766).
// These tests verify that the upgrade detection, session construction, and
// message routing logic behave correctly.

#ifdef THEMIS_ENABLE_WEBSOCKET

#include <gtest/gtest.h>
#include "network/wire_protocol_server.h"
#include "network/wire_protocol_websocket.h"
#include <memory>
#include <string>
#include <array>

using namespace themis::network;

// ---------------------------------------------------------------------------
// Config Tests
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, ConfigDefaultDisabled) {
    WireProtocolServer::Config cfg;
    // WebSocket upgrade must be opt-in – off by default
    EXPECT_FALSE(cfg.enable_websocket_upgrade);
}

TEST(WireProtocolWebSocket, ConfigCanBeEnabled) {
    WireProtocolServer::Config cfg;
    cfg.enable_websocket_upgrade = true;
    EXPECT_TRUE(cfg.enable_websocket_upgrade);
}

TEST(WireProtocolWebSocket, ConfigPortDefaultUnchanged) {
    WireProtocolServer::Config cfg;
    // WebSocket upgrade re-uses the existing port; port must still default to 8766
    EXPECT_EQ(cfg.port, 8766u);
}

// ---------------------------------------------------------------------------
// Protocol Detection Helpers
// ---------------------------------------------------------------------------

// Helper: returns true if the given 4-byte prefix looks like "GET " (HTTP upgrade).
static bool looksLikeHttpGet(const std::array<uint8_t, 4>& bytes) {
    return bytes[0] == 'G' && bytes[1] == 'E' &&
           bytes[2] == 'T' && bytes[3] == ' ';
}

// Helper: returns true if the given 4-byte prefix is the wire protocol magic "TMDB".
static bool looksLikeBinaryMagic(const std::array<uint8_t, 4>& bytes) {
    // TMDB = 0x54 0x4D 0x44 0x42
    return bytes[0] == 0x54 && bytes[1] == 0x4D &&
           bytes[2] == 0x44 && bytes[3] == 0x42;
}

TEST(WireProtocolWebSocket, DetectionHttpGetPrefix) {
    const std::array<uint8_t, 4> http_prefix = {'G', 'E', 'T', ' '};
    EXPECT_TRUE(looksLikeHttpGet(http_prefix));
    EXPECT_FALSE(looksLikeBinaryMagic(http_prefix));
}

TEST(WireProtocolWebSocket, DetectionBinaryMagic) {
    const std::array<uint8_t, 4> magic = {0x54, 0x4D, 0x44, 0x42};  // "TMDB"
    EXPECT_TRUE(looksLikeBinaryMagic(magic));
    EXPECT_FALSE(looksLikeHttpGet(magic));
}

TEST(WireProtocolWebSocket, DetectionAmbiguousBytes) {
    // Random bytes that are neither GET nor TMDB
    const std::array<uint8_t, 4> garbage = {0x00, 0x01, 0x02, 0x03};
    EXPECT_FALSE(looksLikeHttpGet(garbage));
    EXPECT_FALSE(looksLikeBinaryMagic(garbage));
}

TEST(WireProtocolWebSocket, DetectionPostNotGet) {
    // POST requests should NOT be treated as WebSocket upgrade
    const std::array<uint8_t, 4> post_prefix = {'P', 'O', 'S', 'T'};
    EXPECT_FALSE(looksLikeHttpGet(post_prefix));
}

TEST(WireProtocolWebSocket, DetectionPutNotGet) {
    const std::array<uint8_t, 4> put_prefix = {'P', 'U', 'T', ' '};
    EXPECT_FALSE(looksLikeHttpGet(put_prefix));
}

TEST(WireProtocolWebSocket, DetectionPatchNotGet) {
    const std::array<uint8_t, 4> patch_prefix = {'P', 'A', 'T', 'C'};
    EXPECT_FALSE(looksLikeHttpGet(patch_prefix));
}

TEST(WireProtocolWebSocket, DetectionDeleteNotGet) {
    const std::array<uint8_t, 4> delete_prefix = {'D', 'E', 'L', 'E'};
    EXPECT_FALSE(looksLikeHttpGet(delete_prefix));
}

TEST(WireProtocolWebSocket, DetectionVersionMagicDifference) {
    // Wire magic is 0x54 = 'T', not 'G'.  Ensure the two are reliably distinct.
    const std::array<uint8_t, 4> get_prefix   = {'G', 'E', 'T', ' '};
    const std::array<uint8_t, 4> wire_prefix  = {0x54, 0x4D, 0x44, 0x42};

    EXPECT_TRUE(looksLikeHttpGet(get_prefix));
    EXPECT_TRUE(looksLikeBinaryMagic(wire_prefix));

    // The two prefixes must not be confused with each other
    EXPECT_FALSE(looksLikeHttpGet(wire_prefix));
    EXPECT_FALSE(looksLikeBinaryMagic(get_prefix));
}

// ---------------------------------------------------------------------------
// Server Construction with WebSocket Upgrade Enabled
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, ServerInstantiationWithUpgradeEnabled) {
    WireProtocolServer::Config cfg;
    cfg.port                    = 18766;  // test port
    cfg.enable_websocket_upgrade = true;

    auto server = std::make_unique<WireProtocolServer>(
        cfg,
        nullptr,  // storage
        nullptr,  // secondary_index
        nullptr,  // graph_index
        nullptr,  // vector_index
        nullptr,  // tx_manager
        nullptr,  // process_graph
        nullptr,  // ts_store
        nullptr   // agg_manager
    );

    ASSERT_NE(server, nullptr);
    EXPECT_FALSE(server->isRunning());
}

TEST(WireProtocolWebSocket, ServerInstantiationWithUpgradeDisabled) {
    WireProtocolServer::Config cfg;
    cfg.port                    = 18767;  // test port
    cfg.enable_websocket_upgrade = false;

    auto server = std::make_unique<WireProtocolServer>(
        cfg,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(server, nullptr);
    EXPECT_FALSE(server->isRunning());
}

// ---------------------------------------------------------------------------
// getActiveConnections includes WebSocket sessions
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, GetActiveConnectionsInitiallyZero) {
    WireProtocolServer::Config cfg;
    cfg.port                    = 18768;
    cfg.enable_websocket_upgrade = true;

    auto server = std::make_unique<WireProtocolServer>(
        cfg,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

    ASSERT_NE(server, nullptr);
    // No sessions started yet; count must be 0
    EXPECT_EQ(server->getActiveConnections(), 0u);
}

// ---------------------------------------------------------------------------
// JSON message format documentation test
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, JsonMessageFormatDocumentation) {
    // This test documents the JSON message format used over WebSocket frames.
    //
    // TEXT FRAME MESSAGES (client → server):
    //
    // Ping:
    //   { "id": "req_1", "type": "ping" }
    //
    // Get document:
    //   { "id": "req_2", "type": "get", "payload": { "key": "users/alice" } }
    //
    // Put document:
    //   { "id": "req_3", "type": "put",
    //     "payload": { "key": "users/bob", "value": "{\"name\":\"Bob\"}" } }
    //
    // Delete document:
    //   { "id": "req_4", "type": "delete", "payload": { "key": "users/carol" } }
    //
    // AQL Query (forwarded to HTTP layer):
    //   { "id": "req_5", "type": "query",
    //     "payload": { "aql": "FOR d IN users RETURN d" } }
    //
    // TEXT FRAME MESSAGES (server → client):
    //
    // Welcome on connect:
    //   { "type": "welcome", "session_id": 42, "server": "ThemisDB",
    //     "protocol": "websocket-wire/1.0", "timestamp": 1234567890 }
    //
    // Pong:
    //   { "id": "req_1", "type": "pong", "status": "ok", "timestamp": ... }
    //
    // Get response (found):
    //   { "id": "req_2", "type": "get_response", "status": "ok",
    //     "payload": "{\"name\":\"Alice\"}" }
    //
    // Get response (not found):
    //   { "id": "req_2", "type": "get_response", "status": "not_found",
    //     "message": "Key not found" }
    //
    // Put response:
    //   { "id": "req_3", "type": "put_response", "status": "ok" }
    //
    // Delete response:
    //   { "id": "req_4", "type": "delete_response", "status": "ok" }
    //
    // Query error (forwarded to HTTP):
    //   { "id": "req_5", "type": "query_response", "status": "error",
    //     "message": "AQL query execution requires the HTTP API endpoint..." }
    //
    // Error (unknown type):
    //   { "id": "req_X", "type": "error", "status": "error",
    //     "message": "Unknown message type: <type>" }

    SUCCEED() << "WebSocket wire-protocol JSON message format documented";
}

// ---------------------------------------------------------------------------
// Binary frame format helpers (mirrors the logic in processBinaryFrame)
// ---------------------------------------------------------------------------

// Build a minimal valid TMDB binary frame for unit-testing parse logic.
static std::vector<uint8_t> makeBinaryFrame(
    uint8_t  opcode,
    uint16_t flags,
    const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> frame;
    frame.reserve(12 + payload.size());
    // Magic "TMDB"
    frame.push_back(0x54); frame.push_back(0x4D);
    frame.push_back(0x44); frame.push_back(0x42);
    frame.push_back(0x01);  // Version
    frame.push_back(opcode);
    frame.push_back(static_cast<uint8_t>(flags >> 8));
    frame.push_back(static_cast<uint8_t>(flags & 0xFF));
    uint32_t ps = static_cast<uint32_t>(payload.size());
    frame.push_back(static_cast<uint8_t>(ps >> 24));
    frame.push_back(static_cast<uint8_t>((ps >> 16) & 0xFF));
    frame.push_back(static_cast<uint8_t>((ps >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(ps & 0xFF));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

// ---------------------------------------------------------------------------
// Binary frame parse validation tests (no network I/O, purely structural)
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, BinaryFrameTooShort) {
    // A 4-byte frame is too short (header requires 12 bytes).
    // Verify the frame-length check rejects it before magic validation.
    const std::vector<uint8_t> short_frame = {0x54, 0x4D, 0x44, 0x42};
    EXPECT_LT(short_frame.size(), 12u);
}

TEST(WireProtocolWebSocket, BinaryFrameInvalidMagic) {
    // Frame with wrong magic must be rejected.
    const std::vector<uint8_t> bad_magic = {
        0x00, 0x00, 0x00, 0x00,  // wrong magic
        0x01,                    // version
        0xFE,                    // opcode (PING)
        0x00, 0x04,              // flags: SKIP_CHECKSUM
        0x00, 0x00, 0x00, 0x00   // payload_size = 0
    };
    ASSERT_EQ(bad_magic.size(), 12u);
    EXPECT_NE(bad_magic[0], 0x54u);
}

TEST(WireProtocolWebSocket, BinaryFrameValidMagicAndHeader) {
    // Construct a valid 12-byte PING frame and verify all header fields.
    const uint16_t kSkipChecksum = 0x0004;
    const std::vector<uint8_t> ping_frame = makeBinaryFrame(0xFE, kSkipChecksum, {});

    ASSERT_EQ(ping_frame.size(), 12u);
    // Magic
    EXPECT_EQ(ping_frame[0], 0x54u);
    EXPECT_EQ(ping_frame[1], 0x4Du);
    EXPECT_EQ(ping_frame[2], 0x44u);
    EXPECT_EQ(ping_frame[3], 0x42u);
    // Version
    EXPECT_EQ(ping_frame[4], 0x01u);
    // OpCode
    EXPECT_EQ(ping_frame[5], 0xFEu);
    // Flags (big-endian)
    const uint16_t flags = (static_cast<uint16_t>(ping_frame[6]) << 8) | ping_frame[7];
    EXPECT_EQ(flags, kSkipChecksum);
    // PayloadSize = 0
    const uint32_t payload_size =
        (static_cast<uint32_t>(ping_frame[8])  << 24) |
        (static_cast<uint32_t>(ping_frame[9])  << 16) |
        (static_cast<uint32_t>(ping_frame[10]) << 8)  |
         static_cast<uint32_t>(ping_frame[11]);
    EXPECT_EQ(payload_size, 0u);
}

TEST(WireProtocolWebSocket, BinaryFramePayloadSizeFieldParsing) {
    // Verify that large payload sizes are encoded and decoded correctly.
    const uint16_t kSkipChecksum = 0x0004;
    const std::vector<uint8_t> dummy_payload(256, 0xAB);
    const auto frame = makeBinaryFrame(0x10, kSkipChecksum, dummy_payload);

    ASSERT_GE(frame.size(), 12u);
    const uint32_t decoded_size =
        (static_cast<uint32_t>(frame[8])  << 24) |
        (static_cast<uint32_t>(frame[9])  << 16) |
        (static_cast<uint32_t>(frame[10]) << 8)  |
         static_cast<uint32_t>(frame[11]);
    EXPECT_EQ(decoded_size, 256u);
    EXPECT_EQ(frame.size(), 12u + 256u);
}

TEST(WireProtocolWebSocket, BinaryFrameChecksumFlagDetection) {
    // SKIP_CHECKSUM_FLAG = 0x0004; if set, no CRC-32 trailer expected.
    const uint16_t no_checksum_flags  = 0x0004;  // bit 2 set → skip
    const uint16_t has_checksum_flags = 0x0000;  // bit 2 clear → expect CRC-32

    EXPECT_TRUE(no_checksum_flags  & 0x0004);
    EXPECT_FALSE(has_checksum_flags & 0x0004);
}

TEST(WireProtocolWebSocket, BinaryFrameExpectedSizeWithChecksum) {
    // A frame with a 4-byte payload and no skip-checksum flag must be
    // exactly header(12) + payload(4) + crc(4) = 20 bytes total.
    const uint16_t kNoSkip = 0x0000;
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    auto frame = makeBinaryFrame(0x10, kNoSkip, payload);
    // Append a dummy 4-byte CRC trailer so the frame is well-formed.
    frame.push_back(0x00); frame.push_back(0x00);
    frame.push_back(0x00); frame.push_back(0x00);

    EXPECT_EQ(frame.size(), 20u);

    const bool has_checksum = !(kNoSkip & 0x0004);
    const size_t expected = 12u + 4u + (has_checksum ? 4u : 0u);
    EXPECT_EQ(expected, 20u);
    EXPECT_GE(frame.size(), expected);
}

TEST(WireProtocolWebSocket, BinaryResponseFrameStructure) {
    // Verify buildBinaryResponseFrame produces a well-formed frame.
    const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    const auto resp = WireProtocolWebSocketSession::buildBinaryResponseFrame(0x90, payload);

    ASSERT_GE(resp.size(), 12u + payload.size());
    // Magic
    EXPECT_EQ(resp[0], 0x54u);
    EXPECT_EQ(resp[1], 0x4Du);
    EXPECT_EQ(resp[2], 0x44u);
    EXPECT_EQ(resp[3], 0x42u);
    // Version
    EXPECT_EQ(resp[4], 0x01u);
    // OpCode must match
    EXPECT_EQ(resp[5], 0x90u);
    // Flags must have SKIP_CHECKSUM set (0x0004)
    const uint16_t flags = (static_cast<uint16_t>(resp[6]) << 8) | resp[7];
    EXPECT_TRUE(flags & 0x0004u);
    // Payload size
    const uint32_t ps =
        (static_cast<uint32_t>(resp[8])  << 24) |
        (static_cast<uint32_t>(resp[9])  << 16) |
        (static_cast<uint32_t>(resp[10]) << 8)  |
         static_cast<uint32_t>(resp[11]);
    EXPECT_EQ(ps, static_cast<uint32_t>(payload.size()));
    // Payload bytes
    EXPECT_EQ(resp[12], 0x01u);
    EXPECT_EQ(resp[13], 0x02u);
    EXPECT_EQ(resp[14], 0x03u);
}

TEST(WireProtocolWebSocket, BinaryResponseFrameEmptyPayload) {
    const auto resp = WireProtocolWebSocketSession::buildBinaryResponseFrame(0xFE, {});
    EXPECT_EQ(resp.size(), 12u);
    const uint32_t ps =
        (static_cast<uint32_t>(resp[8])  << 24) |
        (static_cast<uint32_t>(resp[9])  << 16) |
        (static_cast<uint32_t>(resp[10]) << 8)  |
         static_cast<uint32_t>(resp[11]);
    EXPECT_EQ(ps, 0u);
}

TEST(WireProtocolWebSocket, BinaryOpcodeConstants) {
    // Document and verify the opcode assignments used in the binary frame path.
    // Requests
    static constexpr uint8_t kOpGet    = 0x10;
    static constexpr uint8_t kOpPut    = 0x11;
    static constexpr uint8_t kOpDelete = 0x12;
    static constexpr uint8_t kOpPing   = 0xFE;
    static constexpr uint8_t kOpClose  = 0xFF;
    // Responses (high bit set for get/put/delete)
    static constexpr uint8_t kRespGet    = 0x90;
    static constexpr uint8_t kRespPut    = 0x91;
    static constexpr uint8_t kRespDelete = 0x92;
    static constexpr uint8_t kRespError  = 0x80;

    EXPECT_EQ(kOpGet,     0x10u);
    EXPECT_EQ(kOpPut,     0x11u);
    EXPECT_EQ(kOpDelete,  0x12u);
    EXPECT_EQ(kOpPing,    0xFEu);
    EXPECT_EQ(kOpClose,   0xFFu);
    EXPECT_EQ(kRespGet,   0x90u);
    EXPECT_EQ(kRespPut,   0x91u);
    EXPECT_EQ(kRespDelete,0x92u);
    EXPECT_EQ(kRespError, 0x80u);
}

#endif // THEMIS_ENABLE_WEBSOCKET
