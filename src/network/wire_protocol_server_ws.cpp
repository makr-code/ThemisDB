/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server_ws.cpp                        ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-23 03:58:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     411                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d6fb9967  2026-02-22  fix(network): audit fixes – connection-count correctness ... ║
    • 6d2d48159  2026-02-22  feat(network): implement WebSocket upgrade support on wir... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// WebSocket upgrade session for ThemisDB wire protocol port (8766).
// Implements the run-loop for clients that connect via HTTP Upgrade: websocket
// on the same port as the native binary wire protocol.

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "network/wire_protocol_websocket.h"
#include "network/wire_protocol_server.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cstring>

using json = nlohmann::json;

namespace themis {
namespace network {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

WireProtocolWebSocketSession::WireProtocolWebSocketSession(
    tcp::socket socket,
    WireProtocolServer* server,
    const std::string& client_ip)
    : ws_(std::move(socket))
    , server_(server)
    , session_id_(server->session_id_counter_.fetch_add(1, std::memory_order_acq_rel))
{
    // Use the IP passed from the binary session if available; otherwise extract
    // from the (moved) socket.  The passed IP is preferred because it ensures the
    // same string is used for unregisterConnection() as was used for
    // registerConnection() in handleAccept().
    if (!client_ip.empty()) {
        client_ip_ = client_ip;
    } else {
        try {
            client_ip_ = ws_.next_layer().socket().remote_endpoint().address().to_string();
        } catch (...) {
            client_ip_ = "unknown";
        }
    }

    // Tune Beast timeouts for a server-side stream
    beast::get_lowest_layer(ws_).expires_never();
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res) {
            res.set(http::field::server, "ThemisDB-WireProtocol/1.0");
        }
    ));

    THEMIS_INFO("[WireWS] session {} created from {}", session_id_, client_ip_);
}

WireProtocolWebSocketSession::~WireProtocolWebSocketSession() {
    THEMIS_INFO("[WireWS] session {} destroyed", session_id_);
}

// ---------------------------------------------------------------------------
// run – complete WebSocket handshake then enter read loop
// ---------------------------------------------------------------------------

void WireProtocolWebSocketSession::run(http::request<http::string_body> req) {
    ws_.async_accept(
        req,
        beast::bind_front_handler(
            &WireProtocolWebSocketSession::onAccept,
            shared_from_this()));
}

void WireProtocolWebSocketSession::onAccept(beast::error_code ec) {
    if (ec) {
        THEMIS_ERROR("[WireWS] session {} accept error: {}", session_id_, ec.message());
        return;
    }

    active_.store(true, std::memory_order_release);
    THEMIS_INFO("[WireWS] session {} WebSocket handshake complete ({})",
                session_id_, client_ip_);

    // Track this session on the server
    {
        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        server_->active_ws_sessions_[session_id_] = shared_from_this();
    }

    // Send welcome frame
    json welcome;
    welcome["type"]       = "welcome";
    welcome["session_id"] = session_id_;
    welcome["server"]     = "ThemisDB";
    welcome["protocol"]   = "websocket-wire/1.0";
    welcome["timestamp"]  = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    send(welcome.dump());

    doRead();
}

// ---------------------------------------------------------------------------
// Read loop
// ---------------------------------------------------------------------------

void WireProtocolWebSocketSession::doRead() {
    buffer_.consume(buffer_.size());
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WireProtocolWebSocketSession::onRead,
            shared_from_this()));
}

void WireProtocolWebSocketSession::onRead(beast::error_code ec,
                                           std::size_t /*bytes_transferred*/)
{
    if (ec == websocket::error::closed) {
        THEMIS_INFO("[WireWS] session {} closed by client", session_id_);
        active_.store(false, std::memory_order_release);

        // Decrement per-IP count and remove session under the same lock so the
        // two changes are atomic from the perspective of checkConnectionLimit.
        {
            std::lock_guard<std::mutex> lock(server_->connections_mutex_);
            auto it = server_->connections_per_ip_.find(client_ip_);
            if (it != server_->connections_per_ip_.end() && it->second > 0)
                it->second--;
            server_->active_ws_sessions_.erase(session_id_);
        }
        return;
    }

    if (ec) {
        THEMIS_ERROR("[WireWS] session {} read error: {}", session_id_, ec.message());
        active_.store(false, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(server_->connections_mutex_);
            auto it = server_->connections_per_ip_.find(client_ip_);
            if (it != server_->connections_per_ip_.end() && it->second > 0)
                it->second--;
            server_->active_ws_sessions_.erase(session_id_);
        }
        return;
    }

    if (ws_.got_binary()) {
        // Binary frame: raw wire-protocol bytes
        const auto* data = static_cast<const uint8_t*>(buffer_.data().data());
        std::vector<uint8_t> payload(data, data + buffer_.size());
        processBinaryFrame(payload);
    } else {
        // Text frame: JSON message
        processTextMessage(beast::buffers_to_string(buffer_.data()));
    }

    doRead();
}

