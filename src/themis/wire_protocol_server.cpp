/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server.cpp                           ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:51:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1143                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol Server – themis::wire module implementation
//
// Implements WireProtocolSession, WireProtocolServer, and MessageDispatcher
// as declared in include/themis/network/wire_protocol_server.hpp.
//
// Phase-3 deliverable for the Themis Core Framework module (ROADMAP item:
// "wire_protocol_server.cpp – move wire protocol implementation from
// src/server/"). Classes live in namespace themis::wire and are compiled into
// themis_core alongside the existing src/network/wire_protocol_server.cpp
// (themis::network namespace) for backward compatibility during the v1.7.0
// migration window.

#include "themis/network/wire_protocol_server.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <boost/asio.hpp>

#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#endif

namespace themis {
namespace wire {

namespace net = boost::asio;
using tcp        = net::ip::tcp;
using error_code = boost::system::error_code;

// ============================================================================
// MessageDispatcher
// ============================================================================

void MessageDispatcher::register_handler(OpCode opcode, handler_fn handler) {
    handlers_[opcode] = std::move(handler);
}

void MessageDispatcher::dispatch(WireProtocolSession&        session,
                                 OpCode                      opcode,
                                 const std::vector<uint8_t>& payload) {
    auto it = handlers_.find(opcode);
    if (it != handlers_.end()) {
        it->second(session, payload);
    } else {
        std::cerr << "[WireV1] No handler for opcode 0x"
                  << std::hex << static_cast<unsigned>(opcode) << std::dec
                  << '\n';
    }
}

// ============================================================================
// Internal helpers (anonymous namespace)
// ============================================================================

namespace {

// CRC32 (ISO-HDLC) look-up table.
static const uint32_t kCrc32Table[256] = {
    0x00000000u, 0x77073096u, 0xEE0E612Cu, 0x990951BAu,
    0x076DC419u, 0x706AF48Fu, 0xE963A535u, 0x9E6495A3u,
    0x0EDB8832u, 0x79DCB8A4u, 0xE0D5E91Eu, 0x97D2D988u,
    0x09B64C2Bu, 0x7EB17CBDu, 0xE7B82D07u, 0x90BF1D91u,
    0x1DB71064u, 0x6AB020F2u, 0xF3B97148u, 0x84BE41DEu,
    0x1ADAD47Du, 0x6DDDE4EBu, 0xF4D4B551u, 0x83D385C7u,
    0x136C9856u, 0x646BA8C0u, 0xFD62F97Au, 0x8A65C9ECu,
    0x14015C4Fu, 0x63066CD9u, 0xFA0F3D63u, 0x8D080DF5u,
    0x3B6E20C8u, 0x4C69105Eu, 0xD56041E4u, 0xA2677172u,
    0x3C03E4D1u, 0x4B04D447u, 0xD20D85FDu, 0xA50AB56Bu,
    0x35B5A8FAu, 0x42B2986Cu, 0xDBBBC9D6u, 0xACBCF940u,
    0x32D86CE3u, 0x45DF5C75u, 0xDCD60DCFu, 0xABD13D59u,
    0x26D930ACu, 0x51DE003Au, 0xC8D75180u, 0xBFD06116u,
    0x21B4F4B5u, 0x56B3C423u, 0xCFBA9599u, 0xB8BDA50Fu,
    0x2802B89Eu, 0x5F058808u, 0xC60CD9B2u, 0xB10BE924u,
    0x2F6F7C87u, 0x58684C11u, 0xC1611DABu, 0xB6662D3Du,
    0x76DC4190u, 0x01DB7106u, 0x98D220BCu, 0xEFD5102Au,
    0x71B18589u, 0x06B6B51Fu, 0x9FBFE4A5u, 0xE8B8D433u,
    0x7807C9A2u, 0x0F00F934u, 0x9609A88Eu, 0xE10E9818u,
    0x7F6A0DBBu, 0x086D3D2Du, 0x91646C97u, 0xE6635C01u,
    0x6B6B51F4u, 0x1C6C6162u, 0x856530D8u, 0xF262004Eu,
    0x6C0695EDu, 0x1B01A57Bu, 0x8208F4C1u, 0xF50FC457u,
    0x65B0D9C6u, 0x12B7E950u, 0x8BBEB8EAu, 0xFCB9887Cu,
    0x62DD1DDFu, 0x15DA2D49u, 0x8CD37CF3u, 0xFBD44C65u,
    0x4DB26158u, 0x3AB551CEu, 0xA3BC0074u, 0xD4BB30E2u,
    0x4ADFA541u, 0x3DD895D7u, 0xA4D1C46Du, 0xD3D6F4FBu,
    0x4369E96Au, 0x346ED9FCu, 0xAD678846u, 0xDA60B8D0u,
    0x44042D73u, 0x33031DE5u, 0xAA0A4C5Fu, 0xDD0D7CC9u,
    0x5005713Cu, 0x270241AAu, 0xBE0B1010u, 0xC90C2086u,
    0x5768B525u, 0x206F85B3u, 0xB966D409u, 0xCE61E49Fu,
    0x5EDEF90Eu, 0x29D9C998u, 0xB0D09822u, 0xC7D7A8B4u,
    0x59B33D17u, 0x2EB40D81u, 0xB7BD5C3Bu, 0xC0BA6CADu,
    0xEDB88320u, 0x9ABFB3B6u, 0x03B6E20Cu, 0x74B1D29Au,
    0xEAD54739u, 0x9DD277AFu, 0x04DB2615u, 0x73DC1683u,
    0xE3630B12u, 0x94643B84u, 0x0D6D6A3Eu, 0x7A6A5AA8u,
    0xE40ECF0Bu, 0x9309FF9Du, 0x0A00AE27u, 0x7D079EB1u,
    0xF00F9344u, 0x8708A3D2u, 0x1E01F268u, 0x6906C2FEu,
    0xF762575Du, 0x806567CBu, 0x196C3671u, 0x6E6B06E7u,
    0xFED41B76u, 0x89D32BE0u, 0x10DA7A5Au, 0x67DD4ACCu,
    0xF9B9DF6Fu, 0x8EBEEFF9u, 0x17B7BE43u, 0x60B08ED5u,
    0xD6D6A3E8u, 0xA1D1937Eu, 0x38D8C2C4u, 0x4FDFF252u,
    0xD1BB67F1u, 0xA6BC5767u, 0x3FB506DDu, 0x48B2364Bu,
    0xD80D2BDAu, 0xAF0A1B4Cu, 0x36034AF6u, 0x41047A60u,
    0xDF60EFC3u, 0xA867DF55u, 0x316E8EEFu, 0x4669BE79u,
    0xCB61B38Cu, 0xBC66831Au, 0x256FD2A0u, 0x5268E236u,
    0xCC0C7795u, 0xBB0B4703u, 0x220216B9u, 0x5505262Fu,
    0xC5BA3BBEu, 0xB2BD0B28u, 0x2BB45A92u, 0x5CB36A04u,
    0xC2D7FFA7u, 0xB5D0CF31u, 0x2CD99E8Bu, 0x5BDEAE1Du,
    0x9B64C2B0u, 0xEC63F226u, 0x756AA39Cu, 0x026D930Au,
    0x9C0906A9u, 0xEB0E363Fu, 0x72076785u, 0x05005713u,
    0x95BF4A82u, 0xE2B87A14u, 0x7BB12BAEu, 0x0CB61B38u,
    0x92D28E9Bu, 0xE5D5BE0Du, 0x7CDCEFB7u, 0x0BDBDF21u,
    0x86D3D2D4u, 0xF1D4E242u, 0x68DDB3F8u, 0x1FDA836Eu,
    0x81BE16CDu, 0xF6B9265Bu, 0x6FB077E1u, 0x18B74777u,
    0x88085AE6u, 0xFF0F6A70u, 0x66063BCAu, 0x11010B5Cu,
    0x8F659EFFu, 0xF862AE69u, 0x616BFFD3u, 0x166CCF45u,
    0xA00AE278u, 0xD70DD2EEu, 0x4E048354u, 0x3903B3C2u,
    0xA7672661u, 0xD06016F7u, 0x4969474Du, 0x3E6E77DBu,
    0xAED16A4Au, 0xD9D65ADCu, 0x40DF0B66u, 0x37D83BF0u,
    0xA9BCAE53u, 0xDEBB9EC5u, 0x47B2CF7Fu, 0x30B5FFE9u,
    0xBDBDF21Cu, 0xCABAC28Au, 0x53B39330u, 0x24B4A3A6u,
    0xBAD03605u, 0xCDD70693u, 0x54DE5729u, 0x23D967BFu,
    0xB3667A2Eu, 0xC4614AB8u, 0x5D681B02u, 0x2A6F2B94u,
    0xB40BBE37u, 0xC30C8EA1u, 0x5A05DF1Bu, 0x2D02EF8Du,
};

/// CRC32 over an arbitrary byte range (initial value 0).
static uint32_t crc32Compute(const uint8_t* data, std::size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < len; ++i)
        crc = kCrc32Table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

/// Serialize WireFrameHeader into a 12-byte big-endian wire buffer.
static std::array<uint8_t, HEADER_SIZE> serializeHeader(const WireFrameHeader& h) {
    std::array<uint8_t, HEADER_SIZE> buf{};
    const uint32_t magic_be = htonl(h.magic);
    std::memcpy(buf.data(), &magic_be, 4);
    buf[4] = h.version;
    buf[5] = h.opcode;
    const uint16_t flags_be = htons(h.flags);
    std::memcpy(buf.data() + 6, &flags_be, 2);
    const uint32_t len_be = htonl(h.payload_length);
    std::memcpy(buf.data() + 8, &len_be, 4);
    return buf;
}

/// Deserialize WireFrameHeader from a 12-byte big-endian wire buffer.
static WireFrameHeader deserializeHeader(const uint8_t* buf) {
    WireFrameHeader h{};
    uint32_t magic_be;
    std::memcpy(&magic_be, buf, 4);
    h.magic = ntohl(magic_be);
    h.version = buf[4];
    h.opcode  = buf[5];
    uint16_t flags_be;
    std::memcpy(&flags_be, buf + 6, 2);
    h.flags = ntohs(flags_be);
    uint32_t len_be;
    std::memcpy(&len_be, buf + 8, 4);
    h.payload_length = ntohl(len_be);
    return h;
}

/// Produce a session-ID string from remote endpoint + monotonic timestamp.
static std::string makeSessionId(const tcp::socket& socket) {
    std::ostringstream ss;
    try {
        ss << socket.remote_endpoint().address().to_string()
           << ':' << socket.remote_endpoint().port()
           << '@' << std::chrono::steady_clock::now().time_since_epoch().count();
    } catch (...) {
        ss << "unknown@"
           << std::chrono::steady_clock::now().time_since_epoch().count();
    }
    return ss.str();
}

/// Sanitize a user-supplied string for safe inclusion in error messages.
/// Replaces control characters (< 0x20) and DEL (0x7F) with '?' to prevent
/// log injection and client confusion via embedded newlines or escape sequences.
static std::string sanitizeForMessage(const std::string& s) {
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

// ============================================================================
// WireProtocolSession
// ============================================================================

WireProtocolSession::WireProtocolSession(socket_t socket)
    : socket_(std::move(socket))
    , session_id_(makeSessionId(socket_))
    , authenticated_(false)
    , disconnect_notified_(false)
    , messages_received_(0)
    , messages_sent_(0)
    , bytes_received_(0)
    , bytes_sent_(0)
{}

WireProtocolSession::~WireProtocolSession() {
    close();
}

void WireProtocolSession::start() {
    async_read_header();
}

void WireProtocolSession::set_disconnect_callback(
    std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(session_mutex_);
    disconnect_callback_ = std::move(callback);
}

void WireProtocolSession::close(const std::string& /*reason*/) {
    std::function<void(const std::string&)> disconnect_callback;
    {
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (disconnect_notified_) return;
        disconnect_notified_ = true;
        disconnect_callback = disconnect_callback_;
    }

    error_code ec;
    if (socket_.is_open()) {
        socket_.shutdown(tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    if (disconnect_callback) {
        disconnect_callback(session_id_);
    }
}

// ---- async read pipeline ----

void WireProtocolSession::async_read_header() {
    auto self = shared_from_this();
    read_buffer_.resize(HEADER_SIZE);

    net::async_read(
        socket_,
        net::buffer(read_buffer_, HEADER_SIZE),
        [this, self](const error_code& ec, std::size_t bytes_read) {
            if (ec) {
                if (ec != net::error::eof &&
                    ec != net::error::operation_aborted)
                    std::cerr << "[WireV1:" << session_id_
                              << "] header read: " << ec.message() << '\n';
                close();
                return;
            }
            bytes_received_ += bytes_read;

            const WireFrameHeader hdr =
                deserializeHeader(read_buffer_.data());
            if (!hdr.is_valid()) {
                send_error(0x01, "Invalid frame header");
                return;
            }
            if (hdr.payload_length > MAX_PAYLOAD_SIZE) {
                send_error(0x02, "Payload exceeds maximum allowed size");
                return;
            }
            async_read_payload(hdr);
        });
}

void WireProtocolSession::async_read_payload(const WireFrameHeader& header) {
    auto self = shared_from_this();
    const bool        with_checksum =
        !header.has_flag(MessageFlags::SKIP_CHECKSUM);
    const std::size_t total =
        header.payload_length + (with_checksum ? CHECKSUM_SIZE : 0u);

    if (total == 0u) {
        ++messages_received_;
        // Dispatch with empty payload inline (captures `this`).
        const OpCode opcode = header.get_opcode();
#if !defined(THEMIS_WIRE_V1_PROTO_AVAILABLE) || !THEMIS_WIRE_V1_PB_HEADER_FOUND
        send_error(0xFF, "Protobuf not compiled in this build");
#else
        switch (opcode) {
            case OpCode::OP_PING:   handle_ping(v1::PingRequest{});     break;
            case OpCode::OP_CLOSE:  handle_close(v1::CloseRequest{});   break;
            default:
                send_error(0x04, "Unsupported opcode for empty payload");
                break;
        }
#endif
        async_read_header();
        return;
    }

    read_buffer_.resize(total);

    net::async_read(
        socket_,
        net::buffer(read_buffer_, total),
        [this, self, header, with_checksum](const error_code& ec,
                                            std::size_t       bytes_read) {
            if (ec) {
                if (ec != net::error::eof &&
                    ec != net::error::operation_aborted)
                    std::cerr << "[WireV1:" << session_id_
                              << "] payload read: " << ec.message() << '\n';
                close();
                return;
            }
            bytes_received_ += bytes_read;

            std::vector<uint8_t> payload(
                read_buffer_.begin(),
                read_buffer_.begin() +
                    static_cast<std::ptrdiff_t>(header.payload_length));

            if (with_checksum && bytes_read >= CHECKSUM_SIZE) {
                uint32_t recv_crc_be;
                std::memcpy(&recv_crc_be,
                            read_buffer_.data() + header.payload_length,
                            CHECKSUM_SIZE);
                if (!verify_checksum(header, payload, ntohl(recv_crc_be))) {
                    send_error(0x03, "Checksum mismatch");
                    return;
                }
            }

            ++messages_received_;

            // Dispatch to the appropriate handler.  Because this lambda
            // captures `this` (as `self`), private members are accessible.
            const OpCode opcode = header.get_opcode();
            const int    isz    = static_cast<int>(payload.size());
#if defined(THEMIS_WIRE_V1_PROTO_AVAILABLE) && THEMIS_WIRE_V1_PB_HEADER_FOUND
            // Helper lambda: log a parse failure and send an error response.
            auto on_parse_fail = [this](const char* name) {
                std::cerr << "[WireV1:" << session_id_
                          << "] protobuf parse failed for " << name << '\n';
                send_error(0x06, std::string("Malformed protobuf message: ") + name);
            };
            switch (opcode) {
                case OpCode::OP_HELLO: {
                    v1::HelloRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_hello(req);
                    else
                        on_parse_fail("HelloRequest");
                    break;
                }
                case OpCode::OP_AUTH_RESPONSE: {
                    v1::AuthResponse req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_auth_response(req);
                    else
                        on_parse_fail("AuthResponse");
                    break;
                }
                case OpCode::OP_GET: {
                    v1::GetRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_get(req);
                    else
                        on_parse_fail("GetRequest");
                    break;
                }
                case OpCode::OP_PUT: {
                    v1::PutRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_put(req);
                    else
                        on_parse_fail("PutRequest");
                    break;
                }
                case OpCode::OP_DELETE: {
                    v1::DeleteRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_delete(req);
                    else
                        on_parse_fail("DeleteRequest");
                    break;
                }
                case OpCode::OP_BATCH_GET: {
                    v1::BatchGetRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_batch_get(req);
                    else
                        on_parse_fail("BatchGetRequest");
                    break;
                }
                case OpCode::OP_BATCH_PUT: {
                    v1::BatchPutRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_batch_put(req);
                    else
                        on_parse_fail("BatchPutRequest");
                    break;
                }
                case OpCode::OP_QUERY_AQL: {
                    v1::QueryRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_query_aql(req);
                    else
                        on_parse_fail("QueryRequest");
                    break;
                }
                case OpCode::OP_CURSOR_NEXT: {
                    v1::CursorNextRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_cursor_next(req);
                    else
                        on_parse_fail("CursorNextRequest");
                    break;
                }
                case OpCode::OP_CURSOR_CLOSE: {
                    v1::CursorCloseRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_cursor_close(req);
                    else
                        on_parse_fail("CursorCloseRequest");
                    break;
                }
                case OpCode::OP_TRANSACTION_BEGIN: {
                    v1::TransactionBeginRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_transaction_begin(req);
                    else
                        on_parse_fail("TransactionBeginRequest");
                    break;
                }
                case OpCode::OP_TRANSACTION_COMMIT: {
                    v1::TransactionCommitRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_transaction_commit(req);
                    else
                        on_parse_fail("TransactionCommitRequest");
                    break;
                }
                case OpCode::OP_TRANSACTION_ABORT: {
                    v1::TransactionAbortRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_transaction_abort(req);
                    else
                        on_parse_fail("TransactionAbortRequest");
                    break;
                }
                case OpCode::OP_VECTOR_SEARCH: {
                    v1::VectorSearchRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_vector_search(req);
                    else
                        on_parse_fail("VectorSearchRequest");
                    break;
                }
                case OpCode::OP_GRAPH_TRAVERSE:
                    handle_graph_traverse();
                    break;
                case OpCode::OP_GEO_QUERY: {
                    v1::GeoQueryRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_geo_query(req);
                    else
                        on_parse_fail("GeoQueryRequest");
                    break;
                }
                case OpCode::OP_TIMESERIES_QUERY: {
                    v1::TimeSeriesQueryRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_timeseries_query(req);
                    else
                        on_parse_fail("TimeSeriesQueryRequest");
                    break;
                }
                case OpCode::OP_BPMN_START_PROCESS: {
                    v1::BpmnStartProcessRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_bpmn_start(req);
                    else
                        on_parse_fail("BpmnStartProcessRequest");
                    break;
                }
                case OpCode::OP_BPMN_TASK_COMPLETE: {
                    v1::BpmnTaskCompleteRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_bpmn_task_complete(req);
                    else
                        on_parse_fail("BpmnTaskCompleteRequest");
                    break;
                }
                case OpCode::OP_BPMN_QUERY_INSTANCE: {
                    v1::BpmnQueryInstanceRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_bpmn_query_instance(req);
                    else
                        on_parse_fail("BpmnQueryInstanceRequest");
                    break;
                }
                case OpCode::OP_PING: {
                    v1::PingRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_ping(req);
                    else
                        on_parse_fail("PingRequest");
                    break;
                }
                case OpCode::OP_CLOSE: {
                    v1::CloseRequest req;
                    if (req.ParseFromArray(payload.data(), isz))
                        handle_close(req);
                    else
                        on_parse_fail("CloseRequest");
                    break;
                }
                default:
                    send_error(0x04, "Unsupported opcode");
                    break;
            }
#else
            send_error(0xFF, "Protobuf not compiled in this build");
#endif
            async_read_header();
        });
}

void WireProtocolSession::async_write_response(
    OpCode opcode, const google::protobuf::Message& message) {
#if defined(THEMIS_WIRE_V1_PROTO_AVAILABLE) && THEMIS_WIRE_V1_PB_HEADER_FOUND
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        send_error(0x05, "Failed to serialize response");
        return;
    }

    WireFrameHeader resp_hdr{};
    resp_hdr.magic          = WIRE_MAGIC;
    resp_hdr.version        = WIRE_VERSION_1;
    resp_hdr.opcode         = static_cast<uint8_t>(opcode);
    resp_hdr.flags          = 0;
    resp_hdr.payload_length = static_cast<uint32_t>(serialized.size());

    const auto     hdr_bytes = serializeHeader(resp_hdr);
    const uint32_t crc    = crc32Compute(
        reinterpret_cast<const uint8_t*>(serialized.data()),
        serialized.size());
    const uint32_t crc_be = htonl(crc);

    write_buffer_.clear();
    write_buffer_.insert(write_buffer_.end(),
                         hdr_bytes.begin(), hdr_bytes.end());
    write_buffer_.insert(
        write_buffer_.end(),
        reinterpret_cast<const uint8_t*>(serialized.data()),
        reinterpret_cast<const uint8_t*>(serialized.data()) +
            serialized.size());
    write_buffer_.insert(
        write_buffer_.end(),
        reinterpret_cast<const uint8_t*>(&crc_be),
        reinterpret_cast<const uint8_t*>(&crc_be) + CHECKSUM_SIZE);

    auto self = shared_from_this();
    net::async_write(
        socket_,
        net::buffer(write_buffer_),
        [this, self](const error_code& ec, std::size_t written) {
            if (ec) {
                std::cerr << "[WireV1:" << session_id_
                          << "] write error: " << ec.message() << '\n';
                close();
                return;
            }
            bytes_sent_ += written;
            ++messages_sent_;
        });
#else
    send_error(0xFF, "Protobuf not compiled in this build");
#endif
}

// ---- utility methods ----

void WireProtocolSession::send_error(uint32_t           err_code,
                                     const std::string& message) {
    std::vector<uint8_t> payload(4u + message.size());
    const uint32_t code_be = htonl(err_code);
    std::memcpy(payload.data(), &code_be, 4u);
    std::memcpy(payload.data() + 4u, message.data(), message.size());

    WireFrameHeader hdr{};
    hdr.magic          = WIRE_MAGIC;
    hdr.version        = WIRE_VERSION_1;
    hdr.opcode         = static_cast<uint8_t>(OpCode::OP_ERROR);
    hdr.flags          = static_cast<uint16_t>(MessageFlags::SKIP_CHECKSUM);
    hdr.payload_length = static_cast<uint32_t>(payload.size());

    const auto hdr_bytes = serializeHeader(hdr);
    write_buffer_.clear();
    write_buffer_.insert(write_buffer_.end(),
                         hdr_bytes.begin(), hdr_bytes.end());
    write_buffer_.insert(write_buffer_.end(),
                         payload.begin(), payload.end());

    auto self = shared_from_this();
    net::async_write(
        socket_,
        net::buffer(write_buffer_),
        [this, self](const error_code& ec, std::size_t written) {
            if (!ec) {
                bytes_sent_ += written;
                ++messages_sent_;
            }
        });
}

void WireProtocolSession::send_ok(const std::string& message) {
    WireFrameHeader hdr{};
    hdr.magic          = WIRE_MAGIC;
    hdr.version        = WIRE_VERSION_1;
    hdr.opcode         = static_cast<uint8_t>(OpCode::OP_OK);
    hdr.flags          = static_cast<uint16_t>(MessageFlags::SKIP_CHECKSUM);
    hdr.payload_length = static_cast<uint32_t>(message.size());

    const auto hdr_bytes = serializeHeader(hdr);
    write_buffer_.clear();
    write_buffer_.insert(write_buffer_.end(),
                         hdr_bytes.begin(), hdr_bytes.end());
    write_buffer_.insert(write_buffer_.end(),
                         message.begin(), message.end());

    auto self = shared_from_this();
    net::async_write(
        socket_,
        net::buffer(write_buffer_),
        [this, self](const error_code& ec, std::size_t written) {
            if (!ec) {
                bytes_sent_ += written;
                ++messages_sent_;
            }
        });
}

uint32_t WireProtocolSession::compute_checksum(
    const WireFrameHeader&      header,
    const std::vector<uint8_t>& payload) {
    const auto hdr_bytes = serializeHeader(header);
    uint32_t crc = 0xFFFFFFFFu;
    for (const uint8_t b : hdr_bytes)
        crc = kCrc32Table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    for (const uint8_t b : payload)
        crc = kCrc32Table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    return ~crc;
}

bool WireProtocolSession::verify_checksum(
    const WireFrameHeader&      header,
    const std::vector<uint8_t>& payload,
    uint32_t                    checksum) {
    return compute_checksum(header, payload) == checksum;
}

std::vector<uint8_t> WireProtocolSession::decompress_lz4(
    [[maybe_unused]] const std::vector<uint8_t>& compressed) {
    // LZ4 decompression deferred until dependency is unconditionally available.
    return {};
}

std::vector<uint8_t> WireProtocolSession::compress_lz4(
    [[maybe_unused]] const std::vector<uint8_t>& data) {
    return {};
}

// ---- message handlers ----

#if defined(THEMIS_WIRE_V1_PROTO_AVAILABLE) && THEMIS_WIRE_V1_PB_HEADER_FOUND

void WireProtocolSession::handle_hello(const v1::HelloRequest& /*req*/) {
    send_ok("HELLO_ACK");
}

void WireProtocolSession::handle_auth_response(const v1::AuthResponse& req) {
    if (!req.username().empty()) {
        authenticated_ = true;
        username_       = req.username();
        namespace_      = req.namespace_();
        send_ok("AUTH_OK");
    } else {
        send_error(0x10, "Authentication failed: missing username");
    }
}

void WireProtocolSession::handle_get(const v1::GetRequest& req) {
    // GET: retrieve a document by collection and UUID.
    // Requires an authenticated session; validates collection and UUID fields.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in GET request");
        return;
    }
    if (req.uuid().empty()) {
        send_error(400, "Missing 'uuid' in GET request");
        return;
    }
    // Storage dispatch is managed by the themis::network::WireProtocolServer
    // (JSON wire protocol) and the HTTP REST API. This protobuf-based session
    // (themis::wire) does not yet hold a storage reference.
    send_error(503,
        "Storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "GET /api/v1/collection/" + sanitizeForMessage(req.collection()) +
        "/" + sanitizeForMessage(req.uuid()));
}

void WireProtocolSession::handle_put(const v1::PutRequest& req) {
    // PUT: store a document by collection and UUID.
    // Requires an authenticated session; validates collection, UUID, and entity.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in PUT request");
        return;
    }
    if (req.uuid().empty()) {
        send_error(400, "Missing 'uuid' in PUT request");
        return;
    }
    if (req.entity().empty()) {
        send_error(400, "Missing 'entity' in PUT request");
        return;
    }
    send_error(503,
        "Storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "PUT /api/v1/collection/" + sanitizeForMessage(req.collection()) +
        "/" + sanitizeForMessage(req.uuid()));
}

void WireProtocolSession::handle_delete(const v1::DeleteRequest& req) {
    // DELETE: remove a document by collection and UUID.
    // Requires an authenticated session; validates collection and UUID fields.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in DELETE request");
        return;
    }
    if (req.uuid().empty()) {
        send_error(400, "Missing 'uuid' in DELETE request");
        return;
    }
    send_error(503,
        "Storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "DELETE /api/v1/collection/" + sanitizeForMessage(req.collection()) +
        "/" + sanitizeForMessage(req.uuid()));
}

void WireProtocolSession::handle_query_aql(const v1::QueryRequest& req) {
    // QUERY_AQL: execute an AQL query string.
    // Requires authentication; validates that the AQL string is non-empty.
    // Full AQL engine integration over the protobuf wire protocol is planned for
    // a future release.  Clients should use HTTP POST /api/v1/query in the meantime.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.aql().empty()) {
        send_error(400, "Missing 'aql' field in QUERY_AQL request");
        return;
    }
    send_error(501,
        "AQL query execution is not yet integrated in the protobuf wire protocol. "
        "Use the HTTP REST API endpoint POST /api/v1/query instead.");
}

void WireProtocolSession::handle_cursor_next(const v1::CursorNextRequest& req) {
    // CURSOR_NEXT: fetch the next batch of results from an open AQL query cursor.
    // Requires authentication; validates cursor_id field.
    // Cursor-based streaming is not yet integrated in the protobuf wire protocol.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.cursor_id().empty()) {
        send_error(400, "Missing 'cursor_id' in CURSOR_NEXT request");
        return;
    }
    send_error(501,
        "Cursor pagination is not yet integrated in the protobuf wire protocol. "
        "Use the HTTP REST API endpoint GET /api/v1/cursor/" +
        sanitizeForMessage(req.cursor_id()) + " instead.");
}

