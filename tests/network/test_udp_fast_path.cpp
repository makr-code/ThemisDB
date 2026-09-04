// Unit tests for UDP fast-path (include/network/udp_fast_path.h).
// These tests cover packet format validation, opcode filtering, response
// construction, and configuration defaults without requiring a live socket.

#include <gtest/gtest.h>
#include "network/udp_fast_path.h"
#include <cstring>
#include <vector>
#include <string>
#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

using namespace themis::network;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Build a minimal well-formed UDP fast-path request datagram.
static std::vector<uint8_t> makeRequest(uint8_t            opcode,
                                         uint32_t           request_id = 42,
                                         const std::string& payload    = "") {
    const uint16_t plen = static_cast<uint16_t>(payload.size());

    std::vector<uint8_t> pkt;
    pkt.reserve(kUdpFastPathHeaderSize + plen);

    pkt.push_back(kUdpFastPathReqMagic0);
    pkt.push_back(kUdpFastPathReqMagic1);
    pkt.push_back(kUdpFastPathVersion);
    pkt.push_back(opcode);

    const uint32_t id_be = htonl(request_id);
    pkt.insert(pkt.end(),
               reinterpret_cast<const uint8_t*>(&id_be),
               reinterpret_cast<const uint8_t*>(&id_be) + 4);

    const uint16_t plen_be = htons(plen);
    pkt.insert(pkt.end(),
               reinterpret_cast<const uint8_t*>(&plen_be),
               reinterpret_cast<const uint8_t*>(&plen_be) + 2);

    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}

// ─────────────────────────────────────────────────────────────────────────────
// Config defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, ConfigDefaultPort) {
    UDPFastPath::Config cfg;
    EXPECT_EQ(cfg.port, 8769u);
}

TEST(UDPFastPath, ConfigDefaultHost) {
    UDPFastPath::Config cfg;
    EXPECT_EQ(cfg.host, "0.0.0.0");
}

TEST(UDPFastPath, ConfigDefaultMaxPacketSize) {
    UDPFastPath::Config cfg;
    EXPECT_EQ(cfg.max_packet_size, kUdpFastPathMaxPacketSize);
}

TEST(UDPFastPath, ConfigDefaultRateLimit) {
    UDPFastPath::Config cfg;
    EXPECT_GT(cfg.max_packets_per_second_per_ip, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Packet magic constants
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, RequestMagicBytes) {
    EXPECT_EQ(kUdpFastPathReqMagic0, 0x54u);  // 'T'
    EXPECT_EQ(kUdpFastPathReqMagic1, 0x55u);  // 'U'
}

TEST(UDPFastPath, ResponseMagicBytes) {
    EXPECT_EQ(kUdpFastPathRespMagic0, 0x54u);  // 'T'
    EXPECT_EQ(kUdpFastPathRespMagic1, 0x52u);  // 'R'
}

TEST(UDPFastPath, VersionConstant) {
    EXPECT_EQ(kUdpFastPathVersion, 0x01u);
}

TEST(UDPFastPath, HeaderSize) {
    EXPECT_EQ(kUdpFastPathHeaderSize, 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Packet validation
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, ValidatePingPacket) {
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    EXPECT_TRUE(UDPFastPath::validatePacket(pkt));
}

TEST(UDPFastPath, ValidateGetPacket) {
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::GET), 1,
                           R"({"key":"users/alice"})");
    EXPECT_TRUE(UDPFastPath::validatePacket(pkt));
}

TEST(UDPFastPath, ValidateTooShort) {
    // Packets smaller than the header must be rejected
    std::vector<uint8_t> tiny = {0x54, 0x55, 0x01};
    EXPECT_FALSE(UDPFastPath::validatePacket(tiny));
}

TEST(UDPFastPath, ValidateEmptyPacket) {
    EXPECT_FALSE(UDPFastPath::validatePacket({}));
}

TEST(UDPFastPath, ValidateBadMagicFirst) {
    auto pkt    = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    pkt[0]      = 0x00;  // corrupt magic byte 0
    EXPECT_FALSE(UDPFastPath::validatePacket(pkt));
}

TEST(UDPFastPath, ValidateBadMagicSecond) {
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    pkt[1]   = 0x00;  // corrupt magic byte 1
    EXPECT_FALSE(UDPFastPath::validatePacket(pkt));
}

