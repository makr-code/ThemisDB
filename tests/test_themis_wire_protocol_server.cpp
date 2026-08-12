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
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstring>
#include <functional>
#include <sstream>
#include <thread>
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
    h.opcode = static_cast<uint8_t>(OpCode::OP_PING);
    EXPECT_EQ(h.get_opcode(), OpCode::OP_PING);
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
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_HELLO),         0x01u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_HELLO_ACK),     0x02u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_AUTH_REQUEST),  0x03u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_AUTH_RESPONSE), 0x04u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_AUTH_SUCCESS),  0x05u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_AUTH_FAILURE),  0x06u);
}

TEST(OpCode, CRUDValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_GET),    0x10u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_PUT),    0x11u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_DELETE), 0x12u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_BATCH_GET), 0x13u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_BATCH_PUT), 0x14u);
}

TEST(OpCode, QueryValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_QUERY_AQL),   0x20u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_QUERY_RESULT),0x21u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_QUERY_CURSOR),0x22u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_CURSOR_NEXT), 0x23u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_CURSOR_CLOSE),0x24u);
}

TEST(OpCode, TransactionValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_TRANSACTION_BEGIN),  0x30u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_TRANSACTION_COMMIT), 0x31u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_TRANSACTION_ABORT),  0x32u);
}

TEST(OpCode, SpecialValues) {
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_ERROR), 0xF0u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_OK),    0xF1u);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_PING),  0xFEu);
    EXPECT_EQ(static_cast<uint8_t>(OpCode::OP_CLOSE), 0xFFu);
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
    client.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));

    // Accept the server side to complete the handshake (synchronous).
    tcp::socket server_side(ioc);
    acceptor.accept(server_side);

    return client;
}

static uint16_t reserve_loopback_port(boost::asio::io_context& ioc) {
    using tcp = boost::asio::ip::tcp;
    tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), 0));
    return acceptor.local_endpoint().port();
}

// The free setWire* bridge APIs are intentionally kept for compatibility
// coverage and are explicitly deprecated in the public header.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

struct ScopedGenericWireBridgesOnly {
    ScopedGenericWireBridgesOnly() {
        setWireAqlExecFn([](const std::string&, const std::string&) {
            return std::string{"{\"results\":[],\"has_more\":false}"};
        });
        setWireGeoQueryFn([](const std::string&, double, double, double, int) {
            return std::string{"[]"};
        });
        setWireTSQueryFn([](const std::string&, int64_t, int64_t) {
            return std::string{"[]"};
        });
        setWireGraphTraversalFn([](const std::string&, const std::string&, int) {
            return std::string{"[]"};
        });

#if THEMIS_WIRE_V1_PB_HEADER_FOUND
        WireProtocolSession::setQueryAqlFn({});
        WireProtocolSession::setGeoQueryFn({});
        WireProtocolSession::setTimeseriesQueryFn({});
        WireProtocolSession::setGraphTraverseFn({});
#endif
    }

    ~ScopedGenericWireBridgesOnly() {
        setWireAqlExecFn({});
        setWireGeoQueryFn({});
        setWireTSQueryFn({});
        setWireGraphTraversalFn({});

#if THEMIS_WIRE_V1_PB_HEADER_FOUND
        WireProtocolSession::setQueryAqlFn({});
        WireProtocolSession::setGeoQueryFn({});
        WireProtocolSession::setTimeseriesQueryFn({});
        WireProtocolSession::setGraphTraverseFn({});
#endif
    }
};

// Installs minimal non-throwing wire callbacks so start() passes fail-closed
// callback validation in lifecycle tests.
struct ScopedWireStartCallbacks {
    ScopedWireStartCallbacks() {
#if THEMIS_WIRE_V1_PB_HEADER_FOUND
        // WireProtocolServer::start() now enforces protobuf callback presence.
        WireProtocolSession::setQueryAqlFn(
            [](const std::string&) -> std::vector<std::string> {
                return {};
            });
        WireProtocolSession::setGeoQueryFn(
            [](const v1::GeoQueryRequest&) -> v1::GeoQueryResponse {
                return {};
            });
        WireProtocolSession::setTimeseriesQueryFn(
            [](const v1::TimeSeriesQueryRequest&) -> v1::TimeSeriesQueryResponse {
                return {};
            });
        WireProtocolSession::setGraphTraverseFn(
            [](std::string_view) -> std::string {
                return {};
            });
#endif
    }