void WireProtocolSession::handle_cursor_close(const v1::CursorCloseRequest& req) {
    // CURSOR_CLOSE: close an open AQL query cursor and release server-side resources.
    // Requires authentication; validates cursor_id field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.cursor_id().empty()) {
        send_error(400, "Missing 'cursor_id' in CURSOR_CLOSE request");
        return;
    }
    send_error(501,
        "Cursor management is not yet integrated in the protobuf wire protocol. "
        "Use the HTTP REST API endpoint DELETE /api/v1/cursor/" +
        sanitizeForMessage(req.cursor_id()) + " instead.");
}

void WireProtocolSession::handle_vector_search(
    const v1::VectorSearchRequest& req) {
    // VECTOR_SEARCH: k-nearest-neighbour search via VectorIndexManager.
    // Requires authentication; validates collection and non-empty query vector.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in VECTOR_SEARCH request");
        return;
    }
    if (req.vector_size() == 0) {
        send_error(400, "Empty query vector in VECTOR_SEARCH request");
        return;
    }
    // Vector index dispatch requires a VectorIndexManager reference that is
    // not yet injected into this protobuf wire session.
    send_error(503,
        "Vector index not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/vector/" + sanitizeForMessage(req.collection()) + "/search");
}

void WireProtocolSession::handle_geo_query(
    const v1::GeoQueryRequest& req) {
    // GEO_QUERY: geospatial proximity / containment query.
    // Requires authentication; validates collection field.
    // Full geo-index integration over the protobuf wire protocol is planned for
    // a future release.  Clients should use HTTP GET /api/v1/geo/query.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in GEO_QUERY request");
        return;
    }
    send_error(501,
        "Geospatial query execution is not yet integrated in the protobuf wire protocol. "
        "Use the HTTP REST API endpoint GET /api/v1/geo/query instead.");
}

