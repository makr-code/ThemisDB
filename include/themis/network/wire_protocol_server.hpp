/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server.hpp                           ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     241                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol Server
// Binary TCP protocol handler for high-performance client connections

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <boost/asio.hpp>
#include <google/protobuf/message.h>

#include "themis_wire_v1.pb.h"

namespace themis {
namespace wire {

// =============================================================================
// Wire Protocol Constants
// =============================================================================

constexpr uint32_t WIRE_MAGIC = 0x544D4442;  // "TMDB" in ASCII
constexpr uint8_t WIRE_VERSION_1 = 0x01;
constexpr size_t HEADER_SIZE = 12;
constexpr size_t CHECKSUM_SIZE = 4;
constexpr size_t MAX_PAYLOAD_SIZE = 64 * 1024 * 1024;  // 64MB

// OpCodes
enum class OpCode : uint8_t {
    HELLO = 0x01,
    HELLO_ACK = 0x02,
    AUTH_REQUEST = 0x03,
    AUTH_RESPONSE = 0x04,
    AUTH_SUCCESS = 0x05,
    AUTH_FAILURE = 0x06,
    
    GET = 0x10,
    PUT = 0x11,
    DELETE = 0x12,
    BATCH_GET = 0x13,
    BATCH_PUT = 0x14,
    
    QUERY_AQL = 0x20,
    QUERY_RESULT = 0x21,
    QUERY_CURSOR = 0x22,
    CURSOR_NEXT = 0x23,
    CURSOR_CLOSE = 0x24,
    
    TRANSACTION_BEGIN = 0x30,
    TRANSACTION_COMMIT = 0x31,
    TRANSACTION_ABORT = 0x32,
    
    VECTOR_SEARCH = 0x40,
    GRAPH_TRAVERSE = 0x41,
    
    GEO_QUERY = 0x50,
    TIMESERIES_QUERY = 0x51,
    
    BPMN_START_PROCESS = 0x60,
    BPMN_TASK_COMPLETE = 0x61,
    BPMN_QUERY_INSTANCE = 0x62,
    
    ERROR = 0xF0,
    OK = 0xF1,
    PING = 0xFE,
    PONG = 0xFE,
    CLOSE = 0xFF
};

// Message Flags
enum class MessageFlags : uint16_t {
    NONE = 0x0000,
    SKIP_CHECKSUM = 0x0001,  // Checksum optional (TLS enabled)
    COMPRESSED = 0x0002,      // Payload is LZ4 compressed
    ENCRYPTED = 0x0004        // Payload is encrypted (ChaCha20-Poly1305)
};

// =============================================================================
// Wire Frame Header
// =============================================================================

#pragma pack(push, 1)
struct WireFrameHeader {
    uint32_t magic;           // 0x544D4442 ("TMDB")
    uint8_t version;          // 0x01
    uint8_t opcode;           // Operation code
    uint16_t flags;           // Message flags
    uint32_t payload_length;  // Payload size in bytes
    
    bool is_valid() const {
        return magic == WIRE_MAGIC && version == WIRE_VERSION_1;
    }
    
    OpCode get_opcode() const {
        return static_cast<OpCode>(opcode);
    }
    
    bool has_flag(MessageFlags flag) const {
        return (flags & static_cast<uint16_t>(flag)) != 0;
    }
};
#pragma pack(pop)

static_assert(sizeof(WireFrameHeader) == HEADER_SIZE, "Header must be 12 bytes");

// =============================================================================
// Wire Protocol Session
// =============================================================================

class WireProtocolSession : public std::enable_shared_from_this<WireProtocolSession> {
public:
    using socket_t = boost::asio::ip::tcp::socket;
    using error_code = boost::system::error_code;
    
    WireProtocolSession(socket_t socket);
    ~WireProtocolSession();
    
    void start();
    void close(const std::string& reason = "");
    
    const std::string& session_id() const { return session_id_; }
    bool is_authenticated() const { return authenticated_; }
    const std::string& username() const { return username_; }
    
private:
    // Async read/write operations
    void async_read_header();
    void async_read_payload(const WireFrameHeader& header);
    void async_write_response(OpCode opcode, const google::protobuf::Message& message);
    
    // Message handlers
    void handle_hello(const v1::HelloRequest& req);
    void handle_auth_response(const v1::AuthResponse& req);
    void handle_get(const v1::GetRequest& req);
    void handle_put(const v1::PutRequest& req);
    void handle_delete(const v1::DeleteRequest& req);
    void handle_query_aql(const v1::QueryRequest& req);
    void handle_vector_search(const v1::VectorSearchRequest& req);
    void handle_geo_query(const v1::GeoQueryRequest& req);
    void handle_timeseries_query(const v1::TimeSeriesQueryRequest& req);
    void handle_bpmn_start(const v1::BpmnStartProcessRequest& req);
    void handle_ping(const v1::PingRequest& req);
    void handle_close(const v1::CloseRequest& req);
    
    // Utility methods
    void send_error(uint32_t error_code, const std::string& message);
    void send_ok(const std::string& message = "");
    uint32_t compute_checksum(const WireFrameHeader& header, const std::vector<uint8_t>& payload);
    bool verify_checksum(const WireFrameHeader& header, const std::vector<uint8_t>& payload, uint32_t checksum);
    
    std::vector<uint8_t> decompress_lz4(const std::vector<uint8_t>& compressed);
    std::vector<uint8_t> compress_lz4(const std::vector<uint8_t>& data);
    
    socket_t socket_;
    std::string session_id_;
    bool authenticated_;
    std::string username_;
    std::string namespace_;
    
    std::vector<uint8_t> read_buffer_;
    std::vector<uint8_t> write_buffer_;
    
    // Statistics
    uint64_t messages_received_;
    uint64_t messages_sent_;
    uint64_t bytes_received_;
    uint64_t bytes_sent_;
};

// =============================================================================
// Wire Protocol Server
// =============================================================================

class WireProtocolServer {
public:
    using acceptor_t = boost::asio::ip::tcp::acceptor;
    using endpoint_t = boost::asio::ip::tcp::endpoint;
    
    WireProtocolServer(boost::asio::io_context& io_context, uint16_t port);
    ~WireProtocolServer();
    
    void start();
    void stop();
    
    // Statistics
    size_t active_sessions() const;
    uint64_t total_connections() const { return total_connections_; }
    uint64_t total_messages() const { return total_messages_; }
    
private:
    void async_accept();
    void handle_accept(std::shared_ptr<WireProtocolSession> session, const boost::system::error_code& error);
    
    boost::asio::io_context& io_context_;
    acceptor_t acceptor_;
    std::unordered_map<std::string, std::shared_ptr<WireProtocolSession>> sessions_;
    
    uint16_t port_;
    uint64_t total_connections_;
    uint64_t total_messages_;
    bool running_;
};

// =============================================================================
// Message Dispatcher
// =============================================================================

class MessageDispatcher {
public:
    using handler_fn = std::function<void(WireProtocolSession&, const std::vector<uint8_t>&)>;
    
    void register_handler(OpCode opcode, handler_fn handler);
    void dispatch(WireProtocolSession& session, OpCode opcode, const std::vector<uint8_t>& payload);
    
private:
    std::unordered_map<OpCode, handler_fn> handlers_;
};

} // namespace wire
} // namespace themis