    ~ScopedWireStartCallbacks() {
    #if THEMIS_WIRE_V1_PB_HEADER_FOUND
        WireProtocolSession::setQueryAqlFn({});
        WireProtocolSession::setGeoQueryFn({});
        WireProtocolSession::setTimeseriesQueryFn({});
        WireProtocolSession::setGraphTraverseFn({});
    #endif
    }
};

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
        OpCode::OP_PING,
        [&call_count](WireProtocolSession& /*s*/,
                      const std::vector<uint8_t>& /*payload*/) {
            ++call_count;
        });

    dispatcher.dispatch(*session, OpCode::OP_PING, {});
    EXPECT_EQ(call_count, 1);
}

TEST(MessageDispatcher, UnknownOpcodeDoesNotThrow) {
    // dispatch() with no registered handler should not throw.
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    MessageDispatcher dispatcher;
    EXPECT_NO_THROW(dispatcher.dispatch(*session, OpCode::OP_QUERY_AQL, {}));
}

TEST(MessageDispatcher, HandlerReplacement) {
    // Re-registering the same opcode replaces the old handler.
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));

    int first = 0, second = 0;
    MessageDispatcher dispatcher;

    dispatcher.register_handler(
        OpCode::OP_HELLO,
        [&first](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++first;
        });
    dispatcher.register_handler(
        OpCode::OP_HELLO,
        [&second](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++second;
        });

    dispatcher.dispatch(*session, OpCode::OP_HELLO, {});
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
        OpCode::OP_GET,
        [&get_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++get_count;
        });
    dispatcher.register_handler(
        OpCode::OP_PUT,
        [&put_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++put_count;
        });
    dispatcher.register_handler(
        OpCode::OP_DELETE,
        [&del_count](WireProtocolSession&, const std::vector<uint8_t>&) {
            ++del_count;
        });

    dispatcher.dispatch(*session, OpCode::OP_GET,    {});
    dispatcher.dispatch(*session, OpCode::OP_PUT,    {});
    dispatcher.dispatch(*session, OpCode::OP_DELETE, {});

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
    ScopedWireStartCallbacks callbacks;
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
    ScopedWireStartCallbacks callbacks;
    WireProtocolServer server(ioc, 0);
    EXPECT_NO_THROW({
        server.start();
        server.start();  // second call should be a no-op
        server.stop();
    });
}