void WireProtocolSession::handle_timeseries_query(
    const v1::TimeSeriesQueryRequest& req) {
    // TIMESERIES_QUERY: time-range aggregation query against TSStore.
    // Requires authentication; validates collection and time-range fields.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in TIMESERIES_QUERY request");
        return;
    }
    if (req.start_time_ns() >= req.end_time_ns()) {
        send_error(400,
            "Invalid time range in TIMESERIES_QUERY: "
            "start time must be less than end time");
        return;
    }
    // TSStore dispatch requires a TSStore reference that is not yet injected
    // into this protobuf wire session.
    send_error(503,
        "Time-series storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "GET /api/v1/timeseries/" + sanitizeForMessage(req.collection()));
}

void WireProtocolSession::handle_bpmn_start(
    const v1::BpmnStartProcessRequest& req) {
    // BPMN_START_PROCESS: start a BPMN process instance.
    // Requires authentication; validates process definition key.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.process_definition_key().empty()) {
        send_error(400,
            "Missing 'process_definition_key' in BPMN_START_PROCESS request");
        return;
    }
    // ProcessGraphManager dispatch requires a reference not yet injected into
    // this protobuf wire session.
    send_error(503,
        "Process graph manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/bpmn/process/" +
        sanitizeForMessage(req.process_definition_key()) + "/start");
}

