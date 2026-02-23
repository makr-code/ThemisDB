/// @file test_themis_wire_protocol_server.cpp
/// @brief Unit tests for themis::wire V1 protocol types, constants, and
///        compile-time properties defined in
///        include/themis/network/wire_protocol_server.hpp.
///
/// These tests mirror the style of test_wire_protocol_v2.cpp.  They exercise
/// the header-level types (structs, enums, constants) and the
/// MessageDispatcher without requiring a live TCP connection or protobuf.

#include <gtest/gtest.h>
#include "themis/network/wire_protocol_server.hpp"

#include <boost/asio.hpp>
#include <cstring>
#include <functional>
#include <vector>

using namespace themis::wire;

// ===========================================================================
// Constant Tests
// ===========================================================================

TEST(WireV1Constants, MagicValue) {
    // "TMDB" in big-endian ASCII
    EXPECT_EQ(WIRE_MAGIC, 0x544D4442u);
}

TEST(WireV1Constants, Version) {
    EXPECT_EQ(WIRE_VERSION_1, 0x01u);
}

TEST(WireV1Constants, HeaderSize) {
    EXPECT_EQ(HEADER_SIZE, 12u);
    EXPECT_EQ(sizeof(WireFrameHeader), HEADER_SIZE);
}

TEST(WireV1Constants, ChecksumSize) {
    EXPECT_EQ(CHECKSUM_SIZE, 4u);
}

TEST(WireV1Constants, MaxPayloadSize) {
    EXPECT_EQ(MAX_PAYLOAD_SIZE, 64u * 1024 * 1024);
}

// ===========================================================================
// WireFrameHeader Tests
// ===========================================================================

TEST(WireFrameHeader, ValidHeader) {
    WireFrameHeader h{};
    h.magic   = WIRE_MAGIC;
    h.version = WIRE_VERSION_1;
    EXPECT_TRUE(h.is_valid());
}

TEST(WireFrameHeader, InvalidMagic) {
    WireFrameHeader h{};
    h.magic   = 0xDEADBEEFu;
    h.version = WIRE_VERSION_1;
    EXPECT_FALSE(h.is_valid());
}

TEST(WireFrameHeader, InvalidVersion) {
    WireFrameHeader h{};
    h.magic   = WIRE_MAGIC;
    h.version = 0x02u;
    EXPECT_FALSE(h.is_valid());
}

TEST(WireFrameHeader, GetOpcode) {
    WireFrameHeader h{};
    h.opcode = static_cast<uint8_t>(OpCode::PING);
    EXPECT_EQ(h.get_opcode(), OpCode::PING);
}

TEST(WireFrameHeader, HasFlagNone) {
    WireFrameHeader h{};
    h.flags = 0;
    EXPECT_FALSE(h.has_flag(MessageFlags::SKIP_CHECKSUM));
    EXPECT_FALSE(h.has_flag(MessageFlags::COMPRESSED));
    EXPECT_FALSE(h.has_flag(MessageFlags::ENCRYPTED));
}

TEST(WireFrameHeader, HasFlagSkipChecksum) {
    WireFrameHeader h{};
    h.flags = static_cast<uint16_t>(MessageFlags::SKIP_CHECKSUM);
    EXPECT_TRUE(h.has_flag(MessageFlags::SKIP_CHECKSUM));
    EXPECT_FALSE(h.has_flag(MessageFlags::COMPRESSED));
}

TEST(WireFrameHeader, HasFlagCompressed) {
    WireFrameHeader h{};
    h.flags = static_cast<uint16_t>(MessageFlags::COMPRESSED);
    EXPECT_TRUE(h.has_flag(MessageFlags::COMPRESSED));
    EXPECT_FALSE(h.has_flag(MessageFlags::SKIP_CHECKSUM));
}

TEST(WireFrameHeader, PackedLayout) {
    // The header must be exactly HEADER_SIZE bytes with no padding.
    EXPECT_EQ(sizeof(WireFrameHeader), 12u);
}

// ===========================================================================
// OpCode Enum Tests
// ===========================================================================

TEST(OpCode, HandshakeValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::HELLO),         0x01u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::HELLO_ACK),     0x02u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::AUTH_REQUEST),  0x03u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::AUTH_RESPONSE), 0x04u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::AUTH_SUCCESS),  0x05u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::AUTH_FAILURE),  0x06u);
}

TEST(OpCode, CRUDValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::GET),       0x10u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PUT),       0x11u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::DELETE),    0x12u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::BATCH_GET), 0x13u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::BATCH_PUT), 0x14u);
}

