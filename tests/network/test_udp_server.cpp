// Unit tests for the UDP ingestion server (include/network/udp_server.h).
// These tests exercise the static packet helpers, opcode/constant values,
// configuration defaults, and statistics without requiring a live socket.

#include <gtest/gtest.h>
#include "network/udp_server.h"

#include <cstring>
#include <string>
#include <vector>
#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Packet-builder helper
// ─────────────────────────────────────────────────────────────────────────────

/// Build a well-formed UDP ingestion server request datagram.
static std::vector<uint8_t> makePacket(uint8_t            opcode,
                                        uint32_t           seq_num  = 1,
                                        uint8_t            flags    = 0,
                                        const std::string& payload  = "") {
    const uint16_t plen = static_cast<uint16_t>(payload.size());

    std::vector<uint8_t> pkt;
    pkt.reserve(kUdpServerHeaderSize + plen);

    pkt.push_back(kUdpServerMagic0);
    pkt.push_back(kUdpServerMagic1);
    pkt.push_back(kUdpServerVersion);
    pkt.push_back(opcode);

    const uint32_t seq_be = htonl(seq_num);
    pkt.insert(pkt.end(),
               reinterpret_cast<const uint8_t*>(&seq_be),
               reinterpret_cast<const uint8_t*>(&seq_be) + 4);

    pkt.push_back(flags);

    const uint16_t plen_be = htons(plen);
    pkt.insert(pkt.end(),
               reinterpret_cast<const uint8_t*>(&plen_be),
               reinterpret_cast<const uint8_t*>(&plen_be) + 2);

    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
// Packet-format constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, RequestMagicByte0) {
    EXPECT_EQ(kUdpServerMagic0, 0x54u);  // 'T'
}

TEST(UDPServer, RequestMagicByte1) {
    EXPECT_EQ(kUdpServerMagic1, 0x4Du);  // 'M'
}

TEST(UDPServer, AckMagicByte0) {
    EXPECT_EQ(kUdpServerAckMagic0, 0x54u);  // 'T'
}

TEST(UDPServer, AckMagicByte1) {
    EXPECT_EQ(kUdpServerAckMagic1, 0x41u);  // 'A'
}

TEST(UDPServer, VersionConstant) {
    EXPECT_EQ(kUdpServerVersion, 0x01u);
}

TEST(UDPServer, HeaderSizeValue) {
    EXPECT_EQ(kUdpServerHeaderSize, 11u);
}

TEST(UDPServer, AckSizeValue) {
    EXPECT_EQ(kUdpServerAckSize, 8u);
}

TEST(UDPServer, MaxPacketSizeValue) {
    EXPECT_EQ(kUdpServerMaxPacketSize, 65507u);
}

TEST(UDPServer, FlagAckRequestedBit) {
    EXPECT_EQ(kUdpServerFlagAckRequested, 0x01u);
}

// ─────────────────────────────────────────────────────────────────────────────
// OpCode enum values
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, OpCodeMetric) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerOpCode::METRIC), 0x01u);
}

TEST(UDPServer, OpCodeLog) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerOpCode::LOG), 0x02u);
}

TEST(UDPServer, OpCodeEvent) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerOpCode::EVENT), 0x03u);
}

TEST(UDPServer, OpCodeBatch) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerOpCode::BATCH), 0x04u);
}

TEST(UDPServer, OpCodePing) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerOpCode::PING), 0xFEu);
}

// ─────────────────────────────────────────────────────────────────────────────
// Status enum values
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, StatusOk) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerStatus::OK), 0x00u);
}

TEST(UDPServer, StatusError) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerStatus::ERROR), 0x01u);
}

TEST(UDPServer, StatusRateLimited) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerStatus::RATE_LIMITED), 0x02u);
}

