/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_wire_protocol_v2.cpp                          ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-02-21 14:08:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     221                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file test_wire_protocol_v2.cpp
/// @brief Unit tests for Wire Protocol V2 types, constants, and frame serialization
///
/// Tests cover:
/// - V2 protocol constants
/// - V2FrameHeader is_valid() / get_type() / has_flag()
/// - V2Stream state machine helpers
/// - V2ConnectionConfig defaults
/// - V2Server construction / lifecycle (without starting a real server)

#include <gtest/gtest.h>
#include "themis/network/wire_protocol_v2.hpp"

#include <cstring>

using namespace themis::wire;

// ===== Constant Tests =====

TEST(WireV2Constants, MagicValue) {
    // "TMD2" in big-endian ASCII
    EXPECT_EQ(WIRE_V2_MAGIC, 0x544D4432u);
}

TEST(WireV2Constants, Version) {
    EXPECT_EQ(WIRE_VERSION_2, 0x02u);
}

TEST(WireV2Constants, HeaderSize) {
    EXPECT_EQ(V2_HEADER_SIZE, 16u);
    EXPECT_EQ(sizeof(V2FrameHeader), V2_HEADER_SIZE);
}

TEST(WireV2Constants, MaxPayload) {
    EXPECT_EQ(V2_MAX_PAYLOAD, 16u * 1024 * 1024);
}

TEST(WireV2Constants, DefaultWindow) {
    EXPECT_EQ(V2_DEFAULT_WINDOW, 64u * 1024);
}

// ===== V2FrameHeader Tests =====

TEST(V2FrameHeader, ValidHeader) {
    V2FrameHeader h{};
    h.magic   = WIRE_V2_MAGIC;
    h.version = WIRE_VERSION_2;
    EXPECT_TRUE(h.is_valid());
}

TEST(V2FrameHeader, InvalidMagic) {
    V2FrameHeader h{};
    h.magic   = 0xDEADBEEF;
    h.version = WIRE_VERSION_2;
    EXPECT_FALSE(h.is_valid());
}

TEST(V2FrameHeader, InvalidVersion) {
    V2FrameHeader h{};
    h.magic   = WIRE_V2_MAGIC;
    h.version = 0x01; // v1, not v2
    EXPECT_FALSE(h.is_valid());
}

TEST(V2FrameHeader, GetType) {
    V2FrameHeader h{};
    h.frame_type = static_cast<uint8_t>(V2FrameType::DATA);
    EXPECT_EQ(h.get_type(), V2FrameType::DATA);

    h.frame_type = static_cast<uint8_t>(V2FrameType::SETTINGS);
    EXPECT_EQ(h.get_type(), V2FrameType::SETTINGS);

    h.frame_type = static_cast<uint8_t>(V2FrameType::GOAWAY);
    EXPECT_EQ(h.get_type(), V2FrameType::GOAWAY);
}

TEST(V2FrameHeader, HasFlag) {
    V2FrameHeader h{};
    h.flags = static_cast<uint16_t>(V2FrameFlags::END_STREAM) |
              static_cast<uint16_t>(V2FrameFlags::COMPRESSED);

    EXPECT_TRUE(h.has_flag(V2FrameFlags::END_STREAM));
    EXPECT_TRUE(h.has_flag(V2FrameFlags::COMPRESSED));
    EXPECT_FALSE(h.has_flag(V2FrameFlags::END_HEADERS));
    EXPECT_FALSE(h.has_flag(V2FrameFlags::PADDED));
}

TEST(V2FrameHeader, NoFlags) {
    V2FrameHeader h{};
    h.flags = static_cast<uint16_t>(V2FrameFlags::NONE);
    EXPECT_FALSE(h.has_flag(V2FrameFlags::END_STREAM));
    EXPECT_FALSE(h.has_flag(V2FrameFlags::COMPRESSED));
}

// ===== V2Stream Tests =====

