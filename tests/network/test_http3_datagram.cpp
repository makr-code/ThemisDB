// Unit tests for HTTP/3 Datagram support (include/server/http3_datagram.h).
//
// Tests validate:
//   - QUIC variable-length integer (varint) encoding/decoding (RFC 9000 §16)
//   - HTTP/3 datagram frame encoding (RFC 9297 §2)
//   - Context registration and unregistration
//   - Dispatch routing to registered handlers
//   - Drop counting for unknown / malformed datagrams
//   - Statistics tracking
//   - Config defaults

#include <gtest/gtest.h>

#ifdef THEMIS_ENABLE_HTTP3

#include "server/http3_datagram.h"
#include <cstring>
#include <vector>

using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Configuration defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, DefaultConfigEnabled) {
    Http3DatagramConfig cfg;
    EXPECT_TRUE(cfg.enable);
}

TEST(Http3DatagramTest, DefaultMaxDatagramFrameSize) {
    Http3DatagramConfig cfg;
    EXPECT_EQ(cfg.max_datagram_frame_size, 65535u);
}

// ─────────────────────────────────────────────────────────────────────────────
// QUIC varint encoding (RFC 9000 §16)
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, VarintEncode1Byte) {
    uint8_t buf[8];
    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(0, buf), 1u);
    EXPECT_EQ(buf[0], 0x00);

    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(63, buf), 1u);
    EXPECT_EQ(buf[0], 0x3Fu);
}

TEST(Http3DatagramTest, VarintEncode2Bytes) {
    uint8_t buf[8];
    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(64, buf), 2u);
    // 64 (0x0040) in 2-byte encoding:
    //   byte 0 = 0x40 | (64 >> 8) = 0x40 (prefix 01, upper 6 bits = 0)
    //   byte 1 = 64 & 0xFF        = 0x40 (lower 8 bits)
    EXPECT_EQ(buf[0] & 0xC0, 0x40u);  // prefix = 01

    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(16383, buf), 2u);
    EXPECT_EQ(buf[0] & 0xC0, 0x40u);
}

TEST(Http3DatagramTest, VarintEncode4Bytes) {
    uint8_t buf[8];
    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(16384, buf), 4u);
    EXPECT_EQ(buf[0] & 0xC0, 0x80u);  // prefix = 10
}

TEST(Http3DatagramTest, VarintEncode8Bytes) {
    uint8_t buf[8];
    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(1073741824ULL, buf), 8u);
    EXPECT_EQ(buf[0] & 0xC0, 0xC0u);  // prefix = 11
}

TEST(Http3DatagramTest, VarintEncodeOutOfRange) {
    uint8_t buf[8];
    // 2^62 is out of range for QUIC varint
    EXPECT_EQ(Http3DatagramDispatcher::encodeVarint(
        4611686018427387904ULL, buf), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// QUIC varint decoding
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, VarintDecode1Byte) {
    uint8_t buf[] = {0x05};
    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 1, val), 1u);
    EXPECT_EQ(val, 5u);
}

TEST(Http3DatagramTest, VarintDecode1ByteZero) {
    uint8_t buf[] = {0x00};
    uint64_t val = 99;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 1, val), 1u);
    EXPECT_EQ(val, 0u);
}

TEST(Http3DatagramTest, VarintDecode2Bytes) {
    // Encode 100 then decode
    uint8_t buf[8];
    size_t n = Http3DatagramDispatcher::encodeVarint(100, buf);
    ASSERT_EQ(n, 2u);

    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 2, val), 2u);
    EXPECT_EQ(val, 100u);
}

TEST(Http3DatagramTest, VarintDecode4Bytes) {
    uint8_t buf[8];
    size_t n = Http3DatagramDispatcher::encodeVarint(100000, buf);
    ASSERT_EQ(n, 4u);

    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 4, val), 4u);
    EXPECT_EQ(val, 100000u);
}

TEST(Http3DatagramTest, VarintDecode8Bytes) {
    uint8_t buf[8];
    size_t n = Http3DatagramDispatcher::encodeVarint(1073741824ULL, buf);
    ASSERT_EQ(n, 8u);

    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 8, val), 8u);
    EXPECT_EQ(val, 1073741824ULL);
}

TEST(Http3DatagramTest, VarintDecodeBufferTooShort) {
    uint8_t buf[8];
    // Encode a 2-byte value but only provide 1 byte
    Http3DatagramDispatcher::encodeVarint(100, buf);
    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(buf, 1, val), 0u);
}

TEST(Http3DatagramTest, VarintDecodeEmpty) {
    uint64_t val = 0;
    EXPECT_EQ(Http3DatagramDispatcher::decodeVarint(nullptr, 0, val), 0u);
}