// ---------------------------------------------------------------------------
// Message processing – text (JSON)
// ---------------------------------------------------------------------------

void WireProtocolWebSocketSession::processTextMessage(const std::string& text) {
    THEMIS_DEBUG("[WireWS] session {} text message: {}", session_id_, text);

    std::string req_id;
    try {
        auto msg = json::parse(text);
        req_id           = msg.value("id",   "");
        std::string type = msg.value("type", "unknown");

        if (type == "ping") {
            json resp;
            resp["id"]        = req_id;
            resp["type"]      = "pong";
            resp["status"]    = "ok";
            resp["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            send(resp.dump());

        } else if (type == "get") {
            auto payload = msg.value("payload", json::object());
            std::string key = payload.value("key", "");
            if (key.empty()) {
                send(makeError(req_id, "get_response", "Missing 'key' in payload"));
                return;
            }
            if (!server_->storage_) {
                send(makeError(req_id, "get_response", "Storage not available"));
                return;
            }
            auto result = server_->storage_->get(key);
            json resp;
            resp["id"]   = req_id;
            resp["type"] = "get_response";
            if (result.has_value()) {
                // Convert raw bytes to string
                const auto& bytes = result.value();
                resp["status"]  = "ok";
                resp["payload"] = std::string(bytes.begin(), bytes.end());
            } else {
                resp["status"]  = "not_found";
                resp["message"] = "Key not found";
            }
            send(resp.dump());

        } else if (type == "put") {
            auto payload = msg.value("payload", json::object());
            std::string key   = payload.value("key",   "");
            std::string value = payload.value("value", "");
            if (key.empty()) {
                send(makeError(req_id, "put_response", "Missing 'key' in payload"));
                return;
            }
            if (!server_->storage_) {
                send(makeError(req_id, "put_response", "Storage not available"));
                return;
            }
            bool ok = server_->storage_->put(key, value);
            json resp;
            resp["id"]     = req_id;
            resp["type"]   = "put_response";
            resp["status"] = ok ? "ok" : "error";
            if (!ok) resp["message"] = "Put operation failed";
            send(resp.dump());

        } else if (type == "delete") {
            auto payload = msg.value("payload", json::object());
            std::string key = payload.value("key", "");
            if (key.empty()) {
                send(makeError(req_id, "delete_response", "Missing 'key' in payload"));
                return;
            }
            if (!server_->storage_) {
                send(makeError(req_id, "delete_response", "Storage not available"));
                return;
            }
            bool ok = server_->storage_->del(key);
            json resp;
            resp["id"]     = req_id;
            resp["type"]   = "delete_response";
            resp["status"] = ok ? "ok" : "error";
            if (!ok) resp["message"] = "Delete operation failed";
            send(resp.dump());

        } else if (type == "query") {
            // AQL query forwarding – server delivers error indicating
            // full AQL execution is handled by the HTTP/gRPC layer.
            // The wire-protocol WebSocket path returns a structured error
            // so clients can fall back gracefully.
            send(makeError(req_id, "query_response",
                           "AQL query execution requires the HTTP API endpoint. "
                           "Use POST /query/aql or connect via the HTTP server."));

        } else {
            THEMIS_WARN("[WireWS] session {} unknown message type: {}", session_id_, type);
            send(makeError(req_id, "error", "Unknown message type: " + type));
        }

    } catch (const json::exception& e) {
        THEMIS_ERROR("[WireWS] session {} JSON parse error: {}", session_id_, e.what());
        send(makeError(req_id, "error", std::string("Invalid JSON: ") + e.what()));
    }
}

// ---------------------------------------------------------------------------
// Message processing – binary (raw wire-protocol frame)
// ---------------------------------------------------------------------------

namespace {

// CRC32 (ISO-HDLC / Ethernet) – same polynomial as wire_protocol_server.cpp.
// Used to verify the optional per-frame checksum carried by binary WebSocket frames.
static uint32_t crc32Binary(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int b = 0; b < 8; ++b)
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1u));
    }
    return crc ^ 0xFFFFFFFFu;
}

} // anonymous namespace