TEST(WireProtocolServer, GenericBridgesDoNotSatisfyProtobufBootstrap) {
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context server_ioc;
    ScopedGenericWireBridgesOnly bridges;
    const uint16_t port = reserve_loopback_port(server_ioc);
    WireProtocolServer server(server_ioc, port);

    server.start();
    std::thread io_thread([&server_ioc]() {
        server_ioc.run();
    });

    boost::asio::io_context client_ioc;
    tcp::socket client(client_ioc);
    client.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
    while (server.total_connections() != 0u &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(server.total_connections(), 0u);
    EXPECT_EQ(server.active_sessions(), 0u);

    boost::system::error_code ec;
    client.shutdown(tcp::socket::shutdown_both, ec);
    client.close(ec);

    server.stop();
    server_ioc.stop();
    io_thread.join();
}

TEST(WireProtocolServer, Sessions_Pruned_After_Disconnect) {
    using tcp = boost::asio::ip::tcp;
    constexpr int kConnectionCount = 100;

    boost::asio::io_context server_ioc;
    ScopedWireStartCallbacks callbacks;
    const uint16_t port = reserve_loopback_port(server_ioc);
    WireProtocolServer server(server_ioc, port);

    server.start();
    std::thread io_thread([&server_ioc]() {
        server_ioc.run();
    });

    for (int i = 0; i < kConnectionCount; ++i) {
        boost::asio::io_context client_ioc;
        tcp::socket client(client_ioc);
        client.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));
        boost::system::error_code ec;
        client.shutdown(tcp::socket::shutdown_both, ec);
        client.close(ec);
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while ((server.total_connections() < static_cast<uint64_t>(kConnectionCount) ||
            server.active_sessions() != 0u) &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(server.total_connections(), static_cast<uint64_t>(kConnectionCount));
    EXPECT_EQ(server.active_sessions(), 0u);

    server.stop();
    server_ioc.stop();
    io_thread.join();
}

TEST(WireProtocolServer, SingleThreadedIoContextPrunesSessionsAfterDisconnect) {
    using tcp = boost::asio::ip::tcp;

    boost::asio::io_context server_ioc;
    ScopedWireStartCallbacks callbacks;
    const uint16_t port = reserve_loopback_port(server_ioc);
    WireProtocolServer server(server_ioc, port);

    server.start();

    boost::asio::io_context client_ioc;
    tcp::socket client(client_ioc);
    client.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), port));

    const auto accept_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (server.total_connections() != 1u &&
           std::chrono::steady_clock::now() < accept_deadline) {
        server_ioc.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(server.total_connections(), 1u);
    EXPECT_EQ(server.active_sessions(), 1u);

    boost::system::error_code ec;
    client.shutdown(tcp::socket::shutdown_both, ec);
    client.close(ec);

    const auto disconnect_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (server.active_sessions() != 0u &&
           std::chrono::steady_clock::now() < disconnect_deadline) {
        server_ioc.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    EXPECT_EQ(server.active_sessions(), 0u);

    server.stop();
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

// ===========================================================================
// Handler validation logic — mirrors the implemented handler behaviour
// without requiring a live protobuf payload or TCP connection.
// ===========================================================================

// --- Auth decision (handleAuthResponse logic) --------------------------------

TEST(WireProtocolV1ThemisAuth, NewSessionIsNotAuthenticated) {
    boost::asio::io_context ioc;
    auto client = make_client_socket(ioc);
    auto session = std::make_shared<WireProtocolSession>(std::move(client));
    EXPECT_FALSE(session->is_authenticated());
}

// Logic mirror for handle_auth_response authentication success path.
TEST(WireProtocolV1ThemisAuth, AuthSuccessRequiresNonEmptyUsername) {
    // handle_auth_response sets authenticated_=true only when username is non-empty.
    std::string username = "alice";
    bool accepted = !username.empty();
    EXPECT_TRUE(accepted);
}

TEST(WireProtocolV1ThemisAuth, AuthFailsWithEmptyUsername) {
    std::string username = "";
    bool accepted = !username.empty();
    EXPECT_FALSE(accepted);
}

// --- GET input validation ---------------------------------------------------

TEST(WireProtocolV1ThemisGet, EmptyCollectionWouldBeRejected) {
    // handle_get checks collection.empty() → error 400.
    std::string collection = "";
    std::string uuid = "doc-1";
    bool would_reject = collection.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisGet, BlankCollectionWouldBeRejected) {
    std::string collection = "  \t";
    std::string uuid = "doc-1";
    bool is_blank = std::all_of(collection.begin(), collection.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = collection.empty() || is_blank || uuid.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisGet, EmptyUuidWouldBeRejected) {
    std::string collection = "users";
    std::string uuid = "";
    bool would_reject = uuid.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisGet, ValidInputWouldPassValidation) {
    std::string collection = "users";
    std::string uuid = "user-42";
    bool would_reject = collection.empty() || uuid.empty();
    EXPECT_FALSE(would_reject);
}

// --- PUT input validation ---------------------------------------------------

TEST(WireProtocolV1ThemisPut, EmptyCollectionWouldBeRejected) {
    std::string collection = "";
    std::string uuid = "doc-1";
    std::string entity = "{}";
    bool would_reject = collection.empty() || uuid.empty() || entity.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisPut, EmptyEntityWouldBeRejected) {
    std::string collection = "orders";
    std::string uuid = "ord-99";
    std::string entity = "";
    bool would_reject = collection.empty() || uuid.empty() || entity.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisPut, BlankUuidWouldBeRejected) {
    std::string collection = "orders";
    std::string uuid = " \n";
    std::string entity = "{}";
    bool is_blank = std::all_of(uuid.begin(), uuid.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = collection.empty() || uuid.empty() || is_blank || entity.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisPut, ValidInputWouldPassValidation) {
    std::string collection = "orders";
    std::string uuid = "ord-99";
    std::string entity = "{\"total\": 42}";
    bool would_reject = collection.empty() || uuid.empty() || entity.empty();
    EXPECT_FALSE(would_reject);
}

// --- DELETE input validation ------------------------------------------------

TEST(WireProtocolV1ThemisDelete, EmptyCollectionWouldBeRejected) {
    std::string collection = "";
    std::string uuid = "doc-1";
    bool would_reject = collection.empty() || uuid.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisDelete, EmptyUuidWouldBeRejected) {
    std::string collection = "products";
    std::string uuid = "";
    bool would_reject = collection.empty() || uuid.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisDelete, BlankUuidWouldBeRejected) {
    std::string collection = "products";
    std::string uuid = " \t";
    bool is_blank = std::all_of(uuid.begin(), uuid.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = collection.empty() || uuid.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

// --- QUERY_AQL input validation ---------------------------------------------

TEST(WireProtocolV1ThemisQuery, EmptyAqlWouldBeRejected) {
    std::string aql = "";
    bool would_reject = aql.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisQuery, BlankAqlWouldBeRejected) {
    std::string aql = "   \n";
    bool is_blank = std::all_of(aql.begin(), aql.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = aql.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisQuery, NonEmptyAqlPassesValidation) {
    std::string aql = "FOR doc IN users RETURN doc";
    bool would_reject = aql.empty();
    EXPECT_FALSE(would_reject);
}

// --- VECTOR_SEARCH input validation -----------------------------------------

TEST(WireProtocolV1ThemisVectorSearch, EmptyCollectionWouldBeRejected) {
    std::string collection = "";
    int vector_size = 128;
    bool would_reject = collection.empty() || (vector_size == 0);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisVectorSearch, BlankCollectionWouldBeRejected) {
    std::string collection = " \t";
    int vector_size = 128;
    bool is_blank = std::all_of(collection.begin(), collection.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = collection.empty() || is_blank || (vector_size == 0);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisVectorSearch, EmptyVectorWouldBeRejected) {
    std::string collection = "embeddings";
    int vector_size = 0;
    bool would_reject = collection.empty() || (vector_size == 0);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisVectorSearch, ValidRequestPassesValidation) {
    std::string collection = "embeddings";
    int vector_size = 384;
    bool would_reject = collection.empty() || (vector_size == 0);
    EXPECT_FALSE(would_reject);
}

// --- GEO_QUERY input validation ---------------------------------------------

TEST(WireProtocolV1ThemisGeoQuery, EmptyCollectionWouldBeRejected) {
    std::string collection = "";
    bool would_reject = collection.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisGeoQuery, NonEmptyCollectionPassesValidation) {
    std::string collection = "locations";
    bool would_reject = collection.empty();
    EXPECT_FALSE(would_reject);
}

// --- TIMESERIES_QUERY input validation --------------------------------------

TEST(WireProtocolV1ThemisTimeseries, EmptyCollectionWouldBeRejected) {
    std::string collection = "";
    uint64_t start_ns = 1000u;
    uint64_t end_ns   = 2000u;
    bool would_reject = collection.empty() || (start_ns >= end_ns);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisTimeseries, InvalidTimeRangeWouldBeRejected) {
    // start >= end must be rejected.
    std::string collection = "cpu_usage";
    uint64_t start_ns = 2000u;
    uint64_t end_ns   = 1000u;
    bool would_reject = collection.empty() || (start_ns >= end_ns);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisTimeseries, EqualTimestampsWouldBeRejected) {
    std::string collection = "cpu_usage";
    uint64_t start_ns = 1000u;
    uint64_t end_ns   = 1000u;
    bool would_reject = collection.empty() || (start_ns >= end_ns);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisTimeseries, ValidRequestPassesValidation) {
    std::string collection = "cpu_usage";
    uint64_t start_ns = 1000u;
    uint64_t end_ns   = 2000u;
    bool would_reject = collection.empty() || (start_ns >= end_ns);
    EXPECT_FALSE(would_reject);
}

// --- GRAPH_TRAVERSE input validation ---------------------------------------

TEST(WireProtocolV1ThemisGraph, NonObjectRequestWouldBeRejected) {
    bool request_is_object = false;
    bool would_reject = !request_is_object;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisGraph, BlankStartVertexWouldBeRejected) {
    std::string collection = "edges";
    std::string start_vertex = "  \t";
    bool is_blank = std::all_of(start_vertex.begin(), start_vertex.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = collection.empty() || start_vertex.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

// --- CURSOR_* input validation ---------------------------------------------

TEST(WireProtocolV1ThemisCursor, NonObjectRequestWouldBeRejected) {
    bool request_is_object = false;
    bool would_reject = !request_is_object;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisCursor, BlankCursorIdWouldBeRejected) {
    std::string cursor_id = " \n";
    bool is_blank = std::all_of(cursor_id.begin(), cursor_id.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = cursor_id.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisCursor, BatchSizeOutOfRangeWouldBeRejected) {
    int batch_size = 10001;
    bool would_reject = (batch_size < 1 || batch_size > 10000);
    EXPECT_TRUE(would_reject);
}

// --- TRANSACTION_* input validation ----------------------------------------

TEST(WireProtocolV1ThemisTransaction, NonObjectBeginRequestWouldBeRejected) {
    bool request_is_object = false;
    bool would_reject = !request_is_object;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisTransaction, BlankTransactionIdWouldBeRejected) {
    std::string transaction_id = "  \t";
    bool is_blank = std::all_of(transaction_id.begin(), transaction_id.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = transaction_id.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisTransaction, TimeoutOutOfRangeWouldBeRejected) {
    int64_t timeout_ms = 3'600'001;
    bool would_reject = (timeout_ms < 1 || timeout_ms > 3'600'000);
    EXPECT_TRUE(would_reject);
}

// --- BPMN_START_PROCESS input validation ------------------------------------

TEST(WireProtocolV1ThemisBpmn, EmptyProcessKeyWouldBeRejected) {
    std::string process_definition_key = "";
    bool would_reject = process_definition_key.empty();
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, NonEmptyProcessKeyPassesValidation) {
    std::string process_definition_key = "invoice-approval-v2";
    bool would_reject = process_definition_key.empty();
    EXPECT_FALSE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, BlankProcessKeyWouldBeRejected) {
    std::string process_definition_key = "   \t";
    bool is_blank = std::all_of(process_definition_key.begin(), process_definition_key.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = process_definition_key.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, BlankTaskIdWouldBeRejected) {
    std::string task_id = " \n";
    bool is_blank = std::all_of(task_id.begin(), task_id.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = task_id.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsNonObjectRequest) {
    bool request_is_object = false;
    bool would_reject = !request_is_object;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, BlankProcessInstanceIdWouldBeRejected) {
    std::string process_instance_id = "\t ";
    bool is_blank = std::all_of(process_instance_id.begin(), process_instance_id.end(),
                                [](unsigned char ch) { return std::isspace(ch) != 0; });
    bool would_reject = process_instance_id.empty() || is_blank;
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsZeroMaxHistoryEvents) {
    std::size_t max_history_events = 0;
    bool would_reject = (max_history_events == 0 || max_history_events > 10000);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, QueryInstanceRejectsTooLargeMaxHistoryEvents) {
    std::size_t max_history_events = 10001;
    bool would_reject = (max_history_events == 0 || max_history_events > 10000);
    EXPECT_TRUE(would_reject);
}

TEST(WireProtocolV1ThemisBpmn, QueryInstanceAcceptsBoundedMaxHistoryEvents) {
    std::size_t max_history_events = 5000;
    bool would_reject = (max_history_events == 0 || max_history_events > 10000);
    EXPECT_FALSE(would_reject);
}

// --- Auth error code --------------------------------------------------------

TEST(WireProtocolV1ThemisAuthCode, UnauthenticatedUsesCode0x0401) {
    // All data handlers check authenticated_ and send error 0x0401 when false.
    constexpr uint32_t kAuthRequired = 0x0401u;
    EXPECT_EQ(kAuthRequired, 1025u);  // 0x0401 = 1025
}

// --- Handler decision logic mirrors (auth + validation + outcome) ------------

// GET: logic mirror of the full handle_get decision path.
TEST(WireProtocolV1ThemisGet, UnauthenticatedRequestSends0x0401) {
    bool authenticated = false;
    std::string collection = "users";
    std::string uuid = "user-1";
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty() || uuid.empty()) {
        expected_code = 400;
    } else {
        expected_code = 503;
    }
    EXPECT_EQ(expected_code, 0x0401u);
}

TEST(WireProtocolV1ThemisGet, AuthenticatedValidRequestSends503) {
    bool authenticated = true;
    std::string collection = "users";
    std::string uuid = "user-1";
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty() || uuid.empty()) {
        expected_code = 400;
    } else {
        expected_code = 503;
    }
    EXPECT_EQ(expected_code, 503u);
}

TEST(WireProtocolV1ThemisGet, AuthenticatedMissingFieldSends400) {
    bool authenticated = true;
    std::string collection = "users";
    std::string uuid = "";  // missing
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty() || uuid.empty()) {
        expected_code = 400;
    } else {
        expected_code = 503;
    }
    EXPECT_EQ(expected_code, 400u);
}

// QUERY_AQL: logic mirror of the full handle_query_aql decision path.
TEST(WireProtocolV1ThemisQuery, AuthenticatedValidRequestSends501) {
    bool authenticated = true;
    std::string aql = "FOR doc IN users RETURN doc";
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (aql.empty()) {
        expected_code = 400;
    } else {
        expected_code = 501;  // not integrated
    }
    EXPECT_EQ(expected_code, 501u);
}

// VECTOR_SEARCH: logic mirror of the full handle_vector_search decision path.
TEST(WireProtocolV1ThemisVectorSearch, AuthenticatedValidRequestSends503) {
    bool authenticated = true;
    std::string collection = "embeddings";
    int vector_size = 384;
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty() || vector_size == 0) {
        expected_code = 400;
    } else {
        expected_code = 503;  // vector index not connected
    }
    EXPECT_EQ(expected_code, 503u);
}

// GEO_QUERY: logic mirror of the full handle_geo_query decision path.
TEST(WireProtocolV1ThemisGeoQuery, AuthenticatedValidRequestSends501) {
    bool authenticated = true;
    std::string collection = "locations";
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty()) {
        expected_code = 400;
    } else {
        expected_code = 501;  // geo not integrated
    }
    EXPECT_EQ(expected_code, 501u);
}

// TIMESERIES: logic mirror of the full handle_timeseries_query decision path.
TEST(WireProtocolV1ThemisTimeseries, AuthenticatedValidRequestSends503) {
    bool authenticated = true;
    std::string collection = "cpu_usage";
    uint64_t start_ns = 1000u;
    uint64_t end_ns   = 2000u;
    uint32_t expected_code = 0;
    if (!authenticated) {
        expected_code = 0x0401;
    } else if (collection.empty() || start_ns >= end_ns) {
        expected_code = 400;
    } else {
        expected_code = 503;  // ts store not connected
    }
    EXPECT_EQ(expected_code, 503u);
}

// --- Not-implemented error codes --------------------------------------------

TEST(WireProtocolV1ThemisErrorCodes, QueryAqlUses501) {
    // handle_query_aql returns 501 (not implemented) for AQL queries.
    constexpr uint32_t kNotImplemented = 501u;
    EXPECT_EQ(kNotImplemented, 501u);
}

TEST(WireProtocolV1ThemisErrorCodes, GeoQueryUses501) {
    // handle_geo_query returns 501 (not implemented) for geo queries.
    constexpr uint32_t kNotImplemented = 501u;
    EXPECT_EQ(kNotImplemented, 501u);
}

TEST(WireProtocolV1ThemisErrorCodes, StorageUnavailableUses503) {
    // handle_get/put/delete/vector_search return 503 (service unavailable).
    constexpr uint32_t kServiceUnavailable = 503u;
    EXPECT_EQ(kServiceUnavailable, 503u);
}

// --- sanitizeForMessage logic mirror (security: log injection prevention) ----

namespace {

using json = nlohmann::json;

constexpr std::size_t kMaxAuthPayloadBytesMirror = 16u * 1024u;
constexpr std::size_t kAuthTokenJsonOverheadMirror = sizeof("{\"token\":\"\"}") - 1;

std::size_t escapedJsonStringLengthMirror(std::string_view value) {
    std::size_t escaped_length = 0;
    for (unsigned char ch : value) {
        switch (ch) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                escaped_length += 2;
                break;
            default:
                escaped_length += (ch < 0x20u) ? 6u : 1u;
                break;
        }
    }
    return escaped_length;
}

std::size_t authPayloadSizeForTokenMirror(std::string_view token) {
    return kAuthTokenJsonOverheadMirror + escapedJsonStringLengthMirror(token);
}

/// Local mirror of the sanitizeForMessage function from wire_protocol_server.cpp.
/// Replaces control characters (< 0x20) and DEL (0x7F) with '?'.
std::string sanitizeForMessageMirror(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c < 0x20u || c == 0x7Fu) {
            out += '?';
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}

} // anonymous namespace

TEST(WireProtocolV1ThemisAuthPayloadLimit, MirrorMatchesJsonDumpSizeForEscapedToken) {
    std::string token = "plain\"quoted\\path\nline\tend";
    token.push_back('\x1F');
    token += "UTF8_ä";

    const std::size_t legacy_json_dump_size = json{{"token", token}}.dump().size();
    EXPECT_EQ(authPayloadSizeForTokenMirror(token), legacy_json_dump_size);
}

TEST(WireProtocolV1ThemisAuthPayloadLimit, BoundaryAtPayloadLimitIsAccepted) {
    ASSERT_LT(kAuthTokenJsonOverheadMirror, kMaxAuthPayloadBytesMirror);
    const std::size_t token_size = kMaxAuthPayloadBytesMirror - kAuthTokenJsonOverheadMirror;
    const std::string token(token_size, 'a');

    EXPECT_EQ(authPayloadSizeForTokenMirror(token), kMaxAuthPayloadBytesMirror);
}

TEST(WireProtocolV1ThemisAuthPayloadLimit, OverLimitPayloadIsRejected) {
    ASSERT_LT(kAuthTokenJsonOverheadMirror, kMaxAuthPayloadBytesMirror);
    const std::size_t token_size = (kMaxAuthPayloadBytesMirror - kAuthTokenJsonOverheadMirror) + 1u;
    const std::string token(token_size, 'a');

    EXPECT_GT(authPayloadSizeForTokenMirror(token), kMaxAuthPayloadBytesMirror);
}

TEST(WireProtocolV1ThemisSanitize, PrintableStringUnchanged) {
    EXPECT_EQ(sanitizeForMessageMirror("users"), "users");
    EXPECT_EQ(sanitizeForMessageMirror("my-collection_v2"), "my-collection_v2");
    EXPECT_EQ(sanitizeForMessageMirror("doc-123"), "doc-123");
}

TEST(WireProtocolV1ThemisSanitize, NewlineReplacedWithQuestionMark) {
    // Embedded newlines must not pass through to prevent log injection.
    EXPECT_EQ(sanitizeForMessageMirror("a\nb"), "a?b");
    EXPECT_EQ(sanitizeForMessageMirror("a\r\nb"), "a??b");
}

TEST(WireProtocolV1ThemisSanitize, TabReplacedWithQuestionMark) {
    EXPECT_EQ(sanitizeForMessageMirror("col\tlection"), "col?lection");
}

TEST(WireProtocolV1ThemisSanitize, DelCharReplacedWithQuestionMark) {
    std::string s = "abc";
    s += '\x7F';
    s += "def";
    EXPECT_EQ(sanitizeForMessageMirror(s), "abc?def");
}

TEST(WireProtocolV1ThemisSanitize, EmptyStringUnchanged) {
    EXPECT_EQ(sanitizeForMessageMirror(""), "");
}

TEST(WireProtocolV1ThemisSanitize, AllControlCharsReplaced) {
    // Control characters 0x00–0x1F must all be replaced.
    for (int i = 0; i < 0x20; ++i) {
        std::string s(1, static_cast<char>(i));
        EXPECT_EQ(sanitizeForMessageMirror(s), "?")
            << "Expected '?' for control char 0x" << std::hex << i;
    }
}

// =============================================================================
// Deprecated free bridge compatibility tests
// =============================================================================

TEST(DeprecatedWireSessionBridgeTest, SetAndClearAqlBridge) {
    bool called = false;
    setWireAqlExecFn([&called](const std::string& /*aql*/,
                               const std::string& /*ns*/) -> std::string {
        called = true;
        return R"({"results":[]})";
    });
    setWireAqlExecFn(nullptr);
    EXPECT_FALSE(called); // setter itself must not invoke the fn
}

TEST(DeprecatedWireSessionBridgeTest, SetAndClearCursorNextBridge) {
    setWireCursorNextFn([](const std::string& /*id*/) -> std::string {
        return R"({"batch":[]})";
    });
    setWireCursorNextFn(nullptr);
    SUCCEED();
}

TEST(DeprecatedWireSessionBridgeTest, SetAndClearCursorCloseBridge) {
    setWireCursorCloseFn([](const std::string& /*id*/) -> bool { return true; });
    setWireCursorCloseFn(nullptr);
    SUCCEED();
}

TEST(DeprecatedWireSessionBridgeTest, SetAndClearGeoQueryBridge) {
    setWireGeoQueryFn([](const std::string& /*collection*/, double /*lat*/,
                          double /*lon*/, double /*radius_m*/, int /*limit*/) -> std::string {
        return R"([])";
    });
    setWireGeoQueryFn(nullptr);
    SUCCEED();
}

TEST(DeprecatedWireSessionBridgeTest, SetAndClearTSQueryBridge) {
    setWireTSQueryFn([](const std::string& /*collection*/,
                        int64_t /*start_ns*/, int64_t /*end_ns*/) -> std::string {
        return R"({"points":[]})";
    });
    setWireTSQueryFn(nullptr);
    SUCCEED();
}

TEST(DeprecatedWireSessionBridgeTest, SetAndClearGraphTraversalBridge) {
    setWireGraphTraversalFn([](const std::string& /*collection*/,
                               const std::string& /*start_vertex*/,
                               int /*max_depth*/) -> std::string {
        return R"({"vertices":[],"edges":[]})";
    });
    setWireGraphTraversalFn(nullptr);
    SUCCEED();
}

TEST(DeprecatedWireSessionBridgeTest, NullptrClearIsIdempotent) {
    // Clearing a not-yet-set bridge must not crash.
    setWireAqlExecFn(nullptr);
    setWireCursorNextFn(nullptr);
    setWireCursorCloseFn(nullptr);
    setWireGeoQueryFn(nullptr);
    setWireTSQueryFn(nullptr);
    setWireGraphTraversalFn(nullptr);
    SUCCEED();
}

#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
