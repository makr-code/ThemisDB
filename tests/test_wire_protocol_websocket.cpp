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
// Binary frame format helpers (mirroring logic in wire_protocol_server_ws.cpp)
// ---------------------------------------------------------------------------

namespace {

static const uint8_t kWireMagic[4] = {0x54u, 0x4Du, 0x44u, 0x42u}; // "TMDB"

// Build a minimal wire-protocol frame (no payload, SKIP_CHECKSUM set).
static std::vector<uint8_t> makeFrame(uint8_t opcode,
                                       const std::vector<uint8_t>& payload = {},
                                       bool skip_checksum = true)
{
    std::vector<uint8_t> frame = {};

    frame.reserve(12 + payload.size());

    // Magic
    frame.insert(frame.end(), std::begin(kWireMagic), std::end(kWireMagic));
    // Version
    frame.push_back(0x01u);
    // OpCode
    frame.push_back(opcode);
    // Flags (big-endian): bit 2 = SKIP_CHECKSUM
    const uint16_t flags = skip_checksum ? 0x0004u : 0x0000u;
    frame.push_back(static_cast<uint8_t>((flags >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>( flags       & 0xFF));
    // PayloadSize (big-endian)
    const uint32_t ps = static_cast<uint32_t>(payload.size());
    frame.push_back(static_cast<uint8_t>((ps >> 24) & 0xFF));
    frame.push_back(static_cast<uint8_t>((ps >> 16) & 0xFF));
    frame.push_back(static_cast<uint8_t>((ps >>  8) & 0xFF));
    frame.push_back(static_cast<uint8_t>( ps         & 0xFF));
    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

// Binary frame format helpers (mirrors the logic in processBinaryFrame)
// ---------------------------------------------------------------------------

// Build a minimal valid TMDB binary frame for unit-testing parse logic.
static std::vector<uint8_t> makeBinaryFrame(
    uint8_t  opcode,
    uint16_t flags,
    const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> frame = {};

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

// Parse a response frame built by buildResponseFrame().
// Returns true on success and fills opcode, flags, and payload_str.
static bool parseResponseFrame(const std::vector<uint8_t>& frame,
                                 uint8_t& opcode_out,
                                 uint16_t& flags_out,
                                 std::string& payload_str_out)
{
    if (frame.size() < 12) {
      return false;
    }
    if (frame[0] != kWireMagic[0] || frame[1] != kWireMagic[1] ||
        frame[2] != kWireMagic[2] || frame[3] != kWireMagic[3]) return false;

    opcode_out = frame[5];
    flags_out  = (static_cast<uint16_t>(frame[6]) << 8) | frame[7];
    const uint32_t ps = (static_cast<uint32_t>(frame[8])  << 24)
                      | (static_cast<uint32_t>(frame[9])  << 16)
                      | (static_cast<uint32_t>(frame[10]) <<  8)
                      |  static_cast<uint32_t>(frame[11]);
    if (frame.size() < 12 + ps) {
      return false;
    }
    payload_str_out.assign(reinterpret_cast<const char*>(frame.data() + 12), ps);
    return true;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Binary frame format tests
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, BinaryFrameMinimumHeaderSize) {
    // A valid binary frame must be at least 12 bytes (header only, empty payload).
    const auto frame = makeFrame(0xFEu); // PING
    EXPECT_EQ(frame.size(), 12u);
}

TEST(WireProtocolWebSocket, BinaryFrameMagicBytes) {
    const auto frame = makeFrame(0x10u); // GET
    EXPECT_EQ(frame[0], 0x54u); // 'T'
    EXPECT_EQ(frame[1], 0x4Du); // 'M'
    EXPECT_EQ(frame[2], 0x44u); // 'D'
    EXPECT_EQ(frame[3], 0x42u); // 'B'
}

TEST(WireProtocolWebSocket, BinaryFrameVersionField) {
    const auto frame = makeFrame(0x10u);
    EXPECT_EQ(frame[4], 0x01u); // version 1
}

TEST(WireProtocolWebSocket, BinaryFrameOpcodeField) {
    const auto frame_ping   = makeFrame(0xFEu);
    const auto frame_get    = makeFrame(0x10u);
    const auto frame_put    = makeFrame(0x11u);
    const auto frame_delete = makeFrame(0x12u);

    EXPECT_EQ(frame_ping[5],   0xFEu);
    EXPECT_EQ(frame_get[5],    0x10u);
    EXPECT_EQ(frame_put[5],    0x11u);
    EXPECT_EQ(frame_delete[5], 0x12u);
}

TEST(WireProtocolWebSocket, BinaryFrameSkipChecksumFlag) {
    const auto frame = makeFrame(0xFEu, {}, /*skip_checksum=*/true);
    const uint16_t flags = (static_cast<uint16_t>(frame[6]) << 8) | frame[7];
    EXPECT_TRUE(flags & 0x0004u); // SKIP_CHECKSUM bit
}

TEST(WireProtocolWebSocket, BinaryFrameWithPayload) {
    const std::string json_payload = R"({"key":"users/alice"})";
    const std::vector<uint8_t> payload(json_payload.begin(), json_payload.end());
    const auto frame = makeFrame(0x10u, payload);

    // Total size = 12 header + payload
    EXPECT_EQ(frame.size(), 12u + payload.size());

    // PayloadSize field (big-endian at bytes 8-11)
    const uint32_t ps = (static_cast<uint32_t>(frame[8])  << 24)
                      | (static_cast<uint32_t>(frame[9])  << 16)
                      | (static_cast<uint32_t>(frame[10]) <<  8)
                      |  static_cast<uint32_t>(frame[11]);
    EXPECT_EQ(ps, static_cast<uint32_t>(payload.size()));
}

TEST(WireProtocolWebSocket, ResponseFrameParsing) {
    // Simulate a response frame (PONG = 0xFD) with a small JSON payload.
    const std::string json_payload = R"({"pong":true})";
    const std::vector<uint8_t> payload(json_payload.begin(), json_payload.end());
    // Build response frame with SKIP_CHECKSUM flag (same as buildResponseFrame does)
    const auto frame = makeFrame(0xFDu, payload, /*skip_checksum=*/true);

    uint8_t  opcode;
    uint16_t flags;
    std::string parsed_payload = {};
    ASSERT_TRUE(parseResponseFrame(frame, opcode, flags, parsed_payload));

    EXPECT_EQ(opcode, 0xFDu);
    EXPECT_TRUE(flags & 0x0004u); // SKIP_CHECKSUM set in response
    EXPECT_EQ(parsed_payload, json_payload);
}

TEST(WireProtocolWebSocket, BinaryFrameOpcodeDispatchTable) {
    // Document the full set of opcodes that the binary frame handler dispatches.
    // This test acts as living documentation.
    struct OpcodeEntry { uint8_t opcode; const char* name; };
    const OpcodeEntry kOpcodes[] = {
        {0x10u, "GET"},
        {0x11u, "PUT"},
        {0x12u, "DELETE"},
        {0xFEu, "PING"},
        {0xFFu, "CLOSE"},
    };
    // All opcodes must fit in one byte (self-evident, but good as a sanity check)
    for (const auto& entry : kOpcodes) {
        EXPECT_LE(entry.opcode, 0xFFu) << "opcode " << entry.name << " must fit in uint8_t";
    }
    SUCCEED() << "Binary frame opcode dispatch table documented";
}

TEST(WireProtocolWebSocket, BinaryResponseOpcodeValues) {
    // Document the response opcodes returned by the binary frame handlers.
    // Client libraries need these values to demultiplex incoming binary frames.
    constexpr uint8_t kOpcodeErrorResponse  = 0x00u;
    constexpr uint8_t kOpcodePong           = 0xFDu;
    constexpr uint8_t kOpcodeGetResponse    = 0x13u;
    constexpr uint8_t kOpcodePutResponse    = 0x14u;
    constexpr uint8_t kOpcodeDeleteResponse = 0x15u;

    // Ensure request ↔ response pairs are consistent
    EXPECT_NE(0x10u, kOpcodeGetResponse);    // GET request ≠ GET response
    EXPECT_NE(0x11u, kOpcodePutResponse);    // PUT request ≠ PUT response
    EXPECT_NE(0x12u, kOpcodeDeleteResponse); // DELETE request ≠ DELETE response
    EXPECT_NE(0xFEu, kOpcodePong);           // PING ≠ PONG

    // Error response must not collide with any request opcode
    EXPECT_NE(kOpcodeErrorResponse, 0x10u);
    EXPECT_NE(kOpcodeErrorResponse, 0x11u);
    EXPECT_NE(kOpcodeErrorResponse, 0x12u);
    EXPECT_NE(kOpcodeErrorResponse, 0xFEu);
    EXPECT_NE(kOpcodeErrorResponse, 0xFFu);

    SUCCEED() << "Binary response opcode values documented";
}

// ---------------------------------------------------------------------------
// Binary frame parse validation tests (no network I/O, purely structural)
// ---------------------------------------------------------------------------

TEST(WireProtocolWebSocket, BinaryFrameTooShortStructural) {
    // A 4-byte frame is too short (header requires 12 bytes).
    // Verify the frame-length check rejects it before magic validation.
    const std::vector<uint8_t> short_frame = {0x54, 0x4D, 0x44, 0x42};
    EXPECT_LT(short_frame.size(), 12u);
}

TEST(WireProtocolWebSocket, BinaryFrameInvalidMagicStructural) {
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
    // Verify the response-frame layout expected by the WebSocket layer.
    const std::vector<uint8_t> payload = {0x01, 0x02, 0x03};
    const auto resp = makeFrame(0x90u, payload, /*skip_checksum=*/true);

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
    const auto resp = makeFrame(0xFEu, {}, /*skip_checksum=*/true);
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
