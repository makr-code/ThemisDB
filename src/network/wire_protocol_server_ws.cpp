/**
 * @file wire_protocol_server_ws.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=13, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// WebSocket upgrade session for ThemisDB wire protocol port (8766).
// Implements the run-loop for clients that connect via HTTP Upgrade: websocket
// on the same port as the native binary wire protocol.

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "network/wire_protocol_websocket.h"
#include <stdexcept>
#include "network/wire_protocol_server.h"
#include "network/wire_protocol_helpers.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cstring>

using json = nlohmann::json;

namespace themis {
namespace network {

namespace {

// Wire-frame constants
static constexpr size_t   kWireHeaderSize       = 12;
static constexpr uint8_t  kWireMagic[4]         = {0x54, 0x4D, 0x44, 0x42};  // "TMDB"
static constexpr uint16_t kWireSkipChecksumFlag = 0x0004;
static constexpr uint32_t kWireMaxPayloadBytes  = 64 * 1024 * 1024;  // 64 MB

// CRC-32 (ISO-HDLC) – same polynomial as wire_protocol_server.cpp.
// Used to verify the integrity of incoming binary wire-protocol frames.
static const uint32_t kWsCrc32Table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD706B3,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

uint32_t wsCrc32Update(uint32_t crc, const uint8_t* data, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i)
        crc = kWsCrc32Table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

} // anonymous namespace

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
        std::vector<uint8_t> payload(data, data + static_cast<int>(buffer_.size()) );
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

    std::string req_id = {};
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
            if (!ok) {
              resp["message"] = "Put operation failed";
            }
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
            if (!ok) {
              resp["message"] = "Delete operation failed";
            }
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
            crc = (crc >> 1) ^ (0xEDB88320u & -(crc & 1));
    }
    return crc ^ 0xFFFFFFFFu;
}

} // anonymous namespace

// Response opcodes used on the WebSocket binary path.
namespace {
constexpr uint8_t kOpcodeErrorResponse  = 0x80u;
constexpr uint8_t kOpcodePongResponse   = 0xFDu;
constexpr uint8_t kOpcodeGetResponse    = 0x90u;
constexpr uint8_t kOpcodePutResponse    = 0x91u;
constexpr uint8_t kOpcodeDeleteResponse = 0x92u;
} // anonymous namespace

std::vector<uint8_t> WireProtocolWebSocketSession::buildBinaryResponseFrame(
    uint8_t resp_opcode, const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> frame = {};

    frame.reserve(kWireHeaderSize + static_cast<int>(payload.size()) );
    frame.insert(frame.end(), std::begin(kWireMagic), std::end(kWireMagic));
    frame.push_back(0x01u);
    frame.push_back(resp_opcode);
    frame.push_back(static_cast<uint8_t>((kWireSkipChecksumFlag >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(kWireSkipChecksumFlag & 0xFF));

    const uint32_t payload_size = static_cast<uint32_t>(payload.size());
    frame.push_back(static_cast<uint8_t>((payload_size >> 24) & 0xFF));
    frame.push_back(static_cast<uint8_t>((payload_size >> 16) & 0xFF));
    frame.push_back(static_cast<uint8_t>((payload_size >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(payload_size & 0xFF));
    frame.insert(frame.end(), payload.begin(), payload.end());
    return frame;
}

void WireProtocolWebSocketSession::sendBinaryError(uint32_t error_code,
                                                    const std::string& message)
{
    json err;
    err["error_code"] = error_code;
    err["error_message"] = message;
    const std::string payload_text = err.dump();
    const std::vector<uint8_t> payload(payload_text.begin(), payload_text.end());
    sendBinary(buildBinaryResponseFrame(kOpcodeErrorResponse, payload));
}

void WireProtocolWebSocketSession::handleBinaryPing()
{
    json resp;
    resp["pong"] = true;
    resp["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string payload_text = resp.dump();
    sendBinary(buildBinaryResponseFrame(kOpcodePongResponse,
                                        {payload_text.begin(), payload_text.end()}));
}

void WireProtocolWebSocketSession::handleBinaryGet(const uint8_t* payload_data,
                                                    uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0005u, "Storage not available");
        return;
    }

    try {
        const std::string payload_text(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_text);
        const std::string key = req.value("key", "");
        if (key.empty()) {
            sendBinaryError(0x0003u, "GET requires field 1 (key)");
            return;
        }

        const auto result = server_->storage_->get(key);
        ProtobufSerializer serializer = {};
        if (result.has_value()) {
            serializer.writeTag(1, 0);
            serializer.writeVarint(0);
            serializer.writeTag(2, 2);
            serializer.writeLengthDelimited(result.value());
        } else {
            serializer.writeTag(1, 0);
            serializer.writeVarint(1);
        }
        sendBinary(buildBinaryResponseFrame(kOpcodeGetResponse, serializer.data()));
    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in GET payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::handleBinaryPut(const uint8_t* payload_data,
                                                    uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0005u, "Storage not available");
        return;
    }

    try {
        const std::string payload_text(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_text);
        const std::string key = req.value("key", "");
        const std::string value = req.value("value", "");
        if (key.empty()) {
            sendBinaryError(0x0003u, "PUT requires field 1 (key)");
            return;
        }

        const bool ok = server_->storage_->put(key, value);
        ProtobufSerializer serializer;
        serializer.writeTag(1, 0);
        serializer.writeVarint(ok ? 0 : 2);
        if (!ok) {
            serializer.writeTag(2, 2);
            serializer.writeString("Put operation failed");
        }
        sendBinary(buildBinaryResponseFrame(kOpcodePutResponse, serializer.data()));
    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in PUT payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::handleBinaryDelete(const uint8_t* payload_data,
                                                       uint32_t payload_size)
{
    if (!server_->storage_) {
        sendBinaryError(0x0005u, "Storage not available");
        return;
    }

    try {
        const std::string payload_text(reinterpret_cast<const char*>(payload_data), payload_size);
        const auto req = json::parse(payload_text);
        const std::string key = req.value("key", "");
        if (key.empty()) {
            sendBinaryError(0x0003u, "DELETE requires field 1 (key)");
            return;
        }

        const bool ok = server_->storage_->del(key);
        ProtobufSerializer serializer;
        serializer.writeTag(1, 0);
        serializer.writeVarint(ok ? 0 : 1);
        sendBinary(buildBinaryResponseFrame(kOpcodeDeleteResponse, serializer.data()));
    } catch (const json::exception& e) {
        sendBinaryError(0x0009u, std::string("Invalid JSON in DELETE payload: ") + e.what());
    }
}

void WireProtocolWebSocketSession::processBinaryFrame(const std::vector<uint8_t>& data)
{
    if (static_cast<int>(data.size()) < kWireHeaderSize) {
        sendBinaryError(0x0008u, "Binary frame too short (minimum 12-byte header required)");
        return;
    }

    if (data[0] != kWireMagic[0] || data[1] != kWireMagic[1] ||
        data[2] != kWireMagic[2] || data[3] != kWireMagic[3]) {
        sendBinaryError(0x0009u, "Invalid magic bytes: expected TMDB");
        return;
    }

    const uint8_t opcode = data[5];
    const uint16_t flags = (static_cast<uint16_t>(data[6]) << 8) | data[7];
    const uint32_t payload_size =
        (static_cast<uint32_t>(data[8]) << 24) |
        (static_cast<uint32_t>(data[9]) << 16) |
        (static_cast<uint32_t>(data[10]) << 8) |
         static_cast<uint32_t>(data[11]);

    if (payload_size > kWireMaxPayloadBytes) {
        sendBinaryError(0x0001u, "Payload size exceeds maximum allowed");
        return;
    }

    const bool has_checksum = !(flags & kWireSkipChecksumFlag);
    const size_t expected_size = kWireHeaderSize + payload_size + (has_checksum ? 4 : 0);
    if (static_cast<int>(data.size()) < expected_size) {
        sendBinaryError(0x000Au, "Binary frame incomplete");
        return;
    }

    if (has_checksum) {
        const size_t crc_offset = kWireHeaderSize + payload_size;
        const uint32_t expected_crc =
            (static_cast<uint32_t>(data[crc_offset]) << 24) |
            (static_cast<uint32_t>(data[crc_offset + 1]) << 16) |
            (static_cast<uint32_t>(data[crc_offset + 2]) << 8) |
             static_cast<uint32_t>(data[crc_offset + 3]);

        uint32_t computed_crc = wsCrc32Update(0, data.data(), kWireHeaderSize);
        if (payload_size > 0) {
            computed_crc = wsCrc32Update(computed_crc,
                                         data.data() + kWireHeaderSize,
                                         payload_size);
        }
        if (computed_crc != expected_crc) {
            sendBinaryError(0x000Fu, "Checksum mismatch");
            return;
        }
    }

    const uint8_t* payload_ptr = data.data() + kWireHeaderSize;
    switch (opcode) {
        case 0xFEu:
            handleBinaryPing();
            break;
        case 0xFFu:
            close();
            break;
        case 0x10u:
            handleBinaryGet(payload_ptr, payload_size);
            break;
        case 0x11u:
            handleBinaryPut(payload_ptr, payload_size);
            break;
        case 0x12u:
            handleBinaryDelete(payload_ptr, payload_size);
            break;
        default:
            sendBinaryError(0x0002u, "Unsupported opcode");
            break;
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
    bool was_registered = false;
    {
        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        auto it = server_->connections_per_ip_.find(client_ip_);
        if (it != server_->connections_per_ip_.end() && it->second > 0) {
            it->second--;
            was_registered = true;
        }
        server_->active_ws_sessions_.erase(session_id_);
    }

    // Mirror the logic in WireProtocolServer::unregisterConnection: only
    // adjust the global atomic when the entry was actually registered.
    // This keeps active_connection_count_ accurate for the backpressure check.
    if (was_registered) {
        const uint32_t prev =
            server_->active_connection_count_.fetch_sub(1, std::memory_order_relaxed);
        if (server_->overloaded_.load(std::memory_order_relaxed) &&
            server_->config_.max_connections > 0 &&
            prev - 1 < server_->config_.max_connections) {
            server_->overloaded_.store(false, std::memory_order_relaxed);
            std::cerr << "[WireProtocol] Backpressure recovery: active connections dropped to "
                      << (prev - 1) << " (limit=" << server_->config_.max_connections
                      << "). Accepting new connections again.\n";
        }
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