// Response OpCodes used on the WebSocket binary path.
// These complement the request opcodes defined in wire_protocol_server.h.
namespace {
    constexpr uint8_t kOpcodeErrorResponse  = 0x00u; ///< Error / NACK
    constexpr uint8_t kOpcodePong           = 0xFDu; ///< Response to PING (0xFE)
    constexpr uint8_t kOpcodeGetResponse    = 0x13u; ///< Response to GET (0x10)
    constexpr uint8_t kOpcodePutResponse    = 0x14u; ///< Response to PUT (0x11)
    constexpr uint8_t kOpcodeDeleteResponse = 0x15u; ///< Response to DELETE (0x12)

    // Flags field value used in all server-originated response frames.
    // Setting SKIP_CHECKSUM (bit 2) means the receiver does not need to
    // read an appended 4-byte CRC32 after the payload.
    constexpr uint16_t kResponseFlags = 0x0004u;

    // Binary frame header constants
    constexpr size_t   kFrameHeaderSize    = 12u;
    constexpr uint8_t  kWireMagic[4]       = {0x54u, 0x4Du, 0x44u, 0x42u}; // "TMDB"
    constexpr uint16_t kSkipChecksumFlag   = 0x0004u;
    constexpr uint32_t kMaxBinaryPayload   = 64u * 1024u * 1024u; // 64 MiB
} // anonymous namespace

