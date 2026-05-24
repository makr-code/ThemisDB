/*
 * ThemisDB | File: wire_protocol_server.hpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=7, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#include <mutex>
#include <boost/asio.hpp>
#include <google/protobuf/message.h>

#ifdef ERROR
#undef ERROR
#endif

#ifdef DELETE
#undef DELETE
#endif

#ifdef GET
#undef GET
#endif

#ifdef PUT
#undef PUT
#endif

#ifdef PING
#undef PING
#endif

#ifdef PONG
#undef PONG
#endif

#ifdef CLOSE
#undef CLOSE
#endif

#if __has_include("themis_wire_v1.pb.h")
#define THEMIS_WIRE_V1_PB_HEADER_FOUND 1
#include "themis_wire_v1.pb.h"
#else
#define THEMIS_WIRE_V1_PB_HEADER_FOUND 0
namespace themis {
namespace wire {
namespace v1 {
class HelloRequest;
class AuthResponse;
class GetRequest;
class PutRequest;
class DeleteRequest;
class BatchGetRequest;
class BatchPutRequest;
class QueryRequest;
class CursorNextRequest;
class CursorCloseRequest;
class TransactionBeginRequest;
class TransactionCommitRequest;
class TransactionAbortRequest;
class VectorSearchRequest;
class GeoQueryRequest;
class TimeSeriesQueryRequest;
class BpmnStartProcessRequest;
class BpmnTaskCompleteRequest;
class BpmnQueryInstanceRequest;
class PingRequest;
class CloseRequest;
}
}
}
#endif

namespace themis {

// Forward declarations for injectable engine types (stub #281)
class QueryEngine;
class TSStore;
class ProcessGraphManager;
namespace index { class SpatialIndexManager; }

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
    OP_HELLO = 0x01,
    OP_HELLO_ACK = 0x02,
    OP_AUTH_REQUEST = 0x03,
    OP_AUTH_RESPONSE = 0x04,
    OP_AUTH_SUCCESS = 0x05,
    OP_AUTH_FAILURE = 0x06,
    
    OP_GET = 0x10,
    OP_PUT = 0x11,
    OP_DELETE = 0x12,
    OP_BATCH_GET = 0x13,
    OP_BATCH_PUT = 0x14,
    
    OP_QUERY_AQL = 0x20,
    OP_QUERY_RESULT = 0x21,
    OP_QUERY_CURSOR = 0x22,
    OP_CURSOR_NEXT = 0x23,
    OP_CURSOR_CLOSE = 0x24,
    
    OP_TRANSACTION_BEGIN = 0x30,
    OP_TRANSACTION_COMMIT = 0x31,
    OP_TRANSACTION_ABORT = 0x32,
    
    OP_VECTOR_SEARCH = 0x40,
    OP_GRAPH_TRAVERSE = 0x41,
    
    OP_GEO_QUERY = 0x50,
    OP_TIMESERIES_QUERY = 0x51,
    
    OP_BPMN_START_PROCESS = 0x60,
    OP_BPMN_TASK_COMPLETE = 0x61,
    OP_BPMN_QUERY_INSTANCE = 0x62,
    
    OP_ERROR = 0xF0,
    OP_OK = 0xF1,
    OP_PING = 0xFE,
    OP_PONG = 0xFE,
    OP_CLOSE = 0xFF
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
// Engine injection configuration (stub #281)
// Declared before WireProtocolSession so the session can reference it.
// =============================================================================

/**
 * @brief Engine references for the Protobuf wire protocol handlers.
 *
 * Passed to WireProtocolServer at construction time and forwarded to each
 * new WireProtocolSession.  All fields are optional shared_ptrs; a null
 * pointer keeps the corresponding handler returning a redirect error.
 */
