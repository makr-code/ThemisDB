/**
 * @file replication_topology_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"
#include "sharding/replication_coordinator.h"
#include "sharding/wal_manager.h"

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace server {

class ReplicationTopologyApiHandler {
public:
    ReplicationTopologyApiHandler(
        std::shared_ptr<sharding::ReplicationCoordinator> coordinator,
        std::shared_ptr<sharding::WALManager>             wal_manager,
        std::string                                       primary_id,
        std::shared_ptr<AuthMiddleware>                   auth
    );

    /**
     * @brief Handle Topology Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTopologyGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Health Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealthGet(
        const http::request<http::string_body>& req);

    /**
     * @brief Handle Ui Get.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleUiGet(
        const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ReplicationCoordinator> coordinator_;
    std::shared_ptr<sharding::WALManager>             wal_manager_;
    std::string                                       primary_id_;
    std::shared_ptr<AuthMiddleware>                   auth_;

    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status          status,
        const std::string&    message,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] content_type Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status          status,
        const std::string&    body,
        const std::string&    content_type,
        const http::request<http::string_body>& req) const;

    /**
     * @brief Build Ui Html.
     * @param[in] api_base Input parameter.
     * @return Return value.
     */
    static std::string buildUiHtml(const std::string& api_base);
};

} // namespace server
} // namespace themis
