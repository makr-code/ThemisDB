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

struct WebSocketFrame {
    enum class Type { Text, Binary, Ping, Pong, Close };

    Type        type    = Type::Text;
    std::string payload;

    /**
     * @brief Text.
     * @param[in] data Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static WebSocketFrame text(std::string data) {
        return {Type::Text, std::move(data)};
    }

    /**
     * @brief Binary.
     * @param[in] data Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static WebSocketFrame binary(std::string data) {
        return {Type::Binary, std::move(data)};
    }
};

// ---------------------------------------------------------------------------
// IWebSocketFrameCallback — pure-virtual callback for inbound frames
// ---------------------------------------------------------------------------

class IWebSocketFrameCallback {
public:
    /**
     * @brief IWeb Socket Frame Callback.
     * @return Return value.
     */
    virtual ~IWebSocketFrameCallback() = default;

    /**
     * @brief On Frame.
     * @param[in,out] session Input/output parameter.
     * @param[in] frame Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void onFrame(WebSocketSession& session, const WebSocketFrame& frame) noexcept = 0;

    /**
     * @brief On Close.
     * @param[in,out] session Input/output parameter.
     * @param[in] code Input parameter.
     * @param[in] reason Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void onClose(WebSocketSession& session,
                         WebSocketCloseCode code,
                         std::string_view reason) noexcept = 0;
};

// ---------------------------------------------------------------------------
// WebSocketSession — opaque handle to an active WebSocket connection
// ---------------------------------------------------------------------------

class WebSocketSession {
public:
    /**
     * @brief Web Socket Session.
     * @return Return value.
     */
    virtual ~WebSocketSession() = default;

    WebSocketSession(const WebSocketSession&) = delete;
    WebSocketSession& operator=(const WebSocketSession&) = delete;

    [[nodiscard]] virtual bool send(WebSocketFrame frame) = 0;

    virtual void close(WebSocketCloseCode code,
                       std::string_view reason = {}) noexcept = 0;

    [[nodiscard]] virtual std::string_view remoteAddress() const noexcept = 0;

    [[nodiscard]] virtual std::string_view sessionId() const noexcept = 0;

    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

protected:
    WebSocketSession() = default;
};

// ---------------------------------------------------------------------------
// IWebSocketHandler — pure-virtual interface for WebSocket upgrade handlers
// ---------------------------------------------------------------------------

class IWebSocketHandler {
public:
    /**
     * @brief IWeb Socket Handler.
     * @return Return value.
     */
    virtual ~IWebSocketHandler() = default;

    [[nodiscard]] virtual themis::Result<WebSocketSession*> upgrade(
        std::string_view method,
        std::string_view path,
        const std::unordered_map<std::string, std::string>& headers,
        IWebSocketFrameCallback& callback) = 0;

    [[nodiscard]] virtual std::string_view handlerName() const noexcept = 0;
};

} // namespace api
} // namespace themis