struct WireEngineConfig {
    /// @brief AQL query engine (enables QUERY_AQL, CURSOR_NEXT, CURSOR_CLOSE)
    std::shared_ptr<QueryEngine> query_engine;
    /// @brief Spatial index for GEO_QUERY dispatch
    std::shared_ptr<index::SpatialIndexManager> spatial_index;
    /// @brief Time-series store for TIMESERIES_QUERY dispatch
    std::shared_ptr<TSStore> ts_store;
    /// @brief BPMN process graph manager (enables BPMN_START, BPMN_TASK_COMPLETE,
    ///        BPMN_QUERY_INSTANCE)
    std::shared_ptr<ProcessGraphManager> process_graph;
};

// =============================================================================
// Wire Protocol Session
// =============================================================================

// ---------------------------------------------------------------------------
// Injection bridge type aliases for WireProtocolSession (stub #281 replacement)
// ---------------------------------------------------------------------------

/**
 * @brief AQL query execution bridge for the Protobuf wire protocol.
 *
 * When set, QUERY_AQL messages are dispatched to this function instead of
 * returning HTTP 501.  The function receives the AQL string and the
 * authenticated session's namespace and must return a JSON-encoded result
 * string.  An exception or empty return keeps the error path.
 *
 * @param aql  AQL query string.
 * @param ns   Namespace / tenant derived from the authenticated session.
 * @return     JSON-encoded query result.
 */
using WireAqlExecFn = std::function<std::string(
    const std::string& aql, const std::string& ns)>;

/**
 * @brief Cursor next-page bridge for the Protobuf wire protocol.
 *
 * @param cursor_id  Cursor identifier from the CURSOR_NEXT request.
 * @return           JSON-encoded next page of results.
 */
using WireCursorNextFn = std::function<std::string(const std::string& cursor_id)>;

/**
 * @brief Cursor close bridge for the Protobuf wire protocol.
 *
 * @param cursor_id  Cursor identifier from the CURSOR_CLOSE request.
 * @return           true if the cursor was found and closed, false otherwise.
 */
using WireCursorCloseFn = std::function<bool(const std::string& cursor_id)>;

/**
 * @brief Geospatial query bridge for the Protobuf wire protocol.
 *
 * @param collection  Target collection name.
 * @param lat         Latitude of the search centre (WGS84, decimal degrees).
 * @param lon         Longitude of the search centre (WGS84, decimal degrees).
 * @param radius_m    Search radius in metres.
 * @param limit       Maximum number of results.
 * @return            JSON-encoded array of matching document objects.
 */
using WireGeoQueryFn = std::function<std::string(
    const std::string& collection, double lat, double lon,
    double radius_m, int limit)>;

/**
 * @brief Time-series query bridge for the Protobuf wire protocol.
 *
 * @param collection  Collection / series name.
 * @param start_ns    Start of the query range (nanoseconds since Unix epoch).
 * @param end_ns      End of the query range (nanoseconds since Unix epoch).
 * @return            JSON-encoded time-series data points.
 */
using WireTSQueryFn = std::function<std::string(
    const std::string& collection, int64_t start_ns, int64_t end_ns)>;

/**
 * @brief Graph traversal bridge for the Protobuf wire protocol.
 *
 * @param collection   Collection / edge-set name.
 * @param start_vertex Start vertex identifier.
 * @param max_depth    Maximum traversal depth (0 = unlimited).
 * @return             JSON-encoded traversal result.
 */
using WireGraphTraversalFn = std::function<std::string(
    const std::string& collection, const std::string& start_vertex, int max_depth)>;

/**
 * @brief Register the AQL execution bridge for the Protobuf wire protocol.
 *
 * Thread-safe.  Pass nullptr to clear.  Registered once at server startup.
 */
void setWireAqlExecFn(WireAqlExecFn fn);

/**
 * @brief Register the cursor next-page bridge for the Protobuf wire protocol.
 * Thread-safe.  Pass nullptr to clear.
 */
void setWireCursorNextFn(WireCursorNextFn fn);

/**
 * @brief Register the cursor close bridge for the Protobuf wire protocol.
 * Thread-safe.  Pass nullptr to clear.
 */
void setWireCursorCloseFn(WireCursorCloseFn fn);