void WireProtocolSession::handle_batch_get(const v1::BatchGetRequest& req) {
    // BATCH_GET: retrieve multiple documents by collection and UUID list.
    // Requires authentication; validates collection and non-empty UUID list.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in BATCH_GET request");
        return;
    }
    if (req.uuids_size() == 0) {
        send_error(400, "Empty 'uuids' list in BATCH_GET request");
        return;
    }
    send_error(503,
        "Storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/collection/" + sanitizeForMessage(req.collection()) + "/batch-get");
}

void WireProtocolSession::handle_batch_put(const v1::BatchPutRequest& req) {
    // BATCH_PUT: store multiple documents by collection.
    // Requires authentication; validates collection and non-empty items list.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.collection().empty()) {
        send_error(400, "Missing 'collection' in BATCH_PUT request");
        return;
    }
    if (req.items_size() == 0) {
        send_error(400, "Empty 'items' list in BATCH_PUT request");
        return;
    }
    send_error(503,
        "Storage not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/collection/" + sanitizeForMessage(req.collection()) + "/batch-put");
}

void WireProtocolSession::handle_transaction_begin(
    const v1::TransactionBeginRequest& req) {
    // TRANSACTION_BEGIN: begin a new transaction.
    // Requires authentication; validates isolation_level field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    // Transaction manager requires a reference not yet injected into this
    // protobuf wire session.
    send_error(503,
        "Transaction manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/transaction/begin (isolation: " +
        sanitizeForMessage(req.isolation_level()) + ")");
}

