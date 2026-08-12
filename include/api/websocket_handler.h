/**
 * @file websocket_handler.h
 * @brief WebSocket transport adapter for ThemisDB GraphQL subscriptions.
 *
 * @details Provides WebSocket connection lifecycle management, frame parsing,
 * and event callback interfaces aligned with RFC 6455.
 *
 * Core components:
 *  - `WebSocketCloseCode`: RFC 6455 close code constants (1000-1015)
 *  - `WebSocketFrame`: Frame representation with type (Text, Binary, Ping, Pong, Close)
 *  - `IWebSocketFrameCallback`: Pure-virtual callback for frame reception and connection closure
 *  - `WebSocketSession`: Opaque handle to an active connection
 *  - `IWebSocketHandler`: Factory interface for protocol upgrade and session creation
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Adapter behavior remains fail-closed on invalid protocol input
 *  - WebSocket frames must be bounded by explicit resource controls
 *  - Connection lifecycle is tied to callback delivery — no frames after onClose()
 *  - Message queuing respects runtime bounds to prevent resource exhaustion
 *
 * ### Lifecycle
 * 1. HTTP Upgrade handshake received by `IWebSocketHandler::upgrade()`
 * 2. On success, creates `WebSocketSession` and invokes callback with frames
 * 3. Callback receives frames via `onFrame()`; may call `send()` or `close()`
 * 4. Connection closes (graceful or abnormal) → `onClose()` invoked once
 * 5. After `onClose()`, session handle is invalid; no further `send()` allowed
 *
 * ### Thread safety
 * - `send()` and `close()` are thread-safe after `upgrade()` returns
 * - Callbacks (`onFrame`, `onClose`) must be `noexcept`; exceptions trigger `std::terminate()`
 * - Session object is internally synchronized; safe for concurrent frame reception and transmission
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

class WebSocketSession;

// ---------------------------------------------------------------------------
// WebSocketCloseCode — strongly-typed close codes aligned with RFC 6455
// ---------------------------------------------------------------------------

/**
 * @brief RFC 6455 WebSocket close codes.
 *
 * See https://www.rfc-editor.org/rfc/rfc6455#section-7.4.1
 */
enum class WebSocketCloseCode : uint16_t {
    NormalClosure       = 1000, ///< Normal, intentional closure.
    GoingAway           = 1001, ///< Endpoint going away (server shutdown, browser navigate).
    ProtocolError       = 1002, ///< Protocol error; message is not well-formed.
    UnsupportedData     = 1003, ///< Received unsupported data type (e.g. binary when text expected).
    NoStatusReceived    = 1005, ///< Reserved; indicates no close code was received (internal use only).
    AbnormalClosure     = 1006, ///< Reserved; abnormal closure without close frame (internal use only).
    InvalidPayload      = 1007, ///< Message payload is not consistent with the message type.
    PolicyViolation     = 1008, ///< Message violates the endpoint's policy.
    MessageTooBig       = 1009, ///< Message too large to process.
    MissingExtension    = 1010, ///< Client expected extension negotiation that did not happen.
    InternalError       = 1011, ///< Server encountered an unexpected condition.
    ServiceRestart      = 1012, ///< Service is restarting; client may reconnect.
    TryAgainLater       = 1013, ///< Temporary service condition; client should try later.
    BadGateway          = 1014, ///< Server received an invalid response from an upstream.
    TlsHandshakeFailed  = 1015, ///< Reserved; TLS handshake failed (internal use only).
};

// ---------------------------------------------------------------------------
// WebSocketFrame — an inbound or outbound WebSocket frame
// ---------------------------------------------------------------------------

/**
 * @brief A single WebSocket message frame.
 */
struct WebSocketFrame {
    enum class Type { Text, Binary, Ping, Pong, Close };

    Type        type    = Type::Text;
    std::string payload;

    static WebSocketFrame text(std::string data) {
        return {Type::Text, std::move(data)};
    }

    static WebSocketFrame binary(std::string data) {
        return {Type::Binary, std::move(data)};
    }
};

// ---------------------------------------------------------------------------
// IWebSocketFrameCallback — pure-virtual callback for inbound frames
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for receiving inbound WebSocket frames.
 *
 * Implementations must be `noexcept` — any exception thrown from `onFrame()`
 * causes `std::terminate()`.  Use error return values or side-channel logging
 * instead.
 *
 * `onClose()` is called exactly once, after which no further `onFrame()` calls
 * will occur for this session.
 */
class IWebSocketFrameCallback {
public:
    virtual ~IWebSocketFrameCallback() = default;

    /**
     * @brief Called for every inbound frame on the session.
     *
     * @param session  The session that received the frame (non-owning reference).
     * @param frame    The received frame (text, binary, ping, pong).
     */
    virtual void onFrame(WebSocketSession& session, const WebSocketFrame& frame) noexcept = 0;