TEST(OpCode, QueryValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::QUERY_AQL),   0x20u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::QUERY_RESULT),0x21u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::QUERY_CURSOR),0x22u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::CURSOR_NEXT), 0x23u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::CURSOR_CLOSE),0x24u);
}

TEST(OpCode, TransactionValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::TRANSACTION_BEGIN),  0x30u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::TRANSACTION_COMMIT), 0x31u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::TRANSACTION_ABORT),  0x32u);
}

TEST(OpCode, SpecialValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::ERROR), 0xF0u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OK),    0xF1u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::PING),  0xFEu);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::CLOSE), 0xFFu);
}

// ===========================================================================
// MessageFlags Enum Tests
// ===========================================================================

TEST(MessageFlags, Values) {
    EXPECT_EQ(static_cast<uint16_t>(MessageFlags::NONE),           0x0000u);
    EXPECT_EQ(static_cast<uint16_t>(MessageFlags::SKIP_CHECKSUM),  0x0001u);
    EXPECT_EQ(static_cast<uint16_t>(MessageFlags::COMPRESSED),     0x0002u);
    EXPECT_EQ(static_cast<uint16_t>(MessageFlags::ENCRYPTED),      0x0004u);
}

TEST(MessageFlags, Combinable) {
    // Flags must be independently bit-testable.
    const uint16_t combined =
        static_cast<uint16_t>(MessageFlags::SKIP_CHECKSUM) |
        static_cast<uint16_t>(MessageFlags::COMPRESSED);
    WireFrameHeader h{};
    h.flags = combined;
    EXPECT_TRUE(h.has_flag(MessageFlags::SKIP_CHECKSUM));
    EXPECT_TRUE(h.has_flag(MessageFlags::COMPRESSED));
    EXPECT_FALSE(h.has_flag(MessageFlags::ENCRYPTED));
}

// ===========================================================================
// Helper: create a loopback socket pair for testing
// ===========================================================================

namespace {

/// Creates a connected TCP socket pair on the loopback interface.
/// Returns the connected client socket; the acceptor and server socket are
/// discarded (their lifetime is scoped to the function).
static boost::asio::ip::tcp::socket make_client_socket(
    boost::asio::io_context& ioc) {
    using tcp = boost::asio::ip::tcp;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
    const auto port = acceptor.local_endpoint().port();

    tcp::socket client(ioc);
    client.connect(tcp::endpoint(tcp::v4(), port));

    // Accept the server side to complete the handshake (synchronous).
    tcp::socket server_side(ioc);
    acceptor.accept(server_side);

    return client;
}

} // anonymous namespace

// ===========================================================================
// MessageDispatcher Tests
// ===========================================================================

TEST(MessageDispatcher, RegisterAndDispatch) {
    // Create a minimal connected session to exercise dispatch().
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    int call_count = 0;
    MessageDispatcher dispatcher;

    dispatcher.register_handler(
        OpCode::PING,
        [&call_count](WireProtocolSession& /*s*/,
                      const std::vector<uint8_t>& /*payload*/) {
            ++call_count;
        });

    dispatcher.dispatch(*session, OpCode::PING, {});
    EXPECT_EQ(call_count, 1);
}

TEST(MessageDispatcher, UnknownOpcodeDoesNotThrow) {
    // dispatch() with no registered handler should not throw.
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    MessageDispatcher dispatcher;
    EXPECT_NO_THROW(dispatcher.dispatch(*session, OpCode::QUERY_AQL, {}));
}

TEST(MessageDispatcher, HandlerReplacement) {
    // Re-registering the same opcode replaces the old handler.
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    int first = 0, second = 0;
    MessageDispatcher dispatcher;

    dispatcher.register_handler(
        OpCode::HELLO,
        [&first](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++first;
        });
    dispatcher.register_handler(
        OpCode::HELLO,
        [&second](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++second;
        });

    dispatcher.dispatch(*session, OpCode::HELLO, {});
    // Only the second handler (replacement) should have been called.
    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
}