void WireProtocolSession::handle_transaction_commit(
    const v1::TransactionCommitRequest& req) {
    // TRANSACTION_COMMIT: commit an open transaction.
    // Requires authentication; validates transaction_id field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.transaction_id().empty()) {
        send_error(400, "Missing 'transaction_id' in TRANSACTION_COMMIT request");
        return;
    }
    send_error(503,
        "Transaction manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/transaction/" +
        sanitizeForMessage(req.transaction_id()) + "/commit");
}

void WireProtocolSession::handle_transaction_abort(
    const v1::TransactionAbortRequest& req) {
    // TRANSACTION_ABORT: abort/roll back an open transaction.
    // Requires authentication; validates transaction_id field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.transaction_id().empty()) {
        send_error(400, "Missing 'transaction_id' in TRANSACTION_ABORT request");
        return;
    }
    send_error(503,
        "Transaction manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/transaction/" +
        sanitizeForMessage(req.transaction_id()) + "/abort");
}

void WireProtocolSession::handle_graph_traverse() {
    // GRAPH_TRAVERSE: traverse graph edges from a start vertex.
    // Requires authentication.
    // Full graph traversal integration over the protobuf wire protocol is planned
    // for a future release.  Clients should use HTTP POST /api/v1/graph/traverse.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    send_error(501,
        "Graph traversal is not yet integrated in the protobuf wire protocol. "
        "Use the HTTP REST API endpoint POST /api/v1/graph/traverse instead.");
}