TEST(Http3DatagramTest, VarintRoundtrip) {
    const std::vector<uint64_t> values = {0, 1, 63, 64, 16383, 16384,
                                          1073741823ULL, 1073741824ULL,
                                          4611686018427387903ULL};
    for (uint64_t v : values) {
        uint8_t buf[8];
        size_t enc_len = Http3DatagramDispatcher::encodeVarint(v, buf);
        ASSERT_GT(enc_len, 0u) << "encodeVarint failed for value " << v;

        uint64_t decoded = 0;
        size_t dec_len = Http3DatagramDispatcher::decodeVarint(buf, enc_len, decoded);
        EXPECT_EQ(dec_len, enc_len) << "for value " << v;
        EXPECT_EQ(decoded, v) << "roundtrip failed for value " << v;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame encoding (RFC 9297 §2)
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, EncodeContextId0WithPayload) {
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto frame = Http3DatagramDispatcher::encode(0, payload, sizeof(payload));

    ASSERT_EQ(frame.size(), 5u);  // 1-byte varint(0) + 4-byte payload
    EXPECT_EQ(frame[0], 0x00);    // context_id = 0 encoded as 1-byte varint
    EXPECT_EQ(frame[1], 0xDE);
    EXPECT_EQ(frame[2], 0xAD);
    EXPECT_EQ(frame[3], 0xBE);
    EXPECT_EQ(frame[4], 0xEF);
}

TEST(Http3DatagramTest, EncodeContextId1WithPayload) {
    const uint8_t payload[] = {0x01, 0x02};
    auto frame = Http3DatagramDispatcher::encode(1, payload, sizeof(payload));

    ASSERT_EQ(frame.size(), 3u);
    EXPECT_EQ(frame[0], 0x01);  // context_id = 1 → 1-byte varint
    EXPECT_EQ(frame[1], 0x01);
    EXPECT_EQ(frame[2], 0x02);
}

TEST(Http3DatagramTest, EncodeNullPayload) {
    auto frame = Http3DatagramDispatcher::encode(5, nullptr, 0);
    // 1-byte varint(5) + empty payload
    ASSERT_EQ(frame.size(), 1u);
    EXPECT_EQ(frame[0], 0x05);
}

TEST(Http3DatagramTest, EncodeOutOfRangeContextId) {
    // 2^62 is not encodable → returns empty frame
    const uint8_t payload[] = {0x00};
    auto frame = Http3DatagramDispatcher::encode(
        4611686018427387904ULL, payload, 1);
    EXPECT_TRUE(frame.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Context registration
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, RegisterContext) {
    Http3DatagramDispatcher d;
    bool ok = d.registerContext(1, [](uint64_t, const uint8_t*, size_t) {});
    EXPECT_TRUE(ok);
    EXPECT_TRUE(d.hasContext(1));
}

TEST(Http3DatagramTest, RegisterContextReplacesExisting) {
    Http3DatagramDispatcher d;
    int call_count = 0;
    d.registerContext(1, [&](uint64_t, const uint8_t*, size_t) { ++call_count; });
    // Replace with a different handler
    d.registerContext(1, [&](uint64_t, const uint8_t*, size_t) { call_count += 10; });

    // Trigger dispatch
    auto frame = Http3DatagramDispatcher::encode(1, nullptr, 0);
    d.dispatch(frame.data(), frame.size());

    EXPECT_EQ(call_count, 10);
}

TEST(Http3DatagramTest, RegisterContextDisabled) {
    Http3DatagramConfig cfg;
    cfg.enable = false;
    Http3DatagramDispatcher d(cfg);

    bool ok = d.registerContext(1, [](uint64_t, const uint8_t*, size_t) {});
    EXPECT_FALSE(ok);
    EXPECT_FALSE(d.hasContext(1));
}

TEST(Http3DatagramTest, UnregisterContext) {
    Http3DatagramDispatcher d;
    d.registerContext(2, [](uint64_t, const uint8_t*, size_t) {});
    EXPECT_TRUE(d.hasContext(2));

    EXPECT_TRUE(d.unregisterContext(2));
    EXPECT_FALSE(d.hasContext(2));
}

TEST(Http3DatagramTest, UnregisterNonExistentContext) {
    Http3DatagramDispatcher d;
    EXPECT_FALSE(d.unregisterContext(99));
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, DispatchRoutesToHandler) {
    Http3DatagramDispatcher d;
    uint64_t received_ctx = 0;
    std::vector<uint8_t> received_data;

    d.registerContext(3, [&](uint64_t ctx, const uint8_t* data, size_t len) {
        received_ctx = ctx;
        received_data.assign(data, data + len);
    });

    const uint8_t payload[] = {0x11, 0x22, 0x33};
    auto frame = Http3DatagramDispatcher::encode(3, payload, sizeof(payload));
    d.dispatch(frame.data(), frame.size());

    EXPECT_EQ(received_ctx, 3u);
    ASSERT_EQ(received_data.size(), 3u);
    EXPECT_EQ(received_data[0], 0x11);
    EXPECT_EQ(received_data[1], 0x22);
    EXPECT_EQ(received_data[2], 0x33);
}

TEST(Http3DatagramTest, DispatchUnknownContextDrops) {
    Http3DatagramDispatcher d;
    // No handler registered for context_id 7
    const uint8_t payload[] = {0xAA};
    auto frame = Http3DatagramDispatcher::encode(7, payload, sizeof(payload));
    d.dispatch(frame.data(), frame.size());

    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_received, 1u);
    EXPECT_EQ(stats.datagrams_dropped, 1u);
    EXPECT_EQ(stats.datagrams_dispatched, 0u);
}

TEST(Http3DatagramTest, DispatchNullDataDrops) {
    Http3DatagramDispatcher d;
    d.registerContext(1, [](uint64_t, const uint8_t*, size_t) {});
    d.dispatch(nullptr, 0);

    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_dropped, 1u);
}