TEST(V2Stream, DefaultState) {
    V2Stream s;
    EXPECT_EQ(s.stream_id,   0u);
    EXPECT_EQ(s.state,       V2StreamState::IDLE);
    EXPECT_EQ(s.send_window, static_cast<int32_t>(V2_DEFAULT_WINDOW));
    EXPECT_EQ(s.recv_window, static_cast<int32_t>(V2_DEFAULT_WINDOW));
    EXPECT_EQ(s.priority,    16u);
    EXPECT_FALSE(s.is_open());
}

TEST(V2Stream, OpenState) {
    V2Stream s;
    s.state = V2StreamState::OPEN;
    EXPECT_TRUE(s.is_open());
}

TEST(V2Stream, HalfClosedRemoteIsOpen) {
    V2Stream s;
    s.state = V2StreamState::HALF_CLOSED_REMOTE;
    EXPECT_TRUE(s.is_open());
}

TEST(V2Stream, HalfClosedLocalIsNotOpen) {
    V2Stream s;
    s.state = V2StreamState::HALF_CLOSED_LOCAL;
    EXPECT_FALSE(s.is_open());
}

TEST(V2Stream, ClosedIsNotOpen) {
    V2Stream s;
    s.state = V2StreamState::CLOSED;
    EXPECT_FALSE(s.is_open());
}

// ===== V2ConnectionConfig Tests =====

TEST(V2ConnectionConfig, Defaults) {
    V2ConnectionConfig cfg;
    EXPECT_EQ(cfg.max_concurrent_streams, 100u);
    EXPECT_EQ(cfg.initial_window_size,    static_cast<uint32_t>(V2_DEFAULT_WINDOW));
    EXPECT_EQ(cfg.max_frame_size,         static_cast<uint32_t>(V2_MAX_PAYLOAD));
    EXPECT_TRUE(cfg.enable_server_push);
    EXPECT_TRUE(cfg.enable_flow_control);
    EXPECT_TRUE(cfg.enable_lz4_compression);
    EXPECT_EQ(cfg.port,             7890u);
    EXPECT_EQ(cfg.num_io_threads,   4u);
}

// ===== V2FrameType Enum Coverage =====

TEST(V2FrameType, EnumValues) {
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::DATA),          0x00u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::HEADERS),       0x01u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::PRIORITY),      0x02u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::RST_STREAM),    0x03u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::SETTINGS),      0x04u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::PUSH_PROMISE),  0x05u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::PING),          0x06u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::GOAWAY),        0x07u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::WINDOW_UPDATE), 0x08u);
    EXPECT_EQ(static_cast<uint8_t>(V2FrameType::CONTINUATION),  0x09u);
}

// ===== V2Server Construction / Destruction (no real socket) =====

TEST(V2Server, ConstructDestruct) {
    V2ConnectionConfig cfg;
    cfg.port = 0; // OS assigns an ephemeral port – we won't call start()
    EXPECT_NO_THROW({
        V2Server server(cfg);
        EXPECT_FALSE(server.is_running());
    });
}

TEST(V2Server, InitialStatistics) {
    V2ConnectionConfig cfg;
    cfg.port = 0;
    V2Server server(cfg);

    EXPECT_EQ(server.active_connections(),    0u);
    EXPECT_EQ(server.total_streams_opened(),  0u);
    EXPECT_EQ(server.total_frames_sent(),     0u);
    EXPECT_EQ(server.total_frames_received(), 0u);
}

TEST(V2Server, HandlerRegistrationDoesNotThrow) {
    V2ConnectionConfig cfg;
    cfg.port = 0;
    V2Server server(cfg);

    EXPECT_NO_THROW({
        server.set_data_handler(
            [](uint32_t, const std::vector<uint8_t>&, bool) {});
        server.set_headers_handler(
            [](uint32_t, const std::unordered_map<std::string, std::string>&, bool) {});
        server.set_rst_stream_handler(
            [](uint32_t, uint32_t) {});
    });
}
