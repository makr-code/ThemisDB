/**
 * @file ws_handler.h
 * @brief WebSocket upgrade and session management for GraphQL subscriptions.
 *
 * @details Manages WebSocket connection upgrade from HTTP, session lifecycle,
 * frame routing to subscription handlers, and graceful connection closure.
 *
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Production Ready
 */


#pragma once

#ifdef THEMIS_ENABLE_WEBSOCKET

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

// Forward declarations
namespace themis {
class AuthMiddleware;
class Changefeed;
} // namespace themis

namespace themis {
namespace api {

namespace beast = boost::beast;
namespace http  = beast::http;

/**
 * @brief WebSocket upgrade handler for the /v2/changes change-stream endpoint.
 *
 * Validates HTTP WebSocket upgrade requests before the WebSocket handshake
 * is accepted.  Responsibilities:
 *  1. Confirm the target URL is /v2/changes.
 *  2. Authenticate the caller using the Authorization header (Bearer token
 *     or JWT), requiring the "cdc:subscribe" scope via AuthMiddleware.
 *  3. Extract CDC filter parameters from the URL query string
 *     (from_sequence, key_prefix).
 *
 * This class intentionally contains no async I/O – it is a pure decision
 * maker that the HTTP session can call synchronously before transferring
 * ownership of the socket to a WebSocketSession.
 *
 * @see src/server/websocket_session.cpp  – actual WebSocket framing & CDC
 *      polling integration
 * @see FUTURE_ENHANCEMENTS.md            – design constraints and perf targets
 */
class WsChangeHandler {
public:
    /**
     * @brief Decision returned by validate().
     *
     * When should_upgrade is true the caller may proceed with the WebSocket
     * handshake.  The CDC subscription fields are populated and ready to be
     * forwarded to WebSocketSession::subscribeToCDC().
     *
     * When should_upgrade is false the caller should respond with an HTTP
     * error whose status is reject_status and whose body should contain
     * reject_reason.
     */
    struct UpgradeDecision {
        bool        should_upgrade = false;
        http::status reject_status = http::status::bad_request;
        std::string  reject_reason;

        // CDC subscription parameters – valid only when should_upgrade == true
        uint64_t    from_sequence = 0;
        std::string key_prefix;
        std::string user_id;
        std::string tenant_id;
    };

    /**
     * @brief Construct a handler.
     *
     * @param auth       AuthMiddleware used for Bearer/JWT validation.
     *                   May be nullptr; in that case authentication is
     *                   bypassed (intended for unit tests only).
     * @param changefeed Optional Changefeed instance (reserved for future
     *                   use, e.g. verifying collections exist).
     */
    explicit WsChangeHandler(std::shared_ptr<AuthMiddleware> auth,
                             Changefeed* changefeed = nullptr);

    /**
     * @brief Validate a WebSocket upgrade request for the /v2/changes path.
     *
     * Thread-safe: may be called from any thread; does not mutate state.
     *
     * @param req  The HTTP upgrade request to validate.
     * @return     UpgradeDecision with should_upgrade set accordingly.
     */
    UpgradeDecision validate(const http::request<http::string_body>& req) const;

    /**
     * @brief Return true if the given URL path targets the change-stream
     *        endpoint.
     *
     * Accepts /v2/changes (exact match on the path component, query string
     * is ignored by the caller before passing the path here).
     *
     * @param path URL path without query string.
     */
    static bool isChangeStreamPath(std::string_view path);

private:
    std::shared_ptr<AuthMiddleware> auth_;
    Changefeed*                     changefeed_;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