TEST(UDPServer, StatusDuplicate) {
    EXPECT_EQ(static_cast<uint8_t>(UdpServerStatus::DUPLICATE), 0x03u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Config defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, ConfigDefaultPort) {
    UDPServer::Config cfg;
    EXPECT_EQ(cfg.port, 8768u);
}

TEST(UDPServer, ConfigDefaultHost) {
    UDPServer::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(UDPServer, ConfigDefaultNumThreads) {
    UDPServer::Config cfg;
    EXPECT_EQ(cfg.num_threads, 4u);
}

TEST(UDPServer, ConfigDefaultMaxPacketSize) {
    UDPServer::Config cfg;
    EXPECT_EQ(cfg.max_packet_size, kUdpServerMaxPacketSize);
}

TEST(UDPServer, ConfigDefaultEnableBatching) {
    UDPServer::Config cfg;
    EXPECT_TRUE(cfg.enable_batching);
}

TEST(UDPServer, ConfigDefaultBatchIntervalMs) {
    UDPServer::Config cfg;
    EXPECT_EQ(cfg.batch_interval_ms, 100u);
}

TEST(UDPServer, ConfigDefaultEnableAcks) {
    UDPServer::Config cfg;
    EXPECT_FALSE(cfg.enable_acks);
}

TEST(UDPServer, ConfigDefaultRateLimit) {
    UDPServer::Config cfg;
    EXPECT_GT(cfg.max_packets_per_second_per_ip, 0u);
}

TEST(UDPServer, ConfigDefaultDedupWindowSize) {
    UDPServer::Config cfg;
    EXPECT_GT(cfg.dedup_window_size, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// validatePacket
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, ValidateMetricPacket) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC), 1, 0,
                          R"({"name":"cpu.usage","value":85.5})");
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateLogPacket) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::LOG), 2, 0,
                          R"({"level":"INFO","msg":"hello"})");
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateEventPacket) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::EVENT), 3);
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateBatchPacket) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::BATCH), 4);
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidatePingPacket) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::PING));
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateTooShort) {
    std::vector<uint8_t> tiny = {0x54, 0x4D, 0x01};
    EXPECT_FALSE(UDPServer::validatePacket(tiny));
}

TEST(UDPServer, ValidateEmptyPacket) {
    EXPECT_FALSE(UDPServer::validatePacket({}));
}

TEST(UDPServer, ValidateBadMagicFirst) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::PING));
    pkt[0] = 0x00;
    EXPECT_FALSE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateBadMagicSecond) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::PING));
    pkt[1] = 0x00;
    EXPECT_FALSE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateBadVersion) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::PING));
    pkt[2] = 0x02;  // unsupported version
    EXPECT_FALSE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidatePayloadLengthMismatch) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC));
    // Claim 20 bytes of payload but the packet has none
    const uint16_t fake_plen = htons(20);
    std::memcpy(pkt.data() + 9, &fake_plen, 2);
    EXPECT_FALSE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateExactHeaderNoPayload) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::PING));
    ASSERT_EQ(pkt.size(), kUdpServerHeaderSize);
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

TEST(UDPServer, ValidateLargePayloadConsistentLength) {
    const std::string big(500, 'x');
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::BATCH), 1, 0, big);
    EXPECT_TRUE(UDPServer::validatePacket(pkt));
}

// ─────────────────────────────────────────────────────────────────────────────
// buildAck
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, AckSize) {
    auto ack = UDPServer::buildAck(1, UdpServerStatus::OK);
    EXPECT_EQ(ack.size(), kUdpServerAckSize);
}

TEST(UDPServer, AckMagic) {
    auto ack = UDPServer::buildAck(1, UdpServerStatus::OK);
    EXPECT_EQ(ack[0], kUdpServerAckMagic0);
    EXPECT_EQ(ack[1], kUdpServerAckMagic1);
}

TEST(UDPServer, AckVersion) {
    auto ack = UDPServer::buildAck(1, UdpServerStatus::OK);
    EXPECT_EQ(ack[2], kUdpServerVersion);
}

TEST(UDPServer, AckStatusOk) {
    auto ack = UDPServer::buildAck(1, UdpServerStatus::OK);
    EXPECT_EQ(ack[3], static_cast<uint8_t>(UdpServerStatus::OK));
}

TEST(UDPServer, AckStatusError) {
    auto ack = UDPServer::buildAck(1, UdpServerStatus::ERROR);
    EXPECT_EQ(ack[3], static_cast<uint8_t>(UdpServerStatus::ERROR));
}

TEST(UDPServer, AckStatusRateLimited) {
    auto ack = UDPServer::buildAck(99, UdpServerStatus::RATE_LIMITED);
    EXPECT_EQ(ack[3], static_cast<uint8_t>(UdpServerStatus::RATE_LIMITED));
}

TEST(UDPServer, AckStatusDuplicate) {
    auto ack = UDPServer::buildAck(7, UdpServerStatus::DUPLICATE);
    EXPECT_EQ(ack[3], static_cast<uint8_t>(UdpServerStatus::DUPLICATE));
}

TEST(UDPServer, AckSeqNumEchoed) {
    const uint32_t seq  = 0xCAFEBABEu;
    auto           ack  = UDPServer::buildAck(seq, UdpServerStatus::OK);
    ASSERT_GE(ack.size(), 8u);
    uint32_t echoed_be;
    std::memcpy(&echoed_be, ack.data() + 4, 4);
    EXPECT_EQ(ntohl(echoed_be), seq);
}