    /**
     * @brief Called once when the WebSocket connection is closed.
     *
     * @param session  The session being closed (non-owning reference; do not
     *                 call `send()` after this point).
     * @param code     The RFC 6455 close code.
     * @param reason   Optional UTF-8 close reason string.
     */
    virtual void onClose(WebSocketSession& session,
                         WebSocketCloseCode code,
                         std::string_view reason) noexcept = 0;
};

// ---------------------------------------------------------------------------
// WebSocketSession — opaque handle to an active WebSocket connection
// ---------------------------------------------------------------------------

/**
 * @brief Opaque handle representing an active WebSocket connection.
 *
 * Obtained from `IWebSocketHandler::upgrade()` on a successful handshake.
 * `send()` is safe to call from any thread after `upgrade()` returns; it is
 * **not** safe to call after `onClose()` is invoked.
 *
 * `close()` initiates a graceful close by sending a close frame; the actual
 * connection teardown is asynchronous and results in `onClose()` being called.
 *
 * This class is a pure-virtual interface so that implementations can use any
 * underlying I/O framework (Boost.Beast, uWebSockets, etc.).
 */
class WebSocketSession {
public:
    virtual ~WebSocketSession() = default;

    WebSocketSession(const WebSocketSession&) = delete;
    WebSocketSession& operator=(const WebSocketSession&) = delete;

    /**
     * @brief Send a frame to the remote peer.
     *
     * Thread-safe; may be called concurrently from multiple threads.
     * Returns `false` if the session is already closed or the write queue
     * exceeds the configured maximum depth.
     *
     * @param frame  Frame to send.
     * @return `true` on success, `false` if the send was rejected.
     */
    [[nodiscard]] virtual bool send(WebSocketFrame frame) = 0;

    /**
     * @brief Initiate a graceful close.
     *
     * Sends a close frame and schedules teardown.  `onClose()` will be called
     * by the I/O thread once the close handshake completes.
     *
     * @param code    RFC 6455 close code.
     * @param reason  Optional UTF-8 close reason (max 123 bytes per RFC 6455).
     */
    virtual void close(WebSocketCloseCode code,
                       std::string_view reason = {}) noexcept = 0;

    /// Return the remote peer's IP address as a string.
    [[nodiscard]] virtual std::string_view remoteAddress() const noexcept = 0;

    /// Return the unique session ID (matches the correlation ID if available).
    [[nodiscard]] virtual std::string_view sessionId() const noexcept = 0;

    /// Return `true` if the session is still open.
    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

protected:
    WebSocketSession() = default;
};

// ---------------------------------------------------------------------------
// IWebSocketHandler — pure-virtual interface for WebSocket upgrade handlers
// ---------------------------------------------------------------------------

/**
 * @brief Pure-virtual interface for HTTP → WebSocket upgrade handlers.
 *
 * Called by the HTTP server when an `Upgrade: websocket` request arrives.
 * `upgrade()` must return immediately (non-blocking); it hands off the
 * connection to the I/O framework and delivers inbound frames asynchronously
 * via the supplied `IWebSocketFrameCallback`.
 *
 * ### Contract
 * - `upgrade()` validates the incoming HTTP request (origin, auth, sub-protocol)
 *   and returns a `Result::error` for rejected upgrades.  The server sends an
 *   HTTP 400/401/403 response in that case; the protocol switch never happens.
 * - On success, `upgrade()` returns a non-null `WebSocketSession` handle.  The
 *   caller must keep the `IWebSocketFrameCallback` alive for the lifetime of
 *   the session.
 * - `close()` on the returned session is the only way to terminate the session
 *   from the server side; deleting the `WebSocketSession` pointer while the
 *   connection is open is undefined behaviour.
 *
 * ### Thread safety
 * `upgrade()` may be called concurrently from the HTTP server's thread pool.
 */
class IWebSocketHandler {
public:
    virtual ~IWebSocketHandler() = default;

    /**
     * @brief Validate and perform the WebSocket upgrade handshake.
     *
     * @param method         HTTP method of the upgrade request (must be "GET").
     * @param path           Request path, e.g. "/v2/changes".
     * @param headers        HTTP headers of the upgrade request.
     * @param callback       Callback interface that will receive inbound frames.
     *                       Must remain valid until `onClose()` is called.
     * @return `Result<WebSocketSession*>` — a non-owning pointer to the session
     *         (lifetime managed by the framework) on success, or an error on
     *         rejection (the error message is sent as the HTTP response body).
     */
    [[nodiscard]] virtual themis::Result<WebSocketSession*> upgrade(
        std::string_view method,
        std::string_view path,
        const std::unordered_map<std::string, std::string>& headers,
        IWebSocketFrameCallback& callback) = 0;

    /// Human-readable handler name used in logs and metrics.
    [[nodiscard]] virtual std::string_view handlerName() const noexcept = 0;
};

} // namespace api
} // namespace themis
