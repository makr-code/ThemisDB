/**
 * @file wire_protocol_websocket.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
     * @param socket     The accepted TCP socket (moved in).
     * @param server     Owning WireProtocolServer (not null).
     * @param client_ip  Remote IP string (passed from binary Session to maintain
     *                   connection-count accounting across the protocol upgrade).
     *                   If empty the constructor falls back to extracting the IP
     *                   from the socket (for standalone construction).
     */
    WireProtocolWebSocketSession(tcp::socket socket, WireProtocolServer* server,
                                  const std::string& client_ip = {});
    ~WireProtocolWebSocketSession();

    /**
     * @brief Begin the WebSocket upgrade from the fully-parsed HTTP request.
     *
     * @param req  The HTTP Upgrade request read by the binary Session's
     *             asyncUpgradeToWebSocket method.
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

    /**
     * @brief Build a binary response frame in ThemisDB wire format.
     *
     * Frame layout: Magic(4) + Version(1) + OpCode(1) + Flags(2) + PayloadSize(4) + Payload
     * Responses always carry SKIP_CHECKSUM_FLAG (0x0004) – WebSocket provides its own
     * frame integrity.
     *
     * @param resp_opcode  Response opcode byte (e.g. 0x90 for GET response).
     * @param payload      Response payload bytes (may be empty).
     * @return             Complete binary frame ready to send.
     */
    static std::vector<uint8_t> buildBinaryResponseFrame(uint8_t resp_opcode,
                                                          const std::vector<uint8_t>& payload);

private:
    // WebSocket handshake callback
    void onAccept(beast::error_code ec);

    // Async read loop
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes_transferred);

    // Message dispatch
    void processTextMessage(const std::string& text);
    void processBinaryFrame(const std::vector<uint8_t>& data);

    // Binary frame helpers – build a wire-protocol response frame and dispatch
    // to the appropriate storage operation, then reply as a binary WebSocket frame.
    std::vector<uint8_t> buildResponseFrame(uint8_t opcode,
                                            const std::vector<uint8_t>& payload) const;
    void sendBinaryError(uint32_t error_code, const std::string& message);
    void handleBinaryPing();
    void handleBinaryGet(const uint8_t* payload_data, uint32_t payload_size);
    void handleBinaryPut(const uint8_t* payload_data, uint32_t payload_size);
    void handleBinaryDelete(const uint8_t* payload_data, uint32_t payload_size);

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

