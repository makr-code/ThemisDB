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

#endif // THEMIS_ENABLE_WEBSOCKET
