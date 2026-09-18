/**
 * @file chaos_admin_api_handler.h
 * @brief Admin HTTP handler for Chaos Engineering fault injection.
 *
 * Exposes `themis::chaos::ChaosScheduler` as production admin/staging HTTP
 * endpoints under `/admin/chaos/`. This header is the production consumer
 * route for the `chaos` module — previously the module had only test consumers.
 *
 * ### Availability gate
 * This handler is compiled only when `THEMIS_CHAOS_ADMIN=ON` is set at
 * CMake configure time. It must **never** be enabled on production deployments
 * without explicit operator approval; it is intended for development, staging,
 * and SRE-controlled chaos test environments only.
 *
 * ### Routes
 *  - `POST  /admin/chaos/inject`  — Inject a fault scenario.
 *  - `POST  /admin/chaos/reset`   — Clear all active fault injections.
 *  - `GET   /admin/chaos/status`  — Report active faults and counters.
 *  - `GET   /admin/chaos/history` — List recent fault injection events.
 *
 * ### Authentication
 * All routes require `admin` role in the ******; validated by
 * `AuthMiddleware`. Calls from non-admin tokens return HTTP 403.
 *
 * @version 1.0.0
 * @note Maturity: 🟡 INTEGRATION-READY (wiring to HttpServer pending)
 * @note Compile gate: `THEMIS_CHAOS_ADMIN`
 * @note Production consumer route for `src/chaos/` — see `src/chaos/ARCHITECTURE.md`
 */

#pragma once

#ifdef THEMIS_CHAOS_ADMIN

#include "server/auth_middleware.h"
#include "chaos/chaos_framework.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;

namespace themis::server {

class ChaosAdminApiHandler {
public:
    ChaosAdminApiHandler(
        std::shared_ptr<themis::AuthMiddleware>   auth,
        std::shared_ptr<themis::chaos::ChaosScheduler> sched);

    ~ChaosAdminApiHandler();

    // Non-copyable, non-movable.
    ChaosAdminApiHandler(const ChaosAdminApiHandler&)            = delete;
    ChaosAdminApiHandler& operator=(const ChaosAdminApiHandler&) = delete;

    /**
     * @brief Handle.
     * @param[in] req Input parameter.
     * @param[in] target Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handle(
        const http::request<http::string_body>& req,
        const std::string&                      target);

private:
    /**
     * @brief Handle Inject.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleInject(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle Reset.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleReset(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleStatus(
        const http::request<http::string_body>& req);
    /**
     * @brief Handle History.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHistory(
        const http::request<http::string_body>& req);

    std::shared_ptr<themis::AuthMiddleware>        auth_;
    std::shared_ptr<themis::chaos::ChaosScheduler> scheduler_;
};

}  // namespace themis::server

#endif  // THEMIS_CHAOS_ADMIN