void WireProtocolSession::handle_bpmn_task_complete(
    const v1::BpmnTaskCompleteRequest& req) {
    // BPMN_TASK_COMPLETE: complete a user task in a process instance.
    // Requires authentication; validates task_id field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.task_id().empty()) {
        send_error(400,
            "Missing 'task_id' in BPMN_TASK_COMPLETE request");
        return;
    }
    send_error(503,
        "Process graph manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "POST /api/v1/bpmn/task/" +
        sanitizeForMessage(req.task_id()) + "/complete");
}

void WireProtocolSession::handle_bpmn_query_instance(
    const v1::BpmnQueryInstanceRequest& req) {
    // BPMN_QUERY_INSTANCE: query a running or completed process instance.
    // Requires authentication; validates process_instance_id field.
    if (!authenticated_) {
        send_error(0x0401, "Authentication required");
        return;
    }
    if (req.process_instance_id().empty()) {
        send_error(400,
            "Missing 'process_instance_id' in BPMN_QUERY_INSTANCE request");
        return;
    }
    send_error(503,
        "Process graph manager not connected to protobuf wire session. "
        "Use the JSON wire protocol port (8766) or HTTP REST API "
        "GET /api/v1/bpmn/instance/" +
        sanitizeForMessage(req.process_instance_id()));
}