TEST(Http3DatagramTest, DispatchDisabledDrops) {
    Http3DatagramConfig cfg;
    cfg.enable = false;
    Http3DatagramDispatcher d(cfg);

    const uint8_t payload[] = {0x01};
    auto frame = Http3DatagramDispatcher::encode(0, payload, sizeof(payload));
    d.dispatch(frame.data(), frame.size());

    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_received, 1u);
    EXPECT_EQ(stats.datagrams_dropped, 1u);
}

TEST(Http3DatagramTest, DispatchMalformedVarintDrops) {
    Http3DatagramDispatcher d;
    d.registerContext(0, [](uint64_t, const uint8_t*, size_t) {});

    // 2-byte varint prefix (0x40) but only 1 byte provided → malformed
    uint8_t bad[] = {0x40};
    d.dispatch(bad, 1);

    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_dropped, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(Http3DatagramTest, InitialStatsAllZero) {
    Http3DatagramDispatcher d;
    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_received,   0u);
    EXPECT_EQ(stats.datagrams_dispatched, 0u);
    EXPECT_EQ(stats.datagrams_dropped,    0u);
    EXPECT_EQ(stats.datagrams_sent,       0u);
}

TEST(Http3DatagramTest, RecordSentIncrementsCounter) {
    Http3DatagramDispatcher d;
    d.recordSent();
    d.recordSent();
    EXPECT_EQ(d.getStats().datagrams_sent, 2u);
}

TEST(Http3DatagramTest, DispatchAndStats) {
    Http3DatagramDispatcher d;
    int called = 0;
    d.registerContext(0, [&](uint64_t, const uint8_t*, size_t) { ++called; });
    d.registerContext(1, [&](uint64_t, const uint8_t*, size_t) { ++called; });

    // Dispatch to context 0
    auto f0 = Http3DatagramDispatcher::encode(0, nullptr, 0);
    d.dispatch(f0.data(), f0.size());

    // Dispatch to context 1
    auto f1 = Http3DatagramDispatcher::encode(1, nullptr, 0);
    d.dispatch(f1.data(), f1.size());

    // Dispatch to unknown context 99
    auto f99 = Http3DatagramDispatcher::encode(99, nullptr, 0);
    d.dispatch(f99.data(), f99.size());

    auto stats = d.getStats();
    EXPECT_EQ(stats.datagrams_received,   3u);
    EXPECT_EQ(stats.datagrams_dispatched, 2u);
    EXPECT_EQ(stats.datagrams_dropped,    1u);
    EXPECT_EQ(called, 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// QuicTransport: datagram config defaults
// ─────────────────────────────────────────────────────────────────────────────

#include "network/quic_transport.h"

using namespace themis::network;

TEST(Http3DatagramTest, QuicTransportDatagramDefaultEnabled) {
    QuicTransport::Config cfg;
    // Default: datagram support enabled (max_datagram_frame_size = 65535)
    EXPECT_EQ(cfg.max_datagram_frame_size, 65535u);
}

TEST(Http3DatagramTest, QuicTransportDatagramStatsInitialZero) {
    QuicTransport::Config cfg;
    QuicTransport transport(cfg);

    auto stats = transport.getStats();
    EXPECT_EQ(stats.datagrams_received, 0u);
    EXPECT_EQ(stats.datagrams_sent,     0u);
}

TEST(Http3DatagramTest, QuicTransportDatagramCanBeDisabled) {
    QuicTransport::Config cfg;
    cfg.max_datagram_frame_size = 0;
    EXPECT_EQ(cfg.max_datagram_frame_size, 0u);
}

#endif  // THEMIS_ENABLE_HTTP3