TEST(UDPServer, AckSeqNumZero) {
    auto ack = UDPServer::buildAck(0, UdpServerStatus::OK);
    uint32_t echoed_be;
    std::memcpy(&echoed_be, ack.data() + 4, 4);
    EXPECT_EQ(ntohl(echoed_be), 0u);
}

TEST(UDPServer, AckSeqNumMaxUint32) {
    const uint32_t seq = 0xFFFFFFFFu;
    auto ack = UDPServer::buildAck(seq, UdpServerStatus::OK);
    uint32_t echoed_be;
    std::memcpy(&echoed_be, ack.data() + 4, 4);
    EXPECT_EQ(ntohl(echoed_be), seq);
}

// ─────────────────────────────────────────────────────────────────────────────
// ACK flag in request packet
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, FlagAckRequestedCanBeSet) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC), 1,
                          kUdpServerFlagAckRequested);
    ASSERT_GE(pkt.size(), kUdpServerHeaderSize);
    EXPECT_NE(pkt[8] & kUdpServerFlagAckRequested, 0);
}

TEST(UDPServer, FlagAckRequestedNotSetByDefault) {
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC));
    ASSERT_GE(pkt.size(), kUdpServerHeaderSize);
    EXPECT_EQ(pkt[8] & kUdpServerFlagAckRequested, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Initial statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, InitialStatsAllZero) {
    UDPServer::Config cfg;
    cfg.port = 0;  // Do not actually bind
    UDPServer server(cfg);

    const auto stats = server.getStats();
    EXPECT_EQ(stats.packets_received,  0u);
    EXPECT_EQ(stats.packets_dropped,   0u);
    EXPECT_EQ(stats.bytes_received,    0u);
    EXPECT_EQ(stats.parse_errors,      0u);
    EXPECT_EQ(stats.rate_limit_drops,  0u);
    EXPECT_EQ(stats.duplicate_drops,   0u);
    EXPECT_EQ(stats.acks_sent,         0u);
    EXPECT_EQ(stats.metrics_ingested,  0u);
    EXPECT_EQ(stats.logs_ingested,     0u);
    EXPECT_EQ(stats.events_ingested,   0u);
    EXPECT_EQ(stats.batches_ingested,  0u);
    EXPECT_EQ(stats.pings_answered,    0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning before start
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, NotRunningBeforeStart) {
    UDPServer::Config cfg;
    cfg.port = 0;
    UDPServer server(cfg);
    EXPECT_FALSE(server.isRunning());
}

// ─────────────────────────────────────────────────────────────────────────────
// Sequence-number offset in header
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, SeqNumEncodedBigEndian) {
    const uint32_t seq = 0x01020304u;
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC), seq);

    // SeqNum is at offset 4, big-endian
    ASSERT_GE(pkt.size(), kUdpServerHeaderSize);
    uint32_t decoded_be;
    std::memcpy(&decoded_be, pkt.data() + 4, 4);
    EXPECT_EQ(ntohl(decoded_be), seq);
}

// ─────────────────────────────────────────────────────────────────────────────
// Payload encoding in request
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, PayloadLengthEncodedBigEndian) {
    const std::string payload = R"({"name":"cpu.usage","value":85.5,"ts":1700000000})";
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC), 1, 0, payload);

    ASSERT_GE(pkt.size(), kUdpServerHeaderSize);
    uint16_t plen_be;
    std::memcpy(&plen_be, pkt.data() + 9, 2);
    const uint16_t plen = ntohs(plen_be);
    EXPECT_EQ(plen, static_cast<uint16_t>(payload.size()));
}

TEST(UDPServer, PayloadContentPreserved) {
    const std::string payload = R"({"name":"cpu.usage","value":85.5})";
    auto pkt = makePacket(static_cast<uint8_t>(UdpServerOpCode::METRIC), 1, 0, payload);

    ASSERT_GE(pkt.size(), kUdpServerHeaderSize + payload.size());
    std::string extracted(reinterpret_cast<const char*>(pkt.data() + kUdpServerHeaderSize),
                          payload.size());
    EXPECT_EQ(extracted, payload);
}

// ─────────────────────────────────────────────────────────────────────────────
// Magic-byte distinctness from fast-path ("TU") and ACK ("TA") protocols
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPServer, MagicDistinctFromFastPathMagic) {
    // UDP fast-path uses "TU" (0x54, 0x55); ingestion server uses "TM" (0x54, 0x4D)
    EXPECT_NE(kUdpServerMagic1, 0x55u);
}

TEST(UDPServer, AckMagicDistinctFromRequestMagic) {
    EXPECT_NE(kUdpServerAckMagic1, kUdpServerMagic1);
}
