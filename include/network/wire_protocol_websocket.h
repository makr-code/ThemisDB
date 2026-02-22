/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_websocket.h                          ║
  Module:          network                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol – WebSocket upgrade session
// Handles WebSocket connections that arrive on the binary wire protocol port (8766).
// When a client sends an HTTP Upgrade: websocket request on port 8766 the server
// detects the HTTP prefix, performs the Beast WebSocket handshake, and then
// processes ThemisDB messages carried over WebSocket frames.
//
// Text frames  → JSON messages: {"id":"<req>","type":"ping|get|put|delete|query","payload":{...}}
// Binary frames→ raw ThemisDB wire-protocol frames (same format as TCP path)

#pragma once

#ifdef THEMIS_ENABLE_WEBSOCKET

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <deque>
#include <mutex>
#include <atomic>
#include <vector>

namespace themis {
namespace network {

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp = net::ip::tcp;

// Forward declaration – defined in wire_protocol_server.h
class WireProtocolServer;

/**
 * @brief WebSocket session on the wire protocol port (8766).
 *
 * Lifecycle:
 *  1. WireProtocolServer detects "GET " HTTP prefix on an incoming connection.
 *  2. It creates a WireProtocolWebSocketSession (moving the accepted socket in).
 *  3. It calls run(peeked, req) which completes the WebSocket handshake.
 *  4. The session enters a read loop, processing JSON (text) or binary frames.
 *  5. On close/error the session removes itself from the server tracking table.
 *
 * Message format (text/JSON):
 * @code
 * // Request
 * { "id": "req_1", "type": "ping" }
 * { "id": "req_2", "type": "get",    "payload": { "key": "users/alice" } }
 * { "id": "req_3", "type": "put",    "payload": { "key": "users/bob", "value": "..." } }
 * { "id": "req_4", "type": "delete", "payload": { "key": "users/carol" } }
 * { "id": "req_5", "type": "query",  "payload": { "aql": "FOR d IN users RETURN d" } }
 *
 * // Response
 * { "id": "req_1", "type": "pong", "status": "ok" }
 * { "id": "req_2", "type": "get_response",    "status": "ok",    "payload": {...} }
 * { "id": "req_3", "type": "put_response",    "status": "ok" }
 * { "id": "req_4", "type": "delete_response", "status": "ok" }
 * { "id": "req_5", "type": "query_response",  "status": "ok",    "payload": [...] }
 * @endcode
 *
 * Binary frames carry a raw ThemisDB wire-protocol frame and the response is
 * sent back as a binary frame using the same encoding used on the TCP path.
 */
class WireProtocolWebSocketSession
    : public std::enable_shared_from_this<WireProtocolWebSocketSession>
{
public:
    /**
     * @brief Construct a WebSocket session from an already-accepted TCP socket.
     *
     * @param socket  The accepted TCP socket (moved in).
     * @param server  Owning WireProtocolServer (not null).
     */
    WireProtocolWebSocketSession(tcp::socket socket, WireProtocolServer* server);
    ~WireProtocolWebSocketSession();

    /**
     * @brief Begin the WebSocket upgrade from the partially-read HTTP request.
     *
     * @param peeked  The first bytes already consumed from the TCP stream
     *                (used to reconstruct the HTTP request).
     * @param req     The fully parsed HTTP upgrade request.
     */
    void run(http::request<http::string_body> req);

    /**
     * @brief Send a text (JSON) message to the client.
     */
    void send(const std::string& message);

    /**
     * @brief Send a binary frame to the client.
     */
    void sendBinary(const std::vector<uint8_t>& data);

    /**
     * @brief Initiate a graceful close.
     */
    void close();

    /**
     * @brief Returns true while the session is open.
     */
    bool isActive() const { return active_.load(std::memory_order_acquire); }

    /**
     * @brief Remote IP address (best-effort; "unknown" on failure).
     */
    const std::string& getClientIP() const { return client_ip_; }

    /**
     * @brief Unique numeric session identifier.
     */
    uint64_t getSessionID() const { return session_id_; }

private:
    // WebSocket handshake callback
    void onAccept(beast::error_code ec);

    // Async read loop
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes_transferred);

    // Message dispatch
    void processTextMessage(const std::string& text);
    void processBinaryFrame(const std::vector<uint8_t>& data);

    // Write helpers
    void doWrite();
    void onWrite(beast::error_code ec, std::size_t bytes_transferred);

    // Build JSON error response
    std::string makeError(const std::string& id,
                          const std::string& type,
                          const std::string& message) const;

    // ---- members ----
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer                   buffer_;

    WireProtocolServer* server_;
    uint64_t            session_id_;
    std::string         client_ip_;

    std::atomic<bool>               active_{false};

    // Serialised write queue (prevents concurrent async_write calls).
    // Each entry records the message payload and whether it is a binary frame.
    struct WriteMessage {
        std::string data;
        bool        is_binary = false;
    };
    std::mutex                  write_mutex_;
    std::deque<WriteMessage>    write_queue_;
    bool                        write_in_progress_{false};
};

} // namespace network
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