void WireProtocolSession::handle_ping(const v1::PingRequest& /*req*/) {
    send_ok("PONG");
}

void WireProtocolSession::handle_close(const v1::CloseRequest& /*req*/) {
    close("client requested close");
}

#endif  // THEMIS_WIRE_V1_PROTO_AVAILABLE && THEMIS_WIRE_V1_PB_HEADER_FOUND

// ============================================================================
// WireProtocolServer
// ============================================================================

WireProtocolServer::WireProtocolServer(boost::asio::io_context& io_context,
                                       uint16_t                 port)
    : io_context_(io_context)
    , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    , port_(port)
    , total_connections_(0)
    , total_messages_(0)
    , running_(false)
{}

WireProtocolServer::~WireProtocolServer() {
    stop();
}

void WireProtocolServer::start() {
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (running_) return;
        running_ = true;
    }
    async_accept();
}

void WireProtocolServer::stop() {
    std::vector<std::shared_ptr<WireProtocolSession>> sessions_to_close;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (!running_) return;
        running_ = false;
        for (const auto& kv : sessions_)
            sessions_to_close.push_back(kv.second);
        sessions_.clear();
    }

    error_code ec;
    acceptor_.close(ec);
    for (const auto& session : sessions_to_close)
        session->close("server shutdown");
}

size_t WireProtocolServer::active_sessions() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return sessions_.size();
}

uint64_t WireProtocolServer::total_connections() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return total_connections_;
}

uint64_t WireProtocolServer::total_messages() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return total_messages_;
}

void WireProtocolServer::async_accept() {
    acceptor_.async_accept(
        [this](const error_code& ec, tcp::socket socket) {
            auto session = std::make_shared<WireProtocolSession>(
                std::move(socket));
            handle_accept(session, ec);
        });
}

void WireProtocolServer::handle_accept(
    std::shared_ptr<WireProtocolSession> session,
    const boost::system::error_code&     error) {
    bool accepted_session = false;
    if (!error) {
        session->set_disconnect_callback(
            [this](const std::string& session_id) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                sessions_.erase(session_id);
            });
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            if (running_) {
                sessions_[session->session_id()] = session;
                ++total_connections_;
                accepted_session = true;
            }
        }
        if (accepted_session) {
            session->start();
        } else {
            session->close("server stopping");
        }
    }

    bool should_continue = false;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        should_continue = running_;
    }
    if (should_continue) async_accept();
}

} // namespace wire
} // namespace themis

