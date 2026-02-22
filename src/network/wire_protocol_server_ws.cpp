/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_server_ws.cpp                        ║
  Module:          network                                            ║
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

using json = nlohmann::json;

namespace themis {
namespace network {

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------

WireProtocolWebSocketSession::WireProtocolWebSocketSession(
    tcp::socket socket,
    WireProtocolServer* server)
    : ws_(std::move(socket))
    , server_(server)
    , session_id_(server->session_id_counter_.fetch_add(1, std::memory_order_acq_rel))
{
    // Best-effort: retrieve remote IP before the socket is wrapped in Beast
    try {
        client_ip_ = ws_.next_layer().socket().remote_endpoint().address().to_string();
    } catch (...) {
        client_ip_ = "unknown";
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

        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        server_->active_ws_sessions_.erase(session_id_);
        return;
    }

    if (ec) {
        THEMIS_ERROR("[WireWS] session {} read error: {}", session_id_, ec.message());
        active_.store(false, std::memory_order_release);

        std::lock_guard<std::mutex> lock(server_->connections_mutex_);
        server_->active_ws_sessions_.erase(session_id_);
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

void WireProtocolWebSocketSession::processBinaryFrame(
    const std::vector<uint8_t>& /*data*/)
{
    // TODO: Implement binary wire-protocol frame dispatch over WebSocket.
    // Binary frames should carry raw ThemisDB wire-protocol frames (same
    // format as the native TCP path) and responses should be binary frames
    // in return.  Track this in the follow-up roadmap item for binary frame
    // support (ref: src/network/ROADMAP.md Phase 2).
    //
    // For now, return a structured JSON error so clients receive a clear
    // signal rather than a silent drop.
    json resp;
    resp["type"]    = "error";
    resp["status"]  = "unsupported";
    resp["message"] = "Binary wire-protocol frames over WebSocket are not yet "
                      "supported. Use text/JSON frames or the native TCP port.";
    send(resp.dump());
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

    std::lock_guard<std::mutex> lock(server_->connections_mutex_);
    server_->active_ws_sessions_.erase(session_id_);
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
