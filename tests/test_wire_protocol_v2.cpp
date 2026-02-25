/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_wire_protocol_v2.cpp                          ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:59:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     214                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
    EXPECT_FALSE(cfg.enable_zstd_compression);
    EXPECT_EQ(cfg.zstd_compression_level, 3);
    EXPECT_EQ(cfg.min_compression_payload_size, 256u);
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

// ===== V2Stream State Machine Extended Tests =====

TEST(V2Stream, ReservedLocalIsNotOpen) {
    V2Stream s;
    s.state = V2StreamState::RESERVED_LOCAL;
    EXPECT_FALSE(s.is_open());
}

TEST(V2Stream, ReservedRemoteIsNotOpen) {
    V2Stream s;
    s.state = V2StreamState::RESERVED_REMOTE;
    EXPECT_FALSE(s.is_open());
}

TEST(V2Stream, WindowDefaults) {
    V2Stream s;
    EXPECT_EQ(s.send_window, static_cast<int32_t>(V2_DEFAULT_WINDOW));
    EXPECT_EQ(s.recv_window, static_cast<int32_t>(V2_DEFAULT_WINDOW));
}

TEST(V2Stream, PriorityDefault) {
    V2Stream s;
    EXPECT_EQ(s.priority, 16u);
}

// ===== V2FrameFlags Enum Coverage =====

TEST(V2FrameFlags, EnumValues) {
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::NONE),          0x0000u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::END_STREAM),    0x0001u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::END_HEADERS),   0x0004u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::PADDED),        0x0008u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::PRIORITY_FLAG), 0x0020u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::ACK),           0x0001u);
    EXPECT_EQ(static_cast<uint16_t>(V2FrameFlags::COMPRESSED),    0x0100u);
}

// ===== V2SettingId Enum Coverage =====

TEST(V2SettingId, EnumValues) {
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::HEADER_TABLE_SIZE),      0x0001u);
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::ENABLE_PUSH),            0x0002u);
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::MAX_CONCURRENT_STREAMS), 0x0003u);
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::INITIAL_WINDOW_SIZE),    0x0004u);
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::MAX_FRAME_SIZE),         0x0005u);
    EXPECT_EQ(static_cast<uint16_t>(V2SettingId::MAX_HEADER_LIST_SIZE),   0x0006u);
}

// ===== V2ConnectionConfig Custom Values =====

TEST(V2ConnectionConfig, CustomValues) {
    V2ConnectionConfig cfg;
    cfg.max_concurrent_streams  = 200;
    cfg.initial_window_size     = 128 * 1024;
    cfg.max_frame_size          = 8 * 1024 * 1024;
    cfg.enable_server_push      = false;
    cfg.enable_flow_control     = false;
    cfg.enable_lz4_compression  = false;
    cfg.port                    = 9000;
    cfg.num_io_threads          = 8;

    EXPECT_EQ(cfg.max_concurrent_streams, 200u);
    EXPECT_EQ(cfg.initial_window_size,    128u * 1024);
    EXPECT_EQ(cfg.max_frame_size,         8u * 1024 * 1024);
    EXPECT_FALSE(cfg.enable_server_push);
    EXPECT_FALSE(cfg.enable_flow_control);
    EXPECT_FALSE(cfg.enable_lz4_compression);
    EXPECT_EQ(cfg.port,           9000u);
    EXPECT_EQ(cfg.num_io_threads, 8u);
}

// ===== V2FrameHeader – stream_id field =====

TEST(V2FrameHeader, StreamIdZeroIsConnectionLevel) {
    V2FrameHeader h{};
    h.magic      = WIRE_V2_MAGIC;
    h.version    = WIRE_VERSION_2;
    h.stream_id  = 0;
    EXPECT_TRUE(h.is_valid());
    EXPECT_EQ(h.stream_id, 0u);
}

TEST(V2FrameHeader, MaxStreamId) {
    V2FrameHeader h{};
    h.magic     = WIRE_V2_MAGIC;
    h.version   = WIRE_VERSION_2;
    h.stream_id = V2_MAX_STREAM_ID;
    EXPECT_TRUE(h.is_valid());
    EXPECT_EQ(h.stream_id, V2_MAX_STREAM_ID);
}

TEST(V2FrameHeader, PayloadLength) {
    V2FrameHeader h{};
    h.magic          = WIRE_V2_MAGIC;
    h.version        = WIRE_VERSION_2;
    h.payload_length = 1024u;
    EXPECT_EQ(h.payload_length, 1024u);
}

// ===== Multiple logical stream IDs are independent =====

TEST(V2Stream, IndependentStreams) {
    V2Stream s1, s2;
    s1.stream_id = 1;  s1.state = V2StreamState::OPEN;
    s2.stream_id = 3;  s2.state = V2StreamState::HALF_CLOSED_REMOTE;

    EXPECT_TRUE(s1.is_open());
    EXPECT_TRUE(s2.is_open());
    EXPECT_NE(s1.stream_id, s2.stream_id);
}

TEST(V2Stream, HalfClosedTransitionFromOpen) {
    V2Stream s;
    s.stream_id = 5;
    s.state     = V2StreamState::OPEN;
    EXPECT_TRUE(s.is_open());

    // Simulate local side sending END_STREAM
    s.state = V2StreamState::HALF_CLOSED_LOCAL;
    EXPECT_FALSE(s.is_open()); // local half-close is not "open" per is_open()

    // Simulate remote side sending END_STREAM -> fully closed
    s.state = V2StreamState::CLOSED;
    EXPECT_FALSE(s.is_open());
}

// ===== V2Server push_to_client on non-existent connection =====

TEST(V2Server, PushToNonexistentConnectionReturnsFalse) {
    V2ConnectionConfig cfg;
    cfg.port = 0;
    V2Server server(cfg);

    bool result = server.push_to_client("no-such-conn", 1u, {}, {});
    EXPECT_FALSE(result);
}

// ===== Audit fix: aggregate frame statistics start at zero =====

TEST(V2Server, AggregateFrameStatsInitiallyZero) {
    V2ConnectionConfig cfg;
    cfg.port = 0;
    V2Server server(cfg);

    // Before any connection, aggregate stats must be 0
    EXPECT_EQ(server.total_frames_sent(),     0u);
    EXPECT_EQ(server.total_frames_received(), 0u);
    EXPECT_EQ(server.total_streams_opened(),  0u);
    EXPECT_EQ(server.active_connections(),    0u);
}

// ===== Audit fix: V2FrameFlags::COMPRESSED is defined and distinguishable =====

TEST(V2FrameHeader, CompressedFlagRoundtrip) {
    V2FrameHeader h{};
    h.magic      = WIRE_V2_MAGIC;
    h.version    = WIRE_VERSION_2;
    h.frame_type = static_cast<uint8_t>(V2FrameType::DATA);
    h.stream_id  = 7u;
    h.flags      = static_cast<uint16_t>(V2FrameFlags::COMPRESSED);

    EXPECT_TRUE(h.is_valid());
    EXPECT_TRUE(h.has_flag(V2FrameFlags::COMPRESSED));
    EXPECT_FALSE(h.has_flag(V2FrameFlags::END_STREAM));
    EXPECT_EQ(h.get_type(), V2FrameType::DATA);
}