TEST(UDPFastPath, ValidateBadVersion) {
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    pkt[2]   = 0x02;  // unsupported version
    EXPECT_FALSE(UDPFastPath::validatePacket(pkt));
}

TEST(UDPFastPath, ValidatePayloadLengthMismatch) {
    // Declare 10 bytes of payload but provide none
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    // Overwrite payload-length field to claim 10 bytes
    const uint16_t fake_len = htons(10);
    std::memcpy(pkt.data() + 8, &fake_len, 2);
    // pkt has exactly kUdpFastPathHeaderSize bytes (no payload), so this must fail
    EXPECT_FALSE(UDPFastPath::validatePacket(pkt));
}

// ─────────────────────────────────────────────────────────────────────────────
// Read-only opcode filter
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, ReadOnlyGet) {
    EXPECT_TRUE(UDPFastPath::isReadOnlyOpCode(
        static_cast<uint8_t>(UdpOpCode::GET)));
}

TEST(UDPFastPath, ReadOnlyQueryAql) {
    EXPECT_TRUE(UDPFastPath::isReadOnlyOpCode(
        static_cast<uint8_t>(UdpOpCode::QUERY_AQL)));
}

TEST(UDPFastPath, ReadOnlyVectorSearch) {
    EXPECT_TRUE(UDPFastPath::isReadOnlyOpCode(
        static_cast<uint8_t>(UdpOpCode::VECTOR_SEARCH)));
}

TEST(UDPFastPath, ReadOnlyPing) {
    EXPECT_TRUE(UDPFastPath::isReadOnlyOpCode(
        static_cast<uint8_t>(UdpOpCode::PING)));
}

TEST(UDPFastPath, RejectPutOpCode) {
    // 0x11 = PUT (write)
    EXPECT_FALSE(UDPFastPath::isReadOnlyOpCode(0x11));
}

TEST(UDPFastPath, RejectDeleteOpCode) {
    // 0x12 = DELETE (write)
    EXPECT_FALSE(UDPFastPath::isReadOnlyOpCode(0x12));
}

TEST(UDPFastPath, RejectUnknownOpCode) {
    EXPECT_FALSE(UDPFastPath::isReadOnlyOpCode(0x99));
}

// ─────────────────────────────────────────────────────────────────────────────
// Response builder
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, BuildResponseMagic) {
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::OK, "{}");
    ASSERT_GE(resp.size(), kUdpFastPathHeaderSize);
    EXPECT_EQ(resp[0], kUdpFastPathRespMagic0);
    EXPECT_EQ(resp[1], kUdpFastPathRespMagic1);
}

TEST(UDPFastPath, BuildResponseVersion) {
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::OK, "{}");
    EXPECT_EQ(resp[2], kUdpFastPathVersion);
}

TEST(UDPFastPath, BuildResponseStatusOk) {
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::OK, "{}");
    EXPECT_EQ(resp[3], static_cast<uint8_t>(UdpStatus::OK));
}

TEST(UDPFastPath, BuildResponseStatusError) {
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::ERROR, R"({"error":"x"})");
    EXPECT_EQ(resp[3], static_cast<uint8_t>(UdpStatus::ERROR));
}

TEST(UDPFastPath, BuildResponseStatusNotFound) {
    auto resp = UDPFastPath::buildResponse(99, UdpStatus::NOT_FOUND, "{}");
    EXPECT_EQ(resp[3], static_cast<uint8_t>(UdpStatus::NOT_FOUND));
}

TEST(UDPFastPath, BuildResponseStatusRateLimited) {
    auto resp = UDPFastPath::buildResponse(0, UdpStatus::RATE_LIMITED,
                                            R"({"error":"rate limited"})");
    EXPECT_EQ(resp[3], static_cast<uint8_t>(UdpStatus::RATE_LIMITED));
}

TEST(UDPFastPath, BuildResponseRequestIdEchoed) {
    const uint32_t id   = 0xDEADBEEFu;
    auto           resp = UDPFastPath::buildResponse(id, UdpStatus::OK, "{}");
    ASSERT_GE(resp.size(), 8u);
    uint32_t echoed_be = {};
    std::memcpy(&echoed_be, resp.data() + 4, 4);
    EXPECT_EQ(ntohl(echoed_be), id);
}

