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

class WsChangeHandler {
public:
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

    explicit WsChangeHandler(std::shared_ptr<AuthMiddleware> auth,
                             Changefeed* changefeed = nullptr);

    /**
     * @brief Validate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    UpgradeDecision validate(const http::request<http::string_body>& req) const;

    /**
     * @brief Is Change Stream Path.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    static bool isChangeStreamPath(std::string_view path);

private:
    std::shared_ptr<AuthMiddleware> auth_;
    Changefeed*                     changefeed_;
};

} // namespace api
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