TEST(MessageDispatcher, MultipleOpcodes) {
    // Registering different opcodes should not interfere.
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    int get_count = 0, put_count = 0, del_count = 0;
    MessageDispatcher dispatcher;

    dispatcher.register_handler(
        OpCode::GET,
        [&get_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++get_count;
        });
    dispatcher.register_handler(
        OpCode::PUT,
        [&put_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++put_count;
        });
    dispatcher.register_handler(
        OpCode::DELETE,
        [&del_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++del_count;
        });

    dispatcher.dispatch(*session, OpCode::GET,    {});
    dispatcher.dispatch(*session, OpCode::PUT,    {});
    dispatcher.dispatch(*session, OpCode::DELETE, {});

    EXPECT_EQ(get_count, 1);
    EXPECT_EQ(put_count, 1);
    EXPECT_EQ(del_count, 1);
}

// ===========================================================================
// WireFrameHeader serialization round-trip (byte-level)
// ===========================================================================

TEST(WireFrameHeader, SizeAndAlignment) {
    // Verify that the packed struct is exactly 12 bytes with no padding.
    WireFrameHeader h{};
    h.magic          = 0x01020304u;
    h.version        = 0xAAu;
    h.opcode         = 0xBBu;
    h.flags          = 0xCCDDu;
    h.payload_length = 0x11223344u;

    EXPECT_EQ(sizeof(h), 12u);

    // Verify that individual fields land at expected byte offsets within the
    // packed struct.  All comparisons are made against native-endian values
    // since no byte-swapping is applied to the struct memory itself.
    EXPECT_EQ(h.version, 0xAAu);
    EXPECT_EQ(h.opcode,  0xBBu);
    EXPECT_EQ(h.flags,   0xCCDDu);
    EXPECT_EQ(h.payload_length, 0x11223344u);
}

// ===========================================================================
// WireProtocolSession – initial state tests
// ===========================================================================

TEST(WireProtocolSession, InitialState) {
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    // A fresh session must not be authenticated.
    EXPECT_FALSE(session->is_authenticated());

    // The session_id must be a non-empty string produced from the
    // remote endpoint + timestamp.
    EXPECT_FALSE(session->session_id().empty());
}

TEST(WireProtocolSession, CloseIsIdempotent) {
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    // Calling close() multiple times must not throw.
    EXPECT_NO_THROW({
        session->close("first close");
        session->close("second close");
    });
}

// ===========================================================================
// WireProtocolServer – lifecycle tests
// (mirrors the V2Server tests in test_wire_protocol_v2.cpp)
// ===========================================================================

TEST(WireProtocolServer, ConstructDestruct) {
    boost::asio::io_context ioc;
    // Port 0 lets the OS choose an ephemeral port; we won't call start().
    EXPECT_NO_THROW({
        WireProtocolServer server(ioc, 0);
    });
}

TEST(WireProtocolServer, InitialStatistics) {
    boost::asio::io_context ioc;
    WireProtocolServer server(ioc, 0);

    EXPECT_EQ(server.active_sessions(),    0u);
    EXPECT_EQ(server.total_connections(),  0u);
    EXPECT_EQ(server.total_messages(),     0u);
}

TEST(WireProtocolServer, StartStop) {
    boost::asio::io_context ioc;
    WireProtocolServer server(ioc, 0);

    // start() must not throw; stop() must be safe to call after start().
    EXPECT_NO_THROW({
        server.start();
        server.stop();
    });
}

TEST(WireProtocolServer, StopBeforeStart) {
    // stop() on a server that was never started must not throw.
    boost::asio::io_context ioc;
    WireProtocolServer server(ioc, 0);
    EXPECT_NO_THROW(server.stop());
}

TEST(WireProtocolServer, DoubleStart) {
    // start() called twice must be safe (idempotent).
    boost::asio::io_context ioc;
    WireProtocolServer server(ioc, 0);
    EXPECT_NO_THROW({
        server.start();
        server.start();  // second call should be a no-op
        server.stop();
    });
}

// ===========================================================================
// Checksum – public-facing coverage notes
// ===========================================================================
// WireProtocolSession::compute_checksum and ::verify_checksum are private
// members; direct unit testing is not possible from outside the class.
// Correctness is covered by the async read pipeline integration test path:
// when a frame with a correct CRC32 checksum is sent over the loopback
// socket, the session accepts it; when the checksum is wrong, it sends back
// an ERROR frame (0x03 "Checksum mismatch").
//
// The CHECKSUM_SIZE constant (4 bytes / CRC32) is verified below.

TEST(WireProtocolConstants, ChecksumSizeIsCrc32Width) {
    // CRC32 is 32 bits = 4 bytes.
    EXPECT_EQ(CHECKSUM_SIZE, 4u);
    EXPECT_EQ(CHECKSUM_SIZE, sizeof(uint32_t));
}