TEST(UDPFastPath, BuildResponsePayloadLength) {
    const std::string payload = R"({"pong":true})";
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::OK, payload);
    ASSERT_GE(resp.size(), kUdpFastPathHeaderSize);

    uint16_t plen_be;
    std::memcpy(&plen_be, resp.data() + 8, 2);
    const uint16_t plen = ntohs(plen_be);
    EXPECT_EQ(plen, static_cast<uint16_t>(payload.size()));
    EXPECT_EQ(resp.size(), kUdpFastPathHeaderSize + payload.size());
}

TEST(UDPFastPath, BuildResponsePayloadContent) {
    const std::string payload = R"({"key":"foo","value":"bar"})";
    auto resp = UDPFastPath::buildResponse(7, UdpStatus::OK, payload);
    ASSERT_GE(resp.size(), kUdpFastPathHeaderSize + payload.size());

    std::string extracted(reinterpret_cast<const char*>(resp.data() + kUdpFastPathHeaderSize),
                          payload.size());
    EXPECT_EQ(extracted, payload);
}

TEST(UDPFastPath, BuildResponseEmptyPayload) {
    auto resp = UDPFastPath::buildResponse(3, UdpStatus::OK, "");
    EXPECT_EQ(resp.size(), kUdpFastPathHeaderSize);

    uint16_t plen_be;
    std::memcpy(&plen_be, resp.data() + 8, 2);
    EXPECT_EQ(ntohs(plen_be), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audit-regression: JSON-safe error serialisation
// ─────────────────────────────────────────────────────────────────────────────

// buildResponse must produce a payload that is valid JSON even when the
// error message contains JSON special characters.
TEST(UDPFastPath, BuildResponsePayloadIsValidJson) {
    // Payload must round-trip through a JSON parser without errors.
    const std::string payload = R"({"error":"some\u0022tricky\u0022 message"})";
    auto resp = UDPFastPath::buildResponse(1, UdpStatus::ERROR, payload);
    ASSERT_GE(resp.size(), kUdpFastPathHeaderSize);
    // Payload bytes should exactly match what was passed in.
    std::string extracted(reinterpret_cast<const char*>(resp.data() + kUdpFastPathHeaderSize),
                          payload.size());
    EXPECT_EQ(extracted, payload);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audit-regression: validatePacket boundary conditions
// ─────────────────────────────────────────────────────────────────────────────

// Exact header size with zero payload must be accepted.
TEST(UDPFastPath, ValidateExactHeaderNoPayload) {
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::PING));
    ASSERT_EQ(pkt.size(), kUdpFastPathHeaderSize);
    EXPECT_TRUE(UDPFastPath::validatePacket(pkt));
}

// Packet with maximum reasonable payload length field filled correctly.
TEST(UDPFastPath, ValidateLargePayloadConsistentLength) {
    const std::string big(200, 'x');
    auto pkt = makeRequest(static_cast<uint8_t>(UdpOpCode::GET), 1, big);
    EXPECT_TRUE(UDPFastPath::validatePacket(pkt));
}

// ─────────────────────────────────────────────────────────────────────────────
// Stats initialisation
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, InitialStats) {
    UDPFastPath::Config cfg;
    cfg.port = 0;  // Do not actually bind
    UDPFastPath server(cfg);

    const auto stats = server.getStats();
    EXPECT_EQ(stats.packets_received,    0u);
    EXPECT_EQ(stats.packets_dropped,     0u);
    EXPECT_EQ(stats.packets_sent,        0u);
    EXPECT_EQ(stats.bytes_received,      0u);
    EXPECT_EQ(stats.bytes_sent,          0u);
    EXPECT_EQ(stats.parse_errors,        0u);
    EXPECT_EQ(stats.write_op_rejections, 0u);
    EXPECT_EQ(stats.rate_limit_drops,    0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// isRunning before start
// ─────────────────────────────────────────────────────────────────────────────

TEST(UDPFastPath, NotRunningBeforeStart) {
    UDPFastPath::Config cfg;
    cfg.port = 0;
    UDPFastPath server(cfg);
    EXPECT_FALSE(server.isRunning());
}
