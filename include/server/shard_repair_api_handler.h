/**
 * @file shard_repair_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/auth_middleware.h"

#include <boost/beast/http.hpp>
#include <memory>
#include <string>

namespace beast = boost::beast;
namespace http = beast::http;

namespace themis {
namespace sharding {
class ShardRepairEngine;
}

namespace server {


class ShardRepairApiHandler {
public:
    ShardRepairApiHandler(
        std::shared_ptr<sharding::ShardRepairEngine> repair_engine,
        std::shared_ptr<themis::AuthMiddleware> auth);

    /**
     * @brief Set Repair Engine.
     * @param[in] repair_engine Input parameter.
     */
    void setRepairEngine(std::shared_ptr<sharding::ShardRepairEngine> repair_engine);

    /**
     * @brief Handle Health.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleHealth(const http::request<http::string_body>& req);
    /**
     * @brief Handle Trigger Repair.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTriggerRepair(const http::request<http::string_body>& req);
    /**
     * @brief Handle Trigger Full Scan.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTriggerFullScan(const http::request<http::string_body>& req);
    /**
     * @brief Handle Job Status.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleJobStatus(const http::request<http::string_body>& req);
    /**
     * @brief Handle Dashboard.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleDashboard(const http::request<http::string_body>& req);

private:
    std::shared_ptr<sharding::ShardRepairEngine> repair_engine_;
    std::shared_ptr<themis::AuthMiddleware> auth_;

    /**
     * @brief Check Auth.
     * @param[in] req Input parameter.
     * @param[in] required_scope Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool checkAuth(const http::request<http::string_body>& req,
                   const std::string& required_scope,
                   http::response<http::string_body>& out) const;

    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] content_type Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const std::string& body,
        const std::string& content_type,
        const http::request<http::string_body>& req) const;
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req) const;
};

} // namespace server
} // namespace themis