std::vector<uint8_t> WireProtocolWebSocketSession::buildResponseFrame(
    uint8_t opcode, const std::vector<uint8_t>& payload) const
{
    // Wire frame layout: Magic(4) + Version(1) + OpCode(1) + Flags(2) + PayloadSize(4) + Payload
    std::vector<uint8_t> frame;
    frame.reserve(kFrameHeaderSize + payload.size());

    // Magic bytes "TMDB"
    frame.push_back(kWireMagic[0]);
    frame.push_back(kWireMagic[1]);
    frame.push_back(kWireMagic[2]);
    frame.push_back(kWireMagic[3]);

    // Version
    frame.push_back(0x01u);

    // OpCode
    frame.push_back(opcode);

    // Flags (big-endian) – SKIP_CHECKSUM so receivers don't expect a trailing CRC32
    frame.push_back(static_cast<uint8_t>((kResponseFlags >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>( kResponseFlags       & 0xFF));

    // PayloadSize (big-endian)
    uint32_t ps = static_cast<uint32_t>(payload.size());
    frame.push_back(static_cast<uint8_t>((ps >> 24) & 0xFF));
    frame.push_back(static_cast<uint8_t>((ps >> 16) & 0xFF));
    frame.push_back(static_cast<uint8_t>((ps >>  8) & 0xFF));
    frame.push_back(static_cast<uint8_t>( ps         & 0xFF));

    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

void WireProtocolWebSocketSession::sendBinaryError(uint32_t error_code,
                                                    const std::string& message)
{
    json err;
    err["error_code"]    = error_code;
    err["error_message"] = message;
    const std::string s  = err.dump();
    const std::vector<uint8_t> payload(s.begin(), s.end());
    sendBinary(buildResponseFrame(kOpcodeErrorResponse, payload));
}

void WireProtocolWebSocketSession::handleBinaryPing() {
    json resp;
    resp["pong"]      = true;
    resp["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string s = resp.dump();
    sendBinary(buildResponseFrame(kOpcodePong, {s.begin(), s.end()}));
}

void WireProtocolWebSocketSession::handleBinaryGet(const uint8_t* payload_data,
                                                    uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0004u, "Storage not available");
        return;
    }
    try {
        const std::string payload_str(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_str);
        const std::string key = req.value("key", "");
        if (key.empty()) {
            sendBinaryError(0x000Au, "Missing 'key' in GET payload");
            return;
        }

        const auto result = server_->storage_->get(key);
        json resp;
        if (result.has_value()) {
            const auto& bytes = result.value();
            resp["status"]  = "ok";
            resp["key"]     = key;
            resp["value"]   = std::string(bytes.begin(), bytes.end());
        } else {
            resp["status"]  = "not_found";
            resp["key"]     = key;
            resp["message"] = "Key not found";
        }
        const std::string s = resp.dump();
        sendBinary(buildResponseFrame(kOpcodeGetResponse, {s.begin(), s.end()}));

    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in GET payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::handleBinaryPut(const uint8_t* payload_data,
                                                    uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0004u, "Storage not available");
        return;
    }
    try {
        const std::string payload_str(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_str);
        const std::string key   = req.value("key",   "");
        const std::string value = req.value("value", "");
        if (key.empty()) {
            sendBinaryError(0x000Au, "Missing 'key' in PUT payload");
            return;
        }

        const bool ok = server_->storage_->put(key, value);
        json resp;
        resp["status"] = ok ? "ok" : "error";
        resp["key"]    = key;
        if (!ok) resp["message"] = "PUT operation failed";
        const std::string s = resp.dump();
        sendBinary(buildResponseFrame(kOpcodePutResponse, {s.begin(), s.end()}));

    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in PUT payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::handleBinaryDelete(const uint8_t* payload_data,
                                                       uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0004u, "Storage not available");
        return;
    }
    try {
        const std::string payload_str(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_str);
        const std::string key = req.value("key", "");
        if (key.empty()) {
            sendBinaryError(0x000Au, "Missing 'key' in DELETE payload");
            return;
        }

        const bool ok = server_->storage_->del(key);
        json resp;
        resp["status"] = ok ? "ok" : "error";
        resp["key"]    = key;
        if (!ok) resp["message"] = "DELETE operation failed";
        const std::string s = resp.dump();
        sendBinary(buildResponseFrame(kOpcodeDeleteResponse, {s.begin(), s.end()}));

    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in DELETE payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::processBinaryFrame(
    const std::vector<uint8_t>& data)
{
    // Binary WebSocket frames carry a raw ThemisDB wire-protocol frame.
    //
    // Frame layout (V1):
    //   Offset  Size  Description
    //   ------  ----  -----------
    //     0       4   Magic ("TMDB" = 0x54 0x4D 0x44 0x42)
    //     4       1   Version (currently 0x01)
    //     5       1   OpCode
    //     6       2   Flags (big-endian); bit 2 = SKIP_CHECKSUM
    //     8       4   PayloadSize (big-endian, in bytes)
    //    12    [size]  Payload bytes
    //   12+n     4   Optional CRC32 (big-endian) if SKIP_CHECKSUM flag is NOT set
    //
    // Responses are sent back as binary WebSocket frames using the same wire
    // frame layout with the SKIP_CHECKSUM flag set.

    if (data.size() < kFrameHeaderSize) {
        THEMIS_WARN("[WireWS] session {} binary frame too short ({} bytes)",
                    session_id_, data.size());
        sendBinaryError(0x0008u, "Binary frame too short (minimum 12-byte header required)");
        return;
    }

    // Validate magic bytes
    if (data[0] != kWireMagic[0] || data[1] != kWireMagic[1] ||
        data[2] != kWireMagic[2] || data[3] != kWireMagic[3]) {
        THEMIS_WARN("[WireWS] session {} invalid magic bytes 0x{:02X}{:02X}{:02X}{:02X}",
                    session_id_, data[0], data[1], data[2], data[3]);
        sendBinaryError(0x0006u, "Invalid magic bytes – expected 'TMDB'");
        return;
    }

    // Parse header fields (version is reserved for future use)
    const uint8_t opcode = data[5];
    uint16_t flags = (static_cast<uint16_t>(data[6]) << 8) | data[7];
    uint32_t payload_size = (static_cast<uint32_t>(data[8])  << 24)
                          | (static_cast<uint32_t>(data[9])  << 16)
                          | (static_cast<uint32_t>(data[10]) <<  8)
                          |  static_cast<uint32_t>(data[11]);

    if (payload_size > kMaxBinaryPayload) {
        THEMIS_WARN("[WireWS] session {} binary frame payload too large ({} bytes)",
                    session_id_, payload_size);
        sendBinaryError(0x0001u, "Payload size exceeds maximum allowed");
        return;
    }

    const bool has_checksum    = !(flags & kSkipChecksumFlag);
    const size_t expected_size = kFrameHeaderSize + payload_size + (has_checksum ? 4u : 0u);

    if (data.size() < expected_size) {
        THEMIS_WARN("[WireWS] session {} binary frame incomplete "
                    "(expected {} bytes, got {})", session_id_, expected_size, data.size());
        sendBinaryError(0x0008u, "Binary frame incomplete");
        return;
    }

    // Optional CRC32 verification
    if (has_checksum) {
        uint32_t expected_crc = (static_cast<uint32_t>(data[kFrameHeaderSize + payload_size + 0]) << 24)
                              | (static_cast<uint32_t>(data[kFrameHeaderSize + payload_size + 1]) << 16)
                              | (static_cast<uint32_t>(data[kFrameHeaderSize + payload_size + 2]) <<  8)
                              |  static_cast<uint32_t>(data[kFrameHeaderSize + payload_size + 3]);
        const uint32_t computed_crc = crc32Binary(data.data(), kFrameHeaderSize + payload_size);
        if (computed_crc != expected_crc) {
            THEMIS_WARN("[WireWS] session {} CRC32 mismatch "
                        "(expected={:#010x}, computed={:#010x})",
                        session_id_, expected_crc, computed_crc);
            sendBinaryError(0x000Fu, "Checksum mismatch");
            return;
        }
    }

    const uint8_t* payload_ptr = data.data() + kFrameHeaderSize;

    THEMIS_DEBUG("[WireWS] session {} binary frame opcode=0x{:02X} payload_size={}",
                 session_id_, opcode, payload_size);

    // Dispatch based on OpCode
    switch (opcode) {
        case 0xFEu: // PING
            handleBinaryPing();
            break;
        case 0xFFu: // CLOSE
            close();
            break;
        case 0x10u: // GET
            handleBinaryGet(payload_ptr, payload_size);
            break;
        case 0x11u: // PUT
            handleBinaryPut(payload_ptr, payload_size);
            break;
        case 0x12u: // DELETE
            handleBinaryDelete(payload_ptr, payload_size);
            break;
        default: {
            char hex_opcode[8];
            std::snprintf(hex_opcode, sizeof(hex_opcode), "0x%02X", opcode);
            THEMIS_WARN("[WireWS] session {} unknown binary opcode: {}",
                        session_id_, hex_opcode);
            sendBinaryError(0x0002u,
                            std::string("Unknown OpCode: ") + hex_opcode +
                            ". Supported binary opcodes: PING(0xFE), CLOSE(0xFF), "
                            "GET(0x10), PUT(0x11), DELETE(0x12)");
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Write helpers
// ---------------------------------------------------------------------------

void WireProtocolWebSocketSession::send(const std::string& message) {
    std::lock_guard<std::mutex> lock(write_mutex_);
    write_queue_.push_back({message, /*is_binary=*/false});
    if (!write_in_progress_) {
        write_in_progress_ = true;
        doWrite();
    }
}

void WireProtocolWebSocketSession::sendBinary(const std::vector<uint8_t>& data) {
    std::string binary_str(data.begin(), data.end());
    std::lock_guard<std::mutex> lock(write_mutex_);
    write_queue_.push_back({binary_str, /*is_binary=*/true});
    if (!write_in_progress_) {
        write_in_progress_ = true;
        doWrite();
    }
}

void WireProtocolWebSocketSession::doWrite() {
    // Called with write_mutex_ held or from onWrite callback
    if (write_queue_.empty()) {
        write_in_progress_ = false;
        return;
    }

    // Set correct frame type (text or binary) before writing
    const auto& front = write_queue_.front();
    ws_.text(!front.is_binary);
    ws_.async_write(
        net::buffer(front.data),
        beast::bind_front_handler(
            &WireProtocolWebSocketSession::onWrite,
            shared_from_this()));
}

void WireProtocolWebSocketSession::onWrite(beast::error_code ec,
                                            std::size_t /*bytes_transferred*/)
{
    std::lock_guard<std::mutex> lock(write_mutex_);

    if (ec) {
        THEMIS_ERROR("[WireWS] session {} write error: {}", session_id_, ec.message());
        active_.store(false, std::memory_order_release);
        write_in_progress_ = false;
        return;
    }

    write_queue_.pop_front();
    if (!write_queue_.empty()) {
        doWrite();
    } else {
        write_in_progress_ = false;
    }
}

// ---------------------------------------------------------------------------
// Close
// ---------------------------------------------------------------------------

void WireProtocolWebSocketSession::close() {
    if (!active_.exchange(false, std::memory_order_acq_rel)) {
        return;  // Already closed
    }

    beast::error_code ec;
    ws_.close(websocket::close_code::normal, ec);
    if (ec) {
        THEMIS_WARN("[WireWS] session {} close error: {}", session_id_, ec.message());
    } else {
        THEMIS_INFO("[WireWS] session {} closed", session_id_);
    }

    // Decrement the per-IP connection count and remove the session atomically
    // under connections_mutex_ (same lock used by checkConnectionLimit).
    {
        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        auto it = server_->connections_per_ip_.find(client_ip_);
        if (it != server_->connections_per_ip_.end() && it->second > 0)
            it->second--;
        server_->active_ws_sessions_.erase(session_id_);
    }
}

// ---------------------------------------------------------------------------
// Utility
// ---------------------------------------------------------------------------

std::string WireProtocolWebSocketSession::makeError(const std::string& id,
                                                     const std::string& type,
                                                     const std::string& message) const
{
    json resp;
    resp["id"]      = id;
    resp["type"]    = type;
    resp["status"]  = "error";
    resp["message"] = message;
    return resp.dump();
}

} // namespace network
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