/**
 * @brief Register the geospatial query bridge for the Protobuf wire protocol.
 * Thread-safe.  Pass nullptr to clear.
 */
void setWireGeoQueryFn(WireGeoQueryFn fn);

/**
 * @brief Register the time-series query bridge for the Protobuf wire protocol.
 * Thread-safe.  Pass nullptr to clear.
 */
void setWireTSQueryFn(WireTSQueryFn fn);

/**
 * @brief Register the graph traversal bridge for the Protobuf wire protocol.
 * Thread-safe.  Pass nullptr to clear.
 */
void setWireGraphTraversalFn(WireGraphTraversalFn fn);

class WireProtocolSession : public std::enable_shared_from_this<WireProtocolSession> {
public:
    using socket_t = boost::asio::ip::tcp::socket;
    using error_code = boost::system::error_code;
    
    explicit WireProtocolSession(socket_t socket);
    ~WireProtocolSession();
    
    void start();
    void close(const std::string& reason = "");
    void set_disconnect_callback(std::function<void(const std::string&)> callback);

    /**
     * @brief Inject engine references for stub #281.
     *
     * Called by WireProtocolServer immediately after session construction.
     * The pointer is non-owning (the WireProtocolServer's engines_ field owns
     * the WireEngineConfig).
     */
    void set_engines(const WireEngineConfig* engines) noexcept {
        engines_ = engines;
    }
    
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
    void handle_batch_get(const v1::BatchGetRequest& req);
    void handle_batch_put(const v1::BatchPutRequest& req);
    void handle_query_aql(const v1::QueryRequest& req);
    void handle_cursor_next(const v1::CursorNextRequest& req);
    void handle_cursor_close(const v1::CursorCloseRequest& req);
    void handle_transaction_begin(const v1::TransactionBeginRequest& req);
    void handle_transaction_commit(const v1::TransactionCommitRequest& req);
    void handle_transaction_abort(const v1::TransactionAbortRequest& req);
    void handle_vector_search(const v1::VectorSearchRequest& req);
    void handle_graph_traverse();
    void handle_geo_query(const v1::GeoQueryRequest& req);
    void handle_timeseries_query(const v1::TimeSeriesQueryRequest& req);
    void handle_bpmn_start(const v1::BpmnStartProcessRequest& req);
    void handle_bpmn_task_complete(const v1::BpmnTaskCompleteRequest& req);
    void handle_bpmn_query_instance(const v1::BpmnQueryInstanceRequest& req);
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
    std::function<void(const std::string&)> disconnect_callback_;
    bool disconnect_notified_;
    mutable std::mutex session_mutex_;

    // Non-owning pointer to server-level engine config (stub #281).
    // Set by WireProtocolServer::handle_accept via set_engines().
    // Null when no engines have been injected (handlers redirect to REST API).
    const WireEngineConfig* engines_ = nullptr;
    
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

    /// @brief Minimal constructor (no engine injection; all advanced handlers return redirect errors).
    WireProtocolServer(boost::asio::io_context& io_context, uint16_t port);

    /**
     * @brief Engine-injected constructor.
     *
     * @param io_context Boost.Asio I/O context for async network operations.
     * @param port        TCP port to listen on.
     * @param engines     Engine references to wire into session handlers.
     */
    WireProtocolServer(boost::asio::io_context& io_context, uint16_t port,
                       WireEngineConfig engines);

    ~WireProtocolServer();
    
    void start();
    void stop();
    
    // Statistics
    size_t active_sessions() const;
    uint64_t total_connections() const;
    uint64_t total_messages() const;
    
private:
    void async_accept();
    void handle_accept(std::shared_ptr<WireProtocolSession> session, const boost::system::error_code& error);
    
    boost::asio::io_context& io_context_;
    acceptor_t acceptor_;
    std::unordered_map<std::string, std::shared_ptr<WireProtocolSession>> sessions_;
    mutable std::mutex state_mutex_;
    
    uint16_t port_;
    uint64_t total_connections_;
    uint64_t total_messages_;
    bool running_;

    WireEngineConfig engines_;
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
